/* parse_line.pike
 *
 */

#pike __REAL_VERSION__

class partial_literal
{
  int length;

  void create(int l) { length = l; }
}

string buffer;

void create(string line)
{
  buffer = line;
}

void skip_whitespace()
{
  sscanf(buffer, "%*[ \t]%s", buffer);
}

#if 0
int eolp()
{
  skip_whitespace();
  return !sizeof(buffer);
}
#endif

// Returns -1 on error. All valid numbers ar non-negative.
int get_number()
{
  skip_whitespace();
      
  if (!sizeof(buffer))
    return -1;

  int i;
  for(i = 0; i<sizeof(buffer); i++)
    if ( (buffer[i] < '0') || ('9' < buffer[i]) )
      break;
      
  // Disallow too large numbers
  if (!i || (i > 9) )
    return -1;
      
  int res = array_sscanf(buffer[..i-1], "%d")[0];
  buffer = buffer[i..];
  return res;
}

string get_atom(int|void with_options)
{
  string atom;

  werror("get_atom: buffer = '%s'\n", buffer);

  sscanf(buffer,
	 (with_options
	  ? "%*[ \t]%[^][(){ \0-\037\177%\"]%s"
	  : "%*[ \t]%[^(){ \0-\037\177%\"]%s"),
	 atom, buffer);

  werror(sprintf("=> atom: %O\n", atom));
  return sizeof(atom) && atom;
}

string|object get_string()
{
  werror("get_string: buffer = '%s'\n", buffer);

  skip_whitespace();

  if (!sizeof(buffer))
    return 0;
      
  switch(buffer[0])
  {
  case '"':
  {
    int i = 0;
    while(1)
    {
      i = search(buffer, "\"", i+1);
      if (i<0)
	return 0;
      if (buffer[i-1] != '\\')
      {
	string res = replace(buffer[1..i-1],
			     ({ "\\\"", "\\\\" }),
			     ({ "\"", "\\" }) );
	buffer = buffer[i+1..];
	return res;
      }
    }
  }
  case '{':
  {
    if (buffer[sizeof(buffer)-1..] != "}")
      return 0;
    string n = buffer[1..sizeof(buffer)-2];

    buffer = "";
    if ( (sizeof(values(n) - values("0123456789")))
	 || (sizeof(n) > 9) )
      return 0;
    if (!sizeof(n))
      return 0;

    return partial_literal(array_sscanf(n, "%d")[0]);
  }
  }
}

string|object get_astring()
{
  werror("get_astring: buffer = '%s'\n", buffer);
      
  skip_whitespace();
  if (!sizeof(buffer))
    return 0;
  switch(buffer[0])
  {
  case '"':
  case '{':
    return get_string();
  default:
    return get_atom();
  }
}
  
/* Returns a set object. */
object get_set()
{
  string atom = get_atom();
  if (!atom)
    return 0;
      
  return .types.imap_set()->init(atom);
}

/* Parses an object that can be a string, an atom (possibly with
 * options in brackets) or a list.
 *
 * eol can be 0, meaning no end of line or list expected,
 * a positive int, meaning a character (e.g. ')' or ']' that terminates the list,
 * or -1, meaning that the list terminates at end of line.
 */
mapping get_token(int eol, int accept_options)
{
  skip_whitespace();
  if (!sizeof(buffer))
    return (eol == -1) && ([ "type" : "eol", "eol" : 1 ]);
  
  if (eol && (buffer[0] == eol))
  {
    buffer = buffer[1..];
    return ([ "type" : "eol", "eol" : 1 ]);
  }
  switch(buffer[0])
  {
  case '(':
    buffer = buffer[1..];
    return ([ "type" : "list", "list" : 1 ]);
  case '"': {
    string s = get_string();
    return s && ([ "type" : "string", "string" : s ]);
  }
  case '{': {
    object s = get_string();
    return s && ([ "type" : "literal", "length" : s->length ]);
  }
  default: {
    if (accept_options) {
      return get_atom_options(2);
    } else {
      string atom = get_atom(accept_options);
      return ([ "type" : "atom", "atom" : atom ]);
    }
  }
  }
}

/* Reads a <start.size> suffix */
mapping get_range(mapping atom)
{
  if (!sizeof(buffer) || (buffer[0] != '<'))
    return atom;

  buffer = buffer[1..];

  int start = get_number();
  if ((start < 0) || !sizeof(buffer) || (buffer[0] != '.'))
    return 0;

  buffer = buffer[1..];
      
  int size = get_number();
  if ((size <= 0) || !sizeof(buffer) || (buffer[0] != '>'))
    return 0;
      
  buffer = buffer[1..];

  atom->range = ({ start, size });

  return atom;
}

/* Get a list of atoms.
 * Primarily intended for use by STORE for the flags argument.
 */
array(string) get_flag_list()
{
  skip_whitespace();

  if (!sizeof(buffer) || (buffer[0] != '(') )
    return 0;

  buffer = buffer[1..];
  array(string) res = ({});

  skip_whitespace();

  string atom;
  while (atom = get_atom())
  {
    res += ({ atom });

    skip_whitespace();
  }
  skip_whitespace();	// This one shouldn't be needed, but...

  if (!sizeof(buffer))
    return 0;

  if (buffer[0] = ')')
  {
    buffer = buffer[1..];
    return res;
  }

  return 0;
}


/* Parses an object that (recursivly) can contain atoms (possible
 * with options in brackets) or lists. Note that strings are not
 * accepted, as it is a little difficult to wait for the
 * continuation of the request.
 *
 * FIXME: This function is used to read fetch commands. This breaks
 * rfc-2060 compliance, as the names of headers can be represented
 * as string literals. */
  
mapping get_simple_list(int max_depth)
{
  skip_whitespace();
  if (!sizeof(buffer))
    return 0;

  if (buffer[0] == '(') {
    /* Recurse */
    if (max_depth > 0) {
      array a = do_parse_simple_list(max_depth - 1, ')');
      return a && ([ "type": "list",
		     "list": a ]);
    } else {
      werror("get_simple_list(): Too deep recursion.\n");
      return 0;
    }
  }
  return 0;
  // return get_atom_options(max_depth-1);
}

array do_parse_simple_list(int max_depth, int terminator)
{
  array a = ({ });

  werror(sprintf("do_parse_simple_list(%d, '%c')\n", max_depth, terminator));
      
  buffer = buffer[1..];

  while(1)
  {
    skip_whitespace();
	
    if (!sizeof(buffer))
      return 0;
	
    if (buffer[0] == terminator)
    {
      buffer = buffer[1..];
      return a;
    }

    mapping atom = get_atom_options(max_depth-1);

    if (!atom) {
      return a;
    }
    a += ({ atom });

    mapping m = get_simple_list(max_depth);
    if (!m) {
      werror(sprintf("get_simple_list(%d) failed\n", max_depth));
    } else {
      a += ({ m });
    }
  }
}

/* Reads an atom, optionally followd by a list enclosed in square
 * brackets. Naturally, the atom itself cannot contain any brackets.
 *
 * Returns a mapping
 *    type : "atom",
 *    atom : name,
 *    raw : name[options]
 *    options : parsed options,
 *    range : ({ start, size })
 */
mapping get_atom_options(int max_depth)
{
  string atom = get_atom(1);
  if (!atom)
    return 0;

  mapping res = ([ "type" : "atom",
		   "atom" : atom ]);
  if (!sizeof(buffer) || (buffer[0] != '['))
  {
    res->raw = atom;
    return res;
  }
      
  /* Parse options */
  string option_start = buffer;
      
  array options = do_parse_simple_list(max_depth - 1, ']');

  if (!options) {
    werror("get_atom_options(): No options.\n");
    res->raw = atom + "[]";
    res->options = ({ ([ "type":"atom", "atom":"" ]) });
    return res;
  }

  res->options = options;
  res->raw = atom +
    option_start[..sizeof(option_start) - sizeof(buffer) - 1];

      
  if (!sizeof(buffer) || (buffer[0] != '<'))
    return res;

  /* Parse <start.size> suffix */
  buffer = buffer[1..];

  int start = get_number();
  if ((start < 0) || !sizeof(buffer) || (buffer[0] != '.'))
    return 0;

  buffer = buffer[1..];
      
  int size = get_number();
  if ((size < 0) || !sizeof(buffer) || (buffer[0] != '>'))
    return 0;
      
  buffer = buffer[1..];

  res->range = ({ start, size });

  return res;
}
