/*
 * Some SQL utility functions.
 * They are kept here to avoid circular references.
 *
 * Henrik Grubbstr�m 1999-07-01
 */

#pike 8.1

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

//! Wrapper to handle zero.
//!
//! @seealso
//!   @[zero]
protected class ZeroWrapper
{
  //! @returns
  //!   Returns the following:
  //!   @string
  //!     @value "NULL"
  //!       If @[fmt] is @expr{'s'@}.
  //!     @value "ZeroWrapper()"
  //!       If @[fmt] is @expr{'O'@}.
  //!   @endstring
  //!   Otherwise it formats a @expr{0@} (zero).
  protected string _sprintf(int fmt, mapping(string:mixed) params)
  {
    if (fmt == 's') return "NULL";
    if (fmt == 'O') return "ZeroWrapper()";
    return sprintf(sprintf("%%*%c", fmt), params, 0);
  }
}

//! Instance of @[ZeroWrapper] used by @[handle_extraargs()].
protected ZeroWrapper zero = ZeroWrapper();

protected class NullArg
{
  protected string _sprintf (int fmt)
    {return fmt == 'O' ? "NullArg()" : "NULL";}
}
protected NullArg null_arg = NullArg();

//! Handle @[sprintf]-based quoted arguments
//!
//! @param query
//!   The query as sent to one of the query functions.
//!
//! @param extraargs
//!   The arguments following the query.
//!
//! @param bindings
//!   Optional bindings mapping to which additional bindings will be
//!   added. It's returned as the second element in the return value.
//!   A new mapping is used if this isn't specified.
//!
//! @returns
//!   Returns an array with two elements:
//!   @array
//!     @elem string 0
//!       The query altered to use bindings-syntax.
//!     @elem mapping(string|int:mixed) 1
//!       A bindings mapping. Zero if it would be empty.
//!   @endarray
array(string|mapping(string|int:mixed))
  handle_extraargs(string query, array(mixed) extraargs,
		   void|mapping(string|int:mixed) bindings) {

  array(mixed) args=allocate(sizeof(extraargs));
  if (!bindings) bindings = ([]);

  if (sizeof(extraargs) && mappingp(extraargs[-1])) {
    bindings += extraargs[-1];
    extraargs = extraargs[..<1];
  }

  // NB: Protect against successive calls of handle_extraargs() with
  //     the same bindings mapping having conflicting entries.
  int a = sizeof(bindings);
  foreach(extraargs; int j; mixed s) {
    if (stringp(s) || multisetp(s)) {
      string bind_name;
      do {
	bind_name = ":arg"+(a++);
      } while (has_index (bindings, bind_name));
      args[j]=bind_name;
      bindings[bind_name] = s;
      continue;
    }
    if (intp(s) || floatp(s)) {
      args[j] = s || zero;
      continue;
    }
    if (objectp (s) && s->is_val_null) {
      args[j] = null_arg;
      continue;
    }
    error("Wrong type to query argument %d: %O\n", j + 1, s);
  }

  return ({sprintf(query,@args), sizeof(bindings) && bindings});
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
                        void|object driver,
                        void|string charset)
{
  array(string|int)k;
  array(string) v;
  if (!bindings)
    return query;

  if (charset) {
    bindings[Sql.QUERY_OPTION_CHARSET] = charset;
  }

  // Reserve binding entries starting with '*' for options.
  // Throws if mapping key is empty string.
  k = filter(indices(bindings),
             lambda(string|int i) {
               return !stringp(i) || (i[0] != '*');
             });

  function my_quote=(driver&&driver->quote?driver->quote:quote);
  v=map(map(k, bindings),
	lambda(mixed m) {
	  if(undefinedp(m))
	    return "NULL";
	  if (objectp (m) && m->is_val_null)
	    // Note: Could need bug compatibility here - in some cases
	    // we might be passed a null object that can be cast to
	    // "", and before this it would be. This is an observed
	    // compat issue in comment #7 in [bug 5900].
	    return "NULL";
	  if(multisetp(m))
	    return sizeof(m) ? indices(m)[0] : "";
	  return "'"+(intp(m)?(string)m:my_quote((string)m))+"'";
	});

  // Throws if mapping key is empty string.
  k=map(k,
        lambda(string s){
          return ( (stringp(s)&&s[0]==':') ? s : ":"+s);
        });
  return replace(query,k,v);
}

//! Result object wrapper performing utf8 decoding of all fields.
class UnicodeWrapper (
		      // The wrapped result object.
		      protected object master_result
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
      if (stringp(val)) {
	row[i] = utf8_to_string(val);
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
	row[i] = utf8_to_string(val);
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
	row[i] = utf8_to_string(val);
      }
    }
    return row;
  }
}
