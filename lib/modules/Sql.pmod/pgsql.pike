/*
 * This is the PostgreSQL direct network module for Pike.
 */

//! This is an interface to the PostgreSQL database
//! server. This module is independent of external libraries.
//! Note that you @b{do not@} need to have a
//! PostgreSQL server running on your host to use this module: you can
//! connect to the database over a TCP/IP socket.
//!
//! This module replaces the functionality of the older @[Sql.postgres]
//! and @[Postgres.postgres] modules.
//!
//! This module supports the following features:
//!
//! - PostgreSQL network protocol version 3, authentication methods
//!   currently supported are: cleartext and MD5 (recommended).
//!
//! - Streaming queries which do not buffer the whole resulset in memory.
//!
//! - Automatic binary transfers to and from the database for most common
//!   datatypes (amongst others: integer, text and bytea types).
//!
//! - SQL-injection protection by allowing just one statement per query
//!   and ignoring anything after the first (unquoted) semicolon in the query.
//!
//! - COPY support for streaming up- and download.
//!
//! - Accurate error messages.
//!
//! - Automatic precompilation of complex queries (session cache).
//!
//! - Multiple simultaneous queries on the same database connection.
//!
//! - Cancelling of long running queries by force or by timeout.
//!
//! - Event driven NOTIFY.
//!
//! - SSL encrypted connections (optional or forced).
//!
//! Refer to the PostgreSQL documentation for further details.
//!
//! @note
//!   Multiple simultaneous queries on the same database connection is a
//!   feature that none of the other database drivers for Pike support.
//!   So, although it's efficient, its use will make switching database drivers
//!   difficult.
//!
//! @seealso
//!  @[Sql.Sql], @[Sql.postgres]

#pike __REAL_VERSION__

#include "pgsql.h"

#define ERROR(X ...)	     predef::error(X)

int _nextportal;
int _closesent;
int _fetchlimit=FETCHLIMIT;
private int unnamedportalinuse;
private int portalsinflight;

object _c;
private string SSauthdata,cancelsecret;
private int backendpid;
private int backendstatus;
private mapping(string:mixed) options;
private string lastmessage;
private int clearmessage;
private int earlyclose;
private mapping(string:array(mixed)) notifylist=([]);
private mapping(string:string) msgresponse;
private mapping(string:string) runtimeparameter;
state _mstate;
private enum querystate {queryidle,inquery,cancelpending,canceled};
private querystate qstate;
private mapping(string:mapping(string:mixed)) prepareds=([]);
private int pstmtcount;
private int pportalcount;
private int totalhits;
private int cachedepth=STATEMENTCACHEDEPTH;
private int timeout=QUERYTIMEOUT;
private int portalbuffersize=PORTALBUFFERSIZE;
private int reconnected;     // Number of times the connection was reset

private string host, database, user, pass;
private int port;
private mapping(string:string) sessiondefaults=([]); // runtime parameters
Thread.Mutex _querymutex;
Thread.Mutex _stealmutex;

protected string _sprintf(int type, void|mapping flags) {
  string res=UNDEFINED;
  switch(type) {
    case 'O':
      res=sprintf(DRIVERNAME"://%s@%s:%d/%s pid:%d %s reconnected:%d\n"
    "mstate: %O  qstate: %O  pstmtcount: %d  pportalcount: %d  prepcache: %d\n"
       "Last message: %s",
       user,host,port,database,backendpid,status_commit(),reconnected,
       _mstate,qstate,pstmtcount,pportalcount,sizeof(prepareds),
       lastmessage||"");
      break;
  }
  return res;
}

#define BOOLOID		16
#define BYTEAOID	17
#define CHAROID		18
#define INT8OID		20
#define INT2OID		21
#define INT4OID		23
#define TEXTOID		25
#define OIDOID		26
#define XMLOID		142
#define FLOAT4OID	700
#define MACADDROID	829
#define INETOID		869
#define BPCHAROID	1042
#define VARCHAROID	1043
#define CTIDOID		1247
#define UUIDOID		2950

#define PG_PROTOCOL(m,n)   (((m)<<16)|(n))

//! @decl void create()
//! @decl void create(string host, void|string database, void|string user,@
//!                   void|string password, void|mapping(string:mixed) options)
//!
//! With no arguments, this function initializes (reinitializes if a
//! connection had been previously set up) a connection to the
//! PostgreSQL backend. Since PostgreSQL requires a database to be
//! selected, it will try to connect to the default database. The
//! connection may fail however for a variety of reasons, in this case
//! the most likely of all is because you don't have enough authority
//! to connect to that database. So use of this particular syntax is
//! discouraged.
//!
//! The host argument can have the syntax @expr{"hostname"@} or
//! @expr{"hostname:portname"@}. This allows to specify the TCP/IP
//! port to connect to. If it is @expr{0@} or @expr{""@}, it will try
//! to connect to localhost, default port.
//!
//! The database argument specifies the database to connect to. If
//! @expr{0@} or @expr{""@}, it will try to connect to the specified
//! database.
//!
//! The options argument currently supports two options: use_ssl and force_ssl
//!
//! @note
//! You need to have a database selected before using the sql-object,
//! otherwise you'll get exceptions when you try to query it. Also
//! notice that this function @b{can@} raise exceptions if the db
//! server doesn't respond, if the database doesn't exist or is not
//! accessible by you.
//!
//! @seealso
//!   @[Postgres.postgres], @[Sql.Sql], @[postgres->select_db]
protected void create(void|string _host, void|string _database,
 void|string _user, void|string _pass, void|mapping(string:mixed) _options) {
  pass = _pass; _pass = "CENSORED"; String.secure(pass);
  user = _user; database = _database; host = _host || PGSQL_DEFAULT_HOST;
  options = _options || ([]);
  if(search(host,":")>=0 && sscanf(_host,"%s:%d",host,port)!=2)
    ERROR("Error in parsing the hostname argument\n");
  if(!port)
    port = PGSQL_DEFAULT_PORT;
  _querymutex=Thread.Mutex();
  _stealmutex=Thread.Mutex();
  reconnect();
}

//! @decl string error()
//!
//! This function returns the textual description of the last
//! server-related error. Returns @expr{0@} if no error has occurred
//! yet. It is not cleared upon reading (can be invoked multiple
//! times, will return the same result until a new error occurs).
//!
//! To clear the error, pass 1 as argument.
//!
//! @seealso
//!   big_query
string error(void|int clear) {
  string s=lastmessage;
  if(clear)
    lastmessage=UNDEFINED;
  return s;
}

//! @decl string host_info()
//!
//! This function returns a string describing what host are we talking to,
//! and how (TCP/IP or UNIX sockets).

string host_info() {
  return sprintf("Via fd:%d over TCP/IP to %s:%d",_c.query_fd(),host,port);
}

final private object getsocket(void|int nossl) {
  object lcon = Stdio.File();
  if(!lcon.connect(host,port))
    return UNDEFINED;

  object fcon;
#if constant(SSL.sslfile)
  if(!nossl && (options->use_ssl || options->force_ssl)) {
     PD("SSLRequest\n");
     { object c=.pgsql_util.PGassist();
       lcon.write(({c.plugint32(8),c.plugint32(PG_PROTOCOL(1234,5679))}));
     }
     switch(lcon.read(1)) {
       case "S":
         SSL.context context = SSL.context();
         context->random = Crypto.Random.random_string;
         fcon=.pgsql_util.PGconnS(lcon, context);
	 if(fcon)
	   return fcon;
       default:lcon.close();
	 if(!lcon.connect(host,port))
	   return UNDEFINED;
       case "N":
	 if(options->force_ssl)
	   ERROR("Encryption not supported on connection to %s:%d\n",
	    host,port);
     }
  }
#else
  if(options->force_ssl)
    ERROR("Encryption library missing, cannot establish connection to %s:%d\n",
     host,port);
#endif
  fcon=.pgsql_util.PGconn(lcon,this);
  return fcon;
}

//! Cancels the currently running query.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void cancelquery() {
  if(qstate==inquery) {
    qstate=cancelpending;
    object lcon;
    PD("CancelRequest\n");
    if(!(lcon=getsocket(1)))
      ERROR("Cancel connect failed\n");
    lcon.write(({_c.plugint32(16),_c.plugint32(PG_PROTOCOL(1234,5678)),
     _c.plugint32(backendpid),cancelsecret}));
    lcon.close();
  }
}

//! Returns the old cachedepth, sets the new cachedepth for prepared
//! statements automatic caching.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int setcachedepth(void|int newdepth) {
  int olddepth=cachedepth;
  if(!zero_type(newdepth) && newdepth>=0)
    cachedepth=newdepth;
  return olddepth;
}

//! Returns the old timeout, sets the new timeout for long running
//! queries.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int settimeout(void|int newtimeout) {
  int oldtimeout=timeout;
  if(!zero_type(newtimeout) && newtimeout>0)
    timeout=newtimeout;
  return oldtimeout;
}

//! Returns the old portalbuffersize, sets the new portalbuffersize
//! for buffering partially concurrent queries.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int setportalbuffersize(void|int newportalbuffersize) {
  int oldportalbuffersize=portalbuffersize;
  if(!zero_type(newportalbuffersize) && newportalbuffersize>0)
    portalbuffersize=newportalbuffersize;
  return oldportalbuffersize;
}

//! Returns the old fetchlimit, sets the new fetchlimit to interleave
//! queries.
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int setfetchlimit(void|int newfetchlimit) {
  int oldfetchlimit=_fetchlimit;
  if(!zero_type(newfetchlimit) && newfetchlimit>=0)
    _fetchlimit=newfetchlimit;
  return oldfetchlimit;
}

final private string glob2reg(string glob) {
  if (!glob||!sizeof(glob))
    return "%";
  return replace(glob,({"*","?","\\","%","_"}),({"%","_","\\\\","\\%","\\_"}));
}

final private string addnlifpresent(void|string msg) {
  return msg?msg+"\n":"";
}

final private string pinpointerror(void|string query,void|string offset) {
  if(!query)
    return "";
  int k=(int)offset;
  if(k<=0)
    return MARKSTART+query+MARKEND;
  return MARKSTART+(k>1?query[..k-2]:"")+MARKERROR+query[k-1..]+MARKEND;
}

final int _decodemsg(void|state waitforstate) {
#ifdef DEBUG
  { array line;
#ifdef DEBUGMORE
    line=backtrace();
#endif
    PD("Waiting for state %O %O\n",waitforstate,line&&line[sizeof(line)-2]);
  }
#endif
  while(_mstate!=waitforstate) {
    if(_mstate!=unauthenticated) {
      if(qstate==cancelpending)
        qstate=canceled,sendclose();
      if(_c.flushed && qstate==inquery && !_c.bpeek(0)) {
        int tcurr=time();
        int told=tcurr+timeout;
        while(!_c.bpeek(told-tcurr))
          if((tcurr=time())-told>=timeout) {
            sendclose();cancelquery();
            break;
          }
      }
    }
    int msgtype=_c.getbyte();
    int msglen=_c.getint32();
    enum errortype { noerror=0, protocolerror, protocolunsupported };
    errortype errtype=noerror;
    switch(msgtype) {
      void getcols() {
        int bintext=_c.getbyte();
        array a;
        int cols=_c.getint16();
        msglen-=4+1+2+2*cols;
        foreach(a=allocate(cols,([]));;mapping m)
          m->type=_c.getint16();
        if(_c.portal) {	      // Discard column info, and make it line oriented
	  a=({(["type":bintext?BYTEAOID:TEXTOID,"name":"line"])});
	  _c.portal->_datarowdesc=a;
        }
        _mstate=gotrowdescription;
      };
      array(string) getstrings() {
        string s;
        if(msglen<1)
          errtype=protocolerror;
        s=_c.getstring(msglen);
        if(s[--msglen])
          errtype=protocolerror;
        if(!msglen)
          return ({});
        s=s[..msglen-1];msglen=0;
        return s/"\0";
      };
      void getresponse() {
        msglen-=4;
        msgresponse=([]);
        foreach(getstrings();;string f)
	  if(sizeof(f))
            msgresponse[f[..0]]=f[1..];
        PD("%O\n",msgresponse);
      };
      case 'R':PD("Authentication\n");
      { string sendpass;
        int authtype;
        msglen-=4+4;
        switch(authtype=_c.getint32()) {
          case 0:PD("Ok\n");
	    _mstate=authenticated;
            break;
          case 2:PD("KerberosV5\n");
            errtype=protocolunsupported;
            break;
          case 3:PD("ClearTextPassword\n");
            sendpass=pass;
            break;
          case 4:PD("CryptPassword\n");
            if(msglen<2)
              errtype=protocolerror;
            sendpass=_c.getstring(msglen);msglen=0;
            errtype=protocolunsupported;
            break;
          case 5:PD("MD5Password\n");
            if(msglen<4)
              errtype=protocolerror;
#if constant(Crypto.MD5.hash)
#define md5hex(x) String.string2hex(Crypto.MD5.hash(x))
            sendpass=md5hex(pass+user);
            sendpass="md5"+md5hex(sendpass+_c.getstring(msglen));
#else
            _c.getstring(msglen);
            errtype=protocolunsupported;
#endif
            msglen=0;
            break;
          case 6:PD("SCMCredential\n");
            errtype=protocolunsupported;
            break;
          case 7:PD("GSS\n");
            errtype=protocolunsupported;
            break;
          case 9:PD("SSPI\n");
            errtype=protocolunsupported;
            break;
          case 8:PD("GSSContinue\n");
            errtype=protocolunsupported;
            if(msglen<1)
              errtype=protocolerror;
            SSauthdata=_c.getstring(msglen);msglen=0;
            break;
          default:PD("Unknown Authentication Method %c\n",authtype);
            errtype=protocolunsupported;
            break;
        }
        switch(errtype) {
          case noerror:
            if(_mstate==unauthenticated)
              _c.sendcmd(({"p",_c.plugint32(4+sizeof(sendpass)+1),
               sendpass,"\0"}),1);
            break;
          default:
          case protocolunsupported:
            ERROR("Unsupported authenticationmethod %c\n",authtype);
            break;
        }
        break;
      }
      case 'K':PD("BackendKeyData\n");
        msglen-=4+4;backendpid=_c.getint32();cancelsecret=_c.getstring(msglen);
        msglen=0;
        break;
      case 'S':PD("ParameterStatus\n");
        msglen-=4;
      { array(string) ts=getstrings();
        if(sizeof(ts)==2) {
          runtimeparameter[ts[0]]=ts[1];
          PD("%s=%s\n",ts[0],ts[1]);
        }
        else
          errtype=protocolerror;
      }
        break;
      case 'Z':PD("ReadyForQuery\n");
        msglen-=4+1;
        backendstatus=_c.getbyte();
        _mstate=readyforquery;
        qstate=queryidle;
        _closesent=0;
        break;
      case '1':PD("ParseComplete\n");
        msglen-=4;
        _mstate=parsecomplete;
        break;
      case 't':
        PD("ParameterDescription (for %s)\n",
	 _c.portal?_c.portal->_portalname:"DISCARDED");
      { array a;
        int cols=_c.getint16();
        msglen-=4+2+4*cols;
        foreach(a=allocate(cols);int i;)
	  a[i]=_c.getint32();
#ifdef DEBUGMORE
        PD("%O\n",a);
#endif
        if(_c.portal)
	  _c.portal->_datatypeoid=a;
        _mstate=gotparameterdescription;
        break;
      }
      case 'T':
        PD("RowDescription (for %s)\n",
	 _c.portal?_c.portal->_portalname:"DISCARDED");
        msglen-=4+2;
      { array a;
        foreach(a=allocate(_c.getint16());int i;) {
          string s;
	  msglen-=sizeof(s=_c.getstring())+1;
          mapping(string:mixed) res=(["name":s]);
          msglen-=4+2+4+2+4+2;
          res->tableoid=_c.getint32()||UNDEFINED;
	  res->tablecolattr=_c.getint16()||UNDEFINED;
          res->type=_c.getint32();
	  { int len=_c.getint16();
            res->length=len>=0?len:"variable";
	  }
          res->atttypmod=_c.getint32();res->formatcode=_c.getint16();
          a[i]=res;
        }
#ifdef DEBUGMORE
        PD("%O\n",a);
#endif
	if(_c.portal)
	  _c.portal->_datarowdesc=a;
        _mstate=gotrowdescription;
        break;
      }
      case 'n':PD("NoData\n");
        msglen-=4;
        _c.portal->_datarowdesc=({});
        _mstate=gotrowdescription;
        break;
      case '2':PD("BindComplete\n");
        msglen-=4;
        _mstate=bindcomplete;
        break;
      case 'D':PD("DataRow\n");
        msglen-=4;
        if(_c.portal) {
#ifdef USEPGsql
          _c.decodedatarow(msglen);msglen=0;
#else
          array a, datarowdesc;
	  _c.portal->_bytesreceived+=msglen;
	  datarowdesc=_c.portal->_datarowdesc;
          int cols=_c.getint16();
	  a=allocate(cols,UNDEFINED);
          msglen-=2+4*cols;
          foreach(a;int i;) {
            int collen=_c.getint32();
            if(collen>0) {
              msglen-=collen;
              mixed value;
              switch(datarowdesc[i]->type) {
	        default:value=_c.getstring(collen);
                  break;
                case CHAROID:
                case BOOLOID:value=_c.getbyte();
                  break;
                case INT8OID:value=_c.getint64();
                  break;
                case FLOAT4OID:value=(float)_c.getstring(collen);
                  break;
                case INT2OID:value=_c.getint16();
                  break;
	        case OIDOID:
                case INT4OID:value=_c.getint32();
              }
	      a[i]=value;
            }
            else if(!collen)
              a[i]="";
          }
	  a=({a});
	  _c.portal->_datarows+=a;
	  _c.portal->_inflight-=sizeof(a);
#endif
        }
        else
	  _c.getstring(msglen),msglen=0;
        _mstate=dataready;
        break;
      case 's':PD("PortalSuspended\n");
        msglen-=4;
        _mstate=portalsuspended;
        break;
      case 'C':PD("CommandComplete\n");
      { msglen-=4;
        if(msglen<1)
          errtype=protocolerror;
        string s=_c.getstring(msglen-1);
        if(_c.portal)
          _c.portal->_statuscmdcomplete=s;
        PD("%s\n",s);
        if(_c.getbyte())
          errtype=protocolerror;
        msglen=0;
        _mstate=commandcomplete;
        break;
      }
      case 'I':PD("EmptyQueryResponse\n");
        msglen-=4;
        _mstate=commandcomplete;
        break;
      case '3':PD("CloseComplete\n");
        msglen-=4;
        _closesent=0;
        break;
      case 'd':PD("CopyData\n");
        msglen-=4;
        if(msglen<0)
          errtype=protocolerror;
        if(_c.portal) {
	  _c.portal->_bytesreceived+=msglen;
          _c.portal->_datarows+=({({_c.getstring(msglen)})});
        }
	msglen=0;
        _mstate=dataready;
        break;
      case 'H':PD("CopyOutResponse\n");
        getcols();
        if(_c.portal)
	  _c.portal->_fetchlimit=0;		   // disables further Executes
        break;
      case 'G':PD("CopyInResponse\n");
        getcols();
        _mstate=copyinresponse;
        break;
      case 'c':PD("CopyDone\n");
        msglen-=4;
        break;
      case 'E':PD("ErrorResponse\n");
        getresponse();
        switch(msgresponse->C) {
#define USERERROR(msg)	throw(({msg, backtrace()[..<1]}))
          case "P0001":
            lastmessage=sprintf("%s: %s",msgresponse->S,msgresponse->M);
	    USERERROR(lastmessage
	     +"\n"+pinpointerror(_c.portal->query,msgresponse->P));
	    break;
	  default:
            lastmessage=sprintf("%s %s:%s %s\n (%s:%s:%s)\n%s%s%s%s\n%s",
	     msgresponse->S,msgresponse->C,msgresponse->P||"",msgresponse->M,
	     msgresponse->F||"",msgresponse->R||"",msgresponse->L||"",
	     addnlifpresent(msgresponse->D),addnlifpresent(msgresponse->H),
	     pinpointerror(_c.portal&&_c.portal->query,msgresponse->P),
	     pinpointerror(msgresponse->q,msgresponse->p),
	     addnlifpresent(msgresponse->W));
            switch(msgresponse->S) {
	      case "PANIC":werror(lastmessage);
            }
	    USERERROR(lastmessage);
        }
        break;
      case 'N':PD("NoticeResponse\n");
        getresponse();
        if(clearmessage)
	  clearmessage=0,lastmessage=UNDEFINED;
        lastmessage=sprintf("%s%s %s: %s",
	 lastmessage?lastmessage+"\n":"",
	 msgresponse->S,msgresponse->C,msgresponse->M);
        break;
      case 'A':PD("NotificationResponse\n");
      { msglen-=4+4;
        int pid=_c.getint32();
        string condition,extrainfo=UNDEFINED;
        { array(string) ts=getstrings();
          switch(sizeof(ts)) {
            case 0:errtype=protocolerror;
	      break;
            default:errtype=protocolerror;
            case 2:extrainfo=ts[1];
            case 1:condition=ts[0];
          }
        }
        PD("%d %s\n%s\n",pid,condition,extrainfo);
        array cb;
        if((cb=notifylist[condition]||notifylist[""])
         && (pid!=backendpid || sizeof(cb)>1 && cb[1]))
	  cb[0](pid,condition,extrainfo,@cb[2..]);
        break;
      }
      default:PD("Unknown message received %c\n",msgtype);
        msglen-=4;PD("%O\n",_c.getstring(msglen));msglen=0;
        errtype=protocolunsupported;
        break;
    }
    if(msglen)
      errtype=protocolerror;
    switch(errtype) {
      case protocolunsupported:
        ERROR("Unsupported servermessage received %c\n",msgtype);
        break;
      case protocolerror:
        reconnect(1);
        ERROR("Protocol error with databasel %s@%s:%d/%s\n",
         user,host,port,database);
        break;
      case noerror:
        break;
    }
    if(zero_type(waitforstate))
      break;
  }
  PD("Found state %O\n",_mstate);
  return _mstate;
}

#ifndef UNBUFFEREDIO
private int read_cb(mixed foo, string d) {
  _c.unread(d);
  do _decodemsg();
  while(_c.bpeek(0)==1);
  return 0;
}
#endif

void destroy() {
  cancelquery();
  if(_c)
    _c.sendterminate();
}

private void reconnect(void|int force) {
  if(_c) {
    reconnected++;
#ifdef DEBUG
    ERROR("While debugging, reconnects are forbidden\n");
    exit(1);
#endif
    if(!force)
      _c.sendterminate();
    foreach(prepareds;;mapping tprepared)
      m_delete(tprepared,"preparedname");
  }
  if(!(_c=getsocket()))
    ERROR("Couldn't connect to database on %s:%d\n",host,port);
  _closesent=0;
  _mstate=unauthenticated;
  qstate=queryidle;
  runtimeparameter=([]);
  array(string) plugbuf=({"",_c.plugint32(PG_PROTOCOL(3,0))});
  if(user)
    plugbuf+=({"user\0",user,"\0"});
  if(database)
    plugbuf+=({"database\0",database,"\0"});
  foreach(sessiondefaults;string name;string value)
    plugbuf+=({name,"\0",value,"\0"});
  plugbuf+=({"\0"});
  int len=4;
  foreach(plugbuf;;string s)
    len+=sizeof(s);
  plugbuf[0]=_c.plugint32(len);
  _c.write(plugbuf);
  PD("%O\n",plugbuf);
  _decodemsg(readyforquery);
  PD("%O\n",runtimeparameter);
}

//! @decl void reload()
//!
//! Resets the connection to the database. Can be used for
//! a variety of reasons, for example to detect the status of a connection.
void reload(void|int special) {
  mixed err;
  int didsync;
  if(err = catch {
      sendclose(1);
      PD("Portalsinflight: %d\n",portalsinflight);
      if(!portalsinflight) {
        if(!earlyclose) {
          PD("Sync\n");
          _c.sendcmd(({"S",_c.plugint32(4)}),2);
        }
        didsync=1;
        if(!special) {
          _decodemsg(readyforquery);
          foreach(prepareds;;mapping tprepared) {
            m_delete(tprepared,"datatypeoid");
            m_delete(tprepared,"datarowdesc");
          }
        }
      }
      earlyclose=0;
    }) {
    earlyclose=0;
    PD("%O\n",err);
    reconnect(1);
  }
  else if(didsync && special==2)
    _decodemsg(readyforquery);
#ifndef UNBUFFEREDIO
  _c.set_read_callback(read_cb);
#endif
}

//! @decl void select_db(string dbname)
//!
//! This function allows you to connect to a database. Due to
//! restrictions of the Postgres frontend-backend protocol, you always
//! have to be connected to a database, so in fact this function just
//! allows you to connect to a different database on the same server.
//!
//! @note
//! This function @b{can@} raise exceptions if something goes wrong
//! (backend process not running, not enough permissions..)
//!
//! @seealso
//!   create
void select_db(string dbname) {
  database=dbname;
  reconnect();
}

//! With PostgreSQL you can LISTEN to NOTIFY events.
//! This function allows you to detect and handle such events.
//!
//! @param condition
//!    Name of the notification event we're listening
//!    to.  A special case is the empty string, which matches all events,
//!    and can be used as fallback function which is called only when the
//!    specific condition is not handled..
//!
//! @param notify_cb
//!    Function to be called on receiving a notification-event of
//!    condition @[condition].
//!    The callback function is invoked with
//!     @expr{void notify_cb(pid,condition,extrainfo, .. args);@}
//!    @[pid] is the process id of the database session that originated
//!    the event.  @[condition] contains the current condition.
//!    @[extrainfo] contains optional extra information specified by
//!    the database.
//!    The rest of the arguments to @[notify_cb] are passed
//!    verbatim from @[args].
//!    The callback function must return no value.
//!
//! @param selfnotify
//!    Normally notify events generated by your own session are ignored.
//!    If you want to receive those as well, set @[selfnotify] to one.
//!
//! @param args
//!    Extra arguments to pass to @[notify_cb].
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void set_notify_callback(string condition,
 void|function(int,string,string,mixed ...:void) notify_cb,void|int selfnotify,
  mixed ... args) {
  if(!notify_cb)
    m_delete(notifylist,condition);
  else {
    array old=notifylist[condition];
    if(!old)
      old=({notify_cb});
    if(selfnotify||args)
      old+=({selfnotify});
    if(args)
      old+=args;
    notifylist[condition]=old;
  }
}

//! This function quotes magic characters inside strings embedded in a
//! textual query.  Quoting must not be done for parameters passed in
//! bindings.
//!
//! @seealso
//!   big_query
string quote(string s) {
  string r=runtimeparameter->standard_conforming_strings;
  if(r && r=="on")
    return replace(s, "'", "''");
  return replace(s, ({ "'", "\\" }), ({ "''", "\\\\" }) );
}

//! This function creates a new database with the given name (assuming we
//! have enough permissions to do this).
//!
//! @seealso
//!   drop_db
void create_db(string db) {
  big_query("CREATE DATABASE :db",([":db":db]));
}

//! This function destroys a database and all the data it contains (assuming
//! we have enough permissions to do so).
//!
//! @seealso
//!   create_db
void drop_db(string db) {
  big_query("DROP DATABASE :db",([":db":db]));
}

//! This function returns a string describing the server we are
//! talking to. It has the form @expr{"servername/serverversion"@}
//! (like the HTTP protocol description) and is most useful in
//! conjunction with the generic SQL-server module.
string server_info () {
  return DRIVERNAME"/"+(runtimeparameter->server_version||"unknown");
}

//! Lists all the databases available on the server.
//! If glob is specified, lists only those databases matching it.
array(string) list_dbs (void|string glob) {
 array row,ret=({});
 object res=big_query("SELECT d.datname "
  "FROM pg_database d "
  "WHERE d.datname ILIKE :glob "
  "ORDER BY d.datname",
  ([":glob":glob2reg(glob)]));
 while(row=res->fetch_row())
   ret+=({row[0]});
 return ret;
}

//! Returns an array containing the names of all the tables and views in the
//! path in the currently selected database.
//! If a glob is specified, it will return only those tables with matching
//! names.
array(string) list_tables (void|string glob) {
 array row,ret=({});	         // This query might not work on PostgreSQL 7.4
 object res=big_query(		 // due to missing schemasupport
  "SELECT CASE WHEN 'public'=n.nspname THEN '' ELSE n.nspname||'.' END "
  "  ||c.relname AS name "
  "FROM pgcatalog.pgclass c "
  "  LEFT JOIN pgcatalog.pg_namespace n ON n.oid=c.relnamespace "
  "WHERE c.relkind IN ('r','v') AND n.nspname<>'pgcatalog' "
  "  AND n.nspname !~ '^pg_toast' AND pgcatalog.pg_table_is_visible(c.oid) "
  "  AND c.relname ILIKE :glob "
  "  ORDER BY 1",
  ([":glob":glob2reg(glob)]));
 while(row=res->fetch_row())
   ret+=({row[0]});
 return ret;
}

//! Returns a mapping, indexed on the column name, of mappings describing
//! the attributes of a table of the current database.
//! If a glob is specified, will return descriptions only of the columns
//! matching it.
//!
//! The currently defined fields are:
//!
//! @mapping
//!   @member int "is_shared"
//!
//!   @member string "owner"
//!     Tableowner
//!
//!   @member string "length"
//!     Size of the columndatatype
//!
//!   @member string "text"
//!     A textual description of the internal (to the server) type-name
//!
//!   @member mixed "default"
//!     Default value for the column
//!
//!   @member mixed "schema"
//!     Schema the table belongs to
//!
//!   @member mixed "table"
//!     Name of the table
//!
//!   @member mixed "kind"
//!     Type of table
//!
//!   @member mixed "has_index"
//!     If the table has any indices
//!
//!   @member mixed "has_primarykey"
//!     If the table has a primary key
//!
//!   @member mixed "rowcount"
//!     Estimated rowcount of the table
//!
//!   @member mixed "pagecount"
//!     Estimated pagecount of the table
//!
//! @endmapping
//!
//! Setting wild to * will include system columns in the list.
//!
array(mapping(string:mixed)) list_fields(void|string table, void|string wild) {
  array row, ret=({});
  string schema=UNDEFINED;

  sscanf(table||"*", "%s.%s", schema, table);

  object res = big_query(
  "SELECT a.attname, a.atttypid, t.typname, a.attlen, "
  " c.relhasindex, c.relhaspkey, c.reltuples, c.relpages, "
  " c.relisshared, t.typdefault, "
  " n.nspname, c.relname, "
  " CASE c.relkind "
  "  WHEN 'r' THEN 'table' "
  "  WHEN 'v' THEN 'view' "
  "  WHEN 'i' THEN 'index' "
  "  WHEN 'S' THEN 'sequence' "
  "  WHEN 's' THEN 'special' "
  "  WHEN 't' THEN 'toastable' "			    // pun intended :-)
  "  WHEN 'c' THEN 'composite' "
  "  ELSE c.relkind::TEXT END AS relkind, "
  " r.rolname "
  "FROM pgcatalog.pgclass c "
  "  LEFT JOIN pgcatalog.pg_namespace n ON n.oid=c.relnamespace "
  "  JOIN pgcatalog.pg_roles r ON r.oid=c.relowner "
  "  JOIN pgcatalog.pg_attribute a ON c.oid=a.attrelid "
  "  JOIN pgcatalog.pg_type t ON a.atttypid=t.oid "
  "WHERE c.relname ILIKE :table AND "
  "  (n.nspname ILIKE :schema OR "
  "   :schema IS NULL "
  "   AND n.nspname<>'pgcatalog' AND n.nspname !~ '^pg_toast') "
  "   AND a.attname ILIKE :wild "
  "   AND (a.attnum>0 OR '*'=:realwild) "
  "ORDER BY n.nspname,c.relname,a.attnum,a.attname",
  ([":schema":glob2reg(schema),":table":glob2reg(table),
   ":wild":glob2reg(wild),":realwild":wild]));

  array colnames=res->fetch_fields();
  { mapping(string:string) renames=([
      "attname":"name",
      "nspname":"schema",
      "relname":"table",
      "rolname":"owner",
      "typname":"type",
      "attlen":"length",
      "typdefault":"default",
      "relisshared":"is_shared",
      "atttypid":"typeoid",
      "relkind":"kind",
      "relhasindex":"has_index",
      "relhaspkey":"has_primarykey",
      "reltuples":"rowcount",
      "relpages":"pagecount",
     ]);
    foreach(colnames;int i;mapping m) {
      string nf,field=m->name;
      if(nf=renames[field])
        field=nf;
      colnames[i]=field;
    }
  }

#define delifzero(m,field) if(!(m)[field]) m_delete(m,field)

  while(row=res->fetch_row()) {
    mapping m=mkmapping(colnames,row);
    delifzero(m,"is_shared");
    delifzero(m,"has_index");
    delifzero(m,"has_primarykey");
    delifzero(m,"default");
    ret+=({m});
  }
  return ret;
}

private int oidformat(int oid) {
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
    case INETOID:
    case BPCHAROID:
    case VARCHAROID:
    case CTIDOID:
    case UUIDOID:
      return 1; //binary
  }
  return 0;     // text
}

final void _sendexecute(int fetchlimit) {
  string portalname=_c.portal->_portalname;
  PD("Execute portal %s fetchlimit %d\n",portalname,fetchlimit);
  _c.sendcmd(({"E",_c.plugint32(4+sizeof(portalname)+1+4),portalname,
   "\0",_c.plugint32(fetchlimit)}),!!fetchlimit);
  _c.portal->_inflight+=fetchlimit;
  if(!fetchlimit) {
    earlyclose=1;
    if(sizeof(portalname)) {
      PD("Close portal %s & Sync\n",portalname);
      _c.sendcmd(({"C",_c.plugint32(4+1+sizeof(portalname)+1),
       "P",portalname,"\0"}));
    }
    _c.sendcmd(({"S",_c.plugint32(4)}),2);
  }
}

final private void sendclose(void|int hold) {
  string portalname;
  portalsinflight--;
  if(_c.portal && (portalname=_c.portal->_portalname)) {
    _c.portal->_portalname = UNDEFINED;
    _c.setportal();
#ifdef DEBUGMORE
    PD("Closetrace %O\n",backtrace());
#endif
    if(!sizeof(portalname))
      unnamedportalinuse--;
    if(sizeof(portalname)) {
      if(!earlyclose) {
        PD("Close portal %s\n",portalname);
        _c.sendcmd(({"C",_c.plugint32(4+1+sizeof(portalname)+1),
         "P",portalname,"\0"}),!hold||portalsinflight?1:0);
      }
      _closesent=1;
    }
  }
}

final private string trbackendst(int c) {
  switch(c) {
    case 'I':return "idle";
    case 'T':return "intransaction";
    case 'E':return "infailedtransaction";
  }
  return "unknown";
}

//! Returns the current commitstatus of the connection.  Returns either one of:
//!  unknown
//!  idle
//!  intransaction
//!  infailedtransaction
//!
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
final string status_commit() {
  return trbackendst(backendstatus);
}

//! This is the only provided interface which allows you to query the
//! database. If you wish to use the simpler "query" function, you need to
//! use the @[Sql.Sql] generic SQL-object.
//!
//! It returns a pgsql_result object (which conforms to the
//! @[Sql.sql_result] standard interface for accessing data). I
//! recommend using @[query()] for simpler queries (because it is
//! easier to handle, but stores all the result in memory), and
//! @[big_query()] for queries you expect to return huge amounts of
//! data (it's harder to handle, but fetches results on demand).
//!
//! @note
//! This function @b{can@} raise exceptions.
//!
//! @note
//! This function does not support multiple queries in one querystring.
//! I.e. it allows for but does not require a trailing semicolon, but it
//! simply ignores any commands after the first semicolon.  This can be
//! viewed as a limited protection against SQL-injection attacks.
//!
//! @seealso
//!   @[Sql.Sql], @[Sql.sql_result]
object big_query(string q,void|mapping(string|int:mixed) bindings) {
  string preparedname="";
  string portalname="";
  if(stringp(q) && String.width(q)>8)
    q=string_to_utf8(q);
  array(string|int) paramValues;
  if(bindings) {
    int pi=0,rep=0;
    paramValues=allocate(sizeof(bindings));
    array(string) from=allocate(sizeof(bindings));
    array(string) to=allocate(sizeof(bindings));
    foreach(bindings; mixed name; mixed value) {
      if(stringp(name)) {	       // Throws if mapping key is empty string
        if(name[0]!=':')
          name=":"+name;
        if(name[1]=='_') {	       // Special option parameter
				       // No options supported at the moment
          continue;
        }
      }
      from[rep]=name;
      string rval;
      if(multisetp(value)) {
        rval=sizeof(value) ? indices(value)[0] : "";
      }
      else {
        if(zero_type(value))
          paramValues[pi++]=UNDEFINED; // NULL
        else {
          if(stringp(value) && String.width(value)>8)
            value=string_to_utf8(value);
          paramValues[pi++]=value;
        }
        rval="$"+(string)pi;
      }
      to[rep++]=rval;
    }
    if(rep--)
      q=replace(q,from[..rep],to[..rep]);
    paramValues= pi ? paramValues[..pi-1] : ({});
  }
  else
    paramValues = ({});
  int tstart;
  mapping(string:mixed) tprepared;
  if(sizeof(q)>=MINPREPARELENGTH) {
    if(tprepared=prepareds[q]) {
      if(tprepared->preparedname)
        preparedname=tprepared->preparedname;
      else if((tstart=tprepared->trun)
       && tprepared->tparse*FACTORPLAN>=tstart)
	preparedname=PREPSTMTPREFIX+(string)pstmtcount++;
    }
    else {
      if(totalhits>=cachedepth) {
        array(string) plugbuf=({});
        foreach(prepareds;string ind;tprepared) {
	  int oldhits=tprepared->hits;
	  totalhits-=oldhits-(tprepared->hits=oldhits>>1);
	  if(oldhits<=1) {
	    string oldprep=tprepared->preparedname;
	    if(oldprep) {
	      PD("Close statement %s\n",oldprep);
              plugbuf+=({"C",_c.plugint32(4+1+sizeof(oldprep)+1),
               "S",oldprep,"\0"});
	    }
	    m_delete(prepareds,ind);
	  }
        }
        if(sizeof(plugbuf))
	  _c.sendcmd(plugbuf,1);			      // close expireds
        PD("%O\n",plugbuf);
      }
      prepareds[q]=tprepared=([]);
    }
    tstart=gethrtime();
  }					  // pgsql_result autoassigns to portal
  .pgsql_util.pgsql_result(this,tprepared,q,_fetchlimit,portalbuffersize);
  if(unnamedportalinuse)
    portalname=PORTALPREFIX+(string)pportalcount++;
  else
    unnamedportalinuse++;
  _c.portal->_portalname=portalname;
  qstate=inquery;
  portalsinflight++;
  clearmessage=1;
  mixed err;
  if(err = catch {
      _c.set_read_callback(0);
      if(!sizeof(preparedname) || !tprepared || !tprepared->preparedname) {
        PD("Parse statement %s\n",preparedname);
        // Even though the protocol doesn't require the Parse command to be
        // followed by a flush, it makes a VERY noticeable difference in
        // performance if it is omitted; seems like a flaw in the PostgreSQL
        // server
        _c.sendcmd(({"P",_c.plugint32(4+sizeof(preparedname)+1+sizeof(q)+1+2),
         preparedname,"\0",q,"\0",_c.plugint16(0)}),3);
        PD("Query: %O\n",q);
      }			 // sends Parameter- and RowDescription for 'S'
      if(!tprepared || !tprepared->datatypeoid) {
        PD("Describe statement %s\n",preparedname);
        _c.sendcmd(({"D",_c.plugint32(4+1+sizeof(preparedname)+1),
         "S",preparedname,"\0"}),1);
      }
      else {
        _c.portal->_datatypeoid=tprepared->datatypeoid;
        _c.portal->_datarowdesc=tprepared->datarowdesc;
      }
      { array(string) plugbuf=({"B",UNDEFINED});
        int len=4+sizeof(portalname)+1+sizeof(preparedname)+1
         +2+sizeof(paramValues)*(2+4)+2+2;
        plugbuf+=({portalname,"\0",preparedname,"\0",
         _c.plugint16(sizeof(paramValues))});
        if(!tprepared || !tprepared->datatypeoid) {
          _decodemsg(gotparameterdescription);
          if(tprepared)
            tprepared->datatypeoid=_c.portal->_datatypeoid;
        }
        array dtoid=_c.portal->_datatypeoid;
        foreach(dtoid;;int textbin)
          plugbuf+=({_c.plugint16(oidformat(textbin))});
        plugbuf+=({_c.plugint16(sizeof(paramValues))});
        foreach(paramValues;int i;mixed value) {
          if(zero_type(value))
            plugbuf+=({_c.plugint32(-1)});				// NULL
          else
            switch(dtoid[i]) {
              default:
              { int k;
                len+=k=sizeof(value=(string)value);
                plugbuf+=({_c.plugint32(k),value});
                break;
              }
              case BOOLOID:plugbuf+=({_c.plugint32(1)});len++;
                switch(stringp(value)?value[0]:value) {
        	  case 'o':case 'O':
        	    _c.plugbyte(stringp(value)&&sizeof(value)>1
                     &&(value[1]=='n'||value[1]=='N'));
        	    break;
                  case 0:case 'f':case 'F':case 'n':case 'N':
                    plugbuf+=({_c.plugbyte(0)});
                    break;
                  default:
                    plugbuf+=({_c.plugbyte(1)});
                    break;
                }
                break;
              case CHAROID:plugbuf+=({_c.plugint32(1)});len++;
                if(intp(value))
                  plugbuf+=({_c.plugbyte(value)});
                else {
        	  value=(string)value;
        	  if(sizeof(value)!=1)
        	    ERROR("\"char\" types must be 1 byte wide, got %d\n",
        	     sizeof(value));
                  plugbuf+=({value});
                }
                break;
              case INT8OID:len+=8;
                plugbuf+=({_c.plugint32(8),_c.plugint64((int)value)});
                break;
              case INT4OID:len+=4;
                plugbuf+=({_c.plugint32(4),_c.plugint32((int)value)});
                break;
              case INT2OID:len+=2;
                plugbuf+=({_c.plugint32(2),_c.plugint16((int)value)});
                break;
            }
        }
        if(!tprepared || !tprepared->datarowdesc) {
          _decodemsg(gotrowdescription);
          if(tprepared)
            tprepared->datarowdesc=_c.portal->_datarowdesc;
        }
        { array a;int i;
          len+=(i=sizeof(a=_c.portal->_datarowdesc))*2;
          plugbuf+=({_c.plugint16(i)});
          foreach(a;;mapping col)
            plugbuf+=({_c.plugint16(oidformat(col->type))});
        }
        plugbuf[1]=_c.plugint32(len);
        PD("Bind portal %s statement %s\n",portalname,preparedname);
        _c.sendcmd(plugbuf);
#ifdef DEBUGMORE
        PD("%O\n",plugbuf);
#endif
      }
      _c.portal->_statuscmdcomplete=UNDEFINED;
      _sendexecute(_fetchlimit && FETCHLIMITLONGRUN);
      if(tprepared) {
        _decodemsg(bindcomplete);
        int tend=gethrtime();
        if(tend==tstart)
	  m_delete(prepareds,q);
        else {
          tprepared->hit++;
          totalhits++;
	  if(!tprepared->preparedname) {
	    if(sizeof(preparedname))
	      tprepared->preparedname=preparedname;
	    tstart=tend-tstart;
	    if(tprepared->tparse>tstart)
	      tprepared->tparse=tstart;
	    else
	      tstart=tprepared->tparse;
	  }
	  tprepared->trunstart=tend;
        }
      }
    }) {
    PD("%O\n",err);
    reload(1);
    backendstatus=UNDEFINED;
    throw(err);
  }
  { object tportal=_c.portal;	        // Make copy, because it might dislodge
    tportal->fetch_row(1);	        // upon initial fetch_row()
    return tportal;
  }
}

//! This is an alias for @[big_query()], since @[big_query()] already supports
//! streaming of multiple simultaneous queries through the same connection.
//!
//! @seealso
//!   @[big_query], @[Sql.Sql], @[Sql.sql_result]
object streaming_query(string q,void|mapping(string|int:mixed) bindings) {
  return big_query(q,bindings);
}
