#pike __REAL_VERSION__

//! The SQL module is a unified interface between pike and all
//! its supported databases. The parts of this module that is
//! usuable for all normal uses is the @[Sql] class and the
//! @[sql_result] class.
//!
//! @example
//! string people_in_group(string group) {
//!    Sql.Sql db = Sql.Sql("mysql://localhost/testdb");
//!    return db->query("SELECT name FROM users WHERE "
//!                     "group=%s", group)->name * ",";
//! }

//! @ignore
// Use getters and Val-> to ensure dynamic resolving in case the
// values in Val.pmod are replaced.
program `->Null() {return Val->Null;}
Val.Null `->NULL() {return Val->null;}
//! @endignore

//! @class Null
//!   Class used to implement the SQL NULL value.
//!
//! @deprecated Val.Null
//!
//! @seealso
//!   @[Val.Null], @[Val.null]

//! @endclass

//! @decl Val.Null NULL;
//!
//!   The SQL NULL value.
//!
//! @deprecated Val.null
//!
//! @seealso
//!   @[Val.null]

//! Redact the password (if any) from an Sql-url.
//!
//! @param sql_url
//!   Sql-url possibly containing an unredacted password.
//!
//! @returns
//!   Returns the same Sql-url but with the password (if any)
//!   replaced by the string @expr{"CENSORED"@}.
string censor_sql_url(string sql_url)
{
  array(string) a = sql_url/"://";
  string prot = a[0];
  string host = a[1..] * "://";
  a = host/"@";
  if (sizeof(a) > 1) {
    host = a[-1];
    a = (a[..<1] * "@")/":";
    string user = a[0];
    if (sizeof(a) > 1) {
      sql_url = prot + "://" + user + ":CENSORED@" + host;
    }
  }
  return sql_url;
}

//! Base class for a connection to an SQL database.
class Connection { inherit __builtin.Sql.Connection; }

//! Base class for the result from @[Connection.big_query()] et al.
class Result { inherit __builtin.Sql.Result; }

//! The result from @[Promise].
class FutureResult { inherit __builtin.Sql.FutureResult; }

//! The result from @[Connection.promise_query()].
class Promise { inherit __builtin.Sql.Promise; }

protected program(Connection) find_dbm(string program_name)
{
  program(Connection) p;
  // we look in Sql.type and Sql.Provider.type.type for a valid sql class.
  p = global.Sql[program_name];
  if(!p && global.Sql["Provider"] && global.Sql["Provider"][program_name])
    p = global.Sql["Provider"][program_name][program_name];
  return p;
}

//! @decl Connection Sql(string host)
//! @decl Connection Sql(string host, string db)
//! @decl Connection Sql(string host, mapping(string:int|string) options)
//! @decl Connection Sql(string host, string db, string user)
//! @decl Connection Sql(string host, string db, string user, string password)
//! @decl Connection Sql(string host, string db, string user, @
//!                      string password, mapping(string:int|string) options)
//!
//! Create a new generic SQL connection.
//!
//! @param host
//!   @mixed
//!     @type string
//!       Connect to the server specified. The string should be on the
//!       format:
//!       @tt{dbtype://[user[:password]@@]hostname[:port][/database]@}
//!       Use the @i{dbtype@} protocol to connect to the database
//!       server on the specified host. If the hostname is @expr{""@}
//!       then the port can be a file name to access through a
//!       UNIX-domain socket or similar, e g
//!       @expr{"mysql://root@@:/tmp/mysql.sock/"@}.
//!
//!       There is a special dbtype @expr{"mysqls"@} which works like
//!       @expr{"mysql"@} but sets the @tt{CLIENT_SSL@} option and
//!       loads the @tt{/etc/my.cnf@} config file to find the SSL
//!       parameters. The same function can be achieved using the
//!       @expr{"mysql"@} dbtype.
//!     @type int(0..0)
//!       Access through a UNIX-domain socket or similar.
//!   @endmixed
//!
//! @param db
//!   Select this database.
//!
//! @param user
//!   User name to access the database as.
//!
//! @param password
//!   Password to access the database.
//!
//! @param options
//!   Optional mapping of options.
//!   See the SQL-database documentation for the supported options.
//!   (eg @[Mysql.mysql()->create()]).
//!
//! @note
//!   In versions of Pike prior to 7.2 it was possible to leave out the
//!   dbtype, but that has been deprecated, since it never worked well.
//!
//! @note
//!  Exactly which databases are supported by pike depends on the
//!  installed set of client libraries when pike was compiled.
//!
//! The possible ones are
//!   @dl
//!     @item mysql
//!       libmysql based mysql connection
//!     @item mysqls
//!       libmysql based mysql connection, using SSL
//!     @item dsn
//!       @[ODBC] based connection
//!     @item msql
//!       @[Msql]
//!     @item odbc
//!       @[ODBC] based connection
//!     @item oracle
//!       @[Oracle] using oracle libraries
//!     @item pgsql
//!       PostgreSQL direct network access.
//!       This module is independent of any external libraries.
//!     @item postgres
//!       PostgreSQL libray access. Uses the @[Postgres] module.
//!     @item rsql
//!       Remote SQL api, requires a rsql server running on another host.
//!       This is an API that uses sockets to communicate with a remote pike
//!       running pike -x rsqld on another host.
//!     @item sqlite
//!       In-process SQLite database, uses the @[SQLite] module
//!     @item sybase
//!       Uses the @[sybase] module to access sybase
//!     @item tds
//!       Sybase and Microsoft SQL direct network access using the TDS protocol.
//!       This module is independent of any external libraries.
//!   @enddl
//!
//! @note
//!   Support for @[options] was added in Pike 7.3.
//!
//! @note
//!   Use of an object @[host] was deprecated in Pike 8.1.
//!
//! @note
//!   Prior to Pike 8.1 this was a wrapper class.
//!
//! @seealso
//!   @[8.0::Sql.Sql], @[Connection]
Connection Sql(string|zero host,
	       void|string|mapping(string:int|string) db,
	       void|string user, void|string _password,
	       void|mapping(string:int|string) options)
{
  // Note: No need to censor host here, since it is rewritten below if
  //       it contains an SQL-URL.
  string password = _password;
  _password = "CENSORED";

  Connection con;

  if (mappingp(db)) {
    options = db;
    db = 0;
  }
  if (db == "") {
    db = 0;
  }
  if (user == "") {
    user = 0;
  }
  if (password == "") {
    password = 0;
  }

  string program_name;

  if (host && (host != replace(host, ([ ":":"", "/":"", "@":"" ]) ))) {

    // The hostname is on the format:
    //
    // dbtype://[user[:password]@]hostname[:port][/database]

    array(string) arr = host/"://";
    if ((sizeof(arr) > 1) && (arr[0] != "")) {
      if (sizeof(arr[0]/".pike") > 1) {
	program_name = (arr[0]/".pike")[0];
      } else {
	program_name = arr[0];
      }
      host = arr[1..] * "://";
    }
    arr = host/"@";
    if (sizeof(arr) > 1) {
      // User and/or password specified
      host = arr[-1];
      arr = (arr[..<1]*"@")/":";
      if (!user && sizeof(arr[0])) {
	user = arr[0];
      }
      if (!password && (sizeof(arr) > 1)) {
	password = arr[1..]*":";
	if (password == "") {
	  password = 0;
	}
      }
    }
    arr = host/"/";
    if (sizeof(arr) > 1) {
      host = arr[..<1]*"/";
      if (!db) {
	db = arr[-1];
      }
    }
  }

  if (host == "") {
    host = 0;
  }

  if (!program_name) {
    error("No protocol specified.\n");
  }
  // Don't call ourselves...
  if ((sizeof(program_name / "_result") != 1) ||
      ((< "Sql", "sql", "sql_util", "module" >)[program_name]) ) {
    error("Unsupported protocol %O.\n", program_name);
  }

  program p;

  if (!(p = find_dbm(program_name))) {
    error("Failed to index module Sql.%s or Sql.Provider.%s.\n",
	  program_name, program_name);
  }

  if (options) {
    con = p(host||"", db||"", user||"", password||"", options);
  } else if (password) {
    con = p(host||"", db||"", user||"", password);
  } else if (user) {
    con = p(host||"", db||"", user);
  } else if (db) {
    con = p(host||"", db);
  } else if (host) {
    con = p(host);
  } else {
    con = p();
  }
  if (!con->query && !con->big_query) {
    con = UNDEFINED;
    error("Failed to index module Sql.%s or Sql.Provider.%s.\n",
	  program_name, program_name);
  }

  return con;
}

#pragma no_deprecation_warnings

//! @decl Connection Sql(__deprecated__(Connection) con)
//! @decl Connection Sql(__deprecated__(Connection) con, string db)
//!
//! Create a new generic SQL connection (DEPRECATED).
//!
//! @param con
//!   Use this connection to access the SQL-database.
//!
//! @param db
//!   Select this database.
//!
//! @note
//!   In Pike 8.1 and later this function is essentially a noop;
//!   if you actually need it, you may want to use @[8.0::Sql.Sql].
//!
//! @returns
//!   Returns @[con].
//!
//! @seealso
//!   @[8.0::Sql.Sql], @[Connection]
variant Connection Sql(__deprecated__(Connection) con, void|string db)
{
  if (db && db != "") {
    con->select_db(db);
  }

  return con;
}

#pragma deprecation_warnings


//! @class mysql_result
//! @deprecated Result

//! @endclass

__deprecated__(program(Result)) mysql_result =
  (__deprecated__(program(Result)))Result;

//! @class mysqls_result
//! @deprecated Result

//! @endclass

__deprecated__(program(Result)) mysqls_result =
  (__deprecated__(program(Result)))Result;
