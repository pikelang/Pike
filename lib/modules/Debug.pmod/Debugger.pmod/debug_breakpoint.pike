#if constant(__builtin.debug_breakpoint)
inherit __builtin.debug_breakpoint;
#else
#error("debugging not available.\n");
#endif

string _sprintf(int conversion_type, mapping(string:int)|void params) {
  return sprintf("Breakpoint(%O,%s,%d)", get_program(), get_filename(), get_line_number());
}
