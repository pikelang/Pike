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
