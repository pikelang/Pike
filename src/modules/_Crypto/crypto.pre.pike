/* $Id: crypto.pre.pike,v 1.4 1997/05/31 22:04:08 grubba Exp $ */

void create()
{
  string module, name;
  foreach( ({ "crypto" }) , module)
    /*    , "idea", "des", "invert", "sha", "pipe", "cbc" }) */
    {
      name = "./" + module + ".so";
      if (!load_module(name)) {
	werror("Couldn't load module \"" + name + "\"\n");
      }
    }
}
