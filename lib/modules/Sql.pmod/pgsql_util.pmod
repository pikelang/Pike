/*
 * Some pgsql utility functions.
 * They are kept here to avoid circular references.
 *
 */

#pike __REAL_VERSION__

#include "pgsql.h"

#define FLUSH		"H\0\0\0\4"

//! Some pgsql utility functions

class PGassist {

  int(-1..1) peek(int timeout) {
  }

  string read(int len,void|int(0..1) not_all) {
  }

  int write(string|array(string) data) {
  }

  int getchar() {
  }

  int close() {
  }

  private final array(string) cmdbuf=({});

#ifdef USEPGsql
  inherit _PGsql.PGsql;
#else
  object portal;

  void setportal(void|object newportal) {
    portal=newportal;
  }

  inline int(-1..1) bpeek(int timeout) {
    return peek(timeout);
  }

  int flushed=-1;

  inline final int getbyte() {
    if(!flushed && !bpeek(0))
      sendflush();
    return getchar();
  }

  final string getstring(void|int len) {
    if(!zero_type(len)) {
      string acc="",res;
      do {
        if(!flushed && !bpeek(0))
          sendflush();
        res=read(len,!flushed);
        if(res) {
          if(!sizeof(res))
            return acc;
          acc+=res;
        }
      }
      while(sizeof(acc)<len&&res);
      return sizeof(acc)?acc:res;
    }
    array(int) acc=({});
    int c;
    while((c=getbyte())>0)
      acc+=({c});
    return `+("",@map(acc,String.int2char));
  }

  inline final int getint16() {
    int s0=getbyte();
    int r=(s0&0x7f)<<8|getbyte();
    return s0&0x80 ? r-(1<<15) : r ;
  }

  inline final int getint32() {
    int r=getint16();
    r=r<<8|getbyte();
    return r<<8|getbyte();
  }

  inline final int getint64() {
    int r=getint32();
    return r<<32|getint32()&0xffffffff;
  }
#endif

  inline final string plugbyte(int x) {
    return String.int2char(x);
  }

  inline final string plugint16(int x) {
    return sprintf("%c%c",x>>8&255,x&255);
  }

  inline final string plugint32(int x) {
    return sprintf("%c%c%c%c",x>>24&255,x>>16&255,x>>8&255,x&255);
  }

  inline final string plugint64(int x) {
    return sprintf("%c%c%c%c%c%c%c%c",x>>56&255,x>>48&255,x>>40&255,x>>32&255,
     x>>24&255,x>>16&255,x>>8&255,x&255);
  }

  final void sendflush() {
    sendcmd(({}),1);
  }

  final void sendcmd(string|array(string) data,void|int flush) {
    if(arrayp(data))
      cmdbuf+=data;
    else
      cmdbuf+=({data});
    switch(flush) {
      case 3:
        cmdbuf+=({FLUSH});
        flushed=1;
        break;
      default:
	flushed=0;
        break;
      case 1:
        cmdbuf+=({FLUSH});
        PD("Flush\n");
      case 2:
        flushed=1;
        { int i=write(cmdbuf);
	  if(portal && portal._pgsqlsess) {
	    portal._pgsqlsess._packetssent++;
	    portal._pgsqlsess._bytessent+=i;
	  }
        }
        cmdbuf=({});
    }
  }

  final void sendterminate() {
    PD("Terminate\n");
    sendcmd(({"X",plugint32(4)}),2);
    close();
  }

  void create() {
#ifdef USEPGsql
    ::create();
#endif
  }
}

class PGconn {

  inherit PGassist:pg;
#ifdef UNBUFFEREDIO
  inherit Stdio.File:std;

  inline int getchar() {
    return std::read(1)[0];
  }
#else
  inherit Stdio.FILE:std;

  inline int getchar() {
    return std::getchar();
  }
#endif

  inline int(-1..1) peek(int timeout) {
    return std::peek(timeout);
  }

  inline string read(int len,void|int(0..1) not_all) {
    return std::read(len,not_all);
  }

  inline int write(string|array(string) data) {
    return std::write(data);
  }

  int close() {
    return std::close();
  }

  void create(Stdio.File stream,object t) {
    std::create();
    std::assign(stream);
    pg::create();
  }
}

#if constant(SSL.sslfile)
class PGconnS {
  inherit SSL.sslfile:std;
  inherit PGassist:pg;

  Stdio.File rawstream;

  inline int(-1..1) peek(int timeout) {
    return rawstream.peek(timeout);			    // This is a kludge
  }			 // Actually SSL.sslfile should provide a peek() method

  inline string read(int len,void|int(0..1) not_all) {
    return std::read(len,not_all);
  }

  inline int write(string|array(string) data) {
    return std::write(data);
  }

  void create(Stdio.File stream, SSL.context ctx) {
    rawstream=stream;
    std::create(stream,ctx,1,1);
    pg::create();
  }
}
#endif

class pgsql_result {

object _pgsqlsess;
private int numrows;
private int eoffound;
private mixed delayederror;
private int copyinprogress;
int _fetchlimit;
int _alltext;

#ifdef NO_LOCKING
int _qmtxkey;
#else
Thread.MutexKey _qmtxkey;
#endif

string query;
string _portalname;

int _bytesreceived;
int _rowsreceived;
int _interruptable;
int _inflight;
int _portalbuffersize;
string _statuscmdcomplete;
array(array(mixed)) _datarows;
array(mapping(string:mixed)) _datarowdesc;
array(int) _datatypeoid;
#ifdef USEPGsql
int _buffer;
#endif

private object fetchmutex;

protected string _sprintf(int type, void|mapping flags) {
  string res=UNDEFINED;
  switch(type) {
    case 'O':
      res=sprintf("pgsql_result  numrows: %d  eof: %d  querylock: %d"
       " inflight: %d\nportalname: %O  datarows: %d\n"
       "query: %O\n"
       "laststatus: %s\n",
       numrows,eoffound,!!_qmtxkey,_inflight,
       _portalname,sizeof(_datarowdesc),
       query,
       _statuscmdcomplete||"");
      break;
  }
  return res;
}

void create(object pgsqlsess,string _query,int fetchlimit,
 int portalbuffersize,int alltyped) {
  _pgsqlsess = pgsqlsess;
  query = _query;
  _datarows = ({ }); numrows = UNDEFINED;
  fetchmutex = Thread.Mutex();
  _fetchlimit=fetchlimit;
  _portalbuffersize=portalbuffersize;
  _alltext = !alltyped;
  steallock();
}

//! Returns the command-complete status for this query.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
//!
//! @seealso
//!  @[affected_rows]
string status_command_complete() {
  return _statuscmdcomplete;
}

//! Returns the number of affected rows by this query.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
//!
//! @seealso
//!  @[status_command_complete]
int affected_rows() {
  int rows;
  if(_statuscmdcomplete)
    sscanf(_statuscmdcomplete,"%*s %d",rows);
  return rows;
}

int num_fields() {
  return sizeof(_datarowdesc);
}

int num_rows() {
  int numrows;
  sscanf(_statuscmdcomplete,"%*s %d",numrows);
  return numrows;
}

int eof() {
  return eoffound;
}

array(mapping(string:mixed)) fetch_fields() {
  return _datarowdesc+({});
}

private void releasesession() {
  if(_pgsqlsess) {
    if(copyinprogress) {
      PD("CopyDone\n");
      _pgsqlsess._c.sendcmd("c\0\0\0\4",1);
    }
    _pgsqlsess.reload(2);
  }
  _qmtxkey=UNDEFINED;
  _pgsqlsess=UNDEFINED;
}

void destroy() {
  catch {			   // inside destructors, exceptions don't work
    releasesession();
  };
}

inline private array(mixed) getdatarow() {
  array(mixed) datarow=_datarows[0];
  _datarows=_datarows[1..];
  return datarow;
}

private void steallock() {
#ifndef NO_LOCKING
  PD("Going to steal oldportal %d\n",!!_pgsqlsess._c.portal);
  Thread.MutexKey stealmtxkey = _pgsqlsess._stealmutex.lock();
  do
    if(_qmtxkey = _pgsqlsess._querymutex.current_locking_key()) {
      pgsql_result portalb;
      if(portalb=_pgsqlsess._c.portal) {
        _pgsqlsess._nextportal++;
        if(portalb->_interruptable)
          portalb->fetch_row(2);
        else {
          PD("Waiting for the querymutex\n");
          if((_qmtxkey=_pgsqlsess._querymutex.lock(2))) {
	    if(copyinprogress)
	      error("COPY needs to be finished first\n");
	    error("Driver bug, please report, "
             "conflict while interleaving SQL-operations\n");
	  }
          PD("Got the querymutex\n");
        }
        _pgsqlsess._nextportal--;
      }
      break;
    }
  while(!(_qmtxkey=_pgsqlsess._querymutex.trylock()));
#else
  PD("Skipping lock\n");
  _qmtxkey=1;
#endif
  _pgsqlsess._c.setportal(this);
  PD("Stealing successful\n");
}

int|array(string|int) fetch_row(void|int|string buffer) {
#ifndef NO_LOCKING
  Thread.MutexKey fetchmtxkey = fetchmutex.lock();
#endif
  if(!buffer && sizeof(_datarows))
    return getdatarow();
  if(copyinprogress) {
    fetchmtxkey = UNDEFINED;
    if(stringp(buffer)) {
      PD("CopyData\n");
      _pgsqlsess._c.sendcmd(({"d",_pgsqlsess._c.plugint32(4+sizeof(buffer)),
       buffer}),2);
    }
    else
      releasesession();
    return UNDEFINED;
  }
  mixed err;
  if(buffer!=2 && (err=delayederror)) {
    delayederror=UNDEFINED;
    throw(err);
  }
  err = catch {
    if(_portalname) {
      if(buffer!=2 && !_qmtxkey) {
        steallock();
        _pgsqlsess._sendexecute(_fetchlimit);
      }
      while(_pgsqlsess._closesent)
        _pgsqlsess._decodemsg();	      // Flush previous portal sequence
      for(;;) {
#ifdef DEBUGMORE
        PD("buffer: %d  nextportal: %d  lock: %d\n",
	 buffer,_pgsqlsess._nextportal,!!_qmtxkey);
#endif
#ifdef USEPGsql
        _buffer=buffer;
#endif
        switch(_pgsqlsess._decodemsg()) {
          case copyinresponse:
            copyinprogress=1;
	    return UNDEFINED;
          case dataready:
            _pgsqlsess._mstate=dataprocessed;
            _rowsreceived++;
	    switch(buffer) {
	      case 0:
	      case 1:
	        if(_fetchlimit)
	          _fetchlimit=
		   min(_portalbuffersize/2*_rowsreceived/_bytesreceived || 1,
	           _pgsqlsess._fetchlimit);
	    }
            switch(buffer) {
              case 2:
              case 3:
                continue;
              case 1:
	        _interruptable=1;
	        if(_pgsqlsess._nextportal)
		  continue;
#if STREAMEXECUTES
	        if(_fetchlimit && _inflight<=_fetchlimit-1)
                  _pgsqlsess._sendexecute(_fetchlimit);
#endif
                return UNDEFINED;
            }
#if STREAMEXECUTES
	    if(_fetchlimit && _inflight<=_fetchlimit-1)
              _pgsqlsess._sendexecute(_fetchlimit);	    // Overlap Executes
#endif
            return getdatarow();
          case commandcomplete:
            _inflight=0;
            releasesession();
	    switch(buffer) {
	      case 1:
	      case 2:
	        return UNDEFINED;
	      case 3:
	        if(sizeof(_datarows))
                  return getdatarow();
	    }
            break;
          case portalsuspended:
	    if(_inflight)
	      continue;
	    if(_pgsqlsess._nextportal) {
	      switch(buffer) {
	        case 1:
	        case 2:
	          _qmtxkey = UNDEFINED;
                  return UNDEFINED;
	        case 3:
	          _qmtxkey = UNDEFINED;
                  return getdatarow();
	      }
              _fetchlimit=FETCHLIMITLONGRUN;
	      if(sizeof(_datarows)) {
	        _qmtxkey = UNDEFINED;
	        return getdatarow();
	      }
	      buffer=3;
	    }
            _pgsqlsess._sendexecute(_fetchlimit);
          default:
            continue;
        }
        break;
      }
    }
    eoffound=1;
    return UNDEFINED;
  };
  PD("Exception %O\n",err);
  _pgsqlsess.reload();
  if(buffer!=2)
    throw(err);
  if(!delayederror)
    delayederror=err;
  return UNDEFINED;
}

};
