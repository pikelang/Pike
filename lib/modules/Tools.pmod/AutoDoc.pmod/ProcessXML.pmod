// This module is the most high-level.
// Some functions to apply to the XML tree after extraction, too, for
// reference resolving etc.

inherit Parser.XML.Tree;
inherit "module.pmod";

#define DEB werror("###%s:%d\n", __FILE__, __LINE__);
static private void processError(string message) {
  throw ( "XML processing error: " + message );
}

//========================================================================
// BUILDING OF THE BIG TREE
//========================================================================

static int isClass(Node node) { return node->get_any_name() == "class"; }
static int isModule(Node node) { return node->get_any_name() == "module"; }
static int isDoc(Node node) { return node->get_any_name() == "doc"; }

static string getName(Node node) { return node->get_attributes()["name"]; }

// dest and source are two <module> or <class> nodes.
// Puts all children of source into the tree dest, in their right
// place hierarchically.
// Used when we have several root nodes, which occurs when there
// are several lib root dirs, and when C files are involved (each
// C extraction generates a separate module tree with a root)
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

static constant docSuffix = ".doc.xml";

static int hasSuffix(string s, string suffix) {
  return suffix == reverse(reverse(s)[0 .. strlen(suffix) - 1]);
}

/*
array(string) getPikeName(string filename) {
  string name, exts;
  if (sscanf(filename, "%s.%s", name, exts) != 2)
    return 0;
  switch (exts) {
    case "pike":
      return ({ "class", filename });
    case "pmod.pike":
    case "pmod":
      return ({ "module", filename });
    default:
      if (reverse(reverse(filename)[0..strlen(docSuffix)-1]) == docSuffix)
        return "doc";
      return 0;
  }
}
*/

static void reportError(string filename, mixed ... args) {
  werror("[%s]\t%s\n", filename, sprintf(@args));
}

static Node fromFile(string kind, string name, string path) {
  string docFilePath = path + docSuffix;
  object stat = file_stat(path);
  object doc_stat = file_stat(docFilePath);

  if (doc_stat && doc_stat->mtime > stat->mtime) {
    werror("parsing %s file %O [CACHED]\n", kind, path);
    return parse_file(docFilePath)[0];
  } else {
    werror("parsing %s file %O\n", kind, path);
    // extract the file...
    // check if there are C style doc comments in it,
    // because if there are, the CExtractor should be
    // used instead of the PikeExtractor
    string contents = Stdio.File(path)->read();
    if (search("/*!", contents)) {
      int styleC = 0;
      int stylePike = 0;
      array(string) a = Parser.Pike.split(contents);
      // check if it _really_ is a C-style doc comment ...
      foreach (a, string s)
        if (strlen(s) >= strlen("/*!*/") && s[0..2] == "/*!")
          styleC = 1;
        else if (isDocComment(s))
          stylePike = 1;
      if (stylePike && styleC) {
        reportError(path, "both C style and Pike style comments in one file");
        return 0;
      }
      if (styleC) {
        object m = 0;
        mixed err = catch { m = .CExtractor.extract(contents, path); };
        if (err) {
          reportError(path, "%O", err);
          return 0;
        }
        return m;
      }
    }

    object m = 0;
    mixed err = catch {
      m = (kind == "module")
        ? .PikeExtractor.extractModule(contents, path, name)
        : .PikeExtractor.extractClass(contents, path, name);
    };

    if (err) {
      // some kind of error report that doesnt throw an exception, please...
      reportError(path, "%O", err);
      return 0;
    }
    string x = m ? m->xml() : "";
    Stdio.File(docFilePath, "cwt")->write(x);
    if (m)
      return parse_input(x)[0];
    return 0;
  }
}

static Node moduleFromFile(string moduleName, string path) {
  return fromFile("module", moduleName, path);
}

static Node classFromFile(string className, string path) {
  werror("classFromFile(className == %O, path == %O)\n", className, path);
  return fromFile("class", className, path);
}

static string getPikeName(string filename) {
  return array_sscanf(filename, "%s.%*s")[0];
}

// Don't use this to extract documentation, use rootModuleFromDir instead
static Node moduleFromDir(string moduleName, string directory) {
  Node module =
    parse_input("<module name='" + moduleName + "'></module>")[0];
  werror("module from directory %O\n", directory);
  array dir = get_dir(directory);
  if (!dir)
    werror("ERROR: getdir(%O) returned NULL\n", directory);
  foreach (dir, string file) {
    Filesystem.Stat stat  = file_stat(directory + "/" + file);
    if (!stat)
      continue;
    string name = getPikeName(file);
    if (stat->isdir) {
      // it's a directory
      if (hasSuffix(file, ".pmod")) {
        string name = getPikeName(file);
        Node subModule = moduleFromDir(name, directory + "/" + file);
        if (subModule)
          module->add_child(subModule);
      }
    }
    else if (stat->size > 0) {
      // it's an ordinary file
      if (hasSuffix(file, ".pmod")) {
        Node subModule = moduleFromFile(name, directory + "/" + file);
        if (subModule)
          // if it is a "module.pmod", it's not really a submodule!
          if (file == "module.pmod") {
            // This will add any <doc></doc> nodes as well!!
            foreach(subModule->get_children(), Node node)
              module->add_child(node);
          }
          else
            module->add_child(subModule);
      }
      else if (hasSuffix(file, ".pike")) {
        Node subClass = classFromFile(name, directory + "/" + file);
        if (subClass)
          module->add_child(subClass);
      }
    }
  }
  werror("leaving directory %O\n", directory);
  return module;
}

// Find all root modules that are not at the root. They may appear because
// the C extractor always produces a root module, and is not aware of
// the directory structure. Some Pike files might be extracted in C
// mode if they contained the /*! comment.
static void rearrangeRootModules(Node n, Node root, mapping|void m) {
  if (!m)
    m = ([]);
  foreach (n->get_children(), Node child)
    if (child->get_any_name() == "module")
      if (child->get_attributes()["name"] == "")
        m[child] = 1;
      else
        rearrangeRootModules(child, root, m);
  if (n == root) {
    array a = indices(m);
    foreach (a, Node module) {
      Node parent = module->get_parent();
      if (parent)
        parent->remove_child(module);
    }
    foreach (a, Node module)
      mergeTrees(root, module);
  }
}

// Use this function on the lib roots
Node rootModuleFromDir(string directory) {
  Node root = moduleFromDir("", directory);
  rearrangeRootModules(root, root);
  return root;
}

//========================================================================
// HANDLING @appears directives
//========================================================================

static Node findNode(Node root, array(string) ref) {
  Node n = root;
  // predef:: is just an anchor to the root
  if (sizeof(ref) && ref[0] == "predef")
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
  mapping attr = current->get_attributes();
  if (attr["appears"] || attr["belongs"] || attr["global"]) {
    ReOrganizeTask t = ReOrganizeTask();
    if (string attrRef = attr["appears"]) {
      array(string) a = splitRef(attrRef);
      t->belongsRef = a[0 .. sizeof(a) - 2];
      t->newName = a[-1];
    }
    else if (string belongsRef = attr["belongs"]) {
      array(string) a = splitRef(attrRef);
      t->belongsRef = a;
    }
    else if (string globalRef = attr["global"]) {
      t->belongsRef = ({});
    }
    t->n = current;
    tasks += ({ t });
  }
  if ( (<"class", "module">)[ current->get_any_name() ] )
    foreach (current->get_children(), Node child)
      if ((<"module", "class", "docgroup">)[child->get_any_name()])
        recurseAppears(root, child);
}

// Tke care of all the @appears and @belongs directives everywhere,
// and rearrange the nodes in the tree accordingly
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
    if (!belongsNode)
      werror("couldn't find the node: %O\n", belongsRef);
    if (type == "docgroup") {
      if (newName)
        foreach (n->get_children(), Node child)
          if (child->get_attributes()["name"])
            // this ought to happen only once in this loop...
            child->get_attributes()["name"] = newName;
    }
    else if (type == "class" || type == "module") {
      if (newName)
        n->get_attributes()["name"] = newName;
    }
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
static array(string) splitRef(string ref) {
  array(string) a = Parser.Pike.split(ref);
  array result = ({});
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
    if ((<".", "->", "::">)[token])
      a = a[1..];
    else
      return result;
  }
}

static class Scope(string|void type, string|void name) {
  multiset(string) idents = (<>);
}

static class ScopeStack {
  /*static*/ array(Scope) scopeArr = ({ 0 }); // sentinel

  void enter(string|void type, string|void name) {
    //werror("entering scope type(%O), name(%O)\n", type, name);
    scopeArr = ({ Scope(type, name) }) + scopeArr;
  }
  void leave() { scopeArr = scopeArr[1..]; }

  void addName(string sym) { scopeArr[0]->idents[sym] = 1; }

  mapping resolveRef(string ref) {
    //werror("[[[[ resolving %O\n", ref);
    array(string) idents = splitRef(ref);
    //werror("[[[[ firstIdent == %O\n", firstIdent);
    if (!sizeof(idents))
      ref = "";
    else if (idents[0] == "predef")
      // predef:: is an anchor to the root.
      ref = idents[1..] * ".";
    else {
      ref = idents * ".";
      string firstIdent = idents[0];
      for(int i = 0; ; ++i) {
        Scope s = scopeArr[i];
        if (!s)
          break;  // end of array
        if (s->idents[firstIdent])
          if (s->type == "params") {
            return ([ "param" : ref ]);
          }
          else {
            //werror("[[[[ found in type(%O) name(%O)\n", s->type, s->name);
            string res = "";
            // work our way from the bottom of the stack
            for (int j = sizeof(scopeArr) - 1; j >= i; --j) {
              string name = 0;
              if (scopeArr[j])
                name = scopeArr[j]->name;
              if (name && name != "")
                res += name + ".";
              //werror("[[[[ name == %O\n", name);
            }
            return ([ "resolved" : res + ref ]);
          }
        if (s->type == "module")
          break; // reached past the innermost module...
      }
    }
    // TODO: should we check that the symbol really DOES appear
    // on the top level too?
    return ([ "resolved" : ref ]); // we consider it to be an absolute reference
  }
}

static void fixupRefs(ScopeStack scopes, Node node) {
  node->walk_preorder(
    lambda(Node n) {
      // More cases than just "ref" ??
      // Add them here if we decide that also references in e.g.
      // object types should be resolved.
      if (n->get_any_name() == "ref") {
        mapping m = n->get_attributes();
        if (m["resolved"])
          return;
        string ref = n->value_of_node();
        //werror("いい resolving reference %O\n", ref);
        //foreach (scopes->scopeArr, Scope s)
        //  werror("いい    %O\n", s ? s->name : "NULL");
        mapping resolved = scopes->resolveRef(n->value_of_node());
        //werror("いい resolved to: %O\n", resolved);
        //werror("いい m == %O\n", m);
        foreach (indices(resolved), string i)
          m[i] = resolved[i];
      }
    }
  );
}

// expects a <module> or <class> node
static void resolveFun(ScopeStack scopes, Node node) {
  // first collect the names of all things inside the scope
  foreach (node->get_children(), Node child)
    switch (child->get_any_name()) {
      case "docgroup":
        // add the names of all things documented
        // in the docgroup.
        foreach (child->get_children(), Node thing) {
          string name = thing->get_attributes()["name"];
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
  foreach (node->get_children(), Node child) {
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
        scopes->enter("params");
        {
          Node doc = 0;
          foreach (child->get_children(), Node n)
            if (n->get_any_name() == "doc")
              doc = n;
            else if (n->get_any_name() == "method") {
              foreach (filter(n->get_children(),
                              lambda (Node n) {
                                return n->get_any_name() == "arguments";
                              }), Node m)
                foreach (m->get_children(), Node argnode)
                  scopes->addName(argnode->get_attributes()["name"]);
            }
          fixupRefs(scopes, doc);
        }
        scopes->leave();
        break;
      case "doc":  // doc for the <class>/<module> itself
        fixupRefs(scopes, child);
        break;
      default:
        ; // do nothing
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
