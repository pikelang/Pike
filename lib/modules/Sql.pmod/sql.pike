/*
 * $Id: sql.pike,v 1.17 1998/03/19 19:55:25 grubba Exp $
 *
 * Implements the generic parts of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

//.
//. File:	sql.pike
//. RCSID:	$Id: sql.pike,v 1.17 1998/03/19 19:55:25 grubba Exp $
//. Author:	Henrik Grubbström (grubba@infovav.se)
//.
//. Synopsis:	Implements the generic parts of the SQL-interface.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. Implements those functions that need not be present in all SQL-modules.
//.

#define throw_error(X)	throw(({ (X), backtrace() }))

import Array;
import Simulate;

//. + master_sql
//.   Object to use for the actual SQL-queries.
object master_sql;

//. + case_convert
//.   Convert all field names in mappings to lower_case.
//.   Only relevant to databases which only implement big_query(),
//.   and use upper/mixed-case fieldnames (eg Oracle).
//.   0 - No (default)
//.   1 - Yes
int case_convert;

//. - quote
//.   Quote a string so that it can safely be pu in a query.
//. > s - String to qoute.
string quote(string s)
{
  if (master_sql && master_sql->quote) {
    return(master_sql->quote(s));
  }
  return(replace(s, "\'", "\'\'"));
}

//. - create
//.   Create a new generic SQL object.
//. > host
//.   object - Use this object to access the SQL-database.
//.   string - Connect to the server specified.
//.            The string should be on the format:
//.              [dbtype://][user[:password]@]hostname[:port][/database]
//.            If dbtype isn't specified, use any available database server
//.            on the specified host.
//.            If the hostname is "", access through a UNIX-domain socket or
//.            similar.
//.   zero   - Access through a UNIX-domain socket or similar.
//. > database
//.   Select this database.
//. > user
//.   User name to access the database as.
//. > password
//.   Password to access the database.
void create(void|string|object host, void|string db,
	    void|string user, void|string password)
{
  if (objectp(host)) {
    master_sql = host;
    if ((user && user != "") || (password && password != "")) {
      throw_error("Sql.sql(): Only the database argument is supported when "
		  "first argument is an object\n");
    }
    if (db && db != "") {
      master_sql->select_db(db);
    }
    return;
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

  array(string) program_names;

  if (host && (host != replace(host, ({ ":", "/", "@" }), ({ "", "", "" })))) {

    // The hostname is on the format:
    //
    // [dbtype://][user[:password]@]hostname[:port][/database]

    array(string) arr = host/"://";
    if ((sizeof(arr) > 1) && (arr[0] !="")) {
      if (sizeof(arr[0]/".pike") > 1) {
	program_names = ({ arr[0] });
      } else {
	program_names = ({ arr[0]+".pike" });
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
      if (!db) {
	db = arr[1..]*"/";
	host = arr[0];
      }
    }
  }

  if (host == "") {
    host = 0;
  }

  foreach(program_names || get_dir(Sql->dirname), string program_name) {
    if ((sizeof(program_name / "_result") == 1) &&
	(sizeof(program_name / ".pike") > 1) &&
	(program_name[..7] != "sql.pike")) {
      /* Don't call ourselves... */
      array(mixed) err;
      
      err = catch {
	program p;
#ifdef PIKE_SQL_DEBUG
	err = catch {p = Sql[program_name];};
#else /* !PIKE_SQL_DEBUG */
	// Ignore compiler errors for the various sql-modules,
	// since we might not have some.
	// This is NOT a nice way to do it, but...
	mixed old_inhib = master()->inhibit_compile_errors;
	master()->inhibit_compile_errors = lambda(){};
	err = catch {p = Sql[program_name];};
	// Restore compiler errors mode to whatever it was before.
	master()->inhibit_compile_errors = old_inhib;
#endif /* PIKE_SQL_DEBUG */
	if (err) {
#ifdef PIKE_SQL_DEBUG
	  Stdio.stderr->write(sprintf("Sql.sql(): Failed to compile module Sql.%s (%s)\n",
				      program_name, err[0]));
#endif /* PIKE_SQL_DEBUG */
	  if (program_names) {
	    throw(err);
	  } else {
	    throw(0);
	  }
	}

	if (p) {
	  err = catch {
	    if (password) {
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
	    return;
	  };
	  if (err) {
	    if (program_names) {
	      throw(err);
	    }
#ifdef PIKE_SQL_DEBUG
	    Stdio.stderr->write(sprintf("Sql.sql(): Failed to connect using module Sql.%s (%s)\n",
					program_name, err[0]));
#endif /* PIKE_SQL_DEBUG */
	  }
	} else {
	  if (program_names) {
	    throw(({ sprintf("Sql.sql(): Failed to index module Sql.%s\n",
			     program_name), backtrace() }));
	  }
#ifdef PIKE_SQL_DEBUG
	  Stdio.stderr->write(sprintf("Sql.sql(): Failed to index module Sql.%s\n",
				      program_name));
#endif /* PIKE_SQL_DEBUG */
	}
      };
      if (err && program_names) {
	throw(err);
      }
    }
  }

  if (!program_names) {
    throw_error("Sql.sql(): Couldn't connect using any of the databases\n");
  } else {
    throw_error("Sql.sql(): Couldn't connect using the " +
		(program_names[0]/".pike")[0] + " database\n");
  }
}

static private array(mapping(string:mixed)) res_obj_to_array(object res_obj)
{
  if (res_obj) {
    /* Not very efficient, but sufficient */
    array(mapping(string:mixed)) res = ({});
    array(string) fieldnames;
    array(mixed) row;
      
    fieldnames = map(res_obj->fetch_fields(),
		     lambda (mapping(string:mixed) m) {
      if (case_convert) {
	return(lower_case(m->name));	/* Hope this is even more unique */
      } else {
	return(m->name);		/* Hope this is unique */
      }
    } );

    while (row = res_obj->fetch_row()) {
      res += ({ mkmapping(fieldnames, row) });
    }
    return(res);
  } else {
    return(0);
  }
}

//. - error
//.   Return last error message.  
int|string error()
{
  return(master_sql->error());
}

//. - select_db
//.   Select database to access.
void select_db(string db)
{
  master_sql->select_db(db);
}

//. - compile_query
//.   Compiles the query (if possible). Otherwise returns it as is.
//.   The resulting object can be used multiple times in query() and
//.   big_query().
//. > q
//.   SQL-query to compile.
string|object compile_query(string q)
{
  if (functionp(master_sql->compile_query)) {
    return(master_sql->compile_query(q));
  }
  return(q);
}

//. - query
//.   Send an SQL query to the underlying SQL-server. The result is returned
//.   as an array of mappings indexed on the name of the columns.
//.   Returns 0 if the query didn't return any result (e.g. INSERT or similar).
//. > q
//.   Query to send to the SQL-server. This can either be a string with the
//.   query, or a previously compiled query (see compile_query()).
array(mapping(string:mixed)) query(object|string q)
{
  object res_obj;

  if (functionp(master_sql->query)) {
    return(master_sql->query(q));
  }
  return(res_obj_to_array(master_sql->big_query(q)));
}

//. - big_query
//.   Send an SQL query to the underlying SQL-server. The result is returned
//.   as a Sql.sql_result object. This allows for having results larger than
//.   the available memory, and returning some more info about the result.
//.   Returns 0 if the query didn't return any result (e.g. INSERT or similar).
//. > q
//.   Query to send to the SQL-server. This can either be a string with the
//.   query, or a previously compiled query (see compile_query()).
object big_query(object|string q)
{
  if (functionp(master_sql->big_query)) {
    return(Sql.sql_result(master_sql->big_query(q)));
  }
  return(Sql.sql_result(master_sql->query(q)));
}

//. - create_db
//.   Create a new database.
//. > db
//.   Name of database to create.
void create_db(string db)
{
  master_sql->create_db(db);
}

//. - drop_db
//.   Drop database
//. > db
//.   Name of database to drop.
void drop_db(string db)
{
  master_sql->drop_db(db);
}

//. - shutdown
//.   Shutdown a database server.
void shutdown()
{
  if (functionp(master_sql->shutdown)) {
    master_sql->shutdown();
  } else {
    throw_error("sql->shutdown(): Not supported by this database\n");
  }
}

//. - reload
//.   Reload the tables.
void reload()
{
  if (functionp(master_sql->reload)) {
    master_sql->reload();
  } else {
    /* Probably safe to make this a NOOP */
  }
}

//. - server_info
//.   Return info about the current SQL-server.
string server_info()
{
  if (functionp(master_sql->server_info)) {
    return(master_sql->server_info());
  }
  return("Unknown SQL-server");
}

//. - host_info
//.   Return info about the connection to the SQL-server.
string host_info()
{
  if (functionp(master_sql->host_info)) {
    return(master_sql->host_info());
  } 
  return("Unknown connection to host");
}

//. - list_dbs
//.   List available databases on this SQL-server.
//. > wild
//.   Optional wildcard to match against.
array(string) list_dbs(string|void wild)
{
  array(string)|array(mapping(string:mixed))|object res;
  
  if (functionp(master_sql->list_dbs)) {
    if (objectp(res = master_sql->list_dbs())) {
      res = res_obj_to_array(res);
    }
  } else {
    res = query("show databases");
  }
  if (sizeof(res) && mappingp(res[0])) {
    res = map(res, lambda (mapping m) {
      return(values(m)[0]);	/* Hope that there's only one field */
    } );
  }
  if (wild) {
    res = map_regexp(res, replace(wild, ({ "%", "_" }), ({ ".*", "." }) ));
  }
  return(res);
}

//. - list_tables
//.   List tables available in the current database.
//. > wild
//.   Optional wildcard to match against.
array(string) list_tables(string|void wild)
{
  array(string)|array(mapping(string:mixed))|object res;
  
  if (functionp(master_sql->list_tables)) {
    if (objectp(res = master_sql->list_tables())) {
      res = res_obj_to_array(res);
    }
  } else {
    res = query("show tables");
  }
  if (sizeof(res) && mappingp(res[0])) {
    res = map(res, lambda (mapping m) {
      return(values(m)[0]);	/* Hope that there's only one field */
    } );
  }
  if (wild) {
    res = map_regexp(res, replace(wild, ({ "%", "_" }), ({ ".*", "." }) ));
  }
  return(res);
}

//. - list_fields
//.   List fields available in the specified table
//. > table
//.   Table to list the fields of.
//. > wild
//.   Optional wildcard to match against.
array(mapping(string:mixed)) list_fields(string table, string|void wild)
{
  array(mapping(string:mixed))|object res;

  if (functionp(master_sql->list_fields)) {
    if (objectp(res = master_sql->list_fields(table))) {
      res = res_obj_to_array(res);
    }
    if (wild) {
      /* Not very efficient, but... */
      res = filter(res, lambda (mapping m, string re) {
	return(sizeof(map_regexp( ({ m->name }), re)));
      }, replace(wild, ({ "%", "_" }), ({ ".*", "." }) ) );
    }
    return(res);
  }
  if (wild) {
    res = query("show fields from \'" + table +
		"\' like \'" + wild + "\'");
  } else {
    res = query("show fields from \'" + table + "\'");
  }
  res = map(res, lambda (mapping m, string table) {
    foreach(indices(m), string str) {
      /* Add the lower case variants */
      string low_str = lower_case(str);
      if (low_str != str && !m[low_str]) {
	m[low_str] = m[str];
	m_delete(m, str);	/* Remove duplicate */
      }
    }
    if ((!m->name) && m->field) {
      m["name"] = m->field;
      m_delete(m, "field");	/* Remove duplicate */
    }
    if (!m->table) {
      m["table"] = table;
    }
    return(m);
  }, table);
  return(res);
}

