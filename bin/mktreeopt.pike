/*
 * $Id$
 *
 * Generates tree-transformation code from a specification.
 *
 * Henrik Grubbström 1999-11-06
 */

#pragma strict_types

/*
 * Input format:
 *
 * Input is first stripped of //-style comments.
 * Then it is parsed with the following pseudo-BNF grammar:
 *
 * source:
 *       | source rule;
 *
 * rule: match ':' transform ';'
 *     | match ':' action ';';
 *
 * match: tag '=' id conditionals children
 *      | tag conditionals children
 *      | id conditionals children
 *      | '$' tag conditionals children
 *      | '-' conditionals;
 *
 * tag: number
 *    | tag number;
 *
 * number: '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9';
 *
 * id: identifier
 *   | '\'' any '\''
 *   | '\'' '\\' any '\''
 *   | '*'
 *   | '+';
 *
 * identifier: uppercase
 *           | identifier uppercase;
 *
 * conditionals:
 *             | conditionals conditional;
 *
 * conditional: '[' expression ']';
 *
 * children:
 *         | '(' match ',' match ')';
 *
 * transform: identifier '(' transform ',' transform ')'
 *          | '\'' any '\'' '(' transform ',' transform ')'
 *          | '\'' '\\' any '\'' '(' transform ',' transform ')'
 *          | '-'
 *          | '$' tag
 *          | tag;
 *
 * action: '{' code '}';
 *
 * uppercase: 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' | 'I'
 *          | 'J' | 'K' | 'L' | 'M' | 'N' | 'O' | 'P' | 'Q' | 'R'
 *          | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' | 'Y' | 'Z' | '_';
 *
 */

/*
 * Matching:
 *
 * A tag without conditionals has the implicit symbol '*'.
 * A tag with conditionals has the implicit symbol '+'.
 * A rule must begin with a token match.
 *
 *   Symbol		Matches against
 *   ----------------------------------------------------------------------
 *   identifier		node->token.
 *   '\'' any '\''	node->token.
 *   '\'' '\\' any '\''	node->token.
 *   '-'		NULL.
 *   '*'		ANY.
 *   '+'		NOT NULL.
 *   '$' tag		Prior occurrance of the tag (SHARED_NODES).
 *
 */

/*
 * Conditionals:
 *
 * During evaluation of the expression $$ referrs to the current node,
 * and $tag to prior tagged nodes.
 *
 */

/*
 * Transforms:
 *
 *   Symbol		Resulting node
 *   ----------------------------------------------------------------------
 *   identifier		New node with identifier as the token.
 *   '\'' any '\''	New node with symbol as the token.
 *   '\'' '\\' any '\''	New node with symbol as the token.
 *   '$' tag		Node tagged during matching.
 *   tag		New integer node.
 *   '-'		NULL.
 *
 */

/*
 * Actions:
 *
 * During actions $tag referrs to nodes tagged during matching, and $$ to
 * the result node.
 *
 * NOTE: Code after the assignment of $$ will not be executed
 *       (a goto will be inserted).
 *
 * NOTE: $tag nodes used in the $$ assignment expression will be freed
 *       as needed.
 */

/*
 * Notes about the generated code:
 *
 * The node to be examined can have 4 basic configurations:
 *
 *      X      X      X      X
 *     / \    / \    / \    / \
 *    -   -  -   X  X   -  X   X
 *
 * The node is then matched against the 16 kinds of match nodes:
 *
 *      X      X      X      X
 *     / \    / \    / \    / \
 *    -   -  -   X  -   +  -   *
 *
 *      X      X      X      X
 *     / \    / \    / \    / \
 *    X   -  X   X  X   +  X   *
 *
 *      X      X      X      X
 *     / \    / \    / \    / \
 *    +   -  +   X  +   +  +   *
 *
 *      X      X      X      X
 *     / \    / \    / \    / \
 *    *   -  *   X  *   +  *   *
 *
 * Since + behaves closely like X, they will be grouped together in the
 * tables below.
 *
 * The match-order is determined by the manhattan distance from the node
 *
 *      X
 *     / \
 *    -   -
 *
 * in the table above. ie:
 *
 *      X        X                           X      X                    X
 *     / \  ->  / \                         / \    / \                  / \
 *    -   -    -   -                       -   *  *   -                *   *
 *
 *      X             	X                    X             X             X
 *     / \  ->         / \                  / \           / \           / \
 *    -   X           -   X                -   *         *   X         *   *
 *
 *      X                      X          	    X             X      X
 *     / \  ->                / \         	   / \           / \    / \
 *    X   -                  X   -        	  *   -         X   *  *   *
 *
 *      X                             X                    X      X      X
 *     / \  ->                       / \                  / \    / \    / \
 *    X   X                         X   X                *   X  X   *  *   *
 *
 * The first part of the table expanded with the implicit +-nodes:
 *
 *      X        X
 *     / \  ->  / \
 *    -   -    -   -
 *
 *      X             	X                    X
 *     / \  ->         / \                  / \
 *    -   X           -   X                -   +
 *
 *      X                      X          	    X
 *     / \  ->                / \         	   / \
 *    X   -                  X   -        	  +   -
 *
 *      X                             X                    X      X      X
 *     / \  ->                       / \                  / \    / \    / \
 *    X   X                         X   X                +   X  X   +  +   +
 *
 * Which would be much easier to generate code for, since every match node
 * occurrs exactly once, and no goto's would be needed.
 *
 *
 * Pseudocode: (Real code needs fixing...)
 *
 *   if (!car(n)) {
 *     if (!cdr(n)) {
 *       // Code for NULL-NULL
 *     } else {
 *       // Code for NULL-X
 *       // Code for NULL-PLUS
 *     }
 *     // Code for NULL-ANY
 *     if (!cdr(n)) {
 *       goto ANY_NULL;
 *     }
 *     goto ANY_X;
 *   } else if (!cdr(n)) {
 *     // Code for X-NULL
 *     // Code for PLUS-NULL
 *   ANY_NULL:
 *     // Code for ANY-NULL
 *     if (car(n)) {
 *       goto X_ANY;
 *     }
 *   } else {
 *     // Code for X-X
 *     // Code for X-PLUS
 *     // Code for PLUS-X
 *     // Code for PLUS-PLUS
 *   X_ANY:
 *     // Code for X-ANY
 *     // Code for PLUS-ANY
 *     if (cdr(n)) {
 *     ANY_X:
 *       // Code for ANY-X
 *       // Code for ANY-PLUS
 *     }
 *   }
 *   // Code for ANY-ANY
 */

constant header =
"/* Tree transformation code.\n"
" *\n"
" * This file was generated from %O by\n"
" * $Id$\n"
" *\n"
" * Do NOT edit!\n"
" */\n"
"\n";

mapping(string: array(object(node))) rules = ([]);

void fail(string msg, mixed ... args)
{
  werror(msg, @args);
  exit(7); /* distinctive error... */
}

string fname;
string data = "";
string tpos = "";
int pos;
int line = 1;

array(string) marks = allocate(10);

void eat_whitespace()
{
  while ((pos < sizeof(data)) && (< '\n', ' ', '\t', '\r' >)[data[pos]]) {
    line += (data[pos] == '\n');
    pos++;
  }
}

int read_int()
{
  int start = pos;

  while ((pos < sizeof(data)) && ('0' <= data[pos]) && (data[pos] <= '9')) {
    pos++;
  }
  return (int) data[start..pos-1];
}

string read_id()
{
  int start = pos;
  int c;

  if (data[pos] == '\'') {
    pos++;
    if (data[pos] == '\\') {
      pos++;
    }
    pos++;
    expect('\'');
    return data[start..pos-1] ;
  }

  while ((pos < sizeof(data)) && (((c = data[pos]) == '_') ||
				  (('A' <= c) && (c <= 'Z')) ||
				  (c == '*') || (c == '+'))) {
    pos++;
  }
  return data[start..pos-1];
}

void expect(int c)
{
  if (data[pos] != c) {
    fail("%s:%d: Expected '%c', got '%c'\n", fname, line, c, data[pos]);
  }
  pos++;
}

int node_cnt;

class node
{
  int tag = -1;
  string token = "*";

  string tpos;

  array(string) extras = ({});

  object(node)|string car, cdr;	// 0 == Ignored.

  object(node)|string real_car, real_cdr;

  object(node) parent;

  string action;

  int node_id = node_cnt++;

  object(node) _copy()
  {
    object(node) n = node();

    // werror(sprintf("Copy called on node %s\n", this_object()));

    n->tag = tag;
    n->tpos = tpos;
    n->token = token;
    n->extras = extras;
    if (objectp(real_car)) {
      n->real_car = real_car->_copy();
      n->real_car->parent = n;
      n->car = car && n->real_car;
    } else {
      n->real_car = real_car;
      n->car = car;
    }
    if (objectp(real_cdr)) {
      n->real_cdr = real_cdr->_copy();
      n->real_cdr->parent = n;
      n->cdr = cdr && n->real_cdr;
    } else {
      n->real_cdr = real_cdr;
      n->cdr = cdr;
    }
    n->action = action;

    return n;
  }

  object(node) copy()
  {
    if (parent) {
      if (parent->real_car == this_object()) {
	return [object(node)]parent->copy()->real_car;
      }
      return [object(node)]parent->copy()->real_cdr;
    }
    return _copy();
  }

  string _sprintf()
  {
    string s = token;
    if (tag >= 0) {
      s = sprintf("%d = %s", tag, token);
    }
    s += "{" + node_id + "}";
    foreach(extras, string e) {
      s += "[" + e + "]";
    }
    if (!(< "*", "-" >)[token]) {
      if (real_car || real_cdr) {
	s += "(";
	if (objectp(real_car)) {
	  s += real_car->_sprintf();
	} else if (real_car) {
	  s += "$" + real_car + "$";
	} else {
	  s += "*";
	}
	s += ", ";
	if (objectp(real_cdr)) {
	  s += real_cdr->_sprintf();
	} else if (real_cdr) {
	  s += "$" + real_cdr + "$";
	} else {
	  s += "*";
	}
	s += ")";
      }
    }
    return s;
  }

  array(string) used_nodes()
  {
    if (token == "*") {
      return ({ sprintf("C%sR(n)", tpos) });
    }
    array(string) res = ({});
    if (objectp(car)) {
      res += car->used_nodes();
    }
    if (objectp(cdr)) {
      res += cdr->used_nodes();
    }
    return res;
  }

  string generate_code()
  {
    if (token == "-")
      return "0";
    if (token == "*") {
      if (sizeof(tpos)) {
	return sprintf("C%sR(n)", tpos);
      } else {
	return "n";
      }
    }
    if (token == "T_INT") {
      return sprintf("mkintnode(%d)", tag);
    }
    return sprintf("mknode(%s, %s, %s)",
		   token,
		   car?car->generate_code():"0",
		   cdr?cdr->generate_code():"0");
  }
}

string fix_extras(string s)
{
  array(string) a = s/"$";

  for(int i=1; i < sizeof(a); i++) {
    string pos;
    if (a[i] == "") {
      // $$
      pos = tpos;
      i++;
    } else {
      int tag;
      sscanf(a[i], "%d%s", tag, a[i]);
      if (!(pos = marks[tag])) {
	fail("%s:%d: Unknown tag %d\n", fname, line, tag);
      }
    }
    if (sizeof(pos)) {
      a[i] = sprintf("C%sR(n)", pos) + a[i];
    } else {
      a[i] = "n" + a[i];
    }
  }
  return a * "";
}

object(node) read_node()
{
  object(node) res = node();

  eat_whitespace();
  int c = data[pos];

  if (('0' <= c) && (c <= '9')) {
    res->tag = read_int();

    if (marks[res->tag]) {
      fail("%s:%d: Tag %d used twice\n", fname, line, res->tag);
    }
    marks[res->tag] = tpos;

    eat_whitespace();
    c = data[pos];

    if ((c != '=') && (c != '['))
      return res;
    if (c == '=') {
      pos++;
      eat_whitespace();
    }

    c = data[pos];
  }
  if (c == '-') {
    res->token = data[pos..pos];
    pos++;
    eat_whitespace();
  } else {
    string token = read_id();
    if (sizeof(token)) {
      res->token = token;
    }
    eat_whitespace();
    c = data[pos];
    while (c == '[') {
      if (token == "*") {
	fail("%s:%d: *-nodes can't have conditionals.\n", fname, line);
      }
      if (res->token == "*") {
	// Implicit any node converted to plus node.
	res->token = "+";
      }
      pos++;
      int cnt = 1;
      int start = pos;
      while (cnt) {
	c = data[pos++];
	if (c == '[') {
	  cnt++;
	} else if (c == ']') {
	  cnt--;
	} else if (c == '\n') {
	  line++;
	}
      }
      res->extras += ({ fix_extras(data[start..pos-2]) });
      eat_whitespace();
      c = data[pos];
    }
    if (c == '(') {
      string otpos = tpos;
      pos++;

      tpos = "A"+otpos;

      if (data[pos] == '$') {
	// FIXME: Support for recurring nodes.
	// Useful for common subexpression elimination.
	pos++;
	int tag = read_int();
	string ntpos;
	if (!(ntpos = marks[tag])) {
	  fail("%s:%d: Tag $%d used before being defined.\n",
	       fname, line, tag);
	} else if (ntpos == "") {
	  fail("%s:%d: Tag $%d is the root, and can't be used for "
	       "exact matching.\n",
	       fname, line, tag);
	} else {	
	  // FIXME: Ought to check that the tag isn't for one of our parents.
	  res->car = res->real_car = sprintf("C%sR(n)", ntpos);
	}
	eat_whitespace();
      } else {
	res->car = res->real_car = read_node();
      }

      expect(',');
      eat_whitespace();

      tpos = "D"+otpos;

      if (data[pos] == '$') {
	// FIXME: Support for recurring nodes.
	// Useful for common subexpression elimination.
	pos++;
	int tag = read_int();
	string ntpos;
	if (!(ntpos = marks[tag])) {
	  fail("%s:%d: Tag $%d used before being defined.\n",
	       fname, line, tag);
	} else if (ntpos == "") {
	  fail("%s:%d: Tag $%d is the root, and can't be used for "
	       "exact matching.\n",
	       fname, line, tag);
	} else {	
	  // FIXME: Ought to check that the tag isn't for one of our parents.
	  res->cdr = res->real_cdr = sprintf("C%sR(n)", ntpos);
	}
	eat_whitespace();
      } else {
	res->cdr = res->real_cdr = read_node();
      }

      tpos = otpos;
      expect(')');

      eat_whitespace();
    }

    if ((res->token == "*") && !sizeof(res->extras) &&
	!res->car && !res->cdr) {
      // No need to consider this node.
      return 0;
    }
  }
  return res;
}

string fix_action(string s)
{
  array(string) a = s/"$$";

  for(int i=1; i < sizeof(a); i++) {
    int j = search(a[i], ";");
    if (j < 0) {
      fail("%s:%d: Syntax error in $$ statement\n", fname, line);
    }
    string new_node = a[i][..j];
    string rest = a[i][j+1..];

    array(string) b = new_node/"$";

    mapping(string:int) used_nodes = ([]);

    for(int j=1; j < sizeof(b); j++) {
      int tag = -1;
      sscanf(b[j], "%d%s", tag, b[j]);
      if (!marks[tag]) {
	fail("%s:%s: Unknown tag: $%d\n", fname, line, tag);
      }
      if (sizeof(marks[tag])) {
	string expr = sprintf("C%sR(n)", marks[tag]);
	used_nodes[expr]++;
	b[j] = expr + b[j];
      } else {
	fail("%s:%d: Use of the main node to generate a new node "
	     "is not supported\n",
	     fname, line);
      }
    }

    new_node = b * "";

    string pre_cleanup = "\n";
    string post_cleanup = "\n";

    if (sizeof(used_nodes)) {
      pre_cleanup = "\n";
      post_cleanup = "\n  ";
      foreach(indices(used_nodes), string used_node) {
	pre_cleanup += ("  ADD_NODE_REF2(" + used_node + ",\n")*
	  used_nodes[used_node];
	post_cleanup += ")" * used_nodes[used_node];
      }
      pre_cleanup += "  ";
      post_cleanup += ";\n";
    }
    a[i] = pre_cleanup +
      "  tmp1" + new_node +
      post_cleanup +
      "  goto use_tmp1;" + rest;
  }
  s = a * "";

  a = s/"$";

  for(int i=1; i < sizeof(a); i++) {
    int tag = -1;
    sscanf(a[i], "%d%s", tag, a[i]);
    if (!marks[tag]) {
      fail("%s:%s: Unknown tag: $%d\n", fname, line, tag);
    }
    if (sizeof(marks[tag])) {
      a[i] = sprintf("C%sR(n)", marks[tag]) + a[i];
    } else {
      a[i] = "n" + a[i];
    }
  }
  return a * "";
}

object(node) read_node2()
{
  object(node) res = node();

  eat_whitespace();
  int c = data[pos];

  if (c == '$') {
    pos++;
    res->tag = read_int();
    eat_whitespace();

    if (!marks[res->tag]) {
      fail("%s:%d: Undefined tag %d\n", fname, line, res->tag);
    }
    res->tpos = marks[res->tag];
      
    return res;
  }
  if (c == '-') {
    pos++;
    c = data[pos];
    if (('0' <= c) && (c <= '9')) {
      // Negative mkintnode.
      res->token = "T_INT";
      res->tag = -read_int();
      eat_whitespace();

      return res;
    } else {
      res->token = "-";
      eat_whitespace();

      return res;
    }
  } else if (('0' <= c) && (c <= '9')) {
    // mkintnode.
    res->token = "T_INT";
    res->tag = read_int();
    eat_whitespace();

    return res;
  }
  if (c == '-') {
    pos++;
    res->token = "-";
    eat_whitespace();

    return res;
  }

  res->token = read_id();
  eat_whitespace();
  c = data[pos];
  if (c == '(') {
    pos++;
    res->car = read_node2();

    expect(',');

    res->cdr = read_node2();

    expect(')');

    eat_whitespace();
  } else {
    res->car = node();
    res->cdr = node();
  }
  return res;
}


void parse_data()
{
  // Get rid of comments

  array(string) a = data/"//";

  for(int i=1; i < sizeof(a); i++) {
    int j = search(a[i], "\n");
    if (j >= 0) {
      // Note: Keep the NL.
      a[i] = a[i][j..];
    } else {
      a[i] = "";
    }
  }
  data = a*"";
  a = 0;

  eat_whitespace();
  while (pos < sizeof(data)) {
    marks = allocate(10);

    object(node) n = read_node();

    // werror(sprintf("%s:\n", n));

    if (rules[n->token]) {
      rules[n->token] += ({ n });
    } else {
      rules[n->token] = ({ n });
    }

    eat_whitespace();
    expect(':');
    eat_whitespace();

    string action;

    if (data[pos] == '{') {
      // Code.
      int cnt = 1;
      int start = pos++;
      while (cnt) {
	int c = data[pos++];
	if (c == '{') {
	  cnt++;
	} else if (c == '}') {
	  cnt--;
	} else if (c == '\n') {
	  line++;
	}
      }

      action = fix_action(data[start..pos-1]);
    } else if (data[pos] != ';') {
      object(node) n2 = read_node2();
      // werror(sprintf("\t%s;\n\n", n2));
      array(string) t = [array(string)]Array.uniq(n2->used_nodes());

      string expr = n2->generate_code();

      // Some optimizations for common cases
      switch(expr) {
      case "0":
	action = "goto zap_node;";
	break;
      case "CAR(n)":
	action = "goto use_car;";
	break;
      case "CDR(n)":
	action = "goto use_cdr;";
	break;
      default:
	string pre_fix_refs = "";
	string post_fix_refs = "";
	if (sizeof(t)) {
	  pre_fix_refs = "  ADD_NODE_REF2(" +
	    t * ",\n  ADD_NODE_REF2(" +
	    ",\n  ";
	  post_fix_refs = "  " + (")" * sizeof(t)) + ";\n";
	}
	action = sprintf("{\n"
			 "%s"
			 "  tmp1 = %s;\n"
			 "%s"
			 "  goto use_tmp1;\n"
			 "}",
			 pre_fix_refs,
			 expr,
			 post_fix_refs);
	break;
      }
      action = sprintf("#ifdef PIKE_DEBUG\n"
		       "  if (l_flag > 4) {\n"
		       "    fprintf(stderr, \"=> \"%O\"\\n\");\n"
		       "  }\n"
		       "#endif /* PIKE_DEBUG */\n", sprintf("%s", n2)) +
	action;
    } else {
      // Null action.
      // Used to force code generation for eg NULL-detection.
      // Obsolete.
      action = "";
    }

    action = sprintf("#ifdef PIKE_DEBUG\n"
		     "  if (l_flag > 4) {\n"
		     "    fprintf(stderr, \"Match: \"%O\"\\n\");\n"
		     "  }\n"
		     "#endif /* PIKE_DEBUG */\n", sprintf("%s", n)) +
      action;

    n->action = action;

    eat_whitespace();
    expect(';');
    eat_whitespace();
  }
}

string do_indent(string code, string indent)
{
  array(string) a = code/"\n";
  for(int i = 0; i < sizeof(a); i++) {
    if (sizeof(a[i]) && (a[i][0] != '#')) {
      a[i] = indent + a[i];
    }
  }
  return a*"\n";
}

void zap_car(array(object(node)) node_set)
{
  foreach(node_set, object(node) n) {
    n->car = 0;
  }
}

void zap_cdr(array(object(node)) node_set)
{
  foreach(node_set, object(node) n) {
    n->cdr = 0;
  }
}

constant NULL_CAR = 0;
constant EXACT_CAR = 1;
constant NULL_CDR = 2;
constant MATCH_CAR = 3;
constant NOT_NULL_CAR = 4;
constant EXACT_CDR = 5;
constant MATCH_CDR = 6;
constant NOT_NULL_CDR = 7;
constant ANY = 8;
constant ANY_CAR = 9;
constant ANY_CDR = 10;

static int label_cnt;

int debug;

string generate_match(array(object(node)) rule_set, string indent)
{
  string res = "";

  if (debug) {
    werror(indent + sprintf("generate_match(%s)\n",
			    rule_set->_sprintf() * ", "));
  }

  // Group the nodes by their class:

  array(array(object(node))) node_classes = allocate(11, ({}));

  foreach(rule_set, object(node) n) {
    int car_kind = ANY;
    if (n->car) {
      if (stringp(n->car)) {
	// EXACT
	car_kind = EXACT_CAR;
      } else if (n->car->token == "-") {
	// NULL
	car_kind = NULL_CAR;
      } else if (n->car->token == "+") {
	// PLUS
	car_kind = NOT_NULL_CAR;
      } else if (n->car->token != "*") {
	// X
	car_kind = MATCH_CAR;
      }
    }
    int cdr_kind = ANY;
    if (n->cdr) {
      if (stringp(n->cdr)) {
	// EXACT
	cdr_kind = EXACT_CDR;
      } else if (n->cdr->token == "-") {
	// NULL
	cdr_kind = NULL_CDR;
      } else if (n->cdr->token == "+") {
	// PLUS
	cdr_kind = NOT_NULL_CDR;
      } else if (n->cdr->token != "*") {
	// X
	cdr_kind = MATCH_CDR;
      }
    }
    if (car_kind < cdr_kind) {
      node_classes[car_kind] += ({ n });
    } else {
      node_classes[cdr_kind] += ({ n });
    }
    if (car_kind != cdr_kind) {
      if (car_kind == ANY && cdr_kind == NOT_NULL_CDR) {
	node_classes[ANY_CAR] += ({ n->copy() });
      } else if (cdr_kind == ANY && car_kind == NOT_NULL_CAR) {
	node_classes[ANY_CDR] += ({ n->copy() });
      }
    }
    if (debug) {
      werror(indent + sprintf("generate_match(%s)\n", n));
      werror(indent + sprintf("car_kind: %d, cdr_kind: %d\n",
			      car_kind, cdr_kind));
    }
  }

  if (debug) {
    werror(do_indent(sprintf("node_classes: %O\n", node_classes), indent));
  }

  string tpos = rule_set[0]->tpos;

  string label;
  int any_cdr_last;

  int last_was_if = 0;
  if (sizeof(node_classes[NULL_CAR]) ||
      sizeof(node_classes[MATCH_CAR]) ||
      sizeof(node_classes[NOT_NULL_CAR])) {
    res += indent + sprintf("if (!CA%sR(n)) {\n", tpos);

    if (sizeof(node_classes[NULL_CAR])) {
      zap_car([array(object(node))]node_classes[NULL_CAR]);

#if 0
      if (sizeof(node_classes[ANY_CAR]) < sizeof(node_classes[ANY_CDR])) {
#endif /* 0 */
	res += generate_match([array(object(node))]
			      node_classes[NULL_CAR] +
			      node_classes[ANY_CAR], indent + "  ");
#if 0
      } else {
	res += generate_match(node_classes[NULL_CAR], indent + "  ");
	if (sizeof(node_classes[ANY_CAR])) {
	  label = sprintf("any_car_%d", label_cnt++);
	  res += indent + "  goto " + label + ";\n";
	}
      }
#endif /* 0 */
    }
    res += indent + "}";
    last_was_if = 1;
  }

  if (sizeof(node_classes[EXACT_CAR])) {
    if (last_was_if) {
      res += " else ";
    } else {
      res += indent;
    }
    res += "{\n";
    indent += "  ";
    last_was_if = 0;
    mapping(string:array(object(node))) exacts = ([]);
    foreach(node_classes[EXACT_CAR], object(node) n) {
      exacts[n->car] += ({ n });
    }
    zap_car(node_classes[EXACT_CAR]);
    foreach(indices(exacts), string expr) {
      if (last_was_if) {
	res += " else ";
      } else {
	res += indent;
      }
      res +=
	sprintf("if ((CA%sR(n) == %s)\n"
		"#ifdef SHARED_NODES_MK2\n" + indent +
		"  || (CA%sR(n) && %s &&\n" + indent +
		"      ((CA%sR(n)->master?CA%sR(n)->master:CA%sR(n))==\n" +
		indent + "       (%s->master?%s->master:%s)))\n"
		"#endif /* SHARED_NODES_MK2 */\n" +
		indent + "  ) {\n",
		tpos, expr,
		tpos, expr,
		tpos, tpos, tpos,
		expr, expr, expr);
      res += generate_match(exacts[expr], indent + "  ");
      res += indent + "}";
    }
    res += "\n";
    last_was_if = 0;
  }

  if (sizeof(node_classes[NULL_CDR]) ||
      sizeof(node_classes[MATCH_CDR]) ||
      sizeof(node_classes[NOT_NULL_CDR])) {
    if (last_was_if) {
      res += " else ";
    } else {
      res += indent;
    }
    res += sprintf("if (!CD%sR(n)) {\n", tpos);
    if (sizeof(node_classes[NULL_CDR])) {
      zap_cdr(node_classes[NULL_CDR]);
#if 0
      if (label) {
#endif /* 0 */
	res += generate_match(node_classes[NULL_CDR] +
			      node_classes[ANY_CDR], indent + "  ");
#if 0
      } else {
	// We can use goto to handle ANY_CDR.
	res += generate_match(node_classes[NULL_CDR], indent + "  ");
	if (sizeof(node_classes[ANY_CDR])) {
	  label = sprintf("any_cdr_%d", label_cnt++);
	  res += indent + "  goto " + label + ";\n";
	  any_cdr_last = 1;
	}      
      }
#endif /* 0 */
    }
    res += indent + "}";
    last_was_if = 1;
  }

  if (sizeof(node_classes[EXACT_CDR])) {
    if (last_was_if) {
      res += " else ";
    } else {
      res += indent;
    }
    res += "{\n";
    indent += "  ";
    last_was_if = 0;
    mapping(string:array(object(node))) exacts = ([]);
    foreach(node_classes[EXACT_CDR], object(node) n) {
      exacts[n->cdr] += ({ n });
    }
    zap_cdr(node_classes[EXACT_CDR]);
    foreach(indices(exacts), string expr) {
      if (last_was_if) {
	res += " else ";
      } else {
	res += indent;
      }
      res+=
	sprintf("if ((CD%sR(n) == %s)\n"
		"#ifdef SHARED_NODES_MK2\n" + indent +
		"  || (CD%sR(n) && %s &&\n" + indent +
		"      ((CD%sR(n)->master?CD%sR(n)->master:CD%sR(n))==\n" +
		indent + "       (%s->master?%s->master:%s)))\n"
		"#endif /* SHARED_NODES_MK2 */\n" +
		indent + "  ) {\n",
		tpos, expr,
		tpos, expr,
		tpos, tpos, tpos,
		expr, expr, expr);
      res += generate_match(exacts[expr], indent + "  ");
      res += indent + "}";
    }
    res += "\n";
    last_was_if = 0;
  }

  if (sizeof(node_classes[MATCH_CAR]) || sizeof(node_classes[NOT_NULL_CAR]) ||
      sizeof(node_classes[MATCH_CDR]) || sizeof(node_classes[NOT_NULL_CDR])) {
    if (last_was_if) {
      res += " else {\n";
    } else {
      res += indent + "{\n";
    }
    if (sizeof(node_classes[MATCH_CAR])) {
      mapping(string:array(object(node))) token_groups = ([]);
      foreach(node_classes[MATCH_CAR], object(node) n) {
	token_groups[n->car->token] += ({ n->car });
      }
      zap_car(node_classes[MATCH_CAR]);
      int last_was_if2;
      foreach(sort(indices(token_groups)), string token) {
	if (last_was_if2) {
	  res += " else ";
	} else {
	  res += indent + "  ";
	}
	last_was_if2 = 1;
	res += sprintf("if (CA%sR(n)->token == %s) {\n", tpos, token);
	res += generate_extras_match(token_groups[token], indent + "    ");
	res += indent + "  }";
      }
      res += "\n";
    }
    if (sizeof(node_classes[MATCH_CDR])) {
      mapping(string:array(object(node))) token_groups = ([]);
      foreach(node_classes[MATCH_CDR], object(node) n) {
	token_groups[n->cdr->token] += ({ n->cdr });
      }
      zap_cdr(node_classes[MATCH_CDR]);
      int last_was_if2;
      foreach(sort(indices(token_groups)), string token) {
	if (last_was_if2) {
	  res += " else ";
	} else {
	  res += indent + "  ";
	}
	last_was_if2 = 1;
	res += sprintf("if (CD%sR(n)->token == %s) {\n", tpos, token);
	res += generate_extras_match(token_groups[token], indent + "    ");
	res += indent + "  }";
      }
      res += "\n";
    }
    array(object(node)) not_null = ({});
    if (sizeof(node_classes[NOT_NULL_CAR])) {
      foreach(node_classes[NOT_NULL_CAR], object(node) n) {
	not_null += ({ n->car });
	n->car = 0;
      }
    }
    if (sizeof(node_classes[NOT_NULL_CDR])) {
      foreach(node_classes[NOT_NULL_CDR], object(node) n) {
	not_null += ({ n->cdr });
	n->cdr = 0;
      }
    }
    if (sizeof(not_null)) {
      res += generate_extras_match(not_null, indent + "  ");
    }
    res += indent + "}";
    last_was_if = 1;
  }
  if (last_was_if) {
    res += "\n";
  }
  if (sizeof(node_classes[EXACT_CDR])) {
    indent = indent[2..];
    res += indent + "}\n";
  }
  if (sizeof(node_classes[EXACT_CAR])) {
    indent = indent[2..];
    res += indent + "}\n";
  }
  if (sizeof(node_classes[ANY])) {
    array(object(node)) parent_set = ({});
    array(object(node)) car_set = ({});
    array(object(node)) cdr_set = ({});
    foreach(node_classes[ANY], object(node) n) {
      if (n->car) {
	car_set += ({ n->car });
	n->car = 0;
      } else if (n->cdr) {
	cdr_set += ({ n->cdr });
	n->cdr = 0;
      } else if (n->parent) {
	parent_set += ({ n->parent });
      } else {
	res += do_indent(n->action, indent) + "\n";
	n->action = 0;
      }
    }
    if (sizeof(car_set) || sizeof(cdr_set)) {
      res += generate_extras_match(car_set + cdr_set, indent);
    }
    if (sizeof(parent_set)) {
      res += generate_match(parent_set, indent);
    }
  }
  // werror(res + "\n");
  return res;
}

string generate_extras_match(array(object(node)) rule_set, string indent)
{
  string res = "";

  mapping(string:array(object(node))) extra_set = ([]);

  array(object(node)) no_extras = ({});

  foreach(rule_set, object(node) n) {
    string t = 0;
    if (n->extras && sizeof(n->extras)) {
      extra_set[n->extras * (") &&\n" +
			     indent + "    (")] += ({ n });
      continue;
    }
    no_extras += ({ n });
  }

  if (debug) {
    werror(do_indent(sprintf("extra_set: %O\n", extra_set), indent));
  }

  if (sizeof(no_extras)) {
    res += generate_match(no_extras, indent);
  }

  foreach(sort(indices(extra_set)), string code) {
    res += indent + sprintf("if ((%s)) {\n", code);
    res += generate_match(extra_set[code], indent + "  ");
    res += indent + "}\n";
  }

  return res;
}

string generate_code()
{
  string res = "";
  // Note: token below can't be 0 or *.
  foreach(sort(indices(rules)), string token) {
    // debug = (token == "'?'");
    res += "case " + token + ":\n" +
      generate_extras_match(rules[token], "  ") +
      "  break;\n\n";
  }
  return res;
}

void generate_parent(object(node) n, object(node) parent, string tpos)
{
  n->tpos = tpos;
  n->parent = parent;
  if (objectp(n->car)) {
    generate_parent([object(node)]n->car, n, "A"+tpos);
  }
  if (objectp(n->cdr)) {
    generate_parent([object(node)]n->cdr, n, "D"+tpos);
  }
}

int main(int argc, array(string) argv)
{
  if (argc < 2) {
    fail("Expected filename\n");
  }

  fname = argv[1];

  if (fname[sizeof(fname)-3..] != ".in") {
    fail("Filename %O doesn't end with \".in\"\n", fname);
  }

  data = Stdio.File(fname, "r")->read();

  parse_data();

  foreach(values(rules), array(object(node)) nodes) {
    foreach(nodes, object(node) n) {
      generate_parent(n , 0, "");
    }
  }

  string result = sprintf(header, fname) + generate_code();

  object(Stdio.File) dest = Stdio.File();

  fname = fname[..sizeof(fname)-4] + ".h";

  if ((!dest->open(fname, "wct")) ||
      (dest->write(result) != sizeof(result))) {
    fail("Failed to write file %O\n", fname);
  }
  exit(0);
}
