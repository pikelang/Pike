/*
 * $Id: dsn.pike,v 1.1 2005/01/26 18:57:45 grubba Exp $
 *
 * Glue for the ODBC-module
 *
 * Carl Grubbström 2005-01-26
 */

#pike __REAL_VERSION__

#if constant(Odbc.odbc)
inherit Odbc.odbc;

void create(string|void host, string|void db, string|void user,
	    string|void password, mapping(string:int|string)|void options)
{
  string connectstring="";
  if(user)
    connectstring+="uid="+user+";";
  if(password)
    connectstring+="pwd="+password+";";
  if(host)
    if(sizeof(host)>0)
      connectstring+="dsn="+host+";";
  if(db)
    if(sizeof(db)>0)
    {
      if (has_suffix(lower_case(db), ".dsn")) // check if file is a dsn file
        connectstring+="filedsn="+db+";";
      else
        connectstring+="dbq="+db+";";
    }
  if(mappingp(options))
  {
    foreach(indices(options),string ind)
    {
      connectstring+=ind+"="+options[ind]+";";
    }
  }
  ::create_dsn(connectstring);
}

int|object big_query(object|string q, mapping(string|int:mixed)|void bindings)
{  
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q, bindings, this));
}

constant list_dbs = Odbc.list_dbs;

#else
constant this_program_does_not_exist=1;
#endif /* constant(Odbc.odbc) */
