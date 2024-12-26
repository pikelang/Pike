/* Auth.pmod
 */

/*
 *    Protocols.X, a Pike interface to the X Window System
 *
 *    See COPYRIGHT for copyright information.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#pike __REAL_VERSION__

//! XAuthority related stuff.

//!
enum AddressFamily {
  FamilyInternet = 0,
  FamilyDECnet = 1,
  FamilyChaos = 2,
  FamilyInternet6 = 6,
  FamilyAmoeba = 33,
  FamilyLocalHost = 252,
  FamilyKrb5Principal = 253,
  FamilyNetname = 254,
  FamilyLocal = 255,
  FamilyWild = 65535,
}

//! XAuthority key database.
class auth_file
{
  //! Mapping from address family to mapping from destination key
  //! (cf @[make_key()]) to authentication entry.
  //!
  //! Known address families:
  //! @int
  //!   @value 0
  //!     IPv4 address.
  //!   @value 1
  //!     DECNet address.
  //!   @value 2
  //!     Chaos net address.
  //!   @value 6
  //!     IPv6 address.
  //!   @value 33
  //!     Amoeba address.
  //!   @value 252
  //!     Localhost.
  //!   @value 253
  //!     Kerberos 5 Principal.
  //!   @value 254
  //!     Netname.
  //!   @value 256
  //!     Local connection (typically unix domain socket or loopback).
  //!   @value 65535
  //!     Wildcard.
  //! @endint
  mapping(int:mapping(string:mapping)) auth = ([]);

  //! Make a lookup key suitable for @[auth] given
  //! @[address] and @[display].
  string make_key(string address, int display)
  {
    return sprintf("%d:%s", display, address);
  }

  //! @param s
  //!   Raw Xauthority file data.
  protected void create(string(8bit) s)
  {
    Stdio.Buffer struct = Stdio.Buffer(s);

    while (sizeof(struct))
      {
	mapping m = ([ ]);

	m->family = struct->read_int(2);
	m->address = struct->read_hstring(2);
	m->display = (int) struct->read_hstring(2);
	m->name = struct->read_hstring(2);
	m->data = struct->read_hstring(2);

	if (!auth[m->family])
	  auth[m->family] = ([]);

	auth[m->family][make_key(m->address, m->display)] = m;
      }
  }

  //! Lookup authentication entry for the unix domain socket
  //! @[name]:@[display].
  mapping lookup_local(string name, int display)
  {
    return auth[256] && auth[256][make_key(name, display)];
  }

  //!
  string ip2string(string ip)
  {
    return sprintf("%@c", Array.map(ip / ".",
				    lambda(string s)
				    { return (int) s; }));
  }

  //!
  mapping lookup_ip(string ip, int display)
  {
    if(ip == "127.0.0.1")
      return lookup_local(gethostname(), display);
    return auth[0] && auth[0][make_key(ip2string(ip), display)];
  }
}

//!
class lock_key
{
  string name;
  string c_name;
  string l_name;

  protected void create(string f)
  {
    name = f;
    c_name = name + "-c";
    l_name = name + "-l";
  }

  int my_hardlink(string from, string to)
  {
#if constant(hardlink)
    return !catch(hardlink(from, to));
#else
    return 0; // Failed, hubbe
#endif
  }


  object lock()
  {
    object f = Stdio.File();
    if (!f->open(c_name, "cxw"))
      return 0;
    f->close();

    return my_hardlink(c_name, l_name) && this;
  }

  protected void _destruct()
  {
    rm(c_name);
    rm(l_name);
  }
}

//!
object lock_file(string name)
{
  return lock_key(name)->lock();
}

//! Read the authentication database specified by @tt{$XAUTHORITY@}.
object(auth_file) read_auth_data()
{
  string fname = getenv("XAUTHORITY");
  if (!fname)
    {
      fname = getenv("HOME");
      if (!fname)
	return 0;
      fname = combine_path(fname, ".Xauthority");
    }

  object|zero key = lock_file(fname);
  if (!key)
    return 0;

  string s = Stdio.read_file(fname);
  key = 0;
  if (!s)
    return 0;

  return auth_file(s);
}
