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
private array(string) lastmessage=({});
private int clearmessage;
private int earlyclose;
private mapping(string:array(mixed)) notifylist=([]);
mapping(string:string) _runtimeparameter;
state _mstate;
private enum querystate {queryidle,inquery,cancelpending,canceled};
private querystate qstate;
private mapping(string:mapping(string:mixed)) prepareds=([]);
private mapping(string:mixed) tprepared;
private int pstmtcount;
private int pportalcount;
private int totalhits;
private int cachedepth=STATEMENTCACHEDEPTH;
private int timeout=QUERYTIMEOUT;
private int portalbuffersize=PORTALBUFFERSIZE;
private int reconnected;     // Number of times the connection was reset
private int sessionblocked;  // Number of times the session blocked on network
private int skippeddescribe; // Number of times we skipped Describe phase
private int portalsopened;   // Number of portals opened
int _msgsreceived;	     // Number of protocol messages received
int _bytesreceived;	     // Number of bytes received
int _packetssent;	     // Number of packets sent
int _bytessent;		     // Number of bytes sent
private int warningsdropcount; // Number of uncollected warnings
private int prepstmtused;    // Number of times prepared statements were used
private int warningscollected;
private int invalidatecache;

private string host, database, user, pass;
private int port;
private object createprefix
 =Regexp("^[ \t\f\r\n]*[Cc][Rr][Ee][Aa][Tt][Ee][ \t\f\r\n]");
private object dontcacheprefix
 =Regexp("^[ \t\f\r\n]*([Ff][Ee][Tt][Cc][Hh]|[Cc][Oo][Pp][Yy])[ \t\f\r\n]");
private object limitpostfix
 =Regexp("[ \t\f\r\n][Ll][Ii][Mm][Ii][Tt][ \t\f\r\n]+[12][; \t\f\r\n]*$");
Thread.Mutex _querymutex;
Thread.Mutex _stealmutex;

#define USERERROR(msg)	throw(({(msg), backtrace()[..<1]}))

protected string _sprintf(int type, void|mapping flags)
{ string res=UNDEFINED;
  switch(type)
  { case 'O':
      res=sprintf(DRIVERNAME"(%s@%s:%d/%s,%d)",
       user,host,port,database,backendpid);
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
#define FLOAT8OID	701
#define MACADDROID	829
#define INETOID		869	    /* Force textmode */
#define BPCHAROID	1042
#define VARCHAROID	1043
#define CTIDOID		1247
#define UUIDOID		2950

#define UTF8CHARSET	"UTF8"
#define CLIENT_ENCODING	"client_encoding"

#define PG_PROTOCOL(m,n)   (((m)<<16)|(n))

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
//! you can use @ref{template1@}.
//!
//! @param options
//! Currently supports at least the following:
//! @mapping
//!   @member int use_ssl
//!	If the database supports and allows SSL connections, the session
//!	will be SSL encrypted, if not, the connection will fallback
//!	to plain unencrypted
//!   @member int force_ssl
//!	If the database supports and allows SSL connections, the session
//!	will be SSL encrypted, if not, the connection will abort
//!   @member int cache_autoprepared_statements
//!	If set to zero, it disables the automatic statement prepare and
//!	cache logic; caching prepared statements can be problematic
//!	when stored procedures and tables are redefined which leave stale
//!	references in the already cached prepared statements
//!   @member string client_encoding
//!	Character encoding for the client side, it defaults to using
//!	the default encoding specified by the database, e.g.
//!	@ref{UTF8@} or @ref{SQL_ASCII@}.
//!   @member string standard_conforming_strings
//!	When on, backslashes in strings must not be escaped any longer,
//!	@[quote()] automatically adjusts quoting strategy accordingly
//!   @member string escape_string_warning
//!	When on, a warning is issued if a backslash (\) appears in an
//!	ordinary string literal and @[standard_conforming_strings] is off,
//!	defaults to on
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
//!   @url{http://search.postgresql.org/search?u=%2Fdocs%2F&q=client+connection+defaults@}
protected void create(void|string _host, void|string _database,
 void|string _user, void|string _pass, void|mapping(string:mixed) _options)
{ pass = _pass; _pass = "CENSORED";
  if(pass)
    String.secure(pass);
  user = _user; database = _database; host = _host || PGSQL_DEFAULT_HOST;
  options = _options || ([]);
  if(has_value(host,":") && sscanf(_host,"%s:%d",host,port)!=2)
    ERROR("Error in parsing the hostname argument\n");
  if(!port)
    port = PGSQL_DEFAULT_PORT;
  _querymutex=Thread.Mutex();
  _stealmutex=Thread.Mutex();
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
string error(void|int clear)
{ string s=lastmessage*"\n";
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
string host_info()
{ return sprintf("fd:%d TCP/IP %s:%d PID %d",
   _c?_c.query_fd():-1,host,port,backendpid);
}

final private object getsocket(void|int nossl)
{ object lcon = Stdio.File();
  if(!lcon.connect(host,port))
    return UNDEFINED;

  object fcon;
#if constant(SSL.sslfile)
  if(!nossl && (options->use_ssl || options->force_ssl))
  { PD("SSLRequest\n");
    { object c=.pgsql_util.PGassist();
      lcon.write(({c.plugint32(8),c.plugint32(PG_PROTOCOL(1234,5679))}));
    }
    switch(lcon.read(1))
    { case "S":
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
//! @seealso
//!   @[reload()], @[resync()]
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void cancelquery()
{ if(qstate==inquery)
  { qstate=cancelpending;
    object lcon;
    PD("CancelRequest\n");
    if(!(lcon=getsocket(1)))
      ERROR("Cancel connect failed\n");
    lcon.write(({_c.plugint32(16),_c.plugint32(PG_PROTOCOL(1234,5678)),
     _c.plugint32(backendpid),cancelsecret}));
    lcon.close();
  }
}

//! Changes the connection charset.  When set to @ref{UTF8@}, the query,
//! parameters and results can be Pike-native wide strings.
//!
//! @param charset
//! A PostgreSQL charset name.
//!
//! @seealso
//!   @[get_charset()], @[create()],
//!   @url{http://search.postgresql.org/search?u=%2Fdocs%2F&q=character+sets@}
void set_charset(string charset)
{ big_query(sprintf("SET CLIENT_ENCODING TO '%s'",quote(charset)));
}

//! @returns
//! The PostgreSQL name for the current connection charset.
//!
//! @seealso
//!   @[set_charset()], @[getruntimeparameters()],
//!   @url{http://search.postgresql.org/search?u=%2Fdocs%2F&q=character+sets@}
string get_charset()
{ return _runtimeparameter[CLIENT_ENCODING];
}

//! @returns
//! Currently active runtimeparameters for
//! the open session; these are initialised by the @ref{options@} parameter
//! during session creation, and then processed and returned by the server.
//! Common values are:
//! @mapping
//!   @member string client_encoding
//!	Character encoding for the client side, e.g.
//!	@ref{UTF8@} or @ref{SQL_ASCII@}.
//!   @member string server_encoding
//!	Character encoding for the server side as determined when the
//!	database was created, e.g. @ref{UTF8@} or @ref{SQL_ASCII@}
//!   @member string DateStyle
//!	Date parsing/display, e.g. @ref{ISO, DMY@}
//!   @member string TimeZone
//!	Default timezone used by the database, e.g. @ref{localtime@}
//!   @member string standard_conforming_strings
//!	When on, backslashes in strings must not be escaped any longer
//!   @member string session_authorization
//!	Displays the authorisationrole which the current session runs under
//!   @member string is_superuser
//!	Indicates if the current authorisationrole has database-superuser
//!	privileges
//!   @member string integer_datetimes
//!	Reports wether the database supports 64-bit-integer dates and times
//!   @member string server_version
//!	Shows the server version, e.g. @ref{8.3.3@}
//! @endmapping
//!
//! The values can be changed during a session using SET commands to the
//! database.
//! For other runtimeparameters check the PostgreSQL documentation.
//!
//! @seealso
//!   @url{http://search.postgresql.org/search?u=%2Fdocs%2F&q=client+connection+defaults@}
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
mapping(string:string) getruntimeparameters()
{ return _runtimeparameter+([]);
}

//! @returns
//! A set of statistics for the current session:
//! @mapping
//!  @member int warnings_dropped
//!    Number of warnings/notices generated by the database but not
//!    collected by the application by using @[error()] after the statements
//!    that generated them.
//!  @member int skipped_describe_count
//!    Number of times the driver skipped asking the database to
//!    describe the statement parameters because it was already cached.
//!  @member int used_prepared_statements
//!    Numer of times prepared statements were used from cache instead of
//!    reparsing in the current session.
//!  @member int current_prepared_statements
//!    Cache size of currently prepared statements.
//!  @member int current_prepared_statement_hits
//!    Sum of the number hits on statements in the current statement cache.
//!  @member int prepared_portal_count
//!    Total number of prepared portals generated.
//!  @member int prepared_statement_count
//!    Total number of prepared statements generated.
//!  @member int portals_opened_count
//!    Total number of portals opened, i.e. number of statements issued
//!    to the database.
//!  @member int blocked_count
//!    Number of times the driver had to (briefly) wait for the database to
//!    send additional data.
//!  @member int bytes_received
//!    Total number of bytes received from the database so far.
//!  @member int messages_received
//!    Total number of messages received from the database (one SQL-statement
//!    requires multiple messages to be exchanged).
//!  @member int bytes_sent
//!    Total number of bytes sent to the database so far.
//!  @member int packets_sent
//!    Total number of packets sent to the database (one packet usually
//!    contains multiple messages).
//!  @member int reconnect_count
//!    Number of times the connection to the database has been lost.
//!  @member int portals_in_flight
//!    Currently still open portals, i.e. running statements.
//! @endmapping
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
mapping(string:mixed) getstatistics()
{ mapping(string:mixed) stats=([
    "warnings_dropped":warningsdropcount,
    "skipped_describe_count":skippeddescribe,
    "used_prepared_statements":prepstmtused,
    "current_prepared_statements":sizeof(prepareds),
    "current_prepared_statement_hits":totalhits,
    "prepared_portal_count":pportalcount,
    "prepared_statement_count":pstmtcount,
    "portals_opened_count":portalsopened,
    "blocked_count":sessionblocked,
    "messages_received":_msgsreceived,
    "bytes_received":_bytesreceived,
    "packets_sent":_packetssent,
    "bytes_sent":_bytessent,
    "reconnect_count":reconnected,
    "portals_in_flight":portalsinflight,
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
int setcachedepth(void|int newdepth)
{ int olddepth=cachedepth;
  if(!zero_type(newdepth) && newdepth>=0)
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
int settimeout(void|int newtimeout)
{ int oldtimeout=timeout;
  if(!zero_type(newtimeout) && newtimeout>0)
    timeout=newtimeout;
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
int setportalbuffersize(void|int newportalbuffersize)
{ int oldportalbuffersize=portalbuffersize;
  if(!zero_type(newportalbuffersize) && newportalbuffersize>0)
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
int setfetchlimit(void|int newfetchlimit)
{ int oldfetchlimit=_fetchlimit;
  if(!zero_type(newfetchlimit) && newfetchlimit>=0)
    _fetchlimit=newfetchlimit;
  return oldfetchlimit;
}

final private string glob2reg(string glob)
{ if(!glob||!sizeof(glob))
    return "%";
  return replace(glob,({"*","?","\\","%","_"}),({"%","_","\\\\","\\%","\\_"}));
}

final private string a2nls(array(string) msg)
{ return msg*"\n"+"\n";
}

final private string pinpointerror(void|string query,void|string offset)
{ if(!query)
    return "";
  int k=(int)offset;
  if(k<=0)
    return MARKSTART+query+MARKEND;
  return MARKSTART+(k>1?query[..k-2]:"")+MARKERROR+query[k-1..]+MARKEND;
}

final int _decodemsg(void|state waitforstate)
{
#ifdef DEBUG
  { array line;
#ifdef DEBUGMORE
    line=backtrace();
#endif
    PD("Waiting for state %O %O\n",waitforstate,line&&line[sizeof(line)-2]);
  }
#endif
  while(_mstate!=waitforstate)
  { if(_mstate!=unauthenticated)
    { if(qstate==cancelpending)
	qstate=canceled,sendclose();
      if(_c.flushed && qstate==inquery && !_c.bpeek(0))
      { int tcurr=time();
	int told=tcurr+timeout;
	sessionblocked++;
	while(!_c.bpeek(told-tcurr))
	  if((tcurr=time())-told>=timeout)
	  { sendclose();cancelquery();
	    break;
	  }
      }
    }
    int msgtype=_c.getbyte();
    int msglen=_c.getint32();
    _msgsreceived++;
    _bytesreceived+=1+msglen;
    enum errortype { noerror=0, protocolerror, protocolunsupported };
    errortype errtype=noerror;
    switch(msgtype)
    { void storetiming()
      { tprepared->trun=gethrtime()-tprepared->trunstart;
	m_delete(tprepared,"trunstart");
	tprepared = UNDEFINED;
      };
      void getcols()
      { int bintext=_c.getbyte();
	array a;
	int cols=_c.getint16();
	msglen-=4+1+2+2*cols;
	foreach(a=allocate(cols,([]));;mapping m)
	  m->type=_c.getint16();
	if(_c.portal)	      // Discard column info, and make it line oriented
	{ a=({(["type":bintext?BYTEAOID:TEXTOID,"name":"line"])});
	  _c.portal->_datarowdesc=a;
	}
	_mstate=gotrowdescription;
      };
      array(string) getstrings()
      { string s;
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
      mapping(string:string) getresponse()
      { mapping(string:string) msgresponse=([]);
	msglen-=4;
	foreach(getstrings();;string f)
	  if(sizeof(f))
	    msgresponse[f[..0]]=f[1..];
	PD("%O\n",msgresponse);
	return msgresponse;
      };
      array(string) showbindings()
      { array(string) msgs=({});
        array from;
	if(from = _c.portal->_params)
	{ array to,paramValues;
	  [from,to,paramValues] = from;
          if(sizeof(paramValues))
          { string val;
            int i;
            string fmt=sprintf("%%%ds %%3s %%.61s",max(@map(from,sizeof)));
            foreach(paramValues;i;val)
               msgs+=({sprintf(fmt,from[i],to[i],sprintf("%O",val))});
          }
        }
	return msgs;
      };
      case 'R':PD("Authentication\n");
      { string sendpass;
	int authtype;
	msglen-=4+4;
	switch(authtype=_c.getint32())
	{ case 0:PD("Ok\n");
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
	    sendpass=_c.getstring(msglen);msglen=0;			// salt
	    errtype=protocolunsupported; // Pike lacks function that takes salt
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
	switch(errtype)
	{ case noerror:
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
	if(sizeof(ts)==2)
	{ _runtimeparameter[ts[0]]=ts[1];
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
	foreach(a=allocate(_c.getint16());int i;)
	{ string s;
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
	_c.portal->_fetchlimit=0;		// disables subsequent Executes
	_mstate=gotrowdescription;
	break;
      case '2':PD("BindComplete\n");
	msglen-=4;
	_mstate=bindcomplete;
	break;
      case 'D':PD("DataRow\n");
	msglen-=4;
	if(_c.portal)
	{ if(tprepared)
	    storetiming();
#ifdef USEPGsql
	  _c.decodedatarow(msglen);msglen=0;
#else
	  array a, datarowdesc;
	  _c.portal->_bytesreceived+=msglen;
	  datarowdesc=_c.portal->_datarowdesc;
	  int cols=_c.getint16();
	  int atext = _c.portal->_alltext;	  // cache locally for speed
	  string cenc=_runtimeparameter[CLIENT_ENCODING];
	  a=allocate(cols,UNDEFINED);
	  msglen-=2+4*cols;
	  foreach(a;int i;)
	  { int collen=_c.getint32();
	    if(collen>0)
	    { msglen-=collen;
	      mixed value;
	      switch(datarowdesc[i]->type)
	      { default:value=_c.getstring(collen);
		  break;
		case TEXTOID:
		case BPCHAROID:
		case VARCHAROID:
		  value=_c.getstring(collen);
		  if(cenc==UTF8CHARSET)
		    value=utf8_to_string(value);
		  break;
		case CHAROID:value=atext?_c.getstring(1):_c.getbyte();
		  break;
		case BOOLOID:value=_c.getbyte();
		  if(atext)
		    value=value?"t":"f";
		  break;
		case INT8OID:value=_c.getint64();
		  break;
#if SIZEOF_FLOAT>=8
		case FLOAT8OID:
#endif
		case FLOAT4OID:
		  value=_c.getstring(collen);
		  if(!atext)
		    value=(float)value;
		  break;
		case INT2OID:value=_c.getint16();
		  break;
		case OIDOID:
		case INT4OID:value=_c.getint32();
	      }
	      if(atext&&!stringp(value))
		value=(string)value;
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
	{ if(tprepared)
	    storetiming();
	  _c.portal->_statuscmdcomplete=s;
	}
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
	if(tprepared)
	  storetiming();
	msglen-=4;
	if(msglen<0)
	  errtype=protocolerror;
	if(_c.portal)
	{ _c.portal->_bytesreceived+=msglen;
	  _c.portal->_datarows+=({({_c.getstring(msglen)})});
	}
	msglen=0;
	_mstate=dataready;
	break;
      case 'H':PD("CopyOutResponse\n");
	getcols();
	break;
      case 'G':PD("CopyInResponse\n");
	getcols();
	_mstate=copyinresponse;
	break;
      case 'c':PD("CopyDone\n");
	msglen-=4;
	break;
      case 'E':PD("ErrorResponse\n");
      { mapping(string:string) msgresponse;
	msgresponse=getresponse();
	warningsdropcount+=warningscollected;
	warningscollected=0;
	switch(msgresponse->C)
	{ case "P0001":
	    lastmessage=({sprintf("%s: %s",msgresponse->S,msgresponse->M)});
	    USERERROR(a2nls(lastmessage
	     +({pinpointerror(_c.portal->_query,msgresponse->P)})
	     +showbindings()));
	  case "08P01":case "42P05":
	    errtype=protocolerror;
	  case "XX000":case "42883":case "42P01":
	    invalidatecache=1;
	  default:
	    lastmessage=({sprintf("%s %s:%s %s\n (%s:%s:%s)",
	     msgresponse->S,msgresponse->C,msgresponse->P||"",msgresponse->M,
	     msgresponse->F||"",msgresponse->R||"",msgresponse->L||"")});
	    if(msgresponse->D)
	      lastmessage+=({msgresponse->D});
	    if(msgresponse->H)
	      lastmessage+=({msgresponse->H});
	    lastmessage+=({
	     pinpointerror(_c.portal&&_c.portal->_query,msgresponse->P)+
	     pinpointerror(msgresponse->q,msgresponse->p)});
	    if(msgresponse->W)
	      lastmessage+=({msgresponse->W});
	    lastmessage+=showbindings();
	    switch(msgresponse->S)
	    { case "PANIC":werror(a2nls(lastmessage));
	    }
	    USERERROR(a2nls(lastmessage));
	}
	break;
      }
      case 'N':PD("NoticeResponse\n");
      { mapping(string:string) msgresponse;
	msgresponse=getresponse();
	if(clearmessage)
	{ warningsdropcount+=warningscollected;
	  clearmessage=warningscollected=0;
	  lastmessage=({});
	}
	warningscollected++;
	lastmessage=({sprintf("%s %s: %s",
	 msgresponse->S,msgresponse->C,msgresponse->M)});
	break;
      }
      case 'A':PD("NotificationResponse\n");
      { msglen-=4+4;
	int pid=_c.getint32();
	string condition,extrainfo=UNDEFINED;
	{ array(string) ts=getstrings();
	  switch(sizeof(ts))
	  { case 0:errtype=protocolerror;
	      break;
	    default:errtype=protocolerror;
	    case 2:extrainfo=ts[1];
	    case 1:condition=ts[0];
	  }
	}
	PD("%d %s\n%s\n",pid,condition,extrainfo);
	runcallback(pid,condition,extrainfo);
	break;
      }
      default:
	if(msgtype!=-1)
	{ PD("Unknown message received %c\n",msgtype);
	  msglen-=4;PD("%O\n",_c.getstring(msglen));msglen=0;
	  errtype=protocolunsupported;
	}
	else
	{ array(string) msg=lastmessage;
	  if(!reconnect(1))
	  { sleep(RECONNECTDELAY);
	    if(!reconnect(1))
	    { sleep(RECONNECTBACKOFF);
	      reconnect(1);
	    }
	  }
	  msg+=lastmessage;
	  string s=sizeof(msg)?a2nls(msg):"";
	  ERROR("%sConnection lost to database %s@%s:%d/%s %d\n",
	   s,user,host,port,database,backendpid);
	}
	break;
    }
    if(msglen)
      errtype=protocolerror;
    switch(errtype)
    { case protocolunsupported:
	ERROR("Unsupported servermessage received %c\n",msgtype);
	break;
      case protocolerror:
	array(string) msg=lastmessage;
	lastmessage=({});
	reconnect(1);
	msg+=lastmessage;
	string s=sizeof(msg)?a2nls(msg):"";
	ERROR("%sProtocol error with database %s\n",s,host_info());
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
private int read_cb(mixed foo, string d)
{ _c.unread(d);
  do _decodemsg();
  while(_c.bpeek(0)==1);
  return 0;
}
#endif

//! Closes the connection to the database, any running queries are
//! terminated instantly.
//!
//! @note
//! This function is PostgreSQL-specific, and thus it is not available
//! through the generic SQL-interface.
void close()
{ cancelquery();
  if(_c)
    _c.sendterminate();
}

void destroy()
{ close();
}

private int reconnect(void|int force)
{ Thread.MutexKey connectmtxkey;
  if(_c)
  { reconnected++;
    prepstmtused=0;
#ifdef DEBUG
    ERROR("While debugging, reconnects are forbidden\n");
    exit(1);
#endif
    if(!force)
      _c.sendterminate();
    _c.close(); _c=0;
    foreach(prepareds;;mapping tp)
      m_delete(tp,"preparedname");
    if(!(connectmtxkey = _stealmutex.trylock(2)))
      return 0;				    // Recursive reconnect, bailing out
  }
  if(!(_c=getsocket()))
  { string msg=sprintf("Couldn't connect to database on %s:%d",host,port);
    if(force)
    { lastmessage+=({msg});
      return 0;
    }
    else
      ERROR(msg+"\n");
  }
  _closesent=0;
  _mstate=unauthenticated;
  qstate=queryidle;
  _runtimeparameter=([]);
  array(string) plugbuf=({"",_c.plugint32(PG_PROTOCOL(3,0))});
  if(user)
    plugbuf+=({"user\0",user,"\0"});
  if(database)
    plugbuf+=({"database\0",database,"\0"});
  foreach(options-(<"use_ssl","force_ssl","cache_autoprepared_statements">);
   string name;mixed value)
    plugbuf+=({name,"\0",(string)value,"\0"});
  plugbuf+=({"\0"});
  int len=4;
  foreach(plugbuf;;string s)
    len+=sizeof(s);
  plugbuf[0]=_c.plugint32(len);
  _c.write(plugbuf);
  PD("%O\n",plugbuf);
  { mixed err=catch(_decodemsg(readyforquery));
    if(err)
      if(force)
	throw(err);
      else
	return 0;
  }
  PD("%O\n",_runtimeparameter);
  if(force)
  { lastmessage+=({sprintf("Reconnected to database %s",host_info())});
    runcallback(backendpid,"_reconnect","");
  }
  return 1;
}

//! @decl void reload()
//!
//! For PostgreSQL this function performs the same function as @[resync()].
//!
//! @seealso
//!   @[resync()], @[cancelquery()]
void reload()
{ resync();
}

//! @decl void resync()
//!
//! Resyncs the database session; typically used to make sure the session is
//! not still in a dangling transaction.
//!
//! If called while queries/portals are still in-flight, this function
//! is a no-op.
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
void resync(void|int special)
{ mixed err;
  int didsync;
  if(err = catch
    { sendclose(1);
      PD("Portalsinflight: %d\n",portalsinflight);
      if(!portalsinflight)
      { if(!earlyclose)
	{ PD("Sync\n");
	  _c.sendcmd(({"S",_c.plugint32(4)}),2);
	}
	didsync=1;
	if(!special)
	{ _decodemsg(readyforquery);
	  switch(backendstatus)
	  { case 'T':case 'E':
	      foreach(prepareds;;mapping tp)
	      { m_delete(tp,"datatypeoid");
		m_delete(tp,"datarowdesc");
	      }
	      big_query("ROLLBACK");
	      big_query("RESET ALL");
	      big_query("CLOSE ALL");
	      big_query("DISCARD TEMP");
	  }
	}
      }
      earlyclose=0;
    })
  { earlyclose=0;
    PD("%O\n",err);
    if(!reconnect(1))
      ERROR(a2nls(lastmessage));
  }
  else if(didsync && special==2)
    _decodemsg(readyforquery);
#ifndef UNBUFFEREDIO
  _c.set_read_callback(read_cb);
#endif
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
void select_db(string dbname)
{ database=dbname;
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
//!    @ref{_reconnect@} which gets called whenever the connection unexpectedly
//!    drops and reconnects to the database.
//!
//! @param notify_cb
//!    Function to be called on receiving a notification-event of
//!    condition @ref{condition@}.
//!    The callback function is invoked with
//!	@expr{void notify_cb(pid,condition,extrainfo, .. args);@}
//!    @ref{pid@} is the process id of the database session that originated
//!    the event.  @ref{condition@} contains the current condition.
//!    @ref{extrainfo@} contains optional extra information specified by
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
  mixed ... args)
{ if(!notify_cb)
    m_delete(notifylist,condition);
  else
  { array old=notifylist[condition];
    if(!old)
      old=({notify_cb});
    if(selfnotify||args)
      old+=({selfnotify});
    if(args)
      old+=args;
    notifylist[condition]=old;
  }
}

final private void runcallback(int pid,string condition,string extrainfo)
{ array cb;
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
string quote(string s)
{ string r=_runtimeparameter->standard_conforming_strings;
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
string quotebinary(string s)
{ return replace(s, ({ "'", "\\", "\0" }), ({ "''", "\\\\", "\\000" }) );
}

//! This function creates a new database (assuming we
//! have sufficient privileges to do this).
//!
//! @param db
//! Name of the new database.
//!
//! @seealso
//!   @[drop_db()]
void create_db(string db)
{ big_query("CREATE DATABASE :db",([":db":db]));
}

//! This function destroys a database and all the data it contains (assuming
//! we have sufficient privileges to do so).  It is not possible to delete
//! the database you're currently connected to.  You can connect to database
//! @ref{template1@} to avoid connecting to any live database.
//!
//! @param db
//! Name of the database to be deleted.
//!
//! @seealso
//!   @[create_db()]
void drop_db(string db)
{ big_query("DROP DATABASE :db",([":db":db]));
}

//! @returns
//! A string describing the server we are
//! talking to. It has the form @expr{"servername/serverversion"@}
//! (like the HTTP protocol description) and is most useful in
//! conjunction with the generic SQL-server module.
//!
//! @seealso
//!   @[host_info()]
string server_info ()
{ return DRIVERNAME"/"+(_runtimeparameter->server_version||"unknown");
}

//! @returns
//! An array of the databases available on the server.
//!
//! @param glob
//! If specified, list only those databases matching it.
array(string) list_dbs (void|string glob)
{ array row,ret=({});
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
array(string) list_tables (void|string glob)
{ array row,ret=({});		 // This query might not work on PostgreSQL 7.4
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
//!   @member string schema
//!	Schema the table belongs to
//!   @member string table
//!	Name of the table
//!   @member string kind
//!	Type of table
//!   @member string owner
//!	Tableowner
//!   @member int rowcount
//!	Estimated rowcount of the table
//!   @member int datasize
//!	Estimated total datasize of the table in bytes
//!   @member int indexsize
//!	Estimated total indexsize of the table in bytes
//!   @member string name
//!	Name of the column
//!   @member string type
//!	A textual description of the internal (to the server) column type-name
//!   @member int typeoid
//!	The OID of the internal (to the server) column type
//!   @member string length
//!	Size of the columndatatype
//!   @member mixed default
//!	Default value for the column
//!   @member int is_shared
//!   @member int has_index
//!	If the table has any indices
//!   @member int has_primarykey
//!	If the table has a primary key
//! @endmapping
//!
//! @param glob
//! If specified, list only the tables with matching names.
//! Setting it to @expr{*@} will include system columns in the list.
array(mapping(string:mixed)) list_fields(void|string table, void|string glob)
{ array row, ret=({});
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
     ]);
    foreach(colnames;int i;mapping m)
    { string nf,field=m->name;
      if(nf=renames[field])
	field=nf;
      colnames[i]=field;
    }
  }

#define delifzero(m,field) if(!(m)[field]) m_delete(m,field)

  while(row=res->fetch_row())
  { mapping m=mkmapping(colnames,row);
    delifzero(m,"is_shared");
    delifzero(m,"has_index");
    delifzero(m,"has_primarykey");
    delifzero(m,"default");
    ret+=({m});
  }
  return ret;
}

private int oidformat(int oid)
{ switch(oid)
  { case BOOLOID:
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

final void _sendexecute(int fetchlimit)
{ string portalname=_c.portal->_portalname;
  PD("Execute portal %s fetchlimit %d\n",portalname,fetchlimit);
  _c.sendcmd(({"E",_c.plugint32(4+sizeof(portalname)+1+4),portalname,
   "\0",_c.plugint32(fetchlimit)}),!!fetchlimit);
  if(!fetchlimit)
  { _c.portal->_fetchlimit=0;			   // disables further Executes
    earlyclose=1;
    if(sizeof(portalname))
    { PD("Close portal %s & Sync\n",portalname);
      _c.sendcmd(({"C",_c.plugint32(4+1+sizeof(portalname)+1),
       "P",portalname,"\0"}));
    }
    _c.sendcmd(({"S",_c.plugint32(4)}),2);
  }
  else
    _c.portal->_inflight+=fetchlimit;
}

final private void sendclose(void|int hold)
{ string portalname;
  if(_c.portal && (portalname=_c.portal->_portalname))
  { _c.portal->_portalname = UNDEFINED;
    _c.setportal();
    portalsinflight--;
#ifdef DEBUGMORE
    PD("Closetrace %O\n",backtrace());
#endif
    if(!sizeof(portalname))
      unnamedportalinuse--;
    if(sizeof(portalname))
    { if(!earlyclose)
      { PD("Close portal %s\n",portalname);
	_c.sendcmd(({"C",_c.plugint32(4+1+sizeof(portalname)+1),
	 "P",portalname,"\0"}),!hold||portalsinflight?1:0);
      }
      _closesent=1;
    }
  }
}

final private string trbackendst(int c)
{ switch(c)
  { case 'I':return "idle";
    case 'T':return "intransaction";
    case 'E':return "infailedtransaction";
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
final string status_commit()
{ return trbackendst(backendstatus);
}

final private array(string) closestatement(mapping tp)
{ string oldprep=tp->preparedname;
  array(string) ret=({});
  if(oldprep)
  { PD("Close statement %s\n",oldprep);
    ret=({"C",_c.plugint32(4+1+sizeof(oldprep)+1),
     "S",oldprep,"\0"});
  }
  return ret;
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
//!  @member int :_cache
//!   Forces caching on or off for the query at hand.
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
//! This function does not support multiple queries in one querystring.
//! I.e. it allows for but does not require a trailing semicolon, but it
//! simply ignores any commands after the first unquoted semicolon.  This can
//! be viewed as a limited protection against SQL-injection attacks.
//!
//! @seealso
//!   @[big_typed_query()], @[Sql.Sql], @[Sql.sql_result],
//!   @[Sql.Sql()->query()], @[Sql.pgsql_util.pgsql_result]
object big_query(string q,void|mapping(string|int:mixed) bindings,
 void|int _alltyped)
{ string preparedname="";
  string portalname="";
  int forcecache=-1;
  string cenc=_runtimeparameter[CLIENT_ENCODING];
  switch(cenc)
  { case UTF8CHARSET:
      q=string_to_utf8(q);
      break;
    default:
      if(String.width(q)>8)
	ERROR("Don't know how to convert %O to %s encoding\n",q,cenc);
  }
  array(string|int) paramValues;
  array from;
  if(bindings)
  { int pi=0,rep=0;
    paramValues=allocate(sizeof(bindings));
    from=allocate(sizeof(bindings));
    array(string) to=allocate(sizeof(bindings));
    foreach(bindings; mixed name; mixed value)
    { if(stringp(name))		       // Throws if mapping key is empty string
      { if(name[0]!=':')
	  name=":"+name;
	if(name[1]=='_')	       // Special option parameter
	{ switch(name)
	  { case ":_cache":forcecache=(int)value;
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
      else
      { paramValues[pi++]=value;
	rval=sprintf("$%d",pi);
      }
      to[rep++]=rval;
    }
    if(rep--)
      q=replace(q,from=from[..rep],to=to[..rep]);
    paramValues= pi ? paramValues[..pi-1] : ({});
    from=({from,to,paramValues});
  }
  else
    paramValues = ({});
  if(String.width(q)>8)
    ERROR("Wide string literals in %O not supported\n",q);
  if(has_value(q,"\0"))
    ERROR("Querystring %O contains invalid literal nul-characters\n",q);
  mapping(string:mixed) tp;
  int tstart;
  if(forcecache==1 || forcecache!=0 && sizeof(q)>=MINPREPARELENGTH)
  { array(string) plugbuf=({});
    if(tp=prepareds[q])
    { if(tp->preparedname)
	prepstmtused++, preparedname=tp->preparedname;
      else if((tstart=tp->trun)
       && tp->tparse*FACTORPLAN>=tstart
       && (zero_type(options->cache_autoprepared_statements)
	|| options->cache_autoprepared_statements))
	preparedname=PREPSTMTPREFIX+(string)pstmtcount++;
    }
    else
    { if(totalhits>=cachedepth)
      { foreach(prepareds;string ind;tp)
	{ int oldhits=tp->hits;
	  totalhits-=oldhits-(tp->hits=oldhits>>1);
	  if(oldhits<=1)
	  { plugbuf+=closestatement(tp);
	    m_delete(prepareds,ind);
	  }
	}
      }
      if(forcecache!=1 && createprefix->match(q))      // Flush cache on CREATE
	invalidatecache=1;
      else
	prepareds[q]=tp=([]);
    }
    if(invalidatecache)
    { invalidatecache=0;
      foreach(prepareds;;mapping np)
      { plugbuf+=closestatement(np);
	m_delete(np,"preparedname");
      }
    }
    if(sizeof(plugbuf))
    { _c.sendcmd(plugbuf,1);				      // close expireds
      PD("%O\n",plugbuf);
    }
    tstart=gethrtime();
  }					  // pgsql_result autoassigns to portal
  else
    tp=UNDEFINED;
  .pgsql_util.pgsql_result(this,q,_fetchlimit,portalbuffersize,_alltyped,from);
  if(unnamedportalinuse)
    portalname=PORTALPREFIX+(string)pportalcount++;
  else
    unnamedportalinuse++;
  _c.portal->_portalname=portalname;
  qstate=inquery;
  portalsinflight++; portalsopened++;
  clearmessage=1;
  mixed err;
  if(err = catch
    { if(!sizeof(preparedname) || !tp || !tp->preparedname)
      { PD("Parse statement %s\n",preparedname);
	// Even though the protocol doesn't require the Parse command to be
	// followed by a flush, it makes a VERY noticeable difference in
	// performance if it is omitted; seems like a flaw in the PostgreSQL
	// server v8.3.3
	_c.sendcmd(({"P",_c.plugint32(4+sizeof(preparedname)+1+sizeof(q)+1+2),
	 preparedname,"\0",q,"\0",_c.plugint16(0)}),3);
	PD("Query: %O\n",q);
      }			 // sends Parameter- and RowDescription for 'S'
      if(!tp || !tp->datatypeoid)
      { PD("Describe statement %s\n",preparedname);
	_c.sendcmd(({"D",_c.plugint32(4+1+sizeof(preparedname)+1),
	 "S",preparedname,"\0"}),1);
      }
      else
      { skippeddescribe++;
	_c.portal->_datatypeoid=tp->datatypeoid;
	_c.portal->_datarowdesc=tp->datarowdesc;
      }
      { array(string) plugbuf=({"B",UNDEFINED});
	int len=4+sizeof(portalname)+1+sizeof(preparedname)+1
	 +2+sizeof(paramValues)*(2+4)+2+2;
	plugbuf+=({portalname,"\0",preparedname,"\0",
	 _c.plugint16(sizeof(paramValues))});
	if(!tp || !tp->datatypeoid)
	{ _decodemsg(gotparameterdescription);
	  if(tp)
	    tp->datatypeoid=_c.portal->_datatypeoid;
	}
	array dtoid=_c.portal->_datatypeoid;
	if(sizeof(dtoid)!=sizeof(paramValues))
	  USERERROR(
	   sprintf("Invalid number of bindings, expected %d, got %d\n",
	    sizeof(dtoid),sizeof(paramValues)));
	foreach(dtoid;;int textbin)
	  plugbuf+=({_c.plugint16(oidformat(textbin))});
	plugbuf+=({_c.plugint16(sizeof(paramValues))});
	foreach(paramValues;int i;mixed value)
	{ if(zero_type(value))
	    plugbuf+=({_c.plugint32(-1)});				// NULL
	  else if(stringp(value) && !sizeof(value))
	  { int k=0;
	    switch(dtoid[i])
	    { default:
		k=-1;	     // cast empty strings to NULL for non-string types
	      case BYTEAOID:
	      case TEXTOID:
	      case XMLOID:
	      case BPCHAROID:
	      case VARCHAROID:;
	    }
	    plugbuf+=({_c.plugint32(k)});
	  }
	  else
	    switch(dtoid[i])
	    { case TEXTOID:
	      case BPCHAROID:
	      case VARCHAROID:
	      { value=(string)value;
		switch(cenc)
		{ case UTF8CHARSET:
		    value=string_to_utf8(value);
		    break;
		  default:
		    if(String.width(value)>8)
		      ERROR("Don't know how to convert %O to %s encoding\n",
		       value,cenc);
		}
		int k;
		len+=k=sizeof(value);
		plugbuf+=({_c.plugint32(k),value});
		break;
	      }
	      default:
	      { int k;
		value=(string)value;
		if(String.width(value)>8)
		  ERROR("Wide string %O not supported for type OID %d\n",
		   value,dtoid[i]);
		len+=k=sizeof(value);
		plugbuf+=({_c.plugint32(k),value});
		break;
	      }
	      case BOOLOID:plugbuf+=({_c.plugint32(1)});len++;
		do
		{ int tval;
		  if(stringp(value))
		    tval=value[0];
		  else if(!intp(value))
		  { value=!!value;			// cast to boolean
		    break;
		  }
		  else
		     tval=value;
		  switch(tval)
		  { case 'o':case 'O':
		      catch
		      { tval=value[1];
			value=tval=='n'||tval=='N';
		      };
		      break;
		    default:
		      value=1;
		      break;
		    case 0:case 'f':case 'F':case 'n':case 'N':
		      value=0;
		      break;
		  }
		}
		while(0);
		plugbuf+=({_c.plugbyte(value)});
		break;
	      case CHAROID:
		if(intp(value))
		  len++,plugbuf+=({_c.plugint32(1),_c.plugbyte(value)});
		else
		{ value=(string)value;
		  switch(sizeof(value))
		  { default:
		      ERROR("\"char\" types must be 1 byte wide, got %O\n",
		       value);
		    case 0:
		      plugbuf+=({_c.plugint32(-1)});		// NULL
		      break;
		    case 1:len++;
		      plugbuf+=({_c.plugint32(1),_c.plugbyte(value[0])});
		  }
		}
		break;
	      case INT8OID:len+=8;
		plugbuf+=({_c.plugint32(8),_c.plugint64((int)value)});
		break;
	      case OIDOID:
	      case INT4OID:len+=4;
		plugbuf+=({_c.plugint32(4),_c.plugint32((int)value)});
		break;
	      case INT2OID:len+=2;
		plugbuf+=({_c.plugint32(2),_c.plugint16((int)value)});
		break;
	    }
	}
	if(!tp || !tp->datarowdesc)
	{ if(tp && dontcacheprefix->match(q))	      // Don't cache FETCH/COPY
	    m_delete(prepareds,q),tp=0;
	  _decodemsg(gotrowdescription);
	  if(tp)
	    tp->datarowdesc=_c.portal->_datarowdesc;
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
      _sendexecute(_fetchlimit
       && !limitpostfix->match(q)		    // Optimisation for LIMIT 1
       && FETCHLIMITLONGRUN);
      if(tp)
      { _decodemsg(bindcomplete);
	int tend=gethrtime();
	if(tend==tstart)
	  m_delete(prepareds,q);
	else
	{ tp->hits++;
	  totalhits++;
	  if(!tp->preparedname)
	  { if(sizeof(preparedname))
	      tp->preparedname=preparedname;
	    tstart=tend-tstart;
	    if(!tp->tparse || tp->tparse>tstart)
	      tp->tparse=tstart;
	  }
	  tp->trunstart=tend;
	}
	tprepared=tp;
      }
    })
  { PD("%O\n",err);
    resync(1);
    backendstatus=UNDEFINED;
    throw(err);
  }
  { object tportal=_c.portal;		// Make copy, because it might dislodge
    tportal->fetch_row(1);		// upon initial fetch_row()
    return tportal;
  }
}

//! This is an alias for @[big_query()], since @[big_query()] already supports
//! streaming of multiple simultaneous queries through the same connection.
//!
//! @seealso
//!   @[big_query()], @[big_typed_query()], @[Sql.Sql], @[Sql.sql_result]
object streaming_query(string q,void|mapping(string|int:mixed) bindings)
{ return big_query(q,bindings);
}

//! This function returns an object that allows streaming and typed
//! results.
//!
//! @seealso
//!   @[big_query()], @[Sql.Sql], @[Sql.sql_result]
object big_typed_query(string q,void|mapping(string|int:mixed) bindings)
{ return big_query(q,bindings,1);
}
