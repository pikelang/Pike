/*
 * Some pgsql utility functions.
 * They are kept here to avoid circular references.
 */

//! The pgsql backend, shared between all connection instances.
//! It runs even in non-callback mode in a separate thread and makes sure
//! that communication with the database is real-time and event driven
//! at all times.
//!
//! @note
//! Callbacks running from this backend directly determine the latency
//! in reacting to communication with the database server; so it
//! would be prudent not to block in these callbacks.

#pike __REAL_VERSION__
#pragma dynamic_dot
#require constant(Thread.Thread)

#include "pgsql.h"

#define PORTALINIT		0	// Portal states
#define PARSING	  		1
#define BOUND			2
#define COMMITTED		3
#define COPYINPROGRESS		4
#define CLOSING			5
#define CLOSED			6
#define PURGED			7

#define NOERROR			0	// Error states networkparser
#define PROTOCOLERROR		1
#define PROTOCOLUNSUPPORTED	2

#define LOSTERROR	"Database connection lost"

#if PG_DEADLOCK_SENTINEL
private multiset mutexes = set_weak_flag((<>), Pike.WEAK);
private int deadlockseq;

private class MUTEX {
  inherit Thread.Mutex;

  private Thread.Thread sentinelthread;

  private void dump_lockgraph(Thread.Thread curthread) {
    sleep(PG_DEADLOCK_SENTINEL);
    String.Buffer buf = String.Buffer();
    buf.add("\n{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{\n");
    buf.sprintf("Deadlock detected at\n%s\n",
     describe_backtrace(curthread.backtrace(), -1));
    foreach(mutexes; this_program mu; )
      if (mu) {
        Thread.Thread ct;
        if (ct = mu.current_locking_thread())
        buf.sprintf("====================================== Mutex: %O\n%s\n",
         mu, describe_backtrace(ct.backtrace(), -1));
      }
    buf.add("}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}");
    werror(replace(buf.get(), "\n", sprintf("\n%d> ", deadlockseq++)) + "\n");
  }

  Thread.MutexKey lock(void|int type) {
    Thread.MutexKey key;
    if (!(key = trylock(type))) {
      sentinelthread = Thread.Thread(dump_lockgraph, this_thread());
      key = ::lock(type);
      sentinelthread->kill();
    }
    return key;
  }

  protected void create() {
    //::create();
    mutexes[this] = 1;
  }
};
#else
#define MUTEX			Thread.Mutex
#endif

//! The instance of the pgsql dedicated backend.
final Pike.Backend local_backend;

private Pike.Backend cb_backend;
private Result qalreadyprinted;
private Thread.Mutex backendmux = Thread.Mutex();
private Thread.ResourceCount clientsregistered = Thread.ResourceCount();

constant emptyarray = ({});
constant describenodata
 = (["datarowdesc":emptyarray, "datarowtypes":emptyarray,
     "datatypeoid":emptyarray]);
private constant censoroptions = (<"use_ssl", "force_ssl",
 "cache_autoprepared_statements", "reconnect", "text_query", "is_superuser",
 "server_encoding", "server_version", "integer_datetimes",
 "session_authorization">);

 /* Statements matching createprefix cause the prepared statement cache
  * to be flushed to prevent stale references to (temporary) tables
  */
final Regexp createprefix = iregexp("^\a*(CREATE|DROP)\a");

 /* Statements matching dontcacheprefix never enter the cache
  */
private Regexp dontcacheprefix = iregexp("^\a*(FETCH|COPY)\a");

 /* Statements not matching paralleliseprefix will cause the driver
  * to stall submission until all previously started statements have
  * run to completion
  */
private Regexp paralleliseprefix
 = iregexp("^\a*((SELEC|INSER)T|(UPDA|DELE)TE|(FETC|WIT)H)\a");

 /* Statements matching transbeginprefix will cause the driver
  * insert a sync after the statement.
  * Failure to do so, will result in portal synchronisation errors
  * in the event of an ErrorResponse.
  */
final Regexp transbeginprefix
 = iregexp("^\a*(BEGIN|START)([; \t\f\r\n]|$)");

 /* Statements matching transendprefix will cause the driver
  * insert a sync after the statement.
  * Failure to do so, will result in portal synchronisation errors
  * in the event of an ErrorResponse.
  */
final Regexp transendprefix
 = iregexp("^\a*(COMMIT|ROLLBACK|END)([; \t\f\r\n]|$)");

 /* For statements matching execfetchlimit the resultrows will not be
  * fetched in pieces.  This heuristic will be sub-optimal whenever
  * either an UPDATE/DELETE/INSERT statement is prefixed by WITH, or
  * if there is a RETURNING with a *lot* of results.  In those cases
  * the portal will be busy until all results have been fetched, and will
  * not be able to deliver results belonging to other parallel queries
  * running on the same filedescriptor.
  *
  * However, considering that the current heuristic increases query-speed
  * in the majority of the real-world cases, it would be considered a good
  * tradeoff.
  */
private Regexp execfetchlimit
 = iregexp("^\a*((UPDA|DELE)TE|INSERT)\a|\aLIMIT\a+[1-9][; \t\f\r\n]*$");

private void default_backend_runs() {		// Runs as soon as the
  cb_backend = Pike.DefaultBackend;		// DefaultBackend has started
}

private void create() {
  // Run callbacks from our local_backend until DefaultBackend has started
  cb_backend = local_backend = Pike.SmallBackend();
  call_out(default_backend_runs, 0);
}

private Regexp iregexp(string expr) {
  Stdio.Buffer ret = Stdio.Buffer();
  foreach (expr; ; int c)
    if (c >= 'A' && c <= 'Z')
      ret->add('[', c, c + 'a' - 'A', ']');
    else if (c == '\a')			// Replace with generic whitespace
      ret->add("[ \t\f\r\n]");
    else
      ret->add_int8(c);
  return Regexp(ret->read());
}

final void closestatement(bufcon|conxsess plugbuffer, string oldprep) {
  if (oldprep) {
    PD("Close statement %s\n", oldprep);
    CHAIN(plugbuffer)->add_int8('C')->add_hstring(({'S', oldprep, 0}), 4, 4);
  }
}

private void run_local_backend() {
  Thread.MutexKey lock;
  int looponce;
  do {
    looponce = 0;
    if (lock = backendmux->trylock()) {
      PD("Starting local backend\n");
      while (!clientsregistered->drained()	// Autoterminate when not needed
       || sizeof(local_backend->call_out_info())) {
        mixed err;
        if (err = catch(local_backend(4096.0)))
          master()->handle_error(err);
      }
      PD("Terminating local backend\n");
      lock = 0;
      looponce = !clientsregistered->drained();
    }
  } while (looponce);
}

//! Registers yourself as a user of this backend.  If the backend
//! has not been started yet, it will be spawned automatically.
final Thread.ResourceCountKey register_backend() {
  int startbackend = clientsregistered->drained();
  Thread.ResourceCountKey key = clientsregistered->acquire();
  if (startbackend)
    Thread.Thread(run_local_backend);
  return key;
}

final void throwdelayederror(Result|proxy parent) {
  if (mixed err = parent->delayederror) {
    if (!objectp(parent->pgsqlsess))
      parent->untolderror = 0;
    else if (parent->pgsqlsess)
      parent->pgsqlsess->untolderror = 0;
    parent.delayederror = 0;
    if (stringp(err))
      err = ({err, backtrace()[..<2]});
    throw(err);
  }
}

private int readoidformat(int oid) {
  switch (oid) {
    case BOOLOID:
    case BYTEAOID:
    case CHAROID:
    case INT8OID:
    case INT2OID:
    case INT4OID:
    case FLOAT4OID:
#if !constant(__builtin.__SINGLE_PRECISION_FLOAT__)
    case FLOAT8OID:
#endif
    case NUMERICOID:
    case TEXTOID:
    case OIDOID:
    case XMLOID:
    case DATEOID:
    case TIMEOID:
    case TIMETZOID:
    case TIMESTAMPOID:
    case TIMESTAMPTZOID:
    case INTERVALOID:
    case INT4RANGEOID:
    case INT8RANGEOID:
    case DATERANGEOID:
    case TSRANGEOID:
    case TSTZRANGEOID:
    case MACADDROID:
    case BPCHAROID:
    case VARCHAROID:
    case CIDROID:
    case INETOID:
    case CTIDOID:
    case UUIDOID:
      return 1; //binary
  }
  return 0;	// text
}

private int writeoidformat(int oid, array(string|int) paramValues,
 array(int) ai) {
  mixed value = paramValues[ai[0]++];
  switch (oid) {
    case BOOLOID:
    case BYTEAOID:
    case CHAROID:
    case INT8OID:
    case INT2OID:
    case INT4OID:
    case TEXTOID:
    case OIDOID:
    case XMLOID:
    case MACADDROID:
    case BPCHAROID:
    case VARCHAROID:
    case CTIDOID:
    case UUIDOID:
      return 1; //binary
    case NUMERICOID:
    case CIDROID:
    case INETOID:
    case DATEOID:
    case TIMEOID:
    case TIMETZOID:
    case TIMESTAMPOID:
    case TIMESTAMPTZOID:
    case INTERVALOID:
    case INT4RANGEOID:
    case INT8RANGEOID:
    case DATERANGEOID:
    case TSRANGEOID:
    case TSTZRANGEOID:
    case FLOAT4OID:
#if !constant(__builtin.__SINGLE_PRECISION_FLOAT__)
    case FLOAT8OID:
#endif
      if (!stringp(value))
        return 1;
  }
  return 0;	// text
}

#define DAYSEPOCHTO2000		10957		// 2000/01/01 00:00:00 UTC
#define USEPOCHTO2000		(DAYSEPOCHTO2000*24*3600*1000000)

private array timestamptotype
                    = ({Val.Timestamp, 8, USEPOCHTO2000,  "usecs", 8});
private array datetotype = ({Val.Date, 4, DAYSEPOCHTO2000, "days", 4});

private mapping(int:array) oidtotype = ([
   DATEOID:        datetotype,
   TIMEOID:        ({Val.Time,    8, 0, "usecs", 8}),
   TIMETZOID:      ({Val.TimeTZ, 12, 0, "usecs", 8, "timezone", 4}),
   INTERVALOID:    ({Val.Interval, 16, 0, "usecs", 8, "days", 4, "months",4}),
   TIMESTAMPOID:   timestamptotype,
   TIMESTAMPTZOID: timestamptotype,
   INT4RANGEOID:   ({0, 4}),
   INT8RANGEOID:   ({0, 8}),
   DATERANGEOID:   datetotype,
   TSRANGEOID:     timestamptotype,
   TSTZRANGEOID:   timestamptotype,
 ]);

private inline mixed callout(function(mixed ...:void) f,
 float|int delay, mixed ... args) {
  return cb_backend->call_out(f, delay, @args);
}

// Some pgsql utility functions

class bufcon {
  inherit Stdio.Buffer;
  private Thread.ResourceCountKey dirty;

#ifdef PG_DEBUGRACE
  final bufcon `chain() {
    return this;
  }
#endif

  private conxion realbuffer;

  private void create(conxion _realbuffer) {
    realbuffer = _realbuffer;
  }

  final Thread.ResourceCount `stashcount() {
    return realbuffer->stashcount;
  }

  final bufcon start(void|int waitforreal) {
    dirty = realbuffer->stashcount->acquire();
#ifdef PG_DEBUG
    if (waitforreal)
      error("pgsql.bufcon not allowed here\n");
#endif
    return this;
  }

  final void sendcmd(int mode, void|Result portal) {
    Thread.MutexKey lock = realbuffer->shortmux->lock();
    if (portal)
      realbuffer->stashqueue->write(portal);
    if (mode == SYNCSEND) {
      add(PGSYNC);
      realbuffer->stashqueue->write(1);
      mode = SENDOUT;	    // Demote it to prevent an extra SYNC upon stashflush
    }
    realbuffer->stash->add(this);
    PD("%d>Stashed mode %d > %d\n",
     realbuffer->socket->query_fd(), mode, realbuffer->stashflushmode);
    if (mode > realbuffer->stashflushmode)
      realbuffer->stashflushmode = mode;
    lock = 0;
    dirty = 0;
    this->clear();
    if (lock = realbuffer->nostash->trylock(1)) {
#ifdef PG_DEBUGRACE
      conxsess sess = conxsess(realbuffer);
      realbuffer->started = lock;
      lock = 0;
      sess->sendcmd(SENDOUT);
#else
      realbuffer->started = lock;
      lock = 0;
      realbuffer->sendcmd(SENDOUT);
#endif
    }
  }
};

class conxiin {
  inherit Stdio.Buffer:i;

  final Thread.Condition fillread;
  final MUTEX fillreadmux;
  final int procmsg;
  private int didreadcb;

#if PG_DEBUGHISTORY > 0
  final array history = ({});

  final int(-1..) input_from(Stdio.Stream stm, void|int(0..) nbytes) {
    int oldsize = sizeof(this);
    int ret = i::input_from(stm, nbytes);
    if (ret) {
      Stdio.Buffer tb = Stdio.Buffer(this);
      tb->consume(oldsize);
      history += ({"<<"+tb->read(ret)});
      history = history[<PG_DEBUGHISTORY - 1 ..];
    }
    return ret;
  }
#endif

  protected final bool range_error(int howmuch) {
#ifdef PG_DEBUG
    if (howmuch < 0) {
      int available = unread(0);
      unread(available);
      error("Out of range %d %O %O\n", howmuch,
       ((string)this)[.. available-1], ((string)this)[available ..]);
    }
#endif
    if (fillread) {
      Thread.MutexKey lock = fillreadmux->lock();
      if (!didreadcb)
        fillread.wait(lock);
      didreadcb = 0;
    } else
      throw(MAGICTERMINATE);
    return true;
  }

  final int read_cb(mixed id, mixed b) {
    PD("Read callback %O\n", b && ((string)b)
#ifndef PG_DEBUGMORE
      [..255]
#endif
     );
    Thread.MutexKey lock = fillreadmux->lock();
    if (procmsg && id)
      procmsg = 0, lock = 0, Thread.Thread(id);
    else if (fillread)
      didreadcb = 1, fillread.signal();
    return 0;
  }

  private void create() {
    i::create();
    fillreadmux = MUTEX();
    fillread = Thread.Condition();
  }
};

class sfile {
  inherit Stdio.File;
  final int query_fd() {
    return is_open() ? ::query_fd() : -1;
  }
};

class conxion {
  inherit Stdio.Buffer:o;
  final conxiin i;

  private Thread.Queue qportals;
  final MUTEX shortmux;
  private int closenext;

  final sfile
#if constant(SSL.File)
             |SSL.File
#endif
                       socket;
  private int towrite;
  final multiset(Result) runningportals = (<>);

  final MUTEX nostash;
  final Thread.MutexKey started;
  final Thread.Queue stashqueue;
  final Thread.Condition stashavail;
  final Stdio.Buffer stash;
  //! @ignore
  final int(KEEP..SYNCSEND) stashflushmode;
  //! @endignore
  final Thread.ResourceCount stashcount;
  final int synctransact;
#ifdef PG_DEBUGRACE
  final mixed nostrack;
#endif
#ifdef PG_DEBUG
  final int queueoutidx;
  final int queueinidx = -1;
#endif

#if PG_DEBUGHISTORY > 0
  final int(-1..) output_to(Stdio.Stream stm, void|int(0..) nbytes) {
    Stdio.Buffer tb = Stdio.Buffer(this);
    int ret = o::output_to(stm, nbytes);
    if (ret) {
      i->history += ({">>" + tb->read(ret)});
      i->history = i->history[<PG_DEBUGHISTORY - 1 ..];
    }
    return ret;
  }
#endif

  private inline void queueup(Result portal) {
    qportals->write(portal); portal->_synctransact = synctransact;
    PD("%d>%O %d %d Queue portal %d bytes\n", socket->query_fd(),
     portal._portalname, ++queueoutidx, synctransact, sizeof(this));
  }

  final bufcon|conxsess start(void|int waitforreal) {
    Thread.MutexKey lock;
#ifdef PG_DEBUGRACE
    if (nostash->current_locking_thread())
      PD("Nostash locked by %s\n",
       describe_backtrace(nostash->current_locking_thread()->backtrace()));
#endif
    while (lock = (waitforreal ? nostash->lock : nostash->trylock)(1)) {
      int mode;
      if (sizeof(stash) && (mode = getstash(KEEP)) > KEEP)
        sendcmd(mode);		// Force out stash to the server
      if (!stashcount->drained()) {
        lock = 0;				// Unlock while we wait
        stashcount->wait_till_drained();
        continue;				// Try again
      }
#ifdef PG_DEBUGRACE
      conxsess sess = conxsess(this);
#endif
      started = lock;		// sendcmd() clears started, so delay assignment
#ifdef PG_DEBUGRACE
      return sess;
#else
      return this;
#endif
    }
    return bufcon(this)->start();
  }

  private int write_cb() {
    Thread.MutexKey lock = shortmux->lock();
    if (this) {				// Guard against async destructs
      towrite -= output_to(socket, towrite);
      lock = 0;
      if (!i->fillread && !sizeof(this))
        close();
    }
    return 0;
  }

  private int getstash(int mode) {
    Thread.MutexKey lock = shortmux->lock();
    add(stash); stash->clear();
    foreach (stashqueue->try_read_array(); ; int|Result portal)
      if (intp(portal))
        qportals->write(synctransact++);
      else
        queueup(portal);
    PD("%d>Got stash mode %d > %d\n",
     socket->query_fd(), stashflushmode, mode);
    if (stashflushmode > mode)
      mode = stashflushmode;
    stashflushmode = KEEP;
    return mode;
  }

  final void sendcmd(void|int mode, void|Result portal) {
    if (portal)
      queueup(portal);
unfinalised:
    do {
      switch (mode) {
        default:
          break unfinalised;
        case SYNCSEND:
          PD("%d>Sync %d %d Queue\n",
           socket->query_fd(), synctransact, ++queueoutidx);
          add(PGSYNC);
          mode = SENDOUT;
          break;
        case FLUSHLOGSEND:
          PD("%d>%O %d Queue simplequery %d bytes\n", socket->query_fd(),
           portal._portalname, ++queueoutidx, sizeof(this));
          mode = FLUSHSEND;
      }
      qportals->write(synctransact++);
    } while (0);
    if (sizeof(stash))
      mode = getstash(mode);
#ifdef PG_DEBUG
    mixed err;
#endif
    for(;;) {
#ifdef PG_DEBUG
      err =
#endif
      catch {
outer:
        do {
          switch (mode) {
            default:
              PD("%d>Skip flush %d Queue %O\n",
               socket->query_fd(), mode, (string)this);
              break outer;
            case FLUSHSEND:
              PD("Flush\n");
              add(PGFLUSH);
            case SENDOUT:;
          }
          if (towrite = sizeof(this)) {
            PD("%d>Sendcmd %O\n",
             socket->query_fd(), ((string)this)[..towrite-1]);
            towrite -= output_to(socket, towrite);
          }
        } while (0);
        started = 0;
        if (sizeof(stash) && (started = nostash->trylock(2))) {
#ifdef PG_DEBUGRACE
          conxsess sess = conxsess(this);
          sess->sendcmd(SENDOUT);
#else
          mode = getstash(SENDOUT);
          continue;
#endif
        }
        return;
      };
      break;
    }
    started = 0;
    PD("Sendcmd failed %s\n", describe_backtrace(err));
    destruct(this);
  }

  final int close() {
    if (!closenext && nostash) {
      closenext = 1;
      {
        Thread.MutexKey lock = i->fillreadmux->lock();
        if (i->fillread) {  // Delayed close() after flushing the output buffer
          i->fillread.signal();
          i->fillread = 0;
        }
      }
      PD("%d>Delayed close, flush write\n", socket->query_fd());
      i->read_cb(socket->query_id(), 0);
      return 0;
    } else
      return -1;
  }

  final void purge() {
    if (stashcount) {
      stashcount = 0;
      PD("%d>Purge conxion %d\n", socket ? socket->query_fd() : -1, !!nostash);
      int|Result portal;
      if (qportals)			// CancelRequest does not use qportals
        while (portal = qportals->try_read())
          if (objectp(portal))
            portal->_purgeportal();
      if (nostash) {
        while (sizeof(runningportals))
          catch {
            foreach (runningportals; Result result; )
              if (!result.datarowtypes) {
                result.datarowtypes = emptyarray;
                if (result._state != PURGED && !result.delayederror)
                  result.delayederror = LOSTERROR;
                result._ddescribe->broadcast();
                runningportals[result] = 0;
              } else
                destruct(result);
          };
        destruct(nostash);
        socket->set_non_blocking();			// Drop all callbacks
        PD("%d>Close socket\n", socket->query_fd());
        socket->close();		// This will be an asynchronous close
      }
      destruct(this);
    }
  }

  private void _destruct() {
    PD("%d>Close conxion %d\n", socket ? socket->query_fd() : -1, !!nostash);
    catch(purge());
  }

  final void connectloop(proxy pgsqlsess, int nossl) {
#ifdef PG_DEBUG
    mixed err =
#endif
    catch {
      for (; ; clear()) {
        socket->connect(pgsqlsess. host, pgsqlsess. port);
#if constant(SSL.File)
        if (!nossl && !pgsqlsess->nossl
         && (pgsqlsess.options.use_ssl || pgsqlsess.options.force_ssl)) {
          PD("SSLRequest\n");
          start()->add_int32(8)->add_int32(PG_PROTOCOL(1234, 5679))
           ->sendcmd(SENDOUT);
          string s = socket.read(1);
          switch (sizeof(s) && s[0]) {
            case 'S':
              SSL.File fcon = SSL.File(socket, SSL.Context());
              if (fcon->connect()) {
                socket->set_backend(local_backend);
                socket = fcon;
                break;
              }
            default:
              PD("%d>Close socket short\n", socket->query_fd());
              socket->close();
              pgsqlsess.nossl = 1;
              continue;
            case 'N':
              if (pgsqlsess.options.force_ssl)
                error("Encryption not supported on connection to %s:%d\n",
                      pgsqlsess.host, pgsqlsess.port);
          }
        }
#else
        if (pgsqlsess.options.force_ssl)
          error("Encryption library missing,"
                " cannot establish connection to %s:%d\n",
                pgsqlsess.host, pgsqlsess.port);
#endif
        break;
      }
      if (!socket->is_open())
        error(strerror(socket->errno()) + ".\n");
      socket->set_backend(local_backend);
      socket->set_buffer_mode(i, 0);
      socket->set_nonblocking(i->read_cb, write_cb, close);
      if (nossl != 2)
        Thread.Thread(pgsqlsess->processloop, this);
      return;
    };
    PD("Connect error %s\n", describe_backtrace(err));
    catch(destruct(pgsqlsess->waitforauthready));
    destruct(this);
  }

  private string _sprintf(int type) {
    string res;
    switch (type) {
      case 'O':
        int fd = -1;
        if (socket)
          catch(fd = socket->query_fd());
        res = predef::sprintf("conxion  fd: %d input queue: %d/%d "
                    "queued portals: %d  output queue: %d/%d\n"
                    "started: %d\n",
                    fd, sizeof(i), i->_size_object(),
                    qportals && qportals->size(), sizeof(this), _size_object(),
                    !!started);
        break;
    }
    return res;
  }

  private void create(proxy pgsqlsess, Thread.Queue _qportals, int nossl) {
    o::create();
    qportals = _qportals;
    synctransact = 1;
    socket = sfile();
    i = conxiin();
    shortmux = MUTEX();
    nostash = MUTEX();
    closenext = 0;
    stashavail = Thread.Condition();
    stashqueue = Thread.Queue();
    stash = Stdio.Buffer();
    stashcount = Thread.ResourceCount();
    Thread.Thread(connectloop, pgsqlsess, nossl);
  }
};

#ifdef PG_DEBUGRACE
class conxsess {
  final conxion chain;

  private void create(conxion parent) {
    if (parent->started)
      werror("Overwriting conxsess %s %s\n",
        describe_backtrace(({"new ", backtrace()[..<1]})),
        describe_backtrace(({"old ", parent->nostrack})));
    parent->nostrack = backtrace();
    chain = parent;
  }

  final void sendcmd(int mode, void|Result portal) {
    chain->sendcmd(mode, portal);
    chain = 0;
  }

  private void _destruct() {
    if (chain)
      werror("Untransmitted conxsess %s\n",
       describe_backtrace(({"", backtrace()[..<1]})));
  }
};
#endif

//! The result object returned by @[Sql.pgsql()->big_query()], except for
//! the noted differences it behaves the same as @[Sql.Result].
//!
//! @seealso
//!   @[Sql.Result], @[Sql.pgsql], @[Sql.Sql], @[Sql.pgsql()->big_query()]
class Result {

  inherit __builtin.Sql.Result;

  private proxy pgsqlsess;
  private int(0..1) eoffound;
  private conxion c;
  private conxiin cr;
  final int(0..1) untolderror;
  final mixed delayederror;
  //! @ignore
  final int(PORTALINIT..PURGED) _state;
  //! @endignore
  final int _fetchlimit;
  private int(0..1) alltext;
  final int(0..1) _forcetext;
  private int syncparse;
  private int transtype;

  final string _portalname;

  private int inflight;
  int portalbuffersize;
  private MUTEX closemux;
  private Thread.Queue datarows;
  private Thread.ResourceCountKey stmtifkey, portalsifkey;
  private array(mapping(string:mixed)) datarowdesc;
  final array(int) datarowtypes;	// types from datarowdesc
  private string statuscmdcomplete;
  private int bytesreceived;
  final int _synctransact;
  final Thread.Condition _ddescribe;
  final MUTEX _ddescribemux;
  final Thread.MutexKey _unnamedportalkey, _unnamedstatementkey;
  final array _params;
  final string _query;
  final string _preparedname;
  final mapping(string:mixed) _tprepared;
  private function(:void) gottimeout;
  private int timeout;

  protected string _sprintf(int type) {
    string res;
    switch (type) {
      case 'O':
        int fd = -1;
        if (c && c->socket)
          catch(fd = c->socket->query_fd());
        res = sprintf(
                      "Result state: %d numrows: %d eof: %d inflight: %d\n"
                      "fd: %O portalname: %O  coltypes: %d"
                      "  synctransact: %d laststatus: %s\n"
                      "stash: %O\n"
#if PG_DEBUGHISTORY > 0
                      "history: %O\n"
#endif
                      "query: %O\n%s\n",
                      _state, index, eoffound, inflight,
		      fd, _portalname,
                      datarowtypes && sizeof(datarowtypes), _synctransact,
                      statuscmdcomplete
                       || (_unnamedstatementkey ? "*parsing*" : ""),
                      qalreadyprinted == this ? 0
                                              : c && (string)c->stash,
#if PG_DEBUGHISTORY > 0
                      qalreadyprinted == this ? 0 : c && c->i->history,
#endif
                      qalreadyprinted == this
                       ? "..." : replace(_query, "xxxx\n", "\\n"),
                      qalreadyprinted == this ? "..." : _showbindings()*"\n");
        qalreadyprinted = this;
        break;
    }
    return res;
  }

  protected void create(proxy _pgsqlsess, conxion _c, string query,
   int _portalbuffersize, int alltyped, array params, int forcetext,
   int _timeout, int _syncparse, int _transtype) {
    pgsqlsess = _pgsqlsess;
    if (c = _c)
      cr = c->i;
    else
      losterror();
    _query = query;
    datarows = Thread.Queue();
    _ddescribe = Thread.Condition();
    _ddescribemux = MUTEX();
    closemux = MUTEX();
    portalbuffersize = _portalbuffersize;
    alltext = !alltyped;
    _params = params;
    _forcetext = forcetext;
    _state = PORTALINIT;
    timeout = _timeout;
    syncparse = _syncparse;
    gottimeout = _pgsqlsess->cancelquery;
    c->runningportals[this] = 1;
    transtype = _transtype;
  }

  final array(string) _showbindings() {
    array(string) msgs = emptyarray;
    if (_params) {
      array from, to, paramValues;
      [from, to, paramValues] = _params;
      if (sizeof(paramValues)) {
        int i;
        string val, fmt = sprintf("%%%ds %%3s %%.61s", max(@map(from, sizeof)));
        foreach (paramValues; i; val)
          msgs += ({sprintf(fmt, from[i], to[i], sprintf("%O", val))});
      }
    }
    return msgs;
  }

  //! Returns the command-complete status for this query.
  //!
  //! @seealso
  //!  @[Sql.Result()->status_command_complete()]
  /*semi*/final string status_command_complete() {
    if (!statuscmdcomplete) {
      if (!datarowtypes)
        waitfordescribe();
      {
        Thread.MutexKey lock = closemux->lock();
        if (_fetchlimit) {
          array reflock = ({ lock });
          lock = 0;
          _sendexecute(_fetchlimit = 0, reflock);
        } else
          lock = 0;			// Force release before acquiring next
        lock = _ddescribemux->lock();
        if (!statuscmdcomplete)
          PT(_ddescribe->wait(lock));
      }
      if (this)		// If object already destructed, skip the next call
        trydelayederror();	// since you cannot call functions anymore
      else
        error(LOSTERROR);
    }
    return statuscmdcomplete;
  }

  //! Returns the number of affected rows by this query.
  //!
  //! @seealso
  //!  @[Sql.Result()->affected_rows()]
  /*semi*/final int affected_rows() {
    int rows;
    sscanf(status_command_complete() || "", "%*s %d %d", rows, rows);
    return rows;
  }

  final void _storetiming() {
    if (_tprepared) {
      _tprepared.trun = gethrtime() - _tprepared.trunstart;
      m_delete(_tprepared, "trunstart");
      _tprepared = 0;
    }
  }

  private void waitfordescribe() {
    {
      Thread.MutexKey lock = _ddescribemux->lock();
      if (!datarowtypes)
        PT(_ddescribe->wait(lock));
    }
    if (this)		// If object already destructed, skip the next call
      trydelayederror();	// since you cannot call functions anymore
    else
      error(LOSTERROR);
  }

  //! @seealso
  //!  @[Sql.Result()->num_fields()]
  /*semi*/final int num_fields() {
    if (!datarowtypes)
      waitfordescribe();
    return sizeof(datarowtypes);
  }

  //! @note
  //!  This method returns the number of rows already received from
  //!  the database for the current query.  Note that this number
  //!  can still increase between subsequent calls if the results from
  //!  the query are not complete yet.  This function is only guaranteed to
  //!  return the correct count after EOF has been reached.
  //! @seealso
  //!  @[Sql.Result()->num_rows()]
  /*semi*/final int num_rows() {
    trydelayederror();
    return index;
  }

  private void losterror() {
    string err;
    if (pgsqlsess)
      err = pgsqlsess->geterror(1);
    error("%s\n", err || LOSTERROR);
  }

  private void trydelayederror() {
    if (delayederror)
      throwdelayederror(this);
    else if (_state == PURGED)
      losterror();
  }

  //! @seealso
  //!  @[Sql.Result()->eof()]
  /*semi*/final int eof() {
    trydelayederror();
    return eoffound;
  }

  //! @seealso
  //!  @[Sql.Result()->fetch_fields()]
  /*semi*/final array(mapping(string:mixed)) fetch_fields() {
    if (!datarowtypes)
      waitfordescribe();
    if (!datarowdesc)
      error(LOSTERROR);
    return datarowdesc + emptyarray;
  }

#ifdef PG_DEBUG
#define INTVOID int
#else
#define INTVOID void
#endif
  final INTVOID _decodedata(int msglen, string cenc) {
    _storetiming(); _releasestatement();
    string serror;
    bytesreceived += msglen;
    int cols = cr->read_int16();
    array a = allocate(cols, !alltext && Val.null);
#ifdef PG_DEBUG
    msglen -= 2 + 4 * cols;
#endif
    foreach (datarowtypes; int i; int typ) {
      int collen = cr->read_sint(4);
      if (collen > 0) {
#ifdef PG_DEBUG
        msglen -= collen;
#endif
        mixed value;
        switch (typ) {
          case FLOAT4OID:
#if !constant(__builtin.__SINGLE_PRECISION_FLOAT__)
          case FLOAT8OID:
#endif
            if (_forcetext) {
              if (!alltext) {
                value = (float)cr->read(collen);
                break;
              }
            } else {
              [ value ] = cr->sscanf(collen == 4 ? "%4F" : "%8F");
              if (alltext)
                value = sprintf("%.*g", collen == 4 ? 9 : 17, value);
              break;
            }
          default:value = cr->read(collen);
            break;
          case CHAROID:
            value = alltext ? cr->read(1) : cr->read_int8();
            break;
          case BOOLOID:value = cr->read_int8();
            switch (value) {
              case 'f':value = 0;
                break;
              case 't':value = 1;
            }
            if (alltext)
              value = value ? "t" : "f";
            break;
          case TEXTOID:
          case BPCHAROID:
          case VARCHAROID:
            value = cr->read(collen);
            if (cenc == UTF8CHARSET && catch(value = utf8_to_string(value))
             && !serror)
              serror = SERROR("%O contains non-%s characters\n",
                                                     value, UTF8CHARSET);
            break;
          case NUMERICOID:
            if (_forcetext) {
              value = cr->read(collen);
              if (!alltext) {
                value = value/".";
                if (sizeof(value) == 1)
                  value = (int)value[0];
                else {
                  int i, denom;
                  for (i = sizeof(value[1]), denom = 1; --i >= 0; denom *= 10);
                  i = (int)value[0];
                  value = (int)value[1];
                  value = Gmp.mpq(i * denom + (i >= 0 ? value : -value),
                   denom);
                }
              }
            } else {
              int nwords = cr->read_int16();
              int magnitude = cr->read_sint(2);
              int sign = cr->read_int16();
              cr->consume(2);
              if (nwords) {
                for (value = cr->read_int16(); --nwords; magnitude--)
                  value = value * NUMERIC_MAGSTEP + cr->read_int16();
                if (sign)
                  value = -value;
                if (magnitude > 0)
                  do
                    value *= NUMERIC_MAGSTEP;
                  while (--magnitude);
                else if (magnitude < 0) {
                  for (sign = NUMERIC_MAGSTEP;
                       ++magnitude;
                       sign *= NUMERIC_MAGSTEP);
                  value = Gmp.mpq(value, sign);
                }
              } else
                value = 0;
              if (alltext)
                value = (string)value;
            }
            break;
          case INT4RANGEOID:
          case INT8RANGEOID:
          case DATERANGEOID:
          case TSRANGEOID:
          case TSTZRANGEOID:
            if (_forcetext)
              value = cr->read(collen);
            else {
              array totype = oidtotype[typ];
              mixed from = -Math.inf, till = Math.inf;
              switch (cr->read_int8()) {
                case 1: from = till = 0;
                  break;
                case 0x12: from = cr->read_sint(cr->read_int32());
                  break;
                case 2: from = cr->read_sint(cr->read_int32());
                case 8: till = cr->read_sint(cr->read_int32());
              }
              if (totype[0]) {
                if (intp(from)) {
                  value = totype[0]();
                  value[totype[3]] = from + totype[2];
                  from = value;
                }
                if (intp(till)) {
                  value = totype[0]();
                  value[totype[3]] = till + totype[2];
                  till = value;
                }
              }
              value = Val.Range(from, till);
              if (alltext)
                value = value->sql();
            }
            break;
          case CIDROID:
          case INETOID:
            if (_forcetext)
              value = cr->read(collen);
            else {
              value = Val.Inet();
              int iptype = cr->read_int8();	// 2 == IPv4, 3 == IPv6
              value->masklen = cr->read_int8() + (iptype == 2 && 12*8);
              cr->read_int8();	// 0 == INET, 1 == CIDR
              value->address = cr->read_hint(1);
              if (alltext)
                value = (string)value;
            }
            break;
          case TIMESTAMPOID:
          case TIMESTAMPTZOID:
          case INTERVALOID:
          case TIMETZOID:
          case TIMEOID:
          case DATEOID:
            if (_forcetext)
              value = cr->read(collen);
            else {
              array totype = oidtotype[typ];
              value = totype[0]();
              value[totype[3]] = cr->read_sint(totype[4]) + totype[2];
              int i = 5;
              while (i < sizeof(totype)) {
                value[totype[i]] = cr->read_sint(totype[i+1]);
                i += 2;
              }
              if (alltext)
                value = (string)value;
            }
            break;
          case INT8OID:case INT2OID:
          case OIDOID:case INT4OID:
            if (_forcetext) {
              value = cr->read(collen);
              if (!alltext)
                value = (int)value;
            } else {
              switch (typ) {
                case INT8OID:value = cr->read_sint(8);
                  break;
                case INT2OID:value = cr->read_sint(2);
                  break;
                case OIDOID:
                case INT4OID:value = cr->read_sint(4);
              }
              if (alltext)
                value = (string)value;
            }
        }
        a[i]=value;
      } else if (!collen)
        a[i]="";
    }
    _processdataready(a);
    if (serror)
      error(serror);
#ifdef PG_DEBUG
    return msglen;
#endif
  }

  final void _setrowdesc(array(mapping(string:mixed)) drowdesc,
   array(int) drowtypes) {
    Thread.MutexKey lock = _ddescribemux->lock();
    datarowdesc = drowdesc;
    datarowtypes = drowtypes;
    _ddescribe->broadcast();
  }

  final void _preparebind(array dtoid) {
    array(string|int) paramValues = _params ? _params[2] : emptyarray;
    if (sizeof(dtoid) != sizeof(paramValues))
      SUSERERROR("Invalid number of bindings, expected %d, got %d\n",
                 sizeof(dtoid), sizeof(paramValues));
    Thread.MutexKey lock = _ddescribemux->lock();
    if (!_portalname) {
      _portalname
       = (_unnamedportalkey = pgsqlsess.unnamedportalmux->trylock(1))
         ? "" : PORTALPREFIX
#ifdef PG_DEBUG
          + (string)(c->socket->query_fd()) + "_"
#endif
          + String.int2hex(pgsqlsess.pportalcount++);
      lock = 0;
#ifdef PG_DEBUGMORE
      PD("ParamValues to bind: %O\n", paramValues);
#endif
      Stdio.Buffer plugbuffer = Stdio.Buffer();
      { array dta = ({sizeof(dtoid)});
        plugbuffer->add(_portalname, 0, _preparedname, 0)
         ->add_ints(dta
           + map(dtoid, writeoidformat, paramValues, ({0})) + dta, 2);
      }
      string cenc = pgsqlsess.runtimeparameter[CLIENT_ENCODING];
      foreach (paramValues; int i; mixed value) {
        if (undefinedp(value) || objectp(value) && value->is_val_null)
          plugbuffer->add_int32(-1);				// NULL
        else if (stringp(value) && !sizeof(value)) {
          int k = 0;
          switch (dtoid[i]) {
            default:
              k = -1;	     // cast empty strings to NULL for non-string types
            case BYTEAOID:
            case TEXTOID:
            case XMLOID:
            case BPCHAROID:
            case VARCHAROID:;
          }
          plugbuffer->add_int32(k);
        } else
          switch (dtoid[i]) {
            case TEXTOID:
            case BPCHAROID:
            case VARCHAROID: {
              if (!value) {
                plugbuffer->add_int32(-1);
                break;
              }
              value = (string)value;
              switch (cenc) {
                case UTF8CHARSET:
                  value = string_to_utf8(value);
                  break;
                default:
                  if (String.width(value)>8) {
                    SUSERERROR("Don't know how to convert %O to %s encoding\n",
                               value, cenc);
                    value="";
                  }
              }
              plugbuffer->add_hstring(value, 4);
              break;
            }
            default: {
              if (!value) {
                plugbuffer->add_int32(-1);
                break;
              }
              if (!objectp(value) || typeof(value) != typeof(Stdio.Buffer()));
                /*
                 *  Like Oracle and SQLite, we accept literal binary values
                 *  from single-valued multisets.
                 */
                if (multisetp(value) && sizeof(value) == 1)
                  value = indices(value)[0];
                else {
                  value = (string)value;
                  if (String.width(value) > 8)
                    if (dtoid[i] == BYTEAOID)
                      /*
                       *  FIXME We should throw an error here, it would
                       *  have been correct, but for historical reasons and
                       *  as a DWIM convenience we autoconvert to UTF8 here.
                       */
                      value = string_to_utf8(value);
                    else {
                      SUSERERROR(
                        "Wide string %O not supported for type OID %d\n",
                        value, dtoid[i]);
                      value = "";
                    }
                }
              plugbuffer->add_hstring(value, 4);
              break;
            }
            case BOOLOID:
              do {
                int tval;
                if (stringp(value))
                  tval = value[0];
                else if (!intp(value)) {
                  value = !!value;			// cast to boolean
                  break;
                } else
                  tval = value;
                switch (tval) {
                  case 'o':case 'O':
                    catch {
                      tval = value[1];
                      value = tval == 'n' || tval == 'N';
                    };
                    break;
                  default:
                    value = 1;
                    break;
                  case 0:case '0':case 'f':case 'F':case 'n':case 'N':
                    value = 0;
                      break;
                }
              } while (0);
              plugbuffer->add("\0\0\0\1", value);
              break;
            case CHAROID:
              if (intp(value))
                plugbuffer->add_hstring(value, 4);
              else {
                value = (string)value;
                switch (sizeof(value)) {
                  default:
                    SUSERERROR(
                     "\"char\" types must be 1 byte wide, got %O\n", value);
                  case 0:
                    plugbuffer->add_int32(-1);			// NULL
                    break;
                  case 1:
                    plugbuffer->add_hstring(value[0], 4);
                }
              }
              break;
            case NUMERICOID:
              if (stringp(value))
                plugbuffer->add_hstring(value, 4);
              else {
                int num, den, sign, magnitude = 0;
                value = Gmp.mpq(value);
                num = value->num();
                den = value->den();
                for (value = den;
                     value > NUMERIC_MAGSTEP;
                     value /= NUMERIC_MAGSTEP);
                if (value > 1) {
                  value = NUMERIC_MAGSTEP / value;
                  num *= value;
                  den *= value;
                }
                if (num < 0)
                  sign = 0x4000, num = -num;
                else
                  sign = 0;
                array stor = ({});
                if (num) {
                  while (!(num % NUMERIC_MAGSTEP))
                    num /= NUMERIC_MAGSTEP, magnitude++;
                  do
                    stor = ({num % NUMERIC_MAGSTEP}) + stor, magnitude++;
                  while (num /= NUMERIC_MAGSTEP);
                  num = --magnitude << 2;
                  while (den > 1)
                    magnitude--, den /= NUMERIC_MAGSTEP;
                }
                plugbuffer->add_int32(4 * 2 + (sizeof(stor) << 1))
                 ->add_int16(sizeof(stor))->add_int16(magnitude)
                 ->add_int16(sign)->add_int16(num)->add_ints(stor, 2);
              }
              break;
            case INT4RANGEOID:
            case INT8RANGEOID:
            case DATERANGEOID:
            case TSRANGEOID:
            case TSTZRANGEOID:
              if (stringp(value))
                plugbuffer->add_hstring(value, 4);
              else if (value->from >= value->till)
                plugbuffer->add("\0\0\0\1\1");
              else {
                array totype = oidtotype[dtoid[i]];
                int w = totype[1];
                int from, till;
                if (totype[0])
                  from = value->from, till = value->till;
                else {
                  from = value->from[totype[3]] - totype[2];
                  till = value->till[totype[3]] - totype[2];
                }
                if (value->till == Math.inf)
                  if (value->from == -Math.inf)
                    plugbuffer->add("\0\0\0\1\30");
                  else
                    plugbuffer->add("\0\0\0", 1 + 4 + w, "\22\0\0\0", w)
                     ->add_int(from, w);
                else {
                  if (value->from == -Math.inf)
                    plugbuffer->add("\0\0\0", 1 + 4 + w, 8);
                  else
                    plugbuffer->add("\0\0\0", 1 + 4 * 2 + w * 2, "\2\0\0\0", w)
                     ->add_int(from, w);
                  plugbuffer->add_int32(w)->add_int(till, w);
                }
              }
              break;
            case CIDROID:
            case INETOID:
              if (stringp(value))
                plugbuffer->add_hstring(value, 4);
              else if (value->address <= 0xffffffff)	// IPv4
                plugbuffer->add("\0\0\0\10\2",
                  value->masklen - 12 * 8, dtoid[i] == CIDROID, 4)
                 ->add_int32(value->address);
              else					// IPv6
                plugbuffer->add("\0\0\0\24\3",
                  value->masklen, dtoid[i] == CIDROID, 16)
                 ->add_int(value->address, 16);
              break;
            case DATEOID:
            case TIMEOID:
            case TIMETZOID:
            case INTERVALOID:
            case TIMESTAMPOID:
            case TIMESTAMPTZOID:
              if (stringp(value))
                plugbuffer->add_hstring(value, 4);
              else {
                array totype = oidtotype[dtoid[i]];
                if (!objectp(value))
                  value = totype[0](value);
                plugbuffer->add_int32(totype[1])
                 ->add_int(value[totype[3]] - totype[2], totype[4]);
                int i = 5;
                while (i < sizeof(totype)) {
                  plugbuffer->add_int(value[totype[i]], totype[i+1]);
                  i += 2;
                }
              }
              break;
            case FLOAT4OID:
#if !constant(__builtin.__SINGLE_PRECISION_FLOAT__)
            case FLOAT8OID:
#endif
              if (stringp(value))
                plugbuffer->add_hstring(value, 4);
              else {
                int w = dtoid[i] == FLOAT4OID ? 4 : 8;
                plugbuffer->add_int32(w)
                 ->sprintf(w == 4 ? "%4F" : "%8F", value);
              }
              break;
            case INT8OID:
              plugbuffer->add_int32(8)->add_int((int)value, 8);
              break;
            case OIDOID:
            case INT4OID:
              plugbuffer->add_int32(4)->add_int32((int)value);
              break;
            case INT2OID:
              plugbuffer->add_int32(2)->add_int16((int)value);
              break;
          }
      }
      if (!datarowtypes) {
        if (_tprepared && dontcacheprefix->match(_query))
          m_delete(pgsqlsess->prepareds, _query), _tprepared = 0;
        waitfordescribe();
      }
      if (_state >= CLOSING)
        lock = _unnamedstatementkey = 0;
      else {
        plugbuffer->add_int16(sizeof(datarowtypes));
        if (sizeof(datarowtypes)) {
          plugbuffer->add_ints(map(datarowtypes, readoidformat), 2);
          lock = 0;
        } else if (syncparse < 0 && !pgsqlsess->wasparallelisable
         && !pgsqlsess->statementsinflight->drained(1)) {
          lock = 0;
          PD("Commit waiting for statements to finish\n");
          catch(PT(pgsqlsess->statementsinflight->wait_till_drained(1)));
        }
        PD("Bind portal %O statement %O\n", _portalname, _preparedname);
        _fetchlimit = pgsqlsess->_fetchlimit;
        _bindportal();
        conxsess bindbuffer = c->start();
        _unnamedstatementkey = 0;
        stmtifkey = 0;
        CHAIN(bindbuffer)->add_int8('B')->add_hstring(plugbuffer, 4, 4);
        if (!_tprepared && sizeof(_preparedname))
          closestatement(CHAIN(bindbuffer), _preparedname);
        _sendexecute(_fetchlimit
                             && !(transtype != NOTRANS
                                  || sizeof(_query) >= MINPREPARELENGTH &&
                                  execfetchlimit->match(_query))
                             && _fetchlimit, bindbuffer);
      }
    }
  }

  final void _processrowdesc(array(mapping(string:mixed)) datarowdesc,
   array(int) datarowtypes) {
    _setrowdesc(datarowdesc, datarowtypes);
    if (_tprepared) {
      _tprepared.datarowdesc = datarowdesc;
      _tprepared.datarowtypes = datarowtypes;
    }
  }

  final void _parseportal() {
    {
      Thread.MutexKey lock = closemux->lock();
      _state = PARSING;
      if (syncparse || syncparse < 0 && pgsqlsess->wasparallelisable) {
        PD("Commit waiting for statements to finish\n");
        catch(PT(pgsqlsess->statementsinflight->wait_till_drained()));
      }
      stmtifkey = pgsqlsess->statementsinflight->acquire();
    }
    statuscmdcomplete = 0;
    pgsqlsess->wasparallelisable = paralleliseprefix->match(_query);
  }

  final void _releasestatement() {
    Thread.MutexKey lock = closemux->lock();
    if (_state <= BOUND) {
      stmtifkey = 0;
      _state = COMMITTED;
    }
  }

  final void _bindportal() {
    Thread.MutexKey lock = closemux->lock();
    _state = BOUND;
    portalsifkey = pgsqlsess->portalsinflight->acquire();
  }

  final void _purgeportal() {
    PD("Purge portal\n");
    datarows->write(1);				   // Signal EOF
    {
      Thread.MutexKey lock = closemux->lock();
      _fetchlimit = 0;				   // disables further Executes
      stmtifkey = portalsifkey = 0;
      _state = PURGED;
    }
    releaseconditions();
  }

  final int _closeportal(conxsess cs, array(Thread.MutexKey) reflock) {
    void|bufcon|conxsess plugbuffer = CHAIN(cs);
    int retval = KEEP;
    PD("%O Try Closeportal %d\n", _portalname, _state);
    _fetchlimit = 0;				   // disables further Executes
    stmtifkey = 0;
    switch (_state) {
      case PORTALINIT:
      case PARSING:
        _unnamedstatementkey = 0;
        _state = CLOSING;
        break;
      case COPYINPROGRESS:
        PD("CopyDone\n");
        plugbuffer->add("c\0\0\0\4");
      case COMMITTED:
      case BOUND:
        _state = CLOSING;
        if (reflock)
          reflock[0] = 0;
        PD("Close portal %O\n", _portalname);
        if (_portalname && sizeof(_portalname)) {
          plugbuffer->add_int8('C')
           ->add_hstring(({'P', _portalname, 0}), 4, 4);
          retval = FLUSHSEND;
        } else
          _unnamedportalkey = 0;
        portalsifkey = 0;
        if (pgsqlsess->portalsinflight->drained()) {
          if (plugbuffer->stashcount->drained() && transtype != TRANSBEGIN)
           /*
            * stashcount will be non-zero if a parse request has been queued
            * before the close was initiated.
            * It's a bit of a tricky race, but this check should be sufficient.
            */
            pgsqlsess->readyforquerycount++, retval = SYNCSEND;
          pgsqlsess->pportalcount = 0;
        }
    }
    return retval;
  }

  private void replenishrows() {
   if (_fetchlimit && datarows->size() <= _fetchlimit >> 1
    && _state >= COMMITTED) {
      Thread.MutexKey lock = closemux->lock();
      if (_fetchlimit) {
        _fetchlimit = pgsqlsess._fetchlimit;
        if (bytesreceived)
          _fetchlimit =
           min((portalbuffersize >> 1) * index / bytesreceived || 1, _fetchlimit);
        if (_fetchlimit)
          if (inflight <= (_fetchlimit - 1) >> 1) {
            array reflock = ({ lock });
            lock = 0;
            _sendexecute(_fetchlimit, reflock);
          } else
            PD("<%O _fetchlimit %d, inflight %d, skip execute\n",
             _portalname, _fetchlimit, inflight);
      }
    }
  }

  final void _processdataready(array datarow, void|int msglen) {
    bytesreceived += msglen;
    inflight--;
    if (_state < CLOSED)
      datarows->write(datarow);
    if (++index == 1)
      PD("<%O _fetchlimit %d=min(%d||1,%d), inflight %d\n", _portalname,
       _fetchlimit, (portalbuffersize >> 1) * index / bytesreceived,
       pgsqlsess._fetchlimit, inflight);
    replenishrows();
  }

  private void releaseconditions() {
    _unnamedportalkey = _unnamedstatementkey = 0;
    if (!datarowtypes) {
      if (_state != PURGED && !delayederror)
        delayederror = LOSTERROR;
      datarowtypes = emptyarray;
      _ddescribe->broadcast();
    }
    if (delayederror && !pgsqlsess.delayederror)
      pgsqlsess.delayederror = delayederror;	// Preserve error upstream
    pgsqlsess = 0;
  }

  final void _releasesession(void|string statusccomplete) {
    c->runningportals[this] = 0;
    if (statusccomplete && !statuscmdcomplete) {
      Thread.MutexKey lock = _ddescribemux->lock();
      statuscmdcomplete = statusccomplete;
      _ddescribe->broadcast();
    }
    inflight = 0;
    conxsess plugbuffer;
    array(Thread.MutexKey) reflock = ({closemux->lock()});
    if (!catch(plugbuffer = c->start()))
      plugbuffer->sendcmd(_closeportal(plugbuffer, reflock));
    reflock = 0;
    if (_state < CLOSED) {
      stmtifkey = 0;
      _state = CLOSED;
    }
    datarows->write(1);				// Signal EOF
    releaseconditions();
  }

  protected void _destruct() {
    catch {			   // inside destructors, exceptions don't work
      _releasesession();
    };
  }

  final void _sendexecute(int fetchlimit,
   void|array(Thread.MutexKey)|bufcon|conxsess plugbuffer) {
    int flushmode;
    array(Thread.MutexKey) reflock;
    PD("Execute portal %O fetchlimit %d transtype %d\n", _portalname,
     fetchlimit, transtype);
    if (arrayp(plugbuffer)) {
      reflock = plugbuffer;
      plugbuffer = c->start(1);
    }
    CHAIN(plugbuffer)->add_int8('E')->add_hstring(({_portalname, 0}), 4, 8)
     ->add_int32(fetchlimit);
    if (!fetchlimit) {
      if (transtype != NOTRANS)
        pgsqlsess.intransaction = transtype == TRANSBEGIN;
      flushmode = _closeportal(plugbuffer, reflock) == SYNCSEND
       || transtype == TRANSEND ? SYNCSEND : FLUSHSEND;
    } else
      inflight += fetchlimit, flushmode = FLUSHSEND;
    plugbuffer->sendcmd(flushmode, this);
  }

  inline private array setuptimeout() {
    return local_backend->call_out(gottimeout, timeout);
  }

  inline private void scuttletimeout(array cid) {
    if (local_backend)
      local_backend->remove_call_out(cid);
  }

  //! @returns
  //!  One result row at a time.
  //!
  //! When using COPY FROM STDOUT, this method returns one row at a time
  //! as a single string containing the entire row.
  //!
  //! @seealso
  //!  @[eof()], @[send_row()]
  /*semi*/final array(mixed) fetch_row() {
    int|array datarow;
    replenishrows();
    if (arrayp(datarow = datarows->try_read()))
      return datarow;
    if (!eoffound) {
      if (!datarow) {
        PD("%O Block for datarow\n", _portalname);
        array cid = setuptimeout();
        PT(datarow = datarows->read());
        if (!this)		// If object already destructed, return fast
          return 0;
        scuttletimeout(cid);
        if (arrayp(datarow))
          return datarow;
      }
      eoffound = 1;
      datarows->write(1);			// Signal EOF for other threads
    }
    trydelayederror();
    return 0;
  }

  //! @returns
  //!  Multiple result rows at a time (at least one).
  //!
  //! When using COPY FROM STDOUT, this method returns one row at a time
  //! as a single string containing the entire row.
  //!
  //! @seealso
  //!  @[eof()], @[fetch_row()]
  /*semi*/final array(array(mixed)) fetch_row_array() {
    if (eoffound)
      return 0;
    replenishrows();
    array(array|int) datarow = datarows->try_read_array();
    if (!sizeof(datarow)) {
      array cid = setuptimeout();
      PT(datarow = datarows->read_array());
      if (!this)		// If object already destructed, return fast
        return 0;
      scuttletimeout(cid);
    }
    replenishrows();
    if (arrayp(datarow[-1]))
      return datarow;
    do datarow = datarow[..<1];			// Swallow EOF mark(s)
    while (sizeof(datarow) && !arrayp(datarow[-1]));
    trydelayederror();
    eoffound = 1;
    datarows->write(1);				// Signal EOF for other threads
    return datarow;
  }

  //! @param copydata
  //! When using COPY FROM STDIN, this method accepts a string or an
  //! array of strings to be processed by the COPY command; when sending
  //! the amount of data sent per call does not have to hit row or column
  //! boundaries.
  //!
  //! The COPY FROM STDIN sequence needs to be completed by either
  //! explicitly or implicitly destroying the result object, or by passing no
  //! argument to this method.
  //!
  //! @seealso
  //!  @[fetch_row()], @[eof()]
  /*semi*/final void send_row(void|string|array(string) copydata) {
    trydelayederror();
    if (copydata) {
      PD("CopyData\n");
      void|bufcon|conxsess cs = c->start();
      CHAIN(cs)->add_int8('d')->add_hstring(copydata, 4, 4);
      cs->sendcmd(SENDOUT);
    } else
      _releasesession();
  }

  private void run_result_cb(
   function(Result, array(mixed), mixed ...:void) callback,
   array(mixed) args) {
    int|array datarow;
    for (;;) {
      array cid = setuptimeout();
      PT(datarow = datarows->read());
      if (!this)		// If object already destructed, return fast
        return 0;
      scuttletimeout(cid);
      if (!arrayp(datarow))
        break;
      callout(callback, 0, this, datarow, @args);
    }
    eoffound = 1;
    callout(callback, 0, this, 0, @args);
  }

  //! Sets up a callback for every row returned from the database.
  //! First argument passed is the resultobject itself, second argument
  //! is the result row (zero on EOF).
  //!
  //! @seealso
  //!  @[fetch_row()]
  /*semi*/final void set_result_callback(
   function(Result, array(mixed), mixed ...:void) callback,
   mixed ... args) {
    if (callback)
      Thread.Thread(run_result_cb, callback, args);
  }

  private void run_result_array_cb(
   function(Result, array(array(mixed)), mixed ...:void) callback,
   array(mixed) args) {
    array(array|int) datarow;
    for (;;) {
      array cid = setuptimeout();
      PT(datarow = datarows->read_array());
      if (!this)		// If object already destructed, return fast
        return 0;
      scuttletimeout(cid);
      if (!datarow || !arrayp(datarow[-1]))
        break;
      callout(callback, 0, this, datarow, @args);
    }
    eoffound = 1;
    if (sizeof(datarow)>1)
      callout(callback, 0, this, datarow = datarow[..<1], @args);
    callout(callback, 0, this, 0, @args);
  }

  //! Sets up a callback for sets of rows returned from the database.
  //! First argument passed is the resultobject itself, second argument
  //! is the array of result rows (zero on EOF).
  //!
  //! @seealso
  //!  @[fetch_row()]
  /*semi*/final void set_result_array_callback(
   function(Result, array(array(mixed)), mixed ...:void) callback,
   mixed ... args) {
    if (callback)
      Thread.Thread(run_result_array_cb, callback, args);
  }

};

class proxy {
  final int _fetchlimit = FETCHLIMIT;
  final MUTEX unnamedportalmux;
  final MUTEX unnamedstatement;
  private Thread.MutexKey termlock;
  private Thread.ResourceCountKey backendreg;
  final Thread.ResourceCount portalsinflight, statementsinflight;
  final int(0..1) wasparallelisable;
  final int(0..1) intransaction;

  final conxion c;
  private string cancelsecret;
  private int backendpid;
  final int(-128..127) backendstatus;
  final mapping(string:mixed) options;
  private array(string) lastmessage = emptyarray;
  final int(0..1) clearmessage;
  final int(0..1) untolderror;
  private mapping(string:array(mixed)) notifylist = ([]);
  final mapping(string:string) runtimeparameter;
  final mapping(string:mapping(string:mixed)) prepareds = ([]);
  final int pportalcount;
  final int totalhits;
  final int msgsreceived;	// Number of protocol messages received
  final int bytesreceived;	// Number of bytes received
  final int warningsdropcount;	// Number of uncollected warnings
  private int warningscollected;
  final int(0..1) invalidatecache;
  private Thread.Queue qportals;
  final mixed delayederror;
  final function (:void) readyforquery_cb;

  final string host;
  final int(0..65535) port;
  final string database, user, pass;
  private Crypto.Hash.SCRAM SASLcontext;
  final Thread.Condition waitforauthready;
  final MUTEX shortmux;
  final int readyforquerycount;

  private string _sprintf(int type) {
    string res;
    switch (type) {
      case 'O':
        res = sprintf(DRIVERNAME".proxy(%s@%s:%d/%s,%d,%d)",
          user, host, port, database, c && c->socket && c->socket->query_fd(),
          backendpid);
        break;
    }
    return res;
  }

  private void create(void|string host, void|string database,
                        void|string user, void|string pass,
                        void|mapping(string:mixed) options) {
    if (this::pass = pass) {
      String.secure(pass);
      pass = "CENSORED";
    }
    this::user = user;
    this::database = database;
    this::options = options;

    if (!host) host = PGSQL_DEFAULT_HOST;
    if (has_value(host,":") && sscanf(host,"%s:%d", host, port) != 2)
      error("Error in parsing the hostname argument\n");
    this::host = host;

    if (!port)
      port = PGSQL_DEFAULT_PORT;
    backendreg = register_backend();
    shortmux = MUTEX();
    PD("Connect\n");
    waitforauthready = Thread.Condition();
    qportals = Thread.Queue();
    readyforquerycount = 1;
    qportals->write(1);
    if (!(c = conxion(this, qportals, 0)))
      error("Couldn't connect to database on %s:%d\n", host, port);
    runtimeparameter = ([]);
    unnamedportalmux = MUTEX();
    unnamedstatement = MUTEX();
    readyforquery_cb = connect_cb;
    portalsinflight = Thread.ResourceCount();
    statementsinflight = Thread.ResourceCount();
    wasparallelisable = 0;
  }

  final int is_open() {
    return c && c->socket && c->socket->is_open();
  }

  final string geterror(void|int clear) {
    untolderror = 0;
    string s = lastmessage * "\n";
    if (clear)
      lastmessage = emptyarray;
    warningscollected = 0;
    return sizeof(s) && s;
  }

  final string host_info() {
    return sprintf("fd:%d TCP/IP %s:%d PID %d",
                   c ? c->socket->query_fd() : -1, host, port, backendpid);
  }

  final void cancelquery() {
    if (cancelsecret > "") {
      PD("CancelRequest\n");
      conxion lcon = conxion(this, 0, 2);
#ifdef PG_DEBUG
      mixed err =
#endif
      catch(lcon->add_int32(16)->add_int32(PG_PROTOCOL(1234, 5678))
        ->add_int32(backendpid)->add(cancelsecret)->sendcmd(FLUSHSEND));
#ifdef PG_DEBUG
      if (err)
        PD("CancelRequest failed to connect %O\n", describe_backtrace(err));
#endif
      destruct(lcon);		// Destruct explicitly to avoid delayed close
#ifdef PG_DEBUGMORE
      PD("Closetrace %O\n", backtrace());
#endif
    } else
      error("Connection not established, cannot cancel any query\n");
  }

  private string a2nls(array(string) msg) {
    return msg * "\n" + "\n";
  }

  private string pinpointerror(void|string query, void|string offset) {
    if (!query)
      return "";
    int k = (int)offset;
    if (k <= 0)
      return MARKSTART + query + MARKEND;
    return MARKSTART + (k > 1 ? query[..k-2] : "")
         + MARKERROR + query[k - 1..] + MARKEND;
  }

  private void connect_cb() {
    PD("%O\n", runtimeparameter);
  }

  private array(string) showbindings(Result portal) {
    return portal ? portal._showbindings() : emptyarray;
  }

  private void preplastmessage(mapping(string:string) msgresponse) {
    lastmessage = ({
      sprintf("%s %s:%s %s\n (%s:%s:%s)",
              msgresponse.S, msgresponse.C, msgresponse.P || "",
              msgresponse.M, msgresponse.F || "", msgresponse.R || "",
              msgresponse.L||"")});
  }

  private int|Result portal;  // state information procmessage
#ifdef PG_DEBUG
  private string datarowdebug;
  private int datarowdebugcount;
#endif

  final void processloop(conxion ci) {
    (c = ci)->socket->set_id(procmessage);
    cancelsecret = 0;
    portal = 0;
    {
      Stdio.Buffer plugbuffer = Stdio.Buffer()->add_int32(PG_PROTOCOL(3, 0));
      if (user)
        plugbuffer->add("user\0", user, 0);
      if (database)
        plugbuffer->add("database\0", database, 0);
      foreach (options - censoroptions; string name; mixed value)
        plugbuffer->add(name, 0, (string)value, 0);
      plugbuffer->add_int8(0);
      PD("%O\n", (string)plugbuffer);
      void|bufcon|conxsess cs;
      if (catch(cs = ci->start())) {
        destruct(waitforauthready);
        unnamedstatement = 0;
        termlock = 0;
        return;
      } else {
        CHAIN(cs)->add_hstring(plugbuffer, 4, 4);
        cs->sendcmd(SENDOUT);
      }
    }		      // Do not flush at this point, PostgreSQL 9.4 disapproves
    procmessage();
  }

  private void procmessage() {
    mixed err;
    int terminating = 0;
    err = catch {
    conxion ci = c;		// cache value
    conxiin cr = ci->i;		// cache value
#ifdef PG_DEBUG
    PD("Processloop\n");

#ifdef PG_DEBUGMORE
    void showportalstack(string label) {
      PD(sprintf(">>>>>>>>>>> Portalstack %s: %O\n", label, portal));
      foreach (qportals->peek_array(); ; int|Result qp)
        PD(" =========== Portal: %O\n", qp);
      PD("<<<<<<<<<<<<<< Portalstack end\n");
    };
#endif
    void showportal(int msgtype) {
      if (objectp(portal))
        PD("%d<%O %d %c switch portal\n",
         ci->socket->query_fd(), portal._portalname, ++ci->queueinidx, msgtype);
      else if (portal > 0)
        PD("%d<Sync %d %d %c portal\n",
         ci->socket->query_fd(), ++ci->queueinidx, portal, msgtype);
    };
#endif
    int msgisfatal(mapping(string:string) msgresponse) {
      int isfatal = (has_prefix(msgresponse.C, "53")
                  || has_prefix(msgresponse.C, "3D")
                  || has_prefix(msgresponse.C, "57P")) && MAGICTERMINATE;
      if (isfatal && !terminating) // Run the callback once per lost connection
        runcallback(backendpid, "_lost", "");
      return isfatal;
    };
    for (;;) {
      err = catch {
#ifdef PG_DEBUG
        if (!portal && datarowdebug) {
          PD("%s rows %d\n", datarowdebug, datarowdebugcount);
          datarowdebug = 0; datarowdebugcount = 0;
        }
#endif
#ifdef PG_DEBUGMORE
        showportalstack("LOOPTOP");
#endif
        if (!sizeof(cr)) {			// Preliminary check, fast path
          Thread.MutexKey lock = cr->fillreadmux->lock();
          if (!sizeof(cr)) {			// Check for real
            if (!cr->fillread) {
              lock = 0;
              throw(MAGICTERMINATE);	// Force proper termination
            }
            cr->procmsg = 1;
            return;			// Terminate thread, wait for callback
          }
        }
        int msgtype = cr->read_int8();
        if (!portal) {
          portal = qportals->try_read();
#ifdef PG_DEBUG
          showportal(msgtype);
#endif
        }
        int msglen = cr->read_int32();
        msgsreceived++;
        bytesreceived += 1 + msglen;
        int errtype = NOERROR;
        PD("%d<", ci->socket->query_fd());
        switch (msgtype) {
          array getcols() {
            int bintext = cr->read_int8();
            int cols = cr->read_int16();
#ifdef PG_DEBUG
            array a;
            msglen -= 4 + 1 + 2 + 2 * cols;
            foreach (a = allocate(cols, ([])); ; mapping m)
              m.type = cr->read_int16();
#else
            cr->consume(cols << 1);
#endif		      // Discard column info, and make it line oriented
            return ({ ({(["name":"line"])}), ({bintext?BYTEAOID:TEXTOID}) });
          };
          array(string) reads() {
#ifdef PG_DEBUG
            if (msglen < 1)
              errtype = PROTOCOLERROR;
#endif
            array ret = emptyarray, aw = ({0});
            do {
              string w = cr->read_cstring();
              msglen -= sizeof(w) + 1; aw[0] = w; ret += aw;
            } while (msglen);
            return ret;
          };
          mapping(string:string) getresponse() {
            mapping(string:string) msgresponse = ([]);
            msglen -= 4;
            foreach (reads(); ; string f)
              if (sizeof(f))
                msgresponse[f[..0]] = f[1..];
            PD("%O\n", msgresponse);
            return msgresponse;
          };
          case 'R': {
            void authresponse(string|array msg) {
              void|bufcon|conxsess cs = ci->start();
              CHAIN(cs)->add_int8('p')->add_hstring(msg, 4, 4);
              cs->sendcmd(SENDOUT); // No flushing, PostgreSQL 9.4 disapproves
            };
            PD("Authentication ");
            msglen -= 4 + 4;
            int authtype, k;
            switch (authtype = cr->read_int32()) {
              case 0:
                PD("Ok\n");
                if (SASLcontext) {
                  PD("Authentication validation still in progress\n");
                  errtype = PROTOCOLUNSUPPORTED;
                } else
                  cancelsecret = "";
                break;
              case 2:
                PD("KerberosV5\n");
                errtype = PROTOCOLUNSUPPORTED;
                break;
              case 3:
                PD("ClearTextPassword\n");
                authresponse(({pass, 0}));
                break;
              case 4:
                PD("CryptPassword\n");
                errtype = PROTOCOLUNSUPPORTED;
                break;
              case 5:
                PD("MD5Password\n");
#ifdef PG_DEBUG
                if (msglen < 4)
                  errtype = PROTOCOLERROR;
#endif
#define md5hex(x) String.string2hex(Crypto.MD5.hash(x))
                authresponse(({"md5",
                 md5hex(md5hex(pass + user) + cr->read(msglen)), 0}));
#ifdef PG_DEBUG
                msglen = 0;
#endif
                break;
              case 6:
                PD("SCMCredential\n");
                errtype = PROTOCOLUNSUPPORTED;
                break;
              case 7:
                PD("GSS\n");
                errtype = PROTOCOLUNSUPPORTED;
                break;
              case 9:
                PD("SSPI\n");
                errtype = PROTOCOLUNSUPPORTED;
                break;
              case 8:
                PD("GSSContinue\n");
                errtype = PROTOCOLUNSUPPORTED;
                cr->read(msglen);				// SSauthdata
#ifdef PG_DEBUG
                if (msglen<1)
                  errtype = PROTOCOLERROR;
                msglen = 0;
#endif
                break;
              case 10: {
                string word;
                PD("AuthenticationSASL\n");
                k = 0;
                while (sizeof(word = cr->read_cstring())) {
                  switch (word) {
                    case "SCRAM-SHA-256":
                      k = 1;
                  }
#ifdef PG_DEBUG
                  msglen -= sizeof(word) + 1;
                  if (msglen < 1)
                    break;
#endif
                }
                if (k) {
                  SASLcontext = Crypto.SHA256.SCRAM();
                  word = SASLcontext.client_1();
                  authresponse(({
                    "SCRAM-SHA-256", 0, sprintf("%4c", sizeof(word)), word
                   }));
                } else
                  errtype = PROTOCOLUNSUPPORTED;
#ifdef PG_DEBUG
                if (msglen != 1)
                  errtype = PROTOCOLERROR;
                msglen = 0;
#endif
                break;
              }
              case 11: {
                PD("AuthenticationSASLContinue\n");
                string response;
                if (response
                 = SASLcontext.client_2(cr->read(msglen), pass))
                  authresponse(response);
                else
                  errtype = PROTOCOLERROR;
#ifdef PG_DEBUG
                msglen = 0;
#endif
                break;
              }
              case 12:
                PD("AuthenticationSASLFinal\n");
                if (SASLcontext.client_3(cr->read(msglen)))
                  SASLcontext = 0;	// Clears context and approves server
                else
                  errtype = PROTOCOLERROR;
#ifdef PG_DEBUG
                msglen = 0;
#endif
                break;
              default:
                PD("Unknown Authentication Method %c\n", authtype);
                errtype = PROTOCOLUNSUPPORTED;
                break;
            }
            switch (errtype) {
              default:
              case PROTOCOLUNSUPPORTED:
                error("Unsupported authenticationmethod %c\n", authtype);
              case NOERROR:
                break;
            }
            break;
          }
          case 'K':
            msglen -= 4 + 4; backendpid = cr->read_int32();
            cancelsecret = cr->read(msglen);
#ifdef PG_DEBUG
            PD("BackendKeyData %O\n", cancelsecret);
            msglen = 0;
#endif
            break;
          case 'S': {
            PD("ParameterStatus ");
            msglen -= 4;
            array(string) ts = reads();
#ifdef PG_DEBUG
            if (sizeof(ts) == 2) {
#endif
              runtimeparameter[ts[0]] = ts[1];
#ifdef PG_DEBUG
              PD("%O=%O\n", ts[0], ts[1]);
            } else
              errtype = PROTOCOLERROR;
#endif
            break;
          }
          case '3':
#ifdef PG_DEBUG
            PD("CloseComplete\n");
            msglen -= 4;
#endif
            break;
          case 'Z': {
            backendstatus = cr->read_int8();
#ifdef PG_DEBUG
            msglen -= 4 + 1;
            PD("ReadyForQuery %c\n", backendstatus);
#endif
#ifdef PG_DEBUGMORE
            showportalstack("READYFORQUERY");
#endif
            int keeplooking;
            do
              for (keeplooking = 0;
                   objectp(portal);
                   portal = qportals->read()) {
#ifdef PG_DEBUG
                showportal(msgtype);
#endif
                if (backendstatus == 'I' && intransaction
                 && portal->transtype != TRANSEND)
                  keeplooking = 1;
                portal->_purgeportal();
              }
            while (keeplooking && (portal = qportals->read()));
            if (backendstatus == 'I')
              intransaction = 0;
            foreach (qportals->peek_array(); ; Result qp) {
              if (objectp(qp) && qp._synctransact
               && qp._synctransact <= portal) {
                PD("Checking portal %O %d<=%d\n",
                 qp._portalname, qp._synctransact, portal);
                qp->_purgeportal();
              }
            }
            portal = 0;
#ifdef PG_DEBUGMORE
            showportalstack("AFTER READYFORQUERY");
#endif
            readyforquerycount--;
            if (readyforquery_cb)
              readyforquery_cb(), readyforquery_cb = 0;
            destruct(waitforauthready);
            break;
          }
          case '1':
#ifdef PG_DEBUG
            PD("ParseComplete portal %O\n", portal);
            msglen -= 4;
#endif
            break;
          case 't': {
            array a;
#ifdef PG_DEBUG
            int cols = cr->read_int16();
            PD("%O ParameterDescription %d values\n", portal._query, cols);
            msglen -= 4 + 2 + 4 * cols;
            a = cr->read_ints(cols, 4);
#else
            a = cr->read_ints(cr->read_int16(), 4);
#endif
#ifdef PG_DEBUGMORE
            PD("%O\n", a);
#endif
            if (portal._tprepared)
              portal._tprepared.datatypeoid = a;
            Thread.Thread(portal->_preparebind, a);
            break;
          }
          case 'T': {
            array a, at;
            int cols = cr->read_int16();
#ifdef PG_DEBUG
            PD("RowDescription %d columns %O\n", cols, portal._query);
            msglen -= 4 + 2;
#endif
            at = allocate(cols);
            foreach (a = allocate(cols); int i; ) {
              string s = cr->read_cstring();
              mapping(string:mixed) res = (["name":s]);
#ifdef PG_DEBUG
              msglen -= sizeof(s) + 1 + 4 + 2 + 4 + 2 + 4 + 2;
              res.tableoid = cr->read_int32() || UNDEFINED;
              res.tablecolattr = cr->read_int16() || UNDEFINED;
#else
              cr->consume(6);
#endif
              at[i] = cr->read_int32();
#ifdef PG_DEBUG
              res.type = at[i];
              {
                int len = cr->read_sint(2);
                res.length = len >= 0 ? len : "variable";
              }
              res.atttypmod = cr->read_int32();
              /* formatcode contains just a zero when Bind has not been issued
               * yet, but the content is irrelevant because it's determined
               * at query time
               */
              res.formatcode = cr->read_int16();
#else
              cr->consume(8);
#endif
              a[i] = res;
            }
#ifdef PG_DEBUGMORE
            PD("%O\n", a);
#endif
            if (portal._forcetext)
              portal->_setrowdesc(a, at);	// Do not consume queued portal
            else {
              portal->_processrowdesc(a, at);
              portal = 0;
            }
            break;
          }
          case 'n': {
#ifdef PG_DEBUG
            msglen -= 4;
            PD("NoData %O\n", portal._query);
#endif
            portal._fetchlimit = 0;		// disables subsequent Executes
            portal->_processrowdesc(emptyarray, emptyarray);
            portal = 0;
            break;
          }
          case 'H':
            portal->_processrowdesc(@getcols());
            PD("CopyOutResponse %O\n", portal. _query);
            break;
          case '2': {
            mapping tp;
#ifdef PG_DEBUG
            msglen -= 4;
            PD("%O BindComplete\n", portal._portalname);
#endif
            if (tp = portal._tprepared) {
              int tend = gethrtime();
              int tstart = tp.trun;
              if (tend == tstart)
                m_delete(prepareds, portal._query);
              else {
                tp.hits++;
                totalhits++;
                if (!tp.preparedname) {
                  if (sizeof(portal._preparedname)) {
                    PD("Finalising stored statement %s\n",
                     portal._preparedname);
                    tp.preparedname = portal._preparedname;
                  }
                  tstart = tend - tstart;
                  if (!tp.tparse || tp.tparse>tstart)
                    tp.tparse = tstart;
                }
                tp.trunstart = tend;
              }
            }
            break;
          }
          case 'D':
            msglen -= 4;
#ifdef PG_DEBUG
#ifdef PG_DEBUGMORE
            PD("%O DataRow %d bytes\n", portal._portalname, msglen);
#endif
            datarowdebugcount++;
            if (!datarowdebug)
              datarowdebug = sprintf(
               "%O DataRow %d bytes", portal._portalname, msglen);
#endif
#ifdef PG_DEBUG
            msglen=
#endif
            portal->_decodedata(msglen, runtimeparameter[CLIENT_ENCODING]);
            break;
          case 's':
#ifdef PG_DEBUG
            PD("%O PortalSuspended\n", portal._portalname);
            msglen -= 4;
#endif
            portal = 0;
            break;
          case 'C': {
            msglen -= 4;
#ifdef PG_DEBUG
            if (msglen < 1)
              errtype = PROTOCOLERROR;
#endif
            string s = cr->read(msglen - 1);
            portal->_storetiming();
            PD("%O CommandComplete %O\n", portal._portalname, s);
#ifdef PG_DEBUG
            if (cr->read_int8())
              errtype = PROTOCOLERROR;
            msglen = 0;
#else
            cr->consume(1);
#endif
#ifdef PG_DEBUGMORE
            showportalstack("COMMANDCOMPLETE");
#endif
            portal->_releasesession(s);
            portal = 0;
            break;
          }
          case 'I':
#ifdef PG_DEBUG
            PD("EmptyQueryResponse %O\n", portal._portalname);
            msglen -= 4;
#endif
#ifdef PG_DEBUGMORE
            showportalstack("EMPTYQUERYRESPONSE");
#endif
            portal->_releasesession();
            portal = 0;
            break;
          case 'd':
            PD("%O CopyData\n", portal._portalname);
            portal->_storetiming();
            msglen -= 4;
#ifdef PG_DEBUG
            if (msglen < 0)
              errtype = PROTOCOLERROR;
#endif
            portal->_processdataready(({cr->read(msglen)}), msglen);
#ifdef PG_DEBUG
            msglen = 0;
#endif
            break;
          case 'G':
            portal->_releasestatement();
            portal->_setrowdesc(@getcols());
            PD("%O CopyInResponse\n", portal._portalname);
            portal._state = COPYINPROGRESS;
            break;
          case 'c':
#ifdef PG_DEBUG
            PD("%O CopyDone\n", portal._portalname);
            msglen -= 4;
#endif
            portal = 0;
            break;
          case 'E': {
#ifdef PG_DEBUGMORE
            showportalstack("ERRORRESPONSE");
#endif
            if (portalsinflight->drained() && !readyforquerycount)
              sendsync();
            PD("%O ErrorResponse %O\n",
             objectp(portal) && (portal._portalname || portal._preparedname),
             objectp(portal) && portal._query);
            mapping(string:string) msgresponse;
            msgresponse = getresponse();
            warningsdropcount += warningscollected;
            warningscollected = 0;
            untolderror = 1;
            switch (msgresponse.C) {
              case "P0001":
                lastmessage
                 = ({sprintf("%s: %s", msgresponse.S, msgresponse.M)});
                USERERROR(a2nls(lastmessage
                               +({pinpointerror(portal._query, msgresponse.P)})
                               +showbindings(portal)));
              case "53000":case "53100":case "53200":case "53300":case "53400":
              case "57P01":case "57P02":case "57P03":case "57P04":case "3D000":
                preplastmessage(msgresponse);
                PD(a2nls(lastmessage)); throw(msgisfatal(msgresponse));
              case "08P01":case "42P05":
                errtype = PROTOCOLERROR;
              case "XX000":case "42883":case "42P01":
                invalidatecache = 1;
              default:
                preplastmessage(msgresponse);
                if (msgresponse.D)
                  lastmessage += ({msgresponse.D});
                if (msgresponse.H)
                  lastmessage += ({msgresponse.H});
                lastmessage += ({
                  pinpointerror(objectp(portal) && portal._query, msgresponse.P)
                  + pinpointerror(msgresponse.q, msgresponse.p)});
                if (msgresponse.W)
                  lastmessage += ({msgresponse.W});
                if (objectp(portal))
                  lastmessage += showbindings(portal);
                switch (msgresponse.S) {
                  case "PANIC":werror(a2nls(lastmessage));
                }
              case "25P02":		        // Preserve last error message
                USERERROR(a2nls(lastmessage));	// Implicitly closed portal
            }
            break;
          }
          case 'N': {
            PD("NoticeResponse\n");
            mapping(string:string) msgresponse = getresponse();
            if (clearmessage) {
              warningsdropcount += warningscollected;
              clearmessage = warningscollected = 0;
              lastmessage = emptyarray;
            }
            warningscollected++;
            lastmessage = ({sprintf("%s %s: %s",
                             msgresponse.S, msgresponse.C, msgresponse.M)});
            int val;
            if (val = msgisfatal(msgresponse)) {  // Some warnings are fatal
              preplastmessage(msgresponse);
              PD(a2nls(lastmessage)); throw(val);
            }
            break;
          }
          case 'A': {
            PD("NotificationResponse\n");
            msglen -= 4 + 4;
            int pid = cr->read_int32();
            string condition, extrainfo;
            {
              array(string) ts = reads();
              switch (sizeof(ts)) {
#if PG_DEBUG
                case 0:
                  errtype = PROTOCOLERROR;
                  break;
                default:
                  errtype = PROTOCOLERROR;
#endif
                case 2:
                  extrainfo = ts[1];
                case 1:
                  condition = ts[0];
              }
            }
            PD("%d %s\n%s\n", pid, condition, extrainfo);
            runcallback(pid, condition, extrainfo);
            break;
          }
          default:
            if (msgtype != -1) {
              string s;
              PD("Unknown message received %c\n", msgtype);
              s = cr->read(msglen -= 4); PD("%O\n", s);
#ifdef PG_DEBUG
              msglen = 0;
#endif
              errtype = PROTOCOLUNSUPPORTED;
            } else {
              lastmessage += ({
               sprintf("Connection lost to database %s@%s:%d/%s %d\n",
                    user, host, port, database, backendpid)});
              runcallback(backendpid, "_lost", "");
              if (!waitforauthready)
                throw(0);
              USERERROR(a2nls(lastmessage));
            }
            break;
        }
#ifdef PG_DEBUG
        if (msglen)
          errtype = PROTOCOLERROR;
#endif
        {
          string msg;
          switch (errtype) {
            case PROTOCOLUNSUPPORTED:
              msg
               = sprintf("Unsupported servermessage received %c\n", msgtype);
              break;
            case PROTOCOLERROR:
              msg = sprintf("Protocol error with database %s", host_info());
              break;
            case NOERROR:
              continue;				// Normal production loop
          }
          error(a2nls(lastmessage += ({msg})));
        }
      };			// We only get here if there is an error
      if (err == MAGICTERMINATE) { // Announce connection termination to server
        catch {
          void|bufcon|conxsess cs = ci->start();
          CHAIN(cs)->add("X\0\0\0\4");
          cs->sendcmd(SENDOUT);
        };
        terminating = 1;
        err = 0;
      } else if (stringp(err)) {
        Result or;
        if (!objectp(or = portal))
          or = this;
        if (!or.delayederror)
          or.delayederror = err;
#ifdef PG_DEBUGMORE
        showportalstack("THROWN");
#endif
        if (objectp(portal))
          portal->_releasesession();
        portal = 0;
        if (!waitforauthready)
          continue;		// Only continue if authentication did not fail
      }
      break;
    }
    PD("Closing database processloop %s\n", err ? describe_backtrace(err) : "");
    delayederror = err;
    if (objectp(portal)) {
#ifdef PG_DEBUG
      showportal(0);
#endif
      portal->_purgeportal();
    }
    destruct(waitforauthready);
    termlock = 0;
    if (err && !stringp(err))
      throw(err);
    };
    catch {
     unnamedstatement = 0;
     termlock = 0;
      if (err) {
        PD("Terminating processloop due to %s\n", describe_backtrace(err));
        delayederror = err;
      }
      destruct(waitforauthready);
      c->purge();
    };
  }

  final void close() {
    throwdelayederror(this);
    {
      Thread.MutexKey lock;
      if (unnamedstatement)
        termlock = unnamedstatement->lock(1);
      foreach (c->runningportals; Result result; )
        catch(result->status_command_complete());
      if (c)				// Prevent trivial backtraces
        c->close();
      if (unnamedstatement)
        lock = unnamedstatement->lock(1);
      if (c)
        c->purge();
    }
    destruct(waitforauthready);
  }

  private void _destruct() {
    string errstring;
    mixed err = catch(close());
    backendreg = 0;
    if (untolderror) {
      /*
       * Flush out any asynchronously reported errors to stderr; because we are
       * inside a destructor, throwing an error will not work anymore.
       * Warnings will be silently discarded at this point.
       */
      lastmessage = filter(lastmessage, lambda(string val) {
        return has_prefix(val, "ERROR ") || has_prefix(val, "FATAL "); });
      if (err || (err = catch(errstring = geterror(1))))
        werror(describe_backtrace(err));
      else if (errstring && sizeof(errstring))
        werror("%s\n", errstring);	// Add missing terminating newline
    }
  }

  final void sendsync() {
    readyforquerycount++;
    c->start()->sendcmd(SYNCSEND);
  }

  private void runcallback(int pid, string condition, string extrainfo) {
    array cb;
    if (condition == "_lost")
      destruct(c);
    if ((cb = notifylist[condition] || notifylist[""])
       && (pid != backendpid || sizeof(cb) > 1 && cb[1]))
      callout(cb[0], 0, pid, condition, extrainfo, @cb[2..]);
  }

  private inline void closestatement(
   bufcon|conxsess plugbuffer, string oldprep) {
    closestatement(plugbuffer, oldprep);
  }
};

#pragma deprecation_warnings

//! @class sql_result
//! @deprecated Result

//! @endclass

__deprecated__(program(Result)) sql_result =
  (__deprecated__(program(Result)))Result;
