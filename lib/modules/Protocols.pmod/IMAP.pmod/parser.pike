/* parser.pike
 *
 * Continuation based imap parser.
 */

object line; /* Current line */

void create(object l)
{
  line = l;
}

/* These functions are all called directly or indirectly by the
 * request enging in imap_server.pike. When a complete value is ready,
 * they call a continuation function given as an argument. They all
 * return an action mapping, saying what to do next. */

/* First a few relatively simple functions */

/* Used when line == 0, i.e. to continue reading after a literal */
class line_handler
{
  function process;
  array args;
    
  void create(function p, mixed ... a)
    {
      process = p;
      args = a;
    }

  mapping `()(object l)
    {
      line = l;
      return process(@args);
    }
}

/* Value is an integer */
mapping get_number(function c)
{
  if (line)
    return c(line->get_number());

  return ([ "action" : "expect_line",
	    "handler" : line_handler(get_number, c) ]);
}
  
/* Value is a string */
mapping get_atom(function c)
{
  if (line)
    return c(line->get_atom());

  return ([ "action" : "expect_line",
	    "handler" : line_handler(get_atom, c) ]);
}

class get_string_handler
{
  function c;
    
  void create(function _c)
    {
      c = _c;
    }

  mapping `()(string s)
    {
      return c(s);
    }
}

/* Value is a string */
mapping get_string(function c)
{
  if (line)
  {
    string|object s = line->get_string();
    if (stringp(s))
      return c(s);

    line = 0;
    return ([ "action" : "expect_literal",
	      "length" : s->length,
	      "handler" : get_string_handler(c) ]);
  }
  return ([ "action" : "expect_line",
	    "handler" : line_handler(get_string, c) ]);
}

mapping get_astring(function c)
{
  if (line)
  {
    string|object s = line->get_astring();
    if (stringp(s))
      return c(s);

    line = 0;
    return ([ "action" : "expect_literal",
	      "length" : s->length,
	      "handler" : get_string_handler(c) ]);
  }
  return ([ "action" : "expect_line",
	    "handler" : line_handler(get_astring, c) ]);
}

/* Value is a set object */
mapping get_set(function c)
{
  if (line)
    return c(line->get_set());

  return ([ "action" : "expect_line",
	    "handler" : line_handler(get_set, c) ]);
}
  
/* The values produced by these functions are mappings, all of which
 * includes a type field. */

class handle_literal
{
  function handler;
  
  void create(function h)
    {
      handler = h;
    }
  mixed `()(string s)
    {
      return handler( ([ "type" : "string", "string" : s ]) );
    }
}

/* The function c is called with the resulting mapping and the
 * optional arguments. */
  
class handle_list
{
  function c;
  
  void create(function _c)
    {
      c = _c;
    }

  mixed `()(array l)
    {
      return c( ([ "type" : "list", "list" : l ]) );
    }
}

class collect_list
{
  array l = ({ });

  int max_depth;
  int eol;

  function c;

  void create(int _max_depth, int _eol, function _c)
    {
      max_depth = _max_depth;
      eol = _eol;
      c = _c;
    }
    
  mapping append(mapping value)
    {
      if (value->eol)
	return c(l);
      l += ({ value });

      return collect();
    }
    
  mapping collect()
    {
      return get_any(max_depth, eol, append);
    }
}

/* Atoms with an option list */
class handle_options
{
  function c;
  mapping value;
    
  void create(mapping v, function _c)
    {
      value = v;
      c = _c;
    }

  mixed `()(array l)
    {
      value->options = l;
      /* line cannot be NULL, as the last token parsed is always ']',
       * not a literal */
      return c(line->get_range(value));
    }
}

mapping get_any(int max_depth, int eol, function c)
{
  if (!line)
    return ([ "action" : "expect_line",
	      "handler" : line_handler(get_any, max_depth, eol, c) ]);

  mapping t = line->get_token(eol);

  if (!t)
    return c(t);
  
  switch(t->type)
  {
  case "atom":
  case "string":
  case "eol":
    return c(t);
  case "literal":
    return ([ "action" : "expect_literal",
	      "length" : t->length,
	      "handler": handle_literal(c) ]);
  case "atom_options":
    if (max_depth > 0)
      return collect_list(max_depth - 1, ']', handle_options(t, c))->collect();
    else return 0;
  case "list":
    return collect_list(max_depth - 1, ')', handle_list(c))->collect();
  default:
    throw( ({ "IMAP: Internal error!\n", backtrace() }) );
  }
}
