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

//! The instance of the pgsql dedicated backend.
final Pike.Backend local_backend = Pike.SmallBackend();

private Thread.Mutex backendmux = Thread.Mutex();
private int clientsregistered;

constant emptyarray = ({});
constant describenodata
 = (["datarowdesc":emptyarray, "datarowtypes":emptyarray,
     "datatypeoid":emptyarray]);
final multiset censoroptions=(<"use_ssl","force_ssl",
 "cache_autoprepared_statements","reconnect","text_query","is_superuser",
 "server_encoding","server_version","integer_datetimes",
 "session_authorization">);
constant stdiobuftype = typeof(Stdio.Buffer());

 /* Statements matching createprefix cause the prepared statement cache
  * to be flushed to prevent stale references to (temporary) tables
  */
final Regexp createprefix=iregexp("^\a*(CREATE|DROP)\a");

 /* Statements matching dontcacheprefix never enter the cache
  */
private Regexp dontcacheprefix=iregexp("^\a*(FETCH|COPY)\a");

 /* Statements not matching paralleliseprefix will cause the driver
  * to stall submission until all previously started statements have
  * run to completion
  */
private Regexp paralleliseprefix
 =iregexp("^\a*((SELEC|INSER)T|(UPDA|DELE)TE|(FETC|WIT)H)\a");

 /* Statements matching transbeginprefix will cause the driver
  * insert a sync after the statement.
  * Failure to do so, will result in portal synchronisation errors
  * in the event of an ErrorResponse.
  */
final Regexp transbeginprefix
 =iregexp("^\a*(BEGIN|START)([; \t\f\r\n]|$)");

 /* Statements matching transendprefix will cause the driver
  * insert a sync after the statement.
  * Failure to do so, will result in portal synchronisation errors
  * in the event of an ErrorResponse.
  */
final Regexp transendprefix
 =iregexp("^\a*(COMMIT|ROLLBACK|END)([; \t\f\r\n]|$)");

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
 =iregexp("^\a*((UPDA|DELE)TE|INSERT)\a|\aLIMIT\a+[1-9][; \t\f\r\n]*$");

private Regexp iregexp(string expr) {
  Stdio.Buffer ret=Stdio.Buffer();
  foreach(expr;;int c)
    if(c>='A'&&c<='Z')
      ret->add('[',c,c+'a'-'A',']');
    else if(c=='\a')			// Replace with generic whitespace
      ret->add("[ \t\f\r\n]");
    else
      ret->add_int8(c);
  return Regexp(ret->read());
}

final void closestatement(bufcon|conxsess plugbuffer,string oldprep) {
  if(oldprep) {
    PD("Close statement %s\n",oldprep);
    CHAIN(plugbuffer)->add_int8('C')->add_hstring(({'S', oldprep, 0}), 4, 4);
  }
}

private void run_local_backend() {
  Thread.MutexKey lock;
  int looponce;
  do {
    looponce=0;
    if(lock=backendmux->trylock()) {
      PD("Starting local backend\n");
      while (clientsregistered) {	// Autoterminate when not needed
        mixed err;
        if (err = catch(local_backend(4096.0)))
          werror(describe_backtrace(err));
      }
      PD("Terminating local backend\n");
      lock=0;
      looponce=clientsregistered;
    }
  } while(looponce);
}

//! Registers yourself as a user of this backend.  If the backend
//! has not been started yet, it will be spawned automatically.
final void register_backend() {
  if(!clientsregistered++)
    Thread.Thread(run_local_backend);
}

//! Unregisters yourself as a user of this backend.  If there are
//! no longer any registered users, the backend will be terminated.
final void unregister_backend() {
  --clientsregistered;
}

final void throwdelayederror(object parent) {
  if(mixed err=parent._delayederror) {
    parent._delayederror=UNDEFINED;
    if(stringp(err))
      err=({err,backtrace()[..<2]});
    throw(err);
  }
}

final int oidformat(int oid) {
  switch(oid) {
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
  }
  return 0;	// text
}

private inline mixed callout(function(mixed ...:void) f,
 float|int delay,mixed ... args) {
  return local_backend->call_out(f,delay,@args);
}

// Some pgsql utility functions

class bufcon {
  inherit Stdio.Buffer;

#ifdef PG_DEBUGRACE
  final bufcon `chain() {
    return this;
  }
#endif

  private conxion realbuffer;

  protected void create(conxion _realbuffer) {
    realbuffer=_realbuffer;
  }

  final int `stashcount() {
    return realbuffer->stashcount;
  }

  final bufcon start(void|int waitforreal) {
    realbuffer->stashcount++;
#ifdef PG_DEBUG
    if(waitforreal)
      error("pgsql.bufcon not allowed here\n");
#endif
    return this;
  }

  final void sendcmd(int mode,void|sql_result portal) {
    Thread.MutexKey lock=realbuffer->shortmux->lock();
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
    if(!--realbuffer->stashcount)
      realbuffer->stashavail.signal();
    lock=0;
    this->clear();
    if(lock=realbuffer->nostash->trylock(1)) {
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
  final Thread.Mutex fillreadmux;
  final int procmsg;
  private int didreadcb;

  protected bool range_error(int howmuch) {
#ifdef PG_DEBUG
    if(howmuch<=0)
      error("Out of range %d\n",howmuch);
#endif
    if(fillread) {
      Thread.MutexKey lock=fillreadmux->lock();
      if(!didreadcb)
        fillread.wait(lock);
      didreadcb=0;
      lock=0;
    } else
      throw(MAGICTERMINATE);
    return true;
  }

  final int read_cb(mixed id,mixed b) {
    PD("Read callback %O\n", b && ((string)b)
#ifndef PG_DEBUGMORE
      [..255]
#endif
     );
    Thread.MutexKey lock=fillreadmux->lock();
    if(procmsg&&id)
      procmsg=0,lock=0,Thread.Thread(id);
    else if(fillread)
      didreadcb=1, fillread.signal();
    lock=0;
    return 0;
  }

  protected void create() {
    i::create();
    fillreadmux=Thread.Mutex();
    fillread=Thread.Condition();
  }
};

class sfile {
  inherit Stdio.File;
  int query_fd() {
    return is_open() ? ::query_fd() : -1;
  }
};

class conxion {
  inherit Stdio.Buffer:o;
  final conxiin i;

  private Thread.Queue qportals;
  final Thread.Mutex shortmux;
  private int closenext;

  final sfile socket;
  private function(void|mixed:void) connectfail;
  private int towrite;
  final multiset(function(void|mixed:void)) closecallbacks=(<>);

  final Thread.Mutex nostash;
  final Thread.MutexKey started;
  final Thread.Queue stashqueue;
  final Thread.Condition stashavail;
  final Stdio.Buffer stash;
  final int stashflushmode;
  final int stashcount;
  final int synctransact;
#ifdef PG_DEBUGRACE
  final mixed nostrack;
#endif
#ifdef PG_DEBUG
  final int queueoutidx;
  final int queueinidx=-1;
#endif

  private inline void queueup(sql_result portal) {
    qportals->write(portal); portal->_synctransact=synctransact;
    PD("%d>%O %d %d Queue portal %d bytes\n",socket->query_fd(),
     portal._portalname,++queueoutidx,synctransact,sizeof(this));
  }

  final bufcon|conxsess start(void|int waitforreal) {
    Thread.MutexKey lock;
    if(lock=(waitforreal?nostash->lock:nostash->trylock)(1)) {
      int mode;
#ifdef PG_DEBUGRACE
      conxsess sess = conxsess(this);
#endif
      started = lock;
      lock=shortmux->lock();
      if(stashcount)
        PT(stashavail.wait(lock));
      mode = getstash(KEEP);
      lock=0;
      if (mode > KEEP)
        sendcmd(mode);			// Force out stash to the server
#ifdef PG_DEBUGRACE
      return sess;
#else
      return this;
#endif
    }
    stashcount++;
    return bufcon(this);
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
    if (sizeof(stash)) {
      add(stash); stash->clear();
      foreach (stashqueue->try_read_array();; int|sql_result portal)
        if (intp(portal))
          qportals->write(synctransact++);
        else
          queueup(portal);
      PD("%d>Got stash mode %d > %d\n",
       socket->query_fd(), stashflushmode, mode);
      if (stashflushmode > mode)
        mode = stashflushmode;
      stashflushmode = KEEP;
    }
    return mode;
  }

  final void sendcmd(void|int mode, void|sql_result portal) {
    Thread.MutexKey lock;
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
    } while(0);
    lock = shortmux->lock();
    mode = getstash(mode);
    catch {
outer:
      do {
        switch(mode) {
          default:
            PD("%d>Skip flush %d Queue %O\n",
             socket->query_fd(), mode, (string)this);
            break outer;
          case FLUSHSEND:
            PD("Flush\n");
            add(PGFLUSH);
          case SENDOUT:;
        }
        if(towrite=sizeof(this)) {
          PD("%d>Sendcmd %O\n",socket->query_fd(),((string)this)[..towrite-1]);
          towrite-=output_to(socket,towrite);
        }
      } while(0);
      lock=started=0;
      return;
    };
    lock=0;
    catch(connectfail());
  }

  final int close() {
    if(!closenext && nostash) {
      closenext=1;
      Thread.MutexKey lock=i->fillreadmux->lock();
      if(i->fillread) {	 // Delayed close() after flushing the output buffer
        i->fillread.signal();
        i->fillread=0;
      }
      lock=0;
      PD("%d>Delayed close, flush write\n",socket->query_fd());
      i->read_cb(socket->query_id(),0);
      return 0;
    } else
      return -1;
  }

  protected void _destruct() {
    PD("%d>Close conxion %d\n", socket ? socket->query_fd() : -1, !!nostash);
    int|.pgsql_util.sql_result portal;
    if (qportals)			// CancelRequest does not use qportals
      while (portal = qportals->try_read())
        if (objectp(portal))
          portal->_purgeportal();
    if(nostash) {
      catch {
        while(sizeof(closecallbacks))
          foreach(closecallbacks;function(void|mixed:void) closecb;)
            closecb();
        destruct(nostash);
        socket->set_nonblocking();		// Drop all callbacks
        PD("%d>Close socket\n",socket->query_fd());
        socket->close();
      };
    }
    connectfail=0;
  }

  final void connectloop(object pgsqlsess, int nossl) {
    mixed err=catch {
      for(;;clear()) {
        socket->connect(pgsqlsess._host,pgsqlsess._port);
#if constant(SSL.File)
        if(!nossl && !pgsqlsess->nossl
         && (pgsqlsess._options.use_ssl || pgsqlsess._options.force_ssl)) {
          PD("SSLRequest\n");
          start()->add_int32(8)->add_int32(PG_PROTOCOL(1234,5679))
           ->sendcmd(SENDOUT);
          switch(read_int8()) {
            case 'S':
              object fcon=SSL.File(socket,SSL.Context());
              if(fcon->connect()) {
                socket=fcon;
                break;
              }
            default:
              PD("%d>Close socket short\n", socket->query_fd());
              socket->close();
              pgsqlsess.nossl=1;
              continue;
            case 'N':
              if(pgsqlsess._options.force_ssl)
                error("Encryption not supported on connection to %s:%d\n",
                      pgsqlsess.host,pgsqlsess.port);
          }
        }
#else
        if(pgsqlsess._options.force_ssl)
          error("Encryption library missing,"
                " cannot establish connection to %s:%d\n",
                pgsqlsess.host,pgsqlsess.port);
#endif
        break;
      }
      connectfail=pgsqlsess->_connectfail;
      if(!socket->is_open())
        error(strerror(socket->errno())+".\n");
      socket->set_backend(local_backend);
      socket->set_buffer_mode(i,0);
      socket->set_nonblocking(i->read_cb,write_cb,close);
      if (nossl != 2) {
        connectfail=pgsqlsess->_connectfail;
        Thread.Thread(pgsqlsess->_processloop,this);
      }
      return;
    };
    catch(connectfail(err));
  }

  private string _sprintf(int type) {
    string res=UNDEFINED;
    switch(type) {
      case 'O':
        int fd=-1;
        if(socket)
          catch(fd=socket->query_fd());
        res=predef::sprintf("conxion  fd: %d input queue: %d/%d "
                    "queued portals: %d  output queue: %d/%d\n"
                    "started: %d\n",
                    fd,sizeof(i),i->_size_object(),
                    qportals && qportals->size(), sizeof(this), _size_object(),
                    !!started);
        break;
    }
    return res;
  }

  protected void create(object pgsqlsess,Thread.Queue _qportals,int nossl) {
    o::create();
    qportals = _qportals;
    synctransact = 1;
    socket=sfile();
    i=conxiin();
    shortmux=Thread.Mutex();
    nostash=Thread.Mutex();
    closenext = 0;
    stashavail=Thread.Condition();
    stashqueue=Thread.Queue();
    stash=Stdio.Buffer();
    Thread.Thread(connectloop,pgsqlsess,nossl);
  }
};

#ifdef PG_DEBUGRACE
class conxsess {
  final conxion chain;

  protected void create(conxion parent) {
    if (parent->started)
      werror("Overwriting conxsess %s %s\n",
        describe_backtrace(({"new ", backtrace()[..<1]})),
        describe_backtrace(({"old ", parent->nostrack})));
    parent->nostrack = backtrace();
    chain = parent;
  }

  final void sendcmd(int mode,void|sql_result portal) {
    chain->sendcmd(mode, portal);
    chain = 0;
  }

  protected void _destruct() {
    if (chain)
      werror("Untransmitted conxsess %s\n",
       describe_backtrace(({"", backtrace()[..<1]})));
  }
};
#endif

//! The result object returned by @[Sql.pgsql()->big_query()], except for
//! the noted differences it behaves the same as @[Sql.sql_result].
//!
//! @seealso
//!   @[Sql.sql_result], @[Sql.pgsql], @[Sql.Sql], @[Sql.pgsql()->big_query()]
class sql_result {

  inherit __builtin.Sql.Result;

  private object pgsqlsess;
  private int eoffound;
  private conxion c;
  private conxiin cr;
  final mixed _delayederror;
  final int _state;
  final int _fetchlimit;
  private int alltext;
  final int _forcetext;
  private int syncparse;
  private int transtype;

  final string _portalname;

  private int rowsreceived;
  private int inflight;
  private int portalbuffersize;
  private Thread.Mutex closemux;
  private Thread.Queue datarows;
  private array(mapping(string:mixed)) datarowdesc;
  private array(int) datarowtypes;	// types from datarowdesc
  private string statuscmdcomplete;
  private int bytesreceived;
  final int _synctransact;
  final Thread.Condition _ddescribe;
  final Thread.Mutex _ddescribemux;
  final Thread.MutexKey _unnamedportalkey,_unnamedstatementkey;
  final array _params;
  final string _query;
  final string _preparedname;
  final mapping(string:mixed) _tprepared;
  private function(:void) gottimeout;
  private int timeout;

  private string _sprintf(int type) {
    string res=UNDEFINED;
    switch(type) {
      case 'O':
        int fd=-1;
        if(c&&c->socket)
          catch(fd=c->socket->query_fd());
        res=sprintf("sql_result state: %d numrows: %d eof: %d inflight: %d\n"
                    "query: %O\n"
                    "fd: %O portalname: %O  datarows: %d"
                    "  synctransact: %d laststatus: %s\n",
                    _state,rowsreceived,eoffound,inflight,
                    _query,fd,_portalname,datarowtypes&&sizeof(datarowtypes),
                    _synctransact,
                    statuscmdcomplete||(_unnamedstatementkey?"*parsing*":""));
        break;
    }
    return res;
  }

  protected void create(object _pgsqlsess,conxion _c,string query,
   int _portalbuffersize,int alltyped,array params,int forcetext,
   int _timeout, int _syncparse, int _transtype) {
    pgsqlsess = _pgsqlsess;
    cr = (c = _c)->i;
    _query = query;
    datarows = Thread.Queue();
    _ddescribe=Thread.Condition();
    _ddescribemux=Thread.Mutex();
    closemux=Thread.Mutex();
    portalbuffersize=_portalbuffersize;
    alltext = !alltyped;
    _params = params;
    _forcetext = forcetext;
    _state = PORTALINIT;
    timeout = _timeout;
    syncparse = _syncparse;
    gottimeout = _pgsqlsess->cancelquery;
    c->closecallbacks+=(<_destruct>);
    transtype = _transtype;
  }

  //! Returns the command-complete status for this query.
  //!
  //! @seealso
  //!  @[affected_rows()]
  //!
  //! @note
  //! This function is PostgreSQL-specific, and thus it is not available
  //! through the generic SQL-interface.
  /*semi*/final string status_command_complete() {
    return statuscmdcomplete;
  }

  //! Returns the number of affected rows by this query.
  //!
  //! @seealso
  //!  @[status_command_complete()]
  //!
  //! @note
  //! This function is PostgreSQL-specific, and thus it is not available
  //! through the generic SQL-interface.
  /*semi*/final int affected_rows() {
    int rows;
    if(statuscmdcomplete)
      sscanf(statuscmdcomplete,"%*s %d",rows);
    return rows;
  }

  final void _storetiming() {
    if(_tprepared) {
      _tprepared.trun=gethrtime()-_tprepared.trunstart;
      m_delete(_tprepared,"trunstart");
      _tprepared = UNDEFINED;
    }
  }

  private void waitfordescribe() {
    Thread.MutexKey lock=_ddescribemux->lock();
    if(!datarowtypes)
      PT(_ddescribe->wait(lock));
    lock=0;
  }

  //! @seealso
  //!  @[Sql.sql_result()->num_fields()]
  /*semi*/final int num_fields() {
    if(!datarowtypes)
      waitfordescribe();
    trydelayederror();
    return sizeof(datarowtypes);
  }

  //! @note
  //!  This method returns the number of rows already received from
  //!  the database for the current query.  Note that this number
  //!  can still increase between subsequent calls if the results from
  //!  the query are not complete yet.  This function is only guaranteed to
  //!  return the correct count after EOF has been reached.
  //! @seealso
  //!  @[Sql.sql_result()->num_rows()]
  /*semi*/final int num_rows() {
    trydelayederror();
    return rowsreceived;
  }

  private inline void trydelayederror() {
    if(_delayederror)
      throwdelayederror(this);
  }

  //! @seealso
  //!  @[Sql.sql_result()->eof()]
  /*semi*/final int eof() {
    trydelayederror();
    return eoffound;
  }

  //! @seealso
  //!  @[Sql.sql_result()->fetch_fields()]
  /*semi*/final array(mapping(string:mixed)) fetch_fields() {
    if(!datarowtypes)
      waitfordescribe();
    trydelayederror();
    return datarowdesc+emptyarray;
  }

#ifdef PG_DEBUG
#define INTVOID int
#else
#define INTVOID void
#endif
  final INTVOID _decodedata(int msglen,string cenc) {
    _storetiming(); _releasestatement();
    string serror;
    bytesreceived+=msglen;
    int cols=cr->read_int16();
    array a=allocate(cols,!alltext&&Val.null);
#ifdef PG_DEBUG
    msglen-=2+4*cols;
#endif
    foreach(datarowtypes;int i;int typ) {
      int collen=cr->read_sint(4);
      if(collen>0) {
#ifdef PG_DEBUG
        msglen-=collen;
#endif
        mixed value;
        switch(typ) {
          case FLOAT4OID:
#if SIZEOF_FLOAT>=8
          case FLOAT8OID:
#endif
            if(!alltext) {
              value=(float)cr->read(collen);
              break;
            }
          default:value=cr->read(collen);
            break;
          case CHAROID:
            value=alltext?cr->read(1):cr->read_int8();
            break;
          case BOOLOID:value=cr->read_int8();
            switch(value) {
              case 'f':value=0;
                break;
              case 't':value=1;
            }
            if(alltext)
              value=value?"t":"f";
            break;
          case TEXTOID:
          case BPCHAROID:
          case VARCHAROID:
            value=cr->read(collen);
            if(cenc==UTF8CHARSET && catch(value=utf8_to_string(value))
             && !serror)
              serror=SERROR("%O contains non-%s characters\n",
                                                     value,UTF8CHARSET);
            break;
          case INT8OID:case INT2OID:
          case OIDOID:case INT4OID:
            if(_forcetext) {
              value=cr->read(collen);
              if(!alltext)
                value=(int)value;
            } else {
              switch(typ) {
                case INT8OID:value=cr->read_sint(8);
                  break;
                case INT2OID:value=cr->read_sint(2);
                  break;
                case OIDOID:
                case INT4OID:value=cr->read_sint(4);
              }
              if(alltext)
                value=(string)value;
            }
        }
        a[i]=value;
      } else if(!collen)
        a[i]="";
    }
    _processdataready(a);
    if(serror)
      error(serror);
#ifdef PG_DEBUG
    return msglen;
#endif
  }

  final void _setrowdesc(array(mapping(string:mixed)) drowdesc,
   array(int) drowtypes) {
    Thread.MutexKey lock=_ddescribemux->lock();
    datarowdesc=drowdesc;
    datarowtypes=drowtypes;
    _ddescribe->broadcast();
    lock=0;
  }

  final void _preparebind(array dtoid) {
    array(string|int) paramValues=_params?_params[2]:({});
    if(sizeof(dtoid)!=sizeof(paramValues))
      SUSERERROR("Invalid number of bindings, expected %d, got %d\n",
                 sizeof(dtoid),sizeof(paramValues));
    Thread.MutexKey lock=_ddescribemux->lock();
    if(!_portalname) {
      _portalname=(_unnamedportalkey=pgsqlsess._unnamedportalmux->trylock(1))
         ? "" : PORTALPREFIX
#ifdef PG_DEBUG
          +(string)(c->socket->query_fd())+"_"
#endif
          +int2hex(pgsqlsess._pportalcount++);
      lock=0;
#ifdef PG_DEBUGMORE
      PD("ParamValues to bind: %O\n",paramValues);
#endif
      Stdio.Buffer plugbuffer=Stdio.Buffer();
      { array dta=({sizeof(dtoid)});
        plugbuffer->add(_portalname,0,_preparedname,0)
         ->add_ints(dta+map(dtoid,oidformat)+dta,2);
      }
      string cenc=pgsqlsess._runtimeparameter[CLIENT_ENCODING];
      foreach(paramValues;int i;mixed value) {
        if(undefinedp(value) || objectp(value)&&value->is_val_null)
          plugbuffer->add_int32(-1);				// NULL
        else if(stringp(value) && !sizeof(value)) {
          int k=0;
          switch(dtoid[i]) {
            default:
              k=-1;	     // cast empty strings to NULL for non-string types
            case BYTEAOID:
            case TEXTOID:
            case XMLOID:
            case BPCHAROID:
            case VARCHAROID:;
          }
          plugbuffer->add_int32(k);
        } else
          switch(dtoid[i]) {
            case TEXTOID:
            case BPCHAROID:
            case VARCHAROID: {
              if(!value) {
                plugbuffer->add_int32(-1);
                break;
              }
              value=(string)value;
              switch(cenc) {
                case UTF8CHARSET:
                  value=string_to_utf8(value);
                  break;
                default:
                  if(String.width(value)>8) {
                    SUSERERROR("Don't know how to convert %O to %s encoding\n",
                               value,cenc);
                    value="";
                  }
              }
              plugbuffer->add_hstring(value,4);
              break;
            }
            default: {
              if(!value) {
                plugbuffer->add_int32(-1);
                break;
              }
	      if (!objectp(value) || typeof(value) != stdiobuftype)
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
                        value,dtoid[i]);
                      value="";
                    }
	        }
              plugbuffer->add_hstring(value,4);
              break;
            }
            case BOOLOID:
              do {
                int tval;
                if(stringp(value))
                  tval=value[0];
                else if(!intp(value)) {
                  value=!!value;			// cast to boolean
                  break;
                } else
                  tval=value;
                switch(tval) {
                  case 'o':case 'O':
                    catch {
                      tval=value[1];
                      value=tval=='n'||tval=='N';
                    };
                    break;
                  default:
                    value=1;
                    break;
                  case 0:case '0':case 'f':case 'F':case 'n':case 'N':
                    value=0;
                      break;
                }
              } while(0);
              plugbuffer->add_int32(1)->add_int8(value);
              break;
            case CHAROID:
              if(intp(value))
                plugbuffer->add_hstring(value,4);
              else {
                value=(string)value;
                switch(sizeof(value)) {
                  default:
                    SUSERERROR(
                     "\"char\" types must be 1 byte wide, got %O\n",value);
                  case 0:
                    plugbuffer->add_int32(-1);			// NULL
                    break;
                  case 1:
                    plugbuffer->add_hstring(value[0],4);
                }
              }
              break;
            case INT8OID:
              plugbuffer->add_int32(8)->add_int((int)value,8);
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
      if(!datarowtypes) {
        if(_tprepared && dontcacheprefix->match(_query))
          m_delete(pgsqlsess->_prepareds,_query),_tprepared=0;
        waitfordescribe();
      }
      if(_state>=CLOSING)
        lock=_unnamedstatementkey=0;
      else {
        plugbuffer->add_int16(sizeof(datarowtypes));
        if(sizeof(datarowtypes))
          plugbuffer->add_ints(map(datarowtypes,oidformat),2);
        else if (syncparse < 0 && !pgsqlsess->_wasparallelisable
         && pgsqlsess->_statementsinflight > 1) {
          lock=pgsqlsess->_shortmux->lock();
          // Decrement temporarily to account for ourselves
          if(--pgsqlsess->_statementsinflight) {
            pgsqlsess->_waittocommit++;
            PD("Commit waiting for statements to finish\n");
            catch(PT(pgsqlsess->_readyforcommit->wait(lock)));
            pgsqlsess->_waittocommit--;
          }
          // Increment again to account for ourselves
          pgsqlsess->_statementsinflight++;
        }
        lock=0;
        PD("Bind portal %O statement %O\n",_portalname,_preparedname);
        _fetchlimit=pgsqlsess->_fetchlimit;
        _bindportal();
        conxsess bindbuffer = c->start();
        _unnamedstatementkey=0;
        CHAIN(bindbuffer)->add_int8('B')->add_hstring(plugbuffer, 4, 4);
        if(!_tprepared && sizeof(_preparedname))
          closestatement(CHAIN(bindbuffer), _preparedname);
        _sendexecute(_fetchlimit
                             && !(transtype != NOTRANS
                                  || sizeof(_query)>=MINPREPARELENGTH &&
                                  execfetchlimit->match(_query))
                             && _fetchlimit,bindbuffer);
      }
    } else
      lock=0;
  }

  final void _processrowdesc(array(mapping(string:mixed)) datarowdesc,
   array(int) datarowtypes) {
    _setrowdesc(datarowdesc,datarowtypes);
    if(_tprepared) {
      _tprepared.datarowdesc=datarowdesc;
      _tprepared.datarowtypes=datarowtypes;
    }
  }

  final void _parseportal() {
    Thread.MutexKey lock = closemux->lock();
    _state=PARSING;
    Thread.MutexKey lockc = pgsqlsess->_shortmux->lock();
    if (syncparse || syncparse < 0 && pgsqlsess->_wasparallelisable) {
      if(pgsqlsess->_statementsinflight) {
        pgsqlsess->_waittocommit++;
        PD("Commit waiting for statements to finish\n");
        // Do NOT put this in a function, it would require passing a lock
        // variable on the argumentstack to the function, which will cause
        // unpredictable lock release issues due to the extra copy on the stack
        catch(PT(pgsqlsess->_readyforcommit->wait(lockc)));
        pgsqlsess->_waittocommit--;
      }
    }
    pgsqlsess->_statementsinflight++;
    lockc = 0;
    lock=0;
    statuscmdcomplete=UNDEFINED;
    pgsqlsess->_wasparallelisable = paralleliseprefix->match(_query);
  }

  final void _releasestatement(void|int nolock) {
    Thread.MutexKey lock;
    if (!nolock)
      lock = closemux->lock();
    if (_state <= BOUND) {
      _state = COMMITTED;
      lock = pgsqlsess->_shortmux->lock();
      if (!--pgsqlsess->_statementsinflight && pgsqlsess->_waittocommit) {
        PD("Signal no statements in flight\n");
        catch(pgsqlsess->_readyforcommit->signal());
      }
    }
    lock = 0;
  }

  final void _bindportal() {
    Thread.MutexKey lock = closemux->lock();
    _state=BOUND;
    Thread.MutexKey lockc = pgsqlsess->_shortmux->lock();
    pgsqlsess->_portalsinflight++;
    lockc = 0;
    lock=0;
  }

  final void _purgeportal() {
    PD("Purge portal\n");
    datarows->write(1);				   // Signal EOF
    Thread.MutexKey lock=closemux->lock();
    _fetchlimit=0;				   // disables further Executes
    switch(_state) {
      case COPYINPROGRESS:
      case COMMITTED:
      case BOUND:
        --pgsqlsess->_portalsinflight;
    }
    switch(_state) {
      case BOUND:
      case PARSING:
        --pgsqlsess->_statementsinflight;
    }
    _state=CLOSED;
    lock=0;
    releaseconditions();
  }

  final int _closeportal(conxsess cs) {
    object plugbuffer = CHAIN(cs);
    int retval=KEEP;
    PD("%O Try Closeportal %d\n",_portalname,_state);
    Thread.MutexKey lock=closemux->lock();
    _fetchlimit=0;				   // disables further Executes
    switch(_state) {
      case PARSING:
      case BOUND:
        _releasestatement(1);
    }
    switch(_state) {
      case PORTALINIT:
      case PARSING:
        _unnamedstatementkey=0;
        _state=CLOSING;
        break;
      case COPYINPROGRESS:
        PD("CopyDone\n");
        plugbuffer->add("c\0\0\0\4");
      case COMMITTED:
      case BOUND:
        _state=CLOSING;
        lock=0;
        PD("Close portal %O\n",_portalname);
        if (_portalname && sizeof(_portalname)) {
          plugbuffer->add_int8('C')->add_hstring(({'P',_portalname,0}),4,4);
          retval=FLUSHSEND;
        } else
          _unnamedportalkey=0;
        Thread.MutexKey lockc=pgsqlsess->_shortmux->lock();
        if(!--pgsqlsess->_portalsinflight) {
          if(!pgsqlsess->_waittocommit && !plugbuffer->stashcount
           && transtype != TRANSBEGIN)
           /*
            * stashcount will be non-zero if a parse request has been queued
            * before the close was initiated.
            * It's a bit of a tricky race, but this check should be sufficient.
            */
            pgsqlsess->_readyforquerycount++, retval=SYNCSEND;
          pgsqlsess->_pportalcount=0;
        }
        lockc=0;
    }
    lock=0;
    return retval;
  }

  final void _processdataready(array datarow,void|int msglen) {
    bytesreceived+=msglen;
    inflight--;
    if(_state<CLOSED)
      datarows->write(datarow);
    if(++rowsreceived==1)
      PD("<%O _fetchlimit %d=min(%d||1,%d), inflight %d\n",_portalname,
       _fetchlimit,(portalbuffersize>>1)*rowsreceived/bytesreceived,
       pgsqlsess._fetchlimit,inflight);
    if(_fetchlimit) {
      _fetchlimit=
       min((portalbuffersize>>1)*rowsreceived/bytesreceived||1,
        pgsqlsess._fetchlimit);
      Thread.MutexKey lock=closemux->lock();
      if(_fetchlimit && inflight<=(_fetchlimit-1)>>1)
        _sendexecute(_fetchlimit);
      else if(!_fetchlimit)
        PD("<%O _fetchlimit %d, inflight %d, skip execute\n",
         _portalname,_fetchlimit,inflight);
      lock=0;
    }
  }

  private void releaseconditions() {
    _unnamedportalkey=_unnamedstatementkey=0;
    pgsqlsess=0;
    if(!datarowtypes) {
      Thread.MutexKey lock=_ddescribemux->lock();
      datarowtypes=emptyarray;
      datarowdesc=emptyarray;
      _ddescribe->broadcast();
      lock=0;
    }
  }

  final void _releasesession(void|string statusccomplete) {
    c->closecallbacks-=(<_destruct>);
    if(statusccomplete && !statuscmdcomplete)
      statuscmdcomplete=statusccomplete;
    inflight=0;
    datarows->write(1);				// Signal EOF
    conxsess plugbuffer;
    if (!catch(plugbuffer = c->start()))
      plugbuffer->sendcmd(_closeportal(plugbuffer));
    _state=CLOSED;
    releaseconditions();
  }

  protected void _destruct() {
    catch {			   // inside destructors, exceptions don't work
      _releasesession();
    };
  }

  final void _sendexecute(int fetchlimit,void|bufcon|conxsess plugbuffer) {
    int flushmode;
    PD("Execute portal %O fetchlimit %d transtype %d\n", _portalname,
     fetchlimit, transtype);
    if(!plugbuffer)
      plugbuffer=c->start(1);
    CHAIN(plugbuffer)->add_int8('E')->add_hstring(({_portalname,0}), 4, 8)
     ->add_int32(fetchlimit);
    if (!fetchlimit) {
      if (transtype != NOTRANS)
        pgsqlsess._intransaction = transtype == TRANSBEGIN;
      flushmode = _closeportal(plugbuffer) == SYNCSEND
       || transtype == TRANSEND ? SYNCSEND : FLUSHSEND;
    } else
      inflight+=fetchlimit, flushmode=FLUSHSEND;
    plugbuffer->sendcmd(flushmode,this);
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
    if(arrayp(datarow=datarows->try_read()))
      return datarow;
    if(!eoffound) {
      if(!datarow) {
        PD("%O Block for datarow\n",_portalname);
        array cid=callout(gottimeout,timeout);
        PT(datarow=datarows->read());
        local_backend->remove_call_out(cid);
        if(arrayp(datarow))
          return datarow;
      }
      eoffound=1;
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
    if(eoffound)
      return 0;
    array(array|int) datarow=datarows->try_read_array();
    if(!datarow) {
      array cid=callout(gottimeout,timeout);
      PT(datarow=datarows->read_array());
      local_backend->remove_call_out(cid);
    }
    if(arrayp(datarow[-1]))
      return datarow;
    trydelayederror();
    eoffound=1;
    datarows->write(1);				// Signal EOF for other threads
    return (datarow=datarow[..<1]);
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
    if(copydata) {
      PD("CopyData\n");
      object cs = c->start();
      CHAIN(cs)->add_int8('d')->add_hstring(copydata, 4, 4);
      cs->sendcmd(SENDOUT);
    } else
      _releasesession();
  }

  private void run_result_cb(
   function(sql_result, array(mixed), mixed ...:void) callback,
   array(mixed) args) {
    int|array datarow;
    for(;;) {
      array cid=callout(gottimeout,timeout);
      PT(datarow=datarows->read());
      local_backend->remove_call_out(cid);
      if(!arrayp(datarow))
        break;
      callout(callback, 0, this, datarow, @args);
    }
    trydelayederror();
    eoffound=1;
    callout(callback, 0, this, 0, @args);
  }

  //! Sets up a callback for every row returned from the database.
  //! First argument passed is the resultobject itself, second argument
  //! is the result row (zero on EOF).
  //!
  //! @seealso
  //!  @[fetch_row()]
  /*semi*/final void set_result_callback(
   function(sql_result, array(mixed), mixed ...:void) callback,
   mixed ... args) {
    if(callback)
      Thread.Thread(run_result_cb,callback,args);
  }

  private void run_result_array_cb(
   function(sql_result, array(array(mixed)), mixed ...:void) callback,
   array(mixed) args) {
    array(array|int) datarow;
    for(;;) {
      array cid=callout(gottimeout,timeout);
      PT(datarow=datarows->read_array());
      local_backend->remove_call_out(cid);
      if(!datarow || !arrayp(datarow[-1]))
        break;
      callout(callback, 0, this, datarow, @args);
    }
    trydelayederror();
    eoffound=1;
    if(sizeof(datarow)>1)
      callout(callback, 0, this, datarow=datarow[..<1], @args);
    callout(callback, 0, this, 0, @args);
  }

  //! Sets up a callback for sets of rows returned from the database.
  //! First argument passed is the resultobject itself, second argument
  //! is the array of result rows (zero on EOF).
  //!
  //! @seealso
  //!  @[fetch_row()]
  /*semi*/final void set_result_array_callback(
   function(sql_result, array(array(mixed)), mixed ...:void) callback,
   mixed ... args) {
    if(callback)
      Thread.Thread(run_result_array_cb,callback,args);
  }

};
