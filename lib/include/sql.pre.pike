/*
 * $Id: sql.pre.pike,v 1.1 1997/01/09 23:38:58 grubba Exp $
 *
 * Implements the generic parts of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#define throw_error(X)	throw(({ (X), backtrace() }))

class sql_result {
  object|array master_res;
  int index;

  void create(object|array res)
  {
    if (!(master_res = res) || (!arrayp(res) && !objectp(res))) {
      throw_error("Bad arguments to Sql_result()\n");
    }
    index = 0;
  }
  int num_rows()
  {
    if (arrayp(master_res)) {
      return(sizeof(master_res));
    }
    return(master_res->num_rows());
  }
  int num_fields()
  {
    if (arrayp(master_res)) {
      return(sizeof(master_res[0]));
    }
    return(master_res->num_fields());
  }
  int eof()
  {
    if (arrayp(master_res)) {
      return(index >= sizeof(master_res));
    }
    return(master_res->eof());
  }
  array(mapping(string:mixed)) fetch_fields()
  {
    if (arrayp(master_res)) {
      /* This isn't very efficient */
      /* Only supports the name field */
      array(mapping(string:mixed)) res = ({});
      foreach(sort(indices(master_res)), string name) {
	res += ({ "name", name });
      }
      return(res);
    }
    return(master_res->fetch_fields);
  }
  void seek(int skip)
  {
    if (skip < 0) {
      throw_error("seek(): Argument 1 not positive\n");
    }
    if (arrayp(master_res)) {
      index += skip;
    } else if (functionp(master_res->seek)) {
      master_res->seek(index);
    } else {
      while (skip--) {
	master_res->fetch_row();
      }
    }
  }
  int|array(string|int) fetch_row()
  {
    if (arrayp(master_res)) {
      array res;

      if (index >= sizeof(master_res)) {
	return(0);
      }
      sort(indices(master_res[index]), res = values(master_res[index]));
      return(res);
    }
    return (master_res->fetch_row());
  }
}

class sql {
  object master_sql;

  void create(void|string|program|object kind,
	      void|string|object host, void|string db,
	      void|string user, void|string password)
  {
    if (kind == "") {
      kind = 0;
    }
    if (objectp(kind)) {
      master_sql = kind;
      if ((host && host != "") || (user && user != "") ||
	  (password && password != "")) {
	throw_error("Sql(): Only the database argument is supported when "
		    "first argument is an object\n");
      }
      if (db && db != "") {
	master_sql->select_db(db);
      }
      return;
    } else if (stringp(kind)) {
      program p;

      foreach (({ "", "/precompiled/sql/", "/precompiled/" }), string prefix) {
	if (p = (program)(prefix + kind)) {
	  break;
	}
      }
      if (p) {
	kind = p;
      } else {
	throw_error("Sql(): Unsupported sql-type " + kind + "\n");
      }
    } else if (!kind) {
      foreach(regexp(indices(master()->programs), "/precompiled/sql/.*"),
	      string program_name) {
	if (sizeof(program_name / "_result") == 1) {
	  array(mixed) err;

	  err = catch {
	    master_sql = ((program)program_name)(host||"", db||"",
						 user||"", password||"");
	  };
	  if (err) {
	    werror(err[0]);
	    describe_backtrace(err[1]);
	  }
	  if (master_sql) {
	    return;
	  } 
	}
      }
    }
    if (programp(kind)) {
      if (master_sql = kind(host, db, user, password)) {
	return;
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
  
  string error()
  {
    if (functionp(master_sql->error)) {
      return(master_sql->error());
    }
    return("");
  }
  void select_db(string db) {
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
      return(((program)"/precompiled/sql_result")(master_sql->big_query(q)));
    }
    return(((program)"/precompiled/sql_result")(master_sql->query(q)));
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
  array(mapping(string:mixed)) list_dbs(string|void wild)
  {
    if (functionp(master_sql->list_dbs)) {
      array(mapping(string:mixed))|object res;
      if (objectp(res = master_sql->list_dbs(wild||""))) {
	return(res_obj_to_array(res));
      }
      return(res);
    }
    if (wild) {
      return(query("show databases like \'" + wild +"\'"));
    }
    return(query("show databases"));
  }
  array(mapping(string:mixed)) list_tables(string|void wild)
  {
    if (functionp(master_sql->list_tables)) {
      array(mapping(string:mixed))|object res;
      if (objectp(res = master_sql->list_tables(wild||""))) {
	return(res_obj_to_array(res));
      }
      return(res);
    }
    if (wild) {
      return(query("show tables like \'" + wild + "\'"));
    }
    return(query("show tables"));
  }
  array(mapping(string:mixed)) list_fields(string table, string|void wild)
  {
    if (functionp(master_sql->list_fields)) {
      array(mapping(string:mixed))|object res;
      if (objectp(res = master_sql->list_fields(wild||""))) {
	return(res_obj_to_array(res));
      }
      return(res);
    }
    if (wild) {
      return(query("show fields in \'" + table + "\' like \'" + wild + "\'"));
    }
    return(query("show fields in \'" + table + "\'"));
  }
}

void create()
{
  master()->add_precompiled_program("/precompiled/sql", sql);
  master()->add_precompiled_program("/precompiled/sql_result", sql_result);
}
