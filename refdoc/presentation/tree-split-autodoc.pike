/*
 * $Id: tree-split-autodoc.pike,v 1.1 2001/07/20 00:04:46 js Exp $
 *
 */

inherit "make_html.pike";

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

    sort(class_children->name, class_children);
    sort(module_children->name, module_children);
    sort(method_children->name, method_children);

    werror("name: %O    type: %O\n", name,type);
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

  string parse_docgroup(Parser.HTML p, mapping m, string c)
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

    parser->add_container("docgroup", parse_docgroup);
    parser->add_container("module", parse_node("module"));
    parser->add_container("class",  parse_node("class"));
    
    parser->feed(in);
    parser->finish();
    return parser->read();
  }

  string make_faked_wrapper(string s)
  {
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
    string fn = cquote(lower_case(name));
    if(parent)
      return parent->make_filename_low()+"."+fn;
    else
      return fn;
  }


  
  string make_filename()
  {
    return replace(make_filename_low()+".html",
		   "top..", "");
  }

  string make_navbar_low(Node root)
  { 
    string res="";
    if(parent)
      res += sprintf("<a href='%s'>Up</a><br />\n", parent->make_filename());
    else
      return "";
    res+="<table border='0' cellpadding='1' cellspacing='0' class='sidebar'>";

    foreach( ({ root->module_children,
		root->class_children,
		root->method_children }),
	     array children)
    {
      if(sizeof(children))
	switch(children[0]->type)
	{
  	  case "module": 
  	    res += "<tr><td>Modules:</b></td></tr>";
  	    break;
  	  case "class": 
  	    res += "<tr><td>Classes:</b></td></tr>";
  	    break;
  	  case "method": 
  	    res += "<tr><td>Methods:</b></td></tr>";
  	    break;
	}
      foreach(children, Node node)
      {
	if(node==this_object())
	  res += sprintf("<tr><td>· <b>%s</b></td></tr>",node->name);
	else
	  res += sprintf("<tr><td>· <a href='%s'>%s</a></td></tr>",
			 node->make_filename(), node->name);
      }
    }
    
    return res+"</table>";
  }
  
  string make_navbar()
  {
    if(type=="method")
      return make_navbar_low(parent);
    else
      return make_navbar_low(this_object());
  }
  
  void make_html()
  {
    class_children->make_html();
    module_children->make_html();
    method_children->make_html();
    if(!name || !type)
      return;

    Stdio.mkdirhier("manual-split/");

    string xmldata = make_faked_wrapper(data);
    Parser.XML.Tree.Node n = Parser.XML.Tree.parse_input(xmldata)[0];

    werror("Layouting...%s\n", name);
    string res =
      "<html><head><title>Pike manual ?.?.?</title>"
      "<link rel='stylesheet' href='style.css' /></head>\n"
      "<body topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' " 
      "bgcolor='#ffffff' text='#000000' link='#000099' alink='#0000ff' "
      "vlink='#000099'><body bgcolor='white' text='black'>\n";

    res+="<table><tr><td valign='top' bgcolor='#f0f0f0'>"+make_navbar()+"</td><td valign='top'>";

    res += parse_children(n, "docgroup", parse_docgroup);
    res += parse_children(n, "module", parse_module);
    res += parse_children(n, "class", parse_class);

    res += "</td></tr></table></body></html>\n";
    
    Stdio.write_file("manual-split/"+make_filename(),
		     res);
  }
  
}


int main(int argc, array(string) argv)
{
  werror("Reading refdoc blob %s...\n", argv[1]);
  string doc = Stdio.read_file(argv[1]);
  werror("Reading structure file %s...\n", argv[2]);
  string struct = Stdio.read_file(argv[2]);
  werror("Splitting to destination directory %s...\n", argv[3]);

  Node top = Node("module","top", doc);
  top->make_html();
}
