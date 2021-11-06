#pike __REAL_VERSION__
#pragma strict_types

constant dont_dump_program = 1;


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
#if constant(_system.GetUserName)
  return GetUserName();
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

//! Drops the process privileges to the provided @[user] and
//! optionally @[group]. If no group is provided the default group for
//! the user is used.
//!
//! @param exception
//!   If true, an error exception will be thrown if
void drop_privs(string user, void|string group, void|int exception) {

  void my_error(string msg, mixed ... args) {
    if( exception )
      error(msg, @args);
    exit(1, msg, @args);
  }

  int uid=-1, gid=-1;
  array pwent;
  while( pwent = System.getpwent() ) {
    if( pwent[0]==user ) {
      uid = [int]pwent[2];
      gid = [int]pwent[3];
      break;
    }
  }
  if( uid==-1 ) {
    my_error("Unable to drop privileges. Unknown user %O.\n", user);
  }

  if( group ) {
    gid = -1;
    array grent;
    while( grent = System.getgrent() ) {
      if( grent[0]==group ) {
        gid = [int]grent[2];
        break;
      }
    }
    if( gid==-1 ) {
      my_error("Unable to drop privileges. Unknown group %O.\n", group);
    }
  }

  if( System.setgid(gid) != 0 ) {
    my_error("Unable to change group to gid %d\n", gid);
  }
  if( System.setuid(uid) != 0 ) {
    my_error("Unable to change user to uid %d\n", uid);
  }
  if( System.setuid(0) == 0 ) {
    my_error("Privileges not actually dropped.\n");
  }
}
