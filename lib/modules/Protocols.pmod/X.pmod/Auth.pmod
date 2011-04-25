/* Auth.pmod
 *
 * $Id$
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
	
	m->family = struct->get_uint(2);
	m->address = struct->get_var_string(2);
	m->display = (int) struct->get_var_string(2);
	m->name = struct->get_var_string(2);
	m->data = struct->get_var_string(2); 

	if (!auth[m->family])
	  auth[m->family] = ([]);
	
	auth[m->family][make_key(m->address, m->display)] = m;
      }
  }

  mapping lookup_local(string name, int display)
  {
    return auth[256] && auth[256][make_key(name, display)];
  }

  string ip2string(string ip)
  {
    return sprintf("%@c", Array.map(ip / ".",
				    lambda(string s)
				    { return (int) s; }));
  }
  
  mapping lookup_ip(string ip, int display)
  {
    if(ip == "127.0.0.1")
      return lookup_local(gethostname(), display);
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
