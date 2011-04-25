// $Id$

#pike __REAL_VERSION__

#if constant(_assembler_debug)
constant assembler_debug = _assembler_debug;
#endif

#if constant(_compiler_trace)
constant compiler_trace  = _compiler_trace;
#endif

#if constant(_debug)
constant debug = _debug;
#endif

#if constant(_describe)
constant describe = _describe;
#endif

#if constant(_describe_program)
constant describe_program = _describe_program;
#endif

#if constant(_dmalloc_set_name)
constant dmalloc_set_name = _dmalloc_set_name;
#endif

#if constant(_dump_backlog)
constant dump_backlog = _dump_backlog;
#endif

#if constant(_gc_set_watch)
constant gc_set_watch = _gc_set_watch;
#endif

#if constant(_gc_status)
constant gc_status = _gc_status;
#endif

#if constant(_list_open_fds)
constant list_open_fds = _list_open_fds;
#endif

#if constant(_locate_references)
constant locate_references = _locate_references;
#endif

#if constant(_memory_usage)
constant memory_usage = _memory_usage;

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
#endif

#if constant(_optimizer_debug)
constant optimizer_debug = _optimizer_debug;
#endif

#if constant(_reset_dmalloc)
constant reset_dmalloc = _reset_dmalloc;
#endif

#if constant(_verify_internals)
constant verify_internals = _verify_internals;
#endif
