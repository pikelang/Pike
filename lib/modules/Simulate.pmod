inherit Array;
inherit Stdio;
inherit String;
inherit Process;

#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

inherit Regexp : regexp;

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

string *map_regexp(string *s, string reg)
{
  
  regexp::create(reg);
  s=filter(s,regexp::match);
  regexp::create(); /* Free compiled regexp */
  return s;
}

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
