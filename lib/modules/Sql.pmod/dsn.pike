/*
 * Glue for the ODBC-module
 *
 * Carl Grubbström 2005-01-26
 */

#pike __REAL_VERSION__
#require constant(Odbc.odbc)

// Cannot dump this since the #require check below depend on the
// presence of system libs at runtime.
optional constant dont_dump_program = 1;

inherit Odbc.odbc;

void create(string|void host, string|void db, string|void user,
	    string|void password, mapping(string:int|string)|void options)
{
  // FIXME: Quoting?
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

variant Sql.Result big_query(object|string q)
{
  return ::big_query(q);
}
