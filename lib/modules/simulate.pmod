import array_functions;
import stdio;
import string_functions;

#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

inherit regexp : regexp;

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

string *map_regexp(string *s, string reg)
{
  
  regexp::create(reg);
  s=filter(s,regexp::match);
  regexp::create(); /* Free compiled regexp */
  return s;
}

constant PI = 3.1415926535897932384626433832795080;
constant all_efuns = all_constants;
constant explode = `/;
constant filter_array = filter;
constant map_array = map;
constant implode = `*;
constant m_indices = indices;
constant m_sizeof = sizeof;
constant m_values = values;
constant strstr = search;
constant sum = `+;
constant add_efun = add_constant;
constant l_sizeof = sizeof;
constant listp = multisetp;
constant mklist = mkmultiset;
constant aggregate_list = aggregate_multiset;
#if efun(gethostname)
constant query_host_name=gethostname;
#endif
