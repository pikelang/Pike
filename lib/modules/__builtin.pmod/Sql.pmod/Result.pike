/*
 * Implements the generic result module of the SQL-interface
 *
 * Henrik Grubbström 1996-01-09
 */

#pike __REAL_VERSION__

//! Base class for results for the SQL-interface.
//!
//! Used for return results from Sql.Connection->big_query().

//! This is the number of the current row. The actual semantics
//! differs between different databases.
int index;

//! Increment the @[index].
//!
//! @param val
//!   Value to increment the @[index] with. Defaults to @expr{1@}.
//!
//! @returns
//!   Returns the new value of the @[index].
//!
//! This is a helper function for implementations to update the @[index].
//!
//! It is typically called from @[fetch_row()].
protected int increment_index(int|void val)
{
  index += val || undefinedp(val);
  return index;
}

//! Create a new Sql.Result object
//!
//! @param res
//!   Result to use as base.
protected void create(mixed res);

protected string _sprintf(int type, mapping|void flags)
{
  int f = num_fields();
  catch( int r = num_rows() );
  int e = eof();
  return type=='O' &&
    sprintf("%O(/* row %d/%s, %d field%s */)",
	    this_program, index,
	    (!r || index==r && !e)?"?":(string)num_rows(),
	    f = num_fields(), f!=1?"s":"");
}

//! Getter for the actual result object.
//!
//! @returns
//!   Returns the current object.
//!
//! @note
//!   Obsoleted in Pike 8.1 due to the wrapper class no longer
//!   existing, and this symbol thus essentially being a noop..
//!
//! @deprecated __builtin.Sql.Result.
__deprecated__ array|this_program `master_res()
{
  return this;
}

//! @returns
//!  The number of affected rows by this query.
//!
//! @seealso
//!  @[status_command_complete()], @[num_rows()]
int affected_rows() {
  return 0;
}

//! @returns
//!  The command-complete status for this query.
//!
//! @seealso
//!  @[affected_rows()]
string status_command_complete() {
  return "";
}

//! @returns
//!  The number of rows in the result.
//!
//! @note
//!  Depending on the database implementation, this number
//!  can still increase between subsequent calls if the results from
//!  the query are not complete yet.  This function is only guaranteed to
//!  return the correct count after EOF has been reached.
//!
//! @seealso
//!  @[affected_rows()], @[eof()]
int num_rows();

//! @returns
//!  The number of fields in the result.
int num_fields();

//! @returns
//!  Non-zero if there are no more rows.
int eof();

//! @returns
//!  Information about the available fields.
array(mapping(string:mixed)) fetch_fields();

//! Skip past a number of rows.
//!
//! @param skip
//!   Number of rows to skip.
void seek(int skip) {
  if(skip<0)
    error("Cannot seek to negative result indices\n");
  while(skip--)
    fetch_row();
}

//! Fetch the next row from the result.
//!
//! @returns
//!   Returns an array with one element per field in the same order as
//!   reported by @[fetch_fields()]. See the @[Sql.Connection] class
//!   documentation for more details on how different data types are
//!   represented.
//!
//!   On EOF it returns @expr{0@}.
//!
//! @seealso
//!  @[fetch_row_array()],
//!  @[set_result_callback()], @[set_result_array_callback()]
array(mixed) fetch_row();

//! @returns
//!   Multiple result rows at a time (at least one).
//!   Every result row with one element per field in the same order as
//!   reported by @[fetch_fields()]. See the @[Sql.Connection] class
//!   documentation for more details on how different data types are
//!   represented.
//!
//!   On EOF it returns @expr{0@}.
//!
//! @seealso
//!  @[fetch_row()],
//!  @[set_result_callback()], @[set_result_array_callback()]
array(array(mixed)) fetch_row_array() {
  array row, ret = ({});
  while (row = fetch_row())
    ret += ({row});
  return sizeof(ret) && ret;
}

//! Sets up a callback for every row returned from the database.
//! First argument passed is the resultobject itself, second argument
//! is the result row (zero on EOF).
//!
//! @seealso
//!  @[fetch_row()], @[set_result_array_callback()]
void set_result_callback(
 function(this_program, array(mixed), mixed ... :void) callback,
 mixed ... args) {
  if (callback) {
    array row;
    do {
      row = fetch_row();
      callback(this, row, @args);
    } while (row);
  }
}

//! Sets up a callback for sets of rows returned from the database.
//! First argument passed is the resultobject itself, second argument
//! is the array of result rows (zero on EOF).
//!
//! @seealso
//!  @[fetch_row_array()], @[set_result_callback()]
void set_result_array_callback(
 function(this_program, array(array(mixed)), mixed ... :void) callback,
 mixed ... args) {
  if (callback) {
    array rows;
    do {
      rows = fetch_row_array();
      callback(this, rows, @args);
    } while (rows);
  }
}

//! Switch to the next set of results.
//!
//! Some databases support returning more than one set of results.
//! This function terminates the current result and switches to
//! the next (if any).
//!
//! @returns
//!   Returns the @[Result] object if there were more results,
//!   and @expr{0@} (zero) otherwise.
//!
//! @throws
//!   May throw the same errors as @[Sql.Connection()->big_query()] et al.
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
