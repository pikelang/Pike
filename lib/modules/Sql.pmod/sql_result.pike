/*
 * $Id: sql_result.pike,v 1.2 1997/03/30 19:07:41 grubba Exp $
 *
 * Implements the generic result module of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

//.
//. File:	sql_result.pike
//. RCSID:	$Id: sql_result.pike,v 1.2 1997/03/30 19:07:41 grubba Exp $
//. Author:	Henrik Grubbström (grubba@infovav.se)
//.
//. Synopsis:	Implements the generic result of the SQL-interface.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. Used to return results from SQL.sql->big_query().
//.

#define throw_error(X)	throw(({ (X), backtrace() }))

//. + master_res
//.   The actual result.
object|array master_res;

//. + index
//.   If the result was an array, this is the current row.
int index;

import Array;

//. - create
//.   Create a new Sql.sql_result object
//. > res
//.   Result to use as base.
void create(object|array res)
{
  if (!(master_res = res) || (!arrayp(res) && !objectp(res))) {
    throw_error("Bad arguments to Sql_result()\n");
  }
  index = 0;
}

//. - num_rows
//.   Returns the number of rows in the result.
int num_rows()
{
  if (arrayp(master_res)) {
    return(sizeof(master_res));
  }
  return(master_res->num_rows());
}

//. - num_fields
//.   Returns the number of fields in the result.
int num_fields()
{
  if (arrayp(master_res)) {
    return(sizeof(master_res[0]));
  }
  return(master_res->num_fields());
}

//. - eof
//.   Returns non-zero if there are no more rows.
int eof()
{
  if (arrayp(master_res)) {
    return(index >= sizeof(master_res));
  }
  return(master_res->eof());
}

//. - fetch_fields
//.   Return information about the available fields.
array(mapping(string:mixed)) fetch_fields()
{
  if (arrayp(master_res)) {
    /* Only supports the name field */
    array(mapping(string:mixed)) res = allocate(sizeof(master_res));
    int index = 0;
    
    foreach(sort(indices(master_res)), string name) {
      res[index++] = ([ "name": name ]);
    }
    return(res);
  }
  return(master_res->fetch_fields());
}

//. - seek
//.   Skip past a number of rows.
//. > skip
//.   Number of rows to skip.
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

//. - fetch_row
//.   Fetch the next row from the result.
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


