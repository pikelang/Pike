/* Auth.pmod
 *
 * $Id: Auth.pmod,v 1.3 1998/04/19 00:30:59 grubba Exp $
 */

/*
 *    Protocols.X, a Pike interface to the X Window System
 *
 *    Copyright (C) 1998, Niels Möller, Per Hedbor, Marcus Comstedt,
 *    Pontus Hagland, David Hedbor.
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

/* Questions, bug fixes and bug reports can be sent to the pike
 * mailing list, pike@idonex.se, or to the athors (see AUTHORS for
 * email addresses. */

class auth_file
{
  mapping(int:mapping(string:mapping)) auth = ([]);

  string make_key(string address, int display)
  {
    return sprintf("%d:%s", display, address);
  }
  
  void create(string s)
  {
    object struct = ADT.struct(s);

    while (!struct->is_empty())
      {
	mapping m = ([ ]);
	
	m->family = struct->get_int(2);
	m->address = struct->get_var_string(2);
	m->display = (int) struct->get_var_string(2);
	m->name = struct->get_var_string(2);
	m->data = struct->get_var_string(2); 

	if (!auth[m->family])
	  auth[m->family] = ([]);
	
	auth[m->family][make_key(m->address, m->display)] = m;
      }
  }

  void lookup_local(string name, int display)
  {
    return auth[256] && auth[256][make_key(name, display)];
  }

  string ip2string(string ip)
  {
    return sprintf("%@c", Array.map(ip / ".",
				    lambda(string s)
				    { return (int) s; }));
  }
  
  void lookup_ip(string ip, int display)
  {
    return auth[0] && auth[0][make_key(ip2string(ip), display)];
  }
}

class lock_key
{
  string name;
  string c_name;
  string l_name;
  
  void create(string f)
  {
    name = f;
    c_name = name + "-c";
    l_name = name + "-l";
  }

  int my_hardlink(string from, string to)
  {
    return !catch(hardlink(from, to));
  }


  object lock()
  {
    object f = Stdio.File();
    if (!f->open(c_name, "cxw"))
      return 0;
    f->close();
    
    return my_hardlink(c_name, l_name) && this_object();
  }
  
  void destroy()
  {
    rm(c_name);
    rm(l_name);
  }
}

object lock_file(string name)
{
  return lock_key(name)->lock();
}

object read_auth_data()
{
  string fname = getenv("XAUTHORITY");
  if (!fname)
    {
      fname = getenv("HOME");
      if (!fname)
	return 0;
      fname = combine_path(fname, ".Xauthority");
    }

  object key = lock_file(fname);
  if (!key)
    return 0;

  string s = Stdio.read_file(fname);
  key = 0;
  if (!s)
    return 0;

  return auth_file(s);
}
