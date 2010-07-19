// $Id: module.pmod,v 1.6 2010/07/19 15:30:47 mast Exp $

#pike __REAL_VERSION__

constant verify_internals = _verify_internals;
constant memory_usage = _memory_usage;
constant gc_status = _gc_status;
constant describe_program = _describe_program;

#if constant(_debug)
// These functions require --with-rtldebug.
constant debug = _debug;
constant optimizer_debug = _optimizer_debug;
constant assembler_debug = _assembler_debug;
constant dump_program_tables = _dump_program_tables;
constant locate_references = _locate_references;
constant describe = _describe;
constant gc_set_watch = _gc_set_watch;
constant dump_backlog = _dump_backlog;
#endif

#if constant(_dmalloc_set_name)
// These functions require --with-dmalloc.
constant reset_dmalloc = _reset_dmalloc;
constant dmalloc_set_name = _dmalloc_set_name;
constant list_open_fds = _list_open_fds;
constant dump_dmalloc_locations = _dump_dmalloc_locations;
#endif

#if constant(_compiler_trace)
// Requires -DYYDEBUG.
constant compiler_trace  = _compiler_trace;
#endif

//! Returns a pretty printed version of the
//! output from @[memory_usage].
string pp_memory_usage() {
  string ret="             Num   Bytes\n";
  mapping mu = memory_usage();
  foreach( ({ "array", "callable", "callback", "frame", "mapping",
              "multiset", "object", "program", "string" }),
           string what) {
    ret += sprintf("%-8s  %6d  %6d (%s)\n", what, mu["num_"+what+"s"],
                   mu[what+"_bytes"], String.int2size(mu[what+"_bytes"]));
  }
  return ret;
}

//! Returns the number of objects of every kind in memory.
mapping(string:int) count_objects() {
  int orig_enabled = Pike.gc_parameters()->enabled;
  Pike.gc_parameters( (["enabled":0]) );

  mapping(string:int) ret = ([]);

  object obj = next_object();
  //  while( zero_type(_prev(obj)) ) obj=_prev(obj);
  while(1) {
    object next_obj;
    if(catch(next_obj=_next(obj))) break;
    string p = sprintf("%O",object_program(obj));
    sscanf(p, "object_program(%s)", p);
    ret[p]++;
    obj = next_obj;
  }

  Pike.gc_parameters( (["enabled":orig_enabled]) );
  return ret;
}
