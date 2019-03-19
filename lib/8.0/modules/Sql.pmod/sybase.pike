#pike 8.1
#require constant(sybase.sybase)

inherit Sql.sybase;

/*
 * Deprecated. Use connect(host,db,user,pass) instead.
 */
void select_db(string db)
{
  big_query("use "+db);
}

/*
 * Deprecated. Use an SQL command instead.
 */
void create_db (string dbname) {
  big_query("create database "+dbname);
}

/*
 * Deprecated. Use an SQL command instead.
 */
void drop_db (string dbname) {
  big_query("drop database "+dbname);
}
