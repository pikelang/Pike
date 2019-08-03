#if constant(__builtin.debug_breakpoint)
inherit __builtin.debug_breakpoint;
#else
#error("Debugger is only when Pike is compiled with '--with-debug' flag.\n");
#endif

protected string _sprintf(int conversion_type, mapping(string:int)|void params) {
  // TODO need to add flag for "want enable"
  if(get_program())
    return sprintf("Breakpoint(%s:%d, %s, %O)",
                    get_within_file(),
                    get_line_number(),
                    (is_enabled() ? "ENABLED" : "DISABLED"),
                    get_program());
  else
    return sprintf("Breakpoint(%s:%d, %s, %s)",
                    get_within_file(),
                    get_line_number(),
                    (is_enabled() ? "PENDING_ENABLED" : "PENDING_DISABLED"),
                    get_filename());
}

