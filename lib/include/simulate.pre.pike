#include <process.h>
#include <array.h>

#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

inherit "/precompiled/file";
inherit "/precompiled/regexp";

string read_bytes(string filename,void|int start,void|int len)
{
  string ret;
  if(!open(filename,"r"))
    error("Couldn't open file "+filename+".\n");
  
  switch(query_num_arg())
  {
  case 1:
  case 2:
    len=0x7fffffff;
  case 3:
    seek(start);
  }
  ret=read(len);
  close();
  return ret;
}

int write_file(string filename,string what)
{
  int ret;
  if(!open(filename,"awc"))
    error("Couldn't open file "+filename+".\n");
  
  ret=write(what);
  close();
  return ret;
}

int file_size(string s)
{
  int *stat;
  stat=file_stat(s);
  if(!stat) return -1;
  return stat[1]; 
}

mixed *sum_arrays(function foo, mixed * ... args)
{
  mixed *ret;
  int e,d;
  ret=allocate(sizeof(args[0]));
  for(e=0;e<sizeof(args[0]);e++)
    ret[e]=foo(@ column(args, e));
  return ret;
}

varargs int member_array(mixed needle,mixed *haystack,int start)
{
  return search(haystack,needle,start);
}

object previous_object()
{
  int e;
  mixed **trace;
  object o,ret;
  trace=backtrace();
  o=function_object(trace[-2][2]);
  for(e=sizeof(trace)-3;e>=0;e--)
  {
    if(!trace[1][2]) continue;
    ret=function_object(trace[1][2]);
    if(o!=ret) return ret;
  }
  return 0;
}

function this_function()
{
  return backtrace()[-2][2];
}

string capitalize(string s)
{
  return upper_case(s[0..0])+s[1..sizeof(s)];
}

function get_function(object o, string a)
{
  mixed ret;
  ret=o[a];
  return functionp(ret) ? ret : 0;
}

string *regexp(string *s, string reg)
{
  
  regexp::create(reg);
  s=filter(s,match);
  regexp::create(); /* Free compiled regexp */
  return s;
}

void create()
{
  add_constant("PI",3.1415926535897932384626433832795080);
  add_constant("capitalize",capitalize);
  add_constant("explode",`/);
  add_constant("file_size",file_size);
  add_constant("all_efuns",all_constants);

  add_constant("filter",filter);
  add_constant("map_array",map);

  add_constant("get_function",get_function);
  add_constant("implode",`*);
  add_constant("m_indices",indices);
  add_constant("m_sizeof",sizeof);
  add_constant("m_values",values);
  add_constant("member_array",member_array);
  add_constant("previous_object",previous_object);
  add_constant("read_bytes",read_bytes);
  add_constant("regexp",regexp);
  add_constant("strstr",search);
  add_constant("sum",`+);
  add_constant("sum_arrays",sum_arrays);
  add_constant("this_function",this_function);
  add_constant("write_file",write_file);
  add_constant("add_efun",add_constant);

  add_constant("l_sizeof",sizeof);
  add_constant("listp",multisetp);
  add_constant("mklist",mkmultiset);
  add_constant("aggregage_list",aggregate_multiset);
}
