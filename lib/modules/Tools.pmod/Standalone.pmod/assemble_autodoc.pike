
#pike __REAL_VERSION__

constant description = "Assembles AutoDoc output file.";

// AutoDoc mk II assembler

#define Node Parser.XML.Tree.Node
#define CommentNode Parser.XML.Tree.CommentNode
#define XML_ELEMENT Parser.XML.Tree.XML_ELEMENT
#define XML_TEXT Parser.XML.Tree.XML_TEXT
#define RootNode Parser.XML.Tree.RootNode
#define HeaderNode Parser.XML.Tree.HeaderNode
#define TextNode Parser.XML.Tree.TextNode
#define ElementNode Parser.XML.Tree.ElementNode

int chapter;
int appendix;

mapping queue = ([]);
mapping appendix_queue = ([]);
mapping ns_queue = ([]);
array(Node) chapters = ({});

Tools.AutoDoc.Flags flags = Tools.AutoDoc.FLAG_NORMAL;

int verbose = Tools.AutoDoc.FLAG_NORMAL;

string refdocdir;

Node void_node = Node(XML_ELEMENT, "void", ([]), 0);

// array( array(name,file,chapter_no,(subchapters)) )
array toc = ({});
class TocNode {
  inherit Node;
  string path;
  int(1..3) depth;

  protected void make_children() {
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
			    ([ "href" : file + "#" + ent[2] + "." + sub ]), 0 );
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

  protected void create(string _path, int(1..3) _depth) {
    path = _path;
    depth = _depth;
    ::create(XML_ELEMENT, "dl", ([]), "");
  }
}

class Entry (Node target) {
  constant type = "";
  mapping args;

  protected string _sprintf(int t) {
    return sprintf("%sEntry( %O )", type, target);
  }

  int done;
  protected void `()(Node data) {
    done++;
  }

  int pass2()
  {
    return done;
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

  Node parent;

  array(Node) result = ({});

  protected void create(Node target, Node parent)
  {
    ::create(target);
    mvEntry::parent = parent;
  }

  protected void `()(Node data) {
    if(args) {
      mapping m = data->get_attributes();
      foreach(indices(args), string index)
	m[index] = args[index];
    }
    result += ({ data });

    ::`()(data);
  }

  int pass2()
  {
    // WARNING! Disrespecting information hiding!
    int pos = search(parent->mChildren, target);
    if (pos < 0) {
      error("Lost track of destination target!\n");
    }
    parent->mChildren =
      parent->mChildren[..pos-1] + result + parent->mChildren[pos+1..];

    return ::pass2();
  }
}

class mvPeelEntry {
  inherit Entry;
  constant type = "mvPeel";

  protected Node parent;

  protected void create(Node target, Node parent)
  {
    ::create(target);
    mvPeelEntry::parent = parent;
  }

  protected void `()(Node data) {
    // WARNING! Disrespecting information hiding!
    int pos = search(parent->mChildren, data);
    array pre = parent->mChildren[..pos-1];
    array post = parent->mChildren[pos+1..];
    parent->mChildren = pre + data->mChildren + post;

    ::`()(data);
  }
}

class cpEntry {
  inherit Entry;
  constant type = "cp";

  protected void `()(Node data) {
    // clone data subtree
    // target->replace_node(clone);

    ::`()(data);
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

Node parse_file(string fn)
{
  Node n;
  mixed err = catch {
    n = Parser.XML.Tree.parse_file(fn);
  };
  if(stringp(err)) error(err);
  if(err) throw(err);
  return n;
}

void chapter_ref_expansion(Node n, string dir, array(int) section_path) {
  int section;
  foreach(n->get_elements(), Node c)
    switch(c->get_tag_name()) {


    case "ul":
    case "ol":
    case "li":
    case "dl":
    case "dt":
    case "dd":
    case "p":
    case "example":
    case "matrix":
      break;

    case "contents":
      n->replace_child(c, TocNode(dir, 3) );
      break;

    case "section":
    case "subsection":
      if (sizeof(section_path) == 1) {
	c->set_tag_name("section");
      } else {
	c->set_tag_name("subsection");
      }
      c->get_attributes()->number = (string)++section;
      chapter_ref_expansion(c, dir,
			    section_path + ({ c->get_attributes()->number }));
      break;

    case "insert-move":
      enqueue_move(c, n);
      break;

    default:
      error("Unknown element %O in %s element.\n",
	    c->get_tag_name(), n->get_tag_name());
      break;
    }
}

int filec;
void ref_expansion(Node n, string dir, void|string file)
{
  string path;
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
      path = combine_path(refdocdir, c->get_attributes()->file);
      if (!Stdio.exist(path)) {
	if ((verbose >= Tools.AutoDoc.FLAG_VERBOSE) ||
	    (!(flags & Tools.AutoDoc.FLAG_COMPAT))) {
	  werror("Warning: Chapter file %O not found.\n", path);
	}
	n->remove_child(c);
	break;
      }
      mixed err = catch {
	  n->replace_child(c, c = parse_file(path)->
			   get_first_element("chapter") );
	};
      if (err) {
	n->remove_child(c);
	if (flags & Tools.AutoDoc.FLAG_KEEP_GOING) {
	  werror("Chapter ref for %O failed:\n"
		 "%s\n", path, describe_backtrace(err));
	  break;
	}
	throw(err);
      }
      // fallthrough
    case "chapter":
      mapping m = c->get_attributes();
      if(m->unnumbered!="1")
	m->number = (string)++chapter;
      toc += ({ ({ m->title, file, m->number }) });
      chapters += ({ c });
      chapter_ref_expansion(c, dir, ({ m->number }));
      break;

    case "appendix-ref":
      if (!(flags & Tools.AutoDoc.FLAG_COMPAT)) {
	error("Appendices are only supported in compat mode.\n");
      }
      if(!file)
	error("appendix-ref element outside file element\n");
      if(c->get_attributes()->name) {
	Entry e = mvEntry(c, n);
	e->args = ([ "number": (string)++appendix ]);
	appendix_queue[c->get_attributes()->name] = e;

	// No more than 26 appendicies...
	toc += ({ ({ c->get_attributes()->name, file,
		     ("ABCDEFGHIJKLMNOPQRSTUVWXYZ"/1)[appendix-1] }) });
	break;
      }
      if(!c->get_attributes()->file)
	error("Neither file nor name attribute on appendix-ref element.\n");
      path = combine_path(refdocdir, c->get_attributes()->file);
      if (!Stdio.exist(path)) {
	if ((verbose >= Tools.AutoDoc.FLAG_VERBOSE) ||
	    (!(flags & Tools.AutoDoc.FLAG_COMPAT))) {
	  werror("Warning: Appendix file %O not found.\n", path);
	}
	n->remove_child(c);
	break;
      }
      err = catch {
	  c = c->replace_node( parse_file(path)->
			       get_first_element("appendix") );
	};
      if (err) {
	n->remove_child(c);
	if (flags & Tools.AutoDoc.FLAG_KEEP_GOING) {
	  werror("Appendix ref for %O failed:\n"
		 "%s\n", path, describe_backtrace(err));
	  break;
	}
	throw(err);
      }
      // fallthrough
    case "appendix":
      if (!(flags & Tools.AutoDoc.FLAG_COMPAT)) {
	error("Appendices are only supported in compat mode.\n");
      }
      c->get_attributes()->number = (string)++appendix;

      // No more than 26 appendicies...
      toc += ({ ({ c->get_attributes()->name, file,
		   ("ABCDEFGHIJKLMNOPQRSTUVWXYZ"/1)[appendix-1] }) });
      break;

    case "void":
      n->remove_child(c);
      void_node->add_child(c);
      chapter_ref_expansion(c, dir, ({}));
      break;

    default:
      werror("Warning unhandled tag %O in ref_expansion\n",
	     c->get_tag_name());
      break;
    }
  }
}

void job_queue_pass2(mapping job_queue)
{
  foreach(job_queue; string ind; mapping|Entry entry) {
    if (mappingp(entry)) {
      job_queue_pass2(entry);
      if (sizeof(entry)) continue;
    } else if (objectp(entry)) {
      if (!entry->done) {
	werror("Entry %O: %O not done!\n", ind, entry);
	continue;
      }
      if (!entry->pass2()) {
	werror("Entry %O: %O failed in pass2!\n", ind, entry);
	continue;
      }
    }
    m_delete(job_queue, ind);
  }
}

void move_appendices(Node n) {
  foreach(n->get_elements("appendix"), Node c) {
    string name = c->get_attributes()->name;
    Node a = appendix_queue[name];
    if(a) {
      a(c);
    }
    else {
      c->remove_node();
      if (verbose) {
	werror("Removed untargeted appendix %O.\n", name);
      }
    }
  }

  job_queue_pass2(appendix_queue);

  if(sizeof(appendix_queue)) {
    if (verbose) {
      werror("Failed to find appendi%s %s.\n",
	     (sizeof(appendix_queue)==1?"x":"ces"),
	     String.implode_nicely(map(indices(appendix_queue),
				       lambda(string in) {
					 return "\""+in+"\"";
				       })) );
    }
    foreach(values(appendix_queue), Node a) {
      // Remove the place-holder.
      a(ElementNode("appendix", ([])));
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

protected void move_items_low(Node parent, Node n, mapping jobs,
			      void|Node wrapper) {
  if(jobs[0]) {
    // Detach the node from its original parent.
    parent->remove_child(n);
    if(wrapper)
      jobs[0]( wrap(n, wrapper->clone()) );
    else
      jobs[0](n);
  }
  if(!sizeof(jobs)) return;

  foreach( ({ "module", "class", "docgroup" }), string type)
    foreach(n->get_elements(type), Node c) {
      mapping parent = jobs;
      mapping m = c->get_attributes();
      string name = m->name || m["homogen-name"];
      string parent_name;
      mapping e;
      if (name) {
	name = replace(name, "-", ".");
	e = jobs[ name ];
      } else if (type == "docgroup") {
	// Check if any of the symbols in the group is relevant.
	foreach(c->get_elements("method")+c->get_elements("directive"),
		Node m) {
	  mapping attrs = m->get_attributes();
	  name = attrs->name && replace(attrs->name, "-", ".");
	  if (e = jobs[name]) break;
	}
      }
      if (!e /*&& (flags & Tools.AutoDoc.FLAG_COMPAT)*/) {
	array(string) path;
	if (path = ([
	      "/precompiled/FILE": "Stdio.FILE"/".",
	      "/precompiled/condition": "Thread.Condition"/".",
	      "/precompiled/fifo": "Thread.Fifo"/".",
	      "/precompiled/file": "Stdio.File"/".",
	      "/precompiled/gdbm": "Gdbm.gdbm"/".",
	      "/precompiled/mpz": "Gmp.mpz"/".",
	      "/precompiled/mutex": "Thread.Mutex"/".",
	      "/precompiled/mysql": "Mysql.mysql"/".",
	      "/precompiled/mysql_result": "Mysql.mysql_result"/".",
	      "/precompiled/port": "Stdio.Port"/".",
	      "/precompiled/queue": "Thread.Queue"/".",
	      "/precompiled/regexp": "Regexp"/".",
	      "/precompiled/sql": "Sql.sql"/".",
	      "/precompiled/sql/msql": "Msql.msql"/".",
	      "/precompiled/sql/mysql": "Mysql.mysql"/".",
	      "/precompiled/sql/mysql_result": "Mysql.mysql_result"/".",
	      "/precompiled/sql/postgres": "Postgres.postgres"/".",
	      "/precompiled/sql/sql_result": "Sql.sql_result"/".",
	      "/precompiled/stack": "Stack"/".",
	      "/precompiled/string_buffer": "String.Buffer"/".",
	    ])[name]) {
	  e = jobs;
	  foreach(path; int i; name) {
	    if (i) parent_name = path[i-1];
	    parent = e;
	    e = e[name];
	    if (!e) {
	      if ((parent != jobs) && parent[0] && parent[0]->parent) {
		// There's a move for our parent, but we can't use it directly,
		// since there may be more nodes that need moving.
		//
		// So we add a move for ourselves.
		//
		// NB: This is a bit round-about, since we could just add
		//     ourselves directly...
		Node dummy = ElementNode("insert-move",
					 ([ "entity":path[..i]*"." ]));
		parent[0]->parent->add_child_after(dummy, parent[0]->target);
		Entry new_mv = mvEntry(dummy, parent[0]->parent);
		e = parent[name] = ([ 0: new_mv ]);
		continue;
	      }
	      break;
	    }
	  }
	}
      }
      if(!e) continue;

      Node wr = Node(XML_ELEMENT, n->get_tag_name(),
		     n->get_attributes()+(["hidden":"1"]), 0);
      if(wrapper)
	wr = wrap( wr, wrapper->clone() );

      move_items_low(n, c, e, wr);
    }
}

void move_items(Node n, mapping jobs)
{
  if(jobs[0]) {
    jobs[0](n);
  }

  // Skip looping if this is the onepage case.
  if((sizeof(jobs) != !!jobs[0]) || sizeof(ns_queue)) {

    foreach(n->get_elements("namespace"), Node c) {
      mapping m = c->get_attributes();
      string name = m->name + "::";

      if(ns_queue[name]) {
	ns_queue[name]( c );
      }

      mapping e = jobs[name];
      if(!e) continue;

      Node wr = Node(XML_ELEMENT, "autodoc",
		     n->get_attributes()+(["hidden":"1"]), 0);

      move_items_low(n, c, e, wr);
    }
  }

  job_queue_pass2(ns_queue);
  job_queue_pass2(jobs);

  foreach(ns_queue; string name; Entry n) {
    if (verbose >= Tools.AutoDoc.FLAG_VERBOSE) {
      werror("Failed to move namespace %O.\n", name);
    }
    n(ElementNode("namespace", ([])));
  }
  job_queue_pass2(ns_queue);
}

void clean_empty_files(Node n)
{
  foreach(n->get_elements("dir"), Node d) {
    foreach(d->get_elements("file"), Node f) {
      foreach(f->get_elements("appendix"), Node a) {
	if (!sizeof(a->get_elements())) {
	  // Empty appendix.
	  f->remove_child(a);
	}
      }
      foreach(f->get_elements("namespace"), Node a) {
	if (!sizeof(a->get_elements())) {
	  // Empty appendix.
	  f->remove_child(a);
	}
      }
      if (!sizeof(f->get_elements())) {
	// No documentation in this file.
	d->remove_child(f);
      }
    }
    if (!sizeof(d->get_elements())) {
      // Remove the directory as well.
      n->remove_child(d);
    }
  }
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
    if (verbose >= Tools.AutoDoc.FLAG_NORMAL) {
      werror("Failed to move %s\n", path[1..]);
    }
    scope[0](CommentNode(sprintf("<insert-move entity=\"%s\" />", path[1..])));
    m_delete(scope, 0);
  }
  foreach(scope; string id; mapping next)
    report_failed_entries(next, path + "." + id);
}

class Options
{
  inherit Arg.Options;

  constant help_pre = "pike -x assemble_autodoc <structure file> <autodoc file>";

  Opt pikever = HasOpt("--pike-version")|Default(version());
  Opt timestamp = Int(HasOpt("--timestamp")|HasOpt("--time-stamp")|Default(time()));
  Opt help = NoOpt("-h")|NoOpt("--help");
  Opt output = HasOpt("-o")|HasOpt("--output")|HasOpt("--out");
  Opt verbose = NoOpt("-v")|NoOpt("--verbose");
  Opt quiet = NoOpt("-q")|NoOpt("--quiet");
  Opt compat = NoOpt("--compat");
  Opt no_dynamic = NoOpt("--no-dynamic");
  Opt keep_going = NoOpt("--keep-going");
}

int(0..1) main(int num, array(string) args)
{
  Options options = Options(args);
  if(options->help) exit(1);

  int T = time();

  if( options->quiet )
  {
    options->verbose = Tools.AutoDoc.FLAG_QUIET;
    flags &= ~Tools.AutoDoc.FLAG_VERB_MASK;
  }
  else if( options->verbose )
    flags = (flags & ~Tools.AutoDoc.FLAG_VERB_MASK) |
      min(verbose, Tools.AutoDoc.FLAG_DEBUG-1);

  if( options->keep_going )
    flags |= Tools.AutoDoc.FLAG_KEEP_GOING;
  if( options->compat )
    flags |= Tools.AutoDoc.FLAG_COMPAT;

  args = options[Arg.REST];
  if(sizeof(args)<2)
    exit(1," Too few arguments\n");

  if (verbose >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Parsing structure file %O.\n", args[0]);
  Node n = parse_file(args[0]);
  n = n->get_first_element("manual");
  n->get_attributes()->version = options->pikever;
  mapping t = localtime(options->timestamp);
  n->get_attributes()["time-stamp"] =
    sprintf("%4d-%02d-%02d", t->year+1900, t->mon+1, t->mday);
  refdocdir = combine_path(args[0], "../..");
  if (verbose >= Tools.AutoDoc.FLAG_VERBOSE) {
    werror("Refdoc directory: %s\n", refdocdir);
    werror("Executing reference expansion and queueing node insertions.\n");
  }
  mixed err = catch {
    ref_expansion(n, ".");
  };
  if (err) {
    if (flags & Tools.AutoDoc.FLAG_COMPAT) {
      werror("ref_expansion() failed:\n"
	     "  ch:%d app:%d toc:%O\n",
	     chapter, appendix, toc);
    } else {
      werror("ref_expansion() failed:\n"
	     "  ch:%d toc:%O\n",
	     chapter, toc);
    }
    if (!(flags & Tools.AutoDoc.FLAG_KEEP_GOING))
      throw(err);
  }

  if (verbose >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Parsing autodoc file %O.\n", args[1]);
  Node m = parse_file(args[1]);
  m = m->get_first_element("autodoc");

  if (flags & Tools.AutoDoc.FLAG_COMPAT) {
    if (verbose >= Tools.AutoDoc.FLAG_VERBOSE)
      werror("Moving appendices.\n");
    move_appendices(m);
  }

  if (verbose >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Executing node insertions.\n");
  move_items(m, queue);
  if(sizeof(queue)) {
    report_failed_entries(queue, "");
    if (!(flags & Tools.AutoDoc.FLAG_KEEP_GOING))
      return 1;
  }

  clean_empty_files(n);

  make_toc();

  Node root = RootNode();
  root->replace_children(({ HeaderNode(([ "version":"1.0",
					  "encoding":"utf-8" ])),
			    TextNode("\n"),
			    n,
			    TextNode("\n"),
			 }));

  if (verbose >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Writing final manual source file.\n");
  if (options->output) {
    Stdio.write_file(options->output, (string)root);
  } else {
    write( (string)root );
  }
  // Zap the XML trees so that the gc doesn't have to.
  m->zap_tree();
  n->zap_tree();
  if (verbose >= Tools.AutoDoc.FLAG_VERBOSE)
    werror("Took %d seconds.\n\n", time()-T);

  return 0;
}
