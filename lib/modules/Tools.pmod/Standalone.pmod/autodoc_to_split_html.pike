#pike __REAL_VERSION__

inherit .autodoc_to_html;

constant description = "AutoDoc XML to splitted HTML converter.";

/* Todo:
 *
 *   * List of classes (and modules?) that inherit the class (or module?).
 *
 *   * Inherited enums, variables and constants.
 *
 *   * Inherited functions and classes.
 */

mapping (string:Node) refs   = ([ ]);
string default_namespace;

string extra_prefix = "";
string low_image_prefix()
{
  return ::image_prefix();
}
string image_prefix()
{
  return extra_prefix + ::image_prefix();
}

int unresolved;
mapping profiling = ([]);
#define PROFILE int profilet=gethrtime
#define ENDPROFILE(X) profiling[(X)] += gethrtime()-profilet;

constant navigation_js = #"
/* Functions for rendering the navigation. */

/* The variable current is assumed to have been set to the node
 * representing the current document.
 */

/* These are all arrays of nodes. */
var module_children = [];
var class_children = [];
var enum_children = [];
var directive_children = [];
var method_children = [];
var namespace_children = [];
var appendix_children = [];
var children = [];
var siblings = [];

function clear_children()
{
  module_children = [];
  class_children = [];
  enum_children = [];
  directive_children = [];
  method_children = [];
  namespace_children = [];
  appendix_children = [];
  children = [];
}

function cmp_nodes(/* node */a, /* node */b)
{
  return (a.name > b.name) - (b.name > a.name);
}

function merge_node_lists(/* array(node) */old, /* array(node) */nodes)
{
  var i;
  var hash = {};
  for (i=0; i < old.length; i++) {
    var node = old[i];

    hash[node.name] = i+1;
  }
  for (i = 0; i < nodes.length; i++) {
    var node = nodes[i];
    var j = hash[node.name];
    if (j) {
      old[j-1] = node;
    } else {
      old[old.length] = node;
    }
  }
  old.sort(cmp_nodes);
  return old;
}

function add_module_children(/* array(node) */children)
{
  module_children = merge_node_lists(module_children, children);
}

function add_class_children(/* array(node) */children)
{
  class_children = merge_node_lists(class_children, children);
}

function add_enum_children(/* array(node) */children)
{
  enum_children = merge_node_lists(enum_children, children);
}

function add_directive_children(/* array(node) */children)
{
  directive_children = merge_node_lists(directive_children, children);
}

function add_method_children(/* array(node) */children)
{
  method_children = merge_node_lists(method_children, children);
}

function add_namespace_children(/* array(node) */children)
{
  namespace_children = merge_node_lists(namespace_children, children);
}

function add_appendix_children(/* array(node) */children)
{
  appendix_children = merge_node_lists(appendix_children, children);
}

function low_end_inherit(/* array(node) */nodes)
{
  var i;
  for (i=0; i < nodes.length; i++) {
    nodes[i].inherited = 1;
  }
}

function end_inherit()
{
  low_end_inherit(module_children);
  low_end_inherit(class_children);
  low_end_inherit(enum_children);
  low_end_inherit(directive_children);
  low_end_inherit(method_children);
  low_end_inherit(namespace_children);
  low_end_inherit(appendix_children);
}

function emit_end_inherit()
{
  document.write(\"<script language='javascript'>\\n\" +
                 \"end_inherit();\\n\" +
                 \"</script>\\n\");
}

function escapehtml(/* string */text)
{
  text = text.split('&').join('&amp;');
  text = text.split('<').join('&lt;');
  return text.split('>').join('&gt;');
}

function basedir(/* string */path)
{
  var i = path.lastIndexOf('/');
  if (i < 1) return '';
  return path.substring(0, i);
}

function has_prefix(/* string */prefix, /* string */other)
{
  return other.substring(0, prefix.length) == prefix;
}

function adjust_link(/* string */link)
{
  var reldir = basedir(current.link);
  var dots = '';
  while (reldir != '' && !has_prefix(link, reldir + '/')) {
    dots += '../';
    reldir = basedir(reldir);
  }
  return dots + link.substring(reldir.length);
}

function emit_load_js(/* string */link)
{
  document.write(\"<script language='javascript' src='\" +
                 adjust_link(link) + \"' >\\n\" +
                 \"</script>\\n\");
}

function low_navbar(/* Document */document, /* string */heading,
                    /* array(node) */nodes, /* string */suffix)
{
  if (!nodes || !nodes.length) return;

  document.write('<br /><b>' + escapehtml(heading) + '</b>\\n');

  document.write('<div style=\"margin-left:0.5em;\">\\n');
  var i;
  for (i=0; i < nodes.length; i++) {
    var n = nodes[i];
    var name = escapehtml(n.name + suffix);
    if (!n.inherited) {
      name = '<b>' + name + '</b>';
    }
    if (n.link == current.link) {
      document.write(name);
    } else {
      document.write('<a href=\"' + adjust_link(n.link) + '\">');
      document.write(name);
      document.write('</a>');
    }
    document.write('<br />\\n');
  }
  document.write('</div>\\n');
}

/* Render the left navigation bar. */
function navbar(/* Document */document)
{
  document.write('<div class=\"sidebar\">\\n');
  low_navbar(document, 'Modules', module_children, '');
  low_navbar(document, 'Classes', class_children, '');
  low_navbar(document, 'Enums', enum_children, '');
  low_navbar(document, 'Directives', directive_children, '');
  low_navbar(document, 'Methods', method_children, '()');
  low_navbar(document, 'Namespaces', namespace_children, '::');
  low_navbar(document, 'Appendices', appendix_children, '');
  document.write('</div>\\n');
}
";

string encode_json(mixed val)
{
  return Standards.JSON.encode(val,
    Standards.JSON.HUMAN_READABLE|Standards.JSON.PIKE_CANONICAL);
}

string cquote(string n)
{
  string ret="";
  // n = replace(n, ([ "&gt;":">", "&lt;":"<", "&amp;":"&" ]));
  while(sscanf((string)n,"%[._a-zA-Z0-9]%c%s",string safe, int c, n)==3) {
    switch(c) {
    default:  ret += sprintf("%s_%02X",safe,c); break;
    case '+': ret += sprintf("%s_add",safe); break;
    case '`': ret += sprintf("%s_backtick",safe);  break;
    case '=': ret += sprintf("%s_eq",safe); break;
    }
  }
  ret += n;
  // Make sure "index.html" is free...
  if (has_suffix(ret, "index")) ret = "_" + ret;
  return ret;
}

// This splits references, but leaves "..+" intact (mainly for refs to `[..]).
array(string) split_reference(string what) {
    array(string) r = what/".";

    for (int i = 1; i < sizeof(r) - 1; i++) {
	if (r[i] == "") {
	    int j = i+1;

	    while (j < sizeof(r) - 1 && r[j] == "") j++;

	    r[i-1] = sprintf("%s.%s%s", r[i-1], "."*(j-i), r[j]);
	    r = r[..i-1] + r[j+1..];
	    i--;
	}
    }

    return r;
}

string create_reference(string from, string to, string text,
			string|void xlink_namespace_prefix) {
  array a = (to/"::");
  switch(sizeof(a)) {
  case 2:
    if (sizeof(a[1])) {
      // Split trailing module path.
      a = a[0..0] + split_reference(a[1]);
    } else {
      // Get rid of trailing "".
      a = a[0..0];
    }
    a[0] += "::";
    break;
  case 1:
    a = split_reference(a[0]);
    break;
  default:
    error("Bad reference: %O\n", to);
  }
  if (!xlink_namespace_prefix) xlink_namespace_prefix = "";
  return "<a style='font-family:courier;' " +
    xlink_namespace_prefix +
    "href='" +
    "../"*max(sizeof(from/"/") - 2, 0) + map(a, cquote)*"/" + ".html'>" +
    text +
    "</a>";
}

multiset missing = (< "foreach", "catch", "throw", "sscanf", "gauge", "typeof" >);

ADT.Stack pending_inherits;

class Node
{
  string type;
  string name;
  string data;
  array(Node) class_children  = ({ });
  array(Node) module_children = ({ });
  array(Node) enum_children = ({ });
  array(Node) directive_children = ({ });
  array(Node) method_children = ({ });
  array(array(string|Node)) inherits = ({});
  array(Node) children = ({});

  Node parent;

  string _sprintf() {
    return sprintf("Node(%O,%O,%d)", type, name, data?sizeof(data):0);
  }

  void create(string _type, string _name, string _data, void|Node _parent)
  {
    if(!_type || !_name) throw( ({ "No type or name\n", backtrace() }) );
    type = _type;
    name = _name;
    parent = _parent;
    data = get_parser()->finish( _data )->read();

    string path = raw_class_path();
    refs[path] = this_object();

    sort(class_children->name, class_children);
    sort(module_children->name, module_children);
    sort(enum_children->name, enum_children);
    sort(directive_children->name, directive_children);
    sort(method_children->name, method_children);

    method_children = check_uniq(method_children);

    /*
    foreach(method_children, Node m)
      if( (<"create","destroy">)[m->name] ) {
	method_children -= ({ m });
	string d;
	sscanf(m->data, "%*s<docgroup%s/docgroup>", d);
	if(d)
	  data += "<docgroup" + d + "/docgroup>";
      }
    */

    data = make_faked_wrapper(data);
  }

  array(Node) check_uniq(array children) {
    array names = children->name;
    foreach(Array.uniq(names), string n)
      if(sizeof(filter(names, lambda(string in) { return in==n; }))!=1) {
	string d="";
	Parser.HTML parser = Parser.HTML();
	parser->case_insensitive_tag(1);
	parser->xml_tag_syntax(3);
	parser->add_container("docgroup",
          lambda(Parser.HTML p, mapping m, string c) {
	    d += sprintf("%s%s</docgroup>", render_tag("docgroup", m), c);
	    return "";
	  });

	foreach(children, Node c)
	  if(c->name==n) parser->feed(c->data);
	parser->finish();
	d = make_faked_wrapper(d);

	foreach(children; int i; Node c)
	  if(c->name==n) {
	    if(d) {
	      c->data = d;
	      d = 0;
	    }
	    else children[i] = 0;
	  }
	children -= ({ 0 });
      }
    return children;
  }

  protected string parse_node(Parser.HTML p, mapping m, string c) {
    if(!m->name) error("Unnamed %O %O\n", p->tag_name(), m);
    this_object()[p->tag_name()+"_children"] +=
      ({ Node( p->tag_name(), m->name, c, this_object() ) });
    return "";
  }

  protected void add_ref(string path, string type, string name,
			 string data, Node n, string c)
  {
    refs[path + name] = Node(type, name, data, n);
    if (type == "inherit") {
      Parser.HTML()->
	add_containers(([ "classname":
			  lambda(Parser.HTML p, mapping m, string c) {
			    string resolved;
			    if (resolved = m->resolved) {
			      resolved = Parser.parse_html_entities(resolved);
			      Node nn = refs[resolved];
			      if (nn) {
				nn->children += ({ n });
			      } else {
				pending_inherits->push(({ resolved, n }));
			      }
			    }
			    n->inherits += ({
			      ({
				name, c, resolved,
			      })
			    });
			  },
		       ]) )->finish(c);
    }
  }

  array(string) my_parse_docgroup(Parser.HTML p, mapping m, string c)
  {
    foreach(({"homogen-name", "belongs"}), string attr) {
      if (m[attr]) m[attr] = Parser.parse_html_entities(m[attr]);
    }
    if(m["homogen-type"]) {
      switch( m["homogen-type"] ) {

      case "method":
	if( m["homogen-name"] ) {
	  string name = m["homogen-name"];
	  if(m->belongs) {
	    if(m->belongs[-1]==':') name = m->belongs + name;
	    else name = m->belongs + "." + name;
	  }
	  method_children +=
	    ({ Node( "method", name, c, this_object() ) });
	  return ({ "" });
	}

	// Several different methods documented with the same blurb.
	array names = ({});
	Parser.HTML parser = Parser.HTML();
	parser->case_insensitive_tag(1);
	parser->xml_tag_syntax(3);
	parser->add_tag("method",
			lambda(Parser.HTML p, mapping m) {
			  names += ({ Parser.parse_html_entities(m->name) });
			} );
	parser->finish(c);
	foreach(Array.uniq(names) - ({ 0, "" }), string name) {
	  method_children +=
	    ({ Node( "method", name, c, this_object() ) });
	}
	return ({ "" });
	break;

      case "directive":
	if( m["homogen-name"] ) {
	  string name = m["homogen-name"];
	  directive_children +=
	    ({ Node( "directive", name, c, this_object() ) });
	  return ({ "" });
	}

	// Several different directives documented with the same blurb.
	array directives = ({});
	Parser.HTML dirparser = Parser.HTML();
	dirparser->case_insensitive_tag(1);
	dirparser->xml_tag_syntax(3);
	dirparser->add_tag("directive",
			   lambda(Parser.HTML p, mapping m) {
			     directives += ({
			       Parser.parse_html_entities(m->name)
			     });
			   } );
	dirparser->finish(c);
	foreach(Array.uniq(directives) - ({ 0, "" }), string name) {
	  directive_children +=
	    ({ Node( "directive", name, c, this_object() ) });
	}
	return ({ "" });
	break;

      case "constant":
      case "variable":
      case "inherit":
	string path = raw_class_path();
	if(sizeof(path) && (path[-1] != ':')) path += ".";
	if(!m["homogen-name"]) {
	  Parser.HTML()->add_tags
	    ( ([ "constant":
		 lambda(Parser.HTML p, mapping m, string c) {
		   string name = Parser.parse_html_entities(m->name);
		   add_ref(path, "constant", name, "", this_object(), c);
		 },
		 "variable":
		 lambda(Parser.HTML p, mapping m, string c) {
		   string name = Parser.parse_html_entities(m->name);
		   add_ref(path, "variable", name, "", this_object(), c);
		 },
		 "inherit":
		 lambda(Parser.HTML p, mapping m, string c) {
		   if (m->name) {
		     string name = Parser.parse_html_entities(m->name);
		     add_ref(path, "inherit", name, "", this_object(), c);
		   }
		 },
	    ]) )->finish(c);
	}
	else
	  add_ref(path, m["homogen-type"], m["homogen-name"],
		  "", this_object(), c);
	break;

      }
    }

    return 0;
  }

  protected Parser.HTML get_parser() {
    Parser.HTML parser = Parser.HTML();

    parser->case_insensitive_tag(1);
    parser->xml_tag_syntax(3);

    parser->add_container("docgroup", my_parse_docgroup);
    parser->add_container("module", parse_node);
    parser->add_container("class",  parse_node);
    parser->add_container("enum",  parse_node);

    return parser;
  }

  string make_faked_wrapper(string s)
  {
    if (type=="appendix")
      return "<appendix name='" + Parser.encode_html_entities(name) + "'>" +
	s + "</appendix>";

    if((type == "method") || (type == "directive"))
      s = sprintf("<docgroup homogen-type='%s' homogen-name='%s'>\n"
		  "%s\n</docgroup>\n",
		  type,
		  Parser.encode_html_entities(name), s);
    else
      s = sprintf("<%s name='%s'>\n%s\n</%s>\n",
		  type, Parser.encode_html_entities(name), s, type);
    if(parent)
      return parent->make_faked_wrapper(s);
    else
      return s;
  }

  string _make_filename_low;
  string make_filename_low()
  {
    if(_make_filename_low) return _make_filename_low;
    if (type == "namespace") {
      _make_filename_low = parent->make_filename_low()+"/"+cquote(name+"::");
    } else {
      _make_filename_low = parent->make_filename_low()+"/"+cquote(name);
    }
    return _make_filename_low;
  }

  string make_filename()
  {
    return make_filename_low()+".html";
  }

  string make_index_filename()
  {
    if((type == "method") || (type == "directive")) {
      return parent->make_index_filename();
    }
    // NB: We need the full path for the benefit of the exporter.
    return make_filename_low() + "/index";
  }

  string make_load_index_filename()
  {
    if((type == "method") || (type == "directive")) {
      return parent->make_load_index_filename();
    }
    // NB: We need the full path for the benefit of the exporter.
    return make_filename_low() + "/load_index.js";
  }

  string make_parent_index_filename()
  {
    if (parent) return parent->make_index_filename();
    return make_index_filename();
  }

  string low_make_link(string to, int|void extra_levels)
  {
    // FIXME: Optimize the length of relative links
    int num_segments = sizeof(make_filename()/"/") + extra_levels - 1;
    return ("../"*num_segments)+to;
  }

  string make_link(Node to, int|void extra_levels)
  {
    return low_make_link(to->make_filename(), extra_levels);
  }

  array(Node) get_ancestors()
  {
    PROFILE();
    array tmp = ({ this_object() }) + parent->get_ancestors();
    ENDPROFILE("get_ancestors");
    return tmp;
  }

  string my_resolve_reference(string _reference, mapping vars)
  {
    array(string) resolved = vars->resolved && vars->resolved/"\0";
    if(default_namespace && has_prefix(_reference, default_namespace+"::"))
      _reference = _reference[sizeof(default_namespace)+2..];

    if(vars->param)
      return "<font face='courier'>" + _reference + "</font>";

    if (resolved) {
      foreach (resolved, string resolution) {
	Node res_obj;

	if(res_obj = refs[resolution]) {
	  while(res_obj && (<"constant","variable">)[res_obj->type]) {
	    res_obj = res_obj->parent;
	  }
	  if (!res_obj && verbosity) {
	    werror("Found no page to link to for reference %O (%O)\n",
		   _reference, resolution);
	    return sprintf("<font face='courier'>" + _reference + "</font>");
	  }
	  // FIXME: Assert that the reference is correct?
	  return create_reference(make_filename(),
				  res_obj->raw_class_path(),
				  _reference);
	}

	// Might be a reference to a parameter.
	// Try cutting of the last section, and retry.
	array(string) tmp = resolution/".";
	if ((sizeof(tmp) > 1) && (res_obj = refs[tmp[..sizeof(tmp)-2]*"."])) {
	  if (res_obj == this_object()) {
	    return sprintf("<font face='courier'>" + _reference + "</font>");
	  }
	  return create_reference(make_filename(),
				  res_obj->raw_class_path(),
				  _reference);
	}

	if (!zero_type(refs[resolution]) &&
	    (verbosity >= Tools.AutoDoc.FLAG_VERBOSE)) {
	  werror("Reference %O (%O) is %O!\n",
		 _reference, resolution, refs[resolution]);
	}
      }
      if (!missing[vars->resolved] && !has_prefix(_reference, "lfun::") &&
	  verbosity) {
	werror("Missed reference %O (%{%O, %}) in %s\n",
	       _reference, resolved||({}), make_class_path());
#if 0
	werror("Potential refs:%O\n",
	       sort(map(resolved,
			lambda (string resolution) {
			  return filter(indices(refs),
					glob(resolution[..sizeof(resolution)/2]+
					     "*", indices(refs)[*]));
			}) * ({})));
#endif /* 0 */
      }
      unresolved++;
    }
    return "<font face='courier'>" + _reference + "</font>";
  }

  string _make_class_path;
  string _raw_class_path;
  string make_class_path(void|int(0..1) header)
  {
    if(_make_class_path) return _make_class_path;
    array a = reverse(parent->get_ancestors());

    _make_class_path = "";
    _raw_class_path = "";
    foreach(a, Node n)
    {
      // Hide most namepaces from the class path.
      if (n->type == "namespace") {
	_raw_class_path += n->name + "::";
	if ((<"","lfun">)[n->name]) {
	  _make_class_path += n->name + "::";
	}
      } else {
	_raw_class_path += n->name + ".";
	_make_class_path += n->name;
	if(n->type=="class")
	  _make_class_path += "()->";
	else if(n->type=="module")
	  _make_class_path += ".";
      }
    }
    _make_class_path += name;
    _raw_class_path += name;

    if(type=="method") {
      _make_class_path += "()";
    } else if (type == "namespace") {
      _make_class_path += "::";
      _raw_class_path += "::";
    }

    return _make_class_path;
  }
  string raw_class_path(void|int(0..1) header)
  {
    if(_raw_class_path) return _raw_class_path;
    make_class_path(header);
    return _raw_class_path;
  }

  string low_make_classic_navbar(array(Node) children, string what,
				 int|void extra_levels)
  {
    if(!sizeof(children)) return "";

    String.Buffer res = String.Buffer(3000);
    res->add("<br /><b>", what, "</b>\n"
	     "<div style='margin-left:0.5em;'>\n");

    foreach(children, Node node)
    {
      string my_name = Parser.encode_html_entities(node->name);
      if(node->type=="method") {
	my_name+="()";
	if (node == this_object()) {
	  my_name="<b>"+my_name+"</b>";
	}
      } else if (node->type == "namespace") {
	my_name="<b>"+my_name+"::</b>";
      }
      else
	my_name="<b>"+my_name+"</b>";

      if(node==this_object())
	res->add( my_name, "<br />\n" );
      else
	res->add( "<a href='", make_link(node, extra_levels), "'>",
		  my_name, "</a><br />\n" );
    }
    res->add("</div>\n");
    return (string)res;
  }

  string make_hier_list(Node node, int|void extra_levels)
  {
    string res="";

    if(node)
    {
      if(node->type=="namespace" && node->name==default_namespace)
	node = node->parent;
      res += make_hier_list(node->parent, extra_levels);

      string my_class_path =
	(node->is_TopNode)?"[Top]":node->make_class_path();

      if(node == this_object())
	res += sprintf("<b>%s</b><br />\n",
		       Parser.encode_html_entities(my_class_path));
      else
	res += sprintf("<a href='%s'><b>%s</b></a><br />\n",
		       make_link(node, extra_levels),
		       Parser.encode_html_entities(my_class_path));
    }
    return res;
  }

  string make_classic_navbar(int|void extra_levels)
  {
    string res="";

    res += make_hier_list(this_object(), extra_levels);

    res += low_make_classic_navbar(module_children, "Modules", extra_levels);
    res += low_make_classic_navbar(class_children, "Classes", extra_levels);

    return res;
  }

  string make_navbar_low(Node root)
  {
    string res="";

    res += make_hier_list(root);

    return res +
      "<script language='javascript'>navbar(document)</script>\n"
      "<noscript>\n"
      "<div class='sidebar'>\n"
      "<a href='" + low_make_link(make_index_filename() + ".html") + "'>"
      "<b>Symbol index</b></a><br />\n"
      "</div>\n"
      "</noscript>\n";
  }

  string make_navbar()
  {
    if((type == "method") || (type == "directive"))
      return make_navbar_low(parent);
    else
      return make_navbar_low(this_object());
  }

  string low_make_index_js(string name, array(Node) nodes)
  {
    if (!sizeof(nodes)) return "";
    array(mapping(string:string)) a =
      map(nodes,
	  lambda(Node n) {
	    return ([ "name":n->name,
		      "link":n->make_filename() ]);
	  });
    sort(a->name, a);
    return sprintf("add_%s(%s);\n", name, encode_json(a));
  }

  string make_index_js()
  {
    string res = "// Direct symbols for " + make_class_path() + ".\n\n";

#define LOW_MAKE_INDEX_JS(CHILDREN) do {				\
      res += low_make_index_js(#CHILDREN, CHILDREN);			\
    } while (0)

    LOW_MAKE_INDEX_JS(module_children);
    LOW_MAKE_INDEX_JS(class_children);
    LOW_MAKE_INDEX_JS(enum_children);
    LOW_MAKE_INDEX_JS(directive_children);
    LOW_MAKE_INDEX_JS(method_children);

    return res +
      "children = module_children.concat(class_children,\n"
      "                                  enum_children,\n"
      "                                  directive_children,\n"
      "                                  method_children);\n";
  }

  string make_load_index_js()
  {
    string res = "";
    if (sizeof(inherits)) {
      foreach(inherits, array(string|Node) inh) {
	Node n = objectp(inh[2])?inh[2]:refs[inh[2]];
	if (!n) continue;

	res += sprintf("// Inherit %s.\n"
		       "emit_load_js(%q);\n"
		       "\n",
		       n->make_class_path(),
		       n->make_load_index_filename());
      }
      if (sizeof(res)) {
	res = sprintf("// Load the symbols from our inherits.\n"
		      "\n"
		      "%s"
		      "emit_end_inherit();\n"
		      "\n",
		      res);
      }
    }
    res = sprintf("// Indirect loader of the symbol index for %s.\n"
		  "\n"
		  "%s"
		  "emit_load_js(%q);\n",
		  make_class_path(),
		  res,
		  make_index_filename() + ".js");
    return res;
  }

  array(Node) find_siblings()
  {
    return parent->find_children();
  }

  array(Node) find_children()
  {
    return
      class_children+
      module_children+
      enum_children+
      directive_children+
      method_children;
  }

  Node find_prev_node()
  {
    array(Node) siblings = find_siblings();
    int index = search( siblings, this_object() );

    Node tmp;

    if(index==0 || index == -1)
      return parent;

    tmp = siblings[index-1];

    while(sizeof(tmp->find_children()))
      tmp = tmp->find_children()[-1];

    return tmp;
  }

  Node find_next_node(void|int dont_descend)
  {
    if(!dont_descend && sizeof(find_children()))
      return find_children()[0];

    array(Node) siblings = find_siblings();
    int index = search( siblings, this_object() );

    Node tmp;
    if(index==sizeof(siblings)-1)
      tmp = parent->find_next_node(1);
    else
      tmp = siblings[index+1];
    return tmp;
  }

  protected void make_inherit_graph(String.Buffer contents)
  {
    contents->add(lay->dochead, "Inheritance graph", lay->_dochead);

    string this_ref = raw_class_path();

    mapping(string:string) names = ([
      this_ref:this_ref,
    ]);

    // The number of non-graphed dependencies.
    mapping(string:multiset(string)) forward_deps = ([]);
    mapping(string:multiset(string)) backward_deps = ([]);

    // First find all the inherits and assign the appropriate weights.
    mapping(string:int) closure = ([]);
    ADT.Queue q = ADT.Queue();
    q->put(({ 1, "", this_ref, this_ref }));
    while(sizeof(q)) {
      [int weight, string name, string text, string ref] = q->get();
      if (!ref) ref = text;
      if (!names[ref]) names[ref] = text;
      if (closure[ref] >= weight) continue; // Already handled.
      closure[ref] = weight;
      Node n = refs[ref];
      if (!n) {
	forward_deps[ref] = (<>);
	continue;
      }
      names[ref] = n->raw_class_path();
      multiset(string) inhs = (<>);
      if (sizeof(n->inherits)) {
	foreach(n->inherits, array(string) entry) {
	  if (entry[2] == ref) {
	    if (verbosity && (ref == this_ref)) {
	      werror("Circular inherit in %s.\n", this_ref);
	    }
	    continue;	// Infinite loop averted.
	  }
	  q->put(({ weight + 1 }) + entry);
	  inhs[entry[2]||entry[1]] = 1;
	}
	foreach(inhs; string inh_ref;) {
	  if (!backward_deps[inh_ref]) {
	    backward_deps[inh_ref] = (<>);
	  }
	  backward_deps[inh_ref][ref] = 1;
	}
      }
      forward_deps[ref] = inhs;
    }

    foreach(children, Node n) {
      string ref = n->raw_class_path();
      names[ref] = ref;
    }

    // Adjust, quote and link the names.
    int no_predef_strip;
    foreach(values(names), string name) {
      if ((!has_prefix(name, "predef::") && has_value(name, "::")) ||
	  name == "predef::") {
	no_predef_strip = 1;
      }
    }
    foreach(names; string ref; string name) {
      if (!no_predef_strip && has_prefix(name, "predef::")) {
	name = name[sizeof("predef::")..];
      }
      name = Parser.encode_html_entities(name);
#ifdef NO_SVG
      if (ref == this_ref) {
	name = "<b style='font-family:courier;'>" + name + "</b>";
      } else if (refs[ref]) {
	name = create_reference(make_filename(), ref, name);
      } else {
	name = "<span style='font-family:courier;'>" + name + "</span>";
      }
#else
      if (ref == this_ref) {
	name = "<tspan style='font-family:courier; font-weight:bold;'>" +
	  name + "</tspan>";
      } else if (refs[ref]) {
	name = create_reference(make_filename(), ref, name, "xlink:");
      } else {
	name = "<tspan style='font-family:courier;'>" + name + "</tspan>";
      }
#endif
      names[ref] = name;
    }

    // Next, sort the references.
    array(string) references = indices(closure);
    array(int) weights = values(closure);
    // Secondary sorting order is according to the reference string.
    sort(references, weights);
    // Primary sorting order is according to the weights.
    sort(weights, references);

    // The approach here is to select the node with the lowest
    // priority, that has all dependencies satisfied.

    mapping(string:int) priorities =
      mkmapping(references, indices(references));

    ADT.Priority_queue pq = ADT.Priority_queue();

    foreach(forward_deps; string ref; multiset(string) deps) {
      if (!sizeof(deps)) {
	pq->push(priorities[ref], ref);
      }
    }

    // NB: No lay->docbody! Neither <pre> nor <svg> may be inside a <dd>.
    //     We simulate the <dd> with a style attribute instead.
#ifdef NO_SVG
    contents->add("<pre style='margin-left:40px;'>\n");
#else
    // FIXME: Why isn't height=100% good enough for Firefox?
    contents->add(sprintf("<svg"
			  " xmlns='http://www.w3.org/2000/svg'"
			  " version='1.1'"
			  " xmlns:xlink='http://www.w3.org/1999/xlink'"
			  " xml:space='preserve'"
			  " style='margin-left:40px;'"
			  " width='100%%' height='%dpx'>\n",
			  (sizeof(references) + sizeof(children))*20 + 5));
#endif

    int pos;
    mapping(string:int) positions = ([]);
    array(string) rev_positions = allocate(sizeof(references));
    array(int) ystart = allocate(sizeof(references));
    while (sizeof(pq)) {
      string ref = pq->pop();
      Node n = refs[ref];
      positions[ref] = pos;
      rev_positions[pos] = ref;
      ystart[pos] = pos * 20 + 25;
      multiset(int) inherit_positions = (<>);
      if (n) {
	foreach(n->inherits, array(string) entry) {
	  inherit_positions[positions[entry[2]||entry[1]]] = 1;
	}
      }

#ifdef NO_SVG
      if (pos) {
	for(int i = 0; i < pos; i++) {
	  if (sizeof(backward_deps[rev_positions[i]])) {
	    contents->add("&nbsp;|&nbsp;&nbsp;");
	  } else {
	    contents->add("&nbsp;&nbsp;&nbsp;&nbsp;");
	  }
	}
	contents->add("\n");
	int got;
	for(int i = 0; i < pos; i++) {
	  if (got) {
	    contents->add("-");
	  } else {
	    contents->add("&nbsp;");
	  }
	  if (inherit_positions[i]) {
	    contents->add("+--");
	    got++;
	    backward_deps[rev_positions[i]][ref] = 0;
	  } else if (got) {
	    contents->add("---");
	  } else if (sizeof(backward_deps[rev_positions[i]])) {
	    contents->add("|&nbsp;&nbsp;");
	  } else {
	    contents->add("&nbsp;&nbsp;&nbsp;");
	  }
	}
      }
      contents->add(names[ref], "\n");
#else
      if (pos) {
	int got;
	for(int i = 0; i < pos; i++) {
	  if (inherit_positions[i]) {
	    if (!got) {
	      // Draw the horizontal line.
	      contents->
		add(sprintf("<line x1='%d' y1='%d' x2='%d' y2='%d' />\n",
			    i*40 + 4, pos*20 + 15,
			    pos*40 - 5, pos*20 + 15));
	    }
	    got++;
	    backward_deps[rev_positions[i]][ref] = 0;
	    if (!sizeof(backward_deps[rev_positions[i]])) {
	      // Draw the inherit line.
	      contents->
		add(sprintf("<line x1='%d' y1='%d' x2='%d' y2='%d' />\n",
			    i*40 + 5, ystart[i],
			    i*40 + 5, pos*20 + 16));
	    }
	  } else if (got && sizeof(backward_deps[rev_positions[i]])) {
	    // We have to jump over the line!
	    contents->
	      add(sprintf("<line x1='%d' y1='%d' x2='%d' y2='%d' />\n",
			  i*40 + 5, ystart[i],
			  i*40 + 5, pos*20 + 12));
	    ystart[i] = pos*20 + 18;
	  }
	}
      }
      contents->add(sprintf("<text x='%d' y='%d'>%s</text>\n",
			    pos*40, pos*20 + 20,
			    names[ref]));
#endif
      pos++;

      foreach(backward_deps[ref] || (<>); string dep_ref;) {
	multiset(string) refs = forward_deps[dep_ref];
	refs[ref] = 0;
	if (!sizeof(refs)) {
	  // All dependencies satisfied.
	  pq->push(priorities[dep_ref], dep_ref);
	}
      }
    }

    // Add the classes that we are a direct parent to.
    if (sizeof(children)) {
      int ppos = sizeof(closure) - 1;
#ifdef NO_SVG
      foreach(sort(children->raw_class_path()), string ref) {
	contents->add("&nbsp;&nbsp;&nbsp;&nbsp;"*ppos,
		      "&nbsp;|&nbsp;&nbsp;&nbsp;\n");
	contents->add("&nbsp;&nbsp;&nbsp;&nbsp;"*ppos,
		      "&nbsp;+--",
		      names[ref], "\n");
      }
#else
      contents->add(sprintf("<line x1='%d' y1='%d' x2='%d' y2='%d' />\n",
			    ppos*40 + 5, ystart[ppos],
			    ppos*40 + 5, (ppos + sizeof(children))*20 + 16));
      foreach(sort(children->raw_class_path()); int i; string ref) {
	contents->add(sprintf("<line x1='%d' y1='%d' x2='%d' y2='%d' />\n",
			      ppos*40 + 4, (ppos + i)*20 + 35,
			      ppos*40 + 35, (ppos + i)*20 + 35,));
	contents->add(sprintf("<text x='%d' y='%d'>%s</text>\n",
			      ppos*40 + 40, (ppos + i)*20 + 40,
			      names[ref]));
      }
#endif
    }

#ifdef NO_SVG
    contents->add("</pre>\n");
#else
    contents->add("</svg>\n");
#endif
  }

  protected string make_content()
  {
    PROFILE();
    string err;
    Parser.XML.Tree.Node n;
    if(err = catch( n = Parser.XML.Tree.parse_input(data)[0] )) {
      werror(err + "\n" + data);
      exit(1);
    }
    ENDPROFILE("XML.Tree");

    resolve_reference = my_resolve_reference;

    if(type=="appendix")
      return parse_appendix(n, 1);

    String.Buffer contents = String.Buffer(100000);
    resolve_class_paths(n);

    if (sizeof(inherits) || sizeof(children)) {
      make_inherit_graph(contents);
    }
    contents->add( parse_children(n, "docgroup", parse_docgroup, 1) );
    contents->add( parse_children(n, "namespace", parse_namespace, 1) );
    contents->add( parse_children(n, "module", parse_module, 1) );
    contents->add( parse_children(n, "class", parse_class, 1) );
    contents->add( parse_children(n, "enum", parse_enum, 1) );

    n->zap_tree();

    return (string)contents;
  }

  string make_link_list(array(Node) children, int|void extra_levels)
  {
    String.Buffer res = String.Buffer(3500);
    foreach(children, Node node)
      res->add("&nbsp;<a href='", make_link(node, extra_levels), "'>",
	       Parser.encode_html_entities(node->name),
	       "()</a><br />\n");
    return (string)res;
  }

  string low_make_index_page(int|void extra_levels)
  {
    resolve_reference = my_resolve_reference;

    string contents = "";

    foreach(({ enum_children, directive_children, method_children }),
	    array(Node) children) {

      if (children && sizeof(children)) {
	if (sizeof(contents)) {
	  contents += "<tr><td colspan='4'><hr /></td></tr>\n";
	}

	contents += "<tr>\n";
	foreach(children/( sizeof(children)/4.0 ), array(Node) children) {
	  contents += "<td nowrap='nowrap' valign='top'>" +
	    make_link_list(children, extra_levels) + "</td>";
	}
	contents += "</tr>\n";
      }
    }

    return contents;
  }

  string make_index_page(int|void extra_levels)
  {
    string contents = low_make_index_page(extra_levels);
    if (sizeof(contents)) {
      contents =
	"<nav><table class='sidebar' style='width:100%;'>\n" +
	contents +
	"</table></nav>";
    }
    return contents;
  }

  void make_html(string template, string path, Git.Export|void exporter)
  {
    if ((type != "method") && (type != "directive")) {
      string index_js = make_index_js();
      string index = make_index_filename() + ".js";
      if (exporter) {
        exporter->filemodify(Git.MODE_FILE, path + "/" + index);
        exporter->data(index_js);
      } else {
        Stdio.mkdirhier(combine_path(path + "/" + index, "../"));
        Stdio.write_file(path + "/" + index, index_js);
      }

      string load_index_js = make_load_index_js();
      string load_index = make_load_index_filename();
      if (exporter) {
        exporter->filemodify(Git.MODE_FILE, path + "/" + load_index);
        exporter->data(load_index_js);
      } else {
        Stdio.mkdirhier(combine_path(path + "/" + load_index, "../"));
        Stdio.write_file(path + "/" + load_index, load_index_js);
      }

      string index_html = make_index_page(1);
      index = make_index_filename() + ".html";

      int num_segments = sizeof(index/"/")-1;
      string extra_prefix = "../"*num_segments;
      string style = low_make_link("style.css", 1);
      string imagedir = low_make_link(low_image_prefix(), 1);

      index_html =
	replace(template,
		(["$navbar$": make_classic_navbar(1),
		  "$contents$": index_html,
		  "$prev_url$": "",
		  "$prev_title$": "",
		  "$next_url$": "",
		  "$next_title$": "",
		  "$type$": String.capitalize("Index of " + type),
		  "$title$": _Roxen.html_encode_string(make_class_path(1)),
		  "$style$": style,
		  "$dotdot$": extra_prefix,
		  "$imagedir$":imagedir,
		  "$filename$": _Roxen.html_encode_string(index),
		  "$extra_headers$": "",
		]));

      if (exporter) {
        exporter->filemodify(Git.MODE_FILE, path + "/" + index);
        exporter->data(index_html);
      } else {
        Stdio.write_file(path + "/" + index, index_html);
      }
    }

    class_children->make_html(template, path, exporter);
    module_children->make_html(template, path, exporter);
    enum_children->make_html(template, path, exporter);
    directive_children->make_html(template, path, exporter);
    method_children->make_html(template, path, exporter);

    int num_segments = sizeof(make_filename()/"/")-1;
    string style = ("../"*num_segments)+"style.css";
    extra_prefix = "../"*num_segments;

    Node prev = find_prev_node();
    Node next = find_next_node();
    string next_url="", next_title="", prev_url="", prev_title="";
    if(next) {
      next_title = next->make_class_path();
      next_url   = make_link(next);
    }
    if(prev) {
      prev_title = prev->make_class_path();
      prev_url   = make_link(prev);
    }

    string extra_headers =
      sprintf("<style type='text/css'>\n"
	      "svg line { stroke:#343434; stroke-width:2; }\n"
	      "svg text { fill:#343434; }\n"
	      "svg a { fill:#0768b2; text-decoration: underline; }\n"
	      "</style>\n") +
      sprintf("<script language='javascript'>\n"
	      "var current = %s;\n"
	      "</script>\n",
	      replace(encode_json(([ "name":name,
				     "link":make_filename()
				  ])),
                      ({ "&", "<", ">" }), ({ "&amp;", "&lt;", "&gt;" }))) +
      sprintf("<script language='javascript' src='%s'>\n"
	      "</script>\n",
	      low_make_link("navigation.js")) +
      sprintf("<script language='javascript' src='%s'>\n"
	      "</script>\n",
	      low_make_link(make_parent_index_filename() + ".js")) +
      sprintf("<script language='javascript'>\n"
	      "siblings = children;\n"
	      "clear_children();\n"
	      "</script>\n") +
      sprintf("<script language='javascript' src='%s'>\n"
	      "</script>\n",
	      low_make_link(make_load_index_filename()));

    string res = replace(template,
      (["$navbar$": make_navbar(),
	"$contents$": make_content(),
	"$prev_url$": prev_url,
	"$prev_title$": _Roxen.html_encode_string(prev_title),
	"$next_url$": next_url,
	"$next_title$": _Roxen.html_encode_string(next_title),
	"$type$": String.capitalize(type),
	"$title$": _Roxen.html_encode_string(make_class_path(1)),
	"$style$": style,
	"$dotdot$": extra_prefix,
	"$imagedir$":image_prefix(),
	"$filename$": _Roxen.html_encode_string(make_filename()),
	"$extra_headers$": extra_headers,
      ]));

    if (exporter) {
      exporter->filemodify(Git.MODE_FILE, path + "/" + make_filename());
      exporter->data(res);
    } else {
      Stdio.mkdirhier(combine_path(path+"/"+make_filename(), "../"));
      Stdio.write_file(path+"/"+make_filename(), res);
    }
  }
}

class TopNode {
  inherit Node;

  constant is_TopNode = 1;
  array(Node) namespace_children = ({ });
  array(Node) appendix_children = ({ });

  string pike_version = version();
  string timestamp;

  string make_index_js()
  {
    string res = ::make_index_js();

#define LOW_MAKE_INDEX_JS(CHILDREN) do {				\
      res += low_make_index_js(#CHILDREN, CHILDREN);			\
    } while (0)

    LOW_MAKE_INDEX_JS(namespace_children);
    LOW_MAKE_INDEX_JS(appendix_children);

    return res;
  }

  void create(string _data) {
    PROFILE();
    mapping m = localtime(time());
    timestamp = sprintf("%4d-%02d-%02d", m->year+1900, m->mon+1, m->mday);
    pending_inherits = ADT.Stack();
    pending_inherits->push(0);	// End sentinel.
    Parser.HTML parser = Parser.HTML();
    parser->case_insensitive_tag(1);
    parser->xml_tag_syntax(3);
    parser->add_container("manual",
			  lambda(Parser.HTML p, mapping args, string c) {
			    if (args->version)
			      pike_version = args->version;
			    if (args["time-stamp"])
			      timestamp = args["time-stamp"];
			    return UNDEFINED;
			  });
    parser->add_container("autodoc",
			  lambda(Parser.HTML p, mapping args, string c) {
			    return ({ c });
			  });

    _data = parser->finish(_data)->read();
    ::create("autodoc", "", _data);
    sort(namespace_children->name, namespace_children);
    sort(appendix_children->name, appendix_children);
    foreach(namespace_children, Node x)
      if(x->type=="namespace" && x->name==default_namespace) {
	//	namespace_children -= ({ x });
	class_children += x->class_children;
	module_children += x->module_children;
	enum_children += x->enum_children;
	directive_children += x->directive_children;
	method_children += x->method_children;
      }

    array(string|Node) inh;
    while (inh = pending_inherits->pop()) {
      [string ref, Node child] = inh;
      Node parent = refs[ref];
      if (parent) {
	parent->children += ({ child });
      }
    }
    type = "autodoc";
    ENDPROFILE("top_create");
  }

  Parser.HTML get_parser() {
    Parser.HTML parser = ::get_parser();
    parser->add_container("appendix", parse_node);
    parser->add_container("namespace", parse_node);
    return parser;
  }

  string make_filename_low() { return "ex"; }
  string make_filename() { return "index.html"; }
  array(Node) get_ancestors() { return ({ }); }
  int(0..0) find_prev_node() { return 0; }
  int(0..0) find_next_node() { return 0; }
  string make_class_path(void|int(0..1) header) {
    if(header && (sizeof(method_children) + sizeof(directive_children))) {
      if(default_namespace)
	return "namespace "+default_namespace;
      else
	return "Namespaces";
    }
    return "";
  }
  string raw_class_path(void|int(0..1) header) {
    return "";
  }

  string make_classic_navbar(int|void extra_levels)
  {
    string res = ::make_classic_navbar(extra_levels);

    res += low_make_classic_navbar(namespace_children, "Namespaces",
				   extra_levels);
    res += low_make_classic_navbar(appendix_children, "Appendices",
				   extra_levels);

    return res;
  }

  string make_navbar()
  {
    return make_classic_navbar();
  }

  string low_make_index_page(int|void extra_levels)
  {
    string contents = ::low_make_index_page(extra_levels);
    string doc = parse_children(Parser.XML.Tree.parse_input(data),
				"docgroup", parse_docgroup, 1);

    if (sizeof(doc)) {
      doc = "<tr><td colspan='4' nowrap='nowrap'>" + doc + "</td></tr>\n";
      if (sizeof(contents)) {
	contents += "<tr><td colspan='4'><hr /></td></tr>\n";
      }
    }
    return contents + doc;
  }

  string make_content() {
    resolve_reference = my_resolve_reference;

    return make_index_page();
  }

  void make_html(string template, string path, Git.Export|void exporter) {
    PROFILE();
    if (exporter) {
      exporter->filemodify(Git.MODE_FILE, path + "/navigation.js");
      exporter->data(navigation_js);
    } else {
      Stdio.mkdirhier(path);
      Stdio.write_file(path + "/navigation.js", navigation_js);
    }
    appendix_children->make_html(template, path, exporter);
    namespace_children->make_html(template, path, exporter);
    ::make_html(template, path, exporter);
    ENDPROFILE("top_make_html");
  }
}

int low_main(string doc_file, string template_file, string outdir,
	     Git.Export|void exporter)
{
  PROFILE();
  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Reading refdoc blob %s...\n", doc_file);
  string doc = Stdio.read_file(doc_file);
  if(!doc) {
    werror("Failed to load refdoc blob %s.\n", doc_file);
    return 1;
  }

  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Reading template file %s...\n", template_file);
  string template = Stdio.read_file(template_file);
  if(!template) {
    werror("Failed to load template %s.\n", template_file);
    return 1;
  }

  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Splitting to destination directory %s...\n", outdir);

  TopNode top = TopNode(doc);
  if (flags & Tools.AutoDoc.FLAG_NO_DYNAMIC) {
    // Attempt to keep down the number of changed files by
    // using a client-sice include for the version and date.
    string pike_version_js =
      sprintf("document.write(%q);\n", top->pike_version);
    string timestamp_js =
      sprintf("document.write(%q);\n",
	      sprintf("<time pubdate=''>%s</time>", top->timestamp));
    if (exporter) {
      exporter->filemodify(Git.MODE_FILE, outdir + "/pike_version.js");
      exporter->data(pike_version_js);
      exporter->filemodify(Git.MODE_FILE, outdir + "/timestamp.js");
      exporter->data(timestamp_js);
    } else {
      Stdio.mkdirhier(outdir);
      Stdio.write_file(outdir + "/pike_version.js", pike_version_js);
      Stdio.write_file(outdir + "/timestamp.js", timestamp_js);
    }

    // Reduce updating of all files to once a year and when the language
    // changes name.
    string product_name = "Pike";
    sscanf(top->pike_version, "%[^0-9 _]", product_name);
    int year = localtime(time())->year + 1900;
    sscanf(top->timestamp, "%d-", year);

    template = replace(template,
		       ([ "$version$":"<script type='text/javascript' src='$dotdot$pike_version.js' ></script><noscript>" + product_name + "</noscript>",
			  "$date$":"<script type='text/javascript' src='$dotdot$timestamp.js' ></script><noscript>" + year + "</noscript>",
		       ]) );
  } else {
    template = replace(template,
		       ([ "$version$":top->pike_version,
			  "$date$":top->timestamp,
		       ]) );
  }

  if (flags & Tools.AutoDoc.FLAG_COMPAT) {
    // Fix markup bugs affecting the images path.

    // The canonic Pike site has moved from
    // pike.roxen.com to pike.ida.liu.se to pike.lysator.liu.se.
    template = replace(template,
		       ({ "pike.roxen.com", "pike.ida.liu.se" }),
		       ({ "pike.lysator.liu.se", "pike.lysator.liu.se" }));

    // These first two affect commits prior to and including 7.3.11.
    template = replace(template,
		       "<img src=\"images/",
		       "<img src=\"$imagedir$");
    template = replace(template,
		       "<td background=\"images/",
		       "<td background=\"$imagedir$");

    // This one however affects quite a few more.
    template = replace(template,
		       ({ "$dotdot$/images/" }),
		       ({ "$imagedir$" }));

    if (!has_value(template, "$extra_headers$")) {
      template = replace(template,
			 ({ "<head>" }),
			 ({ "<head>\n"
			    "$extra_headers$" }));
    }
  }

  top->make_html(template, outdir, exporter);
  ENDPROFILE("main");

  if (verbosity >= Tools.AutoDoc.FLAG_VERBOSE)
    foreach(sort(indices(profiling)), string f)
      werror("%s: %.1f\n", f, profiling[f]/1000000.0);
  if (verbosity >= Tools.AutoDoc.FLAG_NORMAL) {
    werror("%d unresolved references.\n", unresolved);
    werror("%d documented constants/variables/functions/classes/modules.\n",
	   sizeof(refs));
  }
  return 0;
}

int main(int argc, array(string) argv)
{
  foreach(Getopt.find_all_options(argv, ({
    ({ "verbose", Getopt.NO_ARG,  "-v,--verbose"/"," }),
    ({ "quiet",   Getopt.NO_ARG,  "-q,--quiet"/"," }),
    ({ "help",    Getopt.NO_ARG,  "--help"/"," }),
    ({ "compat",  Getopt.NO_ARG,  "--compat"/"," }),
  })), array opt)
    switch(opt[0]) {
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
      write("pike -x autodoc_to_split_html [-v|--verbose] [-q|--quiet]\n"
	    "\t[--help] <input-file> <template-file> <output-dir>"
	    " [<namespace>]\n");
      return 0;
    case "compat":
      flags |= Tools.AutoDoc.FLAG_COMPAT;
      break;
    }
  argv = Getopt.get_args(argv)[1..];
  if(sizeof(argv)<4) {
    werror("Too few arguments.\n"
	   "pike -x autodoc_to_split_html [-v|--verbose] [-q|--quiet]\n"
	   "\t[--help] <input-file> <template-file> <output-dir>"
	   " [<namespace>]\n");
    return 1;
  }
  if(sizeof(argv)>3) default_namespace=argv[3];

  return low_main(argv[0], argv[1], argv[2]);
}
