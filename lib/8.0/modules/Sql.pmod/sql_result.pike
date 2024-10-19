/*
 * Implements the generic result module of the SQL-interface
 *
 * Henrik Grubbstr�m 1996-01-09
 */

#pike 8.1

#pragma no_deprecation_warnings

//! Implements the generic result base class of the SQL-interface.
//! Used for return results from SQL.sql->big_query().
//!
//! You typically don't get a direct clone of this class,
//! but of a class that inherits it, like @[sql_array_result]
//! or @[sql_object_result].

//! The actual result.
mixed master_res;

//! This is the number of the current row. The actual semantics
//! differs between different databases.
int index;

//! Create a new Sql.sql_result object
//!
//! @param res
//!   Result to use as base.
protected void create(mixed res);

protected string _sprintf(int type, mapping|void flags)
{
  int f = num_fields();
  catch( int r = num_rows() );
  int e = eof();
  return type=='O' && master_res &&
    sprintf("%O(/* row %d/%s, %d field%s */)",
	    this_program, index,
	    (!r || index==r && !e)?"?":(string)num_rows(),
	    f = num_fields(), f!=1?"s":"");
}

//! Returns the number of rows in the result.
int num_rows();

//! Returns the number of fields in the result.
int num_fields();

//! Returns non-zero if there are no more rows.
int eof();

//! Return information about the available fields.
array(mapping(string:mixed)) fetch_fields();

//! Skip past a number of rows.
//!
//! @param skip
//!   Number of rows to skip.
void seek(int skip) {
  if(skip<0)
    error("Cannot seek to negative result indices\n");
  while(skip--) {
    index++;
    master_res->fetch_row();
  }
}

//! Fetch the next row from the result.
//!
//! @returns
//!   Returns an array with one element per field in the same order as
//!   reported by @[fetch_fields()]. See the @[Sql.Sql] class
//!   documentation for more details on how different data types are
//!   represented.
int|array(string|int|float) fetch_row();

//! Switch to the next set of results.
//!
//! Some databases support returning more than one set of results.
//! This function terminates the current result and switches to
//! the next (if any).
//!
//! @returns
//!   Returns the @[sql_result] object if there were more results,
//!   and @expr{0@} (zero) otherwise.
//!
//! @throws
//!   May throw the same errors as @[Sql.Sql()->big_query()] et al.
this_program next_result();

// --- Iterator API

class _get_iterator
{
  protected int|array(string|int) row = fetch_row();
  protected int pos = 0;

  int index()
  {
    return pos;
  }

  int|array(string|int) value()
  {
    return row;
  }

  int(0..1) next()
  {
    pos++;
    return !!(row = fetch_row());
  }

  this_program `+=(int steps)
  {
    if(!steps) return this;
    if(steps<0) error("Iterator must advance a positive numbe of steps.\n");
    if(steps>1)
    {
      pos += steps-1;
      seek(steps-1);
    }
    next();
    return this;
  }

  int(0..1) `!()
  {
    return eof();
  }

  int _sizeof()
  {
    return num_fields();
  }
}

protected void encode_json(String.Buffer res, mixed msg)
{
  if (stringp(msg)) {
    res->add("\"", replace(msg, ([ "\"" : "\\\"",
                                   "\\" : "\\\\",
                                   "\n" : "\\n",
                                   "\b" : "\\b",
                                   "\f" : "\\f",
                                   "\r" : "\\r",
                                   "\t" : "\\t" ])), "\"");
  } else if (arrayp(msg)) {
    res->putchar('[');
    foreach(msg; int i; mixed val) {
      if (i) res->putchar(',');
      encode_json(res, val);
    }
    res->putchar(']');
  } else if (mappingp(msg)) {
    // NOTE: For reference only, not currently reached.
    res->putchar('{');
    foreach(sort(indices(msg)); int i; string ind) {
      if (i) res->putchar(',');
      encode_json(res, ind);
      res->putchar(':');
      encode_json(res, msg[ind]);
    }
    res->putchar('}');
  } else {
    res->add((string)msg);
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
