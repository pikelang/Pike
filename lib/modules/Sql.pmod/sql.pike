/*
 * $Id: sql.pike,v 1.2 1997/03/28 12:32:36 grubba Exp $
 *
 * Implements the generic parts of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#define throw_error(X)	throw(({ (X), backtrace() }))

import Array;
import Simulate;

object master_sql;

void create(void|string|object host, void|string db,
	    void|string user, void|string password)
{
  if (objectp(host)) {
    master_sql = host;
    if ((user && user != "") || (password && password != "")) {
      throw_error("Sql(): Only the database argument is supported when "
		  "first argument is an object\n");
    }
    if (db && db != "") {
      master_sql->select_db(db);
    }
    return;
  } else {
    foreach(get_dir(Sql->dirname), string program_name) {
      if (sizeof(program_name / "_result") == 1 &&
	  (program_name != "sql.pike")) {
	/* Don't call ourselves... */
	array(mixed) err;
	
	err = catch {
	  program p = Sql[program_name];

	  if (password && password != "") {
	    master_sql = p(host||"", db||"", user||"", password);
	  } else if (user && user != "") {
	    master_sql = p(host||"", db||"", user);
	  } else if (db && db != "") {
	    master_sql = p(host||"", db);
	  } else if (host && host != "") {
	    master_sql = p(host);
	  } else {
	    master_sql = p();
	  }
	  return;
	};
      }
    }
  }

  throw_error("Sql(): Couldn't connect to database\n");
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
      return(m->name);	/* Hope this is unique */
    } );

    while (row = res_obj->fetch_row()) {
      res += ({ mkmapping(fieldnames, row) });
    }
    return(res);
  } else {
    return(0);
  }
}
  
int|string error()
{
  return(master_sql->error());
}

void select_db(string db)
{
  master_sql->select_db(db);
}

array(mapping(string:mixed)) query(string s)
{
  object res_obj;

  if (functionp(master_sql->query)) {
    return(master_sql->query(s));
  }
  return(res_obj_to_array(master_sql->big_query(s)));
}

object big_query(string q)
{
  if (functionp(master_sql->big_query)) {
    return(Sql.sql_result(master_sql->big_query(q)));
  }
  return(Sql.sql_result(master_sql->query(q)));
}

void create_db(string db)
{
  master_sql->create_db(db);
}

void drop_db(string db)
{
  master_sql->drop_db(db);
}

void shutdown()
{
  if (functionp(master_sql->shutdown)) {
    master_sql->shutdown();
  } else {
    throw_error("sql->shutdown(): Not supported by this database\n");
  }
}

void reload()
{
  if (functionp(master_sql->reload)) {
    master_sql->reload();
  } else {
    /* Probably safe to make this a NOOP */
  }
}

string server_info()
{
  if (functionp(master_sql->server_info)) {
    return(master_sql->server_info());
  }
  return("Unknown SQL-server");
}

string host_info()
{
  if (functionp(master_sql->host_info)) {
    return(master_sql->host_info());
  } 
  return("Unknown connection to host");
}

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

