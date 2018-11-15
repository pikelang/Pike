protected object debugger;


object get_debugger() {
	return debugger;
}

void set_debugger(object _debugger) {
	debugger = _debugger;
}


function get_debugger_handler() {
	return debugger->do_breakpoint;
}