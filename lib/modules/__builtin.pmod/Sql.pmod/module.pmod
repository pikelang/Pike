#pike __REAL_VERSION__

//! Generic base classes for the Sql interfaces.

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
ZeroWrapper zero = ZeroWrapper();

protected class NullArg
{
  protected string _sprintf (int fmt)
  {
    return fmt == 'O' ? "NullArg()" : "NULL";
  }
}
NullArg null_arg = NullArg();

string wild_to_glob(string wild)
{
  return replace(wild,
                 ({ "?", "*", "[", "\\",
                    "%", "_" }),
                 ({ "\\?", "\\*", "\\[", "\\\\",
                    "*", "?" }));
}
