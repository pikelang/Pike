// This module is the most high-level.
// Some functions to apply to the XML tree after extraction, too, for
// reference resolving etc.

#include "./debug.h"

static inherit Parser.XML.Tree;
static inherit "module.pmod";

#define DEB werror("###%s:%d\n", __FILE__, __LINE__);
static private void processError(string message, mixed ... args) {
  throw ( ({ AutoDocError(0, "ProcessXML", sprintf(message, @args)),
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
  werror("extracting file %O\n", filename);
  // extract the file...
  // check if there are C style doc comments in it,
  // because if there are, the CExtractor should be
  // used instead of the PikeExtractor
  int styleC = !pikeMode;
  int stylePike = pikeMode;

  string contents = Stdio.File(filename)->read();

  if (pikeMode) {
    if (search("/*!", contents)) {
      array(string) a = Parser.Pike.split(contents);
      // check if it _really_ is a C-style doc comment ...
      foreach (a, string s)
        if (strlen(s) >= strlen("/*!*/") && s[0..2] == "/*!")
          styleC = 1;
        else if (isDocComment(s))
          stylePike = 1;
      if (stylePike && styleC)
        processError("both C and Pike style doc comments in file " + filename);
    }
  }
  if (styleC) {
    object m = .CExtractor.extract(contents, filename);
    return m->xml();
  }
  else {
    object m = (type == "module")
      ? .PikeExtractor.extractModule(contents, filename, name)
      : .PikeExtractor.extractClass(contents, filename, name);
    return makeWrapper(parentModules, m)->xml();
  }
}

//========================================================================
// IMAGES MOVED TO CANONICAL FILES
//========================================================================

// middot
#define CONCAT_CHAR "·"

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
	      parents += ({ CONCAT_CHAR + "APPENDIX" + hash(attr->name) });
	    break;
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

// Take care of all the @appears and @belongs directives everywhere,
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
    if (!belongsNode) {
      werror("couldn't find the node: %O\n", belongsRef);
      continue;
    }
    if (type == "docgroup") {
      if (newName)
        foreach (n->get_children(), Node child)
          if (child->get_node_type() == XML_ELEMENT) {
            mapping attributes = child->get_attributes();
            if (attributes["name"])
              // this ought to happen only once in this loop...
              attributes["name"] = newName;
          }
    }
    else if (type == "class" || type == "module") {
      if (newName)
        n->get_attributes()["name"] = newName;
    }

    m_delete(n->get_attributes(), "belongs");

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
    int not_param = has_suffix(ref, "()");
    if (!sizeof(idents))
      ref = "";
    else {
      if (idents[0] == "top::")
        // top:: is an anchor to the root.
        ref = mergeRef(idents[1..]);
      else
        ref = mergeRef(idents);
      string firstIdent = idents[0];
      for(int i = 0; ; ++i) {
        Scope s = scopeArr[i];
        if (!s)
          break;  // end of array
        if (s->idents[firstIdent])
          if (s->type == "params" && !not_param) {
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
      if (n->get_node_type() == XML_ELEMENT) {
        // More cases than just "ref" ??
        // Add them here if we decide that also references in e.g.
        // object types should be resolved.
        if (n->get_any_name() == "ref") {
          mapping m = n->get_attributes();
          if (m["resolved"])
            return;
          //werror("¤¤¤¤ resolving reference %O\n", ref);
          //foreach (scopes->scopeArr, Scope s)
          //  werror("¤¤¤¤    %O\n", s ? s->name : "NULL");
          mapping resolved = scopes->resolveRef(n->value_of_node());
          //werror("¤¤¤¤ resolved to: %O\n", resolved);
          //werror("¤¤¤¤ m == %O\n", m);
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
                  foreach (m->get_children(), Node argnode) {
		    if(argnode->get_node_type() != XML_ELEMENT)
		      continue;
                    scopes->addName(argnode->get_attributes()["name"]);
		  }
              }
	    if(doc)
	      fixupRefs(scopes, doc);
	    else
	      werror("No doc element found\n%s\n\n", child->render_xml());
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
}

// Resolves references, regarding the Node 'tree' as the root of the whole
// hierarchy. 'tree' might be the node <root> or <module name="Pike">
void resolveRefs(Node tree) {
  ScopeStack scopes = ScopeStack();
  scopes->enter("module", "");    // The top level scope
  resolveFun(scopes, tree);
  scopes->leave();
}

// Call this method after the extraction and merge of the tree.
void postProcess(Node tree) {
  handleAppears(tree);
  resolveRefs(tree);
}


//========================================================================
// SPLITTING THE BIG TREE INTO DIRECTORIES AND FILES
//========================================================================

// Structure description file format:
//
// <dir name="manual">
//   <dir name="Process">
//     <file name="create_process.xml">
//       <!-- Only create and limit_value på den här sidan,
//       in that order -->
//       <target module="Process" class="create_process" entity="create"/>
//       <target module="Process" class="create_process" entity="limit_value"/>
//     </file>
//   </dir>
//   <dir name="Protocols">
//     <file name="HTTP.xml">
//       <!-- All in Protocols.HTTP, document order. -->
//       <target module="Protocols.HTTP"/>
//     </file>
//   </dir>
//   <dir name="Standards">
//     <file name="URI.xml">
//       <!-- All in Standards.URI, document order. -->
//       <target module="Standards" class="URI"/>
//     </file>
//   </dir>
// </dir>

static class Target {
  string file = 0;     // relative to the virtual root
  Node data = 0;       // the XML to insert in the file
  string _sprintf() {
    return sprintf("Target(%O, <%s>)",
                   file, data ? data->get_any_name() || "" : "NULL");
  }
}

static int splitError(string s, mixed ... args) {
  s = sprintf(s, @args);
  throw(AutoDocError(0, "ProcessXML/splitting", s));
}

//! Takes an autodoc-XML string, an autodoc structure file and a
//! directory name and splits the first according to the second into
//! the third.
//! @param docXMLFile
//!   The contents of the autodoc XML blob to be split up
//! @param structureXMLFile
//!   The structure description for the file tree to be created in
//!   @[rootDir]
//! @param rootDir
//!   The name of the directory that maps to the <dir> element on the
//!   top level of @[structureXMLFile]
void splitIntoDirHier(string docXMLFile, string structureXMLFile,
                      string rootDir)
{
  Node doc = parse_input(docXMLFile)[0];
  Node struc = parse_input(structureXMLFile);

  // First we find and all targets

  // If the value is a string, it's just the href to the file, because the
  // entity was just moved together with something in its docgroup
  mapping(string : Target | string) targets = ([]);

  array(string) dirs = ({});
  string fileName = 0;
  struc->walk_preorder_2(
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT) {
        mapping attr = n->get_attributes();
        switch(n->get_any_name()) {
          case "target":
            {

            if (!sizeof(indices(attr)))
              splitError("No attribute in <target> tag");
            if (sizeof(indices(attr) - ({ "module","class","entity" })))
              splitError("Bad attribute in <target> tag");

            array(string) ids =
              (attr["module"] || "") / "." +
              (attr["class"] || "") / "." +
              (attr["entity"] || "") / "." - ({ "" });

            Target t = Target();

            targets[ids * "."] = t;
            t->file = dirs * "/" + "/" +
              (fileName || splitError("<target> must be inside a <file>"));
            attr["full_ref"] = ids * ".";
            break;
            }
          case "file":
            attr["full_dir"] = rootDir + "/" + dirs * "/";
            fileName = attr["name"]
              || splitError("<file> must have a name attribute");
            break;
          case "dir":
            if (!attr->name)
	      throw( ({ "Dir element is missing name attribute.\n", backtrace() }) );
              dirs += ({ attr->name });
            break;
          default:
            throw( ({ "Found unknown structure tag: <" + n->get_any_name() +
		      ">\n", backtrace() }) );
        }
      }
    },
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT)
        switch (n->get_any_name()) {
          case "file":
            fileName = 0;
            break;
          case "dir":
	    dirs = dirs[0..sizeof(dirs) - 2];
            break;
        }
    }
  );

  // 2. For all targets, beginning with the most specific (most '.'s)
  //   2.1 Remove the corresponding class/docgroup from the doc tree
  //   2.2 Wrap it in placeholder class/modules
  //   2.3 Put it in the Target.data member
  array(string) sortedTargets =
    Array.sort_array(indices(targets),
                     lambda(string s1, string s2) {
                       return (strlen(s1) && sizeof(s1 / "."))
                         < (strlen(s2) && sizeof(s2 / "."));
                     }
                    );

  foreach(sortedTargets, string target) {
    object(Target)|string t = targets[target];

    if (!objectp(t))
      continue; // It was just a "moved along" entry in a docgroup!

    array(string) ref = target / "." - ({ "" });
    Node n = findStuff(doc, ref);
    if (!n)
      splitError("unable to find %O in the doc tree", target);

    // Remove it from the <class> or <module> that it belongs to,
    // replacing it with a placeholder.
    Node parent = n->get_parent();
    if (parent) {
      mapping nattr = n->get_attributes();
      mapping(string : string) attr = ([
        "type" : "inner-placeholder",
        "href" : t->file,
      ]);
      if (nattr["name"])
        attr["name"] = nattr["name"];
      Node placeHolder = Node(XML_ELEMENT, n->get_any_name(), attr, 0);
      if (n->get_any_name() == "docgroup") {
        // put some placeholder stuff inside it too...
        foreach (n->get_children(), Node child)
          if (child->get_node_type() == XML_ELEMENT && child->get_any_name() != "doc") {
            // Reparse it to get a COPY of it ...
            Node new = parse_input(child->html_of_node())[0];
            mapping attr = new->get_attributes() || ([]);
            // Should we really add it?
            attr["type"] = "inner-placeholder";
            placeHolder->add_child(new);
          }
      }
      parent->replace_child(n, placeHolder);
    }

    if (n->get_any_name() == "docgroup") {
      string container = sizeof(ref) > 1 ? ref[0 .. sizeof(ref) - 2] * "." + "." : "";
      foreach (n->get_children(), Node child)
        if (child->get_node_type() == XML_ELEMENT && child->get_any_name() != "doc") {
          string name = (child->get_attributes() || ([])) ["name"];
          if (name && name != ref[-1])
            if (targets[container + name])
              splitError("attempt to split docgroup of %s", container + name);
            else
              // Setting it to a string:
              targets[container + name] = t->file;
        }
    }

    // Traverse it, resolving all <ref> hrefs
    n->walk_preorder(
      lambda(Node n) {
        if (n->get_any_name() == "ref" && n->get_node_type() == XML_ELEMENT) {
          mapping(string : string) attr = n->get_attributes() || ([]);
          if (attr["resolved"]) {
            string href = findHref(targets, attr["resolved"] / ".");
            if (!href)
              werror("unable to find href to %O\n", attr["resolved"]);
            else
              attr["href"] = href;
          }
        }
      }
    );

    // Get all the outer placeholders...
    array(string) parentClasses = ({});
    array(string) parentModules = ({});
    while (parent) {
      string tag = parent->get_any_name();
      if (tag == "class" || tag == "module") {
        string name = (parent->get_attributes() || ([]))["name"] || "";
        if (tag == "class")
          parentClasses = ({ name }) + parentClasses;
        else
          parentModules = ({ name }) + parentModules;
      }
      parent = parent->get_parent();
    }

    // Wrap the stuff in outer placeholder classes and modules
    array(string) allParents = parentModules + parentClasses;

    for (int i = sizeof(allParents) - 1; i >= 0; --i) {
      mapping attr = ([]);

      string href = findHref(targets, allParents[0 .. i]);

      if (href)
        attr["href"] = href;
      else if (i)
        werror("unable to find href to %O", mergeRef(allParents[0 .. i]));

      attr["name"] = allParents[i];
      attr["type"] = "outer-placeholder";
      Node newNode =
        Node(XML_ELEMENT,
             i < sizeof(parentModules) ? "module" : "class",
             attr, 0);
      newNode->add_child(n);
      n = newNode;
    }
    t->data = n;
  }

  // Traverse the tree again, creating the dirs, and writing the
  // targets to the files.
  string path;
  Stdio.File file = Stdio.File();
  struc->walk_preorder_2(
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT) {
        mapping(string : string) attr = n->get_attributes();
        switch (n->get_any_name()) {
          case "file":
            {
            string dir = attr["full_dir"];
            if (!Stdio.mkdirhier(dir))
              splitError("unable to create directory %O", dir);
            path = dir + "/" + attr["name"];
            if (!file->open(path, "cwt"))
              splitError("unable to open file %O for write access", path);
            }
            break;
          case "target":
            {
            string target = attr["full_ref"];
            object(Target)|string t = targets[target];
            if (!objectp(t))
              splitError("THIS SHOULDN'T HAPPEN!");
            string xml = t->data->html_of_node();
            if (file->write(xml) != strlen(xml))
              splitError("unable to write to file %O", path);
            }
            break;
        }
      }
    },
    lambda(Node n) {
      if (n->get_node_type() == XML_ELEMENT && n->get_any_name() == "file")
        if (!file->close())
          splitError("unable to close file %O", path);
    }
  );

  if (!targets[""]) { // The root module has no file of its own ...
    // Then make sure that every top level module has been put somewhere.
    foreach(doc->get_children(), Node child)
      if (child->get_node_type() == XML_ELEMENT) {
        mapping attr = child->get_attributes() || ([]);
        if ((attr["type"] || "") == "inner-placeholder")
          continue;
        string tag = child->get_any_name();
        array(string) forgotten = ({ });

        if (tag == "docgroup") {
          if (attr["homogen-name"])
            forgotten = ({ attr["homogen-name"] });
          else
            foreach(child->get_children(), Node stuff) {
              string name;
              if (stuff->get_node_type() == XML_ELEMENT
                  && (name = (stuff->get_attributes() || ([]))["name"]))
                forgotten += ({ name });
            }
        }
        else if (attr["name"])
          forgotten = ({ attr["name"] });

        werror("WARNING: No <target> for %s\n",
               String.implode_nicely(forgotten));
      }
  }
}

// Find the file where the entity will have its documentation.
static string findHref(mapping(string : Target|string) targets, array(string) ids) {
  for (;;) {
    object(Target)|string t = targets[mergeRef(ids)];
    if (t) {
      //      SHOW(stringp(t) ? t : t->file);
      return stringp(t) ? t : t->file;
    }

    if (sizeof(ids) <= 1)
      break;
    ids = ids[0 .. sizeof(ids) - 2];
  }
  return 0;
}

// If ref points to a class or module, return that <class> / <module>,
// else the <docgroup> that contains the thing...
static Node findStuff(Node root, array(string) ref) {
  Node n = root;
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
    if (!found && sizeof(ref) == 1)
      outerloop: foreach (children, Node child)
        if (child->get_any_name() == "docgroup")
          foreach (child->get_children(), Node thing)
            if ((thing->get_attributes() || ([]))["name"] == ref[0])
              if (child->get_attributes()["type"] == "inner-placeholder")
                splitError("%s is in the same docgroup as another target, cannot", ref * ".");
              else {
                found = child;
                break outerloop;
              }
    if (!found)
      return 0;
    n = found;
    ref = ref[1..];
  }
  return n;
}
