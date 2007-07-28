/*
 * $Id: sql_result.pike,v 1.17 2007/07/28 18:27:32 jhs Exp $
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

//! This is the number of the current row. The actual semantics
//! differs between different databases.
int index;

//! Create a new Sql.sql_result object
//!
//! @param res
//!   Result to use as base.
static void create(mixed res);

static string _sprintf(int type, mapping|void flags)
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

// --- Iterator API

class _get_iterator
{
  static int|array(string|int) row = fetch_row();
  static int pos = 0;

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
