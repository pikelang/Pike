#define UNDEFINED (([])[0])

string fakeroot;

void handle_error(mixed err)
{
  werror("%O\n",err);
}

string read_file(string s)
{
  if(fakeroot)
    s=fakeroot+combine_path(getcwd(),s);

  return _static_modules.files()->Fd(s,"r")->read();
}

program compile_file(string file)
{
  return compile(cpp(read_file(file),file));
}

class Codec
{
  string nameof(string x)
  {
    if(mixed tmp=search(all_constants(),x))  return tmp;
    return UNDEFINED;
  } 
}
void _main(string *argv, string *env)
{
  foreach(argv[1..sizeof(argv)-2], string f)
    sscanf(f,"--fakeroot=%s",fakeroot);
    
  program p=compile_file(argv[-1]);
  string s=encode_value(p, Codec());
  _static_modules.files()->Fd(argv[-1] + ".o","wct")->write(s);
  exit(0);
}


mixed resolv() { return ([])[0]; }
