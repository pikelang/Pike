/*
 * Sybase driver for the Pike programming language.
 * By Francesco Chemolli <kinkie@roxen.com> 10/12/1999
 *
 * $Id: sybase.pike,v 1.9 2004/04/14 20:20:35 nilsson Exp $
 *
 */

#pike __REAL_VERSION__

#if constant(sybase)

inherit sybase.sybase:mo;
#define THROW(X) throw(({X+"\n",backtrace()}))


/*
 * Deprecated. Use connect(host,db,user,pass) instead.
 */
void select_db(string db)
{
  mo::big_query("use "+db);
}

/*
 * Deprecated. Use an SQL command instead.
 */
void create_db (string dbname) {
  mo::big_query("create database "+dbname);
}

/*
 * Deprecated. Use an SQL command instead.
 */
void drop_db (string dbname) {
  mo::big_query("drop database "+dbname);
}

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
  THROW("Unsupported");
}

/*
 * Unimplemented. PLEASE tell me somebody knows how to do this.
 * There MUST be some system stored procedure...
 */
array(string) list_tables(string|void wild) {
  THROW("Unsupported");
}

/*
 * Unimplemented.
 */
array(string) list_fields(string|void wild) {
  THROW("Unsupported");
}

int num_rows() {
  THROW("Unsupported by the DB server");
}

void seek(int skipthismany) {
  if (skipthismany<0)
    THROW("Negative skips are not supported");
  if (!skipthismany)
    return;
  while (skipthismany && fetch_row()){
    skipthismany--;
  }
}

void create(void|string host, void|string db, void|string user,
            void|string pass) {
  mo::create(host||"",db||"",user||"",pass||"");
  if (db && stringp(db) && sizeof(db)) {
    mo::big_query("use "+db);
  }
}

int|object big_query(string q, mapping(string|int:mixed)|void bindings) {
  if (!bindings)
    return ::big_query(q);
  return ::big_query(.sql_util.emulate_bindings(q,bindings,this));
}

#else
constant this_program_does_not_exist=1;
#endif
