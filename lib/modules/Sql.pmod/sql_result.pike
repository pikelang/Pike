/*
 * $Id: sql_result.pike,v 1.1 1997/03/06 21:21:03 grubba Exp $
 *
 * Implements the generic result module of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#define throw_error(X)	throw(({ (X), backtrace() }))

object|array master_res;
int index;

import Array;

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


