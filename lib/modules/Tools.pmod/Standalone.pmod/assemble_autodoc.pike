// $Id: assemble_autodoc.pike,v 1.31 2004/07/08 17:48:48 grubba Exp $

#pike __REAL_VERSION__

constant description = "Assembles AutoDoc output file.";

// AutoDoc mk II assembler

#define Node Parser.XML.Tree.SimpleNode
#define XML_ELEMENT Parser.XML.Tree.XML_ELEMENT
#define XML_TEXT Parser.XML.Tree.XML_TEXT

int chapter;

mapping queue = ([]);
mapping ns_queue = ([]);
array(Node) chapters = ({});

Node void_node = Node(XML_ELEMENT, "void", ([]), 0);

// array( array(name,file,chapter_no,(subchapters)) )
array toc = ({});
class TocNode {
  inherit Node;
  string path;
  int(1..3) depth;

  static void make_children() {
    if(sizeof(mChildren)) return;
    foreach(toc, array ent) {
      string file = ent[1][sizeof(String.common_prefix( ({ path, ent[1] }) ))..];
      if(file[0]=='/') file = file[1..];

      Node dt = Node( XML_ELEMENT, "dt", ([]), 0 );
      Node link = Node( XML_ELEMENT, "url", ([ "href" : file ]), 0 );
      if(ent[2])
	link->add_child( Node( XML_TEXT, 0, 0, ent[2]+". "+ent[0] ) );
      else
	link->add_child( Node( XML_TEXT, 0, 0, ent[0] ) );
      dt->add_child( link );
      add_child( dt );
      add_child( Node( XML_TEXT, 0, 0, "\n" ) );

      int sub;
      if(sizeof(ent)>3 && depth>1)
	foreach(ent[3..], string subtit) {
	  Node dd = Node( XML_ELEMENT, "dd", ([]), 0 );
	  sub++;
	  Node link = Node( XML_ELEMENT, "url",
			    ([ "href" : file+"#"+sub ]), 0 );
	  link->add_child( Node( XML_TEXT, 0, 0, ent[2]+"."+sub+". "+subtit ) );
	  dd->add_child( link );
	  add_child( dd );
	  add_child( Node( XML_TEXT, 0, 0, "\n" ) );

	  // TODO: Add depth 3 here
	}
    }
  }

  int walk_preorder_2(mixed ... args) {
    make_children();
    ::walk_preorder_2( @args );
  }

  array(Node) get_children() {
    make_children();
    return ::get_children();
  }

  int count_children() {
    make_children();
    return ::count_children();
  }

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
     !sizeof(n->get_tag_name()))
    return "";

  string name = n->get_tag_name();
  if(!(<"autodoc","namespace","module","class">)[name])
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

  static Node parent;

  static void create(Node target, Node parent)
  {
    ::create(target);
    mvEntry::parent = parent;
  }

  void `()(Node data) {
    if(args) {
      mapping m = data->get_attributes();
      foreach(indices(args), string index)
	m[index] = args[index];
    }
    parent->replace_child(target, data);
  }
}

class mvPeelEntry {
  inherit Entry;
  constant type = "mvPeel";

  static Node parent;

  static void create(Node target, Node parent)
  {
    ::create(target);
    mvPeelEntry::parent = parent;
  }

  void `()(Node data) {
    // WARNING! Disrespecting information hiding!
    int pos = search(parent->mChildren, data);
    array pre = parent->mChildren[..pos-1];
    array post = parent->mChildren[pos+1..];
    parent->mChildren = pre + data->mChildren + post;
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

void enqueue_move(Node target, Node parent) {

  mapping(string:string) m = target->get_attributes();
  if(m->namespace) {
    string ns = m->namespace;
    sscanf(ns, "%s::", ns);
    ns += "::";
    if(ns_queue[ns])
      error("Move source already allocated (%O).\n", ns);
    if(m->peel="yes")
      ns_queue[ns] = mvPeelEntry(target, parent);
    else
      ns_queue[ns] = mvEntry(target, parent);
    return;
  }
  else if(!m->entity)
    error("Error in insert-move element.\n");

  mapping bucket = queue;

  if(m->entity != "") {
    array path = map(m->entity/".", replace, "-", ".");
    if (!has_value(path[0], "::")) {
      // Default namespace.
      path = ({ "predef::" }) + path;
    } else {
      if (!has_suffix(path[0], "::")) {
	path = path[0]/"::" + path[1..];
      }
    }
    foreach(path, string node) {
      if(!bucket[node])
	bucket[node] = ([]);
      bucket = bucket[node];
    }
  }

  if(bucket[0])
    error("Move source already allocated (%s).\n", m->entity);

  bucket[0] = mvEntry(target, parent);
}

Node parse_file(string fn) {
  Node n;
  mixed err = catch {
    n = Parser.XML.Tree.simple_parse_file(fn);
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
      enqueue_move(c, n);
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
      n->replace_child(c, TocNode(dir, 3) );
      break;

    case "section":
      c->get_attributes()->number = (string)++section;
      section_ref_expansion(c);
      break;

    case "insert-move":
      enqueue_move(c, n);
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
      n->replace_child(c, c = parse_file(c->get_attributes()->file)->
		       get_first_element("chapter") );
      // fallthrough
    case "chapter":
      mapping m = c->get_attributes();
      if(m->unnumbered!="1")
	m->number = (string)++chapter;
      toc += ({ ({ m->title, file, m->number }) });
      chapters += ({ c });
      chapter_ref_expansion(c, dir);
      break;

    case "void":
      n->remove_child(c);
      void_node->add_child(c);
      chapter_ref_expansion(c, dir);
      break;

    default:
      werror("Warning unhandled tag %O in ref_expansion\n",
	     c->get_tag_name());
      break;
    }
  }
}

Node wrap(Node n, Node wrapper) {
  if(wrapper->count_children())
    wrap(n, wrapper[0]);
  else {
    wrapper->add_child(n);
  }
  return wrapper;
}

static void move_items_low(Node n, mapping jobs, void|Node wrapper) {
  if(jobs[0]) {
    if(wrapper)
      jobs[0]( wrap(n, wrapper->clone()) );
    else
      jobs[0](n);
    m_delete(jobs, 0);
  }
  if(!sizeof(jobs)) return;

  foreach( ({ "module", "class", "docgroup" }), string type)
    foreach(n->get_elements(type), Node c) {
      mapping m = c->get_attributes();
      string name = m->name || m["homogen-name"];
      mapping e = jobs[ name ];
      if(!e) continue;

      Node wr = Node(XML_ELEMENT, n->get_tag_name(),
		     n->get_attributes()+(["hidden":"1"]), 0);
      if(wrapper)
	wr = wrap( wr, wrapper->clone() );

      move_items_low(c, e, wr);

      if(!sizeof(e))
	m_delete(jobs, name);
    }
}

void move_items(Node n, mapping jobs)
{
  if(jobs[0]) {
    jobs[0](n);
    m_delete(jobs, 0);
  }
  if(!sizeof(jobs) && !sizeof(ns_queue)) return;

  foreach(n->get_elements("namespace"), Node c) {
    mapping m = c->get_attributes();
    string name = m->name + "::";

    if(ns_queue[name]) {
      ns_queue[name]( c );
      m_delete(ns_queue, name);
    }

    mapping e = jobs[name];
    if(!e) continue;

    Node wr = Node(XML_ELEMENT, "autodoc",
		   n->get_attributes()+(["hidden":"1"]), 0);

    move_items_low(c, e, wr);

    if(!sizeof(e))
      m_delete(jobs, name);
  }

  foreach(indices(ns_queue), string name)
    werror("Failed to move namespace %O.\n", name);
}

string make_toc_entry(Node n) {
  if(n->get_tag_name()=="section")
    return n->get_attributes()->title;

  while(n->get_attributes()->hidden)
    n = n->get_first_element();

  array a = reverse(n->get_ancestors(1));
  array b = a->get_any_name();
  int root;
  foreach( ({ "manual", "dir", "file", "chapter",
	      "section", "subsection" }), string node)
    root = max(root, search(b, node));
  a = a[root+1..];
  if(sizeof(a) && a[0]->get_attributes()->name=="")
    a = a[1..];
  return a->get_attributes()->name * ".";
}

void make_toc() {
  for(int i; i<sizeof(chapters); i++) {
    foreach(chapters[i]->get_elements(), Node c)
      if( (< "section", "module", "class" >)[c->get_tag_name()] )
	toc[i] += ({ make_toc_entry(c) });
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
  if(has_value(args, "--version"))
     werror("$Id: assemble_autodoc.pike,v 1.31 2004/07/08 17:48:48 grubba Exp $\n");
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
	   "  ch:%d toc:%O\n",
	   chapter, toc);
    throw(err);
  }

  werror("Parsing autodoc file %O.\n", args[2]);
  Node m = parse_file(args[2]);
  m = m->get_first_element("autodoc");

  werror("Executing node insertions.\n");
  move_items(m, queue);
  if(sizeof(queue)) {
    report_failed_entries(queue, "");
    return 1;
  }

  make_toc();

  werror("Writing final manual source file.\n");
  write( (string)n );
  werror("Took %d seconds.\n\n", time()-T);
  return 0;
}
