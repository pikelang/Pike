/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dumpmaster.pike,v 1.12 2004/04/17 15:18:52 marcus Exp $
*/

string fr;

array encoded=({});

void handle_error(mixed err)
{
  foreach(encoded, mixed o)
    {
      werror("***Failed to encode %t: %O\n",o,o);
#if constant(_describe)
      _describe(o);
#endif
    }

  werror("%O\n",err);
}

void compile_error(string file,int line,string err)
{
  werror("%s:%s:%s\n", file, line?(string)line:"-", err);
}

string fakeroot(string s)
{
 if(fr) s=fr+combine_path(getcwd(),s);
 return s;
}

string read_file(string s)
{
  return _static_modules.files()->Fd(fakeroot(s),"r")->read();
}

program compile_file(string file)
{
  return compile(cpp(read_file(file),file));
}

class Codec
{
  string nameof(mixed x)
  {
    if(mixed tmp=search(all_constants(),x))  return tmp;
    switch(x)
    {
#define CONST(X) case X: return #X
      CONST(_static_modules.files.Stat);
      CONST(_static_modules.Builtin.__backend);
    }
    encoded+=({x});
    return UNDEFINED;
  } 
}

void _main(array(string) argv, array(string) env)
{
  foreach(argv[1..sizeof(argv)-2], string f)
    sscanf(f, "--fakeroot=%s", fr);
    
  program p=compile_file(argv[-1]);
  string s=encode_value(p, Codec());
  _static_modules.files()->Fd(fakeroot(argv[-1]) + ".o","wct")->write(s);
  exit(0);
}

mixed resolv() { return UNDEFINED; }
