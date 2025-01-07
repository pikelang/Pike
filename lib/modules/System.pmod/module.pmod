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
string|zero get_user()
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
string|zero get_home()
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

#if constant(_system.getpwent)
  // FIXME: Reset the cursor for System.getpwent()!
  int uid=-1, gid=-1;
  array|zero pwent;
  while( pwent = getpwent() ) {
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
    // FIXME: Reset the cursor for System.getgrent()!
    gid = -1;
    array|zero grent;
    while( grent = getgrent() ) {
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
#else
  my_error("Dropping of privileges not implemented.\n");
#endif
}

#if defined(__APPLE__) && constant(resolvepath)
// Needed to avoid resolver loop.
private constant Stdio = _static_modules._Stdio;

protected int(0..1) compare_stat(Stdio.Stat st, Stdio.Stat st2)
{
  if (!st && !st2) return 1;
  return st && st2 &&
    (st->mode == st2->mode) &&
    (st->dev == st2->dev) &&
    (st->ino == st2->ino);
}

//!   Normalize an existing MacOS X file system path.
//!
//!   The following transformations are currently done:
//!   @ul
//!     @item
//!       If the @[path] is not valid UTF-8, it will be converted
//!       into UTF-8.
//!     @item
//!       Relative paths are expanded to absolute paths.
//!     @item
//!       Initial components @expr{"/private"@} and @expr{"/var/automount"@}
//!       (or both) are removed if the result indicates the same
//!       directory node.
//!     @item
//!       Trailing slashes (@expr{'/'@}) are removed.
//!     @item
//!       Current- and parent-directory path components (@expr{"."@}
//!       and @expr{".."@}) are followed, similar to @[combine_path].
//!     @item
//!       Case-information in directory and file names is restored.
//!     @item
//!       File fork information is stripped (ie all stuff after the
//!       first non-directory in the path is stripped).
//!   @endul
//!
//! @returns
//!   A normalized absolute path without trailing slashes.
//!
//!   May throw errors on failure, e.g. if the file or directory doesn't
//!   exist.
//!
//! @seealso
//!   @[combine_path()]
utf8_string normalize_path(utf8_string path)
{
  /* Re: Feature set:
   * cf https://developer.apple.com/documentation/foundation/nsstring/1407194-standardizingpath
   */

  if (!validate_utf8(path, ~0)) {
    path = string_to_utf8(path, 1);
  }
  path = [object(utf8_string)]combine_path(getcwd(), path);
  // path = resolvepath(path);

  array(utf8_string) res = [array(utf8_string)](path/"/");
  int i;

  Stdio.Fd d = [object(Stdio.Fd)]Pike.Lazy.Stdio.File();
  if (has_prefix(path, "/")) {
    d->open("/", "r");
    i = 1;
  } else {
    d->open(".", "r");
  }
  // NB: We assume that "/" and "." exist and are directories.

  for (;i < sizeof(res); i++) {
    utf8_string seg = res[i];

    Stdio.Stat st = d->statat(seg);
    if (!st) {
      i--;
      break;
    }
    array(utf8_string) files = [array(utf8_string)]d->get_dir();
    if (!has_value(files, seg)) {
      string seg2 = lower_case(Unicode.normalize(utf8_to_string(seg), "NFC"));
      array(string) files2 = map(map(map(files, utf8_to_string),
                                     Unicode.normalize, "NFC"),
                                 lower_case);
      int best = -1;
      foreach(files2; int j; string f2) {
        Stdio.Stat st2 = d->statat(files[j]);
        if (compare_stat(st, st2)) {
          best = j;
          if (f2 == seg2) {
            // This looks as good as it gets.
            break;
          }
        }
      }
      if (best < 0) {
        // Not found!
        // The path segment in seg apparently works, so keep it.
      } else {
        res[i] = seg = files[best];
      }
    }
    if (!st->isdir) break;
    d = d->openat(seg, "r");
  }

  // Strip potential fork information.
  if (i || (res[0] != "")) {
    res = res[..i];
  } else {
    // Root directory.
    res = ({ "", "" });
  }

  path = [object(utf8_string)](res * "/");

  // Some hard-coded stuff...
  foreach (({ "/private/", "/var/automount/" }), utf8_string prefix) {
    if (has_prefix(path, prefix)) {
      int i = search(path, "/", sizeof(prefix));
      if (i < 0) i = sizeof(path);
      if (i) {
        utf8_string seg = [object(utf8_string)]path[sizeof(prefix)-1..i-1];
        Stdio.Stat st = file_stat(seg);
        Stdio.Stat st2 = file_stat(path[..i-1]);
        if (compare_stat(st, st2)) {
          path = [object(utf8_string)]path[sizeof(prefix)-1..];
        }
      }
    }
  }

  return path;
}
#endif /* __APPLE__ && resolvepath() */
