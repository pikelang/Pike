inherit "/precompiled/file";

void perror(string s)
{
#if efun(strerror)
  write(s+": "+strerror(errno())+"\n");
#else
  write(s+": errno: "+errno()+"\n");
#endif
}

void create()
{
  file::create("stderr");
  add_constant("perror",perror);
}