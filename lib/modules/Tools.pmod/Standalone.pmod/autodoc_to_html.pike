#pike __REAL_VERSION__

//! AutoDoc XML to HTML converter.
//!
//! @seealso
//!    @[tree_split]

constant description = "AutoDoc XML to HTML converter.";

#define Node Parser.XML.Tree.SimpleNode
#define XML_COMMENT Parser.XML.Tree.XML_COMMENT
#define XML_ELEMENT Parser.XML.Tree.XML_ELEMENT
#define XML_TEXT Parser.XML.Tree.XML_TEXT
#define DEBUG

Tools.AutoDoc.Flags flags = Tools.AutoDoc.FLAG_NORMAL;

int verbosity = Tools.AutoDoc.FLAG_VERBOSE; //NORMAL;

function resolve_reference; 
string image_path = "images/";
string dest_path;
string default_ns;

//! Layout template headers and trailers.
mapping lay = ([
 "docgroup" : "\n\n<hr clear='all' size='1' noshadow='noshadow' />\n<dl>",
 "_docgroup" : "</dl>\n",
 "ndochead" : "<dd><p>",
 "_ndochead" : "</p></dd>\n",

 "dochead" : "\n<dt style='font-family: sans-serif'>",
 "_dochead" : "</dt>\n",
 "typehead" : "\n<dt style='font-family: sans-serif'>",
 "_typehead" : "</dt>\n",
 "docbody" : "<dd style='font-family: sans-serif'>",
 "_docbody" : "</dd>",
 "fixmehead" : "<dt style='font-family: sans-serif; color: red'>",
 "_fixmehead" : "</dt>\n",
 "fixmebody" : "<dd style='font-family: sans-serif; color: red'>",
 "_fixmebody" : "</dd>",

 "parameter" : "<tt><font color='#8000F0'>",
 "_parameter" : "</font></tt>",
 "example" : "<dd><pre>",
 "_example" : "</pre></dd>",

 "pre" : "<font face='courier'><pre>",
 "_pre" : "</pre></font>",
 "code" : "<font face='courier'><pre><code>",
 "_code" : "</code></pre></font>",
 "expr" : "<font face='courier'><code>",
 "_expr" : "</code></font>",

]);

//! Path to where in the destination filesystem the images are.
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

string render_tag(string tag, mapping(string:string) attrs, int|void term)
{
  string res = "<" + tag;
  foreach(sort(indices(attrs)), string attr) {
    res += " " + attr + "='" + attrs[attr] + "'";
  }
  if (term) res += " /";
  return res + ">";
}

Node get_first_element(Node n)
{
  if (!n) return UNDEFINED;
  foreach(n->get_children(), Node c)
    if(c->get_node_type()==XML_ELEMENT) {
      if(c->get_any_name()!="source-position")
	return c;
      else
	position->update(c);
    }
  return UNDEFINED;
}

int section, subsection;

string low_parse_chapter(Node n, int chapter) {
  string ret = "";
  Node dummy = Node(XML_ELEMENT, "dummy", ([]), "");
  foreach(n->get_elements(), Node c)
    switch(c->get_any_name()) {

    case "p":
      dummy->replace_children( ({ c }) );
      ret += parse_text(dummy);
      break;

    case "example":
      ret += "<p><pre>" + quote(c->value_of_node()) + "</pre></p>\n";
      break;

    case "ul":
    case "ol":
      ret += (string)c;
      break;

    case "dl":
      ret += "<dl>\n";
      foreach(c->get_elements(), Node cc) {
	string tag = cc->get_any_name();
	if(tag!="dt" && tag!="dd")
	  error("dl contained element %O.\n", tag);
	ret += "<" + tag + ">" + parse_text(cc) + "</" + tag + ">\n";
      }
      ret += "</dl>";
      break;

    case "matrix":
      ret += layout_matrix( map(c->get_elements("r")->get_elements("c"), map, parse_text) );
      break;

    case "section":
      if(section)
	error("Section inside section.\n");
      if(subsection)
	error("Section inside subsection.\n");
      section = (int)c->get_attributes()->number;
      ret += "</dd>\n<dt><a name='" + section + "'></a>\n"
	"<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
	"<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; " + chapter + "." + section +
	". " + quote(c->get_attributes()->title ||
		     // The following for bug compat.
		     c->get_attributes()->name) +
	"</font></td></tr></table><br />\n"
	"</dt>\n<dd>";
      ret += low_parse_chapter(c, chapter);
      section = 0;
      break;

    case "subsection":
      if(!section)
	error("Subsection outside section.\n");
      if(subsection)
	error("Subsection inside subsection.\n");
      subsection = (int)c->get_attributes()->number;
      ret += "</dd><dt>"
	"<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
	"<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; " + chapter + "." + section + "." + subsection +
	". " + quote(c->get_attributes()->title) +
	"</font></td></tr></table><br />\n"
	"</dt><dd>";
      ret += low_parse_chapter(c, chapter);
      subsection = 0;
      break;

    case "docgroup":
      ret += parse_docgroup(c);
      break;

    case "autodoc":
      ret += parse_autodoc(c);
      break;

    case "namespace":
      ret += parse_namespace(c);
      break;

    case "module":
      ret += parse_module(c);
      break;

    case "class":
      ret += parse_class(c);
      break;

    case "enum":
      ret += parse_enum(c);
      break;

    default:
      if (!(flags & Tools.AutoDoc.FLAG_KEEP_GOING)) {
	error("Unknown element %O.\n", c->get_any_name());
      }
      break;
    }
  return ret;
}

string parse_chapter(Node n, void|int noheader) {
  string ret = "";
  if(!noheader) {
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; ";
    if(n->get_attributes()->number)
      ret += n->get_attributes()->number + ". ";
    ret += quote(n->get_attributes()->title) +
      "</font></td></tr></table><br />\n"
      "</dt><dd>";
  }

  ret += low_parse_chapter(n, (int)n->get_attributes()->number);

  if(!noheader)
    ret = ret + "</dd></dl>"; 

  return ret;
}

string parse_appendix(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; Appendix " +
      (string)({ 64+(int)n->get_attributes()->number }) + ". " +
      n->get_attributes()->name + "</font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");
  if(c)
    ret += parse_text(c);
  else
    error( "No doc element in appendix.\n" );

#ifdef DEBUG
  if(sizeof(n->get_elements("doc"))>1)
    error( "More than one doc element in appendix node.\n" );
#endif

  if(!noheader)
    ret = ret + "</dd></dl>"; 

  return ret;
}

string parse_autodoc(Node n)
{
  string ret ="";

  mapping m = n->get_attributes();

  Node c = n->get_first_element("doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  if((sizeof(n->get_elements("doc"))>1) &&
     ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
      Tools.AutoDoc.FLAG_DEBUG)) {
    error( "More than one doc element in autodoc node.\n" );
  }

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "namespace", parse_namespace);

  return ret;
}

string parse_namespace(Node n, void|int noheader)
{
  string ret = "";

  mapping m = n->get_attributes();
  int(0..1) header = !noheader && !(m->hidden) && m->name!=default_ns;
  if(header)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; Namespace <b>" +
      m->name + "::</b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  if((sizeof(n->get_elements("doc"))>1) &&
     ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
      Tools.AutoDoc.FLAG_DEBUG)) {
    error( "More than one doc element in namespace node.\n" );
  }

  ret += parse_children(n, "docgroup", parse_docgroup, noheader);
  ret += parse_children(n, "enum", parse_enum, noheader);
  ret += parse_children(n, "class", parse_class, noheader);
  ret += parse_children(n, "module", parse_module, noheader);

  if(header)
    ret += "</dd></dl>"; 

  return ret;
}

string parse_module(Node n, void|int noheader) {
  string ret ="";

  mapping m = n->get_attributes();
  int(0..1) header = !noheader && !(m->hidden);
  if(header)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; Module <b>" +
      m->class_path + m->name + "</b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");
  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  if((sizeof(n->get_elements("doc"))>1) &&
     ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
      Tools.AutoDoc.FLAG_DEBUG)) {
    error( "More than one doc element in module node.\n" );
  }

  ret += parse_children(n, "docgroup", parse_docgroup, noheader);
  ret += parse_children(n, "enum", parse_enum, noheader);
  ret += parse_children(n, "class", parse_class, noheader);
  ret += parse_children(n, "module", parse_module, noheader);

  if(header)
    ret += "</dd></dl>"; 

  return ret;
}

string parse_class(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; CLASS <b><font color='#005080'>" +
      n->get_attributes()->class_path + n->get_attributes()->name +
      "</font></b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");

  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  if((sizeof(n->get_elements("doc"))>1) &&
     ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
      Tools.AutoDoc.FLAG_DEBUG)) {
    error( "More than one doc element in class node.\n" );
  }

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "enum", parse_enum);
  ret += parse_children(n, "class", parse_class, noheader);

  if(!noheader)
    ret += "</dd></dl>";
  return ret;
}

string parse_enum(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<table width='100%' cellpadding='3' cellspacing='0' border='0'><tr>"
      "<td bgcolor='#EEEEEE'><font size='+3'>&nbsp; ENUM <b><font color='#005080'>" +
      n->get_attributes()->class_path + n->get_attributes()->name +
      "</font></b></font></td></tr></table><br />\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");

  if(c)
    ret += "<dl>" + parse_doc(c) + "</dl>";

  if((sizeof(n->get_elements("doc"))>1) &&
     ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
      Tools.AutoDoc.FLAG_DEBUG)) {
    error( "More than one doc element in enum node.\n" );
  }

  ret += parse_children(n, "docgroup", parse_docgroup);

  if(!noheader)
    ret += "</dd></dl>";
  return ret;
}

string layout_matrix( array(array(string)) rows ) {
  string ret = "<table bgcolor='black' border='0' cellspacing='0' cellpadding='0'><tr><td>\n"
    "<table cellspacing='1' cellpadding='3' border='0' bgcolor='black'>\n";

  int dim;
  foreach(rows, array row)
    dim = max(dim, sizeof(row));

  foreach(rows, array row) {
    ret += "<tr valign='top'>";
    if(sizeof(row)<dim)
      ret += "<td bgcolor='white'>" + row[..sizeof(row)-2]*"</td><td bgcolor='white'>" +
	"</td><td bgcolor='white' colspan='"+ (dim-sizeof(row)) + "'>" + row[-1] + "</td>";
    else
      ret += "<td bgcolor='white'>" + row*"</td><td bgcolor='white'>" + "</td>";
    ret += "</tr>\n";
  }

  return ret + "</table></td></tr></table><br />\n";
}

// ({  ({ array(string)+, void|string  })* })
void nicebox(array rows, String.Buffer ret) {
  ret->add( "<table bgcolor='black' border='0' cellspacing='0' cellpadding='0'><tr><td>\n"
	    "<table cellspacing='1' cellpadding='3' border='0' bgcolor='black'>\n" );

  int dim;
  foreach(rows, array row)
    dim = max(dim, sizeof(row));

  foreach(rows, array row) {
    if(sizeof(row)==1) {
      if(stringp(row[0]))
	ret->add( "<tr valign='top'><td bgcolor='white' colspan='", (string)dim, "'>",
		  row[0], "</td></tr>\n" );
      else
	foreach(row[0], string elem)
	  ret->add( "<tr valign='top'><td bgcolor='white'><tt>", elem, "</tt></td>",
		    (dim==2?"<td bgcolor='white'>&nbsp;</td>":""), "</tr>\n" );
    }
    else if(sizeof(row[0])==1)
      ret->add( "<tr valign='top'><td bgcolor='white'><tt>", row[0][0],
		"</tt></td><td bgcolor='white'>", row[1], "</td></tr>\n" );
    else {
      ret->add( "<tr valign='top'><td bgcolor='white'><tt>", row[0][0],
		"</tt></td><td bgcolor='white' rowspan='", (string)sizeof(row[0]), "'>",
		row[1], "</td></tr>\n" );
      foreach(row[0][1..], string elem)
	ret->add( "<tr valign='top'><td bgcolor='white'><tt>", elem, "</tt></td></tr>\n" );
    }
  }

  ret->add( "</table></td></tr></table><br />\n" );
}

void build_box(Node n, String.Buffer ret, string first, string second, function layout, void|string header) {
  array rows = ({});
  if(header) rows += ({ ({ header }) });
  foreach(n->get_elements(first), Node d) {
    array elems = ({});
    foreach(d->get_elements(second), Node e)
      elems += ({ layout(e) });
    if(d->get_first_element("text"))
      rows += ({ ({ elems, parse_text(d->get_first_element("text")) }) });
    else
      rows += ({ ({ elems }) });
  }
  nicebox(rows, ret);
}

// type(min..max)
string range_type( string type, Node min, Node max )
{
    if( !min && !max )
        return type;
    if( !min )
        return type+"(.."+parse_text(max)+")";
    if( !max )
        return type+"("+parse_text(min)+"..)";

    int low = (int)parse_text(min);
    int high = (int)parse_text(max);

    if( low == 0 && (high+1)->popcount() == 1 )
    {
        return type+"("+strlen((high)->digits(2))+"bit)";
    }
    return type+"("+low+".."+high+")";
}

//! Typically called with a <group/> node or a sub-node that is a container.
string parse_text(Node n, void|String.Buffer ret) {
  if(n->get_node_type()==XML_TEXT && n->get_text()) {
    if(ret)
      ret->add(quote(n->get_text()));
    else
      return quote(n->get_text());
  }

  int cast;
  if(!ret) {
    ret = String.Buffer();
    cast = 1;
  }

  foreach(n->get_children(), Node c) {
    int node_type = c->get_node_type();
    if(c->get_node_type()==XML_TEXT) {
      // Don't use quote() here since we don't want to strip whitespace.
      ret->add(Parser.XML.Tree.text_quote (c->get_text()));
      continue;
    }

    if(c->get_node_type()==XML_COMMENT)
      continue;

    if((c->get_node_type()!=XML_ELEMENT) &&
       ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
	Tools.AutoDoc.FLAG_DEBUG)) {
      error( "Forbidden node type " + c->get_node_type() + " in doc node.\n" );
    }

    array rows;
    string name = c->get_any_name();
    switch(name) {
    case "text":
      ret->add("<dd>");
      parse_text(c, ret);
      ret->add("</dd>\n");
      break;

    case "p":
    case "b":
    case "i":
    case "tt":
    case "sub":
    case "sup":
      ret->add("<", name, ">");
      parse_text(c, ret);
      ret->add("</", name, ">");
      break;

    case "pre":
      ret->add(lay->pre);
      parse_text(c, ret);
      ret->add(lay->_pre);
      break;

    case "code":
      ret->add(lay->code);
      parse_text(c, ret);
      ret->add(lay->_code);
      break;

    case "expr":
      ret->add(lay->expr, replace(parse_text(c), " ", "&nbsp;"), lay->_expr);
      break;

    case "ref":
      if(resolve_reference) {
	ret->add(resolve_reference(parse_text(c), c->get_attributes()));
	break;
      }
      string ref;
      //ref = c->get_attributes()->resolved;
      if(!ref) ref = parse_text(c);
      ret->add("<font face='courier'>", ref, "</font>");
      break;

    case "dl":
      ret->add("<dl>", map(c->get_elements("group"), parse_text)*"", "</dl>");
      break;

    case "item":
      if(c->get_attributes()->name)
	ret->add("<dt>", c->get_attributes()->name, "</dt>\n");
      if(c->count_children() &&
	 ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
	  Tools.AutoDoc.FLAG_DEBUG)) {
	error( "dl item has a child.\n" );
      }
      break;

    case "mapping":
      build_box(c, ret, "group", "member",
		lambda(Node n) {
		  string res = "";
		  Node nn = n->get_first_element("index");
		  if (nn) {
		    res +=
		      "<font color='green'>" + parse_text(nn) + "</font> : ";
		  }
		  nn = n->get_first_element("type");
		  if (nn) {
		    res += parse_type(get_first_element(nn));
		  }
		  return res;
		});
      break;

    case "array":
      build_box(c, ret, "group", "elem",
		lambda(Node n) {
		  string index ="";
		  if(n->get_first_element("index"))
		    index = parse_text(n->get_first_element("index"));
		  else {
		    if(n->get_first_element("minindex"))
		      index = parse_text(n->get_first_element("minindex"));
		    index += "..";
		    if(n->get_first_element("maxindex"))
		      index += parse_text(n->get_first_element("maxindex"));
		  }
		  return parse_type(get_first_element(n->get_first_element("type"))) +
		    " <font color='green'>" + index + "</font>"; }, "Array" );
      break;

    case "int":
      build_box(c, ret, "group", "value",
		lambda(Node n) {
           return "<font color='green'>" +
               range_type( parse_text(n),
                           n->get_first_element("minvalue"),
                           n->get_first_element("maxvalue"))+
               "</font>";
		} );
      break;

    case "mixed":
      if(c->get_attributes()->name)
	ret->add("<tt>", c->get_attributes()->name, "</tt> can have any of the following types:<br />");
      rows = ({});
      foreach(c->get_elements("group"), Node d)
	rows += ({ ({
	  ({ parse_type(get_first_element(d->get_first_element("type"))) }),
	  parse_text(d->get_first_element("text"))
	}) });
      nicebox(rows, ret);
      break;

    case "string": // Not in XSLT
      build_box(c, ret, "group", "value",
		lambda(Node n) {
            return "<font color='green'>" +
                range_type( parse_text(n),
                            n->get_first_element("min"),
                            n->get_first_element("max")) +
                "</font>";
		} );
      break;

    case "multiset": // Not in XSLT
      build_box(c, ret, "group", "index",
		lambda(Node n) {
		  return "<font color='green'>" +
		    parse_text(n->get_first_element("value")) + "</font>";
		} );
      break;

    case "image": // Not in XSLT
      mapping m = c->get_attributes();
      m->src = image_prefix() + m_delete(m, "file");
      ret->add( render_tag("img", m, 1) );
      break;

    case "url": // Not in XSLT
      if((c->count_children()!=1 && c->get_node_type()!=XML_ELEMENT) &&
	 ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
	  Tools.AutoDoc.FLAG_DEBUG)) {
	error( "url node has not one child. %O\n", c->html_of_node() );
      }
      m = c->get_attributes();
      if(!m->href)
	m->href=c->value_of_node();
      ret->add( sprintf("%s%s</a>", render_tag("a", m), c->value_of_node()) );
      break;

    case "section":
      if(section && !(flags & Tools.AutoDoc.FLAG_KEEP_GOING))
	error("Section inside section.\n");
      ret->add ("<h2>", quote (c->get_attributes()->title ||
			       // The following for bug compat.
			       c->get_attributes()->name),
		"</h2>\n");
      if (!equal (c->get_children()->get_any_name(), ({"text"})) &&
	  !(flags & Tools.AutoDoc.FLAG_KEEP_GOING))
	error ("Expected a single <text> element inside <section>.\n");
      section = -1;
      parse_text (c->get_children()[0], ret);
      section = 0;
      break;

    case "ul":
      ret->add( "<ul>\n" );
      foreach(c->get_elements("group"), Node c) {
	ret->add("<li>");
	array(Node) d = c->get_elements("item");
	if(sizeof(d->get_attributes()->name-({0}))) {
	  ret->add("<b>");
	  ret->add( String.implode_nicely(d->get_attributes()->name-({0})) );
	  ret->add("</b>");
	}
	Node e = c->get_first_element("text");
	if(e)
	  parse_text(e, ret);
	ret->add("</li>");
      }
      ret->add("</ul>");
      break;

    case "ol":
      ret->add("<ol>\n");
      foreach(c->get_elements("group"), Node c) {
	ret->add("<li>");
	parse_text(c->get_first_element("text"), ret);
	ret->add("</li>");
      }
      ret->add("</ol>");
      break;

    case "source-position":
      position->update(c);
      break;

    case "matrix":
      ret->add( layout_matrix( map(c->get_elements("r")->get_elements("c"),
				   map, parse_text) ) );
      break;

    case "fixme":
      ret->add("<font color='red'>FIXME: ");
      parse_text(c, ret);
      ret->add("</font>");
      break;

    // Not really allowed
    case "br":
      ret->add( render_tag(c->get_any_name(), c->get_attributes(), 1) );
      break;
    case "table":
    case "td":
    case "tr":
    case "th":
      ret->add( sprintf("%s%s</%s>",
			render_tag(c->get_any_name(), c->get_attributes()),
			parse_text(c), c->get_any_name()) );
      break;

    default:
      if (verbosity >= Tools.AutoDoc.FLAG_DEBUG) {
	werror("\n%s\n", (string)c);
	error( "Illegal element \"" + name + "\" in mode text.\n" );
      }
      break;
    }
  }

  if(cast)
    return ret->get();
}

string parse_doc(Node n, void|int no_text) {
  string ret ="";

  Node c = n->get_first_element("text");
  if(c)
    ret += lay->dochead + "Description" + lay->_dochead +
      lay->docbody + parse_text(c) + lay->_docbody;

  foreach(n->get_elements("group"), Node c) {
    Node header = c->get_first_element();
    string name = header->get_any_name();
    switch(name) {
    case "param":
      foreach(c->get_elements("param"), Node d)
	ret += lay->dochead + "Parameter " + lay->parameter +
	  quote(d->get_attributes()->name || "") +
	  lay->_parameter + lay->_dochead +
	  "<dd></dd>";
      if (c = c->get_first_element("text")) {
	ret += lay->docbody + parse_text(c) + lay->_docbody;
      }
      break;

    case "seealso":
      ret += lay->dochead + "See also" + lay->_dochead;
      if (c = c->get_first_element("text")) {
	ret += lay->docbody + parse_text(c) + lay->_docbody;
      }
      break;

    case "fixme":
      ret += lay->fixmehead + "FIXME" + lay->_fixmehead;
      if (c = c->get_first_element("text")) {
	ret += 	lay->fixmebody + parse_text(c) + lay->_fixmebody;
      }
      break;

    case "deprecated":
      ret += lay->dochead + String.capitalize(name) + lay->_dochead;
      array(Node) replacements = header->get_elements("name");
      if (sizeof(replacements)) {
	ret += lay->docbody +
	  "<p>Replaced by " +
	  String.implode_nicely(map(replacements, parse_text)) + ".</p>\n" +
	  lay->_docbody;
      }
      if (c = c->get_first_element("text")) {
	ret += lay->docbody + parse_text(c) + lay->_docbody;
      }
      break;

    case "bugs":
    case "copyright":
    case "note":
    case "returns":
    case "thanks":
    case "throws":
      ret += lay->dochead + String.capitalize(name) + lay->_dochead;
      // FALL_THROUGH
    case "text":
      if (c = c->get_first_element("text")) {
	ret += lay->docbody + parse_text(c) + lay->_docbody;
      }
      break;

    case "example":
      ret += lay->dochead + "Example" + lay->_dochead;
      if (c = c->get_first_element("text")) {
	ret += lay->example + parse_text(c) + lay->_example;
      }
      break;

    default:
      error( "Unknown group type \"" + name + "\".\n" );
    }
  }

  return ret;
}

string parse_type(Node n, void|string debug) {
  if(!n || (n->get_node_type()!=XML_ELEMENT))
    return "";

  string ret = "";
  Node c, d;
  switch(n->get_any_name()) {

  case "object":
    if(n->count_children()) {
      if (resolve_reference) {
	ret += "<font color='#005080'>" +
	  resolve_reference(n->value_of_node(), n->get_attributes()) +
	  "</font>";
      } else {
	ret += "<font color='#005080'>" + n->value_of_node() + "</font>";
      }
    } else
      ret += "<font color='#202020'>object</font>";
    break;

  case "type":
    ret += "<font color='#202020'>type</font>";
    if (n->count_children() && (c = get_first_element(n)) &&
	(c->get_any_name() != "mixed")) {
      ret += "(" + parse_type(c) + ")";
    }
    break;

  case "multiset":
    ret += "<font color='#202020'>multiset</font>";
    c = n->get_first_element("indextype");
    if(c) ret += "(" + parse_type( get_first_element(c) ) + ")";
    break;

  case "array":
    ret += "<font color='#202020'>array</font>";
    c = n->get_first_element("valuetype");
    if(c) ret += "(" + parse_type( get_first_element(c) ) + ")";
    break;

  case "mapping":
    ret += "<font color='#202020'>mapping</font>";
    c = n->get_first_element("indextype");
    d = n->get_first_element("valuetype");
    if(c && d)
      ret += "(" + parse_type( get_first_element(c) ) + ":" +
	parse_type( get_first_element(d) ) + ")";
#ifdef DEBUG
    if( !c != !d )
      error( "Indextype/valuetype defined while the other is not in mapping.\n" );
#endif
    break;

  case "function":
    ret += "<font color='#202020'>function</font>";
    array(Node) args = n->get_elements("argtype");
    d = n->get_first_element("returntype");
    // Doing different than the XSL here. Must both
    // argtype and returntype be defined?
    if(args || d) {
      ret += "(";
      if(args) ret += map(args->get_children() * ({}), parse_type)*", ";
      ret += ":";
      if(d) ret += parse_type( get_first_element(d) );
      else ret += "<font color='#202020'>void</font>";
      ret += ")";
    }
    break;

  case "varargs":
#ifdef DEBUG
    if(!n->count_children())
      error( "varargs node must have child node.\n" );
#endif
    ret += parse_type(get_first_element(n)) + " ... ";
    break;

  case "or":
    ret += map(filter(n->get_children(),
		      lambda(Node n){
			return n->get_node_type()==XML_ELEMENT;
		      }), parse_type)*"|";
    break;

  case "void": case "program": case "mixed": case "float":
  case "zero":
    ret += "<font color='#202020'>" + n->get_any_name() + "</font>";
    break;

  case "string":
  case "int":
      ret += ("<font color='#202020'>" +
              range_type( n->get_any_name(),
                          n->get_first_element("min"),
                          n->get_first_element("max")) +
              "</font>");
    break;

  case "attribute":
    string attr = n->get_first_element("attribute")->value_of_node();
    string subtype =
      parse_type(n->get_first_element("subtype")->get_first_element());
    if (n->get_first_element("prefix")) {
      if (attr == "\"deprecated\"") {
	ret += "<font color='#600000'>__deprecated__</font> " + subtype;
      } else {
	ret += sprintf("__attribute__(%s) %s", attr, subtype);
      }
    } else if (attr == "\"deprecated\"") {
      ret += "<font color='#600000'>__deprecated__</font>(" + subtype + ")";
    } else {
      ret += sprintf("__attribute__(%s, %s)", attr, subtype);
    }
    break;

    // Modifiers:
  case "extern": // Not in XSLT
    ret += "extern ";
    break;
  case "final": // Not in XSLT
  case "nomask": // Not in XSLT
    ret += "final ";
    break;
  case "inline": // Not in XSLT
  case "local": // Not in XSLT
    ret += "local ";
    break;
  case "optional": // Not in XSLT
    ret += "optional ";
    break;
  case "private": // Not in XSLT
    ret += "private ";
    break;
  case "protected": // Not in XSLT
  case "static": // Not in XSLT
    ret += "protected ";
    break;
  case "public": // Not in XSLT
    // Ignored.
    break;
  case "variant": // Not in XSLT
    ret += "variant ";
    break;

  default:
    error( "Illegal element " + n->get_any_name() + " in mode type.\n" );
    break;
  }
  return ret;
}

void resolve_class_paths(Node n, string|void path, Node|void parent)
{
  if (!path) path = "";
  mapping attrs = n->get_attributes() || ([]);
  string name = attrs->name;
  switch(n->get_any_name()) {
  case "arguments":
  case "returntype":
  case "type":
  case "doc":
  case "source-position":
  case "modifiers":
  case "classname":
    // We're not interrested in the stuff under the above nodes.
    return;
  default:
    if (verbosity >= Tools.AutoDoc.FLAG_NORMAL) {
      werror("Unhandled node: %O path:%O name: %O, parent: %O\n",
	     n->get_any_name(), path, name, parent&&parent->get_any_name());
    }
    // FALL_THROUGH
  case "":
  case "docgroup":
    break;
    
  case "manual":
  case "dir":
  case "file":
  case "chapter":
  case "section":
  case "subsection":
    foreach(filter(n->get_children(),
		   lambda(Node n) {
		     return (<"manual", "dir", "file", "chapter",
			      "section", "subsection", "autodoc">)
		       [n->get_any_name()];
		   }), Node child) {
      resolve_class_paths(child, "", n);
    }
    return;
  case "autodoc":
    path = "";
    break;
  case "namespace":
    if ((<"", "lfun">)[name]) {
      // Censor namespaces other than :: and lfun::
      path = name+"::";
    } else {
      path = "";
    }
    break;
  case "class":
  case "module":
    attrs->class_path = path;
    path += name + ".";
    break;
  case "enum":
    // Enum names don't show up in the path.
    attrs->class_path = path;
    break;
  case "method":
    // Special case for create().
    if (name == "create") {
      // Get rid of the extra dot.
      attrs->class_path = path[..sizeof(path)-2];
    }
    else
    {
      // Hide the class path for methods.
      attrs->class_path = "";
    }
    return;
  case "constant":
  case "inherit":
  case "import":
  case "typedef":
  case "variable":
  case "directive":
    // These don't have children.
    attrs->class_path = path;
    return;
  }
  foreach(n->get_children(), Node child)
  {
    resolve_class_paths(child, path, n);
  }
}

#if 0
// DEAD code (for reference only).
string render_class_path(Node n,int|void class_only)
{
  array a = reverse(n->get_ancestors(0));
  array b = a->get_any_name();
  int root;
  foreach( ({ "manual", "dir", "file", "chapter",
	      "section", "subsection" }), string node)
    root = max(root, search(b, node));
  a = a[root+1..];
  if(sizeof(a) && a[0]->get_any_name() == "autodoc")
    a = a[1..];
  if (!sizeof(a)) return "";
  string ret = "";
  if ((sizeof(a) > 1) && a[-2]->get_any_name() == "enum") {
    // Enum names don't show up in the path.
    a = a[..sizeof(a)-3] + a[sizeof(a)-1..];
  }
  if (a[0]->get_any_name() == "namespace") {
    a = a->get_attributes()->name;
    a[0] += "::";
    if ((<"::","lfun::">)[a[0]]) {
      // Censor namespaces other than :: and lfun::
      ret = a[0];
    }
    a = a[1..];
  } else {
    a = a->get_attributes()->name;
  }
  ret += a * ".";
  if (!sizeof(ret) || ret[-1] == ':') return ret;
  if((<"class", "module", "enum">)[n->get_any_name()])
    return ret + ".";

  // Note we need two calls to get_parent() to skip over the docgroup.
  Node parent = n->get_parent()->get_parent();

  // Hide the enum part.
  if (parent->get_any_name()=="enum") parent = parent->get_parent();

  if(parent->get_any_name()=="class")
    if( !class_only )
      return ""; //ret + "()->";
    else
      return ret;
  if(parent->get_any_name()=="module")
    return ret + ".";
  if (verbosity >= Tools.AutoDoc.FLAG_NORMAL) {
    werror("Failure, raw path: %{%O, %}\n",
	   reverse(n->get_ancestors(0))->get_any_name());
  }
  return " (could not resolve) ";
#ifdef DEBUG
  error( "Parent module is " + n->get_parent()->get_any_name() + ".\n" );
#else
    return "";
#endif
}
#endif /* 0 */

string parse_not_doc(Node n) {
  string ret = "";
  int method, argument, variable, const, typedf, cppdir;

  if (!n) return "";

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
#if 0
      if(!c->get_first_element("returntype"))
	error( "No returntype element in method element.\n" );
#endif
      switch( c->get_attributes()->name )
      {
	case "create":
	  ret += "<tt>" + parse_type(get_first_element(c->get_first_element("returntype"))); // Check for more children
	  ret += " ";
	  ret += c->get_attributes()->class_path+"<b>(</b>";
	  ret += parse_not_doc( c->get_first_element("arguments") );
	  ret += "<b>)</b></tt>";
	  break;
	default:
	  ret += "<tt>";
	  cc = c->get_first_element("modifiers");
	  if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
	  ret += parse_type(get_first_element(c->get_first_element("returntype"))); // Check for more children
	  ret += " ";
	  ret += c->get_attributes()->class_path;
	  ret += "<b><font color='#000066'>" + c->get_attributes()->name + "</font>(</b>";
	  ret += parse_not_doc( c->get_first_element("arguments") );
	  ret += "<b>)</b></tt>";
	  break;
      }
      break;

    case "argument":
      if(argument++) ret += ", ";
      cc = c->get_first_element("value");
      if(cc) ret += "<font color='green'>" + cc->value_of_node() + "</font>";
      else if( !c->count_children() );
      else if( get_first_element(c)->get_any_name()=="type") {
	ret += parse_type(get_first_element(get_first_element(c)));
	if(c->get_attributes()->name)
	  ret += " <font color='#005080'>" +
	    c->get_attributes()->name + "</font>";
      }
      else
	error( "Malformed argument element.\n" + c->html_of_node() + "\n" );
      break;

    case "variable":
      if(variable++) ret += "<br />\n";
      ret += "<tt>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      if (c->get_first_element("type")) {
	ret += parse_type(get_first_element(c->get_first_element("type")),
			  "variable") + " " +
	  c->get_attributes()->class_path + "<b><font color='#F000F0'>" +
	  c->get_attributes()->name + "</font></b></tt>";
      } else
	error("Malformed variable element.\n" + c->html_of_node() + "\n");
      break;

    case "constant":
      if(const++) ret += "<br />\n";
      ret += "<tt>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += "constant ";
      if (Node type = c->get_first_element ("type"))
	ret += parse_type (get_first_element (type), "constant") + " ";
      ret += c->get_attributes()->class_path;
      ret += "<font color='#F000F0'>" + c->get_attributes()->name + "</font>";
      cc = c->get_first_element("typevalue");
      if(cc) ret += " = " + parse_type(get_first_element(cc));
      ret += "</tt>";
      break;

    case "typedef":
      if(typedf++) ret += "<br />\n";
      ret += "<tt>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += "typedef ";
      ret += parse_type(get_first_element(c->get_first_element("type")), "typedef") + " " +
	c->get_attributes()->class_path + "<font color='#F000F0'>" + c->get_attributes()->name +
	"</font></tt>";
      break;

    case "inherit":
      ret += "<font face='courier'>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += "inherit ";
      Node n = c->get_first_element("classname");
      if (resolve_reference) {
	ret += "</font>" +
	  resolve_reference(n->value_of_node(), n->get_attributes());
      } else {
	ret += Parser.encode_html_entities(n->value_of_node()) + "</font>";
      }
      if (c->get_attributes()->name) {
	ret += "<font face='courier'> : " + "<font color='#F000F0'>" +
	  Parser.encode_html_entities(c->get_attributes()->name) +
	  "</font></font>";
      }
      break;

    // We don't need import information.
    case "import":
      break;

    case "directive":
      if(cppdir++) ret += "<br />\n";
      ret += "<tt><font color='#006666'>" + quote(c->get_attributes()->name) +
	"</font></tt>";
      break;

    default:
      error( "Illegal element " + c->get_any_name() + " in !doc.\n" );
      break;
    }
  }

  return ret;
}

int foo;

string parse_docgroup(Node n) {
  mapping m = n->get_attributes();
  string ret = lay->docgroup;

  if (m["homogen-type"] == "import") {
    // We don't need import information.
    return "";
  }

  if(lay->typehead) {
    ret += lay->typehead;
    if(m["homogen-type"]) {
      string type = "<font face='Helvetica'>" +
	quote(String.capitalize(m["homogen-type"])) +
	"</font>\n";
      if(m["homogen-name"]) {
	ret += type + "<font size='+1'><b>" +
	  quote((m->belongs?m->belongs+" ":"") + m["homogen-name"]) +
	  "</b></font>\n";
      } else {
	array(string) names =
	  Array.uniq(map(n->get_elements(m["homogen-type"]),
			 lambda(Node child) {
			   return child->get_attributes()->name ||
			     child->value_of_node();
			 }));
	foreach(names, string name)
	  ret += type +
	    "<font size='+1'><b>" + quote(name) + "</b></font><br />\n";
      }
    }
    else
      ret += "Syntax";
    ret += lay->_typehead;
  }

  ret += lay->ndochead;

  ret += parse_not_doc(n);

  ret += lay->_ndochead;

  foreach(n->get_elements("doc"), Node c)
    ret += parse_doc(c);

  return ret + lay->_docgroup;
}

string parse_children(Node n, string tag, function cb, mixed ... args) {
  string ret = "";
  foreach(n->get_elements(tag), Node c)
    ret += cb(c, @args);

  return ret;
}

string manual_title = "Pike Reference Manual";
string frame_html(string res, void|string title) {
  title = title || manual_title;
  return "<html><head><title>" + quote(title) + "</title></head>\n"
    "<body bgcolor='white' text='black'>\n" + res +
    "</body></html>";
}

string layout_toploop(Node n, Git.Export|void exporter) {
  string res = "";
  foreach(n->get_elements(), Node c)
    switch(c->get_any_name()) {

    case "dir":
      if (exporter) {
	layout_toploop(c, exporter);
	break;
      }
      string cwd;
      if(dest_path)
      {
        cwd=getcwd();
        cd(dest_path);
      }  
      Stdio.mkdirhier(c->get_attributes()->name);
      layout_toploop(c);
      if(cwd)
        cd(cwd);
      break;

    case "file":
      if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE) {
	werror("\t%s\n", c->get_attributes()->name);
      }
      if (exporter) {
	string html = frame_html(layout_toploop(c));
	exporter->filemodify(Git.MODE_FILE, c->get_attributes()->name);
	exporter->data(html);
	break;
      }
      if(dest_path)
      {
        cwd=getcwd();
        cd(dest_path);
      }  
      Stdio.write_file( c->get_attributes()->name,
			frame_html(layout_toploop(c)) );
      if(cwd)
        cd(cwd);
      break;

    case "appendix":
      if (!(flags & Tools.AutoDoc.FLAG_COMPAT)) {
	error("Appendices are only supported in compat mode.\n");
      }
      res += parse_appendix(c);
      break;

    case "chapter":
      res += parse_chapter(c);
      break;

    default:
      error("Unknown element %O.\n", c->get_any_name());
    }
  return res;
}

int low_main(string title, string input_file, string|void output_file,
	     Git.Export|void exporter)
{
  int t = time();
  string file = Stdio.read_file(input_file);
  if(!file)
    error( "Could not read %s.\n", input_file );
  if(!sizeof(file))
    error( "%s is empty.\n", input_file );

  // We are only interested in what's in the
  // module container.
  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE) {
    werror("Parsing %O...\n", input_file);
  }
  Node n = Parser.XML.Tree.simple_parse_input(file);

  Node n2 = n->get_first_element("manual");
  if(!n2) {
    n2 = n->get_first_element("autodoc");
    if(!n2) {
      if (verbosity >= Tools.AutoDoc.FLAG_NORMAL) {
	werror("Not an autodoc XML file.\n");
      }
      return 1;
    }

    Node chap = Node(XML_ELEMENT, "chapter",
		     (["title":title||"Documentation"]), 0);
    chap->add_child( n2 );

    string fn = output_file || "index.html";
    Node file = Node(XML_ELEMENT, "file", (["name":fn]), 0);
    file->add_child( chap );

    Node top = Node(XML_ELEMENT, "manual", ([]), 0);
    top->add_child( file );
    n = top;
  }
  else
    n = n2;

  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE) {
    werror("Resolving class paths...\n");
  }
  resolve_class_paths(n);

  mapping m = n->get_attributes();

  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE) {
    werror("Layouting...\n");
  }
  if (flags & Tools.AutoDoc.FLAG_NO_DYNAMIC) {
    manual_title = m->title || "Pike Reference Manual";
  } else {
    manual_title = m->title ||
      (m->version?"Reference Manual for "+m->version:"Pike Reference Manual");
  }
  layout_toploop(n, exporter);

  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE) {
    werror("Took %d seconds.\n\n", time()-t);
  }

  return 0;
}

int main(int num, array args) {

  string title;

  foreach(Getopt.find_all_options(args, ({
    ({ "img",     Getopt.HAS_ARG, "--img"   }),
    ({ "dest",    Getopt.HAS_ARG, "--dest"  }),
    ({ "title",   Getopt.HAS_ARG, "--title" }),
    ({ "defns",   Getopt.HAS_ARG, "--default-ns" }),
    ({ "verbose", Getopt.NO_ARG,  "-v,--verbose"/"," }),
    ({ "quiet",   Getopt.NO_ARG,  "-q,--quiet"/"," }),
    ({ "help",    Getopt.NO_ARG,  "--help"  }),
  })), array opt)
    switch(opt[0]) {
    case "img":
      image_path = opt[1];
      break;
    case "dest":
      dest_path = opt[1];
      break;
    case "title":
      title = opt[1];
      break;
    case "defns":
      default_ns = opt[1];
      break;
    case "verbose":
      if (verbosity < Tools.AutoDoc.FLAG_DEBUG) {
	verbosity += 1;
	flags = (flags & ~Tools.AutoDoc.FLAG_VERB_MASK) | verbosity;
      }
      break;
    case "quiet":
      flags &= ~Tools.AutoDoc.FLAG_VERB_MASK;
      verbosity = Tools.AutoDoc.FLAG_QUIET;
      break;
    case "help":
      write(#"pike -x autodoc_to_html [args] <input file> [<output file>]
--img=<image path>
--dest=<destination path>

--title=<document title>
");
      break;
    }
  args = Getopt.get_args(args)[1..];

  if(!sizeof(args))
    error( "No input file given.\n" );

  return low_main(title, @args);
}
