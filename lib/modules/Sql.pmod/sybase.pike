/*
 * Sybase driver for the Pike programming language.
 * By Francesco Chemolli <kinkie@roxen.com> 10/12/1999
 *
 */

#pike __REAL_VERSION__
#require constant(sybase.sybase)

// Cannot dump this since the #require check may depend on the
// presence of system libs at runtime.
constant dont_dump_program = 1;

inherit sybase.sybase:mo;

void shutdown() {
  catch { //there _will_ be an error. It's just that we don't care about it.
    mo::big_query("shutdown");
  };
}

string server_info () {
  return "sybase/10.X or 11.X";
}

/*
 * Unimplemented. Anyone knows Transact-SQL well enough?
 * maybe we could use the connection properties otherwise (CS_HOSTNAME)
 */
string host_info() {
  return "unknown";
}

/*
 * Unimplemented. Anyone knows Transact-SQL well enough?
 */
array(string) list_dbs(string|void wild) {
  error("Unsupported.\n");
}

/*
 * Unimplemented. PLEASE tell me somebody knows how to do this.
 * There MUST be some system stored procedure...
 */
array(string) list_tables(string|void wild) {
  error("Unsupported.\n");
}

/*
 * Unimplemented.
 */
array(string) list_fields(string|void wild) {
  error("Unsupported.\n");
}

int num_rows() {
  error("Unsupported by the DB server.\n");
}

void seek(int skipthismany) {
  if (skipthismany<0)
    error("Negative skips are not supported.\n");
  if (!skipthismany)
    return;
  while (skipthismany && fetch_row()){
    skipthismany--;
  }
}

void create(void|string host, void|string db, void|string user,
	    void|string _pass, void|mapping options) {
  string pass = _pass;
  _pass = "CENSORED";
  mo::create(host||"",db||"",user||"",pass||"",options);
  if (db && stringp(db) && sizeof(db)) {
    mo::big_query("use "+db);
  }
}

int|object big_query(string q, mapping(string|int:mixed)|void bindings) {
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q,bindings,this));
}
