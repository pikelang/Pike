#define Node Parser.XML.Tree.Node
#define XML_ELEMENT Parser.XML.Tree.XML_ELEMENT
#define XML_TEXT Parser.XML.Tree.XML_TEXT
#define DEBUG

string quote(string in) {
  if(in-" "-"\t"=="") return "";
  if(String.trim_all_whites(in)=="") return "\n";
  return Parser.XML.Tree.text_quote(in);
}

Node get_tag(Node n, string name) {
  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT && c->get_any_name()==name)
      return c;
  return 0;
}

array(Node) get_tags(Node n, string name) {
  array nodes = ({});
  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT && c->get_any_name()==name)
      nodes += ({ c });
  return nodes;
}

Node get_first_element(Node n) {
  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT && c->get_any_name()!="source-position")
      return c;
  throw( ({ "Node had no element child.\n", backtrace() }) );
}

string parse_module(Node n) {
  string ret = "<dl><dt>"
    "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
    "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; MODULE <b>" +
    n->get_attributes()->name + "</b></font></td></tr></table><br />\n"
    "</dt><dd>";

  Node c = get_tag(n, "doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  // Check for more than one doc.

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "class", parse_class);
  ret += parse_children(n, "module", parse_module);

  return ret + "</dd></dl>";
}

string parse_class(Node n) {
  string ret = "<dl><dt>"
    "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
    "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; CLASS <b><font color='#005080'>" +
    quote(reverse(n->get_ancestors(1)->get_attributes()->name)[2..]*".") +
    "</font></b></font></td></tr></table><br />\n"
    "</dt><dd>";

  Node c = get_tag(n, "doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  // Check for more than one doc.

  if(get_tag(n, "inherit")) {
    ret += "<h3>Inherits</h3><ul>";
    foreach(n->get_children(), Node c)
      if(c->get_node_type==XML_ELEMENT && n->get_any_name()=="inherit")
	ret += "<li>" + quote(c->get_attributes()->classname) + "</li>\n";
    ret += "</ul>\n";
  }

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "class", parse_class);

  return ret + "</dd></dl>";
}

string parse_text(Node n) {
  string ret = "";

  if(n->get_node_type()==XML_TEXT && n->get_text())
    return quote(n->get_text());

  foreach(n->get_children(), Node c) {
    int node_type = c->get_node_type();
    if(c->get_node_type()==XML_TEXT) {
      ret += quote(c->get_text());
      continue;
    }

#ifdef DEBUG
    if(c->get_node_type()!=XML_ELEMENT) {
      throw( ({ "Forbidden node type " + c->get_node_type() + " in doc node.\n", backtrace() }) );
    }
#endif

    string name = c->get_any_name();
    switch(name) {
    case "text":
      ret += "<dd>" + parse_text(c) + "</dd>\n";
    case "p":
    case "b":
    case "i":
    case "tt":
      ret += "<"+name+">" + parse_text(c) + "</"+name+">";
      break;
    case "pre":
    case "code":
      ret += "<font face='courier'><"+name+">" + parse_text(c) + "</"+name+"></font>";
      break;
    case "ref":
      ret += "<font face='courier' size='-1'>" + parse_text(c) + "</font>";
      break;
    case "dl":
      // Not implemented yet.
      break;
    case "item":
      ret += "<dt>" + parse_text(c) + "</dt>\n";
      break;

    case "mapping":
    case "multiset": // Not in XSLT
    case "array":
    case "int":
    case "string": // Not in XSLT
    case "mixed":
      // Not implemented yet.
      break;

    case "image": // Not in XSLT
      mapping m = c->get_attributes();
      m->src = m_delete(m, "file");
      ret += sprintf("<img%{ %s='%s'%} />", (array)m);
      break;

    case "section":
    case "br":
    case "a":
    case "table":
    case "ul":
    case "url":
    case "wbr":
    case "sub":
    case "sup":
    case "ol":
      // Found...
      break;
    case "source-position":
      // Ignore...
      break;
    default:
#ifdef DEBUG
      throw( ({ "Illegal element \"" + name + "\" in mode text.\n", backtrace() }) );
#endif
      break;
    }
  }

  return ret;
}

string parse_doc(Node n, void|int no_text) {
  string ret = "";

#ifdef DEBUG
  int text;
#endif

  if(!no_text) {
    Node c = get_tag(n, "text");
    if(c)
      ret += "<dt><font face='Helvetica'>Description</font></dt>\n"
	"<dd><font face='Helvetica'>" + parse_text(c) + "</font></dd>";
  }

  foreach(n->get_children(), Node c) {
    int node_type = c->get_node_type();
    if(c->get_node_type()==XML_TEXT) {
      if(sizeof(String.trim_all_whites(c->get_text())))
	ret += c->get_text();
      continue;
    }

#ifdef DEBUG
    if(c->get_node_type()!=XML_ELEMENT) {
      throw( ({ "Forbidden node type " + c->get_node_type() + " in doc node.\n", backtrace() }) );
    }
#endif

    switch(c->get_any_name()) {
    case "text":
      if(text++)
	throw( ({ "More than one 'text' node in doc node.\n", backtrace() }) );
      break;
    case "group":
      Node cc = get_tag(c, "text");
      ret += parse_doc(c, 1);
      if(cc)
	ret += "<dd><font face='Helvetica'>" + parse_text(cc) + "</font></dd>\n";
      break;
    case "param":
      ret += "<dt><font face='Helvetica'>Parameter <tt><font color='#8000F0'>" +
	quote(c->get_attributes()->name) + "</font></tt></font></dt><dd></dd>";
      break;
    case "seealso":
      ret += "<dt><font face='Helvetica'>See also</font></dt>\n";
      break;
    default:
      if(c->get_parent()->get_any_name()=="group") {
	ret += "<dt><font face='Helvetica'>" +
	  String.capitalize( c->get_any_name() ) + "</font></dt>";
	parse_doc(c);
      }
#ifdef DEBUG
      else
	throw( ({ "Illegal element " + c->get_any_name() + ".\n", backtrace() }) );
#endif
    }
  }

  return ret;
}

string parse_type(Node n, void|string debug) {
  if(n->get_node_type()!=XML_ELEMENT)
    return "";

  string ret = "";
  Node c, d;
  switch(n->get_any_name()) {

  case "object":
    if(n->count_children())
      ret += "<font color='#005080'>" + n[0]->get_text() + "</font>";
    else
      ret += "<font color='#202020'>object</font>";
    break;

  case "multiset":
    ret += "<font color='#202020'>multiset</font>";
    c = get_tag(n, "indextype");
    if(c) ret += "(" + parse_type( get_first_element(c) ) + ")";
    break;

  case "array":
    ret += "<font color='#202020'>array</font>";
    c = get_tag(n, "valuetype");
    if(c) ret += "(" + parse_type( get_first_element(c) ) + ")";
    break;

  case "mapping":
    ret += "<font color='#202020'>mapping</font>";
    c = get_tag(n, "indextype");
    d = get_tag(n, "valuetype");
    if(c && d)
      ret += "(" + parse_type( get_first_element(c) ) + ":" +
	parse_type( get_first_element(d) ) + ")";
#ifdef DEBUG
    if( !c != !d )
      throw( ({ "Indextype/valuetype defined while the other is not in mapping.\n",
		backtrace() }) );
#endif
    break;

  case "function":
    ret += "<font color='#202020'>function</font>";
    c = get_tag(n, "argtype");
    d = get_tag(n, "returntype");
    // Doing different than the XSL here. Must both
    // argtype and returntype be defined?
    if(c || d) {
      ret += "(";
      if(c) ret += map(c->get_children(), parse_type)*", ";
      ret += " : ";
      if(d) ret += parse_type( get_first_element(d) );
      ret += ")";
    }
    break;

  case "varargs":
#ifdef DEBUG
    if(!n->count_children())
      throw( ({ "varargs node must have child node.\n", backtrace() }) );
#endif
    ret += parse_type(get_first_element(n)) + " ... ";
    break;

  case "or":
    ret += map(n->get_children(), parse_type)*"|";
    break;

  case "string": case "void": case "program": case "mixed": case "float":
    ret += "<font color='#202020'>" + n->get_any_name() + "</font>";
    break;

  case "int":
    ret += "<font color='#202020'>int</font>";
    // min/max ...
    break;

  case "static": // Not in XSLT
    ret += "static";
    break;

  default:
    throw( ({ "Illegal element " + n->get_any_name() + " in mode type.\n", backtrace() }) );
    break;
  }
  return ret;
}

string render_class_path(Node n) {
  array a = reverse(n->get_ancestors(0));
  if(sizeof(a)<4) return "";
  string ret = a[2..sizeof(a)-2]->get_attributes()->name * ".";
  if(n->get_parent()->get_parent()->get_any_name()=="class")
    return ret + "->";
  if(n->get_parent()->get_parent()->get_any_name()=="module")
    return ret + ".";
#ifdef DEBUG
  throw( ({ "Parent module is " + n->get_parent()->get_any_name() + ".\n",
	   backtrace() }) );
#else
    return "";
#endif
}

string parse_not_doc(Node n) {
  string ret = "";
  int method, argument, variable, const;

  foreach(n->get_children(), Node c) {

    if(c->get_node_type()!=XML_ELEMENT)
      continue;

    Node cc;
    switch(c->get_any_name()) {
    case "doc":
    case "source-position":
      continue;
    case "method":
      if(method++) ret += "<br />\n";
#ifdef DEBUG
      if(!get_tag(c, "returntype"))
	continue;
	//	throw( ({ "No returntype element in method element.\n", backtrace() }) );
#endif
      ret += "<tt>" + parse_type(get_first_element(get_tag(c, "returntype"))); // Check for more children
      ret += " ";
      ret += render_class_path(c);
      ret += "<b><font color='#0000EE'>" + c->get_attributes()->name + "</font>(</b>";
      ret += parse_not_doc( get_tag(c, "arguments") );
      ret += "<b>)</b></tt>";
      break;

    case "argument":
      if(argument++) ret += ", ";
      cc = get_tag(c, "value");
      if(cc) ret += "<font color='green'>" + cc[0]->get_text() + "</font>";
      else if( !c->count_children() );
      else if( get_first_element(c)->get_any_name()=="type" && c->get_attributes()->name) {
	ret += parse_type(get_first_element(get_first_element(c))) + " <font color='#005080'>" +
	  c->get_attributes()->name + "</font>";
      }
      else
	throw( ({ "Malformed argument element.\n" + c->html_of_node() + "\n", backtrace() }) );
      break;

    case "variable":
      if(variable++) ret += "<br />\n";
      ret += "<tt>";
      cc = get_tag(c, "modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += parse_type(get_first_element(get_tag(c, "type")), "variable") + " " +
	render_class_path(c) + "<b><font color='#F000F0'>" + c->get_attributes()->name +
	"</font></b></tt>";
      break;

    case "constant":
      if(const++) ret += "<br />\n";
      ret += "<tt>constant ";
      ret += render_class_path(c);
      ret += "<font color='#F000F0'>" + c->get_attributes()->name + "</font>";
      cc = get_tag(c, "typevalue");
      if(cc) ret += " = " + parse_type(get_first_element(cc));
      ret += "</tt>";
      break;

    case "typedef":
    case "inherit":
      // Not implemented yet.
      break;

    default:
      throw( ({ "Illegal element " + c->get_any_name() + " in !doc.\n", backtrace() }) );
      break;
    }
  }

  return ret;
}

string parse_docgroup(Node n) {
  mapping m = n->get_attributes();
  string ret = "\n\n<hr clear='all' />\n<dl><dt>";

  if(m["homogen-type"]) {
    ret += "<font face='Helvetica'>" + quote(String.capitalize(m["homogen-type"])) + "</font>\n";
    if(m["homogen-name"])
      ret += "<font size='+1'><b>" + quote((m->belongs?m->belongs+" ":"") + m["homogen-name"]) +
	"</b></font>\n";
  }
  else
    ret += "syntax";

  ret += "</dt>\n<dd><p>";

  ret += parse_not_doc(n);

  ret += "</p></dd>\n";

  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT && c->get_any_name()=="doc")
      ret += parse_doc(c);

  return ret + "</dl>\n";
}

string parse_children(Node n, string tag, function cb) {
  string ret = "";
  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT && c->get_any_name()==tag)
      ret += cb(c);
  return ret;
}

string start_parsing(Node n) {
  werror("Layouting...\n");
  string ret = "<html><head><title>Pike manual ?.?.?</title></head>\n"
    "<body bgcolor='white' text='black'>\n";

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "module", parse_module);
  ret += parse_children(n, "class", parse_class);

  return ret + "</body></html>\n";
}


int main() {

  int t = gethrtime();
  string file = Stdio.read_file("../manual.xml");

  // We are only interested in what's in the
  // module container.
  werror("Parsing file...\n");
  write(start_parsing( Parser.XML.Tree.parse_input(file)[0] ));
  werror("Took %.1f seconds.\n", (gethrtime()-t)/1000000.0);

  return 0;
}
