#pike 8.1

//! Pike 8.0 compatibility @[predef::Stdio] implementation.

#pragma no_deprecation_warnings

//! @decl inherit 8.1::Stdio

//! @ignore
inherit Stdio.module;
//! @endignore

class File
{
  inherit ::this_program;

  // NB: Deprecated in Pike 7.8.
  optional __deprecated__ this_program set_peek_file_before_read_callback(int(0..1) ignored)
  {
    // This hack is not necessary anymore - the backend now properly
    // ignores events if other callbacks/threads have managed to read
    // the data before the read callback.
    return this;
  }
}

class FILE
{
  inherit ::this_program;

  // NB: Deprecated in Pike 7.8.
  optional __deprecated__ this_program set_peek_file_before_read_callback(int(0..1) ignored)
  {
    // This hack is not necessary anymore - the backend now properly
    // ignores events if other callbacks/threads have managed to read
    // the data before the read callback.
    return this;
  }
}

class Readline
{
  inherit Stdio.Readline;

  // Keep the old hook available to subclasses, including calls to ::destroy().
  protected void destroy(void|int reason)
  {
    ::_destruct();
  }

  protected void _destruct(void|int reason)
  {
    destroy(reason);
  }
}
