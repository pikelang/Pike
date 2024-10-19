/*
 * Implements the generic parts of the SQL-interface
 *
 * Henrik Grubbstr�m 1996-01-09
 */

#pike 8.1

#pragma no_deprecation_warnings

//! This class encapsulates a connection to an SQL server. It is a
//! generic interface on top of the DB server specific
//! implementations. That doesn't mean that there aren't plenty of
//! server specific characteristics that still shine through, though.
//!
//! This class also serves as an interface guideline for the DB server
//! specific connection classes.
//!
//! @section Untyped and typed mode
//!
//! The query results are returned in different ways depending on the
//! query functions used: The @tt{..typed_query@} functions select
//! typed mode, while the other query functions uses the older untyped
//! mode.
//!
//! In untyped mode, all values except SQL NULL are returned as
//! strings in their display representation, and SQL NULL is returned
//! as zero.
//!
//! In typed mode, values are returned in pike native form where it
//! works well. That means at least that SQL integer fields are
//! returned as pike integers, floats as floats, SQL NULL as
//! @[Val.null], and of course strings still as strings. The
//! representation of other SQL types depends on the capabilities of
//! the server specific backends. It's also possible that floats in
//! some cases are represented in other ways if too much precision is
//! lost in the conversion to pike floats.
//!
//! @endsection
//!
//! @note
//! For historical reasons, there may be server specific backends that
//! operate differently from what is described here, e.g. some that
//! return a bit of typed data in untyped mode.
//!
//! @note
//! Typed operation was not supported at all prior to Pike 7.8.363,
//! and may not be supported for all databases.

#define ERROR(X ...)	predef::error(X)

//! Server specific connection object used for the actual SQL queries.
object master_sql;

//! Convert all field names in mappings to lower_case.
//! Only relevant to databases which only implement big_query(),
//! and use upper/mixed-case fieldnames (eg Oracle).
//! @int
//! @value 0
//!   No (default)
//! @value 1
//!   Yes
//! @endint
int(0..1) case_convert;

//! @decl string quote(string s)
//! Quote a string @[s] so that it can safely be put in a query.
//!
//! All input that is used in SQL-querys should be quoted to prevent
//! SQL injections.
//!
//! Consider this harmfull code:
//! @code
//!   string my_input = "rob' OR name!='rob";
//!   string my_query = "DELETE FROM tblUsers WHERE name='"+my_input+"'";
//!   my_db->query(my_query);
//! @endcode
//!
//! This type of problems can be avoided by quoting @tt{my_input@}.
//! @tt{my_input@} would then probably read something like
//! @i{rob\' OR name!=\'rob@}
//!
//! Usually this is done - not by calling quote explicitly - but through
//! using a @[sprintf] like syntax
//! @code
//!   string my_input = "rob' OR name!='rob";
//!   my_db->query("DELETE FROM tblUsers WHERE name=%s",my_input);
//! @endcode

function(string:string) quote = .sql_util.quote;

//! @decl string encode_time(int t, int|void is_utc)
//! Converts a system time value to an appropriately formatted time
//! spec for the database.
//! @param t
//!   Time to encode.
//! @param is_utc
//!   If nonzero then time is taken as a "full" unix time spec
//!   (where the date part is ignored), otherwise it's converted as a
//!   seconds-since-midnight value.

function(int,void|int:string) encode_time;

//! @decl int decode_time(string t, int|void want_utc)
//! Converts a database time spec to a system time value.
//! @param t
//!   Time spec to decode.
//! @param want_utc
//!   Take the date part from this system time value. If zero, a
//!   seconds-since-midnight value is returned.

function(string,void|int:int) decode_time;

//! @decl string encode_date(int t)
//! Converts a system time value to an appropriately formatted
//! date-only spec for the database.
//! @param t
//!   Time to encode.

function(int:string) encode_date;

//! @decl int decode_date(string d)
//! Converts a database date-only spec to a system time value.
//! @param d
//!   Date spec to decode.

function(string:int) decode_date;

//! @decl string encode_datetime(int t)
//! Converts a system time value to an appropriately formatted
//! date and time spec for the database.
//! @param t
//!   Time to encode.

function(int:string) encode_datetime;

//! @decl int decode_datetime(string datetime)
//! Converts a database date and time spec to a system time value.
//! @param datetime
//!   Date and time spec to decode.

function(string:int) decode_datetime;

protected program find_dbm(string program_name) {
  program p;
  // we look in Sql.type and Sql.Provider.type.type for a valid sql class.
  p = Sql[program_name];
  if(!p && Sql["Provider"] && Sql["Provider"][program_name])
    p = Sql["Provider"][program_name][program_name];
  return p;
}

//! @decl void create(string host)
//! @decl void create(string host, string db)
//! @decl void create(string host, mapping(string:int|string) options)
//! @decl void create(string host, string db, string user)
//! @decl void create(string host, string db, string user, @
//!                   string password)
//! @decl void create(string host, string db, string user, @
//!                   string password, mapping(string:int|string) options)
//! @decl void create(object host)
//! @decl void create(object host, string db)
//!
//! Create a new generic SQL object.
//!
//! @param host
//!   @mixed
//!     @type object
//!       Use this object to access the SQL-database.
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
//! @dl
//! @item mysql
//!   libmysql based mysql connection
//! @item mysqls
//!   libmysql based mysql connection, using SSL
//! @item dsn
//!   @[ODBC] based connection
//! @item msql
//!   @[Msql]
//! @item odbc
//!   @[ODBC] based connection
//! @item oracle
//!  @[Oracle] using oracle libraries
//! @item pgsql
//!  PostgreSQL direct network access.
//!  This module is independent of any external libraries.
//! @item postgres
//!  PostgreSQL libray access. Uses the @[Postgres] module.
//! @item rsql
//!  Remote SQL api, requires a rsql server running on another host.
//!  This is an API that uses sockets to communicate with a remote pike
//!  running pike -x rsqld on another host.
//! @item sqlite
//!   In-process SQLite database, uses the @[SQLite] module
//! @item sybase
//!   Uses the @[sybase] module to access sybase
//! @item tds
//!  Sybase and Microsoft SQL direct network access using the TDS protocol.
//!  This module is independent of any external libraries.
//! @enddl
//!
//! @note
//!   Support for @[options] was added in Pike 7.3.
//!
void create(string|object|zero host, void|string|mapping(string:int|string) db,
	    void|string user, void|string _password,
	    void|mapping(string:int|string) options)
{
  // Note: No need to censor host here, since it is rewritten below if
  //       it contains an SQL-URL.
  string|zero password = _password;
  _password = "CENSORED";

  if (objectp(host)) {
    master_sql = host;
    if ((user && user != "") || (password && password != "") ||
	(options && sizeof(options))) {
      ERROR("Only the database argument is supported when "
	    "first argument is an object.\n");
    }
    if (db && db != "") {
      master_sql->select_db(db);
    }
  }
  else {
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
      ERROR("No protocol specified.\n");
    }
    // Don't call ourselves...
    if ((sizeof(program_name / "_result") != 1) ||
	  ((< "Sql", "sql", "sql_util", "module" >)[program_name]) ) {
      ERROR("Unsupported protocol %O.\n", program_name);
    }


    program p;

    if (p = find_dbm(program_name)) {
      if (options) {
	master_sql = p(host||"", db||"", user||"", password||"", options);
      } else if (password) {
	master_sql = p(host||"", db||"", user||"", password);
      } else if (user) {
	master_sql = p(host||"", db||"", user);
      } else if (db) {
	master_sql = p(host||"", db);
      } else if (host) {
	master_sql = p(host);
      } else {
	master_sql = p();
      }
      if (!master_sql->query && !master_sql->big_query) {
	master_sql = UNDEFINED;
	ERROR("Failed to index module Sql.%s or Sql.Provider.%s.\n",
	      program_name, program_name);
      }
    } else {
      ERROR("Failed to index module Sql.%s or Sql.Provider.%s.\n",
	    program_name, program_name);
    }
  }

  if (master_sql->quote) quote = master_sql->quote;
  encode_time = master_sql->encode_time || .sql_util.fallback;
  decode_time = master_sql->decode_time || .sql_util.fallback;
  encode_date = master_sql->encode_date || .sql_util.fallback;
  decode_date = master_sql->decode_date || .sql_util.fallback;
  encode_datetime = master_sql->encode_datetime || .sql_util.fallback;
  decode_datetime = master_sql->decode_datetime || .sql_util.fallback;
}

protected void _destruct() {
  if (master_sql)
    destruct(master_sql);
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
int is_open()
{
  if (!master_sql) return 0;
  if (master_sql->is_open) return master_sql->is_open();
  return 1;
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
//!       The connection reconnected automatically.
//!     @value -1
//!       The server has gone away, and the connection is dead.
//!   @endint
//!
//! @seealso
//!   @[is_open()]
int ping()
{
  if (!master_sql) return -1;
  if (master_sql->ping) return master_sql->ping();
  catch {
    return sizeof(query("SELECT 0 AS zero") || ({})) - 1;
  };
  master_sql = UNDEFINED;	// Inform is_open().
  return -1;
}

void set_charset (string charset)
//! Changes the charset that the connection uses for queries and
//! returned text strings.
//!
//! @param charset
//!   The charset to use. The valid values and their meanings depends
//!   on the database brand. However, the special value
//!   @expr{"unicode"@} (if supported) selects a mode where the query
//!   and result strings are unencoded (and possibly wide) unicode
//!   strings.
//!
//! @throws
//!   An error is thrown if the connection doesn't support the
//!   specified charset, or doesn't support charsets being set this
//!   way at all.
//!
//! @note
//!   See the @expr{set_charset@} functions for each database
//!   connection type for further details about the effects on the
//!   connection.
//!
//! @seealso
//!   @[get_charset], @[Sql.mysql.set_charset]
{
  if (!master_sql->set_charset)
    predef::error ("This database connection does not "
		   "support charset switching.\n");
  master_sql->set_charset (charset);
}

string get_charset()
//! Returns the (database dependent) name of the charset used for (at
//! least) query strings. Returns zero if the connection doesn't
//! support charsets this way (typically means that a call to
//! @[set_charset] will throw an error).
//!
//! @seealso
//!   @[set_charset], @[Sql.mysql.get_charset]
{
  return master_sql->get_charset && master_sql->get_charset();
}

protected string _sprintf(int type, mapping|void flags)
{
  if(type=='O' && master_sql && master_sql->_sprintf)
    return sprintf("Sql.%O", master_sql);
}

protected array(mapping(string:mixed)) res_obj_to_array(object res_obj)
{
  if (!res_obj)
    return 0;

  array(mapping(string:mixed)) res = ({});
  while (res_obj)
  {
    // Not very efficient, but sufficient
    array(string) fieldnames;
    array(mixed) row;

    array(mapping) fields = res_obj->fetch_fields();
    if(!sizeof(fields)) return ({});

    int has_table = fields[0]->table && fields[0]->table!="";

    if(has_table)
      fieldnames = (map(fields,
			lambda (mapping(string:mixed) m) {
			  return (m->table||"") + "." + m->name;
			}) +
		    fields->name);
    else
      fieldnames = fields->name;

    if (case_convert)
      fieldnames = map(fieldnames, lower_case);

    if(has_table)
      while (row = res_obj->fetch_row())
	res += ({ mkmapping(fieldnames, row + row) });
    else
      while (row = res_obj->fetch_row())
	res += ({ mkmapping(fieldnames, row) });

    // Try the next result.
    res_obj = res_obj->next_result && res_obj->next_result();
  }
  return res;
}

//! Return last error message.
int|string error()
{
  if (master_sql && functionp (master_sql->error))
    return master_sql->error();
  return "Unknown error";
}

//! Return last SQLSTATE.
//!
//! The SQLSTATE error codes are specified in ANSI SQL.
string sqlstate() {
    if (master_sql && functionp(master_sql->sqlstate)) {
        return master_sql->sqlstate();
    }
    return "IM001"; // "driver does not support this function"
}

//! Select database to access.
void select_db(string db)
{
  master_sql->select_db(db);
}

//! Compiles the query (if possible). Otherwise returns it as is.
//! The resulting object can be used multiple times to the query
//! functions.
//!
//! @param q
//!   SQL-query to compile.
//!
//! @seealso
//! @[query], @[typed_query], @[big_query], @[big_typed_query],
//! @[streaming_query], @[streaming_typed_query]
string|object compile_query(string q)
{
  if (functionp(master_sql->compile_query)) {
    return master_sql->compile_query(q);
  }
  return q;
}

array(string|mapping(string|int:mixed))
  handle_extraargs(string query, array(mixed) extraargs)
// Compat wrapper.
{
  return .sql_util.handle_extraargs (query, extraargs);
}

//!
int insert_id()
{
  return master_sql->insert_id();
}

//!
int affected_rows()
{
  return master_sql->affected_rows();
}

//! Sends an SQL query synchronously to the underlying SQL-server and
//! returns the results in untyped mode.
//!
//! @param q
//!   Query to send to the SQL-server. This can either be a string with the
//!   query, or a previously compiled query (see @[compile_query()]).
//! @param extraargs
//!   This parameter, if specified, can be in two forms:
//!
//!   @ol
//!     @item
//!     A mapping containing bindings of variables used in the query.
//!     A variable is identified by a colon (:) followed by a name or number.
//!     Each index in the mapping corresponds to one such variable, and the
//!     value for that index is substituted (quoted) into the query wherever
//!     the variable is used.
//!
//! @code
//! res = query("SELECT foo FROM bar WHERE gazonk=:baz",
//!             ([":baz":"value"]));
//! @endcode
//!
//!     Binary values (BLOBs) may need to be placed in multisets.
//!
//!     @item
//!     Arguments as you would use in sprintf. They are automatically
//!     quoted.
//!
//! @code
//! res = query("select foo from bar where gazonk=%s","value");
//! @endcode
//!   @endol
//!
//! @returns
//!   Returns one of the following on success:
//!   @mixed
//!     @type array(mapping(string:string))
//!       The result as an array of mappings indexed on the name of
//!       the columns. The values are either strings with the display
//!       representations or zero for the SQL NULL value.
//!     @type zero
//!       The value @expr{0@} (zero) if the query didn't return any
//!       result (eg @tt{INSERT@} or similar).
//!   @endmixed
//!
//! @throws
//!   Throws an exception if the query fails.
//!
//! @seealso
//!   @[typed_query], @[big_query], @[streaming_query]
array(mapping(string:string)) query(object|string q,
				    mixed ... extraargs)
{
  if (sizeof(extraargs)) {
    mapping(string|int:mixed) bindings;

    if (mappingp(extraargs[0]))
      bindings=extraargs[0];
    else
      [q,bindings]=.sql_util.handle_extraargs(q,extraargs);

    if(bindings) {
      if(master_sql->query)
	return master_sql->query(q, bindings);
      return res_obj_to_array(master_sql->big_query(q, bindings));
    }
  }

  if (master_sql->query)
    return master_sql->query(q);
  return res_obj_to_array(master_sql->big_query(q));
}

//! Sends an SQL query synchronously to the underlying SQL-server and
//! returns the results in typed mode.
//!
//! For the arguments, please see the @[query()] function.
//!
//! @returns
//!   Returns one of the following on success:
//!   @mixed
//!     @type array(mapping(string:mixed))
//!       The result as an array of mappings indexed on the name of
//!       the columns. The values have the appropriate native pike
//!       types where they fit the SQL data types - see the class doc
//!       for details on typed mode.
//!     @type zero
//!       The value @expr{0@} (zero) if the query didn't return any
//!       result (eg @tt{INSERT@} or similar).
//!   @endmixed
//!
//! @note
//!   Typed mode is not supported by all sql databases. If not
//!   supported, an error is thrown.
//!
//! @seealso
//!   @[query], @[big_typed_query]
array(mapping(string:mixed)) typed_query(object|string q, mixed ... extraargs)
{
  if (!master_sql->big_typed_query)
    ERROR ("Typed mode not supported by the backend %O.\n", master_sql);

  if (sizeof(extraargs)) {
    mapping(string|int:mixed) bindings;

    if (mappingp(extraargs[0]))
      bindings=extraargs[0];
    else
      [q,bindings]=.sql_util.handle_extraargs(q,extraargs);

    if(bindings) {
      if(master_sql->typed_query)
	return master_sql->typed_query(q, bindings);
      return res_obj_to_array(master_sql->big_typed_query(q, bindings));
    }
  }

  if (master_sql->typed_query)
    return master_sql->typed_query(q);
  return res_obj_to_array(master_sql->big_typed_query(q));
}

//! Sends an SQL query synchronously to the underlying SQL-server and
//! returns the results in untyped mode.
//!
//! For the arguments, please see the @[query()] function.
//!
//! The result is returned as an @[Sql.sql_result] object in untyped
//! mode. This allows for having some more info about the result as
//! well as processing the result in a streaming fashion, although the
//! result itself wasn't obtained streamingly from the server. Returns
//! @expr{0@} if the query didn't return any result (e.g. @tt{INSERT@}
//! or similar).
//!
//! @note
//! Despite the name, this function is not only useful for "big"
//! queries. It typically has less overhead than @[query] also for
//! ones that return only a few rows.
//!
//! @seealso
//!   @[query], @[streaming_query]
int|object big_query(object|string q, mixed ... extraargs)
{
  mapping(string|int:mixed) bindings;

  if (sizeof(extraargs)) {
    if (mappingp(extraargs[0]))
      bindings = extraargs[0];
    else
      [q, bindings] = .sql_util.handle_extraargs(q, extraargs);
  }

  object|array(mapping) pre_res;

  if(bindings) {
    if(master_sql->big_query)
      pre_res = master_sql->big_query(q, bindings);
    else
      pre_res = master_sql->query(q, bindings);
  } else {
    if (master_sql->big_query)
      pre_res = master_sql->big_query(q);
    else
      pre_res = master_sql->query(q);
  }

  if(pre_res) {
    if(objectp(pre_res))
      return .sql_object_result(pre_res);
    else
      return .sql_array_result(pre_res);
  }
  return 0;
}

//! Sends an SQL query synchronously to the underlying SQL-server and
//! returns the results in typed mode.
//!
//! For the arguments, please see the @[query()] function.
//!
//! The result is returned as an @[Sql.sql_result] object in typed
//! mode. This allows for having some more info about the result as
//! well as processing the result in a streaming fashion, although the
//! result itself wasn't obtained streamingly from the server. Returns
//! @expr{0@} if the query didn't return any result (e.g. @tt{INSERT@}
//! or similar).
//!
//! @note
//!   Typed mode is not supported by all sql databases. If not
//!   supported, an error is thrown.
//!
//! @note
//! Despite the name, this function is not only useful for "big"
//! queries. It typically has less overhead than @[typed_query] also
//! for ones that return only a few rows.
//!
//! @seealso
//!   @[query], @[typed_query], @[big_query], @[streaming_query]
int|object big_typed_query(object|string q, mixed ... extraargs)
{
  if (!master_sql->big_typed_query)
    ERROR ("Typed mode not supported by the backend %O.\n", master_sql);

  mapping(string|int:mixed) bindings;

  if (sizeof(extraargs)) {
    if (mappingp(extraargs[0]))
      bindings = extraargs[0];
    else
      [q, bindings] = .sql_util.handle_extraargs(q, extraargs);
  }

  object|array(mapping) pre_res;

  if (bindings) {
    pre_res = master_sql->big_typed_query(q, bindings);
  } else {
    pre_res = master_sql->big_typed_query(q);
  }

  if(pre_res) return .sql_object_result(pre_res);
  return 0;
}

//! Sends an SQL query synchronously to the underlying SQL-server and
//! returns the results streaming in untyped mode.
//!
//! For the arguments, please see the @[query()] function.
//!
//! The result is returned as a streaming @[Sql.sql_result] object in
//! untyped mode. This allows for having results larger than the
//! available memory, and returning some more info about the result.
//! Returns @expr{0@} if the query didn't return any result (e.g.
//! INSERT or similar). For the other arguments, they are the same as
//! for the @[query()] function.
//!
//! @note
//!   Streaming operation is not supported by all sql databases.
//!   If not supported, this function will fall back to calling
//!   @[big_query()].
//!
//! @seealso
//!   @[big_query], @[streaming_typed_query]
int|object streaming_query(object|string q, mixed ... extraargs)
{
  if(!master_sql->streaming_query) return big_query(q, @extraargs);

  mapping(string|int:mixed) bindings;

  if (sizeof(extraargs)) {
    if(mappingp(extraargs[0]))
      bindings = extraargs[0];
    else
      [q, bindings] = .sql_util.handle_extraargs(q, extraargs);
  }

  object pre_res;

  if(bindings) {
    pre_res = master_sql->streaming_query(q, bindings);
  } else {
    pre_res = master_sql->streaming_query(q);
  }

  if(pre_res) return .sql_object_result(pre_res);
  return 0;
}

//! Sends an SQL query synchronously to the underlying SQL-server and
//! returns the results streaming in typed mode.
//!
//! For the arguments, please see the @[query()] function.
//!
//! The result is returned as a streaming @[Sql.sql_result] object in
//! typed mode. This allows for having results larger than the
//! available memory, and returning some more info about the result.
//! Returns @expr{0@} if the query didn't return any result (e.g.
//! INSERT or similar).
//!
//! @note
//!   Neither streaming operation nor typed results are supported
//!   by all sql databases. If not supported, this function will
//!   fall back to calling @[big_typed_query()].
//!
//! @seealso
//!   @[streaming_query], @[big_typed_query]
int|object streaming_typed_query(object|string q, mixed ... extraargs)
{
  if(!master_sql->streaming_typed_query) return big_typed_query(q, @extraargs);

  mapping(string|int:mixed) bindings;

  if (sizeof(extraargs)) {
    if(mappingp(extraargs[0]))
      bindings = extraargs[0];
    else
      [q, bindings] = .sql_util.handle_extraargs(q, extraargs);
  }

  object pre_res;

  if(bindings) {
    pre_res = master_sql->streaming_typed_query(q, bindings);
  } else {
    pre_res = master_sql->streaming_typed_query(q);
  }

  if(pre_res) return .sql_object_result(pre_res);
  return 0;
}

//! Create a new database.
//!
//! @param db
//!   Name of database to create.
void create_db(string db)
{
  master_sql->create_db(db);
}

//! Drop database
//!
//! @param db
//!   Name of database to drop.
void drop_db(string db)
{
  master_sql->drop_db(db);
}

//! Shutdown a database server.
void shutdown()
{
  if (functionp(master_sql->shutdown)) {
    master_sql->shutdown();
  } else {
    ERROR("Not supported by this database.\n");
  }
}

//! Reload the tables.
void reload()
{
  if (functionp(master_sql->reload)) {
    master_sql->reload();
  } else {
    // Probably safe to make this a NOOP
  }
}

//! Return info about the current SQL-server.
string server_info()
{
  if (functionp(master_sql->server_info)) {
    return master_sql->server_info() ;
  }
  return "Unknown SQL-server";
}

//! Return info about the connection to the SQL-server.
string host_info()
{
  if (functionp(master_sql->host_info)) {
    return master_sql->host_info();
  }
  return "Unknown connection to host";
}

//! List available databases on this SQL-server.
//!
//! @param wild
//!   Optional wildcard to match against.
array(string) list_dbs(string|void wild)
{
  array(string)|array(mapping(string:mixed))|object res;

  if (functionp(master_sql->list_dbs)) {
    if (objectp(res = master_sql->list_dbs())) {
      res = res_obj_to_array(res);
    }
  } else {
    catch {
      res = query("show databases");
    };
  }
  if (res && sizeof(res) && mappingp(res[0])) {
    res = map(res, lambda (mapping m) {
		     return values(m)[0]; // Hope that there's only one field
		   } );
  }
  if (res && wild) {
    res = filter(res,
		 Regexp(replace(wild, ({"%", "_"}), ({".*", "."})))->match);
  }
  return res;
}

//! List tables available in the current database.
//!
//! @param wild
//!   Optional wildcard to match against.
array(string) list_tables(string|void wild)
{
  array(string)|array(mapping(string:mixed))|object res;

  if (functionp(master_sql->list_tables)) {
    if (objectp(res = master_sql->list_tables())) {
      res = res_obj_to_array(res);
    }
  } else {
    catch {
      res = query("show tables");
    };
  }
  if (res && sizeof(res) && mappingp(res[0])) {
    string col_name = indices(res[0])[0];
    if (sizeof(res[0]) > 1) {
      if (!zero_type(res[0]["TABLE_NAME"])) {
	// MSSQL.
	col_name = "TABLE_NAME";
      }
    }
    res = map(res, lambda (mapping m, string col_name) {
		     return m[col_name];
		   }, col_name);
  }
  if (res && wild) {
    res = filter(res,
		 Regexp(replace(wild, ({"%", "_"}), ({".*", "."})))->match);
  }
  return res;
}

//! List fields available in the specified table
//!
//! @param table
//!   Table to list the fields of.
//!
//! @param wild
//!   Optional wildcard to match against.
array(mapping(string:mixed)) list_fields(string table, string|void wild)
{
  array(mapping(string:mixed))|object res;

  if (functionp(master_sql->list_fields)) {
    if (objectp(res = master_sql->list_fields(table))) {
      res = res_obj_to_array(res);
    }
    if (wild) {
      res = filter(res,
		   lambda(mapping row, function(string:int) match) {
		     return match(row->name);
		   },
		   Regexp(replace(wild, ({"%", "_"}), ({".*", "."})))->match);
    }
    return res;
  }
  catch {
    if (wild) {
      res = query("show fields from \'" + table +
		  "\' like \'" + wild + "\'");
    } else {
      res = query("show fields from \'" + table + "\'");
    }
  };
  res = res && map(res, lambda (mapping m, string table) {
    foreach(indices(m), string str) {
      // Add the lower case variants
      string low_str = lower_case(str);
      if (low_str != str && !m[low_str])
	m[low_str] = m_delete(m, str);
    }

    if ((!m->name) && m->field)
      m["name"] = m_delete(m, "field");

    if (!m->table)
      m["table"] = table;

    return m;
  }, table);
  return res;
}
