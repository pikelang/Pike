/*
 * Sybase driver for the Pike programming language.
 */

#pike __REAL_VERSION__
#require constant(sybase.sybase)

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
optional constant dont_dump_program = 1;

inherit sybase.sybase:mo;

string server_info () {
  return "sybase/10.X or 11.X";
}

void create(string host = "", string db = "", string user = "",
	    string _pass = "", void|mapping options) {
  string pass = _pass;
  _pass = "CENSORED";
  mo::create(host, db, user, pass, options);
  if (sizeof(db))
    mo::big_query("use " + db);
}

variant __builtin.Sql.Result big_query(string q) {
  return mo::big_query(q);
}
