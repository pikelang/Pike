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

protected Thread.Mutex backendmux = Thread.Mutex();
final Pike.Backend local_backend = Pike.SmallBackend();
protected int clientsregistered;

multiset cachealways=(<"BEGIN","begin","END","end","COMMIT","commit">);
object createprefix
 =Regexp("^[ \t\f\r\n]*[Cc][Rr][Ee][Aa][Tt][Ee][ \t\f\r\n]");
protected object dontcacheprefix
 =Regexp("^[ \t\f\r\n]*([Ff][Ee][Tt][Cc][Hh]|[Cc][Oo][Pp][Yy])[ \t\f\r\n]");
protected object execfetchlimit
 =Regexp("^[ \t\f\r\n]*(([Uu][Pp][Dd][Aa]|[Dd][Ee][Ll][Ee])[Tt][Ee]|\
[Ii][Nn][Ss][Ee][Rr][Tt])[ \t\f\r\n]|\
[ \t\f\r\n][Ll][Ii][Mm][Ii][Tt][ \t\f\r\n]+[12][; \t\f\r\n]*$");

final void closestatement(object plugbuffer,string oldprep) {
  if(oldprep) {
    PD("Close statement %s\n",oldprep);
    plugbuffer->add_int8('C')->add_hstring(({'S',oldprep,0}),4,4);
  }
}

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

protected sctype mergemode(PGassist realbuffer,sctype mode) {
  if(mode>realbuffer->stashflushmode)
    realbuffer->stashflushmode=mode;
  return realbuffer->stashflushmode;
}

//! Some pgsql utility functions

class PGplugbuffer {
  inherit Stdio.Buffer;

  protected PGassist realbuffer;

  protected void create(PGassist _realbuffer) {
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

  protected Thread.Condition fillread;
  protected Thread.Mutex fillreadmux;
  protected Thread.Queue qportals;
  final Stdio.File socket;
  protected object pgsqlsess;
  protected int towrite;

  final function(:void) gottimeout;
  final int timeout;
  final Thread.Mutex nostash;
  final Thread.MutexKey started;
  final Thread.Mutex stashupdate;
  final Thread.Queue stashqueue;
  final Thread.Condition stashavail;
  final Stdio.Buffer stash;
  final int stashflushmode;
  final int stashcount;
  final int synctransact;
#ifdef PG_DEBUG
  final int queueoutidx;
  final int queueinidx;
#endif

  inline void queueup(pgsql_result portal) {
    qportals->write(portal); portal->_synctransact=synctransact;
    PD(">%O %d %d Queue portal %d bytes\n",portal._portalname,++queueoutidx,
     synctransact,sizeof(this));
  }

  final PGassist|PGplugbuffer start(void|int waitforreal) {
    Thread.MutexKey lock;
    if(lock=(waitforreal?nostash->lock:nostash->trylock)(1)) {
      started=lock;
      lock=stashupdate->lock();
      if(stashcount)
        stashavail.wait(lock);
      add(stash); stash->clear();
      foreach(stashqueue->try_read_array();;pgsql_result portal)
        queueup(portal);
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
    if(fillread) {
      array cid=local_backend->call_out(gottimeout,timeout);
      Thread.MutexKey lock=fillreadmux->lock();
      fillread.wait(lock);
      lock=0;
      local_backend->remove_call_out(cid);
    } else
      throw(MAGICTERMINATE);
    return true;
  }

  protected int read_cb(mixed id,mixed b) {
    Thread.MutexKey lock=fillreadmux->lock();
    if(fillread)
      fillread.signal();
    lock=0;
    return 0;
  }

  protected int write_cb() {
    towrite-=output_to(socket,towrite);
    if(!fillread && !sizeof(this))
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
    if(portal)
      queueup(portal);
    if(mode==flushlogsend) {
      mode=flushsend; qportals->write(synctransact++);
      PD(">%O %d Queue simplequery %d bytes\n",portal._portalname,
       ++queueoutidx,sizeof(this));
    }
    if(started) {
      Thread.MutexKey lock=stashupdate->lock();
      if(sizeof(stash)) {
        add(stash); stash->clear();
        foreach(stashqueue->try_read_array();;pgsql_result portal)
          queueup(portal);
      }
      mode=mergemode(this,mode);
      stashflushmode=keep;
      lock=0;
    }
    catch {
outer:
      do {
        switch(mode) {
          default:
            break outer;
          case syncsend:
            PD(">Sync %d %d Queue\n",synctransact,++queueoutidx);
            qportals->write(synctransact++); add(PGSYNC);
            break;
          case flushsend:
            PD("Flush\n");
            add(PGFLUSH);
          case sendout:;
        }
        if(towrite=sizeof(this)) {
          PD(">Sendcmd %O\n",((string)this)[..towrite-1]);
          towrite-=output_to(socket,towrite);
        }
      } while(0);
      started=0;
      return;
    };
    pgsqlsess->_connectfail();
  }

  final void sendterminate() {
    destruct(fillread);  // Delayed close() after flushing the output buffer
  }

  final int close() {
    return socket->close();
  }

  final void destroy() {
    catch(close());		// Exceptions don't work inside destructors
    pgsqlsess=0;
  }

  final void connectloop(int nossl) {
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
      if(!socket->is_open())
        error(strerror(socket->errno()));
      socket->set_backend(local_backend);
      socket->set_buffer_mode(i::this,0);
      socket->set_nonblocking(read_cb,write_cb,0);
      Thread.Thread(pgsqlsess->_processloop,this);
      return;
    };
    pgsqlsess->_connectfail(err);
  }

  protected void create(object _pgsqlsess,Thread.Queue _qportals,int nossl) {
    i::create(); o::create();
    qportals = _qportals;
    synctransact = 1;
    fillread=Thread.Condition();
    fillreadmux=Thread.Mutex();
    gottimeout=sendcmd;		// Preset it with a NOP
    timeout=128;		// Just a reasonable amount
    socket=Stdio.File();
    nostash=Thread.Mutex();
    stashupdate=Thread.Mutex();
    stashqueue=Thread.Queue();
    stashavail=Thread.Condition();
    stash=Stdio.Buffer();
    pgsqlsess=_pgsqlsess;
    Thread.Thread(connectloop,nossl);
  }
}

//! The result object returned by @[Sql.pgsql()->big_query()], except for
//! the noted differences it behaves the same as @[Sql.sql_result].
//!
//! @seealso
//!   @[Sql.sql_result], @[Sql.pgsql], @[Sql.Sql], @[Sql.pgsql()->big_query()]
class pgsql_result {

  protected object pgsqlsess;
  protected int numrows;
  protected int eoffound;
  protected PGassist c;
  final mixed _delayederror;
  final portalstate _state;
  final int _fetchlimit;
  final int _alltext;
  final int _forcetext;

  final string _portalname;

  final int _bytesreceived;
  final int _rowsreceived;
  final int _inflight;
  final int _portalbuffersize;
  final int _synctransact;
  final Thread.Condition _ddescribe;
  final Thread.Mutex _ddescribemux;
  final Thread.MutexKey _unnamedportalkey,_unnamedstatementkey;
  protected Thread.Mutex closemux;
  final array _params;
  final string _statuscmdcomplete;
  final string _query;
  final Thread.Queue _datarows;
  final array(mapping(string:mixed)) _datarowdesc;
  final int _oldpbpos;
  protected object prepbuffer;
  protected Thread.Condition prepbufferready;
  protected Thread.Mutex prepbuffermux;
  final string _preparedname;
  final mapping(string:mixed) _tprepared;

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
                    _portalname,_datarowdesc&&sizeof(_datarowdesc),
                    _statuscmdcomplete||"");
        break;
    }
    return res;
  }

  protected void create(object _pgsqlsess,PGassist _c,string query,
              int portalbuffersize,int alltyped,array params,int forcetext) {
    pgsqlsess = _pgsqlsess;
    c = _c;
    _query = query;
    _datarows = Thread.Queue(); numrows = UNDEFINED;
    _ddescribe=Thread.Condition();
    _ddescribemux=Thread.Mutex();
    closemux=Thread.Mutex();
    prepbufferready=Thread.Condition();
    prepbuffermux=Thread.Mutex();
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

  protected void waitfordescribe() {
    Thread.MutexKey lock=_ddescribemux->lock();
    if(!_datarowdesc)
      _ddescribe->wait(lock);
    lock=0;
  }

  //! @seealso
  //!  @[Sql.sql_result()->num_fields()]
  int num_fields() {
    if(!_datarowdesc)
      waitfordescribe();
    trydelayederror();
    return sizeof(_datarowdesc);
  }

  //! @seealso
  //!  @[Sql.sql_result()->num_rows()]
  int num_rows() {
    trydelayederror();
    return _rowsreceived;
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
    if(!_datarowdesc)
      waitfordescribe();
    trydelayederror();
    return _datarowdesc+({});
  }

  final void _setrowdesc(array(mapping(string:mixed)) datarowdesc) {
    _datarowdesc=datarowdesc;
    Thread.MutexKey lock=_ddescribemux->lock();
    _ddescribe->broadcast();
    lock=0;
  }

  final void _preparebind(array dtoid) {
    array(string|int) paramValues=_params?_params[2]:({});
    if(sizeof(dtoid)!=sizeof(paramValues))
      SUSERERROR("Invalid number of bindings, expected %d, got %d\n",
                 sizeof(dtoid),sizeof(paramValues));
#ifdef PG_DEBUGMORE
    PD("ParamValues to bind: %O\n",paramValues);
#endif
    object plugbuffer=Stdio.Buffer();
    plugbuffer->add(_portalname=
      (_unnamedportalkey=pgsqlsess._unnamedportalmux->trylock(1))
       ? "" : PORTALPREFIX+int2hex(pgsqlsess._pportalcount++) )->add_int8(0)
     ->add(_preparedname)->add_int8(0)->add_int16(sizeof(paramValues));
    foreach(dtoid;;int textbin)
      plugbuffer->add_int16(oidformat(textbin));
    plugbuffer->add_int16(sizeof(paramValues));
    string cenc=pgsqlsess._runtimeparameter[CLIENT_ENCODING];
    foreach(paramValues;int i;mixed value) {
      if(undefinedp(value))
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
            value=(string)value;
            if(String.width(value)>8)
              if(dtoid[i]==BYTEAOID)
                value=string_to_utf8(value);
              else {
                SUSERERROR("Wide string %O not supported for type OID %d\n",
                           value,dtoid[i]);
                value="";
              }
            plugbuffer->add_hstring(value,4);
            break;
          }
          case BOOLOID:plugbuffer->add_int32(1);
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
            plugbuffer->add_int8(value);
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
    prepbuffer=plugbuffer;
    int skipsignal=0;
    if(_tprepared)
      if(_tprepared.datarowdesc)
        skipsignal=1, gotdatarowdesc();
      else if(dontcacheprefix->match(_query))	// Don't cache FETCH/COPY
        m_delete(pgsqlsess->_prepareds,_query),_tprepared=0;
    if(!skipsignal) {
      Thread.MutexKey lock=prepbuffermux->lock();
      prepbufferready->signal();
      lock=0;
    }
  }

  final void _processrowdesc(array(mapping(string:mixed)) datarowdesc) {
    _setrowdesc(datarowdesc);
    mapping(string:mixed) tp=_tprepared;   // FIXME Is caching this worthwhile?
    if(!tp || !tp.datarowdesc)
      Thread.Thread(gotdatarowdesc);
    if(tp)
      tp.datarowdesc=datarowdesc;
  }

  protected void gotdatarowdesc() {
    if(!prepbuffer) {
      Thread.MutexKey lock=prepbuffermux->lock();
      prepbufferready->wait(lock);
      lock=0;
    }
    object plugbuffer=prepbuffer;
    prepbuffer=0;
    plugbuffer->add_int16(sizeof(_datarowdesc));
    foreach(_datarowdesc;;mapping col)
      plugbuffer->add_int16(oidformat(col.type));
    PD("Bind portal %O statement %O\n",_portalname,_preparedname);
    _fetchlimit=pgsqlsess->_fetchlimit;
    _openportal();
    object bindbuffer=c->start(1);
    _unnamedstatementkey=0;
    bindbuffer->add_int8('B')->add_hstring(plugbuffer,4,4);
    if(!_tprepared)
      closestatement(bindbuffer,_preparedname);
    _sendexecute(pgsqlsess->_fetchlimit
                         && !(cachealways[_query]
                              || sizeof(_query)>=MINPREPARELENGTH &&
                              execfetchlimit->match(_query))
                         && FETCHLIMITLONGRUN,bindbuffer);
  }

  final void _openportal() {
    pgsqlsess->_portalsinflight++;
    Thread.MutexKey lock=closemux->lock();
    _state=bound;
    lock=0;
    _statuscmdcomplete=UNDEFINED;
  }

  final void _purgeportal() {
    _unnamedportalkey=0;
    Thread.MutexKey lock=closemux->lock();
    _fetchlimit=0;				   // disables further Executes
    switch(_state) {
      case copyinprogress:
      case bound:
        --pgsqlsess->_portalsinflight;
    }
    _state=closed;
    lock=0;
  }

  final sctype _closeportal(PGplugbuffer plugbuffer) {
    sctype retval=keep;
    PD("%O Try Closeportal %d\n",_portalname,_state);
    Thread.MutexKey lock=closemux->lock();
    _fetchlimit=0;				   // disables further Executes
    switch(_state) {
      case portalinit:
        _state=closed;
        break;
      case copyinprogress:
        PD("CopyDone\n");
        plugbuffer->add("c\0\0\0\4");
      case bound:
        _state=closed;
        lock=0;
        PD("Close portal %O\n",_portalname);
        if(sizeof(_portalname)) {
          plugbuffer->add_int8('C')->add_hstring(({'P',_portalname,0}),4,4);
          retval=flushsend;
        } else
          _unnamedportalkey=0;
        if(!--pgsqlsess->_portalsinflight) {
          pgsqlsess->_readyforquerycount++;
          pgsqlsess->_pportalcount=0;
          retval=syncsend;
        }
    }
    lock=0;
    return retval;
  }

  final void _processdataready() {
    _rowsreceived++;
    if(_rowsreceived==1)
      PD("<%O _fetchlimit %d=min(%d||1,%d), _inflight %d\n",_portalname,
       _fetchlimit,(_portalbuffersize>>1)*_rowsreceived/_bytesreceived,
       pgsqlsess._fetchlimit,_inflight);
    if(_fetchlimit) {
      _fetchlimit=
       min((_portalbuffersize>>1)*_rowsreceived/_bytesreceived||1,
        pgsqlsess._fetchlimit);
      Thread.MutexKey lock=closemux->lock();
      if(_fetchlimit && _inflight<=_fetchlimit-1)
        _sendexecute(_fetchlimit);
      else if(!_fetchlimit)
        PD("<%O _fetchlimit %d, _inflight %d, skip execute\n",
         _portalname,_fetchlimit,_inflight);
      lock=0;
    }
  }

  final void _releasesession() {
    _inflight=0;
    _datarows->write(1);			// Signal EOF
    object plugbuffer=c->start(1);
    plugbuffer->sendcmd(_closeportal(plugbuffer));
    pgsqlsess=UNDEFINED;
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
    if(!fetchlimit)
      flushmode=_closeportal(plugbuffer)==syncsend?syncsend:flushsend;
    else
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
    if(!eoffound) {
      if(!datarow
       && (PD("%O Block for datarow\n",_portalname),
           arrayp(datarow=_datarows->read())))
        return datarow;
      eoffound=1;
      _datarows->write(1);			// Signal EOF for other threads
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
    _datarows->write(1);			// Signal EOF for other threads
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
