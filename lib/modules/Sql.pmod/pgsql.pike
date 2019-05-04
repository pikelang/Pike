/*
 * This is the PostgreSQL direct network module for Pike.
 */

//! This is an interface to the PostgreSQL database
//! server. This module is independent of any external libraries.
//! Note that you @b{do not@} need to have a
//! PostgreSQL server running on @b{your@} host to use this module: you can
//! connect to the database over a TCP/IP socket on a different host.
//!
//! This module replaces the functionality of the older @[Sql.postgres]
//! and @[Postgres.postgres] modules.
//!
//! This module supports the following features:
//! @ul
//! @item
//!  PostgreSQL network protocol version 3, authentication methods
//!   currently supported are: cleartext, md5 and scram (recommended).
//! @item
//!  Optional asynchronous query interface through callbacks.
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
//!   Multiple simultaneous queries on the same database connection are a
//!   feature that none of the other database drivers for Pike support.
//!   So, although it's efficient, its use will make switching database drivers
//!   difficult.
//!
//! @seealso
//!  @[Sql.Connection], @[Sql.postgres],
//!  @url{https://www.postgresql.org/docs/current/static/@}

#pike __REAL_VERSION__
#pragma dynamic_dot
#require constant(Thread.Thread)

#include "pgsql.h"

#define ERROR(X ...)	     predef::error(X)

inherit __builtin.Sql.Connection;

private .pgsql_util.proxy proxy;

private int pstmtcount;
private int ptstmtcount;	// Periodically one would like to reset these
				// but checking when this is safe to do
				// probably is more costly than the gain
#ifdef PG_STATS
private int skippeddescribe;	// Number of times we skipped Describe phase
private int portalsopened;	// Number of portals opened
private int prepstmtused;	// Number of times we used prepared statements
#endif
private int cachedepth = STATEMENTCACHEDEPTH;
private int portalbuffersize = PORTALBUFFERSIZE;
private int timeout = QUERYTIMEOUT;
private array connparmcache;
private int reconnected;
private int lastping = time(1);

protected string _sprintf(int type) {
  string res;
  switch(type) {
    case 'O':
      res = sprintf(DRIVERNAME"(%s@%s:%d/%s,%d,%d)",
       proxy.user, proxy.host, proxy.port, proxy.database,
       proxy.c?->socket && proxy.c->socket->query_fd(), proxy.backendpid);
      break;
  }
  return res;
}

//! With no arguments, this function initialises a connection to the
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
//!    Set it to zero to disable automatic reconnects upon losing
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
//!     by semicolons (not recommended).
//!   @member int "sync_parse"
//!     Set it to zero to turn synchronous parsing off for statements.
//!     Setting this to off can cause surprises because statements could
//!     be parsed before the previous statements have been executed
//!     (e.g. references to temporary tables created in the preceding
//!     statement),
//!     but it can speed up parsing due to increased parallelism.
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
//! You need to have a database selected before using the SQL-object,
//! otherwise you'll get exceptions when you try to query it. Also
//! notice that this function @b{can@} raise exceptions if the db
//! server doesn't respond, if the database doesn't exist or is not
//! accessible to you.
//!
//! @note
//! It is possible that the exception from a failed connect
//! will not be triggered on this call (because the connect
//! proceeds asynchronously in the background), but on the first
//! attempt to actually use the database instead.
//!
//! @seealso
//!   @[Postgres.postgres], @[Sql.Connection], @[select_db()],
//!   @url{https://www.postgresql.org/docs/current/static/runtime-config-client.html@}
protected void create(void|string host, void|string database,
                      void|string user, void|string pass,
                      void|mapping(string:mixed) options) {
  string spass = pass && pass != "" ? Standards.IDNA.to_ascii(pass) : pass;
  if(pass) {
    String.secure(pass);
    pass = "CENSORED";
  }
  connparmcache = ({ host, database,
   user && user != "" ? Standards.IDNA.to_ascii(user, 1) : user,
   spass, options || ([])});
  proxy = .pgsql_util.proxy(@connparmcache);
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
//! since the last statement.
//!
//! @note
//! The string returned is not newline-terminated.
//!
//! @param clear
//! To clear the error, set it to @expr{1@}.
//!
//! @seealso
//!   @[big_query()]
/*semi*/final string error(void|int clear) {
  throwdelayederror(proxy);
  return proxy.geterror(clear);
}

//! This function returns a string describing what host are we talking to,
//! and how (TCP/IP or UNIX socket).
//!
//! @seealso
//!   @[server_info()]
/*semi*/final string host_info() {
  return proxy.host_info();
}

//! Returns true if the connection seems to be open.
//!
//! @note
//!   This function only checks that there's an open connection,
//!   and that the other end hasn't closed it yet. No data is
//!   sent over the connection.
//!
//!   For a more reliable check of whether the connection
//!   is alive, please use @[ping()] instead.
//!
//! @seealso
//!   @[ping()]
/*semi*/final int is_open() {
  return proxy.is_open();
}

//! Check whether the connection is alive.
//!
//! @returns
//!   Returns one of the following:
//!   @int
//!     @value 0
//!       Everything ok.
//!     @value -1
//!       The server has gone away, and the connection is dead.
//!   @endint
//!
//! @seealso
//!   @[is_open()]
/*semi*/final int ping() {
  int t, ret;
  waitauthready();
  if ((ret = is_open())
     // Pinging more frequently than MINPINGINTERVAL seconds
     // is suppressed to avoid artificial TCP-ACK latency
   && (t = time(1)) - lastping > MINPINGINTERVAL
   && (ret = !catch(proxy.c->start()->sendcmd(FLUSHSEND))))
    lastping = t;
  return ret ? !!reconnected : -1;
}

//! Cancels all currently running queries in this session.
//!
//! @seealso
//!   @[reload()], @[resync()]
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final void cancelquery() {
  proxy.cancelquery();
}

//! Changes the connection charset.  When set to @expr{"UTF8"@}, the query,
//! parameters and results can be Pike-native wide strings.
//!
//! @param charset
//! A PostgreSQL charset name.
//!
//! @seealso
//!   @[get_charset()], @[create()],
//!   @url{https://www.postgresql.org/docs/current/static/multibyte.html@}
/*semi*/final void set_charset(string charset) {
  if(charset)
    big_query(sprintf("SET CLIENT_ENCODING TO '%s'", quote(charset)));
}

//! @returns
//! The PostgreSQL name for the current connection charset.
//!
//! @seealso
//!   @[set_charset()], @[getruntimeparameters()],
//!   @url{https://www.postgresql.org/docs/current/static/multibyte.html@}
/*semi*/final string get_charset() {
  return getruntimeparameters()[CLIENT_ENCODING];
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
//!   @url{https://www.postgresql.org/docs/current/static/runtime-config-client.html@}
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final mapping(string:string) getruntimeparameters() {
  waitauthready();
  return proxy.runtimeparameter + ([]);
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
//!    describe the statement parameters because it was already cached.
//!    Only available if PG_STATS is compile-time enabled.
//!  @member int "used_prepared_statements"
//!    Number of times prepared statements were used from cache instead of
//!    reparsing in the current session.
//!    Only available if PG_STATS is compile-time enabled.
//!  @member int "current_prepared_statements"
//!    Cache size of currently prepared statements.
//!  @member int "current_prepared_statement_hits"
//!    Sum of the number hits on statements in the current statement cache.
//!  @member int "prepared_statement_count"
//!    Total number of prepared statements generated.
//!  @member int "portals_opened_count"
//!    Total number of portals opened, i.e. number of statements issued
//!    to the database.
//!    Only available if PG_STATS is compile-time enabled.
//!  @member int "bytes_received"
//!    Total number of bytes received from the database so far.
//!  @member int "messages_received"
//!    Total number of messages received from the database (one SQL-statement
//!    requires multiple messages to be exchanged).
//!  @member int "portals_in_flight"
//!    Currently still open portals, i.e. running statements.
//! @endmapping
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final mapping(string:mixed) getstatistics() {
  mapping(string:mixed) stats = ([
    "warnings_dropped":proxy.warningsdropcount,
    "current_prepared_statements":sizeof(proxy.prepareds),
    "current_prepared_statement_hits":proxy.totalhits,
    "prepared_statement_count":pstmtcount,
#ifdef PG_STATS
    "used_prepared_statements":prepstmtused,
    "skipped_describe_count":skippeddescribe,
    "portals_opened_count":portalsopened,
#endif
    "messages_received":proxy.msgsreceived,
    "bytes_received":proxy.bytesreceived,
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
//! This function is PostgreSQL-specific.
/*semi*/final int setcachedepth(void|int newdepth) {
  int olddepth = cachedepth;
  if (!undefinedp(newdepth) && newdepth >= 0)
    cachedepth = newdepth;
  return olddepth;
}

//! @param newtimeout
//! Sets the new timeout for long running queries.
//!
//! @returns
//! The previous timeout.
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final int settimeout(void|int newtimeout) {
  int oldtimeout = timeout;
  if (!undefinedp(newtimeout) && newtimeout > 0)
    timeout = newtimeout;
  return oldtimeout;
}

//! @param newportalbuffersize
//!  Sets the new portalbuffersize for buffering partially concurrent queries.
//!
//! @returns
//!  The previous portalbuffersize.
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final int setportalbuffersize(void|int newportalbuffersize) {
  int oldportalbuffersize = portalbuffersize;
  if (!undefinedp(newportalbuffersize) && newportalbuffersize>0)
    portalbuffersize = newportalbuffersize;
  return oldportalbuffersize;
}

//! @param newfetchlimit
//!  Sets the new fetchlimit to interleave queries.
//!
//! @returns
//!  The previous fetchlimit.
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final int setfetchlimit(void|int newfetchlimit) {
  int oldfetchlimit = proxy._fetchlimit;
  if (!undefinedp(newfetchlimit) && newfetchlimit >= 0)
    proxy._fetchlimit = newfetchlimit;
  return oldfetchlimit;
}

private string glob2reg(string glob) {
  if (!glob || !sizeof(glob))
    return "%";
  return replace(glob, ({"*", "?",   "\\",   "%",   "_"}),
                       ({"%", "_", "\\\\", "\\%", "\\_"}));
}

private void waitauthready() {
  if (proxy.waitforauthready) {
    PD("%d Wait for auth ready %O\n",
     proxy.c?->socket && proxy.c->socket->query_fd(), backtrace()[-2]);
    Thread.MutexKey lock = proxy.shortmux->lock();
    catch(PT(proxy.waitforauthready->wait(lock)));
    PD("%d Wait for auth ready released.\n",
     proxy.c?->socket && proxy.c->socket->query_fd());
  }
}

//! Closes the connection to the database, any running queries are
//! terminated instantly.
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final void close() {
  proxy.close();
}

protected void _destruct() {
  destruct(proxy);
}

//! For PostgreSQL this function performs the same function as @[resync()].
//!
//! @seealso
//!   @[resync()], @[cancelquery()]
/*semi*/final void reload() {
  resync();
}

private void reset_dbsession() {
  proxy.statementsinflight->wait_till_drained();
  proxy.delayederror = 0;
  error(1);
  big_query("ROLLBACK");
  big_query("RESET ALL");
  big_query("CLOSE ALL");
  big_query("DISCARD TEMP");
}

private void resync_cb() {
  switch (proxy.backendstatus) {
    case 'T':case 'E':
      foreach (proxy.prepareds; ; mapping tp) {
        m_delete(tp,"datatypeoid");
        m_delete(tp,"datarowdesc");
        m_delete(tp,"datarowtypes");
      }
      Thread.Thread(reset_dbsession);	  // Urgently and deadlockfree
  }
}

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
//! @note
//! This function @b{can@} raise exceptions.
//!
//! @seealso
//!   @[cancelquery()], @[reload()]
//!
//! @note
//! This function is PostgreSQL-specific.
/*semi*/final void resync() {
  mixed err;
  if (is_open()) {
    err = catch {
      PD("Statementsinflight: %d  Portalsinflight: %d\n",
       proxy.statementsinflight, proxy.portalsinflight);
      if(!proxy.waitforauthready) {
        proxy.readyforquery_cb = resync_cb;
        proxy.sendsync();
      }
      return;
    };
    PD("%O\n", err);
  }
  if (sizeof(proxy.lastmessage))
    ERROR(proxy.a2nls(proxy.lastmessage));
}

//! Due to restrictions of the Postgres frontend-backend protocol, you always
//! already have to be connected to a database.
//! To connect to a different database you have to select the right
//! database while connecting instead.  This function is a no-op when
//! specifying the same database, and throws an error otherwise.
//!
//! @seealso
//!   @[create()]
/*semi*/final void select_db(string dbname) {
  if (proxy.database != dbname)
    ERROR("Cannot switch databases from %O to %O"
      " in an already established connection\n",
     proxy.database, dbname);
}

//! With PostgreSQL you can LISTEN to NOTIFY events.
//! This function allows you to detect and handle such events.
//!
//! @param condition
//!    Name of the notification event we're listening
//!    to.  A special case is the empty string, which matches all events,
//!    and can be used as fallback function which is called only when the
//!    specific condition is not handled.  Another special case is
//!    @expr{"_lost"@} which gets called whenever the connection
//!    to the database unexpectedly drops.
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
//! This function is PostgreSQL-specific.
/*semi*/final void set_notify_callback(string condition,
 void|function(int,string,string,mixed ...:void) notify_cb, void|int selfnotify,
  mixed ... args) {
  if (!notify_cb)
    m_delete(proxy.notifylist, condition);
  else {
    array old = proxy.notifylist[condition];
    if (!old)
      old = ({notify_cb});
    if (selfnotify || args)
      old += ({selfnotify});
    if (args)
      old += args;
    proxy.notifylist[condition] = old;
  }
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
/*semi*/final string quote(string s) {
  waitauthready();
  string r = proxy.runtimeparameter.standard_conforming_strings;
  if (r && r == "on")
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
//! This function is PostgreSQL-specific.
/*semi*/final string quotebinary(string s) {
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
/*semi*/final void create_db(string db) {
  big_query(sprintf("CREATE DATABASE %s", db));
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
/*semi*/final void drop_db(string db) {
  big_query(sprintf("DROP DATABASE %s", db));
}

//! @returns
//! A string describing the server we are
//! talking to. It has the form @expr{"servername/serverversion"@}
//! (like the HTTP protocol description) and is most useful in
//! conjunction with the generic SQL-server module.
//!
//! @seealso
//!   @[host_info()]
/*semi*/final string server_info () {
  waitauthready();
  return DRIVERNAME"/" + (proxy.runtimeparameter.server_version || "unknown");
}

//! @returns
//! An array of the databases available on the server.
//!
//! @param glob
//! If specified, list only those databases matching it.
/*semi*/final array(string) list_dbs (void|string glob) {
  array row, ret = .pgsql_util.emptyarray;
  .pgsql_util.Result res=big_query("SELECT d.datname "
                                         "FROM pg_database d "
                                         "WHERE d.datname ILIKE :glob "
                                         "ORDER BY d.datname",
                                         ([":glob":glob2reg(glob)]));
  while(row = res->fetch_row())
    ret += ({row[0]});
  return ret;
}

//! @returns
//! An array containing the names of all the tables and views in the
//! path in the currently selected database.
//!
//! @param glob
//! If specified, list only the tables with matching names.
/*semi*/final array(string) list_tables (void|string glob) {
  array row, ret = .pgsql_util.emptyarray;
  .pgsql_util.Result res = big_query(     // due to missing schemasupport
   // This query might not work on PostgreSQL 7.4
   "SELECT CASE WHEN 'public'=n.nspname THEN '' ELSE n.nspname||'.' END "
   "  ||c.relname AS name "
   "FROM pg_catalog.pg_class c "
   "  LEFT JOIN pg_catalog.pg_namespace n ON n.oid=c.relnamespace "
   "WHERE c.relkind IN ('r','v') AND n.nspname<>'pg_catalog' "
   "  AND n.nspname !~ '^pg_toast' AND pg_catalog.pg_table_is_visible(c.oid) "
   "  AND c.relname ILIKE :glob "
   "  ORDER BY 1",
   ([":glob":glob2reg(glob)]));
  while(row = res->fetch_row())
    ret += ({row[0]});
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
/*semi*/final array(mapping(string:mixed)) list_fields(void|string table,
 void|string glob) {
  array row, ret = .pgsql_util.emptyarray;
  string schema;

  sscanf(table||"*", "%s.%s", schema, table);

  .pgsql_util.Result res = big_typed_query(
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
    mapping(string:string) renames = ([
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
    foreach(colnames; int i; mapping m) {
      string nf, field=m.name;
      if(nf=renames[field])
        field=nf;
      colnames[i]=field;
    }
  }

#define delifzero(m, field) if(!(m)[field]) m_delete(m, field)

  while(row=res->fetch_row()) {
    mapping m=mkmapping(colnames, row);
    delifzero(m,"is_shared");
    delifzero(m,"has_index");
    delifzero(m,"has_primarykey");
    delifzero(m,"default");
    ret += ({m});
  }
  return ret;
}

private string trbackendst(int c) {
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
//! This function is PostgreSQL-specific.
final string status_commit() {
  return trbackendst(proxy.backendstatus);
}

private inline void closestatement(
  .pgsql_util.bufcon|.pgsql_util.conxsess plugbuffer, string oldprep) {
  .pgsql_util.closestatement(plugbuffer, oldprep);
}

private inline string int2hex(int i) {
  return String.int2hex(i);
}

private inline void throwdelayederror(object parent) {
  .pgsql_util.throwdelayederror(parent);
}

private void startquery(int forcetext, .pgsql_util.Result portal, string q,
 mapping(string:mixed) tp, string preparedname) {
  .pgsql_util.conxion c = proxy.c;
  if (!c && (proxy.options["reconnect"]
             || zero_type(proxy.options["reconnect"]))) {
    sleep(BACKOFFDELAY);	// Force a backoff delay
    if (!proxy.c) {
      reconnected++;
      proxy = .pgsql_util.proxy(@connparmcache);
    }
    c = proxy.c;
  }
  if (forcetext) {	// FIXME What happens if portals are still open?
    portal._unnamedportalkey = proxy.unnamedportalmux->lock(1);
    portal._portalname = "";
    portal->_parseportal(); portal->_bindportal();
    proxy.readyforquerycount++;
    {
      Thread.MutexKey lock = proxy.unnamedstatement->lock(1);
      .pgsql_util.conxsess cs = c->start(1);
      CHAIN(cs)->add_int8('Q')->add_hstring(({q, 0}), 4, 4);
      cs->sendcmd(FLUSHLOGSEND, portal);
    }
    PD("Simple query: %O\n", q);
  } else {
    object plugbuffer;
    portal->_parseportal();
    if (!sizeof(preparedname) || !tp || !tp.preparedname) {
      if (!sizeof(preparedname))
        preparedname =
          (portal._unnamedstatementkey = proxy.unnamedstatement->trylock(1))
           ? "" : PTSTMTPREFIX + int2hex(ptstmtcount++);
      PD("Parse statement %O=%O\n", preparedname, q);
      plugbuffer = c->start();
      CHAIN(plugbuffer)->add_int8('P')
       ->add_hstring(({preparedname, 0, q, "\0\0\0"}), 4, 4)
#if 0
      // Even though the protocol doesn't require the Parse command to be
      // followed by a flush, it makes a VERY noticeable difference in
      // performance if it is omitted; seems like a flaw in the PostgreSQL
      // server v8.3.3
      // In v8.4 and later, things speed up slightly when it is omitted.
      ->add(PGFLUSH)
#endif
      ;
    } else {				// Use the name from the cache
      preparedname = tp.preparedname;	// to shortcut a potential race
      PD("Using prepared statement %s for %O\n", preparedname, q);
    }
    portal._preparedname = preparedname;
    if (!tp || !tp.datatypeoid) {
      PD("Describe statement %O\n", preparedname);
      if (!plugbuffer)
        plugbuffer = c->start();
      CHAIN(plugbuffer)->add_int8('D')
       ->add_hstring(({'S', preparedname, 0}), 4, 4);
      plugbuffer->sendcmd(FLUSHSEND, portal);
    } else {
      if (plugbuffer)
        plugbuffer->sendcmd(KEEP);
#ifdef PG_STATS
      skippeddescribe++;
#endif
      portal->_setrowdesc(tp.datarowdesc, tp.datarowtypes);
    }
    if ((portal._tprepared=tp) && tp.datatypeoid) {
      mixed e = catch(portal->_preparebind(tp.datatypeoid));
      if (e && !portal.delayederror) {
        portal._unnamedstatementkey = 0;	// Release early, release often
        throw(e);
      }
    }
    if (!proxy.unnamedstatement)
      portal._unnamedstatementkey = 0;		// Cover for a destruct race
  }
}

//! This is the only provided direct interface which allows you to query the
//! database.  A simpler synchronous interface can be used through @[query()].
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
//!  @member int ":_sync"
//!   Forces synchronous parsing on or off for statements.
//!   Setting this to off can cause surprises because statements could
//!   be parsed before the previous statements have been executed
//!   (e.g. references to temporary tables created in the preceding
//!   statement),
//!   but it can speed up parsing due to increased parallelism.
//! @endmapping
//!
//! @note
//!  The parameters referenced via the @expr{bindings@}-parameter-mapping
//!  passed to this function must remain unaltered
//!  until the parameters have been sent to the database.  The driver
//!  currently does not expose this moment, but to avoid a race condition
//!  it is sufficient to keep them unaltered until the first resultrow
//!  has been fetched (or EOF is reached, in case of no resultrows).
//!
//! @returns
//! A @[Sql.pgsql_util.Result] object (which conforms to the
//! @[Sql.Result] standard interface for accessing data). It is
//! recommended to use @[query()] for simpler queries (because
//! it is easier to handle, but stores all the result in memory), and
//! @[big_query()] for queries you expect to return huge amounts of
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
//! @ref{:_text@} option (not recommended).
//!
//! @seealso
//!   @[big_typed_query()], @[Sql.Connection], @[Sql.Result],
//!   @[query()], @[Sql.pgsql_util.Result]
/*semi*/final variant .pgsql_util.Result big_query(string q,
                                   void|mapping(string|int:mixed) bindings,
                                   void|int _alltyped) {
  throwdelayederror(proxy);
  string preparedname = "";
  mapping(string:mixed) options = proxy.options;
  .pgsql_util.conxion c = proxy.c;
  int forcecache = -1, forcetext = options.text_query;
  int syncparse = zero_type(options.sync_parse)
                   ? -1 : options.sync_parse;
  if (proxy.waitforauthready)
    waitauthready();
  string cenc = proxy.runtimeparameter[CLIENT_ENCODING];
  switch(cenc) {
    case UTF8CHARSET:
      q = string_to_utf8(q);
      break;
    default:
      if (String.width(q) > 8)
        ERROR("Don't know how to convert %O to %s encoding\n", q, cenc);
  }
  array(string|int) paramValues;
  array from;
  if (bindings) {
    if (forcetext)
      q = .sql_util.emulate_bindings(q, bindings, this),
      paramValues = .pgsql_util.emptyarray;
    else {
      int pi = 0;
      paramValues = allocate(sizeof(bindings));
      from = allocate(sizeof(bindings));
      array(string) litfrom, litto, to = allocate(sizeof(bindings));
      litfrom = litto = .pgsql_util.emptyarray;
      foreach (bindings; mixed name; mixed value) {
        if (stringp(name)) {	       // Throws if mapping key is empty string
          if (name[0] != ':')
            name = ":" + name;
          if (name[1] == '_') {	       // Special option parameter
            switch(name) {
              case ":_cache":
                forcecache = (int)value;
                break;
              case ":_text":
                forcetext = (int)value;
                break;
              case ":_sync":
                syncparse = (int)value;
                break;
            }
            continue;
          }
          if (!has_value(q, name))
            continue;
        }
        if (multisetp(value)) {			// multisets are taken literally
           litto += ({indices(value)*","});	// and bypass the encoding logic
           litfrom += ({name});
        } else {
          paramValues[pi] = value;
          to[pi] = sprintf("$%d", pi + 1);
          from[pi++] = name;
        }
      }
      if (pi--) {
        paramValues = paramValues[.. pi];
        q = replace(q, litfrom += from = from[.. pi], litto += to = to[.. pi]);
      } else {
        paramValues = .pgsql_util.emptyarray;
        if (sizeof(litfrom))
          q = replace(q, litfrom, litto);
      }
      from = ({from, to, paramValues});
    }
  } else
    paramValues = .pgsql_util.emptyarray;
  if (String.width(q) > 8)
    ERROR("Wide string literals in %O not supported\n", q);
  if (has_value(q, "\0"))
    ERROR("Querystring %O contains invalid literal nul-characters\n", q);
  mapping(string:mixed) tp;
  /*
   * FIXME What happens with regards to this detection when presented with
   *       multistatement text-queries?
   *       The primary function of this detection is to ensure a SYNC
   *       right after a COMMIT, and no SYNC after a BEGIN.
   */
  int transtype = .pgsql_util.transendprefix->match(q) ? TRANSEND
   : .pgsql_util.transbeginprefix->match(q) ? TRANSBEGIN : NOTRANS;
  if (transtype != NOTRANS)
    tp = .pgsql_util.describenodata;		// Description already known
  else if (!forcetext && forcecache == 1
        || forcecache && sizeof(q) >= MINPREPARELENGTH) {
    object plugbuffer = c->start();
    if (tp = proxy.prepareds[q]) {
      if (tp.preparedname) {
#ifdef PG_STATS
        prepstmtused++;
#endif
        preparedname = tp.preparedname;
      } else if(tp.trun && tp.tparse*FACTORPLAN >= tp.trun
              && (undefinedp(options.cache_autoprepared_statements)
             || options.cache_autoprepared_statements))
        preparedname = PREPSTMTPREFIX + int2hex(pstmtcount++);
    } else {
      if (proxy.totalhits >= cachedepth)
        foreach (proxy.prepareds; string ind; tp) {
          int oldhits = tp.hits;
          proxy.totalhits -= oldhits-(tp.hits = oldhits >> 1);
          if (oldhits <= 1) {
            closestatement(plugbuffer, tp.preparedname);
            m_delete(proxy.prepareds, ind);
          }
        }
      if (forcecache != 1 && .pgsql_util.createprefix->match(q)) {
        PD("Invalidate cache\n");
        proxy.invalidatecache = 1;		// Flush cache on CREATE
        tp = 0;
      } else
        proxy.prepareds[q] = tp = ([]);
    }
    if (proxy.invalidatecache) {
      proxy.invalidatecache = 0;
      foreach (proxy.prepareds; ; mapping np) {
        closestatement(plugbuffer, np.preparedname);
        m_delete(np, "preparedname");
      }
    }
    if (sizeof(CHAIN(plugbuffer))) {
      PD("%O\n", (string)CHAIN(plugbuffer));
      plugbuffer->sendcmd(FLUSHSEND);			      // close expireds
    } else
      plugbuffer->sendcmd(KEEP);			       // close start()
  } else				  // Result autoassigns to portal
    tp = 0;
  .pgsql_util.Result portal;
  portal = .pgsql_util.Result(proxy, c, q, portalbuffersize, _alltyped,
   from, forcetext, timeout, syncparse, transtype);
  portal._tprepared = tp;
#ifdef PG_STATS
  portalsopened++;
#endif
  proxy.clearmessage = 1;
  // Do not run a query in the local_backend to prevent deadlocks
  if (Thread.this_thread() == .pgsql_util.local_backend.executing_thread())
    Thread.Thread(startquery, forcetext, portal, q, tp, preparedname);
  else
    startquery(forcetext, portal, q, tp, preparedname);
  throwdelayederror(portal);
  return portal;
}

//! This is an alias for @[big_query()], since @[big_query()] already supports
//! streaming of multiple simultaneous queries through the same connection.
//!
//! @seealso
//!   @[big_query()], @[big_typed_query()], @[streaming_typed_query()],
//!   @[Sql.Connection], @[Sql.Result]
/*semi*/final variant inline .pgsql_util.Result streaming_query(string q,
                                     void|mapping(string|int:mixed) bindings) {
  return big_query(q, bindings);
}

//! This function returns an object that allows streaming and typed
//! results.
//!
//! @seealso
//!   @[big_query()], @[Sql.Connection], @[Sql.Result]
/*semi*/final variant inline .pgsql_util.Result big_typed_query(string q,
                                     void|mapping(string|int:mixed) bindings) {
  return big_query(q, bindings, 1);
}

//! This function returns an object that allows streaming and typed
//! results.
//!
//! @seealso
//!   @[big_query()], @[Sql.Connection], @[Sql.Result]
/*semi*/final variant inline .pgsql_util.Result streaming_typed_query(string q,
                                     void|mapping(string|int:mixed) bindings) {
  return big_query(q, bindings, 1);
}
