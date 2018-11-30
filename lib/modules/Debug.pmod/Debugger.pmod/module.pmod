#if constant(__builtin.debug_breakpoint)
 program Breakpoint = .debug_breakpoint;
#endif

protected object debugger;
protected mapping breakpoints = ([]);

//! 
void deferred_breakpoint_completer(string filename, program prog) {
//werror("breakpoint completer: %O\n", prog);
//werror("programs: %O\n", master()->programs);
  if(!sizeof(breakpoints)) return;

  foreach(breakpoints; object bp;) {
    if(bp->is_deferred() && bp->get_filename() == filename)
      bp->set_program(prog);
  }

}

//!
variant object add_breakpoint(program p, string within_file, int line_number) {
  object bp = Breakpoint(p, within_file, line_number);
  breakpoints[bp] = 1;
  return bp;
}

//! support for deferred breakpoints, TBD
variant object add_breakpoint(string filename, int line_number) {
	object bp;
    program p = master()->programs[filename];
    if(!p) {
      werror("could not find program %s. Deferring.\n", filename);
  	bp = Breakpoint(filename, filename, line_number);
    } else {
      bp = Breakpoint(p, filename, line_number);
    }
  breakpoints[bp] = 1;
  return bp;
}

//! support for deferred breakpoints, TBD
variant object add_breakpoint(string filename, string within_file, int line_number) {
	object bp;
  program p = master()->programs[filename];
  if(!p) {
    werror("could not find program %s. Deferring.\n", filename);
	bp = Breakpoint(filename, within_file, line_number);
  } else {
    bp = Breakpoint(p, within_file, line_number);
  }
  breakpoints[bp] = 1;
  return bp;
}


//!
object remove_breakpoint(object breakpoint) {
  if(breakpoint->get_enabled())  breakpoint->disable();
  return m_delete(breakpoints, breakpoint);
}

mapping get_breakpoints() {
  return breakpoints + ([]);
}

object get_debugger() {
	return debugger;
}

void set_debugger(object _debugger) {
	debugger = _debugger;
}


function get_debugger_handler() {
	return debugger->do_breakpoint;
}
