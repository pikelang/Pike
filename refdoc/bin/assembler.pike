#define Node Parser.XML.Tree.Node
#define XML_ELEMENT Parser.XML.Tree.XML_ELEMENT
#define XML_TEXT Parser.XML.Tree.XML_TEXT

int chapter;
int appendix;

mapping queue = ([]);
mapping appendix_queue = ([]);

Node void_node = Node(XML_ELEMENT, "void", ([]), 0);

// array( array(name,file,(subchapters)) )
array toc = ({});
class TocNode {
  inherit Node;
  string path;
  int(1..3) depth;

  int|void walk_preorder_2(mixed ... args) {
    mChildren = ({});
    int chapter;
    foreach(toc, array ent) {
      string file = ent[1][sizeof(String.common_prefix( ({ path, ent[1] }) ))..];
      if(file[0]=='/') file = file[1..];

      Node dt = Node( XML_ELEMENT, "dt", ([]), 0 );
      Node link = Node( XML_ELEMENT, "url", ([ "href" : file ]), 0 );
      link->add_child( Node( XML_TEXT, 0, 0, (++chapter)+". "+ent[0] ) );
      dt->add_child( link );
      add_child( dt );
      add_child( Node( XML_TEXT, 0, 0, "\n" ) );

      int sub;
      if(sizeof(ent)>2 && depth>1)
	foreach(ent[2..], string subtit) {
	  Node dd = Node( XML_ELEMENT, "dd", ([]), 0 );
	  Node link = Node( XML_ELEMENT, "url", ([ "href" : file ]), 0 );
	  link->add_child( Node( XML_TEXT, 0, 0, chapter+"."+(++sub)+". "+subtit ) );
	  dd->add_child( link );
	  add_child( dd );
	  add_child( Node( XML_TEXT, 0, 0, "\n" ) );

	  // TODO: Add depth 3 here
	}
    }
    ::walk_preorder_2( @args );
  }

  int count_children() { return sizeof(toc)*3; }

  void create(string _path, int(1..3) _depth) {
    path = _path;
    depth = _depth;
    ::create(XML_ELEMENT, "dl", ([]), "");
  }
}

class Entry (Node target) {
  constant type = "";
  mapping args;
  string _sprintf() {
    return sprintf("%sTarget( %O )", type, target);
  }
}


// --- Some debug functions

string visualize(Node n, int depth) {
  if(n->get_node_type() == XML_TEXT)
    return Parser.XML.Tree.text_quote(n->get_text());
  if(n->get_node_type() != XML_ELEMENT ||
     !strlen(n->get_tag_name()))
    return "";

  string name = n->get_tag_name();
  if(name!="module" && name!="class")
    return "";

  string data = "<" + name;
  if (mapping attr = n->get_attributes()) {
    foreach(indices(attr), string a)
      data += " " + a + "='"
	+ Parser.XML.Tree.attribute_quote(attr[a]) + "'";
  }
  if (!n->count_children())
    return data + "/>";

  data += ">";
  if(depth==0)
    data += "...";
  else
    data += map(n->get_children(), visualize, depth-1)*"";
  return data + "</" + name + ">";
}

string vis(Node n) {
  if(!n || n->get_node_type()!=XML_ELEMENT) return sprintf("%O", n);
  return sprintf("<%s%{ %s='%s'%}>", n->get_tag_name(), (array)n->get_attributes());
}

// ---


class mvEntry {
  inherit Entry;
  constant type = "mv";

  void `()(Node data) {
    if(args) {
      mapping m = data->get_attributes();
      foreach(indices(args), string index)
	m[index] = args[index];
    }
    target->replace_node(data);
  }
}

class cpEntry {
  inherit Entry;
  constant type = "cp";

  void `()(Node data) {
    // clone data subtree
    // target->replace_node(clone);
  }
}

void enqueue_move(string source, Node target) {
  mapping bucket = queue;
  array path = map(source/".", replace, "-", ".");
  if(source != "")
    foreach(path, string node) {
      if(!bucket[node])
	bucket[node] = ([]);
      bucket = bucket[node];
    }

  if(bucket[0])
    error("Move source already allocated (%s).\n", source);

  bucket[0] = mvEntry(target);
}

Node parse_file(string fn) {
  Node n;
  mixed err = catch {
    n = Parser.XML.Tree.parse_file(fn);
  };
  if(stringp(err)) error(err);
  if(err) throw(err);
  return n;
}

void section_ref_expansion(Node n) {
  int subsection;
  foreach(n->get_elements(), Node c)
    switch(c->get_tag_name()) {

    case "p":
    case "example":
    case "dl":
    case "ul":
    case "matrix":
      break;

    case "subsection":
      c->get_attributes()->number = (string)++subsection;
      break;

    case "insert-move":
      enqueue_move(c->get_attributes()->entity, c);
      break;

    default:
      error("Unknown element %O in section element.\n", c->get_tag_name());
      break;
    }
}

void chapter_ref_expansion(Node n, string dir) {
  int section;
  foreach(n->get_elements(), Node c)
    switch(c->get_tag_name()) {

    case "p":
      break;

    case "contents":
      c->replace_node( TocNode(dir, 3) );
      break;

    case "section":
      c->get_attributes()->number = (string)++section;
      toc[chapter] += ({ c->get_attributes()->title });
      section_ref_expansion(c);
      break;

    case "insert-move":
      enqueue_move(c->get_attributes()->entity, c);
      break;

    default:
      error("Unknown element %O in chapter element.\n", c->get_tag_name());
      break;
    }
}

int filec;
void ref_expansion(Node n, string dir, void|string file) {
  foreach(n->get_elements(), Node c) {
    switch(c->get_tag_name()) {

    case "file":
      if(file)
	error("Nested file elements (%O).\n", file);
      file = c->get_attributes()->name;
      if(!file) {
	if(sizeof(c->get_elements())==1) {
	  string name = c->get_elements()[0]->get_tag_name();
	  if(name == "chapter" || name == "chapter-ref")
	    file = "chapter_" + (1+chapter);
	  else if(name == "appendix" || name == "appendix-ref")
	    file = "appendix_" + (string)({ 65+appendix });
	  else
	    file = "file_" + (++filec);
	}
	else
	  file = "file_" + (++filec);
	file += ".html";
      }
      c->get_attributes()->name = dir + "/" + file;
      ref_expansion(c, dir, c->get_attributes()->name);
      file = 0;
      break;

    case "dir":
      c->get_attributes()->name = dir + "/" + c->get_attributes()->name;
      ref_expansion(c, c->get_attributes()->name, file);
      break;

    case "chapter-ref":
      if(!file)
	error("chapter-ref element outside file element\n");
      if(!c->get_attributes()->file)
	error("No file attribute on chapter-ref element.\n");
      c = c->replace_node( parse_file(c->get_attributes()->file)->
			   get_first_element("chapter") );
      // fallthrough
    case "chapter":
      if(c->get_attributes()->unnumbered!="1")
	c->get_attributes()->number = (string)++chapter;
      toc += ({ ({ c->get_attributes()->title, file }) });
      chapter_ref_expansion(c, dir);
      break;

    case "appendix-ref":
      if(!file)
	error("appendix-ref element outside file element\n");
      if(c->get_attributes()->name) {
	Entry e = mvEntry(c);
	e->args = ([ "number": (string)++appendix ]);
	appendix_queue[c->get_attributes()->name] = e;
	break;
      }
      if(!c->get_attributes()->file)
	error("Neither file nor name attribute on appendix-ref element.\n");
      c = c->replace_node( parse_file(c->get_attributes()->file)->
			   get_first_element("appendix") );
      // fallthrough
    case "appendix":
      c->get_attributes()->number = (string)++appendix;
      toc += ({ ({ c->get_attributes()->name, file }) });
      break;

    case "void":
      c->get_parent()->remove_child(c);
      void_node->add_child(c);
      chapter_ref_expansion(c, dir);
    }
  }
}

void move_appendices(Node n) {
  foreach(n->get_elements("appendix"), Node c) {
    string name = c->get_attributes()->name;
    Node a = appendix_queue[name];
    if(a) {
      a(c);
      m_delete(appendix_queue, name);
    }
    else {
      c->remove_node();
      werror("Removed untargeted appendix %O.\n", name);
    }
  }
  if(sizeof(appendix_queue))
    werror("Failed to find appendi%s %s.\n", (sizeof(appendix_queue)==1?"x":"ces"),
	   String.implode_nicely(map(indices(appendix_queue),
				     lambda(string in) {
				       return "\""+in+"\"";
				     })) );
}

Node wrap(Node n, Node wrapper) {
  if(wrapper->count_children())
    wrap(n, wrapper[0]);
  else {
    Node p = n->get_parent();
    if(p) p->remove_child(n);
    wrapper->add_child(n);
  }
  return wrapper;
}

void move_items(Node n, mapping jobs, void|Node wrapper) {
  if(jobs[0]) {
    if(wrapper)
      jobs[0]( wrap(n, wrapper->clone()) );
    else
      jobs[0](n);
    m_delete(jobs, 0);
  }

  foreach( ({ "module", "class", "docgroup" }), string type)
    foreach(n->get_elements(type), Node c) {
      mapping m = c->get_attributes();
      string name = m->name || m["homogen-name"];
      mapping|Entry e = jobs[ name ];
      if(!e) continue;
      if(mappingp(e)) {
	Node wr = Node(XML_ELEMENT, n->get_tag_name(),
		       n->get_attributes()+(["hidden":"1"]), 0);
	if(wrapper)
	  wr = wrap( wr, wrapper->clone() );

	move_items(c, e, wr);

	if(!sizeof(e))
	  m_delete(jobs, name);
      }
    }
}

void report_failed_entries(mapping scope, string path) {
  if(scope[0]) {
    werror("Failed to move %s\n", path[1..]);
    m_delete(scope, 0);
  }
  foreach(scope; string id; mapping next)
    report_failed_entries(next, path + "." + id);
}

int(0..1) main(int num, array(string) args) {

  int T = time();
  if(num<3)
    error("To few arguments\n");

  werror("Parsing structure file %O.\n", args[1]);
  Node n = parse_file(args[1]);
  n = n->get_first_element("manual");
  n->get_attributes()->version = version();
  mapping t = localtime(time());
  n->get_attributes()["time-stamp"] =
    sprintf("%4d-%02d-%02d", t->year+1900, t->mon+1, t->mday);
  werror("Executing reference expansion and queueing node insertions.\n");
  mixed err = catch {
    ref_expansion(n, ".");
  };
  if (err) {
    werror("ref_expansion() failed:\n"
	   "  ch:%d app:%d toc:%O\n",
	   chapter, appendix, toc);
    throw(err);
  }

  werror("Parsing autodoc file %O.\n", args[2]);
  Node m = parse_file(args[2]);
  m = m->get_first_element("module");

  werror("Moving appendices.\n");
  move_appendices(m);

  werror("Executing node insertions.\n");
  move_items(m, queue);
  if(sizeof(queue)) {
    report_failed_entries(queue, "");
    return 1;
  }

  werror("Writing final manual source file.\n");
  write( (string)n );
  werror("Took %d seconds.\n\n", time()-T);
  return 0;
}
