#pike __REAL_VERSION__

//! Generic base classes for the Sql interfaces.

//! Field to set in the query bindings mapping to set a character set
//! for just the current query. Only supported by some databases.
constant QUERY_OPTION_CHARSET = "*charset*";

//! Wrapper to handle conversion of zero to NULL in
//! @[Connection()->handle_extraargs()].
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

//! Instance of @[ZeroWrapper] used by @[Connection()->handle_extraargs()].
ZeroWrapper zero_arg = ZeroWrapper();

protected class NullArg
{
  protected string _sprintf (int fmt)
  {
    return fmt == 'O' ? "NullArg()" : "NULL";
  }
}
NullArg null_arg = NullArg();

//! @ignore
// Result cursor for the rows collected by Connection.promise_query().
class BufferedResult
{
  inherit .Result;

  private array(array(mixed)) rows;
  private array(mapping(string:mixed)) columns;
  private int affected;
  private string command_status;

  protected void create(array(array(mixed)) rows,
                        array(mapping(string:mixed)) columns,
                        int affected, string command_status)
  {
    this::rows = rows;
    this::columns = columns;
    this::affected = affected;
    this::command_status = command_status;
  }

  int num_rows() { return sizeof(rows); }
  int num_fields() { return sizeof(columns); }
  int eof() { return index >= sizeof(rows); }
  int affected_rows() { return affected; }
  string status_command_complete() { return command_status; }
  array(mapping(string:mixed)) fetch_fields() { return columns; }

  array(mixed) fetch_row()
  {
    if (eof()) return 0;
    return rows[index++];
  }

  void seek(int skip)
  {
    if (skip < 0) predef::error("Cannot seek to negative result indices.\n");
    index += skip;
  }

  this_program|zero next_result() { return 0; }
}
//! @endignore

string wild_to_glob(string wild)
{
  return replace(wild,
                 ({ "?", "*", "[", "\\",
                    "%", "_" }),
                 ({ "\\?", "\\*", "\\[", "\\\\",
                    "*", "?" }));
}
