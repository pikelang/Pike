/* Auth.pmod
 *
 */

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
