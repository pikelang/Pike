void create()
{
  if (!load_module("./crypto.so")) {
    werror("Could't load module \"./crypto.so\"\n");
  }
}
