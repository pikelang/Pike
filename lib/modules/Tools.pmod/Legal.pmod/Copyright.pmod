#pike __REAL_VERSION__

// $Id: Copyright.pmod,v 1.7 2004/09/13 22:03:22 nilsson Exp $

//! Contains functions and information to store and present
//! copyright information about Pike and it's components.

static mapping(string:array(string)) copyrights = ([

  "Pike":
  ({
    "Copyright © 1994-1996 Fredrik Hübinette",
    "Copyright © 1996-1997 Informationsvävarna AB",
    "Copyright © 1997-2000 Idonex AB",
    "Copyright © 2000-2002 Roxen Internet Software AB",
    "Copyright © 2002-2004 Department of Computer and Information Science,\n"
    "                      Linköping University",
  }),

  "Unicode Character Database":
  ({ "Copyright © 1991-2004 Unicode, Inc." }),

  "TrueVision Targa code":
  ({ "Copyright © 1997 Raphael FRANCOIS and Gordon Matzigkeit" }),

  "Emacs font lock definitions":
  ({ "Copyright © 2002 Martin Stjernholm" }),

  "IDEA encryption and decryption code":
  ({ "Copyright © Xuejia Lai" }),

  "MD5 password hash code":
  ({ "Copyright © Poul-Henning Kamp" }),

  "Regular expression matching code":
  ({ "Copyright © 1986 by University of Toronto." }),

  "Zlib detection and verification code":
  ({ "Copyright © 1995-1998 Jean-loup Gailly and Mark Adler" }),

  "JPEG transform code":
  ({ "Copyright © 1991-1998, Thomas G. Lane, Independent JPEG Group" }),
]);

//! Adds a copyright message for the copyright @[holders] for the
//! component @[what].
//! @throws
//!   An error is thrown if the copyrighted component @[what] is
//!   already in the list of copyrights.
void add(string what, array(string) holders) {
  if(copyrights[what])
    error("Copyright for %O already present.\n", what);
  copyrights[what] = holders;
}

//! Return the latest copyright holder of Pike.
string get_latest_pike() {
  return copyrights->Pike[-1];
}

//! Returns a mapping containing all the stored copyrights. The
//! mapping maps component name to an array of copyright holders.
mapping(string:array(string)) get_all() {
  return copy_value(copyrights);
}

//! Returns the copyrights as a string, suitable for saving as a
//! file.
string get_text() {
  string ret = "Pike is protected by international copyright laws.\n\n";
  ret += copyrights->Pike*"\n";
  ret += #"

Pike refers to the source code, and any executables
created from the same source code. Parts of other
copyrighted works are included in Pike, in accordance
with their respective conditions for distribution.
";

  foreach(sort(indices(copyrights)), string component) {
    if(component=="Pike") continue;
    ret += "\n" + component + "\n" + copyrights[component]*"\n" + "\n";
  }
  return ret;
}
