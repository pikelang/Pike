/*
 * $Id: mktreeopt.pike,v 1.1 1999/11/07 23:56:05 grubba Exp $
 *
 * Generates tree-transformation code from a specification.
 *
 * Henrik Grubbström 1999-11-06
 */

mapping(string: mixed) rules = ([]);

void fail(string msg, mixed ... args)
{
  werror(msg, @args);
  exit(1);
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
				  (c == '*'))) {
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

class node
{
  int tag = -1;
  string token = "*";

  string tpos;

  array(string) extras = ({});

  object(node) car, cdr;	// 0 == Ignored.

  object(node) follow;

  string action;

  string _sprintf()
  {
    string s = token;
    if (tag >= 0) {
      s = sprintf("%d = %s", tag, token);
    }
    foreach(extras, string e) {
      s += "[" + e + "]";
    }
    if (!(< "*", "-" >)[token]) {
      s += "(";
      if (car) {
	s += car->_sprintf();
      } else {
	s += "-";
      }
      s += ", ";
      if (cdr) {
	s += cdr->_sprintf();
      } else {
	s += "-";
      }
      s += ")";
    }
    return s;
  }

  array(string) used_nodes()
  {
    if (token == "*") {
      return ({ sprintf("C%sR(n)", tpos) });
    }
    array(string) res = ({});
    if (car) {
      res += car->used_nodes();
    }
    if (cdr) {
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

object read_node()
{
  object res = node();

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
    res->token = read_id();
    eat_whitespace();
    c = data[pos];
    while (c == '[') {
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
      res->extras += ({ data[start..pos-2] });
      eat_whitespace();
      c = data[pos];
    }
    if (c == '(') {
      string otpos = tpos;
      pos++;

      tpos = "A"+otpos;
      res->car = read_node();

      expect(',');

      tpos = "D"+otpos;
      res->cdr = read_node();

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

object read_node2()
{
  object res = node();

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
  if (('0' <= c) && (c <= '9')) {
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

    werror(sprintf("%s:\n", n));

    if (rules[n->token]) {
      rules[n->token] += ({ n });
    } else {
      rules[n->token] = ({ n });
    }

    eat_whitespace();
    expect(':');
    eat_whitespace();

    object(node)|string action;

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

      // FIXME: Fix substitutions!
      action = data[start..pos-1];
    } else {
      object(node) n2 = read_node2();
      // werror(sprintf("\t%s;\n\n", n2));
      array(string) t = Array.uniq(n2->used_nodes());

      action = sprintf("{\n"
		       "  tmp1 = %s;\n"
		       "%s"
		       "  goto use_tmp1;\n"
		       "}",
		       n2->generate_code(),
		       sizeof(t)?("  " + (t * " = ") + " = 0;\n"):"");
    }

    while(n->cdr && (n->cdr->token != "-")) {
      n = n->cdr;
    }
    n->action = action;

    eat_whitespace();
    expect(';');
    eat_whitespace();
  }
}

string do_indent(string code, string indent)
{
  array a = code/"\n";
  for(int i = 0; i < sizeof(a); i++) {
    if (sizeof(a[i])) {
      a[i] = indent + a[i];
    }
  }
  return a*"\n";
}

string generate_back_match(array(object(node)) rule_set, string indent)
{
  string res = "";

  array(object(node)) back_nodes = rule_set->follow;
  int i;
  string post_res = "";
  while ((i = search(back_nodes, 0, i)) >= 0) {
    post_res += do_indent(rule_set[i]->action, indent) + "\n";
    i++;
  }
  back_nodes -= ({ 0 });
  if (sizeof(back_nodes)) {
    res += generate_cdr_match(back_nodes, indent);
  }
  res += post_res;

  return res;
}

string generate_cdr_match(array(object(node)) rule_set, string indent)
{
  mapping(string: array(object(node))) cdr_follow = ([]);

  foreach(rule_set, object(node) n) {
    string t;
    if (n->cdr) {
      // Follow.
      t = n->cdr->token;
      if (t != "-") {
	n = n->cdr;
      }
    } else {
      // Null.
      t = 0;
    }
    if (!cdr_follow[t]) {
      cdr_follow[t] = ({ n });
    } else {
      cdr_follow[t] += ({ n });
    }
  }

  string tpos = rule_set[0]->tpos;

  string res = "";

  array(object(node)) any_follow = cdr_follow["*"];
  m_delete(cdr_follow, "*");

  array(object(node)) back_follow = cdr_follow[0];
  m_delete(cdr_follow, 0);

  int last_was_if;

  if (cdr_follow["-"]) {

    res = sprintf("%sif (!CD%sR(n)) {\n", indent, tpos);

    res += generate_back_match(cdr_follow["-"], "  " + indent);

    m_delete(cdr_follow, "-");

    last_was_if = 1;

    res += indent + "}";
  }

  if (sizeof(cdr_follow)) {
    foreach(indices(cdr_follow), string t) {
      if (last_was_if) {
	res += " else ";
      } else {
	res += indent;
      }
      last_was_if = 1;
      res += sprintf("if (CD%sR(n)->token == %s) {\n",
		     tpos, t);

      res += generate_car_match(cdr_follow[t], "  " + indent);

      res += indent + "}";
    }
  }

  if (last_was_if) {
    res += "\n";
  }

  if (any_follow) {
    // This node is ignored, but nodes further down are interresting.
    res += generate_car_match(any_follow, indent);
  }

  if (back_follow) {
    // The cdr part is ignored.
    // Back up the tree.
    res += generate_back_match(back_follow, indent);
  }

  return res;
}

string generate_car_match(array(object(node)) rule_set, string indent)
{
  mapping(string: array(object(node))) car_follow = ([]);

  foreach(rule_set, object(node) n) {
    string t;
    if (n->car) {
      // Follow.
      t = n->car->token;
      if (t != "-") {
	n = n->car;
      }
    } else {
      // Null.
      t = 0;
    }
    if (!car_follow[t]) {
      car_follow[t] = ({ n });
    } else {
      car_follow[t] += ({ n });
    }
  }

  string tpos = rule_set[0]->tpos;

  string res = "";

  array(object(node)) any_follow = car_follow["*"];
  m_delete(car_follow, "*");

  array(object(node)) cdr_follow = car_follow[0];
  m_delete(car_follow, 0);

  int last_was_if;

  if (car_follow["-"]) {

    res = sprintf("%sif (!CA%sR(n)) {\n", indent, tpos);

    res += generate_cdr_match(car_follow["-"], "  " + indent);

    m_delete(car_follow, "-");

    last_was_if = 1;

    res += indent + "}";
  }

  if (sizeof(car_follow)) {
    foreach(indices(car_follow), string t) {
      if (last_was_if) {
	res += " else ";
      } else {
	res += indent;
      }
      last_was_if = 1;
      res += sprintf("if (CA%sR(n)->token == %s) {\n",
		     tpos, t);

      res += generate_car_match(car_follow[t], "  " + indent);

      res += indent + "}";
    }
  }

  if (last_was_if) {
    res += "\n";
  }

  if (any_follow) {
    // This node is ignored, but nodes further down are interresting.
    res += generate_car_match(any_follow, indent);
  }

  if (cdr_follow) {
    // The car part is ignored.
    // Generate code for the cdr part instead.
    res += generate_cdr_match(cdr_follow, indent);
  }

  return res;
}

string generate_code()
{
  string res = "";
  // Note: token below can't be 0 or *.
  foreach(indices(rules), string token) {
    res += "case " + token + ":\n" +
      generate_car_match(rules[token], "  ") +
      "  break;\n\n";
  }
  return res;
}

void generate_follow(object(node) n, object(node) follow, string tpos)
{
  n->tpos = tpos;
  n->follow = follow;
  if (n->car) {
    generate_follow(n->car, n, "A"+tpos);
  }
  if (n->cdr) {
    generate_follow(n->cdr, follow, "D"+tpos);
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
      generate_follow(n , 0, "");
    }
  }

  string result = generate_code();

  object dest = Stdio.File();

  fname = fname[..sizeof(fname)-4] + ".h";

  if ((!dest->open(fname, "wct")) ||
      (dest->write(result) != sizeof(result))) {
    fail("Failed to write file %O\n", fname);
  }
  exit(0);
}
