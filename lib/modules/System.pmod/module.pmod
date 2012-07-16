#pike __REAL_VERSION__
#pragma strict_types

// This module is to allow the _system module to be called System.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;

//! Get the username of the user the process is running as.
//!
//! @note
//!  On NT systems, this returns the user the current thread is running as.
string get_user()
{
#if constant(System.GetUserName)
  return System.GetUserName();
#elseif constant(System.getuid)
  return getpwuid(System.getuid())[0];
#else
  return "UNKNOWN";
#endif /* System.GetUserName */
}

//! Get the full path for the current user's home directory
string get_home()
{
  string home = [string]getenv("HOME");
  if(home) return home;

#if __NT__
  string homedrive = [string]getenv("HOMEDRIVE");
  home = [string]getenv("HOMEPATH");
  if(homedrive)
    home = homedrive + (home||"\");
  if(home) return home;
#endif

  return 0;      
}

