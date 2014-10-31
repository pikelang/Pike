/*
 * This is the PostgreSQL direct network module for Pike.
 */

//! This is an interface to the PostgreSQL database
//! server. This module is independent of any external libraries.
//! Note that you @b{do not@} need to have a
//! PostgreSQL server running on your host to use this module: you can
//! connect to the database over a TCP/IP socket.
//!
//! This module replaces the functionality of the older @[Sql.postgres]
//! and @[Postgres.postgres] modules.
//!
//! This module supports the following features:
//! @ul
//! @item
//!  PostgreSQL network protocol version 3, authentication methods
//!   currently supported are: cleartext and MD5 (recommended).
//! @item
//!  Streaming queries which do not buffer the whole resultset in memory.
//! @item
//!  Automatic binary transfers to and from the database for most common
//!   datatypes (amongst others: integer, text and bytea types).
//! @item
//!  Automatic character set conversion and native wide string support.
//!  Supports UTF8/Unicode for multibyte characters, and all single-byte
//!  character sets supported by the database.
//! @item
//!  SQL-injection protection by allowing just one statement per query
//!   and ignoring anything after the first (unquoted) semicolon in the query.
//! @item
//!  COPY support for streaming up- and download.
//! @item
//!  Accurate error messages.
//! @item
//!  Automatic precompilation of complex queries (session cache).
//! @item
//!  Multiple simultaneous queries on the same database connection.
//! @item
//!  Cancelling of long running queries by force or by timeout.
//! @item
//!  Event driven NOTIFY.
//! @item
//!  SSL encrypted connections (optional or forced).
//! @endul
//! Check the PostgreSQL documentation for further details.
//!
//! @note
//!   Multiple simultaneous queries on the same database connection is a
//!   feature that none of the other database drivers for Pike support.
//!   So, although it's efficient, its use will make switching database drivers
//!   difficult.
//!
//! @seealso
//!  @[Sql.Sql], @[Sql.postgres], @url{http://www.postgresql.org/docs/@}

#pike __REAL_VERSION__

#include "pgsql.h"

#define ERROR(X ...)	     predef::error(X)

int _fetchlimit=FETCHLIMIT;
Thread.Mutex _unnamedportalmux;
protected Thread.Mutex unnamedstatement;
int _portalsinflight;

protected .pgsql_util.PGassist c;
protected string cancelsecret;
protected int backendpid;
protected int backendstatus;
mapping(string:mixed) options;
protected array(string) lastmessage=({});
protected int clearmessage;
protected mapping(string:array(mixed)) notifylist=([]);
mapping(string:string) _runtimeparameter;
mapping(string:mapping(string:mixed)) _prepareds=([]);
protected int pstmtcount;
protected int ptstmtcount;	// Periodically one would like to reset this
				// but checking when this is safe to do
				// probably is more costly than the gain
int _pportalcount;
protected int totalhits;
protected int cachedepth=STATEMENTCACHEDEPTH;
protected int timeout=QUERYTIMEOUT;
protected int portalbuffersize=PORTALBUFFERSIZE;
protected int reconnected;     // Number of times the connection was reset
protected int reconnectdelay;	// Time to next reconnect
#ifdef PG_STATS
protected int skippeddescribe; // Number of times we skipped Describe phase
protected int portalsopened;   // Number of portals opened
protected int prepstmtused;    // Number of times prepared statements were used
#endif
int _msgsreceived;	     // Number of protocol messages received
int _bytesreceived;	     // Number of bytes received
protected int warningsdropcount; // Number of uncollected warnings
protected int warningscollected;
protected int invalidatecache;
protected Thread.Queue qportals;
mixed _delayederror;
protected function (:void) readyforquery_cb;

string _host;
protected string database, user, pass;
int _port;
protected Thread.Mutex waitforauth;
protected Thread.Condition waitforauthready;
int _readyforquerycount;

protected string _sprintf(int type, void|mapping flags) {
  string res=UNDEFINED;
  switch(type) {
    case 'O':
      res=sprintf(DRIVERNAME"(%s@%s:%d/%s,%d)",
       user,_host,_port,database,backendpid);
      break;
  }
  return res;
}

//! @decl void create()
//! @decl void create(string host, void|string database, void|string user,@
//!		      void|string password, void|mapping(string:mixed) options)
//!
//! With no arguments, this function initialises (reinitialises if a
//! connection has been set up previously) a connection to the
//! PostgreSQL backend. Since PostgreSQL requires a database to be
//! selected, it will try to connect to the default database. The
//! connection may fail however, for a variety of reasons; in this case
//! the most likely reason is because you don't have sufficient privileges
//! to connect to that database. So use of this particular syntax is
//! discouraged.
//!
//! @param host
//! Should either contain @expr{"hostname"@} or
//! @expr{"hostname:portname"@}. This allows you to specify the TCP/IP
//! port to connect to. If the parameter is @expr{0@} or @expr{""@},
//! it will try to connect to localhost, default port.
//!
//! @param database
//! Specifies the database to connect to.  Not specifying this is
//! only supported if the PostgreSQL backend has a default database
//! configured.  If you do not want to connect to any live database,
//! you can use @expr{"template1"@}.
//!
//! @param options
//! Currently supports at least the following:
//! @mapping
//!   @member int "reconnect"
//!	Set it to zero to disable automatic reconnects upon losing
//!     the connection to the database.  Not setting it, or setting
//!     it to one, will cause one timed reconnect to take place.
//!     Setting it to -1 will cause the system to try and reconnect
//!     indefinitely.
//!   @member int "use_ssl"
//!	If the database supports and allows SSL connections, the session
//!	will be SSL encrypted, if not, the connection will fallback
//!	to plain unencrypted.
//!   @member int "force_ssl"
//!	If the database supports and allows SSL connections, the session
//!	will be SSL encrypted, if not, the connection will abort.
//!   @member int "text_query"
//!	Send queries to and retrieve results from the database using text
//!     instead of the, generally more efficient, default native binary method.
//!     Turning this on will allow multiple statements per query separated
//!     by semicolons.
//!   @member int "cache_autoprepared_statements"
//!	If set to zero, it disables the automatic statement prepare and
//!	cache logic; caching prepared statements can be problematic
//!	when stored procedures and tables are redefined which leave stale
//!	references in the already cached prepared statements.
//!   @member string "client_encoding"
//!	Character encoding for the client side, it defaults to using
//!	the default encoding specified by the database, e.g.
//!	@expr{"UTF8"@} or @expr{"SQL_ASCII"@}.
//!   @member string "standard_conforming_strings"
//!	When on, backslashes in strings must not be escaped any longer,
//!	@[quote()] automatically adjusts quoting strategy accordingly.
//!   @member string "escape_string_warning"
//!	When on, a warning is issued if a backslash (\) appears in an
//!	ordinary string literal and @expr{"standard_conforming_strings"@}
//!	is off, defaults to on.
//! @endmapping
//! For the numerous other options please check the PostgreSQL manual.
//!
//! @note
//! You need to have a database selected before using the sql-object,
//! otherwise you'll get exceptions when you try to query it. Also
//! notice that this function @b{can@} raise exceptions if the db
//! server doesn't respond, if the database doesn't exist or is not
//! accessible by you.
//!
//! @seealso
//!   @[Postgres.postgres], @[Sql.Sql], @[select_db()],
//!   @url{http://www.postgresql.org/search/?u=%2Fdocs%2Fcurrent%2F&q=client+connection+defaults@}
protected void create(void|string host, void|string database,
                      void|string user, void|string pass,
                      void|mapping(string:mixed) options) {
  this::pass = pass;
  if(pass) {
    String.secure(pass);
    pass = "CENSORED";
  }
  this::user = user;
  this::database = database;
  this::options = options || ([]);

  if(!host) host = PGSQL_DEFAULT_HOST;
  if(has_value(host,":") && sscanf(host,"%s:%d",host,_port)!=2)
    ERROR("Error in parsing the hostname argument\n");
  this::_host = host;

  if(!_port)
    _port = PGSQL_DEFAULT_PORT;
  .pgsql_util.register_backend();
  waitforauth=Thread.Mutex();
  reconnect();
}

//! @returns
//! The textual description of the last
//! server-related error. Returns @expr{0@} if no error has occurred
//! yet. It is not cleared upon reading (can be invoked multiple
//! times, will return the same result until a new error occurs).
//!
//! During the execution of a statement, this function accumulates all
//! non-error messages (notices, warnings, etc.).  If a statement does not
//! generate any errors, this function will return all collected messages
//! from the last statement.
//!
//! @note
//! The string returned is not newline-terminated.
//!
//! @param clear
//! To clear the error, set it to @expr{1@}.
//!
//! @seealso
//!   @[big_query()]
string error(void|int clear) {
  throwdelayederror(this);
  string s=lastmessage*"\n";
  if(clear)
    lastmessage=({});
  warningscollected=0;
  return sizeof(s) && s;
}

//! This function returns a string describing what host are we talking to,
//! and how (TCP/IP or UNIX sockets).
//!
//! @seealso
//!   @[server_info()]
string host_info() {
  return sprintf("fd:%d TCP/IP %s:%d PID %d",
                 c?c->socket->query_fd():-1,_host,_port,backendpid);
}

//! Returns true if the connection seems to be open.
//!
//! @note
//!   This function only checks that there's an open connection,
//!   and that the other end hasn't closed it yet. No data is
//!   sent over the connection.
//!
//!   For a more reliable check of whether the connection
//!   is alive, please use @[ping()].
//!
//! @seealso
//!   @[ping()]
int is_open() {
  catch {
    return c->socket->query_fd()>=0;
  };
  return 0;
}

//! @decl int ping()
//!
//! Check whether the connection is alive.
//!
//! @returns
//!   Returns one of the following:
//!   @int
//!     @value 0
//!       Everything ok.
//!     @value 1
//!       The connection has reconnected automatically.
//!     @value -1
//!       The server has gone away, and the connection is dead.
//!   @endint
//!
//! @seealso
//!   @[is_open()]
int ping() {
  return is_open() && !catch(c->start()->sendcmd(flushsend))
   ? !!reconnected : -1;
}

final protected object getsocket(void|int nossl) {
  return .pgsql_util.PGassist(this,qportals,(int)nossl);
}

//! Cancels all currently running queries in this session.
//!
//! @seealso
//!   @[reload()], @[resync()]
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void cancelquery() {
  PD("CancelRequest\n");
  object lcon=getsocket(1);
  lcon->add_int32(16)->add_int32(PG_PROTOCOL(1234,5678))
   ->add_int32(backendpid)->add(cancelsecret)->sendcmd(flushsend);
  lcon->close();
#ifdef PG_DEBUGMORE
  PD("Closetrace %O\n",backtrace());
#endif
  object plugbuffer=c->start(1);
  foreach(qportals->peek_array();;object portal)
    portal->_closeportal(plugbuffer);
  plugbuffer->sendcmd(sendout);
}

//! Changes the connection charset.  When set to @expr{"UTF8"@}, the query,
//! parameters and results can be Pike-native wide strings.
//!
//! @param charset
//! A PostgreSQL charset name.
//!
//! @seealso
//!   @[get_charset()], @[create()],
//!   @url{http://www.postgresql.org/search/?u=%2Fdocs%2Fcurrent%2F&q=character+sets@}
void set_charset(string charset) {
  big_query(sprintf("SET CLIENT_ENCODING TO '%s'",quote(charset)));
}

//! @returns
//! The PostgreSQL name for the current connection charset.
//!
//! @seealso
//!   @[set_charset()], @[getruntimeparameters()],
//!   @url{http://www.postgresql.org/search/?u=%2Fdocs%2Fcurrent%2F&q=character+sets@}
string get_charset() {
  return _runtimeparameter[CLIENT_ENCODING];
}

//! @returns
//! Currently active runtimeparameters for
//! the open session; these are initialised by the @tt{options@} parameter
//! during session creation, and then processed and returned by the server.
//! Common values are:
//! @mapping
//!   @member string "client_encoding"
//!	Character encoding for the client side, e.g.
//!	@expr{"UTF8"@} or @expr{"SQL_ASCII"@}.
//!   @member string "server_encoding"
//!	Character encoding for the server side as determined when the
//!	database was created, e.g. @expr{"UTF8"@} or @expr{"SQL_ASCII"@}.
//!   @member string "DateStyle"
//!	Date parsing/display, e.g. @expr{"ISO, DMY"@}.
//!   @member string "TimeZone"
//!	Default timezone used by the database, e.g. @expr{"localtime"@}.
//!   @member string "standard_conforming_strings"
//!	When on, backslashes in strings must not be escaped any longer.
//!   @member string "session_authorization"
//!	Displays the authorisationrole which the current session runs under.
//!   @member string "is_superuser"
//!	Indicates if the current authorisationrole has database-superuser
//!	privileges.
//!   @member string "integer_datetimes"
//!	Reports wether the database supports 64-bit-integer dates and times.
//!   @member string "server_version"
//!	Shows the server version, e.g. @expr{"8.3.3"@}.
//! @endmapping
//!
//! The values can be changed during a session using SET commands to the
//! database.
//! For other runtimeparameters check the PostgreSQL documentation.
//!
//! @seealso
//!   @url{http://www.postgresql.org/search/?u=%2Fdocs%2Fcurrent%2F&q=client+connection+defaults@}
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
mapping(string:string) getruntimeparameters() {
  return _runtimeparameter+([]);
}

//! @returns
//! A set of statistics for the current session:
//! @mapping
//!  @member int "warnings_dropped"
//!    Number of warnings/notices generated by the database but not
//!    collected by the application by using @[error()] after the statements
//!    that generated them.
//!  @member int "skipped_describe_count"
//!    Number of times the driver skipped asking the database to
//!    describe the statement parameters because it was already
//!    cached. Depends on the PG_STATS define being set.
//!  @member int "used_prepared_statements"
//!    Numer of times prepared statements were used from cache instead
//!    of reparsing in the current session. Depends on the PG_STATS
//!    define being set.
//!  @member int "current_prepared_statements"
//!    Cache size of currently prepared statements.
//!  @member int "current_prepared_statement_hits"
//!    Sum of the number hits on statements in the current statement cache.
//!  @member int "prepared_statement_count"
//!    Total number of prepared statements generated.
//!  @member int "portals_opened_count"
//!    Total number of portals opened, i.e. number of statements issued
//!    to the database. Depends on the PG_STATS define being set.
//!  @member int "bytes_received"
//!    Total number of bytes received from the database so far.
//!  @member int "messages_received"
//!    Total number of messages received from the database (one SQL-statement
//!    requires multiple messages to be exchanged).
//!  @member int "reconnect_count"
//!    Number of times the connection to the database has been lost.
//!  @member int "portals_in_flight"
//!    Currently still open portals, i.e. running statements.
//! @endmapping
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
mapping(string:mixed) getstatistics() {
  mapping(string:mixed) stats=([
    "warnings_dropped":warningsdropcount,
    "current_prepared_statements":sizeof(_prepareds),
    "current_prepared_statement_hits":totalhits,
    "prepared_statement_count":pstmtcount,
#ifdef PG_STATS
    "used_prepared_statements":prepstmtused,
    "skipped_describe_count":skippeddescribe,
    "portals_opened_count":portalsopened,
#endif
    "messages_received":_msgsreceived,
    "bytes_received":_bytesreceived,
    "reconnect_count":reconnected,
    "portals_in_flight":_portalsinflight,
   ]);
  return stats;
}

//! @param newdepth
//! Sets the new cachedepth for automatic caching of prepared statements.
//!
//! @returns
//! The previous cachedepth.
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int setcachedepth(void|int newdepth) {
  int olddepth=cachedepth;
  if(!undefinedp(newdepth) && newdepth>=0)
    cachedepth=newdepth;
  return olddepth;
}

//! @param newtimeout
//! Sets the new timeout for long running queries.
//!
//! @returns
//! The previous timeout.
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int settimeout(void|int newtimeout) {
  int oldtimeout=timeout;
  if(!undefinedp(newtimeout) && newtimeout>0) {
    timeout=newtimeout;
    if(c)
      c->timeout=timeout;
  }
  return oldtimeout;
}

//! @param newportalbuffersize
//!  Sets the new portalbuffersize for buffering partially concurrent queries.
//!
//! @returns
//!  The previous portalbuffersize.
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int setportalbuffersize(void|int newportalbuffersize) {
  int oldportalbuffersize=portalbuffersize;
  if(!undefinedp(newportalbuffersize) && newportalbuffersize>0)
    portalbuffersize=newportalbuffersize;
  return oldportalbuffersize;
}

//! @param newfetchlimit
//!  Sets the new fetchlimit to interleave queries.
//!
//! @returns
//!  The previous fetchlimit.
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
int setfetchlimit(void|int newfetchlimit) {
  int oldfetchlimit=_fetchlimit;
  if(!undefinedp(newfetchlimit) && newfetchlimit>=0)
    _fetchlimit=newfetchlimit;
  return oldfetchlimit;
}

final protected string glob2reg(string glob) {
  if(!glob||!sizeof(glob))
    return "%";
  return replace(glob,({"*","?","\\","%","_"}),({"%","_","\\\\","\\%","\\_"}));
}

final protected string a2nls(array(string) msg) {
  return msg*"\n"+"\n";
}

final protected string pinpointerror(void|string query,void|string offset) {
  if(!query)
    return "";
  int k=(int)offset;
  if(k<=0)
    return MARKSTART+query+MARKEND;
  return MARKSTART+(k>1?query[..k-2]:"")+MARKERROR+query[k-1..]+MARKEND;
}

protected void connect_cb() {
  PD("%O\n",_runtimeparameter);
}

protected void reconnect_cb() {
  lastmessage+=({sprintf("Reconnected to database %s",host_info())});
  runcallback(backendpid,"_reconnect","");
}

protected array(string) showbindings(object portal) {
  array(string) msgs=({});
  array from;
  if(portal && (from = portal._params)) {
    array to,paramValues;
    [from,to,paramValues] = from;
    if(sizeof(paramValues)) {
      string val;
      int i;
      string fmt=sprintf("%%%ds %%3s %%.61s",max(@map(from,sizeof)));
      foreach(paramValues;i;val)
        msgs+=({sprintf(fmt,from[i],to[i],sprintf("%O",val))});
    }
  }
  return msgs;
}

protected void preplastmessage(mapping(string:string) msgresponse) {
  lastmessage=({
    sprintf("%s %s:%s %s\n (%s:%s:%s)",
            msgresponse.S,msgresponse.C,msgresponse.P||"",
            msgresponse.M,msgresponse.F||"",msgresponse.R||"",
            msgresponse.L||"")});
}

protected void storetiming(object portal) {
  mapping(string:mixed) tp=portal._tprepared;
  tp.trun=gethrtime()-tp.trunstart;
  m_delete(tp,"trunstart");
  portal._tprepared = UNDEFINED;
}

final void _processloop(object ci) {
  int die=0,terminating=0;
  int|.pgsql_util.pgsql_result portal;
  mixed err;
  {
    object plugbuffer=Stdio.Buffer()->add_int32(PG_PROTOCOL(3,0));
    if(user)
      plugbuffer->add("user\0")->add(user)->add_int8(0);
    if(database)
      plugbuffer->add("database\0")->add(database)->add_int8(0);
    options->reconnect=undefinedp(options->reconnect) || options->reconnect;
    foreach(options
          -(<"use_ssl","force_ssl","cache_autoprepared_statements","reconnect",
               "text_query","is_superuser","server_encoding","server_version",
               "integer_datetimes","session_authorization">);
            string name;mixed value)
      plugbuffer->add(name)->add_int8(0)->add((string)value)->add_int8(0);
    plugbuffer->add_int8(0);
    PD("%O\n",(string)plugbuffer);
    ci->start()->add_hstring(plugbuffer,4,4)->sendcmd(flushsend);
  }
  cancelsecret=0;
#ifdef PG_DEBUG
  PD("Processloop\n");
  string datarowdebug;
  int datarowdebugcount;

  void showportal(int msgtype) {
    if(objectp(portal))
      PD("<%O %d %c switch portal\n",
       portal._portalname,++ci->queueinidx,msgtype);
    else if(portal>0)
      PD("<Sync %d %d %c portal\n",++ci->queueinidx,portal,msgtype);
  };
#endif
  for(;;) {
    err=catch {
#ifdef PG_DEBUG
      if(!portal && datarowdebug) {
        PD("%s rows %d\n",datarowdebug,datarowdebugcount);
        datarowdebug=0; datarowdebugcount=0;
      }
#endif
      int msgtype=ci->read_int8();
      if(!portal) {
        portal=qportals->try_read();
#ifdef PG_DEBUG
        showportal(msgtype);
#endif
      }
      int msglen=ci->read_int32();
      _msgsreceived++;
      _bytesreceived+=1+msglen;
      enum errortype {
        noerror=0,
        protocolerror,
        protocolunsupported
      };
      errortype errtype=noerror;
      switch(msgtype) {
        array(mapping) getcols() {
          int bintext=ci->read_int8();
          int cols=ci->read_int16();
#ifdef PG_DEBUG
          array a;
          msglen-=4+1+2+2*cols;
          foreach(a=allocate(cols,([]));;mapping m)
            m.type=ci->read_int16();
#else
	  ci->consume(cols<<1);
#endif			      // Discard column info, and make it line oriented
          return ({(["type":bintext?BYTEAOID:TEXTOID,"name":"line"])});
        };
        array(string) reads() {
#ifdef PG_DEBUG
          if(msglen<1)
            errtype=protocolerror;
#endif
          array ret=({}),aw=({0});
          do {
            string w=ci->read_cstring();
            msglen-=sizeof(w)+1; aw[0]=w; ret+=aw;
          } while(msglen);
          return ret;
        };
        mapping(string:string) getresponse() {
          mapping(string:string) msgresponse=([]);
          msglen-=4;
          foreach(reads();;string f)
            if(sizeof(f))
              msgresponse[f[..0]]=f[1..];
          PD("%O\n",msgresponse);
          return msgresponse;
        };
        case 'R': {
          PD("<Authentication ");
          string sendpass;
          int authtype;
          msglen-=4+4;
          switch(authtype=ci->read_int32()) {
            case 0:
              PD("Ok\n");
              .pgsql_util.local_backend->remove_call_out(reconnect);
              ci->gottimeout=cancelquery;
              ci->timeout=timeout;
              reconnectdelay=0;
              cancelsecret="";
              break;
            case 2:
              PD("KerberosV5\n");
              errtype=protocolunsupported;
              break;
            case 3:
              PD("ClearTextPassword\n");
              sendpass=pass;
              break;
            case 4:
              PD("CryptPassword\n");
              errtype=protocolunsupported;
              break;
            case 5:
              PD("MD5Password\n");
#ifdef PG_DEBUG
              if(msglen<4)
                errtype=protocolerror;
#endif
#define md5hex(x) String.string2hex(Crypto.MD5.hash(x))
              sendpass=md5hex(pass+user);
              sendpass="md5"+md5hex(sendpass+ci->read(msglen));
#ifdef PG_DEBUG
              msglen=0;
#endif
              break;
            case 6:
              PD("SCMCredential\n");
              errtype=protocolunsupported;
              break;
            case 7:
              PD("GSS\n");
              errtype=protocolunsupported;
              break;
            case 9:
              PD("SSPI\n");
              errtype=protocolunsupported;
              break;
            case 8:
              PD("GSSContinue\n");
              errtype=protocolunsupported;
              cancelsecret=ci->read(msglen);		// Actually SSauthdata
#ifdef PG_DEBUG
              if(msglen<1)
                errtype=protocolerror;
              msglen=0;
#endif
              break;
            default:
              PD("Unknown Authentication Method %c\n",authtype);
              errtype=protocolunsupported;
              break;
          }
          switch(errtype) {
            case noerror:
              if(cancelsecret!="")
                ci->start()->add_int8('p')->add_hstring(sendpass,4,5)
                 ->add_int8(0)->sendcmd(flushsend);
              break;
            default:
            case protocolunsupported:
              ERROR("Unsupported authenticationmethod %c\n",authtype);
              break;
          }
          break;
        }
        case 'K':
          msglen-=4+4;backendpid=ci->read_int32();
          cancelsecret=ci->read(msglen);
#ifdef PG_DEBUG
          PD("<BackendKeyData %O\n",cancelsecret);
          msglen=0;
#endif
          break;
        case 'S': {
          PD("<ParameterStatus ");
          msglen-=4;
          array(string) ts=reads();
#ifdef PG_DEBUG
          if(sizeof(ts)==2) {
#endif
            _runtimeparameter[ts[0]]=ts[1];
#ifdef PG_DEBUG
            PD("%O=%O\n",ts[0],ts[1]);
          } else
            errtype=protocolerror;
#endif
          break;
        }
        case '3':
#ifdef PG_DEBUG
          PD("<CloseComplete\n");
          msglen-=4;
#endif
          break;
        case 'Z':
          backendstatus=ci->read_int8();
#ifdef PG_DEBUG
          msglen-=4+1;
          PD("<ReadyForQuery %c\n",backendstatus);
#endif
          for(;objectp(portal);portal->read()) {
#ifdef PG_DEBUG
            showportal(msgtype);
#endif
            portal->_purgeportal();
          }
          foreach(qportals->peek_array();;.pgsql_util.pgsql_result qp) {
            PD("Checking portal %O %d<=%d\n",
             qp._portalname,qp._synctransact,portal);
            if(qp._synctransact && qp._synctransact<=portal)
              qp->_purgeportal();
          }
          portal=0;
          _readyforquerycount--;
          if(readyforquery_cb)
            readyforquery_cb(),readyforquery_cb=0;
          if(waitforauthready)
            destruct(waitforauthready);
          break;
        case '1':
#ifdef PG_DEBUG
          PD("<ParseComplete\n");
          msglen-=4;
#endif
          break;
        case 't': {
          array a;
          int cols=ci->read_int16();
#ifdef PG_DEBUG
          PD("<%O ParameterDescription %d values\n",portal._query,cols);
          msglen-=4+2+4*cols;
#endif
          foreach(a=allocate(cols);int i;)
            a[i]=ci->read_int32();
#ifdef PG_DEBUGMORE
          PD("%O\n",a);
#endif
          if(portal._tprepared)
            portal._tprepared.datatypeoid=a;
          portal->_preparebind();
          break;
        }
        case 'T': {
          array a;
#ifdef PG_DEBUG
          int cols=ci->read_int16();
          PD("<RowDescription %d columns %O\n",cols,portal._query);
          msglen-=4+2;
          foreach(a=allocate(cols);int i;)
#else
          foreach(a=allocate(ci->read_int16());int i;)
#endif
          {
            string s=ci->read_cstring();
            mapping(string:mixed) res=(["name":s]);
#ifdef PG_DEBUG
            msglen-=sizeof(s)+1+4+2+4+2+4+2;
            res.tableoid=ci->read_int32()||UNDEFINED;
            res.tablecolattr=ci->read_int16()||UNDEFINED;
#else
            ci->consume(6);
#endif
            res.type=ci->read_int32();
#ifdef PG_DEBUG
            {
              int len=ci->read_sint(2);
              res.length=len>=0?len:"variable";
            }
            res.atttypmod=ci->read_int32();
            /* formatcode contains just a zero when Bind has not been issued
             * yet, but the content is irrelevant because it's determined
             * at query time
             */
            res.formatcode=ci->read_int16();
#else
            ci->consume(8);
#endif
            a[i]=res;
          }
#ifdef PG_DEBUGMORE
          PD("%O\n",a);
#endif
          portal->_processrowdesc(a);
          portal=0;
          break;
        }
        case 'n': {
#ifdef PG_DEBUG
          msglen-=4;
          PD("<NoData %O\n",portal._query);
#endif
          portal._fetchlimit=0;			// disables subsequent Executes
          portal->_processrowdesc(({}));
          portal=0;
          break;
        }
        case 'H':
          portal->_processrowdesc(getcols());
          PD("<CopyOutResponse %d %O\n",
           sizeof(portal._datarowdesc),portal._query);
          break;
        case '2': {
          mapping tp;
#ifdef PG_DEBUG
          msglen-=4;
          PD("<%O BindComplete\n",portal._portalname);
#endif
          if(tp=portal._tprepared) {
            int tend=gethrtime();
            int tstart=tp.trun;
            if(tend==tstart)
              m_delete(_prepareds,portal._query);
            else {
              tp.hits++;
              totalhits++;
              if(!tp.preparedname) {
                if(sizeof(portal._preparedname))
                  tp.preparedname=portal._preparedname;
                tstart=tend-tstart;
                if(!tp.tparse || tp.tparse>tstart)
                  tp.tparse=tstart;
              }
              tp.trunstart=tend;
            }
          }
          break;
        }
        case 'D': {
          msglen-=4;
          string serror;
          if(portal._tprepared)
            storetiming(portal);
          array a, datarowdesc;
          portal._bytesreceived+=msglen;
          datarowdesc=portal._datarowdesc;
          int cols=ci->read_int16();
#ifdef PG_DEBUG
#ifdef PG_DEBUGMORE
          PD("<%O DataRow %d cols %d bytes\n",portal._portalname,cols,msglen);
#endif
          datarowdebugcount++;
          if(!datarowdebug)
            datarowdebug=sprintf(
             "<%O DataRow %d cols %d bytes",portal._portalname,cols,msglen);
#endif
          int atext = portal._alltext;		     // cache locally for speed
          int forcetext = portal._forcetext;	     // cache locally for speed
          string cenc=_runtimeparameter[CLIENT_ENCODING];
          a=allocate(cols,UNDEFINED);
          msglen-=2+4*cols;
          foreach(datarowdesc;int i;mapping m) {
            int collen=ci->read_sint(4);
            if(collen>0) {
              msglen-=collen;
              mixed value;
              switch(int typ=m.type) {
              case FLOAT4OID:
#if SIZEOF_FLOAT>=8
              case FLOAT8OID:
#endif
                if(!atext) {
                  value=(float)ci->read(collen);
                  break;
                }
              default:value=ci->read(collen);
                break;
              case CHAROID:
                value=atext?ci->read(1):ci->read_int8();
                break;
              case BOOLOID:value=ci->read_int8();
                switch(value) {
                  case 'f':value=0;
                    break;
                  case 't':value=1;
                }
                if(atext)
                  value=value?"t":"f";
                break;
              case TEXTOID:
              case BPCHAROID:
              case VARCHAROID:
                value=ci->read(collen);
                if(cenc==UTF8CHARSET && catch(value=utf8_to_string(value))
                 && !serror)
                  serror=SERROR("%O contains non-%s characters\n",
                                                         value,UTF8CHARSET);
                break;
              case INT8OID:case INT2OID:
              case OIDOID:case INT4OID:
                if(forcetext) {
                  value=ci->read(collen);
                  if(!atext)
                    value=(int)value;
                } else {
                  switch(typ) {
                    case INT8OID:value=ci->read_sint(8);
                      break;
                    case INT2OID:value=ci->read_sint(2);
                      break;
                    case OIDOID:
                    case INT4OID:value=ci->read_sint(4);
                  }
                  if(atext)
                    value=(string)value;
                }
              }
              a[i]=value;
            } else if(!collen)
              a[i]="";
          }
          portal._inflight--;
          portal._datarows->write(a);
          if(serror)
            ERROR(serror);
          portal->_processdataready();
          break;
        }
        case 's':
#ifdef PG_DEBUG
          PD("<%O PortalSuspended\n",portal._portalname);
          msglen-=4;
#endif
          portal=0;
          break;
        case 'C': {
          msglen-=4;
#ifdef PG_DEBUG
          if(msglen<1)
            errtype=protocolerror;
#endif
          string s=ci->read(msglen-1);
          if(portal._tprepared)
            storetiming(portal);
          PD("<%O CommandComplete %O\n",portal._portalname,s);
          if(!portal._statuscmdcomplete)
            portal._statuscmdcomplete=s;
#ifdef PG_DEBUG
          if(ci->read_int8())
            errtype=protocolerror;
          msglen=0;
#else
          ci->consume(1);
#endif
          portal->_releasesession();
          portal=0;
          break;
        }
        case 'I':
#ifdef PG_DEBUG
          PD("<EmptyQueryResponse %O\n",portal._portalname);
          msglen-=4;
#endif
          portal->_releasesession();
          portal=0;
          break;
        case 'd':
          PD("<%O CopyData\n",portal._portalname);
          if(portal._tprepared)
            storetiming(portal);
          msglen-=4;
#ifdef PG_DEBUG
          if(msglen<0)
            errtype=protocolerror;
#endif
          portal._bytesreceived+=msglen;
          portal._datarows->write(({ci->read(msglen)}));
#ifdef PG_DEBUG
          msglen=0;
#endif
          portal->_processdataready();
          break;
        case 'G':
          portal->_setrowdesc(getcols());
          PD("<%O CopyInResponse %d columns\n",
           portal._portalname,sizeof(portal._datarowdesc));
          portal._state=copyinprogress;
          {
            Thread.MutexKey resultlock=portal._resultmux->lock();
            portal._newresult.signal();
            resultlock=0;
          }
          break;
        case 'c':
#ifdef PG_DEBUG
          PD("<%O CopyDone\n",portal._portalname);
          msglen-=4;
#endif
          portal=0;
          break;
        case 'E': {
          if(!_readyforquerycount)
            sendsync();
          PD("<%O ErrorResponse %O\n",
           portal&&(portal._portalname||portal._preparedname),
           portal&&portal._query);
          mapping(string:string) msgresponse;
          msgresponse=getresponse();
          warningsdropcount+=warningscollected;
          warningscollected=0;
          switch(msgresponse.C) {
            case "P0001":
              lastmessage=({sprintf("%s: %s",msgresponse.S,msgresponse.M)});
              USERERROR(a2nls(lastmessage
                              +({pinpointerror(portal._query,msgresponse.P)})
                              +showbindings(portal)));
            case "57P01":case "57P02":case "57P03":die=1;
              preplastmessage(msgresponse);
              PD(a2nls(lastmessage));USERERROR(a2nls(lastmessage));
            case "08P01":case "42P05":
              errtype=protocolerror;
            case "XX000":case "42883":case "42P01":
              invalidatecache=1;
            default:
              preplastmessage(msgresponse);
              if(msgresponse.D)
                lastmessage+=({msgresponse.D});
              if(msgresponse.H)
                lastmessage+=({msgresponse.H});
              lastmessage+=({
                pinpointerror(portal&&portal._query,msgresponse.P)+
                pinpointerror(msgresponse.q,msgresponse.p)});
              if(msgresponse.W)
                lastmessage+=({msgresponse.W});
              lastmessage+=showbindings(portal);
              switch(msgresponse.S) {
                case "PANIC":werror(a2nls(lastmessage));
              }
              USERERROR(a2nls(lastmessage));
          }
          if(portal)
            portal->_releasesession();
          break;
        }
        case 'N': {
          PD("<NoticeResponse\n");
          mapping(string:string) msgresponse;
          msgresponse=getresponse();
          if(clearmessage) {
            warningsdropcount+=warningscollected;
            clearmessage=warningscollected=0;
            lastmessage=({});
          }
          warningscollected++;
          lastmessage=({sprintf("%s %s: %s",
                                  msgresponse.S,msgresponse.C,msgresponse.M)});
          break;
        }
        case 'A': {
          PD("<NotificationResponse\n");
          msglen-=4+4;
          int pid=ci->read_int32();
          string condition,extrainfo=UNDEFINED;
          {
            array(string) ts=reads();
            switch(sizeof(ts)) {
#if PG_DEBUG
              case 0:
                errtype=protocolerror;
                break;
              default:
                errtype=protocolerror;
#endif
              case 2:
                extrainfo=ts[1];
              case 1:
                condition=ts[0];
            }
          }
          PD("%d %s\n%s\n",pid,condition,extrainfo);
          runcallback(pid,condition,extrainfo);
          break;
        }
        default:
          if(msgtype!=-1) {
            string s;
            PD("Unknown message received %c\n",msgtype);
            s=ci->read(msglen-=4);PD("%O\n",s);
#ifdef PG_DEBUG
            msglen=0;
#endif
            errtype=protocolunsupported;
          } else {
            if(!waitforauthready)
              die=1;
            lastmessage+=({
             sprintf("Connection lost to database %s@%s:%d/%s %d\n",
                  user,_host,_port,database,backendpid)});
            USERERROR(a2nls(lastmessage));
          }
          break;
      }
#ifdef PG_DEBUG
      if(msglen)
        errtype=protocolerror;
#endif
      {
        string msg;
        switch(errtype) {
          case protocolunsupported:
            msg=sprintf("Unsupported servermessage received %c\n",msgtype);
            break;
          case protocolerror:
            msg=sprintf("Protocol error with database %s",host_info());
            break;
          case noerror:
            continue;				// Normal production loop
        }
        ERROR(a2nls(lastmessage+=({msg})));
      }
    };				// We only get here if there is an error
    if(err==MAGICTERMINATE) {
      ci->start()->add("X\0\0\0\4")->sendcmd(sendout);
      terminating=1;
      if(!sizeof(ci))
        break;
    }
    if(stringp(err)) {
      .pgsql_util.pgsql_result or;
      if(!(or=portal))
        or=this;
      if(!or._delayederror)
        or._delayederror=err;
      if(portal)
        portal->_releasesession();
      portal=0;
      continue;
    }
    break;
  }
  _delayederror=err;
  if(!ci->close() && !terminating && options.reconnect)
    _connectfail();
  throw(err);
}

//! Closes the connection to the database, any running queries are
//! terminated instantly.
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void close() {
  cancelquery();
  if(c)
    c->sendterminate();
  c=0;
}

void destroy() {
  close();
  .pgsql_util.unregister_backend();
}

void _connectfail(void|mixed err) {
  if(err)
    _delayederror=err;
  if(!err || reconnectdelay) {
    int tdelay;
    switch(tdelay=reconnectdelay) {
      case 0:
        reconnectdelay=RECONNECTDELAY;
        break;
      default:
        if(options.reconnect!=-1)
          return;
        reconnectdelay=RECONNECTBACKOFF;
        break;
    }
    Thread.MutexKey lock=waitforauth->lock();
    if(!waitforauthready)
      waitforauthready=Thread.Condition();
    lock=0;
    .pgsql_util.local_backend->call_out(reconnect,tdelay,1);
  }
}

protected int reconnect(void|int force,void|object tt) {
  if(!force) {
    Thread.MutexKey lock=waitforauth->lock();
    if(waitforauthready) {
      lock=0;
      return 0;			// Connect still in progress in other thread
    }
    waitforauthready=Thread.Condition();
    lock=0;
  }
  if(c) {
    reconnected++;
#ifdef PG_STATS
    prepstmtused=0;
#endif
    if(!force)
      c->sendterminate();
    else
      c->close();
    c=0;
    foreach(_prepareds;;mapping tp)
      m_delete(tp,"preparedname");
    if(!options.reconnect)
      return 0;
  }
  qportals=Thread.Queue();
  _readyforquerycount=1;
  qportals->write(1);
  if(!(c=getsocket())) {
    string msg=sprintf("Couldn't connect to database on %s:%d",_host,_port);
    if(force) {
      if(!sizeof(lastmessage) || lastmessage[sizeof(lastmessage)-1]!=msg)
        lastmessage+=({msg});
      return 0;
    } else
      ERROR(msg+"\n");
  }
  _runtimeparameter=([]);
  _unnamedportalmux=Thread.Mutex();
  unnamedstatement=Thread.Mutex();
  readyforquery_cb=force?reconnect_cb:connect_cb;
  _portalsinflight=0;
  return 1;
}

//! @decl void reload()
//!
//! For PostgreSQL this function performs the same function as @[resync()].
//!
//! @seealso
//!   @[resync()], @[cancelquery()]
void reload() {
  resync();
}

protected void resync_cb() {
  switch(backendstatus) {
    case 'T':case 'E':
      foreach(_prepareds;;mapping tp) {
        m_delete(tp,"datatypeoid");
        m_delete(tp,"datarowdesc");
      }
      big_query("ROLLBACK");
      big_query("RESET ALL");
      big_query("CLOSE ALL");
      big_query("DISCARD TEMP");
  }
}

protected void sendsync() {
  _readyforquerycount++;
  c->start()->sendcmd(syncsend);
}

//! @decl void resync()
//!
//! Resyncs the database session; typically used to make sure the session is
//! not still in a dangling transaction.
//!
//! If called while the connection is in idle state, the function is
//! lightweight and briefly touches base with the database server to
//! make sure client and server are in sync.
//!
//! If issued while inside a transaction, it will rollback the transaction,
//! close all open cursors, drop all temporary tables and reset all
//! session variables to their default values.
//!
//! @seealso
//!   @[cancelquery()], @[reload()]
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void resync(void|int|object portal) {
  mixed err;
  if(!is_open()&&!reconnect())
    ERROR(a2nls(lastmessage));
  err = catch {
    PD("Portalsinflight: %d\n",_portalsinflight);
    readyforquery_cb=resync_cb;
    sendsync();
    return;
  };
  PD("%O\n",err);
  if(!reconnect())
    ERROR(a2nls(lastmessage));
}

//! This function allows you to connect to a database. Due to
//! restrictions of the Postgres frontend-backend protocol, you always
//! have to be connected to a database, so in fact this function just
//! allows you to connect to a different database on the same server.
//!
//! @note
//! This function @b{can@} raise exceptions if something goes wrong
//! (backend process not running, insufficient privileges...)
//!
//! @seealso
//!   @[create()]
void select_db(string dbname) {
  database=dbname;
  reconnect();
  reconnected=0;
}

//! With PostgreSQL you can LISTEN to NOTIFY events.
//! This function allows you to detect and handle such events.
//!
//! @param condition
//!    Name of the notification event we're listening
//!    to.  A special case is the empty string, which matches all events,
//!    and can be used as fallback function which is called only when the
//!    specific condition is not handled.  Another special case is
//!    @expr{"_reconnect"@} which gets called whenever the connection
//!    unexpectedly drops and reconnects to the database.
//!
//! @param notify_cb
//!    Function to be called on receiving a notification-event of
//!    condition @ref{condition@}.
//!    The callback function is invoked with
//!	@expr{void notify_cb(pid,condition,extrainfo, .. args);@}
//!    @tt{pid@} is the process id of the database session that originated
//!    the event.  @tt{condition@} contains the current condition.
//!    @tt{extrainfo@} contains optional extra information specified by
//!    the database.
//!    The rest of the arguments to @ref{notify_cb@} are passed
//!    verbatim from @ref{args@}.
//!    The callback function must return no value.
//!
//! @param selfnotify
//!    Normally notify events generated by your own session are ignored.
//!    If you want to receive those as well, set @ref{selfnotify@} to one.
//!
//! @param args
//!    Extra arguments to pass to @ref{notify_cb@}.
//!
//! @note
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

final protected void runcallback(int pid,string condition,string extrainfo) {
  array cb;
  if((cb=notifylist[condition]||notifylist[""])
     && (pid!=backendpid || sizeof(cb)>1 && cb[1]))
    cb[0](pid,condition,extrainfo,@cb[2..]);
}

//! @returns
//! The given string, but escapes/quotes all contained magic characters
//! according to the quoting rules of the current session for non-binary
//! arguments in textual SQL-queries.
//!
//! @note
//! Quoting must not be done for parameters passed in bindings.
//!
//! @seealso
//!   @[big_query()], @[quotebinary()], @[create()]
string quote(string s) {
  string r=_runtimeparameter.standard_conforming_strings;
  if(r && r=="on")
    return replace(s, "'", "''");
  return replace(s, ({ "'", "\\" }), ({ "''", "\\\\" }) );
}

//! @returns
//! The given string, but escapes/quotes all contained magic characters
//! for binary (bytea) arguments in textual SQL-queries.
//!
//! @note
//! Quoting must not be done for parameters passed in bindings.
//!
//! @seealso
//!   @[big_query()], @[quote()]
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
string quotebinary(string s) {
  return replace(s, ({ "'", "\\", "\0" }), ({ "''", "\\\\", "\\000" }) );
}

//! This function creates a new database (assuming we
//! have sufficient privileges to do this).
//!
//! @param db
//! Name of the new database.
//!
//! @seealso
//!   @[drop_db()]
void create_db(string db) {
  big_query(sprintf("CREATE DATABASE %s",db));
}

//! This function destroys a database and all the data it contains (assuming
//! we have sufficient privileges to do so).  It is not possible to delete
//! the database you're currently connected to.  You can connect to database
//! @expr{"template1"@} to avoid connecting to any live database.
//!
//! @param db
//! Name of the database to be deleted.
//!
//! @seealso
//!   @[create_db()]
void drop_db(string db) {
  big_query(sprintf("DROP DATABASE %s",db));
}

//! @returns
//! A string describing the server we are
//! talking to. It has the form @expr{"servername/serverversion"@}
//! (like the HTTP protocol description) and is most useful in
//! conjunction with the generic SQL-server module.
//!
//! @seealso
//!   @[host_info()]
string server_info () {
  return DRIVERNAME"/"+(_runtimeparameter.server_version||"unknown");
}

//! @returns
//! An array of the databases available on the server.
//!
//! @param glob
//! If specified, list only those databases matching it.
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

//! @returns
//! An array containing the names of all the tables and views in the
//! path in the currently selected database.
//!
//! @param glob
//! If specified, list only the tables with matching names.
array(string) list_tables (void|string glob) {
  array row,ret=({});		 // This query might not work on PostgreSQL 7.4
  object res=big_query(		 // due to missing schemasupport
   "SELECT CASE WHEN 'public'=n.nspname THEN '' ELSE n.nspname||'.' END "
   "  ||c.relname AS name "
   "FROM pg_catalog.pg_class c "
   "  LEFT JOIN pg_catalog.pg_namespace n ON n.oid=c.relnamespace "
   "WHERE c.relkind IN ('r','v') AND n.nspname<>'pg_catalog' "
   "  AND n.nspname !~ '^pg_toast' AND pg_catalog.pg_table_is_visible(c.oid) "
   "  AND c.relname ILIKE :glob "
   "  ORDER BY 1",
   ([":glob":glob2reg(glob)]));
  while(row=res->fetch_row())
    ret+=({row[0]});
  return ret;
}

//! @returns
//! A mapping, indexed on the column name, of mappings describing
//! the attributes of a table of the current database.
//! The currently defined fields are:
//!
//! @mapping
//!   @member string "schema"
//!	Schema the table belongs to
//!   @member string "table"
//!	Name of the table
//!   @member string "kind"
//!	Type of table
//!   @member string "owner"
//!	Tableowner
//!   @member int "rowcount"
//!	Estimated rowcount of the table
//!   @member int "datasize"
//!	Estimated total datasize of the table in bytes
//!   @member int "indexsize"
//!	Estimated total indexsize of the table in bytes
//!   @member string "name"
//!	Name of the column
//!   @member string "type"
//!	A textual description of the internal (to the server) column type-name
//!   @member int "typeoid"
//!	The OID of the internal (to the server) column type
//!   @member string "length"
//!	Size of the columndatatype
//!   @member mixed "default"
//!	Default value for the column
//!   @member int "is_shared"
//!   @member int "has_index"
//!	If the table has any indices
//!   @member int "has_primarykey"
//!	If the table has a primary key
//! @endmapping
//!
//! @param glob
//! If specified, list only the tables with matching names.
//! Setting it to @expr{*@} will include system columns in the list.
array(mapping(string:mixed)) list_fields(void|string table, void|string glob) {
  array row, ret=({});
  string schema=UNDEFINED;

  sscanf(table||"*", "%s.%s", schema, table);

  object res = big_typed_query(
  "SELECT a.attname, a.atttypid, t.typname, a.attlen, "
  " c.relhasindex, c.relhaspkey, CAST(c.reltuples AS BIGINT) AS reltuples, "
  " (c.relpages "
  " +COALESCE( "
  "  (SELECT SUM(tst.relpages) "
  "    FROM pg_catalog.pg_class tst "
  "    WHERE tst.relfilenode=c.reltoastrelid) "
  "  ,0) "
  " )*8192::BIGINT AS datasize, "
  " (COALESCE( "
  "  (SELECT SUM(pin.relpages) "
  "   FROM pg_catalog.pg_index pi "
  "    JOIN pg_catalog.pg_class pin ON pin.relfilenode=pi.indexrelid "
  "   WHERE pi.indrelid IN (c.relfilenode,c.reltoastrelid)) "
  "  ,0) "
  " )*8192::BIGINT AS indexsize, "
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
  "FROM pg_catalog.pg_class c "
  "  LEFT JOIN pg_catalog.pg_namespace n ON n.oid=c.relnamespace "
  "  JOIN pg_catalog.pg_roles r ON r.oid=c.relowner "
  "  JOIN pg_catalog.pg_attribute a ON c.oid=a.attrelid "
  "  JOIN pg_catalog.pg_type t ON a.atttypid=t.oid "
  "WHERE c.relname ILIKE :table AND "
  "  (n.nspname ILIKE :schema OR "
  "   :schema IS NULL "
  "   AND n.nspname<>'pg_catalog' AND n.nspname !~ '^pg_toast') "
  "   AND a.attname ILIKE :glob "
  "   AND (a.attnum>0 OR '*'=:realglob) "
  "ORDER BY n.nspname,c.relname,a.attnum,a.attname",
  ([":schema":glob2reg(schema),":table":glob2reg(table),
    ":glob":glob2reg(glob),":realglob":glob]));

  array colnames=res->fetch_fields();
  {
    mapping(string:string) renames=([
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
     ]);
    foreach(colnames;int i;mapping m) {
      string nf,field=m.name;
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

final protected string trbackendst(int c) {
  switch(c) {
    case 'I': return "idle";
    case 'T': return "intransaction";
    case 'E': return "infailedtransaction";
  }
  return "";
}

//! @returns
//! The current commitstatus of the connection.  Returns either one of:
//! @string
//!  @value idle
//!  @value intransaction
//!  @value infailedtransaction
//! @endstring
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
final string status_commit() {
  return trbackendst(backendstatus);
}

final inline void closestatement(object plugbuffer,string oldprep) {
  .pgsql_util.closestatement(plugbuffer,oldprep);
}

protected inline string int2hex(int i) {
  return String.int2hex(i);
}

final void throwdelayederror(object parent) {
  .pgsql_util.throwdelayederror(parent);
}

//! @decl Sql.pgsql_util.pgsql_result big_query(string query)
//! @decl Sql.pgsql_util.pgsql_result big_query(string query, mapping bindings)
//!
//! This is the only provided interface which allows you to query the
//! database. If you wish to use the simpler @[Sql.Sql()->query()] function,
//! you need to use the @[Sql.Sql] generic SQL-object.
//!
//! Bindings are supported natively straight across the network.
//! Special bindings supported are:
//! @mapping
//!  @member int ":_cache"
//!   Forces caching on or off for the query at hand.
//!  @member int ":_text"
//!   Forces text mode in communication with the database for queries on or off
//!   for the query at hand.  Potentially more efficient than the default
//!   binary method for simple queries with small or no result sets.
//!   Note that this mode causes all but the first query result of a list
//!   of semicolon separated statements to be discarded.
//! @endmapping
//!
//! @returns
//! A @[Sql.pgsql_util.pgsql_result] object (which conforms to the
//! @[Sql.sql_result] standard interface for accessing data). It is
//! recommended to use @[Sql.Sql()->query()] for simpler queries (because
//! it is easier to handle, but stores all the result in memory), and
//! @[Sql.Sql()->big_query()] for queries you expect to return huge amounts of
//! data (it's harder to handle, but fetches results on demand).
//!
//! @note
//! This function @b{can@} raise exceptions.
//!
//! @note
//! This function supports multiple simultaneous queries (portals) on a single
//! database connection.  This is a feature not commonly supported by other
//! database backends.
//!
//! @note
//! This function, by default, does not support multiple queries in one
//! querystring.
//! I.e. it allows for but does not require a trailing semicolon, but it
//! simply ignores any commands after the first unquoted semicolon.  This can
//! be viewed as a limited protection against SQL-injection attacks.
//! To make it support multiple queries in one querystring, use the
//! @ref{:_text@} option.
//!
//! @seealso
//!   @[big_typed_query()], @[Sql.Sql], @[Sql.sql_result],
//!   @[Sql.Sql()->query()], @[Sql.pgsql_util.pgsql_result]
object big_query(string q,void|mapping(string|int:mixed) bindings,
                 void|int _alltyped) {
  throwdelayederror(this);
  string preparedname="";
  int forcecache=-1;
  int forcetext=options.text_query;
  string cenc=_runtimeparameter[CLIENT_ENCODING];
  switch(cenc) {
    case UTF8CHARSET:
      q=string_to_utf8(q);
      break;
    default:
      if(String.width(q)>8)
        ERROR("Don't know how to convert %O to %s encoding\n",q,cenc);
  }
  array(string|int) paramValues;
  array from;
  if(bindings) {
    int pi=0,rep=0;
    paramValues=allocate(sizeof(bindings));
    from=allocate(sizeof(bindings));
    array(string) to=allocate(sizeof(bindings));
    foreach(bindings; mixed name; mixed value) {
      if(stringp(name)) {	       // Throws if mapping key is empty string
        if(name[0]!=':')
	  name=":"+name;
	if(name[1]=='_') {	       // Special option parameter
          switch(name) {
            case ":_cache":
              forcecache=(int)value;
              break;
            case ":_text":
              forcetext=(int)value;
              break;
	  }
	  continue;
	}
	if(!has_value(q,name))
	  continue;
      }
      from[rep]=name;
      string rval;
      if(multisetp(value))		// multisets are taken literally
	rval=indices(value)*",";	// and bypass the encoding logic
      else {
        paramValues[pi++]=value;
	rval=sprintf("$%d",pi);
      }
      to[rep++]=rval;
    }
    if(rep--)
      q=replace(q,from=from[..rep],to=to[..rep]);
    paramValues= pi ? paramValues[..pi-1] : ({});
    from=({from,to,paramValues});
  } else
    paramValues = ({});
  if(String.width(q)>8)
    ERROR("Wide string literals in %O not supported\n",q);
  if(has_value(q,"\0"))
    ERROR("Querystring %O contains invalid literal nul-characters\n",q);
  mapping(string:mixed) tp;
  if(waitforauthready) {
    Thread.MutexKey lock=waitforauth->lock();
    catch(waitforauthready->wait(lock));
    lock=0;
  }
  int tstart;
  if(forcetext) {
    if(bindings)
      q = .sql_util.emulate_bindings(q, bindings, this);
  } else if(forcecache==1
        || forcecache!=0
         && (sizeof(q)>=MINPREPARELENGTH || .pgsql_util.cachealways[q])) {
    object plugbuffer=c->start();
    if(tp=_prepareds[q]) {
      if(tp.preparedname) {
#ifdef PG_STATS
	prepstmtused++;
#endif
        preparedname=tp.preparedname;
      } else if((tstart=tp.trun)
              && tp.tparse*FACTORPLAN>=tstart
              && (undefinedp(options.cache_autoprepared_statements)
             || options.cache_autoprepared_statements))
	preparedname=PREPSTMTPREFIX+int2hex(pstmtcount++);
    } else {
      if(totalhits>=cachedepth)
        foreach(_prepareds;string ind;tp) {
          int oldhits=tp.hits;
	  totalhits-=oldhits-(tp.hits=oldhits>>1);
	  if(oldhits<=1) {
            closestatement(plugbuffer,tp.preparedname);
	    m_delete(_prepareds,ind);
	  }
	}
      if(forcecache!=1 && .pgsql_util.createprefix->match(q)) {
	invalidatecache=1;				// Flush cache on CREATE
        tp=UNDEFINED;
      } else
	_prepareds[q]=tp=([]);
    }
    if(invalidatecache) {
      invalidatecache=0;
      foreach(_prepareds;;mapping np) {
        closestatement(plugbuffer,np.preparedname);
	m_delete(np,"preparedname");
      }
    }
    if(sizeof(plugbuffer)) {
      PD("%O\n",(string)plugbuffer);
      plugbuffer->sendcmd(flushsend);			      // close expireds
    } else
      plugbuffer->sendcmd();				       // close start()
    tstart=gethrtime();
  } else				  // pgsql_result autoassigns to portal
    tp=UNDEFINED;
  object portal;
  portal=.pgsql_util.pgsql_result(this,c,q,
                                    portalbuffersize,_alltyped,from,forcetext);
  portal._tprepared=tp;
#ifdef PG_STATS
  portalsopened++;
#endif
  clearmessage=1;
  object plugbuffer=c;
  if(forcetext) {	// FIXME What happens if portals are still open?
    portal._unnamedportalkey=_unnamedportalmux->lock(1);
    portal->_openportal();
    _readyforquerycount++;
    Thread.MutexKey lock=unnamedstatement->lock(1);
    plugbuffer->start(1)->add_int8('Q')->add_hstring(q,4,4+1)->add_int8(0)
     ->sendcmd(flushlogsend,portal);
    lock=0;
    PD("Simple query: %O\n",q);
  } else {
    object parsebuffer;
    if(!sizeof(preparedname) || !tp || !tp.preparedname) {
      if(!sizeof(preparedname))
        preparedname=
          (portal._unnamedstatementkey=unnamedstatement->trylock(1))
           ? "" : PTSTMTPREFIX+int2hex(ptstmtcount++);
      // Even though the protocol doesn't require the Parse command to be
      // followed by a flush, it makes a VERY noticeable difference in
      // performance if it is omitted; seems like a flaw in the PostgreSQL
      // server v8.3.3
      PD("Parse statement %O=%O\n",preparedname,q);
      parsebuffer=plugbuffer->start()->add_int8('P')
       ->add_hstring(({preparedname,0,q,"\0\0\0"}),4,4)->add(PGFLUSH);
    }
    if(!tp || !tp.datatypeoid) {
      PD("Describe statement %O\n",preparedname);
      (parsebuffer||plugbuffer->start())->add_int8('D')
       ->add_hstring(({'S',preparedname,0}),4,4)->sendcmd(flushsend,portal);
    } else {
      if(parsebuffer)
        parsebuffer->sendcmd();
#ifdef PG_STATS
      skippeddescribe++;
#endif
      portal->_setrowdesc(tp.datarowdesc);
    }
    portal._preparedname=preparedname;
    if((portal._tprepared=tp) && tp.datatypeoid) {
      mixed e=catch(portal->_preparebind());
      if(e && !portal._delayederror) {
        if(!stringp(e))
          throw(e);
        portal._delayederror=e;
      }
    }
  }
  throwdelayederror(portal);
  return portal;
}

//! This is an alias for @[big_query()], since @[big_query()] already supports
//! streaming of multiple simultaneous queries through the same connection.
//!
//! @seealso
//!   @[big_query()], @[big_typed_query()], @[Sql.Sql], @[Sql.sql_result]
object streaming_query(string q,void|mapping(string|int:mixed) bindings) {
  return big_query(q,bindings);
}

//! This function returns an object that allows streaming and typed
//! results.
//!
//! @seealso
//!   @[big_query()], @[Sql.Sql], @[Sql.sql_result]
object big_typed_query(string q,void|mapping(string|int:mixed) bindings) {
  return big_query(q,bindings,1);
}
