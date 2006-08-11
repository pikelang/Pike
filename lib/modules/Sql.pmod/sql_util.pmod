/*
 * $Id: sql_util.pmod,v 1.15 2006/08/11 11:08:44 grubba Exp $
 *
 * Some SQL utility functions.
 * They are kept here to avoid circular references.
 *
 * Henrik Grubbström 1999-07-01
 */

#pike __REAL_VERSION__

//! Some SQL utility functions

//! Quote a string so that it can safely be put in a query.
//!
//! @param s
//!   String to quote.
string quote(string s)
{
  return replace(s, "\'", "\'\'");
}

//! Throw an error in case an unimplemented function is called.
void fallback()
{
  error( "Function not supported in this database." );
}

//! Build a raw SQL query, given the cooked query and the variable bindings
//! It's meant to be used as an emulation engine for those drivers not
//! providing such a behaviour directly (i.e. Oracle).
//! The raw query can contain some variables (identified by prefixing
//! a colon to a name or a number (i.e. ":var" or  ":2"). They will be
//! replaced by the corresponding value in the mapping.
//!
//! @param query
//!   The query.
//!
//! @param bindings
//!   Optional mapping containing the variable bindings. Make sure that
//!   no confusion is possible in the query. If necessary, change the
//!   variables' names.
string emulate_bindings(string query, mapping(string|int:mixed)|void bindings,
                        void|object driver)
{
  array(string)k, v;
  if (!bindings)
    return query;
  function my_quote=(driver&&driver->quote?driver->quote:quote);
  v=map(values(bindings),
	lambda(mixed m) {
	  if(multisetp(m)) m = indices(m)[0];
	  return (stringp(m)? "'"+my_quote(m)+"'" : (string)m);
	});
  // Throws if mapping key is empty string.
  k=map(indices(bindings),lambda(string s){
			    return ( (stringp(s)&&s[0]==':') ?
				     s : ":"+s);
			  });
  return replace(query,k,v);
}

//! Result object wrapper performing utf8 decoding of all fields.
//!
//! Useful for eg Mysql connections which have been set to utf8-mode
//! using eg @expr{"SET NAMES 'utf8'"@}.
class UnicodeWrapper (
		      //! The wrapped result object.
		      static object master_result
		      )
{
  //! Returns the number of rows in the result.
  int num_rows()
  {
    return master_result->num_rows();
  }

  //! Returns the number of fields in the result.
  int num_fields()
  {
    return master_result->num_fields();
  }

  //! Returns @expr{1@} if there are no more rows in the result.
  int(0..1) eof()
  {
    return master_result->eof();
  }

  //! Cached @[fetch_fields()] result.
  static array(int|mapping(string:mixed)) field_info;

  //! Returns Information about the fields in the result.
  //!
  //! The following fields are converted from UTF8 if present:
  //! @mapping
  //!   @member string "name"
  //!     The name of the field. Always present.
  //!   @member string "table"
  //!     The table the field is from. Not present from all databases.
  //!   @member string "default"
  //!     The default value for the column. Not available from all databases.
  //! @endmapping
  array(int|mapping(string:mixed)) fetch_fields()
  {
    if (!field_info) {
      field_info = master_result->fetch_fields();
      foreach(field_info, int|mapping(string:mixed) field) {
	if (mappingp(field)) {
	  field->name = utf8_to_string(field->name);
	  if (field->table) {
	    field->table = utf8_to_string(field->table);
	  }
	  if (field->default) {
	    field->default = utf8_to_string(field->default);
	  }
	}
      }
    }
    return field_info;
  }

  //! Skip ahead the specified number of rows.
  void seek(int rows)
  {
    master_result->seek(rows);
  }

  //! Fetch the next row from the result.
  //!
  //! All strings in the result are decoded from UTF8.
  int|array(string) fetch_row()
  {
    int|array(string) row = master_result->fetch_row();
    if (!arrayp(row)) return row;
    array(int|mapping(string:mixed)) field_info = fetch_fields();
    foreach(row; int i; string|int val) {
      if (stringp(val) && field_info[i]->flags &&
	  !field_info[i]->flags->binary) {
	row[i] = utf8_to_string(val);
      }
    }
    return row;
  }
}
