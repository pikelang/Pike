void create()
{
  if (!load_module("./crypto.so")) {
    werror("Couldn't load module \"./crypto.so\"\n");
  }
}
