#pike __REAL_VERSION__
#pragma strict_types

// This module is to allow the _system module to be called System.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;
#if constant(__builtin.TM)
constant TM    = __builtin.TM;
#endif
//! Get the username of the user that started the process.
//!
//! @returns
//!  the username of the user "associated" with the current process, or zero
//!  if a method to find this information does not exist on the current system.
//!
//! @note
//!  On NT systems, this returns the user the current thread is running as,
//!  while on Unix-like systems this function returns the user that started
//!  the process (rather than the effective user)..
string get_user()
{
#if constant(System.GetUserName)
  return System.GetUserName();
#elseif constant(getuid)
  return [string]getpwuid(getuid())[0];
#else
  return 0;
#endif
}

//! Get the full path for the current user's home directory
//!
//! @returns
//! the full path to the current user's home directory, or zero
//! if the appropriate environment variables have not been set.
//!
//! @note
//! This method uses the standard environment variables for
//! various systems to determine the home directory.
string get_home()
{
  string home = [string]getenv("HOME");
  if(home) return home;

#if __NT__
  string homedrive = [string]getenv("HOMEDRIVE");
  home = [string]getenv("HOMEPATH");
  if(homedrive)
    home = combine_path(homedrive + "\\", (home||"\\"));
  if(home) return home;
#endif

  return 0;      
}

