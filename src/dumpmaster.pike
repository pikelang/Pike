#define UNDEFINED (([])[0])

void handle_error(mixed err)
{
  werror("%O\n",err);
}
program compile_file(string file)
{
  return compile(cpp(_static_modules.files()->Fd(file,"r")->read(),file));
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
  program p=compile_file(argv[-1]);
  string s=encode_value(p, Codec());
  _static_modules.files()->Fd(argv[-1] + ".o","wct")->write(s);
  exit(0);
}


mixed resolv() { return ([])[0]; }
