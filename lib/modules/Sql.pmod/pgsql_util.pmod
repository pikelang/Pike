/*
 * Some pgsql utility functions.
 * They are kept here to avoid circular references.
 */

#pike __REAL_VERSION__

#include "pgsql.h"

//!
//! The pgsql backend, shared between all connection instances.
//! It even runs in non-callback mode.
//!

protected Thread.Mutex backendmux;
final Pike.Backend local_backend;
protected int clientsregistered;

protected void run_local_backend() {
  Thread.MutexKey lock;
  int looponce;
  do {
    looponce=0;
    if(lock=backendmux->trylock()) {
      PD("Starting local backend\n");
      while(clientsregistered)		// Autoterminate when not needed
        local_backend(4096.0);
      PD("Terminating local backend\n");
      lock=0;
      looponce=clientsregistered;
    }
  } while(looponce);
}

protected void create() {
  backendmux = Thread.Mutex();
  local_backend = Pike.SmallBackend();
}

final void register_backend() {
  if(!clientsregistered++)
    Thread.Thread(run_local_backend);
}

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

protected sctype mergemode(PGassist realbuffer,sctype mode) {
  switch(realbuffer->stashflushmode) {
    case sendout:
      if(mode!=flushsend) {
        mode=sendout;
        break;
      }
    case flushsend:
      mode=flushsend;
  }
  return realbuffer->stashflushmode=mode;
}

//! Some pgsql utility functions

class PGplugbuffer {
  inherit Stdio.Buffer;

  protected PGassist realbuffer;

  final void create(PGassist _realbuffer) {
    realbuffer=_realbuffer;
  }

  final PGplugbuffer start(void|int waitforreal) {
    realbuffer->stashcount++;
#ifdef PG_DEBUG
    if(waitforreal)
      error("pgsql.PGplugbuffer not allowed here\n");
#endif
    return this;
  }

  final
   void sendcmd(void|sctype mode,void|object portal) {
    Thread.MutexKey lock=realbuffer->stashupdate->lock();
    if(portal)
      realbuffer->stashqueue->write(portal);
    realbuffer->stash->add(this);
    mode=mergemode(realbuffer,mode);
    if(!--realbuffer->stashcount)
      realbuffer->stashavail.signal();
    lock=0;
    this->clear();
    if(lock=realbuffer->nostash->trylock(1)) {
      realbuffer->started=lock; lock=0;
      realbuffer->sendcmd(sendout);
    }
  }

}

class PGassist {
  inherit Stdio.Buffer:i;
  inherit Stdio.Buffer:o;

  protected Thread.Condition condition;
  protected Thread.Mutex mux;
  protected Thread.Queue qportals;
  final Stdio.File socket;
  protected int towrite;

  final function(:void) gottimeout;
  final int timeout;
  Thread.Mutex nostash;
  Thread.MutexKey started;
  Thread.Mutex stashupdate;
  Thread.Queue stashqueue;
  Thread.Condition stashavail;
  Stdio.Buffer stash;
  int stashflushmode;
  int stashcount;
#ifdef PG_DEBUG
  int queueoutidx;
  int queueinidx;
#endif

  final PGassist|PGplugbuffer start(void|int waitforreal) {
    Thread.MutexKey lock;
    if(lock=(waitforreal?nostash->lock:nostash->trylock)(1)) {
      started=lock;
      lock=stashupdate->lock();
      if(stashcount) {
        stashavail.wait(lock);
        add(stash); stash->clear();
        foreach(stashqueue->try_read_array();;pgsql_result qp) {
          qportals->write(qp);
          PD(">%O %d Queue portal %d bytes\n",qp._portalname,++queueoutidx,
           sizeof(this));
        }
      }
      lock=0;
      return this;
    }
    stashcount++;
    return PGplugbuffer(this);
  }

  protected final bool range_error(int howmuch) {
    if(!howmuch)
      return false;
#ifdef PG_DEBUG
    if(howmuch<0)
      error("Out of range %d\n",howmuch);
#endif
    if(condition) {
      array cid=local_backend->call_out(gottimeout,timeout);
      Thread.MutexKey lock=mux->lock();
      condition.wait(lock);
      lock=0;
      local_backend->remove_call_out(cid);
    } else
      throw(MAGICTERMINATE);
    return true;
  }

  protected int read_cb(mixed id,mixed b) {
    Thread.MutexKey lock=mux->lock();
    if(condition)
      condition.signal();
    lock=0;
    return 0;
  }

  protected int write_cb() {
    towrite-=output_to(socket,towrite);
    if(!condition && !sizeof(this))
      socket->close();
    return 0;
  }

  inline final    int consume(int w)     { return i::consume(w);     }
  inline final    int unread(int w)      { return i::unread(w);      }
  inline final string read(int w)        { return i::read(w);        }
  inline final object read_buffer(int w) { return i::read_buffer(w); }
  inline final    int read_sint(int w)   { return i::read_sint(w);   }
  inline final    int read_int8()        { return i::read_int8();    }
  inline final    int read_int16()       { return i::read_int16();   }
  inline final    int read_int32()       { return i::read_int32();   }
  inline final string read_cstring()     { return i::read_cstring(); }

  final
   void sendcmd(void|sctype mode,void|pgsql_result portal) {
    if(portal) {
      qportals->write(portal);
      PD(">%O %d Queue portal %d bytes\n",portal._portalname,++queueoutidx,
       sizeof(this));
    }
    if(started) {
      Thread.MutexKey lock=stashupdate->lock();
      if(sizeof(stash)) {
        add(stash); stash->clear();
        foreach(stashqueue->try_read_array();;pgsql_result qp) {
          qportals->write(qp);
          PD(">%O %d Queue portal %d bytes\n",qp._portalname,++queueoutidx,
           sizeof(this));
        }
      }
      mode=mergemode(this,mode);
      stashflushmode=keep;
      lock=0;
    }
    switch(mode) {
      case flushsend:
        add(PGFLUSH);
        PD("Flush\n");
      case sendout:
        if(towrite+=sizeof(this)) {
          PD(">Sendcmd %O\n",((string)this)[..towrite-1]);
          towrite-=output_to(socket,towrite);
        }
    }
    started=0;
  }

  final void sendterminate() {
    destruct(condition);  // Delayed close() after flushing the output buffer
  }

  final int close() {
    return socket->close();
  }

  final void destroy() {
    catch(close());		// Exceptions don't work inside destructors
  }

  final void connectloop(object pgsqlsess,int nossl) {
    mixed err=catch {
      for(;;clear()) {
        socket->connect(pgsqlsess._host,pgsqlsess._port);
#if constant(SSL.File)
        if(!nossl && !pgsqlsess->nossl
         && (pgsqlsess.options.use_ssl || pgsqlsess.options.force_ssl)) {
          PD("SSLRequest\n");
          start()->add_int32(8)->add_int32(PG_PROTOCOL(1234,5679))
           ->sendcmd(sendout);
          switch(read_int8()) {
            case 'S':
              object fcon=SSL.File(socket,SSL.Context());
              if(fcon->connect()) {
                socket=fcon;
                break;
              }
            default:socket->close();
              pgsqlsess.nossl=1;
              continue;
            case 'N':
              if(pgsqlsess.options.force_ssl)
                error("Encryption not supported on connection to %s:%d\n",
                      pgsqlsess.host,pgsqlsess.port);
          }
        }
#else
        if(pgsqlsess.options.force_ssl)
          error("Encryption library missing,"
                " cannot establish connection to %s:%d\n",
                pgsqlsess.host,pgsqlsess.port);
#endif
        break;
      }
      socket->set_backend(local_backend);
      socket->set_buffer_mode(i::this,0);
      socket->set_nonblocking(read_cb,write_cb,0);
      Thread.Thread(pgsqlsess->_processloop,this);
      return;
    };
    pgsqlsess->_connectfail(err);
  }

  void create(object pgsqlsess,Thread.Queue _qportals,int nossl) {
    i::create(); o::create();
    qportals = _qportals;
    condition=Thread.Condition();
    mux=Thread.Mutex();
    gottimeout=sendcmd;		// Preset it with a NOP
    timeout=128;		// Just a reasonable amount
    socket=Stdio.File();
    nostash=Thread.Mutex();
    stashupdate=Thread.Mutex();
    stashqueue=Thread.Queue();
    stashavail=Thread.Condition();
    stash=Stdio.Buffer();
    Thread.Thread(connectloop,pgsqlsess,nossl);
  }
}

//! The result object returned by @[Sql.pgsql()->big_query()], except for
//! the noted differences it behaves the same as @[Sql.sql_result].
//!
//! @seealso
//!   @[Sql.sql_result], @[Sql.pgsql], @[Sql.Sql], @[Sql.pgsql()->big_query()]
class pgsql_result {

  object _pgsqlsess;
  protected int numrows;
  protected int eoffound;
  protected PGassist c;
  mixed _delayederror;
  portalstate _state;
  int _fetchlimit;
  int _alltext;
  int _forcetext;

  string _portalname;

  int _bytesreceived;
  int _rowsreceived;
  int _inflight;
  int _portalbuffersize;
  Thread.MutexKey _unnamedportalkey,_unnamedstatementkey;
  protected Thread.Mutex closemux;
  array _params;
  string _statuscmdcomplete;
  string _query;
  Thread.Queue _datarows;
  array(mapping(string:mixed)) _datarowdesc=({});
  int _oldpbpos;
  object _plugbuffer;
  string _preparedname;
  mapping(string:mixed) _tprepared;

  protected string _sprintf(int type, void|mapping flags) {
    string res=UNDEFINED;
    switch(type) {
      case 'O':
        res=sprintf("pgsql_result  numrows: %d  eof: %d inflight: %d\n"
#ifdef PG_DEBUGMORE
                    "query: %O\n"
#endif
                    "portalname: %O  datarows: %d"
                    "  laststatus: %s\n",
                    numrows,eoffound,_inflight,
#ifdef PG_DEBUGMORE
                    _query,
#endif
                    _portalname,sizeof(_datarowdesc),
                    _statuscmdcomplete||"");
        break;
    }
    return res;
  }

  void create(object pgsqlsess,PGassist _c,string query,
              int portalbuffersize,int alltyped,array params,int forcetext) {
    _pgsqlsess = pgsqlsess;
    c = _c;
    _query = query;
    _datarows = Thread.Queue(); numrows = UNDEFINED;
    closemux=Thread.Mutex();
    _portalbuffersize=portalbuffersize;
    _alltext = !alltyped;
    _params = params;
    _forcetext = forcetext;
    _state = portalinit;
  }

  //! Returns the command-complete status for this query.
  //!
  //! @seealso
  //!  @[affected_rows()]
  //!
  //! @note
  //! This function is PostgreSQL-specific, and thus it is not available
  //! through the generic SQL-interface.
  string status_command_complete() {
    return _statuscmdcomplete;
  }

  //! Returns the number of affected rows by this query.
  //!
  //! @seealso
  //!  @[status_command_complete()]
  //!
  //! @note
  //! This function is PostgreSQL-specific, and thus it is not available
  //! through the generic SQL-interface.
  int affected_rows() {
    int rows;
    if(_statuscmdcomplete)
      sscanf(_statuscmdcomplete,"%*s %d",rows);
    return rows;
  }

  //! @seealso
  //!  @[Sql.sql_result()->num_fields()]
  int num_fields() {
    return sizeof(_datarowdesc);
  }

  //! @seealso
  //!  @[Sql.sql_result()->num_rows()]
  int num_rows() {
    int numrows;
    if(_statuscmdcomplete)
      sscanf(_statuscmdcomplete,"%*s %d",numrows);
    return numrows;
  }

  protected inline void trydelayederror() {
    if(_delayederror)
      throwdelayederror(this);
  }

  //! @seealso
  //!  @[Sql.sql_result()->eof()]
  int eof() {
    trydelayederror();
    return eoffound;
  }

  //! @seealso
  //!  @[Sql.sql_result()->fetch_fields()]
  array(mapping(string:mixed)) fetch_fields() {
    trydelayederror();
    return _datarowdesc+({});
  }

  void _openportal() {
    _pgsqlsess->_portalsinflight++;
    Thread.MutexKey lock=closemux->lock();
    _state=bound;
    lock=0;
    _statuscmdcomplete=UNDEFINED;
  }

  int _closeportal(PGplugbuffer plugbuffer) {
    int retval=0;
    PD("%O Try Closeportal %d\n",_portalname,_state);
    Thread.MutexKey lock=closemux->lock();
    _fetchlimit=0;				   // disables further Executes
    switch(_state) {
      case copyinprogress:
        PD("CopyDone\n");
        plugbuffer->add("c\0\0\0\4");
      case bound:
        _state=closed;
        lock=0;
        PD("Close portal %O\n",_portalname);
        if(sizeof(_portalname)) {
          plugbuffer->add_int8('C')->add_hstring(({'P',_portalname,0}),4,4);
          retval=1;
        } else
          _unnamedportalkey=0;
        if(!--_pgsqlsess->_portalsinflight) {
          PD("Sync\n");
          plugbuffer->add(PGSYNC);
          _pgsqlsess->_pportalcount=0;
          retval=2;
        }
    }
    lock=0;
    return retval;
  }

  final void _processdataready(int gfetchlimit) {
    _rowsreceived++;
    if(_rowsreceived==1)
      PD("<%O _fetchlimit %d=min(%d||1,%d), _inflight %d\n",_portalname,
       _fetchlimit,(_portalbuffersize>>1)*_rowsreceived/_bytesreceived,
       gfetchlimit,_inflight);
    if(_fetchlimit) {
      _fetchlimit=
       min((_portalbuffersize>>1)*_rowsreceived/_bytesreceived||1,gfetchlimit);
#if STREAMEXECUTES
      Thread.MutexKey lock=closemux->lock();
      if(_fetchlimit && _inflight<=_fetchlimit-1)
        _sendexecute(_fetchlimit);
#ifdef PG_DEBUG
      else if(!_fetchlimit)
        PD("<%O _fetchlimit %d, _inflight %d, skip execute\n",
         _portalname,_fetchlimit,_inflight);
#endif
      lock=0;
#endif
    }
  }

  void _releasesession() {
    _inflight=0;
    _datarows->write(1);
    object plugbuffer=c->start(1);
    plugbuffer->sendcmd(_closeportal(plugbuffer));
    _pgsqlsess=UNDEFINED;
  }

  protected void destroy() {
    catch {			   // inside destructors, exceptions don't work
      _releasesession();
    };
  }

  final void _sendexecute(int fetchlimit,void|PGplugbuffer plugbuffer) {
    int flushmode;
    PD("Execute portal %O fetchlimit %d\n",_portalname,fetchlimit);
    if(!plugbuffer)
      plugbuffer=c->start(1);
    plugbuffer->add_int8('E')->add_hstring(_portalname,4,8+1)
     ->add_int8(0)->add_int32(fetchlimit);
    if(!fetchlimit) {
      _closeportal(plugbuffer);
      flushmode=sendout;
    } else
      _inflight+=fetchlimit, flushmode=flushsend;
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
  array(mixed) fetch_row() {
    int|array datarow;
    if(arrayp(datarow=_datarows->try_read()))
      return datarow;
    if(!datarow && !eoffound
     && (
#ifdef PG_DEBUG
         PD("%O Block for datarow\n",_portalname),
#endif
         arrayp(datarow=_datarows->read())))
      return datarow;
    trydelayederror();
    eoffound=1;
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
  array(array(mixed)) fetch_row_array() {
    if(eoffound)
      return 0;
    array(array|int) datarow=_datarows->try_read_array();
    if(!datarow)
       datarow=_datarows->read_array();
    if(arrayp(datarow[-1]))
      return datarow;
    trydelayederror();
    eoffound=1;
    return (datarow=datarow[..<1]);
  }

  //! @param copydatasend
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
  void send_row(void|string|array(string) copydata) {
    trydelayederror();
    if(copydata) {
      PD("CopyData\n");
      c->start()->add_int8('d')->add_hstring(copydata,4,4)->sendcmd(sendout);
    } else
      _releasesession();
  }

  protected void run_result_cb(
   function(pgsql_result, array(mixed), mixed ...:void) callback,
   array(mixed) args) {
    int|array datarow;
    while(arrayp(datarow=_datarows->read_array()))
      callback(this, datarow, @args);
    trydelayederror();
    eoffound=1;
    callback(this, 0, @args);
  }

  //! Sets up a callback for every row returned from the database.
  //! First argument passed is the resultobject itself, second argument
  //! is the result row (zero on EOF).
  //!
  //! @seealso
  //!  @[fetch_row()]
  void set_result_callback(
   function(pgsql_result, array(mixed), mixed ...:void) callback,
   mixed ... args) {
    if(callback)
      Thread.Thread(run_result_cb,callback,args);
  }

  protected void run_result_array_cb(
   function(pgsql_result, array(array(mixed)), mixed ...:void) callback,
   array(mixed) args) {
    array(array|int) datarow;
    while((datarow=_datarows->read_array()) && arrayp(datarow[-1]))
      callback(this, datarow, @args);
    trydelayederror();
    eoffound=1;
    if(sizeof(datarow)>1)
      callback(this, datarow=datarow[..<1], @args);
    callback(this, 0, @args);
  }

  //! Sets up a callback for sets of rows returned from the database.
  //! First argument passed is the resultobject itself, second argument
  //! is the array of result rows (zero on EOF).
  //!
  //! @seealso
  //!  @[fetch_row()]
  void set_result_array_callback(
   function(pgsql_result, array(array(mixed)), mixed ...:void) callback,
   mixed ... args) {
    if(callback)
      Thread.Thread(run_result_array_cb,callback,args);
  }

}
