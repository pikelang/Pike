#if constant(__builtin.debug_breakpoint)
 program Breakpoint = .debug_breakpoint;
#endif

protected object debugger;
protected mapping breakpoints = ([]);

// TODO: we need to make sure that we don't accidentally create multiple breakpoints for the same code position.
// also need to handle request for lines that don't exist.

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

//! add a breakpoint for a particular program, at a particular file and ine number (ie, possibly an included file path)
variant object add_breakpoint(program p, string within_file, int line_number) {
  object bp = Breakpoint(p, within_file, line_number);
  breakpoints[bp] = 1;
  return bp;
}

//! supports breakpoints on programs not yet loaded.
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

//! supports breakpoints on programs not yet loaded.
//!  @param within_file
//!    search for lines in pogram coming from within_file, which may be different than the file containing
//!    the larger program definition, such as includes.
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

//!
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
