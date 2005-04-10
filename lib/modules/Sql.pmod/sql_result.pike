/*
 * $Id: sql_result.pike,v 1.13 2005/04/10 03:29:40 nilsson Exp $
 *
 * Implements the generic result module of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#pike __REAL_VERSION__

//! Implements the generic result of the SQL-interface.
//! Used for return results from SQL.sql->big_query().

//! The actual result.
mixed master_res;

//! This is the number of the current row. (0 <= @[index] < @[num_rows()])
int index;

//! Create a new Sql.sql_result object
//!
//! @param res
//!   Result to use as base.
static void create(mixed res);

static string _sprintf(int type, mapping|void flags)
{
  int f;
  return type=='O' && sprintf("%O(/* row %d/%d, %d field%s */)",
			      this_program, index, num_rows(),
			      f=num_fields(), f>1?"s":"");
}

//! Returns the number of rows in the result.
int num_rows();

//! Returns the number of fields in the result.
int num_fields()
{
  return master_res->num_fields();
}

//! Returns non-zero if there are no more rows.
int eof();

//! Return information about the available fields.
array(mapping(string:mixed)) fetch_fields();

//! Skip past a number of rows.
//!
//! @param skip
//!   Number of rows to skip.
void seek(int skip) {
  if(skip<0) error("Skip argument not positive\n");
  while(skip--) {
    index++;
    master_res->fetch_row();
  }
}

//! Fetch the next row from the result.
int|array(string|int) fetch_row();
