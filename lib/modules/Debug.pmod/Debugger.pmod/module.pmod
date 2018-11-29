#if constant(__builtin.debug_breakpoint)
 program Breakpoint = .debug_breakpoint;
#endif

protected object debugger;
protected mapping breakpoints = ([]);

//!
variant object add_breakpoint(program p, string filename, int line_number) {
  object bp = Breakpoint(p, filename, line_number);
  breakpoints[bp] = 1;
  return bp;
}

//! support for deferred breakpoints, TBD
variant object add_breakpoint(string filename, int line_number) {
  object bp = Breakpoint(filename, line_number);
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
