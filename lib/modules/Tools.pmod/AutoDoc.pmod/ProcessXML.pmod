#pike __REAL_VERSION__

// This module is the most high-level.
// Some functions to apply to the XML tree after extraction, too, for
// reference resolving etc.

#include "./debug.h"

static inherit Parser.XML.Tree;
static inherit "module.pmod";

#define DEB werror("###%s:%d\n", __FILE__, __LINE__);
static private void processError(string message, mixed ... args) {
  throw ( ({ sprintf("ProcessXML: "+message+"\n", @args),
	     backtrace() }) );
}


//========================================================================
// From source file to XML
//========================================================================

static object makeWrapper(array(string) modules, object|void child)
{
  foreach(reverse(modules) + ({ "" }), string n) {
    object m = .PikeObjects.Module();
    m->name = n;
    if (child)
      m->AddChild(child);
    child = m;
  }
  return child;
}

//! This function extracts documentation from a file. The parameters
//! @[type], @[name], and @[parentModules] are used only when
//! @[pikeMode] != 0 and no C-style doc comments are present.
//!
//! @param filename
//!   The file to extract from.
//! @param pikeMode
//!   Non-zero if it is a Pike file. If the file contains
//!   style doc comments, C-mode is used despite pikeMode != 0.
//! @param type
//!   "class" or "module".
//! @param name
//!   The name of the class/module.
//! @param parentModules
//!   The ancestors of the class/module.
//!
//! @example
//!   // To extract doc for Foo.Bar.Ippa:
//!   string xml = extractXML("lib/modules/Foo.pmod/Bar.pmod/Ippa.pike", 1,
//!     "class", "Ippa", ({ "Foo", "Bar" }));
//!
string extractXML(string filename, int|void pikeMode, string|void type,
                  string|void name, array(string)|void parentModules)
{
  // extract the file...
  // check if there are C style doc comments in it,
  // because if there are, the CExtractor should be
  // used instead of the PikeExtractor
  int styleC = !pikeMode;
  int stylePike = pikeMode;

  string contents = Stdio.File(filename)->read();

  //  if (pikeMode && has_value(contents, "/*!")) {
  //    array(string) a = Parser.Pike.split(contents);
  //    // check if it _really_ is a C-style doc comment ...
  //    foreach (a, string s)
  //      if (strlen(s) >= strlen("/*!*/") && s[0..2] == "/*!")
  //	styleC = 1;
  //      else if (isDocComment(s))
  //	stylePike = 1;
  //    if (stylePike && styleC)
  //      processError("both C and Pike style doc comments in file " + filename);
  //  }

  if (styleC && has_value(contents, "/*!")) {
    object m = .CExtractor.extract(contents, filename);
    return m->xml();
  }
  else if(stylePike && has_value(contents, "//!")) {
    if(has_suffix(filename, ".pmod.in"))
      contents = replace(contents, "@module@", "\"___"+parentModules*"."+"\"");
    object m = (type == "module")
      ? .PikeExtractor.extractModule(contents, filename, name)
      : .PikeExtractor.extractClass(contents, filename, name);
    return makeWrapper(parentModules, m)->xml();
  }
  return "";
}

//========================================================================
// IMAGES MOVED TO CANONICAL FILES
//========================================================================

#define CONCAT_CHAR "."

//!   Copy all images to canonical files in a flat directory.
//! @param docXMLFile
//!   The contents of the XML file. The XML file is the result of extraction
//!   from a single C or Pike file, for example the result of calling
//!   @[extractXML].
//! @param imageSourceDir
//!   The directory that is the base of the relative paths to images. This
//!   should be the directory of the source file that was the input to
//!   extract the XML file.
//! @param imageDestDir
//!   The directory where the images should be copied.
//! @returns
//!   The XML file contents (with decorated <image>-tags)
string moveImages(string docXMLFile,
                  string imageSourceDir,
                  string imageDestDir)
{
  array(string) parents = ({});
  int counter = 0;
  array(int) docgroupcounters = ({}); // docgroup on top level is impossible
  string cwd = getcwd();
  Node n = parse_input(docXMLFile)[0];
  n->walk_preorder_2(
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT) {
        mapping(string : string) attr = n->get_attributes();
        switch (n->get_any_name()) {

          case "docgroup":
            ++docgroupcounters[-1];
            if (attr["homogen-name"])
              parents += ({ attr["homogen-name"] });
            else {
              string name = 0;
              foreach (n->get_children(), Node c) {
                if (c->get_node_type() == XML_ELEMENT)
                  if (name = c->get_attributes()["name"])
                    break;
              }
              parents += ({ name || (string) docgroupcounters[-1] });
            }
            counter = 0;
            break;

  	  case "appendix":
	    if(attr->name != "")
	      parents += ({ "APPENDIX" + hash(attr->name) });
	    break;

	  case "chapter":
	    if(attr->name != "")
	      parents += ({ "CHAPTER" + hash(attr->name) });

          case "class":
          case "module":
            if (attr["name"] != "")
              parents += ({ attr["name"] });
            counter = 0;
            docgroupcounters += ({ 0 });
            break;

          case "image":
            array(Node) children = n->get_children();
            if (sizeof(children || ({})) != 1
                || children[0]->get_node_type() != XML_TEXT)
              processError("bad image tag: %s\n", n->html_of_node());
            string imageFilename = children[0]->get_text();
            imageFilename = combine_path(imageSourceDir, imageFilename);
            imageFilename = combine_path(cwd, imageFilename);
            string formatExt = (basename(imageFilename) / ".")[-1];
            string canonicalName =
              (parents + ({ (string) ++counter })) * CONCAT_CHAR;
            string destFilename = imageDestDir + "/" + canonicalName
              + "." + formatExt;

	    mapping args = n->get_attributes();
            werror("copying from [%s] to [%s]\n", imageFilename, destFilename);
	    if(!Stdio.cp(imageFilename, destFilename)) {
	      werror("Error: Could not move %s to %s.\n", imageFilename, destFilename);
	      if(!Stdio.read_file(imageFilename))
		werror("(Could not read %s)\n", imageFilename);
	    }
	    else {
	      Image.Image o = Image.load(imageFilename);
	      if(o && o->xsize() && o->ysize()) {
		args->width = (string)o->xsize();
		args->height = (string)o->ysize();
	      }
	    }
            args->file = canonicalName + "." + formatExt;
            break;
        }
      }
    },
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT) {
        switch (n->get_any_name()) {
          case "class":
          case "module":
            docgroupcounters =
              docgroupcounters[0 .. sizeof(docgroupcounters) - 2];
            // fall through
          case "docgroup":
            parents = parents[0 .. sizeof(parents) - 2];
            break;
        }
      }
    }
  );
  return n->html_of_node();
}

//========================================================================
// BUILDING OF THE BIG TREE
//========================================================================

static int isClass(Node node) { return node->get_any_name() == "class"; }
static int isModule(Node node) { return node->get_any_name() == "module"; }
static int isDoc(Node node) { return node->get_any_name() == "doc"; }

static string getName(Node node) { return node->get_attributes()["name"]; }

//!   Puts all children of @[source] into the tree @[dest], in their right
//!   place module-hierarchically.
//!   Used to merge the results of extractions of different Pike and C files.
//! @param source
//! @param dest
//!   The nodes @[source] and @[dest] are @tt{<class>@} or @tt{<module>@}
//!   nodes that are identical in the sense that they represent the same
//!   module or class. Typically they both represent the top module.
//! @note
//!   After calling this method, any @tt{<class>@} or @tt{<module>@} nodes
//!   that have been marked with @@appears or @@belongs, are still in their
//!   wrong place in the tree, so @[handleAppears] must be called on the
//!   whole documentation tree has been merged.
void mergeTrees(Node dest, Node source) {
  mapping(string : Node) dest_children = ([]);
  int dest_has_doc = 0;

  foreach(dest->get_children(), Node node)
    if (isClass(node) || isModule(node))
      dest_children[getName(node)] = node;
    else if (isDoc(node))
      dest_has_doc = 1;

  foreach(source->get_children(), Node node)
    switch(node->get_any_name()) {
      case "class":
      case "module":
        {
        string name = getName(node);
        Node n = dest_children[name];
        if (n) {
          if (node->get_any_name() != n->get_any_name())
            processError("entity '" + name +
                         "' used both as a class and a module");
          mergeTrees(n, node);
        }
        else
          dest->add_child(node);
        }
        break;
      case "inherit":
      case "modifiers":
        // these shouldn't be duplicated..
        break;
      case "doc":
        if (dest_has_doc) {
          if (isClass(dest))
            processError("duplicate documentation for class " + getName(dest));
          else if (isModule(dest))
            processError("duplicate documentation for module " +
                         getName(dest));
          processError("duplicate documentation");
        }
        // fall through
      default:
        dest->add_child(node);
    }
}

static void reportError(string filename, mixed ... args) {
  werror("[%s]\t%s\n", filename, sprintf(@args));
}

//========================================================================
// HANDLING @appears directives
//========================================================================

static Node findNode(Node root, array(string) ref) {
  Node n = root;
  // top:: is just an anchor to the root
  if (sizeof(ref) && ref[0] == "top::")
    ref = ref[1..];
  if (!sizeof(ref))
    return root;
  while (sizeof(ref)) {
    array(Node) children = n->get_children();
    Node found = 0;
    foreach (children, Node child) {
      string tag = child->get_any_name();
      if (tag == "module" || tag == "class") {
        string name = child->get_attributes()["name"];
        if (name && name == ref[0]) {
          found = child;
          break;
        }
      }
    }
    if (!found)
      return 0;
    n = found;
    ref = ref[1..];
  }
  return n;
}

static class ReOrganizeTask {
  array(string) belongsRef = 0;
  string newName = 0;
  Node n;
}

static array(ReOrganizeTask) tasks;

// current is a <module>, <class> or <docgroup> node
static void recurseAppears(Node root, Node current) {
  mapping attr = current->get_attributes() || ([]);
  if (attr["appears"] || attr["belongs"] || attr["global"]) {
    ReOrganizeTask t = ReOrganizeTask();
    if (string attrRef = attr["appears"]) {
      array(string) a = splitRef(attrRef);
      t->belongsRef = a[0 .. sizeof(a) - 2];
      t->newName = a[-1];
    }
    else if (string belongsRef = attr["belongs"]) {
      array(string) a = splitRef(belongsRef);
      t->belongsRef = a;
    }
    t->n = current;
    tasks += ({ t });
  }
  if ( (<"class", "module">)[ current->get_any_name() ] )
    foreach (current->get_children(), Node child)
      if ((<"module", "class", "docgroup">)[child->get_any_name()])
        recurseAppears(root, child);
}

//! Take care of all the @@appears and @@belongs directives everywhere,
//! and rearrange the nodes in the tree accordingly
void handleAppears(Node root) {
  tasks = ({ });
  recurseAppears(root, root);
  tasks = Array.sort_array(
    tasks,
    lambda (ReOrganizeTask task1, ReOrganizeTask task2) {
      return sizeof(task1->belongsRef) > sizeof(task2->belongsRef);
    } );
  foreach (tasks, ReOrganizeTask task) {
    Node n = task->n;
    string type = n->get_any_name();
    array(string) belongsRef = task->belongsRef;
    string newName = task->newName;
    Node belongsNode = findNode(root, belongsRef);
    if (!belongsNode) {
      werror("Couldn't find the node: %O\n", belongsRef*".");
      continue;
    }
    if (type == "docgroup") {
      if (newName) {
	mapping m = n->get_attributes();
	if(m["homogen-name"])
	  m["homogen-name"] = newName;
        foreach (n->get_children(), Node child)
          if (child->get_node_type() == XML_ELEMENT) {
            mapping attributes = child->get_attributes();
            if (attributes["name"])
              // this ought to happen only once in this loop...
              attributes["name"] = newName;
          }
      }
    }
    else if (type == "class" || type == "module") {
      if (newName)
        n->get_attributes()["name"] = newName;
    }

    m_delete(n->get_attributes(), "belongs");
    m_delete(n->get_attributes(), "appears");

    Node parent = n->get_parent();
    if (parent)
      parent->remove_child(n);
    belongsNode->add_child(n);
  }
}

//========================================================================
// RESOLVING REFERENCES
//========================================================================

// Rather DWIM splitting of a string into a chain of identifiers:
// "Module.Class->funct(int i)" ==> ({"Module","Class","func"})
// "\"foo.pike\"" ==> ({ "\"foo.pike\"" })
static array(string) splitRef(string ref) {
  if ((sizeof(ref)>1) && (ref[0] == '"')) {
    // Explictly named program.
    // Try to DWIM it...
    ref = ref[1..sizeof(ref)-2];
    ref = replace(ref, ({ ".pike", ".pmod" }), ({ "", "" }));
    // FIXME: What about module.pike/module.pmod?
    return ref/"/";
  }
  array(string) a = Parser.Pike.split(ref);
  string scope = "";
  array result = ({});
  if (sizeof(a) && (a[0] == "lfun" || a[0] == "predef" || a[0] == "top")) {
    scope += a[0];
    a = a[1..];
  }
  if (sizeof(a) && a[0] == "::") {
    scope += a[0];
    a = a[1..];
  }
  if (strlen(scope))
    result = ({ scope });
  for (;;) {
    if (!sizeof(a))
      return result;
    string token = a[0];
    a = a[1..];
    if (isIdent(token))
      result += ({ token });
    else
      return result;
    token = sizeof(a) ? a[0] : "";
    if (token == "(") {
      // Skip parenthesized expressions.
      int cnt = 1;
      int i = 1;
      while ((cnt > 0) && (i < sizeof(a))) {
	if (a[i] == "(") cnt++;
	else if (a[i] == ")") cnt--;
	i++;
      }
      a = a[i..];
      token = sizeof(a) ? a[0] : "";
    }
    if ((<".", "->">)[token])
      a = a[1..];
    else
      return result;
  }
}

static string mergeRef(array(string) ref) {
  string s = "";
  if (sizeof(ref) && search(ref[0], "::") >= 0) {
    s = ref[0];
    ref = ref[1..];
  }
  s += (ref - ({ "" })) * ".";
  return s;
}

static class Scope(string|void type, string|void name) {
  multiset(string) idents = (<>);

  multiset(string) failures = (<>);

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O:%O:%s)", this_program, type, name,
			     (sort(indices(idents))*",") );
  }
}

static class ScopeStack {
  /*static*/ array(Scope) scopeArr = ({ 0 }); // sentinel

  void enter(string|void type, string|void name) {
    //werror("entering scope type(%O), name(%O)\n", type, name);
    scopeArr += ({ Scope(type, name) });
  }
  void leave() {
    if (sizeof(scopeArr[-1]->failures)) {
      werror("WARNING: Failed to resolv the following symbols in scope %O:\n"
	     "%{  %O\n%}\n",
	     scopeArr[1..]->name * ".",
	     indices(scopeArr[-1]->failures));
    }
    scopeArr = scopeArr[..sizeof(scopeArr)-2];
  }

  void addName(string sym) { scopeArr[-1]->idents[sym] = 1; }

  void remName(string sym) { scopeArr[-1]->idents[sym] = 0; }

  mapping resolveRef(string ref) {
    array(string) idents = splitRef(ref);
    int not_param = has_suffix(ref, "()");
    if (!sizeof(idents))
      return ([ "resolution-failure" : "yes" ]);

#if 0
    if (has_prefix(ref, "glGetIntegerv")) {
      werror("ref:%O\n"
	     "idents:%O\n"
	     "stack:%O\n",
	     ref, idents,
	     scopeArr);
    }
#endif /* 0 */

    if (idents[0] == "top::") {
      // top:: is an anchor to the root.
      ref = mergeRef(idents[1..]);
      idents = idents[1..];
    } else if(idents[0] == "predef::") {
      // Better-than-nothing
      // FIXME: Should look backwards until it finds a
      // matching symbol.
      ref = mergeRef(idents[1..]);
      idents = idents[1..];
    }
    else {
      if (!scopeArr[-1]->failures[ref]) {
	array(string) matches = ({});

	ref = mergeRef(idents);

	string firstIdent = idents[0];
	for(int i = sizeof(scopeArr)-1; i ; i--) {
	  Scope s = scopeArr[i];
	  if (s->idents[firstIdent])
	    if (s->type == "params" && !not_param) {
	      return ([ "param" : ref ]);
	    }
	    else {
	      //werror("[[[[ found in type(%O) name(%O)\n", s->type, s->name);
	      string res = "";
	      // work our way from the root of the stack
	      for (int j = 1; j <= i; j++) {
		string name = scopeArr[j]->name;
		if (name && name != "")
		  res += name + ".";
		//werror("[[[[ name == %O\n", name);
	      }
	      matches += ({ res + ref });
	    }
	}
	if (sizeof(matches)) {
	  return ([ "resolved" : matches*"\0" ]);
	}
	// Resolution failure.
	int i;
	for (i = sizeof(scopeArr)-1; i; i--) {
	  if (scopeArr[i]->type != "params") {
	    scopeArr[i]->failures[ref] = 1;
	    break;
	  }
	}
      }
      return ([ "resolution-failure" : "yes" ]);
    }

    // TODO: should we check that the symbol really DOES appear
    // on the top level too?
    return ([ "resolved" : ref ]); // we consider it to be an absolute reference
  }
}

static void fixupRefs(ScopeStack scopes, Node node) {
  node->walk_preorder(
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT) {
        // More cases than just "ref" ??
        // Add them here if we decide that also references in e.g.
        // object types should be resolved.
	string name = n->get_any_name();
        if ((<"ref", "classname", "object">)[name]) {
          mapping m = n->get_attributes();
          if (m["resolved"])
            return;
          mapping resolved = scopes->resolveRef(m->to || n->value_of_node());
          foreach (indices(resolved), string i)
            m[i] = resolved[i];
        }
      }
    }
  );
}

// expects a <module> or <class> node
static void resolveFun(ScopeStack scopes, Node node) {
  // first collect the names of all things inside the scope
  foreach (node->get_children(), Node child)
    if (child->get_node_type() == XML_ELEMENT) {
      switch (child->get_any_name()) {
        case "docgroup":
          // add the names of all things documented
          // in the docgroup.
          foreach (child->get_children(), Node thing)
            if (child->get_node_type() == XML_ELEMENT) {
              mapping attributes = thing->get_attributes() || ([]);
              string name = attributes["name"];
              if (name)
                scopes->addName(name);
            }
          break;
        case "module":
        case "class":
          // add the name of the submodule/subclass to the scope
          string name = child->get_attributes()["name"];
          if (name)
            scopes->addName(name);
          break;
        default:
          ; // do nothing (?)
      }
    }
  foreach (node->get_children(), Node child) {
    if (child->get_node_type() == XML_ELEMENT) {
      string tag = child->get_any_name();
      switch (tag) {
        case "class":
        case "module":
          scopes->enter(tag, child->get_attributes()["name"]);
          {
            resolveFun(scopes, child);
          }
          scopes->leave();
          break;
        case "docgroup":
          {
            scopes->enter("params");
            foreach (child->get_children(), Node n) {
              if (n->get_any_name() == "method") {
                foreach (filter(n->get_children(),
                                lambda (Node n) {
                                  return n->get_any_name() == "arguments";
                                }), Node m)
                  foreach (m->get_children(), Node argnode) {
		    if(argnode->get_node_type() != XML_ELEMENT)
		      continue;
                    scopes->addName(argnode->get_attributes()["name"]);
		  }
	      }
	    }
            Node doc = 0;
	    foreach(child->get_children(), Node n) {
	      if (n->get_any_name() == "doc") {
		doc = n;
		fixupRefs(scopes, n);
	      }
	    }
	    if (!doc)
	      werror("No doc element found\n%s\n\n", child->render_xml());
	    scopes->leave();
	    if ((child->get_attributes()["homogen-type"] == "inherit") &&
		(child->get_attributes()["homogen-name"])) {
	      // Avoid finding ourselves...
	      scopes->remName(child->get_attributes()["homogen-name"]);
	      foreach(child->get_children(), Node n) {
		if (n->get_any_name() != "doc") {
		  fixupRefs(scopes, n);
		}
	      }
	      scopes->addName(child->get_attributes()["homogen-name"]);
	    } else {
	      foreach(child->get_children(), Node n) {
		if (n->get_any_name() != "doc") {
		  fixupRefs(scopes, n);
		}
	      }
	    }
          }
          break;
        case "doc":  // doc for the <class>/<module> itself
          fixupRefs(scopes, child);
          break;
        case "inherit":
	  child = child->get_first_element("classname");
          mapping m = child->get_attributes();
          if (m["resolved"])
            break;
          mapping resolved = scopes->resolveRef(child->value_of_node());
          foreach (resolved; string i; string v)
	    m[i] = v;
	  // werror("Inherit: m:%O\n", m);
	  break;
        default:
          ; // do nothing
      }
    }
  }
}

// Resolves references, regarding the Node 'tree' as the root of the whole
// hierarchy. 'tree' might be the node <root> or <module name="Pike">
void resolveRefs(Node tree) {
  ScopeStack scopes = ScopeStack();
  scopes->enter("module", "");    // The top level scope
  resolveFun(scopes, tree);
  scopes->leave();
}

void cleanUndocumented(Node tree) {

  void check_node(Node n) {
    string name = n->get_tag_name();
    if(name!="class" && name!="module") return;
    array ch = n->get_elements()->get_tag_name();
    ch -= ({ "modifiers" });
    ch -= ({ "source-position" });
    if(sizeof(ch)) return;
    Node p = n->get_parent();
    p->remove_child(n);
    werror("Removed empty %s %O\n", name, n->get_attributes()->name);
    check_node(p);
  };

  tree->walk_preorder( check_node );
}

// Call this method after the extraction and merge of the tree.
void postProcess(Node tree) {
  //  werror("handleAppears\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
  handleAppears(tree);
  //  werror("cleanUndocumented\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
  cleanUndocumented(tree);
  //  werror("resolveRefs\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
  resolveRefs(tree);
  //  werror("done postProcess\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
}
