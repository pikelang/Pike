
/*
 * An LR(1) Parser in Pike
 *
 * Henrik Grubbström 1996-11-23
 */

#pike 7.2

import LR;

object(parser) g;

/*
 * Test actions
 */

int add_values(int x, mixed ignore, int y)
{
  werror(x+" + "+y+" = "+(x+y)+"\n");
  return (x+y);
}

int mul_values(int x, mixed ignore, int y)
{
  werror(x+" * "+y+" = "+(x*y)+"\n");
  return (x*y);
}

int get_second_value(mixed ignored, int x, mixed ... ignored_also)
{
  return(x);
}

int concat_values(int x, int y)
{
  return (x*10 + y);
}

int make_value(string s)
{
  return((int)s);
}

/*
 * Test grammar
 */

array(string) nonterminals = ({
  "E'", "E", "T", "F", "id", "value",
});

array(array(string|int)) g_init = ({
  ({ 0, 1, "" }),
  ({ 1, 1, "+", 2 }),
  ({ 1, 2 }),
  ({ 2, 2, "*", 3 }),
  ({ 2, 3 }),
  ({ 3, "(", 1, ")" }),
  ({ 3, 4 }),
  ({ 4, 5 }),
  ({ 4, 4, 5 }),
  ({ 5, "0" }),
  ({ 5, "1" }),
  ({ 5, "2" }),
  ({ 5, "3" }),
  ({ 5, "4" }),
  ({ 5, "5" }),
  ({ 5, "6" }),
  ({ 5, "7" }),
  ({ 5, "8" }),
  ({ 5, "9" }),
});

array(int|function(mixed ...:mixed)) action_init = ({
  0,
  add_values,
  0,
  mul_values,
  0,
  get_second_value,
  0,
  0,
  concat_values,
  make_value,
  make_value,
  make_value,
  make_value,
  make_value,
  make_value,
  make_value,
  make_value,
  make_value,
  make_value,
});

/*
 * Test action
 */
string a_init(string ... args)
{
  if (sizeof(args)) {
    werror(sprintf("Reducing %s => \"%s\"\n",
		   Array.map(args, g->symbol_to_string) * ", ",
		   args * ""));
    return (`+(@args));
  } else {
    /* Empty rule */
    werror("Reducing /* empty */ => \"\"\n");
    return("");
  }
}

string symbol_to_string(int|string symbol) 
{
  if (intp(symbol)) {
    if (symbol < sizeof(nonterminals)) {
      return(nonterminals[symbol]);
    } else {
      return("nonterminal"+symbol);
    }
  } else {
    return("\""+symbol+"\"");
  }
}

void create()
{
  g = parser();

  g->set_symbol_to_string(symbol_to_string);
  
  foreach (indices(g_init), int i) {
    g->add_rule(rule(g_init[i][0], g_init[i][1..], action_init[i]));
  }
}

class scan {
  /*
   * Test input
   */

  array(string) s_init = ({
    "1", "*", "(", "3", "+", "2", ")", "+", "2", "*", "3",
    "",
  });

  int s_pos = 0;

  string scan()
  {
    return(s_init[s_pos++]);
  }
}

object(scan) scanner = scan();

int main(int argc, array(string) argv)
{
  mixed result;

  werror("Grammar:\n\n" + (string) g);

#if constant(_memory_usage)
  werror(sprintf("Memory usage:\n%O\n", _memory_usage()));
#endif

  werror("Compiling...\n");

  g->verbose = 0;

  g->compile();

  werror("Compilation finished!\n");

#if constant(_memory_usage)
  werror(sprintf("Memory usage:\n%O\n", _memory_usage()));
#endif

  result = g->parse(scanner->scan);

  werror(sprintf("Result of parsing: \"%s\"\n", result + ""));
}
