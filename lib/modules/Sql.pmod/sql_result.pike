/*
 * Implements the generic result module of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#pike __REAL_VERSION__

//! Implements the generic result base class of the SQL-interface.
//! Used for return results from SQL.sql->big_query().
//!
//! You typically don't get a direct clone of this class,
//! but of a class that inherits it, like @[sql_array_result]
//! or @[sql_object_result].

inherit __builtin.Sql.Result;

//! The actual result.
array|__builtin.Sql.Result master_res;

//! Skip past a number of rows.
//!
//! @param skip
//!   Number of rows to skip.
void seek(int skip) {
  if(skip<0)
    error("Cannot seek to negative result indices\n");
  while(skip--) {
    increment_index(1);
    master_res->fetch_row();
  }
}

//! Fetch remaining result as @tt{JSON@}-encoded data.
string fetch_json_result()
{
  if (arrayp(master_res) || !master_res->fetch_json_result) {
    String.Buffer res = String.Buffer();
    res->putchar('[');
    array row;
    for (int i = 0; row = fetch_row(); i++) {
      if(i) res->putchar(',');
      encode_json(res, row);
    }
    res->putchar(']');
    return string_to_utf8(res->get());
  }
  index = num_rows();
  return master_res->fetch_json_result();
}
