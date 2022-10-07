#pike 8.1

class File
{
  inherit Stdio.File;

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
  inherit Stdio.FILE;

  // NB: Deprecated in Pike 7.8.
  optional __deprecated__ this_program set_peek_file_before_read_callback(int(0..1) ignored)
  {
    // This hack is not necessary anymore - the backend now properly
    // ignores events if other callbacks/threads have managed to read
    // the data before the read callback.
    return this;
  }
}

