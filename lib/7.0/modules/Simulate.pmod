#pike 7.1

local inherit Array;
local inherit String;

#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

int member_array(mixed needle,array(mixed) haystack,int|void start)
{
  return search(haystack,needle,start);
}

object previous_object()
{
  int e;
  array(array(mixed)) trace;
  object o,ret;
  trace=backtrace();
  o=function_object(trace[-2][2]);
  for(e=sizeof(trace)-3;e>=0;e--)
  {
    if(!trace[e][2]) continue;
    ret=function_object(trace[e][2]);
    if(o!=ret) return ret;
  }
  return 0;
}

function this_function()
{
  return backtrace()[-2][2];
}


function get_function(object o, string a)
{
  mixed ret;
  ret=o[a];
  return functionp(ret) ? ret : 0;
}

array(string) map_regexp(array(string) s, string reg)
{
  
  object(Regexp) regexp = Regexp(reg);
  s=filter(s,regexp->match);
  return s;
}

#define GP(X) constant X = Process.X

GP(create_process);
GP(exec);
GP(search_path);
GP(sh_quote);
GP(split_quoted_string);
GP(spawn);
GP(popen);
GP(system);

constant PI = 3.1415926535897932384626433832795080;
function all_efuns = all_constants;
function explode = `/;
function filter_array = filter;
function map_array = map;
function implode = `*;
function m_indices = indices;
function m_sizeof = sizeof;
function m_values = values;
function strstr = search;
function sum = `+;
function add_efun = add_constant;
function l_sizeof = sizeof;
function listp = multisetp;
function mklist = mkmultiset;
function aggregate_list = aggregate_multiset;
#if efun(gethostname)
function query_host_name=gethostname;
#endif
