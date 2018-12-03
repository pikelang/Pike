#if constant(__builtin.debug_breakpoint)
inherit __builtin.debug_breakpoint;
#else
#error("debugging not available.\n");
#endif

string _sprintf(int conversion_type, mapping(string:int)|void params) {
	// TODO need to add flag for "want enable"
	if(get_program())
      return sprintf("Breakpoint(%O,%s,%d,%s)", get_program(), get_within_file(), get_line_number(), (is_enabled()?"ENABLED":"DISABLED"));
  	else
        return sprintf("Breakpoint(%s,%s,%d,%s)", get_filename(), get_within_file(), get_line_number(), (is_enabled()?"PENDING_ENABLE":"PENDING_DISABLED"));

}
