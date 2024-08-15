/*
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

//! Result object wrapper performing utf8 decoding of all fields.
class UnicodeWrapper (
		      // The wrapped result object.
		      protected object master_result
		      )
{
  inherit Sql.Result;

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
  protected array(int|mapping(string:mixed)) field_info;

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
	  field->name = utf8_to_string(field->name, 2);
	  if (field->table) {
	    field->table = utf8_to_string(field->table, 2);
	  }
	  if (field->default) {
	    field->default = utf8_to_string(field->default, 2);
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
      if (stringp(val)) {
	row[i] = utf8_to_string(val, 2);
      }
    }
    return row;
  }

  //! JSON is always utf8 default, do nothing.
  int|string fetch_json_result()
  {
    return master_result->fetch_json_result();
  }
}

class MySQLUnicodeWrapper
//! Result wrapper for MySQL that performs UTF-8 decoding of all
//! nonbinary fields. Useful if the result charset of the connection
//! has been set to UTF-8.
//!
//! @note
//! There's normally no need to use this class directly. It's used
//! automatically when @[mysql.set_unicode_decode_mode] is activated.
{
  inherit UnicodeWrapper;

  int|array(string) fetch_row()
  {
    int|array(string) row = master_result->fetch_row();
    if (!arrayp(row)) return row;
    array(int|mapping(string:mixed)) field_info = fetch_fields();
    foreach(row; int i; string|int val) {
      if (stringp(val) && field_info[i]->charsetnr != 63) {
	row[i] = utf8_to_string(val, 2);
      }
    }
    return row;
  }
}

class MySQLBrokenUnicodeWrapper
//! This one is used to get a buggy unicode support when compiled with
//! an old MySQL client lib that doesn't have the charsetnr property in
//! the field info. It looks at the binary flag instead, which is set
//! for binary fields but might also be set for text fields (e.g. with
//! a definition like @expr{"VARCHAR(255) BINARY"@}).
//!
//! I.e. the effect of using this one is that text fields with the
//! binary flag won't be correctly decoded in unicode decode mode.
//!
//! This has to be enabled either by passing @expr{"broken-unicode"@} as
//! charset to @[Sql.mysql.create] or @[Sql.mysql.set_charset], by calling
//! @[Sql.mysql.set_unicode_decode_mode(-1)], or by defining the
//! environment variable @tt{PIKE_BROKEN_MYSQL_UNICODE_MODE@}. That will
//! cause this buggy variant to be used if and only if the MySQL client
//! lib doesn't support the charsetnr property.
{
  inherit UnicodeWrapper;

  int|array(string) fetch_row()
  {
    int|array(string) row = master_result->fetch_row();
    if (!arrayp(row)) return row;
    array(int|mapping(string:mixed)) field_info = fetch_fields();
    foreach(row; int i; string|int val) {
      if (stringp(val) && field_info[i]->flags &&
	  !field_info[i]->flags->binary) {
	row[i] = utf8_to_string(val, 2);
      }
    }
    return row;
  }
}
