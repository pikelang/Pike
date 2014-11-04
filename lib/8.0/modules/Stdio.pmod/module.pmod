#pike 8.1

inherit Stdio : pre;

class File
{
  inherit pre::File;

  this_program set_peek_file_before_read_callback(int(0..1) ignored)
  {
    // This hack is not necessary anymore - the backend now properly
    // ignores events if other callbacks/threads have managed to read
    // the data before the read callback.
    return this;
  }
}