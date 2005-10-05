/*
 * $Id: Sql.pike,v 1.83 2005/10/05 07:51:32 nilsson Exp $
 *
 * Implements the generic parts of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#pike __REAL_VERSION__

//! Implements those functions that need not be present in all SQL-modules.

#define ERROR(X ...)	predef::error(X)

//! Object to use for the actual SQL-queries.
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

static program find_dbm(string program_name) {
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
//!   Support for @[options] was added in Pike 7.3.
//!
void create(string|object host, void|string|mapping(string:int|string) db,
	    void|string user, void|string password,
	    void|mapping(string:int|string) options)
{
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
	arr = (arr[0..sizeof(arr)-2]*"@")/":";
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
	host = arr[..sizeof(arr)-2]*"/";
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

static string _sprintf(int type, mapping|void flags)
{
  if(type=='O' && master_sql && master_sql->_sprintf)
    return sprintf("Sql.%O", master_sql);
}

static private array(mapping(string:mixed)) res_obj_to_array(object res_obj)
{
  if (res_obj) 
  {
    // Not very efficient, but sufficient
    array(mapping(string:mixed)) res = ({});
    array(string) fieldnames;
    array(mixed) row;
    array(mapping) fields = res_obj->fetch_fields();

    fieldnames = (map(fields,
		      lambda (mapping(string:mixed) m) {
			return (m->table||"") + "." + m->name;
		      }) +
                  fields->name);

    if (case_convert)
      fieldnames = map(fieldnames, lower_case);


    while (row = res_obj->fetch_row())
      res += ({ mkmapping(fieldnames, row + row) });

    return res;
  }
  return 0;
}

//! Return last error message.
int|string error()
{
  if (functionp (master_sql->error))
    return master_sql->error();
  return "Unknown error";
}

//! Select database to access.
void select_db(string db)
{
  master_sql->select_db(db);
}

//! Compiles the query (if possible). Otherwise returns it as is.
//! The resulting object can be used multiple times in query() and
//! big_query().
//!
//! @param q
//!   SQL-query to compile.
string|object compile_query(string q)
{
  if (functionp(master_sql->compile_query)) {
    return master_sql->compile_query(q);
  }
  return q;
}

//! Handle sprintf-based quoted arguments
private array(string|mapping(string|int:mixed))
  handle_extraargs(string query, array(mixed) extraargs) {

  array(mixed) args=allocate(sizeof(extraargs));
  mapping(string|int:mixed) b = ([]);

  int a;
  foreach(extraargs; int j; mixed s) {
    if (stringp(s) || multisetp(s)) {
      args[j]=":arg"+(a++);
      b[args[j]] = s;
      continue;
    }
    if (intp(s) || floatp(s)) {
      args[j]=s;
      continue;
    }
    ERROR("Wrong type to query argument #"+(j+1)+".\n");
  }
  if(!sizeof(b)) b=0;

  return ({sprintf(query,@args), b});
}

//!   Send an SQL query to the underlying SQL-server.
//! @param q
//!   Query to send to the SQL-server. This can either be a string with the
//!   query, or a previously compiled query (see compile_query()).
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
//! mixed err = catch {
//!   query("SELECT foo FROM bar WHERE gazonk=:baz",
//!     ([":baz":"value"]));
//! };
//! if(err)
//!   werror("An error occured.");
//! @endcode
//!
//!     Binary values (BLOBs) may need to be placed in multisets.
//!
//!     @item
//!     Arguments as you would use in sprintf. They are automatically
//!     quoted.
//!
//! @code
//! query("select foo from bar where gazonk=%s","value") )
//! @endcode
//!   @endol
//!
//! @returns
//!   Returns one of the following on success:
//!   @mixed
//!     @type array(mapping(string:string))
//!       The result as an array of mappings indexed on the name
//!       of the columns
//!     @type zero
//!       The value @expr{0@} (zero) if the query didn't return any
//!       result (eg @tt{INSERT@} or similar).
//!   @endmixed
//!
//! @throws
//!   Throws an exception if the query fails.
//!
//! @seealso
//!   @[big_query]
array(mapping(string:mixed)) query(object|string q,
                                   mixed ... extraargs)
{
  if (sizeof(extraargs)) {
    mapping(string|int:mixed) bindings;

    if (mappingp(extraargs[0]))
      bindings=extraargs[0];
    else
      [q,bindings]=handle_extraargs(q,extraargs);

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

//! Send an SQL query to the underlying SQL-server. The result is
//! returned as an Sql.sql_result object. This allows for having some
//! more info about the result as well as processing the result in a
//! streaming fashion, although the result itself wasn't obtained
//! streamingly from the server. Returns @expr{0@} if the query didn't
//! return any result (e.g. INSERT or similar). For the other
//! arguments, they are the same as for the @[query()] function.
//!
//! @seealso
//!   @[query], @[streaming_query]
int|object big_query(object|string q, mixed ... extraargs)
{
  object|array(mapping) pre_res;

  switch( sizeof(extraargs) ) {

  default:
    mapping(string|int:mixed) bindings;

    if (mappingp(extraargs[0]))
      bindings=extraargs[0];
    else
      [q,bindings]=handle_extraargs(q,extraargs);

    if(bindings) {
      if(master_sql->big_query)
	pre_res = master_sql->big_query(q, bindings);
      else
	pre_res = master_sql->query(q, bindings);
      break;
    }
    // Fallthrough

  case 0:
    if (master_sql->big_query)
      pre_res = master_sql->big_query(q);
    else
      pre_res = master_sql->query(q);
    break;
  }

  if(pre_res) {
    if(objectp(pre_res))
      return .sql_object_result(pre_res);
    else
      return .sql_array_result(pre_res);
  }
  return 0;
}

//! Send an SQL query to the underlying SQL-server. The result is
//! returned as a streaming Sql.sql_result object. This allows for
//! having results larger than the available memory, and returning
//! some more info about the result. Returns @expr{0@} if the query
//! didn't return any result (e.g. INSERT or similar). For the other
//! arguments, they are the same as for the @[query()] function.
//!
//! @seealso
//!   @[big_query]
int|object streaming_query(object|string q, mixed ... extraargs)
{
  if(!master_sql->streaming_query) return 0;
  object pre_res;

  switch( sizeof(extraargs) ) {

  default:
    mapping(string|int:mixed) bindings;

    if(mappingp(extraargs[0]))
      bindings=extraargs[0];
    else
      [q,bindings]=handle_extraargs(q,extraargs);

    if(bindings) {
      pre_res = master_sql->streaming_query(q,bindings);
      break;
    }
    // Fallthough

  case 0:
    pre_res = master_sql->streaming_query(q);
    break;
  }

  if(pre_res)
    return .sql_object_result(pre_res);
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
