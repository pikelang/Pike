#define Node Parser.XML.Tree.Node
#define XML_ELEMENT Parser.XML.Tree.XML_ELEMENT
#define XML_TEXT Parser.XML.Tree.XML_TEXT
#define DEBUG

function resolve_reference; 
string image_path = "images/";

string image_prefix()
{
  return image_path;
} 

class Position {
  string file;
  int line;

  void update(Node n) {
    mapping m = n->get_attributes();
    if(!m->file || !m["first-line"]) {
      werror("Missing attribute in source-position element. %O\n", m);
      return;
    }
    file = m->file;
    line = (int) m["first-line"];
  }

  string get() {
    return file + ":" + line;
  }
}

Position position = Position();

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
    if(c->get_node_type()==XML_ELEMENT) {
      if(c->get_any_name()!="source-position")
	return c;
      else
	position->update(c);
    }
  throw( ({ "Node had no element child.\n", backtrace() }) );
}

string parse_appendix(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; APPENDIX <b>" +
      n->get_attributes()->name + "</b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = get_tag(n, "doc");
  if(c)
    ret += parse_text(c);
  else
    throw( ({ "No doc element in appendix.\n", backtrace() }) );

#ifdef DEBUG
  if(sizeof(get_tags(n, "doc"))>1)
    throw( ({ "More than one doc element in appendix node.\n", backtrace() }) );
#endif

  if(!noheader)
    ret = ret + "</dd></dl>"; 

  return ret;
}

string parse_module(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; MODULE <b>" +
      n->get_attributes()->name + "</b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = get_tag(n, "doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

#ifdef DEBUG
  if(sizeof(get_tags(n, "doc"))>1)
    throw( ({ "More than one doc element in module node.\n", backtrace() }) );
#endif

  ret += parse_children(n, "docgroup", parse_docgroup, noheader);
  ret += parse_children(n, "class", parse_class, noheader);
  ret += parse_children(n, "module", parse_module, noheader);

  if(!noheader)
    ret = ret + "</dd></dl>"; 

  return ret;
}

string parse_class(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; CLASS <b><font color='#005080'>" +
      quote(reverse(n->get_ancestors(1)->get_attributes()->name)[2..]*".") +
      "</font></b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = get_tag(n, "doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

#ifdef DEBUG
  if(sizeof(get_tags(n, "doc"))>1)
    throw( ({ "More than one doc element in class node.\n", backtrace() }) );
#endif

  if(get_tag(n, "inherit")) {
    ret += "<h3>Inherits</h3><ul>";
    foreach(n->get_children(), Node c)
      if(c->get_node_type()==XML_ELEMENT && c->get_any_name()=="inherit")
	ret += "<li>" + quote(get_tag(c, "classname")[0]->get_text()) + "</li>\n";
    ret += "</ul>\n";
  }

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "class", parse_class, noheader);

  if(!noheader)
    ret = ret + "</dd></dl>";
  return ret;
}

// ({  ({ array(string)+, void|string  })* })
string nicebox(array rows) {
  string ret = "<table bgcolor='black' border='0' cellpadding='0'><tr><td>\n"
    "<table cellspacing='1' cellpadding='2' border='0' bgcolor='white'>\n";

  int dim;
  foreach(rows, array row)
    dim = max(dim, sizeof(row));

#ifdef DEBUG
  if(dim!=1 && dim!=2)
    throw( ({ "Table has got wrong dimensions. ("+dim+")\n", backtrace() }) );
#endif

  foreach(rows, array row) {
    if(sizeof(row)==1)
      foreach(row[0], string elem)
	ret += "<tr><td><tt>" + elem + "</tt></td>" +
	  (dim==2?"<td>&nbsp;</td>":"") + "</tr>\n";
    else if(sizeof(row[0])==1)
      ret += "<tr valign='top'><td><tt>" + row[0][0] +
	"</tt></td><td>" + row[1] + "</td></tr>\n";
    else {
      ret += "<tr valign='top'><td><tt>" + row[0][0] +
	"</tt></td><td rowspan='" + sizeof(row[0]) + "'>" + row[1] + "</td></tr>\n";
      foreach(row[0][1..], string elem)
	ret += "<tr valign='top'><td><tt>" + elem + "</tt></td></tr>\n";
    }
  }

  return ret + "</table></td></tr></table>";
}

string build_box(Node n, string first, string second, function layout) {
  array rows = ({});
  foreach(get_tags(n, first), Node d) {
    array elems = ({});
    foreach(get_tags(d, second), Node e)
      elems += ({ layout(e) });
    if(get_tag(d, "text"))
      rows += ({ ({ elems, parse_text(get_tag(d, "text")) }) });
    else
      rows += ({ ({ elems }) });
  }
  return nicebox(rows);
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

    array rows;
    string name = c->get_any_name();
    switch(name) {
    case "text":
      ret += "<dd>" + parse_text(c) + "</dd>\n";
      break;

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
      if(resolve_reference) {
	ret += resolve_reference(parse_text(c), c->get_attributes());
	break;
      }
      string ref;
      ref = c->get_attributes()->resolved;
      if(!ref) ref = parse_text(c);
      ret += "<font face='courier'>" + ref + "</font>";
      break;

    case "dl":
      ret += "<dl>" + map(get_tags(c, "group"), parse_text)*"" + "</dl>";
      break;

    case "item":
      if(c->get_attributes()->name)
	ret += "<dt>" + c->get_attributes()->name + "</dt>\n";
#ifdef DEBUG
      if(c->count_children())
	throw( ({ "dl item has a child.\n", backtrace() }) );
#endif
      break;

    case "mapping":
      ret += build_box(c, "group", "member",
		       lambda(Node n) {
			 return "<font color='green'>" + parse_text(get_tag(n, "index")) +
			   "</font> : " + parse_type(get_first_element(get_tag(n, "type"))); } );
      break;

    case "array":
      ret += build_box(c, "group", "elem",
		       lambda(Node n) {
			 return parse_type(get_first_element(get_tag(n, "type"))) +
			   " <font color='green'>" + parse_text(get_tag(n, "index")) +
			   "</font>"; } );
      break;

    case "int":
      ret += build_box(c, "group", "value",
		       lambda(Node n) {
			 string tmp = "<font color='green'>";
			 Node min = get_tag(n, "minvalue");
			 Node max = get_tag(n, "maxvalue");
			 if(min || max) {
			   if(min) tmp += parse_text(min);
			   tmp += "..";
			   if(max) tmp += parse_text(max);
			 }
			 else
			   tmp += parse_text(n);
			 return tmp + "</font>";
		       } );
      break;

    case "mixed":
      ret += "<tt>" + c->get_attributes()->name + "</tt> can have any of the following types:<br />";
      rows = ({});
      foreach(get_tags(c, "group"), Node d)
	rows += ({ ({ ({ parse_type(get_first_element(get_tag(d, "type"))) }),
		      parse_text(get_tag(d, "text")) }) });
      ret += nicebox(rows);
      break;

    case "string": // Not in XSLT
      ret += build_box(c, "group", "value",
		       lambda(Node n) {
			 return "<font color='green'>" + parse_text(n) + "</font>";
		       } );
      break;

    case "multiset": // Not in XSLT
      ret += build_box(c, "group", "index",
		       lambda(Node n) {
			 return "<font color='green'>" +
			   parse_text(get_tag(n, "value")) + "</font>";
		       } );
      break;

    case "image": // Not in XSLT
      mapping m = c->get_attributes();
      m->src = image_prefix() + m_delete(m, "file");
      ret += sprintf("<img%{ %s='%s'%} />", (array)m);
      break;

    case "url": // Not in XSLT
#ifdef DEBUG
      if(c->count_children()!=1 && c->get_node_type()!=XML_ELEMENT)
	throw( ({ sprintf("url node has not one child. %O\n", c->html_of_node()),
		  backtrace() }) );
#endif
      m = c->get_attributes();
      if(!m->href)
	m->href=c[0]->get_text();
      ret += sprintf("<a%{ %s='%s'%}>%s</a>", (array)m, c[0]->get_text());

    case "section":
      //      werror(c->html_of_node()+"\n");
      // Found...
      break;

    case "ul":
      ret += "<ul>\n";
      foreach(c->get_elements("group"), Node c)
	ret += "<li>" + parse_text(c->get_first_element("text")) + "</li>";
      ret += "</ul>";
      break;

    case "ol":
      ret += "<ol>\n";
      foreach(c->get_elements("group"), Node c)
	ret += "<li>" + parse_text(c->get_first_element("text")) + "</li>";
      ret += "</ol>";
      break;

    case "source-position":
      position->update(c);
      break;

    // Not really allowed
    case "br":
      ret += sprintf("<%s%{ %s='%s'%} />", c->get_any_name(), (array)c->get_attributes());
      break;
    case "table":
    case "sub":
    case "sup":
    case "td":
    case "tr":
    case "th":
      ret += sprintf("<%s%{ %s='%s'%}>%s</%s>", c->get_any_name(), (array)c->get_attributes(),
		     parse_text(c), c->get_any_name());
      break;

    default:
#ifdef DEBUG
      werror("\n%s\n", (string)c);
      throw( ({ "Illegal element \"" + name + "\" in mode text.\n", backtrace() }) );
#endif
      break;
    }
  }

  return ret;
}

string parse_doc(Node n, void|int no_text) {
  string ret ="";

  Node c = n->get_first_element("text");
  if(c)
    ret += "<dt><font face='Helvetica'>Description</font><dt>\n"
      "<dd><font face='Helvetica'>" + parse_text(c) + "</font></dd>";

  foreach(n->get_elements("group"), Node c) {
    string name = c->get_first_element()->get_any_name();
    switch(name) {
    case "param":
      foreach(c->get_elements("param"), Node d)
	ret += "<dt><font face='Helvetica'>Parameter <tt><font color='#8000F0'>" +
	  quote(d->get_attributes()->name) + "</font></tt></font></dt><dd></dd>";
      ret += "<dd><font face='Helvetica'>" + parse_text(c->get_first_element("text")) +
	"</font></dd>";
      break;

    case "seealso":
      ret += "<dt><font face='Helvetica'>See also</font></dt>\n"
	"<dd><font face='Helvetica'>" + parse_text(c->get_first_element("text")) +
	"</font></dd>";
      break;

    case "fixme":
      ret += "<dt><font face='Helvetica' color='red'>FIXME</font></dt>\n"
	"<dd><font face='Helvetica' color='red'>" + parse_text(c->get_first_element("text")) +
	"</font></dd>";
      break;

    case "bugs":
    case "note":
    case "returns":
    case "throws":
      ret += "<dt><font face='Helvetica'>" + String.capitalize(name) +"</font></dt>\n"
	"<dd><font face='Helvetica'>" + parse_text(c->get_first_element("text")) +
	"</font></dd>";
      break;

    case "example":
      ret += "<dt><font face='Helvetica'>Example</font></dt>\n"
	"<dd><pre>" + parse_text(c->get_first_element("text")) + "</pre></dd>";
      break;

    default:
      throw( ({ "Unknown group type \"" + name + "\".\n", backtrace() }) );
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
      ret += "<font color='#005080'>object(" + n[0]->get_text() + ")</font>";
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
      ret += ":";
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
    ret += map(filter(n->get_children(),
		      lambda(Node n){
			return n->get_node_type()==XML_ELEMENT;
		      }), parse_type)*"|";
    break;

  case "string": case "void": case "program": case "mixed": case "float":
    ret += "<font color='#202020'>" + n->get_any_name() + "</font>";
    break;

  case "int":
    ret += "<font color='#202020'>int</font>";
    c = get_tag(n, "min");
    d = get_tag(n, "max");
    if(c) c=c[0];
    if(d) d=d[0];
    if(c && d)
      ret += "(" + c->get_text() + ".." + d->get_text() + ")";
    else if(c)
      ret += "(" + c->get_text() + "..)";
#ifdef DEBUG
    else if(d)
      throw( ({ "max element without min element in int node.\n", backtrace() }) );
#endif
    break;

  case "static": // Not in XSLT
    ret += "static";
    break;
  case "optional": // Not in XSLT
    ret += "optional";
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
    return ret + "()->";
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
      continue;

    case "source-position":
      position->update(c);
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
      ret += "<b><font color='#000066'>" + c->get_attributes()->name + "</font>(</b>";
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
      ret += "<font color='red'>Missing content (" + c->render_xml() + ")</font>";
      // Not implemented yet.
      break;

    default:
      throw( ({ "Illegal element " + c->get_any_name() + " in !doc.\n", backtrace() }) );
      break;
    }
  }

  return ret;
}

int foo;

string parse_docgroup(Node n) {
  mapping m = n->get_attributes();
  string ret = "\n\n<hr clear='all' />\n<dl><dt>";

  //  werror("%O\n", m["homogen-name"]);
  if(m["homogen-type"]) {
    string type = "<font face='Helvetica'>" + quote(String.capitalize(m["homogen-type"])) + "</font>\n";
    if(m["homogen-name"])
      ret += type + "<font size='+1'><b>" + quote((m->belongs?m->belongs+" ":"") + m["homogen-name"]) +
	"</b></font>\n";
    else
      foreach(Array.uniq(get_tags(n, "method")->get_attributes()->name), string name)
	ret += type + "<font size='+1'><b>" + name + "</b></font><br />\n";
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

string parse_children(Node n, string tag, function cb, mixed ... args) {
  string ret = "";
  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT && c->get_any_name()==tag)
      ret += cb(c, @args);

  return ret;
}

string start_parsing(Node n) {
  werror("Layouting...\n");
  string ret = "<html><head><title>Pike manual ?.?.?</title></head>\n"
    "<body bgcolor='white' text='black'>\n";

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "module", parse_module);
  ret += parse_children(n, "class", parse_class);
  ret += parse_children(n, "appendix", parse_appendix);

  return ret + "</body></html>\n";
}


int main(int num, array args) {

  int t = gethrtime();

  args = args[1..];
  foreach(args, string arg) {
    string tmp;
    if(sscanf(arg, "--img=%s", tmp)) {
      image_path = tmp;
      args -= ({ arg });
    }
    if(arg=="--help") {
      write(#"make_html.pike [args] <input file>
--img=<image path>
");
      return 0;
    }
  }

  if(!sizeof(args))
    throw( ({ "No input file given.\n", backtrace() }) );

  string file = Stdio.read_file(args[-1]);

  // We are only interested in what's in the
  // module container.
  werror("Parsing %O...\n", args[-1]);
  write(start_parsing( Parser.XML.Tree.parse_input(file)[0] ));
  werror("Took %.1f seconds.\n", (gethrtime()-t)/1000000.0);

  return 0;
}
