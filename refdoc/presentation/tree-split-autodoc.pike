/*
 * $Id: tree-split-autodoc.pike,v 1.3 2001/07/27 03:20:45 nilsson Exp $
 *
 */

inherit "make_html.pike";

mapping (string:Node) refs = ([ ]);

string extra_prefix = "";
string image_prefix()
{
  return extra_prefix + ::image_prefix();
}


class Node
{
  string type;
  string name;
  string data;
  array(Node) class_children  = ({ });
  array(Node) module_children = ({ });
  array(Node) method_children = ({ });

  Node parent;

  void create(string _type, string _name, string _data, void|Node _parent)
  {
    type = _type;
    name = _name;
    parent = _parent;
    data = parse( _data );

    refs[make_class_path()] = this_object();
    refs[replace(make_class_path(), "()->", "->")] = this_object();

    sort(class_children->name, class_children);
    sort(module_children->name, module_children);
    sort(method_children->name, method_children);
  }

  static function parse_node(string type)
  {
    return lambda(Parser.HTML p, mapping m, string c)
	   {
	     if(m->name)
	       this_object()[type+"_children"] +=
	     ({ Node( type, m->name, c, this_object() ) });
	     return "";
	   };
  }

  string my_parse_docgroup(Parser.HTML p, mapping m, string c)
  {
    if(m["homogen-name"] && m["homogen-type"] && m["homogen-type"]=="method")
    {
      method_children +=
      ({ Node( "method", m["homogen-name"], c, this_object() ) });
      return "";
    }
    else
      return 0;
  }

  static string parse(string in)
  {
    Parser.HTML parser = Parser.HTML();
    
    parser->case_insensitive_tag(1);
    parser->xml_tag_syntax(3);

    parser->add_container("docgroup", my_parse_docgroup);
    parser->add_container("module", parse_node("module"));
    parser->add_container("class",  parse_node("class"));
    
    parser->feed(in);
    parser->finish();
    return parser->read();
  }

  string make_faked_wrapper(string s)
  {
    if(name=="top")
      return s;
    if(type=="method")
      s = sprintf("<docgroup homogen-type='method' homogen-name='%s'>\n"
		  "%s\n</docgroup>\n",
		   name, s);
    else
      s = sprintf("<%s name='%s'>\n%s\n</%s>\n",
		  type, name, s, type);
    if(parent)
      return parent->make_faked_wrapper(s);
    else
      return s;
  }

  string cquote(string n)
  {
    string ret="";
    while(sscanf((string)n,"%[a-zA-Z0-9]%c%s",string safe, int c, n)==3)
    {
      switch(c)
      {
        default:  ret+=sprintf("%s_%02X",safe,c); break;
        case '+': ret+=sprintf("%s_add",safe); break;
        case '`': ret+=sprintf("%s_backtick",safe);  break;
        case '_': ret+=sprintf("%s_",safe); break;
        case '=': ret+=sprintf("%s_eq",safe); break;
      }
    }
    ret+=n;
    return ret;
  }

  
  string make_filename_low()
  {
    string fn = cquote(name);
    if(fn=="")
      fn=="__index";
    if(parent)
      return parent->make_filename_low()+"/"+fn;
    else
      return fn;
  }

  string make_filename()
  {
    string s=make_filename_low()[5..]+".html";
    if(s==".html")
      return "index.html";
    else
      return s;
  }

  string make_link(Node to)
  {
    // FIXME: Optimize the length of relative links
    int num_segments = sizeof(make_filename()/"/") - 1;
    return ("../"*num_segments)+to->make_filename();
  }

  array(Node) get_ancestors()
  {
    if(!parent)
      return ({ });
    else
      return ({ this_object() }) + parent->get_ancestors();
  }

  string my_resolve_reference(string _reference)
  {
    foreach( ({ _reference,
		Parser.parse_html_entities(_reference) }),
	     string reference)
    {
      if(refs[reference])
	return make_link( refs[reference] );

      if(refs[reference])
	return make_link( refs[reference] );
      
      foreach(find_siblings(), Node sibling)
	if(sibling->name==reference ||
	   sibling->name+"()"==reference)
	  return make_link( sibling );
    }
      
    werror("Missed reference: %O\n", _reference);
    return 0;
  }

  string make_class_path()
  {
    array a;
    if(parent)
      a = reverse(parent->get_ancestors());
    else
      return "";

    string res = "";
    foreach(a[1..], Node n)
    {
      res += n->name;
      if(n->type=="class")
	res += "()->";
      else if(n->type=="module")
	res += ".";
    }
    res += name;

    if(type=="method")
      res +="()";

    return res;
  }

  string make_navbar_really_low(array(Node) children, void|int notables)
  {
    string a="<tr><td>", b="</td></tr>";
    if(notables)
    {
      a=""; b="<br />";
    }
    string res = "";
    foreach(children, Node node)
    {
      string extension = "";
      string my_name=node->name;
      if(node->type=="method")
	extension="()";
      else
	my_name="<b>"+my_name+"</b>";
	  
      if(node==this_object())
	res += sprintf("%s· %s%s%s",
		       a, my_name, extension, b);
      else
	res += sprintf("%s· <a href='%s'>%s%s</a>%s",
		       a, make_link(node), my_name, extension, b);
    }
    return res;
  }

  string make_hier_list(Node node)
  {
    string res="";

    if(node)
    {
      res += make_hier_list(node->parent);

      string my_class_path = (node->name!="")?node->make_class_path():"[Top]";
      
      if(node == this_object())
	res += sprintf("<b>%s</b><br />\n", my_class_path);
      else
	res += sprintf("<a href='%s'><b>%s</b></a><br />\n",
		       make_link(node), my_class_path);
    }
    return res;
  }
  
  string make_navbar_low(Node root)
  { 
    string res="";

    res += make_hier_list(root);

    res+="<table border='0' cellpadding='1' cellspacing='0' class='sidebar'>";

    array(array(Node)) children_groups;

    if(root->name=="")
      children_groups = ({ root->module_children,
			   root->class_children,  });
    else
      children_groups = ({ root->module_children,
			   root->class_children,
			   root->method_children,  });
    
    foreach( children_groups, array children)
      res += make_navbar_really_low(children);
    
    return res+"</table>";
  }
  
  string make_navbar()
  {
    if(type=="method")
      return make_navbar_low(parent);
    else
      return make_navbar_low(this_object());
  }

  array(Node) find_siblings()
  {
    return
      parent->class_children+
      parent->module_children+
      parent->method_children;
  }

  array(Node) find_children()
  {
    return
      class_children+
      module_children+
      method_children;
  }

  Node find_prev_node()
  {
    if(!parent)
      return 0;

    array(Node) siblings = find_siblings();
    int index = search( siblings, this_object() );

    Node tmp;

    if(index==0)
      return parent;
    
    tmp = siblings[index-1];

    while(sizeof(tmp->find_children()))
      tmp = tmp->find_children()[-1];
    
    return tmp;
  }

  Node find_next_node(void|int dont_descend)
  {
    if(!parent)
      return 0;

    if(!dont_descend && sizeof(find_children()))
      return find_children()[0];

    array(Node) siblings = find_siblings();
    int index = search( siblings, this_object() );

    if(index==sizeof(siblings)-1)
      return parent->find_next_node(1);
    else
      return siblings[index+1];
  }

  void make_html(string template, string path)
  {
    werror("Layouting...%s\n", make_class_path());
    
    class_children->make_html(template, path);
    module_children->make_html(template, path);
    method_children->make_html(template, path);
    if(!name || !type || name == "top")
      return;

    string res;
    
    int num_segments = sizeof(make_filename()/"/")-1;
    string style = ("../"*num_segments)+"style.css";
    
    extra_prefix = "../"*num_segments;
    string contents;
    
    if(name=="")
    {
      contents = "<h1>Top level methods</h1><table class='sidebar'><tr>";
      foreach(method_children/( sizeof(method_children)/4.0 ), array(Node) children)
      {
	contents += "<td>";
	contents += make_navbar_really_low(children, 1);
	contents += "</td>"; 
      }
      contents += "</tr></table>";
    }
    else
    {
      string xmldata = make_faked_wrapper(data);
      Parser.XML.Tree.Node n = Parser.XML.Tree.parse_input(xmldata);

      resolve_reference = my_resolve_reference;
      contents = parse_children(n, "docgroup", parse_docgroup, 1);
      contents += parse_children(n, "module", parse_module, 1);
      contents += parse_children(n, "class", parse_class, 1);
    }

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

    res = replace(template,
		  (["$navbar$": make_navbar(),
		    "$contents$": contents,
		    "$prev_url$": prev_url,
		    "$prev_title$": prev_title,
		    "$next_url$": next_url,
		    "$next_title$": next_title,
		    "$title$": make_class_path(),
		    "$style$": style,
		  ]));

    Stdio.mkdirhier(combine_path(path+"/"+make_filename(), "../"));
    Stdio.write_file(path+"/"+make_filename(), res);
  }
}


int main(int argc, array(string) argv)
{
  if(argc!=4) {
    werror("Too few arguments. (in-file, template, out-dir)\n");
    return 1;
  }

  werror("Reading refdoc blob %s...\n", argv[1]);
  string doc = Stdio.read_file(argv[1]);
  if(!doc) {
    werror("Failed to load refdoc blob %s.\n", argv[1]);
    return 1;
  }

  werror("Reading template file %s...\n", argv[2]);
  string template = Stdio.read_file(argv[2]);
  if(!template) {
    werror("Failed to load template %s.\n", argv[2]);
    return 1;
  }
  mapping m = localtime(time());
  template = replace(template, ([ "$ver$":version(),
				  "$when$":sprintf("%4d-%02d-%02d", m->year+1900, m->mon+1, m->mday),
  ]) );

  werror("Splitting to destination directory %s...\n", argv[3]);
  Node top = Node("module","top", doc);
  top->make_html(template, argv[3]);
  return 0;
}
