#pike 7.1

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

// used to be "inherit Array;" in Pike 7.0 Simulate.pmod:
#define GA(X) constant X = Array.X

GA(sort);
GA(everynth);
GA(interleave_array);
GA(diff);
GA(diff_longest_sequence);
GA(diff_compare_table);
GA(longest_ordered_sequence);
GA(map);
GA(filter);
GA(splice);
GA(transpose);
GA(columns);
GA(reduce);
GA(rreduce);
GA(shuffle);
GA(permute);
GA(search_array);
GA(sum_arrays);
GA(sort_array);
GA(uniq);
GA(transpose_old);
GA(diff3);
GA(diff3_old);
GA(dwim_sort_func);
GA(lyskom_sort_func);

// used to be "inherit String;" in Pike 7.0 Simulate.pmod:
#define GS(X) constant X = String.X

GS(count);
GS(width);
GS(implode_nicely);
GS(capitalize);
GS(sillycaps);
GS(strmult);
GS(common_prefix);
GS(String_buffer);
GS(fuzzymatch);
GS(trim_whites);
GS(trim_all_whites);

// used to be "inherit Process;" in Pike 7.0 Simulate.pmod:
#define GP(X) constant X = Process.X

GP(exec);

#ifndef __NT__
GP(exece);
GP(fork);
GP(Spawn);
#endif /* !__NT__ */

GP(create_process);
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
