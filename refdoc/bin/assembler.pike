#define Node Parser.XML.Tree.Node

int chapter;
int appendix;

mapping queue = ([]);
mapping appendix_queue = ([]);

class Entry (Node target) {
  constant type = "";
  mapping args;
  string _sprintf() {
    return sprintf("%sTarget( %O )", type, target);
  }
}

class mvEntry {
  inherit Entry;
  constant type = "mv";

  void `()(Node data) {
    data->get_parent()->remove_child(data);
    if(args) {
      mapping m = data->get_attributes();
      foreach(indices(args), string index)
	m[index] = args[index];
    }
    target->replace_node(data);
  }
}

class cpEntry (Node target) {
  inherit Entry;
  constant type = "cp";

  void `()(Node data) {
    // clone data subtree
    // target->replace_node(clone);
  }
}

void enqueue_move(string source, Node target) {
  mapping bucket = queue;
  array path = source/".";
  if(sizeof(path)>1)
    foreach(path, string node) {
      if(!bucket[node])
	bucket[node] = ([]);
      bucket = bucket[node];
    }

  if(bucket[path[-1]])
    error("Move source already allocated.\n");

  bucket[path[-1]] = mvEntry(target);
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

void chapter_ref_expansion(Node n) {
  int section;
  foreach(n->get_elements(), Node c)
    switch(c->get_tag_name()) {

    case "p":
      break;

    case "section":
      c->get_attributes()->number = (string)++section;
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

void ref_expansion(Node n, string dir, void|string file) {
  foreach(n->get_elements(), Node c)
    switch(c->get_tag_name()) {

    case "file":
      if(file)
	error("Nested file elements.\n");
      file = c->get_attributes()->name;
      c->get_attributes()->name = dir + "/" + file;
      ref_expansion(c, dir, c->get_attributes()->name);
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
      c->get_attributes()->number = (string)++chapter;
      chapter_ref_expansion(c);
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
      break;
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

void move_items(Node n, mapping jobs) {
  array ent = n->get_elements("module") +
    n->get_elements("class") +
    n->get_elements("docgroup");
  foreach(ent, Node c) {
    mapping m = c->get_attributes();
    mapping|Entry e = jobs[ m->name || m["homogen-name"] ];
    if(!e) continue;
    if(mappingp(e)) {
      move_items(c, e);
      if(!sizeof(e))
	m_delete(jobs, m->name||m["homogen-name"]);
      continue;
    }
    e(c);
    m_delete(jobs, m->name||m["homogen-name"]);
  }
}

void main(int num, array(string) args) {

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
  ref_expansion(n, ".");

  werror("Parsing autodoc file %O.\n", args[2]);
  Node m = parse_file(args[2]);
  m = m->get_first_element("module");
  werror("Moving appendices.\n");
  move_appendices(m);
  werror("Executing node insetions.\n");
  if(queue[""])
    m_delete(queue, "")(m);
  move_items(m, queue);
  werror("Writing final manual source file.\n");
  write( (string)n );
}
