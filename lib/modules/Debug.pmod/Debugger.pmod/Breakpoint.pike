#if !_Debug.HAVE_DEBUG
#error("To use the debugger, compile Pike --with-debug.")
#endif
inherit _Debug.debug_breakpoint;

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

