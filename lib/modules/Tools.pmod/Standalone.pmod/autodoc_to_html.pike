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

 "docgroup" : "\n\n<hr />\n<dl class='group--doc'>",
 "_docgroup" : "</dl>\n",
 "ndochead" : "<dd><p>",
 "_ndochead" : "</p></dd>\n",

 "dochead" : "\n<dt class='head--doc'>",
 "_dochead" : "</dt>\n",
 "typehead" : "\n<dt class='head--type'>",
 "_typehead" : "</dt>\n",
 "docbody" : "<dd class='body--doc'>",
 "_docbody" : "</dd>",
 "fixmehead" : "<dt class='head--fixme'>",
 "_fixmehead" : "</dt>\n",
 "fixmebody" : "<dd class='body--fixme'>",
 "_fixmebody" : "</dd>",

 "parameter" : "<code class='parameter'>",
 "_parameter" : "</code>",
 "example" : "<dd class='example'><pre>",
 "_example" : "</pre></dd>",

 "pre" : "<pre>",
 "_pre" : "</pre>",
 "code" : "<pre><code>",
 "_code" : "</code></pre>",
 "expr" : "<code class='expr'>",
 "_expr" : "</code>",

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
  if(String.trim(in)=="") return "\n";
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
      ret += "<p></p><pre>" + quote(c->value_of_node()) +
             "</pre><p></p>\n";
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
	"<h2 class='header'>" + chapter + "." + section +
	". " + quote(c->get_attributes()->title ||
		     // The following for bug compat.
		     c->get_attributes()->name) +
	"</h2></dt>\n<dd>";
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
	"<h3 class='header'>" + chapter + "." + section + "." + subsection +
	". " + quote(c->get_attributes()->title) +
	"</h3></dt><dd>";
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
      "<h1 class='header'>";
    if(n->get_attributes()->number)
      ret += n->get_attributes()->number + ". ";
    ret += quote(n->get_attributes()->title) +
      "</h1>"
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
      "<h2 class='header'>Appendix " +
      (string)({ 64+(int)n->get_attributes()->number }) + ". " +
      n->get_attributes()->name + "</h2>\n"
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
      "<h2 class='header'>Namespace <b class='ms datatype'>" +
      m->name + "::</b></h2>\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");
  if(c)
    ret += "<dl class='group--doc'>" + parse_doc(c) + "</dl>";

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
      "<h2 class='header'>Module <b class='ms datatype'>" +
      m->class_path + m->name + "</b></h2>\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");
  if(c)
    ret += "<dl class='group--doc'>" + parse_doc(c) + "</dl>";

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

ADT.Stack old_class_name = ADT.Stack();
string parse_class(Node n, void|int noheader) {
  string ret ="";
  if(!noheader)
    ret += "<dl><dt>"
      "<h2 class='header'>Class <b class='ms datatype'>" +
      n->get_attributes()->class_path + n->get_attributes()->name +
      "</b></h2>\n"
      "</dt><dd>";

  Node c = n->get_first_element("doc");
  old_class_name->push(class_name);
  class_name = n->get_attributes()->class_path+n->get_attributes()->name;
  if(c)
    ret += "<dl class='group--doc'>" + parse_doc(c) + "</dl>";

  if((sizeof(n->get_elements("doc"))>1) &&
     ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
      Tools.AutoDoc.FLAG_DEBUG)) {
    error( "More than one doc element in class node.\n" );
  }

  ret += parse_children(n, "docgroup", parse_docgroup);
  ret += parse_children(n, "enum", parse_enum, noheader);
  ret += parse_children(n, "class", parse_class, noheader);
  ret += parse_children(n, "module", parse_module, noheader);

  if(!noheader)
    ret += "</dd></dl>";
  class_name = old_class_name->pop();
  return ret;
}

string parse_enum(Node n, void|int noheader) {
  string ret ="";
  if(!noheader) {
    ret += "<dl><dt>"
      "<h2 class='header'>Enum <b class='ms datatype'>" +
      n->get_attributes()->class_path + n->get_attributes()->name +
      "</b></h2>\n"
      "</dt><dd>";
  }

  Node c = n->get_first_element("doc");

  if(c)
    ret += "<dl class='group--doc'>" + parse_doc(c) + "</dl>";

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

  string ret =
    "<table class='box'>\n";


  int dim;
  foreach(rows, array row)
    dim = max(dim, sizeof(row));

  foreach(rows, array row) {
    ret += "<tr>";
    if(sizeof(row)<dim)
      ret += "<td>" + row[..sizeof(row)-2]*"</td><td>" +
	"</td><td colspan='"+ (dim-sizeof(row)) + "'>" + row[-1] + "</td>";
    else
      ret += "<td>" + row*"</td><td>" + "</td>";
    ret += "</tr>\n";
  }

  return ret + "</table>\n";
}

// ({  ({ array(string)+, void|string  })* })
void nicebox(array rows, String.Buffer ret) {
  ret->add("<table class='box'>");

  int dim;
  foreach (rows, array row) {
    dim = max(dim, sizeof(row));
  }

  foreach (rows, array row) {
    if (sizeof(row) == 1) {
      if (stringp(row[0])) {
        ret->add("<tr><td colspan='", (string)dim, "'>", row[0], "</td></tr>\n");
      }
      else {
        foreach (row[0], string elem) {
          ret->add("<tr><td><code>", elem, "</code></td>",
                   (dim == 2 ? "<td>&nbsp;</td>" : ""), "</tr>\n");
        }
      }
    }
    else if (sizeof(row[0]) == 1) {
      ret->add("<tr><td><code>", row[0][0], "</code></td>",
               "<td>", row[1], "</td></tr>\n");
    }
    else {
      ret->add("<tr><td><code>", row[0][0], "</code></td>",
               "<td rowspan='", (string)sizeof(row[0]), "'>",
               row[1], "</td></tr>\n");
      foreach (row[0][1..], string elem) {
        ret->add("<tr><td><code>", elem, "</code></td></tr>\n");
      }
    }
  }

  ret->add("</table>");
}

void build_box(Node n, String.Buffer ret, string first, string second, function layout, void|string header) {

        //werror("build_box: %O\n", n);

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
    // Work with plain text; if there's no node, that's the same as an empty node.
    string min_text = min ? parse_text(min) : "";
    string max_text = max ? parse_text(max) : "";
    if( min_text == "" && max_text == "" )
        return type;
    if( min_text == "" )
        return type+"(.."+max_text+")";
    if( max_text == "" )
        return type+"("+min_text+"..)";

    int low = (int)min_text;
    int high = (int)max_text;

    if( low == 0 && high && (high+1)->popcount() == 1 )
    {
      if( high == 1 && type == "int" )
        return "bool";
      return type+"("+strlen((high)->digits(2))+"bit)";
    }
    return type+"("+low+".."+high+")";
}

Tools.Standalone.pike_to_html code_highlighter;

// Normalizes indentation of @code ... @endcode blocks.
// Also tries to syntax highlight code that looks like Pike.
string parse_code(Node n, void|String.Buffer ret)
{
  string text;
  if (n->get_tag_name() != "code") {
    return parse_text(n, ret);
  }

  array(string) lines = n->get_children()[0]->get_children()->value_of_node();

  if (!sizeof(lines)) {
    return parse_text(n, ret);
  }

  if (sizeof(lines) && lines[-1] == "\n") {
    lines[0..<1];
  }

  if (sizeof(lines) > 1) {
    for (int i = 0; i < sizeof(lines); i++) {
      string ln = map(lines[i]/"\n", lambda (string s) {
        if (has_prefix(s, " "))
          s = s[1..];
        return s;
      }) * "\n";

      if (!has_suffix(ln, "\n"))
        ln += "\n";

      lines[i] = ln;
    }

    text = lines*"";
  }
  else {
    text = lines*"";
  }

  while (text[-1] == '\n')
    text = text[..<1];

  bool is_quoted = false;

  if (has_value(text, "->") ||
      (has_value(text, "{") && has_value(text, "}")) ||
      (has_value(text, "(") && has_value(text, "\"")) ||
      (has_value(text, ".") && has_value(text, "=")))
  {
    if (!code_highlighter) {
      code_highlighter = Tools.Standalone.pike_to_html();
    }

    is_quoted = true;

    if (catch(text = code_highlighter->convert(text))) {
      text = text;
      is_quoted = false;
    }
  }

  if (ret) {
    if (!is_quoted) {
      text = quote(text);
    }

    ret->add(text);
  }
  else return quote(text);
}

//! Typically called with a <group/> node or a sub-node that is a container.
string parse_text(Node n, void|String.Buffer ret) {
  if(n->get_node_type()==XML_TEXT && n->get_text()) {
    if(ret)
      ret->add("parse_text:#1:", quote(n->get_text()));
    else
      return quote("parse_text:#1:" + n->get_text());
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
      parse_code(c, ret);
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
      ret->add("<code>", ref, "</code>");
      break;

    case "rfc":
      {
        string rfc = upper_case(parse_text(c));
        string section;
        sscanf(rfc, "%[0-9]:%[.A-Z0-9]", rfc, section);
        if (sizeof(rfc) < 4) rfc = ("0000" + rfc)[<3..];
        ret->add("<b><a href='http://pike.lysator.liu.se/rfc", rfc, ".xml");
        if( section && sizeof(section) )
          ret->add("#", section);
        else
          section = 0;
        ret->add("'>RFC ", rfc);
        if( section )
        {
          if( section[0] > '9' )
            ret->add(" appendix ");
          else
            ret->add(" section ");
          ret->add(section);
        }
        ret->add("</a></b>");
      }
      break;

    case "dl":
      ret->add("<dl class='group--doc'>", map(c->get_elements("group"), parse_text)*"", "</dl>");
      break;

    case "item":
      if(c->get_attributes()->name) {
	ret->add("<dt>", c->get_attributes()->name, "</dt>\n");
	if(c->count_children() &&
	   ((flags & (Tools.AutoDoc.FLAG_KEEP_GOING|Tools.AutoDoc.FLAG_DEBUG)) ==
	    Tools.AutoDoc.FLAG_DEBUG)) {
	  error( "dl item has a child.\n" );
	}
      } else if (c->count_children()) {
	ret->add("<dt>");
	parse_text(c, ret);
	ret->add("</dt>");
      }
      break;

    case "mapping":
      build_box(c, ret, "group", "member",
		lambda(Node n) {
		  string res = "";
		  Node nn = n->get_first_element("index");
		  if (nn) {
		    res +=
		      "<code class='key'>" + parse_text(nn) + "</code> : ";
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
		    " <code class='key'>" + index + "</code>"; }, "Array" );
      break;

    case "int":
      build_box(c, ret, "group", "value",
		lambda(Node n) {
           return "<code class='key'>" +
               range_type( parse_text(n),
                           n->get_first_element("minvalue"),
                           n->get_first_element("maxvalue"))+
               "</code>";
		} );
      break;

    case "mixed":
      if(c->get_attributes()->name)
	ret->add("<code>", c->get_attributes()->name, "</code> can have any of the following types:<br />");
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
            return "<code class='key'>" +
                range_type( parse_text(n),
                            n->get_first_element("min"),
                            n->get_first_element("max")) +
                "</code>";
		} );
      break;

    case "multiset": // Not in XSLT
      build_box(c, ret, "group", "index",
		lambda(Node n) {
		  return "<code class='key'>" +
		    parse_text(n->get_first_element("value")) + "</code>";
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
    case "ol":
      ret->add( "<", name, ">\n" );
      foreach(c->get_elements("group"), Node c) {
	int got_item;
	foreach(c->get_elements(), Node e) {
	  switch(e->get_any_name()) {
	  case "item":
	    if (got_item) {
	      ret->add("</li>");
	    }
	    ret->add("<li>");
	    string title = e->get_attributes()->name;
	    if (title) {
	      ret->add("<b>", title, "</b>");
	    }
	    got_item = 1;
	    break;
	  case "text":
	    if (!got_item) {
	      ret->add("<li>");
	    }
	    parse_text(e, ret);
	    ret->add("</li>");
	    got_item = 0;
	    break;
	  default:
	    // Ignored.
	    break;
	  }
	}
	if (got_item) {
	  ret->add("</li>");
	  got_item = 0;
	}
      }
      ret->add("</", name, ">");
      break;

    case "source-position":
      position->update(c);
      break;

    case "matrix":
      ret->add( layout_matrix( map(c->get_elements("r")->get_elements("c"),
				   map, parse_text) ) );
      break;

    case "fixme":
      ret->add("<span class='fixme'>FIXME: ");
      parse_text(c, ret);
      ret->add("</span>");
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
  if(c) {
    ret += lay->dochead + "Description" + lay->_dochead +
      lay->docbody + parse_text(c) + lay->_docbody;
  }

#define MAKE_ID(N) ("<span id='p-" + N + "'></span>")

  foreach(n->get_elements("group"), Node c) {
    Node header = c->get_first_element();
    string name = header->get_any_name();
    switch(name) {
    case "param":
      foreach(c->get_elements("param"), Node d) {
        string name = quote(d->get_attributes()->name || "");
	ret += lay->dochead + MAKE_ID(name) + "Parameter " +
          lay->parameter + name + lay->_parameter + lay->_dochead +
	  "<dd></dd>";
      }
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
	ret += "<code class='object resolved'>" +
	  resolve_reference(n->value_of_node(), n->get_attributes()) +
	  "</code>";
      } else {
	ret += "<code class='object unresolved'>" +
               n->value_of_node() + "</code>";
      }
    } else
      ret += "<code class='datatype'>object</code>";
    break;

  case "type":
    ret += "<code class='type'>type</code>";
    if (n->count_children() && (c = get_first_element(n)) &&
	(c->get_any_name() != "mixed")) {
      ret += "(" + parse_type(c) + ")";
    }
    break;

  case "multiset":
    ret += "<code class='datatype'>multiset</code>";
    c = n->get_first_element("indextype");
    if(c) ret += "(" + parse_type( get_first_element(c) ) + ")";
    break;

  case "array":
    ret += "<code class='datatype'>array</code>";
    c = n->get_first_element("valuetype");
    if(c) ret += "(" + parse_type( get_first_element(c) ) + ")";
    break;

  case "mapping":
    ret += "<code class='datatype'>mapping</code>";
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
    ret += "<code class='datatype'>function</code>";
    array(Node) args = n->get_elements("argtype");
    d = n->get_first_element("returntype");
    // Doing different than the XSL here. Must both
    // argtype and returntype be defined?
    if(args || d) {
      ret += "(";
      if(args) ret += map(args->get_children() * ({}), parse_type)*", ";
      ret += ":";
      if(d) ret += parse_type( get_first_element(d) );
      else ret += "<code class='datatype void'>void</code>";
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
    ret += "<code class='datatype'>" + n->get_any_name() + "</code>";
    break;

  case "string":
  case "int":
      ret += ("<code class='datatype'>" +
              range_type( n->get_any_name(),
                          n->get_first_element("min"),
                          n->get_first_element("max")) +
              "</code>");
    break;

  case "attribute":
    string attr = n->get_first_element("attribute")->value_of_node();
    string subtype =
      parse_type(n->get_first_element("subtype")->get_first_element());
    if (n->get_first_element("prefix")) {
      if (attr == "\"deprecated\"") {
	ret += "<code class='deprecated'>__deprecated__</code> " +
               subtype;
      } else {
	ret += sprintf("__attribute__(%s) %s", attr, subtype);
      }
    } else if (attr == "\"deprecated\"") {
      ret += "<code class='deprecated'>__deprecated__</code>(" +
             subtype + ")";
    } else {
      ret += sprintf("__attribute__(%s, %s)", attr, subtype);
    }
    break;

    // Modifiers:
  case "extern": // Not in XSLT
    ret += "<code class='modifier'>extern</code> ";
    break;
  case "final": // Not in XSLT
  case "nomask": // Not in XSLT
    ret += "<code class='modifier'>final</code> ";
    break;
  case "inline": // Not in XSLT
  case "local": // Not in XSLT
    ret += "<code class='modifier'>local</code> ";
    break;
  case "optional": // Not in XSLT
    ret += "<code class='modifier'>optional</code> ";
    break;
  case "private": // Not in XSLT
    ret += "<code class='modifier'>private</code> ";
    break;
  case "protected": // Not in XSLT
  case "static": // Not in XSLT
    ret += "<code class='modifier'>protected</code> ";
    break;
  case "public": // Not in XSLT
    // Ignored.
    break;
  case "variant": // Not in XSLT
    ret += "<code class='modifier'>variant</code> ";
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
  case "annotations":
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
string class_name = "";

#define HTML_ENC(S) (Parser.encode_html_entities(S))

string parse_not_doc(Node n) {
  string ret = "";
  int method, argument, variable, num_const, typedf, cppdir;

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
      if(method++) ret += "<br>\n";
#if 0
      if(!c->get_first_element("returntype"))
	error( "No returntype element in method element.\n" );
#endif

      void emit_default_func()
      {
          ret += "<code>";
          cc = c->get_first_element("modifiers");
          if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
          ret += parse_type(get_first_element(c->get_first_element("returntype")));
          ret += " ";
          ret += c->get_attributes()->class_path;
          ret += "<b><span class='method'>" + HTML_ENC(c->get_attributes()->name) + "</span>(</b>";
          ret += parse_not_doc( c->get_first_element("arguments") );
          ret += "<b>)</b>";
          ret += "</code>";
      };
      if( class_name == "" )
      {
        emit_default_func();
      }
      else switch( string method = c->get_attributes()->name )
      {
        case "_get_iterator":
          ret += "<code><span class='class'>"+class_name+"</span> "
                 "<span class='method'>a</span>;<br>\n";
          /* NOTE: We could get index and value types from the
             iterator type here.  Doing so might be somewhat hard,
             however.
          */
          ret += "foreach( a; index; value ) or<br></code>";
          emit_default_func();
          break;

        case "pow":
          ret += "<code>"+parse_type(get_first_element(c->get_first_element("returntype")));
          ret += " res = "+method+"([<span class='class'>"+class_name+"</span>]a, b) or <br></code>";
          emit_default_func();
          break;

        case "sqrt":
          ret += "<code>"+parse_type(get_first_element(c->get_first_element("returntype")));
          ret += " res = "+method+"([<span class='class'>"+class_name+"</span>]a) or <br></code>";
          emit_default_func();
          break;
/*
        case "_is_type":
          overloads all the TYPEp() functions and cast.

          ret += "<tt>bool stringp(["+class_name+"])a</font>) or <br></tt>";
          emit_default_func();
          break;
*/
	case "create":
	  ret += "<code><span class='object'>" +class_name; // Check for more children
	  ret += "</span> <span class='class'>";
	  ret += class_name+"</span><b>(</b>";
	  ret += parse_not_doc( c->get_first_element("arguments") );
	  ret += "<b>)</b></code>";
	  break;

        case "__hash":
          method = "_hash_value";

        case "_random":
        case "_sqrt":
        case "_sizeof":
        case "_indices":
        case "_values":
          /* simple overload type lfuns. */
          ret += "<code>";
          ret += parse_type(get_first_element(c->get_first_element("returntype")));
          ret += " ";
          ret += "<b><span class='method'>"+method[1..]+"</span>(</b> ";
          ret += "<span class='class'>"+class_name+"</span> <span class='argument'>arg</span>";
          ret += " <b>)</b></code>";
          break;

        case "_m_delete":
        case "_equal":
        case "_search":
          ret += "<code>";
          ret += parse_type(get_first_element(c->get_first_element("returntype")));
          ret += " <b><span class='method'>"+method[1..];
          ret += "</span>(</b><span class='class'>"+class_name+"</span> "
                 "<span class='argument'>from</span>, ";
          ret += parse_not_doc( c->get_first_element("arguments") );
          ret += "<b>)</b></code>";
          break;

        case "_sprintf":
          //ret += "<tt>";
          ret += "<code><span class='datatype'>string</span> ";
          ret += ("<b><span class='method'>sprintf</span>("
                  "</b><span class='datatype'>string</span> "
                  "<span class='constant'>format</span>, ... <span class='class'>"
                  +class_name+"</span> <span class='constant'>"
                  "arg</span> ... <b>)</b></code>");
          break;

        case "_decode":
          ret += "<code>";
          ret += "<span class='class'>"+class_name+"</span> ";
          ret += ("<b><span class='method'>decode_value</span>("
                  "</b><span class='datatype'>string(8bit)</span> "
                  "<span class='argument'>data</span>)</b></code>");
          break;
        case "_encode":
          ret += "<code>";
          ret += "<span class='datatype'>string(8bit)</span> ";
          ret += ("<b><span class='method'>encode_value</span>("
                  "</b><span class='class'>"+class_name+"</span> "
                  "<span class='argument'>data</span>)</b></code>");
          break;

        case "cast":
          {
            ret += "<code>";

            string base =
              "<b>(</b><span class='datatype'>_@_TYPE_@_</span>"
              "<b>)</b><span class='class'>"+class_name+"</span>()";

            multiset seen = (<>);
            void add_typed( string type )
            {
              if( type == "mixed" )
              {
                add_typed("int"); ret +="<br>";
                add_typed("float"); ret +="<br>";
                add_typed("string"); ret +="<br>";
                add_typed("array"); ret +="<br>";
                add_typed("mapping"); ret +="<br>";
                add_typed("multiset");
                return;
              }
              if( !seen[type]++ )
                ret += replace(base,"_@_TYPE_@_", type);
            };

            void output_from_type(Node x, bool toplevel )
            {
              if( !x || x->get_node_type() != XML_ELEMENT ) return;
              switch( x->get_any_name() )
              {
                case "object": /*add_typed("object"); */break; /* this is a no-op.. */
                case "int":
                case "float":
                case "array":
                case "mapping":
                case "string":
                case "function":
                  add_typed(parse_type(x));
                  if( !toplevel )
                    ret += "<br>";
                  break;
                case "mixed":
                  add_typed("mixed");
                  if( !toplevel )
                    ret += "<br>";
                break;
                case "or":
                  if( toplevel )
                    map( x->get_children(), output_from_type, false );
              }
            };
            output_from_type(get_first_element(c->get_first_element("returntype")),
                             true);
            if( !sizeof(seen) )
              add_typed("mixed");

            ret += "</code>";
          };
          break;

        default:
          if( string pat = Tools.AutoDoc.PikeObjects.lfuns[method] )
          {
            Node a = c->get_first_element("arguments") ;
            array(Node) args = a->get_elements("argument");
            mapping repl = (["OBJ":"<code class='class'>"+
                                   class_name+"()</code>"]);
            /* there are a few cases:  obj OP arg - we do not want types
               RET func(op,ARG) - we do want types.

               This code should only be triggered for the first case.
               Hence, no types wanted.
            */
            if( sizeof(args) > 0 )
            {
              repl->x = "<code class='class'>"+
                        args[0]->get_attributes()->name+"</code>";
            }
            if( sizeof(args) > 1 )
            {
              repl->y = "<code class='class'>"+
                        args[1]->get_attributes()->name+"</code>";
            }

            ret += "<code>";
            if( method != "`+=" && method != "`[]=" && method != "`->=")
            {
              ret += parse_type(get_first_element(c->get_first_element("returntype")));
              ret += " res = ";
            }

            ret += replace(HTML_ENC(pat), repl );
            ret += "</code>";
            break;
          }
          emit_default_func( );
          break;
      }
      break;

    case "argument":
      if(argument++) ret += ", ";
      cc = c->get_first_element("value");
      string arg_name = cc && cc->value_of_node();
      if (arg_name) {
        ret += "<code class='argument'>" + arg_name + "</code>";
      }
      else if( !c->count_children() );
      else if( get_first_element(c)->get_any_name()=="type") {
	ret += parse_type(get_first_element(get_first_element(c)));
        arg_name = c->get_attributes()->name;
	if(arg_name) {
	  ret += " <code class='argument'>" + arg_name + "</code>";
        }
      }
      else
	error( "Malformed argument element.\n" + c->html_of_node() + "\n" );
      break;

    case "variable":
      if(variable++) ret += "<br>\n";
      ret += "<code>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      if (c->get_first_element("type")) {
	ret += parse_type(get_first_element(c->get_first_element("type")),
			  "variable") + " " +
	  c->get_attributes()->class_path + "<b><span class='variable'>" +
	  c->get_attributes()->name + "</span></b></code>";
      } else
	error("Malformed variable element.\n" + c->html_of_node() + "\n");
      break;

    case "constant":
      if(num_const++) ret += "<br>\n";
      ret += "<code><code class='datatype'>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += "constant</code> ";
      if (Node type = c->get_first_element ("type"))
	ret += parse_type (get_first_element (type), "constant") + " ";
      ret += c->get_attributes()->class_path;
      ret += "<code class='constant'>" + c->get_attributes()->name + "</code>";
      cc = c->get_first_element("typevalue");
      if(cc) {
        ret += " = <code class='value'>" +
               parse_type(get_first_element(cc)) + "</code>";
      }
      ret += "</code>";
      break;

    case "typedef":
      if(typedf++) ret += "<br>\n";
      ret += "<code><code class='datatype'>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += "typedef</code> ";
      ret += parse_type(get_first_element(c->get_first_element("type")), "typedef") + " " +
	c->get_attributes()->class_path +
        "<code class='typedef'>" + c->get_attributes()->name + "</code></code>";
      break;

    case "inherit":
      ret += "<code><span class='datatype'>";
      cc = c->get_first_element("modifiers");
      if(cc) ret += map(cc->get_children(), parse_type)*" " + " ";
      ret += "inherit ";
      Node n = c->get_first_element("classname");
      if (resolve_reference) {
	ret += "</span>" +
	  resolve_reference(n->value_of_node(), n->get_attributes());
      } else {
	ret += Parser.encode_html_entities(n->value_of_node()) + "</span>";
      }
      if (c->get_attributes()->name) {
	ret += " : " + "<span class='inherit'>" +
	  Parser.encode_html_entities(c->get_attributes()->name) +
	  "</span>";
      }
      ret += "</code>";
      break;

    // We don't need import information.
    case "import":
      break;

    case "directive":
      if(cppdir++) ret += "<br>\n";
      ret += "<code class='directive'>" + quote(c->get_attributes()->name) +
	     "</code>";
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
      string type = "<span class='homogen--type'>" +
	quote(String.capitalize(m["homogen-type"])) +
	"</span>\n";
      if(m["homogen-name"]) {
	ret += type + "<span class='homogen--name'><b>" +
	  quote((m->belongs?m->belongs+" ":"") + m["homogen-name"]) +
	  "</b></span>\n";
      } else {
	array(string) names =
	  Array.uniq(map(n->get_elements(m["homogen-type"]),
			 lambda(Node child) {
			   return child->get_attributes()->name ||
			     child->value_of_node();
			 }));
	foreach(names, string name)
	  ret += type +
	    "<span class='homogen--name'><b>" + quote(name) + "</b></span><br>\n";
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

// This can be overridden by passing --template=template.html to main
string html_template =
        "<!doctype html><html><head><title>$title$</title>\n"
        "<meta charset='utf-8'></head>\n"
        "<body>$contents$</body></html>";
string manual_title = "Pike Reference Manual";
string frame_html(string res, void|string title) {
  title = title || manual_title;
  return replace(html_template, ([ "$title$" : quote(title),
                                   "$contents$" : res ]));
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
	exporter->data(string_to_utf8(html));
	break;
      }
      if(dest_path)
      {
        cwd=getcwd();
        cd(dest_path);
      }
      Stdio.write_file( c->get_attributes()->name,
			string_to_utf8(frame_html(layout_toploop(c))) );
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

void help(void|string err)
{
  string msg = #"pike -x autodoc_to_html [args] <input file> [<output file>]
--img=<image path>
--dest=<destination path>

--title=<document title>

--template=<template.html path>
";

  if( err )
    exit(1, err+"\n"+msg);
  write(msg);
  exit(0);
}

int main(int num, array args) {

  string title, template;

  foreach(Getopt.find_all_options(args, ({
    ({ "img",      Getopt.HAS_ARG, "--img"   }),
    ({ "dest",     Getopt.HAS_ARG, "--dest"  }),
    ({ "title",    Getopt.HAS_ARG, "--title" }),
    ({ "template", Getopt.HAS_ARG, "--template" }),
    ({ "defns",    Getopt.HAS_ARG, "--default-ns" }),
    ({ "verbose",  Getopt.NO_ARG,  "-v,--verbose"/"," }),
    ({ "quiet",    Getopt.NO_ARG,  "-q,--quiet"/"," }),
    ({ "help",     Getopt.NO_ARG,  "--help"  }),
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
    case "template":
      template = opt[1];
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
      help();
      break;
    }
  args = Getopt.get_args(args)[1..];

  if(!sizeof(args))
  {
    help( "No input file given.\n" );
  }

  if (template && Stdio.exist(template)) {
    string dir = dirname(template);
    html_template = Stdio.read_file(template);
    Parser.HTML p = Parser.HTML();
    p->add_tag("link", lambda (Parser.HTML pp, mapping attr) {
      if (attr->href && attr["data-inline"]) {
        return "<style>" + (Stdio.read_file(combine_path(dir, attr->href))) +
               "</style>";
      }
    });

    html_template = p->feed(html_template)->finish()->read();
  }

  return low_main(title, @args);
}
