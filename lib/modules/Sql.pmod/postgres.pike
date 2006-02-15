/*
 * This is part of the Postgres module for Pike.
 *
 * $Id: postgres.pike,v 1.25 2006/02/15 15:35:01 nilsson Exp $
 *
 */

//! This is an interface to the Postgres (Postgres95, pgsql) database
//! server. This module may or may not be availible on your Pike,
//! depending whether the appropriate include and library files could
//! be found at compile-time. Note that you @b{do not@} need to have a
//! Postgres server running on your host to use this module: you can
//! connect to the database over a TCP/IP socket.
//!
//! @note
//! Also note that @b{this module uses blocking I/O@} I/O to connect
//! to the server. Postgres is quite slow, and so you might want to
//! consider this particular aspect. It is (at least should be)
//! thread-safe, and so it can be used in a multithread environment.
//!
//! The behavior of the Postgres C API also depends on certain
//! environment variables defined in the environment of the Pike
//! interpreter.
//!
//! @string
//!   @value "PGHOST"
//!     Sets the name of the default host to connect to. It defaults
//! 	to @expr{"localhost"@}.
//!   @value "PGOPTIONS"
//!     Sets some extra flags for the frontend-backend connection.
//!     @b{do not set@} unless you're sure of what you're doing.
//!   @value "PGPORT"
//!     Sets the default port to connect to, otherwise it will use
//!     compile-time defaults (that is: the time you compiled the postgres
//! 	library, not the Pike driver).
//!   @value "PGTTY"
//!     Sets the file to be used for Postgres frontend debugging.
//!     Do not use, unless you're sure of what you're doing.
//!   @value "PGDATABASE"
//!     Sets the default database to connect to.
//!   @value "PGREALM"
//!     Sets the default realm for Kerberos authentication. I never used
//!   	this, so I can't help you.
//! @endstring
//!
//! Refer to the Postgres documentation for further details.
//!
//! @seealso
//!  @[Sql.Sql], @[Postgres.postgres], @[Sql.postgres_result]

#pike __REAL_VERSION__

#if constant(Postgres.postgres)

#define ERROR(X) throw (({X,backtrace()}))

inherit Postgres.postgres: mo;
private static mixed  callout;
private string has_relexpires = "unknown";

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

//! @decl string error()
//!
//! This function returns the textual description of the last
//! server-related error. Returns @expr{0@} if no error has occurred
//! yet. It is not cleared upon reading (can be invoked multiple
//! times, will return the same result until a new error occurs).
//!
//! @seealso
//!   big_query

//! @decl string host_info()
//!
//! This function returns a string describing what host are we talking to,
//! and how (TCP/IP or UNIX sockets).

//! @decl void reset()
//!
//! This function resets the connection to the backend. Can be used for
//! a variety of reasons, for example to detect the status of a connection.
//!
//! @note
//! This function is Postgres-specific, and thus it is not availible
//! through the generic SQL-interface.

//! @decl string version
//!
//! Should you need to report a bug to the author, please submit along with
//! the report the driver version number, as returned by this call.

private static string glob_to_regexp (string glob) {
	if (!glob||!sizeof(glob))
		return 0;
	return "^"+replace(glob,({"*","?","'","\\"}),({".*",".","\\'","\\\\"}))+"$";
}

static private int mkbool(string s) {
	if (s=="f")
		return 0;
	return 1;
}

//! @decl void create()
//! @decl void create(string host, void|string database, void|string user,@
//!                   void|string password)
//!
//! With no arguments, this function initializes (reinitializes if a
//! connection had been previously set up) a connection to the
//! Postgres backend. Since Postgres requires a database to be
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
//! The username and password arguments are silently ignored, since
//! the Postgres C API doesn't allow to connect to the server as any
//! user different than the user running the interface.
//!
//! @note
//! You need to have a database selected before using the sql-object,
//! otherwise you'll get exceptions when you try to query it. Also
//! notice that this function @b{can@} raise exceptions if the db
//! server doesn't respond, if the database doesn't exist or is not
//! accessible by you.
//!
//! You don't need bothering about syncronizing the connection to the database:
//! it is automatically closed (and the database is sync-ed) when the
//! object is destroyed.
//!
//! @seealso
//!   @[Postgres.postgres], @[Sql.Sql], @[postgres->select_db]
void create(void|string host, void|string database, void|string user,
		void|string pass) {
	string real_host=host, real_db=database;
	int port=0;
	if (stringp(host)&&(search(host,":")>=0))
		if (sscanf(host,"%s:%d",real_host,port)!=2)
			ERROR("Error in parsing the hostname argument.\n");
	
	mo::create(real_host||"",real_db||"",user||"",pass||"",port);
}

static void poll (int delay)
{
	callout=call_out(poll,delay,delay);
	big_query("");
}

//! @decl void set_notify_callback()
//! @decl void set_notify_callback(function f)
//! @decl void set_notify_callback(function f, int|float poll_delay)
//!
//! With Postgres you can associate events and notifications to tables.
//! This function allows you to detect and handle such events.
//!
//! With no arguments, resets and removes any callback you might have
//! put previously, and any polling cycle.
//!
//! With one argument, sets the notification callback (there can be only
//! one for each sqlobject). 
//! 
//! With two arguments, sets a notification callback and sets a polling
//! cycle.
//!
//! The polling cycle is necessary because of the way notifications are
//! delivered, that is piggyback with a query result. This means that
//! if you don't do any query, you'll receive no notification. The polling
//! cycle starts a call_out cycle which will do an empty query when
//! the specified interval expires, so that pending notifications 
//! may be delivered.
//!
//! The callback function must return no value, and takes a string argument,
//! which will be the name of the table on which the notification event
//! has occured. In future versions, support for user-specified arguments
//! will be added.
//!
//! @note
//! The polling cycle can be run only if your process is in "event-driven mode"
//! (that is, if 'main' has returned a negative number).
//!
//! This function is Postgres-specific, and thus it is not availible
//! through the generic SQL-interface.
//!
//! @fixme
//! An integer can be passed as first argument, but it's effect is
//! not documented.
void set_notify_callback(int|function f, int|float|void poll_delay) {
	if (callout) {
		remove_call_out(callout);
		callout=0;
	}
	if (intp(f)) {
		mo::_set_notify_callback(0);
		return;
	}
	mo::_set_notify_callback(f);
	if(poll_delay>0) 
		poll(poll_delay);
}

string quote(string s)
{
  return replace(s, ({ "\\", "'", "\0" }), ({ "\\\\", "''", "\\0" }) );
}

//! This function creates a new database with the given name (assuming we
//! have enough permissions to do this).
//!
//! @seealso
//!   drop_db
void create_db(string db) {
	big_query(sprintf("CREATE DATABASE %s",db));
}

//! This function destroys a database and all the data it contains (assuming
//! we have enough permissions to do so).
//!
//! @seealso
//!   create_db
void drop_db(string db) {
	big_query(sprintf("DROP database %s",db));
}

//! This function returns a string describing the server we are
//! talking to. It has the form @expr{"servername/serverversion"@}
//! (like the HTTP protocol description) and is most useful in
//! conjunction with the generic SQL-server module.
string server_info () {
  return "Postgres/unknown";
}

//! Lists all the databases available on the server.
//! If glob is specified, lists only those databases matching it.
array(string) list_dbs (void|string glob) {
	array name,ret=({});
	object res=
		big_query(
				"SELECT datname from pg_database"+
				((glob&&sizeof(glob))? " WHERE datname ~ '"+glob_to_regexp(glob)+"'" : "")
				);
	while (name=res->fetch_row()) {
		ret += ({name[0]});
	}
	return sort(ret);
}

//! Returns an array containing the names of all the tables in the currently
//! selected database.
//! If a glob is specified, it will return only those tables
//! whose name matches it.
array(string) list_tables (void|string glob) {
	array name,ret=({});
	object res;
	res=big_query(
			"SELECT relname, relkind FROM pg_class, pg_user "
			"WHERE ( relkind = 'r' OR relkind = 'i' OR relkind = 'S') "
			"AND relname !~ '^pg_' "
			"AND usesysid = relowner " +
			((glob && sizeof(glob)) ? "AND relname ~ '"+glob_to_regexp(glob)+"' " : "") +
			"ORDER BY relname"
			);
	while (name=res->fetch_row()) {
		ret += ({name[0]});
	}
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
//!   @member int "has_rules"
//!
//!   @member int "is_shared"
//!
//!   @member string "owner"
//!     The textual representation of a Postgres uid.
//!
//!   @member string "length"
//!
//!   @member string "text"
//!     A textual description of the internal (to the server) type-name
//!
//!   @member mixed "default"
//!
//!   @member string "expires"
//!     The "relexpires" attribute for the table. Obsolescent; modern
//!     versions of Postgres don't seem to use this feature, so don't
//!     count on this field to contain any useful value.
//!
//! @endmapping
//!
array(mapping(string:mixed)) list_fields (string table, void|string wild)
{
  array row, ret=({});
  string schema;

  if (has_relexpires == "unknown")
  {
    if (catch (big_query("SELECT relexpires FROM pg_class WHERE 1 = 0")))
      has_relexpires = "no";
    else
      has_relexpires = "yes";
  }

  sscanf(table, "%s.%s", schema, table);

  object res = big_query(
	"SELECT a.attnum, a.attname, t.typname, a.attlen, c.relowner, "
	"c.relisshared, c.relhasrules, t.typdefault " +
        (has_relexpires == "yes" ? ", c.relexpires " : "") +
	(schema ? ", s.schemaname " : "") +
	"FROM pg_class c, pg_attribute a, pg_type t " +
	(schema ? ", pg_tables s " : "") +
	"WHERE c.relname = '"+table+"' AND a.attnum > 0 " +
	(schema ? "AND s.tablename = '"+table+"' " : "") +
	"AND a.attrelid = c.oid AND a.atttypid = t.oid ORDER BY attnum");

  while (row = res->fetch_row())
  {
    if (wild && sizeof(wild) && !glob(wild, row[1]))
      continue;
    ret +=
      ({ ([
	 "name":	row[1],
         "type":	row[2],
         "length":	row[3],
	 "owner":	row[4],
         "is_shared":	mkbool(row[5]),
	 "has_rules":   mkbool(row[6]),
	 "default":     row[7],
         "expires":     (sizeof(row) > 8 ? row[8] : 0)
       ]) });
  }
  return ret;
}

//! This is the only provided interface which allows you to query the
//! database. If you wish to use the simpler "query" function, you need to
//! use the @[Sql.Sql] generic SQL-object.
//!
//! It returns a postgres_result object (which conforms to the
//! @[Sql.sql_result] standard interface for accessing data). I
//! recommend using @[query()] for simpler queries (because it is
//! easier to handle, but stores all the result in memory), and
//! @[big_query()] for queries you expect to return huge amounts of
//! data (it's harder to handle, but fectches results on demand).
//!
//! @note
//! This function @b{can@} raise exceptions.
//!
//! @seealso
//!   @[Sql.Sql], @[Sql.sql_result]
int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{  
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this));
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(Postgres.postgres) */
