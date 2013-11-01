#pike __REAL_VERSION__

// This module is the most high-level.
// Some functions to apply to the XML tree after extraction, too, for
// reference resolving etc.

#include "./debug.h"

protected inherit Parser.XML.Tree;
protected inherit "module.pmod";

#define DEB werror("###%s:%d\n", __FILE__, __LINE__);
protected private void processError(string message, mixed ... args) {
  throw ( ({ sprintf("ProcessXML: "+message+"\n", @args),
	     backtrace() }) );
}

protected private void processWarning(string message, mixed ... args) {
  werror("ProcessXML: "+message+"\n", @args);
}

// XML format:
//
// <autodoc>
//   <namespace/>
//   <appendix/> <!-- Note: Only in compat mode! -->
//   <doc/>?
// </autodoc>
//
// <namespace>
//   <module/>
//   <class/>
//   <enum/>
//   <docgroup/>
//   <doc/>
// </namespace>
//
// <module>
//   <module/>
//   <class/>
//   <enum/>
//   <docgroup/>
//   <doc/>
// </module>
//
// <class>
//   <modifiers/>
//   <class/>
//   <enum/>
//   <docgroup/>
//   <doc/>
// </class>
//
// <enum>
//   <docgroup/>?
//   <doc/>
// </enum>
//
// <modifiers>
//   <local/>
//   <optional/>
//   <private/>
//   <static/>
// </modifiers>
//
// <docgroup>
//   <method/>
//   <variable/>
//   <constant/>
//   <typedef/>
//   <inherit/>
//   <directive/>
//   <doc/>
// </docgroup>
//
// <doc>
//   <group/>
//   <text/>
// </doc>
//
// <group>
//   <bugs/>
//   <deprecated/>
//   <elem/>
//   <example/>
//   <fixme/>
//   <index/>
//   <item/>
//   <member/>
//   <note/>
//   <param/>
//   <returns/>
//   <seealso/>
//   <text/>
//   <throws/>
//   <type/>
//   <value/>
// </group>
//
// <text>
//   <array/>
//   <br/>
//   <dl/>
//   <int/>
//   <mapping/>
//   <mixed/>
//   <multiset/>
//   <ol/>
//   <p/>
//   <ref/>
//   <section/>
//   <string/>
//   <ul/>
// </text>

//========================================================================
// From source file to XML
//========================================================================

// Wrap child in the proper modules and namespace.
protected object makeWrapper(array(string) modules, object|void child)
{
  object m;
  if (child->objtype != "autodoc") {
    if (child->objtype != "namespace") {
      string namespace = "predef";	// Default namespace.
      if (sizeof(modules) && has_suffix(modules[0], "::")) {
	// The parent module list starts with a namespace.
	namespace = modules[0][..<2];
	modules = modules[1..];
      }
      foreach(reverse(modules), string n) {
	m = .PikeObjects.Module();
	m->name = n;
	if (child)
	  m->AddChild(child);
	child = m;
      }
      m = .PikeObjects.NameSpace();
      m->name = namespace;
      m->AddChild(child);
      child = m;
    }
    m = .PikeObjects.AutoDoc();
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
//!   @expr{"class"@}, @expr{"module"@} or @expr{"namespace"@}.
//! @param name
//!   The name of the class/module/namespace.
//! @param parentModules
//!   The ancestors of the class/module/namespace.
//! @param flags
//!   Flags adjusting the extractor behaviour.
//!   Defaults to @[FLAG_NORMAL].
//!
//! @example
//!   // To extract doc for Foo.Bar.Ippa:
//!   string xml = extractXML("lib/modules/Foo.pmod/Bar.pmod/Ippa.pike", 1,
//!     "class", "Ippa", ({ "Foo", "Bar" }));
//!
string extractXML(string filename, int|void pikeMode, string|void type,
                  string|void name, array(string)|void parentModules,
		  void|.Flags flags)
{
  if (zero_type(flags)) flags = .FLAG_NORMAL;

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
  //      if (sizeof(s) >= sizeof("/*!*/") && s[0..2] == "/*!")
  //	styleC = 1;
  //      else if (isDocComment(s))
  //	stylePike = 1;
  //    if (stylePike && styleC)
  //      processError("both C and Pike style doc comments in file " + filename);
  //  }

  if (styleC && has_value(contents, "/*!")) {
    string namespace = ((parentModules||({})) + ({"predef::"}))[0];
    if (has_suffix(namespace, "::")) {
      namespace = namespace[..<2];
    }
    object m = .CExtractor.extract(contents, filename, namespace, flags);
    return m->xml(flags);
  }
  else if(stylePike && has_value(contents, "//!")) {
    if(has_suffix(filename, ".pmod.in")) {
      contents = replace(contents, "@module@",
			 "\"___" + (parentModules[1..] + ({ name }))*"." +"\"");
    }
    object m;
    switch(type) {
    case "module":
      m = .PikeExtractor.extractModule(contents, filename, name, flags);
      break;
    default:
    case "class":
      m = .PikeExtractor.extractClass(contents, filename, name, flags);
      break;
    case "namespace":
      m = .PikeExtractor.extractNamespace(contents, filename, name, flags);
      break;
    }
    if (m)
      return makeWrapper(parentModules, m)->xml(flags);
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
//! @param quiet
//!   Quiet operation.
//! @returns
//!   The XML file contents (with decorated <image>-tags)
string moveImages(string docXMLFile,
                  string imageSourceDir,
                  string imageDestDir,
		  int|void quiet)
{
  if(!has_value(docXMLFile, "<image")) return docXMLFile;

  array(string) parents = ({});
  int counter = 0;
  array(int) docgroupcounters = ({}); // docgroup on top level is impossible
  string cwd = getcwd();
  SimpleNode n;
  mixed err = catch {
      n = simple_parse_input(docXMLFile)->get_first_element();
    };
  if (err) {
    int offset;
    if (sscanf(err[0], "%*s[Offset: %d]", offset) == 2) {
      werror("XMLError: %O#%O\n",
	     docXMLFile[offset-20..offset-1],
	     docXMLFile[offset..offset+20]);
    }
    throw(err);
  }
  n->walk_preorder_2(
    lambda(SimpleNode n) {
      if (n->get_node_type() == XML_ELEMENT) {
        mapping(string : string) attr = n->get_attributes();
        switch (n->get_any_name()) {

          case "docgroup":
            ++docgroupcounters[-1];
            if (attr["homogen-name"])
              parents += ({ attr["homogen-name"] });
            else {
              string name = 0;
              foreach (n->get_children(), SimpleNode c) {
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

	  case "namespace":
          case "class":
          case "module":
            if (attr["name"] != "")
              parents += ({ attr["name"] });
	    // FALL_THROUGH
	  case "autodoc":
            counter = 0;
            docgroupcounters += ({ 0 });
            break;

          case "image":
            string imageFilename;
            array(SimpleNode) children = n->get_children();
            if (sizeof(children || ({})) != 1
                || children[0]->get_node_type() != XML_TEXT) {
	      if (!(imageFilename = n->get_attributes()["src"]))
		processError("bad image tag: %s\n", n->html_of_node());
	      processWarning("Invalid image tag: %s\n", n->html_of_node());
	    } else
	      imageFilename = children[0]->get_text();
            imageFilename = combine_path(imageSourceDir, imageFilename);
            imageFilename = combine_path(cwd, imageFilename);
            string formatExt = (basename(imageFilename) / ".")[-1];
            string canonicalName =
              (parents + ({ (string) ++counter })) * CONCAT_CHAR;
            string destFilename = imageDestDir + "/" + canonicalName
              + "." + formatExt;

	    mapping args = n->get_attributes();
	    if (!quiet)
	      werror("copying from [%s] to [%s]\n",
		     imageFilename, destFilename);
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
    lambda(SimpleNode n) {
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
  SimpleRootNode root = SimpleRootNode();
  root->replace_children(({ SimpleHeaderNode(([ "version":"1.0",
						"encoding":"utf-8",
					     ])),
			    SimpleTextNode("\n"),
			    n,
			    SimpleTextNode("\n"),
			 }));
  return root->html_of_node();
}

//========================================================================
// BUILDING OF THE BIG TREE
//========================================================================

protected int isAutoDoc(SimpleNode node) { return node->get_any_name() == "autodoc"; }
protected int isNameSpace(SimpleNode node) { return node->get_any_name() == "namespace"; }
protected int isClass(SimpleNode node) { return node->get_any_name() == "class"; }
protected int isModule(SimpleNode node) { return node->get_any_name() == "module"; }
protected int isInherit(SimpleNode node) { return node->get_any_name() == "inherit"; }
protected int isImport(SimpleNode node) { return node->get_any_name() == "import"; }
protected int isDoc(SimpleNode node) { return node->get_any_name() == "doc"; }
protected int isDocGroup(SimpleNode node) { return node->get_any_name() == "docgroup"; }
protected int isModifiers(SimpleNode node) { return node->get_any_name() == "modifiers"; }

protected string getName(SimpleNode node) {
  string name = node->get_attributes() && node->get_attributes()["name"];
  if (name) {
    if (isInherit(node) || isImport(node)) {
      // Separate name-space for inherits and imports.
      return node->get_any_name() + ":" + name;
    }
  }
  return name;
}

protected int isText(SimpleNode node) { return node->get_node_type() == XML_TEXT; }

// This function is just used to get something suitable to use
// for sorting multiple docgroups for the same symbol.
protected string renderType(SimpleNode node)
{
  SimpleNode type_node;
  if ((< "method", "variable", "constant", "directive",
	 "typedef", "inherit", "import" >)[node->get_any_name()]) {
    type_node = node;
  } else {
    type_node =
      node->get_first_element("method") ||
      node->get_first_element("variable") ||
      node->get_first_element("constant") ||
      node->get_first_element("directive") ||
      node->get_first_element("typedef") ||
      node->get_first_element("inherit") ||
      node->get_first_element("import");
    if (!type_node) {
      werror("Unknown type_node: %O\n", node->render_xml());
      type_node = node;
    }
  }
  return type_node->render_xml();
}

protected SimpleNode mergeDoc(SimpleNode orig, SimpleNode new)
{
  // NB: There can only be one <text> node in a <doc> node,
  //     and it comes first among the element nodes.
  SimpleNode orig_text = orig->get_first_element("text");
  SimpleNode new_text = new->get_first_element("text");
  if (new_text && sizeof(String.trim_all_whites(new_text->value_of_node()))) {
    if (!orig_text) {
      orig->add_child(new_text);
    } else if (!sizeof(String.trim_all_whites(orig_text->value_of_node()))) {
      orig->replace_child(orig_text, new_text);
    } else {
      orig_text->replace_children(orig_text->get_children() +
				  new_text->get_children());
    }
  }

  // Append the remaining (typically <group>) nodes in new after
  // the previously existing in orig.
  array(SimpleNode) new_children = new->get_children() - ({ new_text });
  orig->replace_children(orig->get_children() + new_children);
}

//!   Puts all children of @[source] into the tree @[dest], in their
//!   correct place module-hierarchically.
//!
//!   Used to merge the results of extractions of different Pike and C files.
//!
//!   Some minor effort is expended to normalize the result to some
//!   sort of canonical order.
//!
//! @param source
//! @param dest
//!   The nodes @[source] and @[dest] are @tt{<class>@}, @tt{<module>@},
//!   @tt{<namespace>@} or @tt{<autodoc>@} nodes that are identical in
//!   the sense that they represent the same module, class or namespace.
//!   Typically they both represent @tt{<autodoc>@} nodes.
//!
//! @note
//!   Both @[source] and @[dest] are modified destructively.
//!
//! @note
//!   After calling this method, any @tt{<class>@} or @tt{<module>@} nodes
//!   that have been marked with @@appears or @@belongs, are still in the
//!   wrong place in the tree, so @[handleAppears()] (or @[postProcess()])
//!   must be called on the whole documentation tree to relocate them once
//!   the tree has been fully merged.
void mergeTrees(SimpleNode dest, SimpleNode source)
{
  mapping(string : SimpleNode) dest_children = ([]);
  mapping(string : array(SimpleNode)) dest_groups = ([]);
  SimpleNode dest_has_doc;
  multiset(string) dest_modifiers = (<>);
  array(SimpleNode) other_children = ({});

  if ((dest->get_node_type() == Parser.XML.Tree.XML_ROOT) ||
      (source->get_node_type() == Parser.XML.Tree.XML_ROOT)) {
    error("mergeTrees() MUST be called with element nodes.\n");
  }

  // Transfer any attributes (like eg appears).
  mapping(string:string) dest_attrs = dest->get_attributes();
  foreach(source->get_attributes(); string attr; string val) {
    if ((dest_attrs[attr] || val) != val) {
      processWarning("Attribute '" + attr +
		     "' ('" + dest_attrs[attr] + "') for node " +
		     getName(dest) +
		     "differs from the same for node " +
		     getName(source) +
		     " ('" + val + "').");
    } else {
      dest_attrs[attr] = val;
    }
  }

  foreach(dest->get_children(), SimpleNode node) {
    string name = getName(node);
    if (name) {
      dest_children[name] = node;
    } else if (isDoc(node)) {
      // Strip empty doc nodes.
      if (sizeof(String.trim_all_whites(node->value_of_node())))
	dest_has_doc = node;
    } else if (isDocGroup(node)) {
      // Docgroups are special:
      //  * More than one symbol my be documented in the group.
      //  * The same symbol may be documented multiple times
      //    with different signatures.
      foreach(node->get_elements(), SimpleNode sub) {
	if (name = getName(sub)) {
	  dest_groups[name] += ({ node });
	}
      }
    } else if (isModifiers(node)) {
      dest_modifiers = (multiset(string))node->get_elements()->get_any_name();
    } else if (!isText(node) ||
	       sizeof(String.trim_all_whites(node->value_of_node()))) {
      other_children += ({ node });
    }
  }

  array(SimpleNode) children = source->get_elements();
  foreach(children; int i; SimpleNode node) {
    switch(node->get_any_name()) {
      case "appendix":
	// FIXME: Warn when not in compat mode.
	// FALL_THROUGH
      case "namespace":
      case "class":
      case "module":
      case "enum":
        {
	  string name = getName(node);

	  if (!name) {
	    processError("No name for %s:\n"
			 "%O\n",
			 node->get_any_name(), node->render_xml()[..256]);
	  }

	  SimpleNode n = dest_children[name];
	  if (n) {
	    if (node->get_any_name() != n->get_any_name()) {
	      if (!(<"module", "class">)[n->get_any_name()] ||
		  !(<"module", "class">)[node->get_any_name()]) {
		processError("entity '" + name +
			     "' used both as a " + node->get_any_name() +
			     " and a " + n->get_any_name() + ".");
	      }
	      processWarning("entity '" + name +
			     "' used both as a " + node->get_any_name() +
			     " and a " + n->get_any_name() + ".");
	      processWarning("Converting to 'class'.");
	      n->set_tag_name("class");
	      node->set_tag_name("class");
	    }
	  } else {
	    // Create a dummy node to merge with, so that we can normalize.
	    n = SimpleElementNode(node->get_any_name(),
				  node->get_attributes());
	    dest_children[name] = n;
	  }
	  mergeTrees(n, node);
        }
	children[i] = 0;
        break;

      case "docgroup":
	{
	  foreach(node->get_elements(), SimpleNode sub) {
	    string name = getName(sub);
	    if (!name) continue;
	    dest_groups[name] += ({ node });
	  }
	  SimpleNode doc = node->get_first_element("doc");
	  if (doc && !sizeof(String.trim_all_whites(doc->value_of_node()))) {
	    // The doc is NULL.
	    node->remove_child(doc);
	  }
        }
	children[i] = 0;
        break;

      case "modifiers":
        dest_modifiers |= (multiset)node->get_elements()->get_any_name();
        break;

      case "doc":
	if (!sizeof(String.trim_all_whites(node->value_of_node()))) {
	  // NULL doc.
	} else if (dest_has_doc) {
	  if ((node->get_attributes()["placeholder"] == "true") ||
	      (String.trim_all_whites(node->value_of_node()) ==
	       String.trim_all_whites(dest_has_doc->value_of_node()))) {
	    // New doc is placeholder or same as old.
	  } else if (dest_has_doc->get_attributes()["placeholder"] != "true") {
	    // werror("Original doc: %O\n", dest_has_doc->value_of_node());
	    // werror("New doc: %O\n", node->value_of_node());
	    if (isNameSpace(dest))
	      processWarning("Duplicate documentation for namespace " +
			     getName(dest));
	    else if (isClass(dest))
	      processWarning("Duplicate documentation for class " +
			     getName(dest));
	    else if (isModule(dest))
	      processWarning("Duplicate documentation for module " +
			     getName(dest));
	    else
	      processWarning("Duplicate documentation");
	    // Attempt to merge the two.
	    mergeDoc(dest_has_doc, node);
	  } else {
	    // Old doc was placeholder or empty.
	    dest_has_doc = node;
	  }
        } else {
	  dest_has_doc = node;
	}
	children[i] = 0;
	break;

      default:
	if (!isText(node) ||
	    sizeof(String.trim_all_whites(node->value_of_node()))) {
	  if (node->get_any_name() != "source-position") {
	    werror("Got other node: %O\n", node->render_xml());
	  }
	  other_children += ({ node });
	}
	children[i] = 0;
	break;
    }
  }

  source->replace_children(children - ({ 0 }));

  // Create a canonical order (and whitespace) for the children
  // of the dest node.
  children = ({});

  if (sizeof(dest_modifiers)) {
    SimpleElementNode modifiers = SimpleElementNode("modifiers", ([]));
    modifiers->replace_children(map(indices(dest_modifiers),
				    SimpleElementNode, ([])));
    children = ({ SimpleTextNode("\n"), modifiers });
  }

  if (dest_has_doc) {
    children += ({ SimpleTextNode("\n"), dest_has_doc });
  }

  multiset(SimpleNode) added_nodes = (<>);

  foreach(sort(indices(dest_groups)), string name) {
    array(SimpleNode) nodes = dest_groups[name];

    if (!nodes) continue;

    if (sizeof(nodes) > 1)
      sort(map(nodes, renderType), nodes);

    foreach(nodes, SimpleNode n) {
      if (added_nodes[n]) continue;

      children += ({ SimpleTextNode("\n"), n, });
      added_nodes[n] = 1;
    }
  }

  foreach(sort(indices(dest_children)), string name) {
    SimpleNode node = dest_children[name];

    if (!node) continue;

    children += ({ SimpleTextNode("\n"), node, });
  }

  // Handle any remaining nodes.
  if (sizeof(other_children)) {
    if (!isText(other_children[0]))
      children += ({ SimpleTextNode("\n") });
    children += other_children;
    if (!isText(other_children[-1]))
      children += ({ SimpleTextNode("\n") });
  } else
    children += ({ SimpleTextNode("\n") });

  dest->replace_children(children);
}

protected void reportError(string filename, mixed ... args) {
  werror("[%s]\t%s\n", filename, sprintf(@args));
}

//========================================================================
// HANDLING @appears directives
//========================================================================

protected SimpleNode findNode(SimpleNode root, array(string) ref) {
  SimpleNode n = root;
  // top:: is an anchor to the root of the current namespace.
  if (sizeof(ref) && ref[0] == "top::")
    ref = ref[1..];
  if (!sizeof(ref))
    return root;
  while (sizeof(ref)) {
    array(SimpleNode) children = n->get_children();
    SimpleNode found = 0;
    foreach (children, SimpleNode child) {
      string tag = child->get_any_name();
      if (tag == "namespace" || tag == "module" || tag == "class") {
        string name = child->get_attributes()["name"];
        if (name && name == ref[0]) {
          found = child;
          break;
        }
      }
    }
    if (!found) {
      if ((n == root) && (root->get_any_name() == "autodoc")) {
	// Create namespaces on demand.
	found = SimpleElementNode("namespace", ([ "name": ref[0] ]));
	root->add_child(found);
      } else {
	return 0;
      }
    }
    n = found;
    ref = ref[1..];
  }
  return n;
}

protected class ReOrganizeTask(SimpleNode n, SimpleNode parent) {
  array(string) belongsRef;
  string newName;
}

protected array(ReOrganizeTask) tasks;

// current is a <module>, <class> or <docgroup> node
protected void recurseAppears(string namespace,
			   SimpleNode current,
			   SimpleNode parent) {
  mapping attr = current->get_attributes() || ([]);
  if (attr["appears"] || attr["belongs"] || attr["global"]) {
    ReOrganizeTask t = ReOrganizeTask(current, parent);
    array(string) a = ({});
    if (string attrRef = attr["appears"]) {
      a = splitRef(attrRef);
      t->newName = a[-1];
      a = a[0 .. sizeof(a)-2];
    }
    else if (string belongsRef = attr["belongs"]) {
      a = splitRef(belongsRef);
    }
    if (sizeof(a) && has_suffix(a[0], "::")) {
      // Note: This assignment affects the default namespace
      //       for nodes under this one, but it seems a
      //       reasonable behaviour.
      if (a[0] != "top::")
	namespace = a[0];
    } else {
      // Prefix with the durrent namespace.
      a = ({ namespace }) + a;
    }
    // Strip the :: from the namespace name.
    a[0] = a[0][..<2];
    t->belongsRef = a;
    tasks += ({ t });
  }
  if ( (<"class", "module">)[ current->get_any_name() ] )
    foreach (current->get_children(), SimpleNode child)
      if ((<"module", "class", "docgroup">)[child->get_any_name()])
        recurseAppears(namespace, child, current);
}

//! Take care of all the @@appears and @@belongs directives everywhere,
//! and rearranges the nodes in the tree accordingly
//!
//! @param root
//!   The root (@tt{<autodoc>@}) node of the documentation tree.
void handleAppears(SimpleNode root, .Flags|void flags)
{
  if (zero_type(flags)) flags = .FLAG_NORMAL;
  tasks = ({ });
  foreach(root->get_elements("namespace"), SimpleNode namespaceNode) {
    string namespace = namespaceNode->get_attributes()->name + "::";
    foreach(namespaceNode->get_children(), SimpleNode child) {
      if ((<"module", "class", "docgroup">)[child->get_any_name()]) {
	recurseAppears(namespace, child, namespaceNode);
      }
    }
  }
  tasks = Array.sort_array(
    tasks,
    lambda (ReOrganizeTask task1, ReOrganizeTask task2) {
      return sizeof(task1->belongsRef) > sizeof(task2->belongsRef);
    } );
  foreach (tasks, ReOrganizeTask task) {
    SimpleNode n = task->n;
    string type = n->get_any_name();
    array(string) belongsRef = task->belongsRef;
    string newName = task->newName;
    SimpleNode belongsNode = findNode(root, belongsRef);
    if (!belongsNode) {
      if (flags & .FLAG_VERB_MASK)
	werror("Couldn't find the new parent node: %O for fragment:\n%s\n",
	       belongsRef*".", (string)n);
      continue;
    }
    if (type == "docgroup") {
      if (newName) {
	mapping m = n->get_attributes();
	if(m["homogen-name"])
	  m["homogen-name"] = newName;
        foreach (n->get_children(), SimpleNode child)
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

    task->parent->remove_child(n);

    // Perform a merge in case the destination already has some doc.
    if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE) {
      werror("Merging <%s> node %s:: with <%s> %s...\n",
	     belongsNode->get_any_name(), belongsRef*".",
	     n->get_any_name(), newName || "");
    }
    SimpleNode fakeBelongsNode =
      SimpleElementNode(belongsNode->get_any_name(), ([]))->add_child(n);
    mergeTrees(belongsNode, fakeBelongsNode);
  }
}

//========================================================================
// RESOLVING REFERENCES
//========================================================================

// Rather DWIM splitting of a string into a chain of identifiers:
// "Module.Class->funct(int i)" ==> ({"Module","Class","func"})
// "\"foo.pike\"" ==> ({ "\"foo.pike\"" })
// ".protocol" ==> ({ "", "protocol" })
// ".module.ANY" ==> ({ "", "ANY" })
protected array(string) splitRef(string ref) {
  if ((sizeof(ref)>1) && (ref[0] == '"')) {
    if ((ref == "\".\"") || (ref == "\"module.pmod\"")) {
      // Some special cases for referring to the current module.
      return ({ "" });
    }
    // Explictly named program.
    // Try to DWIM it...
    ref = ref[1..<1];
    ref = replace(ref, ({ ".pike", ".pmod" }), ({ "", "" }));
    // FIXME: What about module.pike/module.pmod?
    return ref/"/";
  }
  array(string) a = Parser.Pike.split(ref);
  string namespace;
  array result = ({});
  if (sizeof(a)) {
    // Check for a namespace.
    if (a[0] == "::") {
      namespace = "::";
      a = a[1..];
    } else if ((sizeof(a) > 1) && a[1] == "::") {
      namespace = a[0]+"::";
      a = a[2..];
    }
  }
  if (namespace) {
    result = ({ namespace });
  } else if (a[0] == ".") {
    result = ({ "" });
    a = a[1..];
    if (sizeof(a) > 1 && (a[0] == "module") && (a[1] == ".")) {
      // .module.XXXX
      a = a[2..];
    }
  }
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

protected string mergeRef(array(string) ref) {
  string s = "";
  if (sizeof(ref) && has_suffix(ref[0], "::")) {
    s = ref[0];
    ref = ref[1..];
  }
  s += (ref - ({ "" })) * ".";
  return s;
}

protected class Scope(string|void type, string|void name) {
  multiset(string) idents = (<>);

  multiset(string) failures = (<>);

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O:%O:%s)", this_program, type, name,
			     (sort(indices(idents))*",") );
  }
}

protected class ScopeStack {
  mapping(string:array(Scope)) scopes = ([]);
  mapping(string:multiset(string)) namespace_extends = ([]);
  string namespace = "predef";

  array(array(string|array(Scope))) namespaceStack = ({});

  void enter(string|void type, string|void name) {
    //werror("entering scope type(%O), name(%O)\n", type, name);
    if (type == "namespace") {
      namespaceStack += ({ ({ namespace, scopes[name] }) });
      scopes[namespace = name] = ({ Scope(type, name+"::") });
      if (name = ([ "7.8":"7.7",
		    "7.6":"7.5",
		    "7.4":"7.3",
		    "7.2":"7.1",
		    "7.0":"0.7" ])[name]) {
	// Add an alias for development version.
	scopes[name] = scopes[namespace];
      }
    } else {
      if (!sizeof(scopes[namespace]||({}))) {
	werror("WARNING: Implicit enter of namespace %s:: for %s %s\n",
	       namespace, type, name||"");
	scopes[namespace] = ({ Scope("namespace", namespace+"::") });
      }
      scopes[namespace] += ({ Scope(type, name) });
    }
  }

  void leave() {
    if (sizeof(scopes[namespace]||({}))) {
      if (sizeof(scopes[namespace][-1]->failures)) {
	werror("WARNING: Failed to resolv the following symbols in scope %O:\n"
	       "%{  %O\n%}\n",
	       namespace + "::" + scopes[namespace][1..]->name * ".",
	       indices(scopes[namespace][-1]->failures));
      }
      if (sizeof(scopes[namespace][-1]) == 1) {
	// Leaving namespace...
	scopes[namespace] = namespaceStack[-1][1];
	namespace = namespaceStack[-1][0];
	namespaceStack = namespaceStack[..<1];
      } else {
	scopes[namespace] = scopes[namespace][..<1];
      }
    }
  }

  void extend_namespace(string parent, string child)
  {
    if (!namespace_extends[child]) {
      namespace_extends[child] = (<parent>);
    } else {
      namespace_extends[child][parent] = 1;
    }
  }

  void addName(string sym) { scopes[namespace][-1]->idents[sym] = 1; }

  void remName(string sym) { scopes[namespace][-1]->idents[sym] = 0; }

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
	     namespaceStack);
    }
#endif /* 0 */

    if (has_suffix(idents[0], "::")) {
      // Namespace specifier.
      if (idents[0] == "top::") {
	// top:: is an anchor to the root of the current namespace.
	idents[0] = namespace + "::";
      }
      ref = mergeRef(idents);

      // we consider it to be an absolute reference
      // TODO: should we check that the symbol really DOES appear
      // in the specified scope too?
      return ([ "resolved" : ref ]);
    }

    // Relative reference.
    if (!scopes[namespace][-1]->failures[ref]) {
      array(string) matches = ({});

      ref = (idents - ({""}))*".";

      string firstIdent = idents[0];
      for(int i = sizeof(scopes[namespace]); i--;) {
	Scope s = scopes[namespace][i];
	if (s->idents[firstIdent] || s->idents["/precompiled/" + firstIdent])
	  if (s->type == "params" && !not_param) {
	    return ([ "param" : ref ]);
	  }
	  else {
	    //werror("[[[[ found in type(%O) name(%O)\n", s->type, s->name);
	    string res = namespace + "::";
	    // work our way from the root of the stack
	    for (int j = 1; j <= i; j++) {
	      string name = scopes[namespace][j]->name;
	      if (name && name != "")
		res += name + ".";
	      //werror("[[[[ name == %O\n", name);
	    }
	    if (!s->idents[firstIdent]) res += "/precompiled/";
	    matches += ({ res + ref });
	  }
      }
      string ns = namespace;
      for (multiset(string) extends = namespace_extends[ns];
	   (ns && extends); extends = namespace_extends[ns]) {
	ns = 0;
	foreach(extends; string ns2;) {
	  if (namespace_extends[ns2]) ns = ns2;
	  matches += ({ ns2 + "::" + ref });
	}
      }
      if (sizeof(matches)) {
	return ([ "resolved" : matches*"\0" ]);
      }
      // Resolution failure.
      int i;
      for (i = sizeof(scopes[namespace]); i--;) {
	if (scopes[namespace][i]->type != "params") {
	  scopes[namespace][i]->failures[ref] = 1;
	  break;
	}
      }
    }
    return ([ "resolution-failure" : "yes" ]);
  }
}

protected void fixupRefs(ScopeStack scopes, SimpleNode node) {
  node->walk_preorder(
    lambda(SimpleNode n) {
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

// expects a <autodoc>, <namespace>, <module> or <class> node
protected void resolveFun(ScopeStack scopes, SimpleNode node) {
  if (node->get_any_name() == "namespace") {
    // Create the namespace.
    scopes->enter("namespace", node->get_attributes()["name"]);
  }
  // first collect the names of all things inside the scope
  foreach (node->get_children(), SimpleNode child) {
    if (child->get_node_type() == XML_ELEMENT) {
      switch (child->get_any_name()) {
        case "docgroup":
          // add the names of all things documented
          // in the docgroup.
          foreach (child->get_children(), SimpleNode thing)
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
          break; // do nothing (?)
      }
    }
  }
  foreach (node->get_children(), SimpleNode child) {
    if (child->get_node_type() == XML_ELEMENT) {
      string tag = child->get_any_name();
      switch (tag) {
        case "namespace":
	  resolveFun(scopes, child);
	  break;
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
            foreach (child->get_children(), SimpleNode n) {
              if (n->get_any_name() == "method") {
                foreach (filter(n->get_children(),
                                lambda (SimpleNode n) {
                                  return n->get_any_name() == "arguments";
                                }), SimpleNode m)
                  foreach (m->get_children(), SimpleNode argnode) {
		    if(argnode->get_node_type() != XML_ELEMENT)
		      continue;
                    scopes->addName(argnode->get_attributes()["name"]);
		  }
	      }
	    }
            SimpleNode doc = 0;
	    foreach(child->get_children(), SimpleNode n) {
	      if (n->get_any_name() == "doc") {
		doc = n;
		fixupRefs(scopes, n);
	      }
	    }
	    if (!doc)
	      werror("No doc element found\n%s\n\n", child->render_xml());
	    scopes->leave();
	    if (((child->get_attributes()["homogen-type"] == "inherit") ||
		 (child->get_attributes()["homogen-type"] == "import")) &&
		(child->get_attributes()["homogen-name"])) {
	      // Avoid finding ourselves...
	      scopes->remName(child->get_attributes()["homogen-name"]);
	      foreach(child->get_children(), SimpleNode n) {
		if (n->get_any_name() != "doc") {
		  fixupRefs(scopes, n);
		}
	      }
	      scopes->addName(child->get_attributes()["homogen-name"]);
	    } else {
	      foreach(child->get_children(), SimpleNode n) {
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
        case "import":
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
  if (node->get_any_name() == "namespace") {
    // Leave the namespace.
    scopes->leave();
  }
}

// Resolves references, regarding the SimpleNode 'tree' as the root of
// the whole hierarchy. 'tree' might be the node <root> or <module
// name="Pike">
void oldResolveRefs(SimpleNode tree) {
  ScopeStack scopes = ScopeStack();
  scopes->enter("namespace", "predef");    // The default scope
  resolveFun(scopes, tree);
  scopes->leave();
}

protected class DummyNScope(string name)
{
  string lookup(array(string) ref)
  {
    return 0;
  }
}

//! A symbol lookup scope.
class NScope
{
  string name;
  string type;
  string path;
  mapping(string:int(1..1)|NScope) symbols = ([]);
  mapping(string:string|NScope) inherits;
  multiset(string|NScope) imports;

  // @[tree] is a node of type autodoc, namespace, module, class, enum,
  // or docgroup.
  protected void create(SimpleNode tree, string|void path)
  {
    if (tree->get_node_type() == XML_ROOT) {
      tree = tree->get_first_element();
    }
    type = tree->get_any_name();
    name = tree->get_attributes()->name;
    if (path) {
      name = path + name;
      path = name + ".";
    } else if (type == "namespace") {
      name += "::";
      path = name;
    } else if (type != "autodoc") {
      error("Unsupported node type: %O, name: %O, path: %O\n",
	    type, name, path);
    }
    if (!(<"autodoc", "namespace", "module", "class", "method", "enum">)[type]) {
      error("Unsupported node type: %O, name: %O, path: %O\n",
	    type, name, path);
    }
    this_program::path = path;
    enterNode(tree);
  }

  //! This function improves the symbol resolution by adding
  //! implicit inherits for modules in compatibility namespaces.
  void addImplicitInherits(string|void fallback_namespace)
  {
    switch(type) {
    default:
      return;
    case "autodoc":
      foreach(symbols;;int(1..)|NScope scope) {
	if (objectp(scope)) {
	  scope->addImplicitInherits();
	}
      }
      break;
    case "namespace":
      if (inherits && sizeof(inherits) == 1) {
	foreach(symbols;;int(1..)|NScope scope) {
	  if (objectp(scope)) {
	    scope->addImplicitInherits(values(inherits)[0]);
	  }
	}
      }
      break;
    case "module":
      if (!inherits) inherits = ([]);
      string inh = fallback_namespace + (name/"::")[-1];
      inherits["\0"+inh] = inh;
      foreach(symbols;;int(1..)|NScope scope) {
	if (objectp(scope)) {
	  scope->addImplicitInherits(fallback_namespace);
	}
      }
      break;
    }
  }

  void enterNode(SimpleNode tree)
  {
    foreach(tree->get_children(), SimpleNode child) {
      string n;
      switch (child->get_any_name()) {
      case "docgroup":
	string h_name = child->get_attributes()["homogen-name"];
	NScope h_scope = symbols[h_name];
	foreach(child->get_children(), SimpleNode thing) {
	  n = (thing->get_attributes() || ([]))->name;
	  string subtype = thing->get_any_name();
	  switch(subtype) {
	  case "method":
	    if (n) {
	      NScope scope = h_scope || symbols[n];
	      if (!scope) {
		scope = NScope(thing, path);
	      } else if (!objectp(scope)) {
		werror("%s is both a non-scope and a %s scope!\n",
		       path + n, subtype);
		scope = NScope(thing, path);
	      } else {
		scope->enterNode(thing);
	      }
	      symbols[n] = scope;
	    }
	    break;
	  case "import":
	    SimpleNode imp = thing->get_first_element("classname");
	    string scope = imp->value_of_node();
	    if (!n) n = scope;
	    // We can't lookup the import yet, so put a place holder for it.
	    if (imports) {
	      imports[scope] = 1;
	    } else {
	      imports = (< scope >);
	    }
	    break;
	  case "inherit":
	    SimpleNode inh = thing->get_first_element("classname");
	    scope = inh->value_of_node();
	    if (!n) n = scope;
	    // We can't lookup the inherit yet, so put a place holder for it.
	    if (inherits) {
	      inherits[n] = scope;
	    } else {
	      inherits = ([ n:scope ]);
	    }
	    break;
	  default:
	    // variable, constant, typedef, enum, etc.
	    if (n) {
	      if (objectp(symbols[n])) {
		werror("%s is both a %s scope and a %O!\n",
		       path + n, symbols[n]->type, subtype);
	      } else {
		symbols[n] = 1;
	      }
	    }
	    break;
	  }
	}
	break;
      case "namespace":
      case "module":
      case "class":
      case "enum":
	// FIXME: What about unnamed enums?
	n = child->get_attributes()->name;	  
	if (n) {
	  if (child->get_any_name() == "namespace") {
	    n += "::";
	  }
	  symbols[n] = NScope(child, path);
	  if (child->get_any_name() == "enum") {
	    // Ugly kludge:
	    // Use an implicit inherit to be able to find the values
	    // where expected...
	    if (inherits) {
	      inherits["\0"+n] = symbols[n];
	    } else {
	      inherits = ([ "\0"+n:symbols[n] ]);
	    }
	    break;
	  }
	}
	break;
      case "arguments":
	foreach(child->get_children(), SimpleNode arg) {
	  if (arg->get_node_type() == XML_ELEMENT) {
	    string arg_name = arg->get_attributes()->name;
	    if (objectp(symbols[arg_name])) {
	      werror("%s is both a %s scope and an argument!\n",
		     path + arg_name, h_scope->type);
	    } else {
	      symbols[arg_name] = 1;
	    }
	  }
	}
	break;
      default:
	break;
      }
    }
  }

  protected string _sprintf(int c)
  {
    return sprintf("NScope(type:%O, name:%O, symbols:%d, inherits:%d)",
		   type, name, sizeof(symbols), sizeof(inherits||([])));
  }

  string lookup(array(string) path, int(0..1)|void no_imports )
  {
    int(1..1)|NScope scope =
      symbols[path[0]] || symbols["/precompiled/"+path[0]];
    if (!scope) {
      // Not immediately available in this scope.
      if (inherits) {
	foreach(inherits; string inh; scope) {
	  if (objectp(scope)) {
	    string res = scope->lookup(path, 1);
	    if (res) return res;
	  }
	}
      }
      if (imports && !no_imports) {
	foreach(imports; scope; ) {
	  if (objectp(scope)) {
	    string res = scope->lookup(path, 1);
	    if (res) return res;
	  }
	}
      }
      return 0;
    } else if (sizeof(path) == 1) {
      if (objectp(scope)) {
	return scope->name;
      }
      return name + "." + path[0];
    } else if (!objectp(scope)) {
      return 0;
    }
    return scope->lookup(path[1..], 1);
  }
}

class NScopeStack
{
  NScope scopes;
  NScope top;
  array(NScope) stack = ({});
  mapping(string:mapping(string:int(1..))) failures = ([]);
  string logfile;
  .Flags flags;

  protected void warn(sprintf_format fmt, sprintf_args ... args)
  {
    if (flags & .FLAG_VERB_MASK) {
      werror(fmt, @args);
    }
  }

  protected void create(NScope scopes, string|void logfile, .Flags|void flags)
  {
    this_program::scopes = scopes;
    this_program::logfile = logfile;
    if (zero_type(flags)) flags = .FLAG_NORMAL;
    this_program::flags = flags;
  }

  protected void destroy()
  {
    if (sizeof(failures)) {
      logfile = logfile || "resolution.log";
      warn("Resolution failed for %d symbols. Logging to %s\n",
	   sizeof(failures), logfile);
      Stdio.File f = Stdio.File(logfile, "cwt");
      f->write("Reference target: Reference source:references\n\n");
      mapping(string:array(string)) rev = ([]);
      foreach(sort(indices(failures)), string ref) {
	mapping(string:int) where = failures[ref];
	array(array(string|int)) srcs = (array)where;
	sort(map(srcs, predef::`[], 0), srcs);
	f->write("  %O: %{%O:%d, %}\n", ref, srcs);
	foreach(indices(where), string source)
	  if(rev[source])
	    rev[source] += ({ ref });
	  else
	    rev[source] = ({ ref });
      }
      f->write("\n\nReference source: Reference targets.\n\n");
      foreach(sort(indices(rev)), string source) {
	array(string) targets = rev[source];
	f->write("%O:%{ %O%}\n", source, sort(targets));
      }
      f->close();
    }
  }

  protected string _sprintf(int c)
  {
    return sprintf("NScopeStack(num_scopes: %d, top: %O)",
		   sizeof(stack), top);
  }

  void addImplicitInherits()
  {
    scopes->addImplicitInherits();
  }

  void reset()
  {
    top = scopes;
    stack = ({});
  }

  void enter(string symbol)
  {
    int(1..1)|NScope scope = top->symbols[symbol];
    if (!scope) {
      if (!(scope = top->symbols["/precompiled/" + symbol])) {
	error("No such symbol: %O in scope %O %O\n", symbol, top, stack);
      }
    }
    if (!objectp(scope)) {
      error("Symbol %s is not a scope.\n"
	    "Stack: %O\n", top->path + symbol, stack);
    }
    stack += ({ top });
    top = scope;
  }

  void leave()
  {
    if (!top) {
      error("leave() called too many times()\n");
    }
    if (sizeof(stack)) {
      top = stack[-1];
      stack = stack[..<1];
    } else {
      top = 0;
    }
  }

  NScope|int(1..1) lookup(string ref)
  {
    array(string) path = splitRef(ref);
    int(1..1)|NScope val = scopes;
    foreach(path, string sym) {
      if (objectp(val)) {
	val = val->symbols[sym] || val->symbols["/precompiled/" + sym];
      } else {
	return 0;
      }
    }
    return val;
  }

  string resolve(array(string) ref)
  {
    if (!sizeof(ref)) {
      return top->name;
    }
    int pos = sizeof(stack);
    NScope current = top;
    if (has_suffix(ref[0], "::")) {
      // Inherit or namespace.
      switch(ref[0]) {
      case "this_program::":
	while (pos) {
	  if ((<"class", "module", "namespace">)[current->type]) {
	    return current->lookup(ref[1..]);
	  }
	  pos--;
	  current = stack[pos];
	}
	return 0;
      case "::":
	while(pos) {
	  if (current->inherits) {
	    foreach(current->inherits; ; string|NScope scope) {
	      if (stringp(scope)) scope = lookup(scope);
	      if (objectp(scope)) {
		string res = scope->lookup(ref[1..]);
		if (res) return res;
	      }
	    }
	  }
	  pos--;
	  current = stack[pos];
	}
	break;
      default:
	// Strip the trailing "::".
	string inh = ref[0][..sizeof(ref[0])-3];
	// Map intermediate version namespaces to existing.
	// FIXME: This ought be done based on the set of namespaces.
	inh = ([
	  "0.7":"7.0",
	  "7.1":"7.2",
	  "7.3":"7.4",
	  "7.5":"7.6",
	  "7.7":"7.8",
	  "7.9":"predef",
	])[inh] || inh;
	while(pos) {
	  string|NScope scope;
	  if (current->inherits && (scope = current->inherits[inh])) {
	    if (stringp(scope)) scope = lookup(scope);
	    if (objectp(scope)) {
	      string res = scope->lookup(ref[1..]);
	      if (res) return res;
	    }
	  }
	  pos--;
	  current = stack[pos];
	}
	ref = ({ inh + "::" }) + ref[1..];
	break;
      }
      return scopes->lookup(ref, 1);
    }
    if (!sizeof(ref[0])) {
      // .module
#if 0
      // FIXME: The following does not handle the module.pmod case.
      if (pos) {
	current = stack[--pos];
      } else {
	current = top;
      }
#endif /* 0 */
      ref = ref[1..];
    }
    if (sizeof(ref) <= 1) {
      // Handle some special symbols.
      switch(sizeof(ref) && ref[0]) {
      case 0:
      case "this":
      case "this_program":
	int tmp = pos;
	while(tmp--) {
	  if ((<"class", "module">)[stack[tmp]->type]) {
	    return stack[tmp]->name;
	  }
	}
	break;
      }
    }
    while(pos--) {
      // Look in imports too.
      string res = current->lookup(ref);
      if (res) return res;
      current = stack[pos];
    }
    return 0;
  }

  void resolveInherits()
  {
    int removed_self;
    string name = sizeof(stack) && splitRef(top->name)[-1];
    if (sizeof(stack) && (stack[-1]->symbols[name] == top)) {
      removed_self = 1;
      m_delete(stack[-1]->symbols, name);
    }
    // FIXME: Inherits and imports add visibility of symbols
    //        to later inherits and imports. This is not handled
    //        by the code below.
    foreach(top->inherits||([]); string inh; string|NScope scope) {
      if (stringp(scope)) {
	if (sizeof(scope) && scope[0] == '"') {
	  // Inherit of files not supported yet.
	} else {
	  top->inherits[inh] = DummyNScope(scope);
	  string path = resolve(splitRef(scope));
	  if (path) {
	    top->inherits[inh] = path;
	    continue;
	  } else {
	    warn("Failed to resolve inherit %O.\n"
		 "  Top: %O\n"
		 "  Scope: %O\n"
		 "  Stack: %O\n"
		 "  Ref: %O\n",
		 inh, top, scope, stack, splitRef(scope));
	  }
	}
	m_delete(top->inherits, inh);
      }
    }
    foreach(indices(top->imports||(<>)); ; string|NScope scope) {
      if (stringp(scope)) {
	top->imports[scope] = 0;
	if (sizeof(scope) && scope[0] == '"') {
	  // Import of files not supported yet.
	} else {
	  string path = resolve(splitRef(scope));
	  if (path) {
	    top->imports[path] = 1;
	    continue;
	  } else {
	    warn("Failed to resolve import %O.\n"
		 "  Top: %O\n"
		 "  Stack: %O\n",
		 scope, top, stack);
	  }
	}
      }
    }
    // Make ourselves available again.
    // This is needed for looking up of symbols in ourself.
    if (removed_self) {
      stack[-1]->symbols[name] = top;
    }
    foreach(top->inherits||([]); string inh; string|NScope scope) {
      if (stringp(scope)) {
	string path = [string]scope;
	int(1..1)|NScope nscope = lookup(path);
	if (objectp(nscope)) {
	  // Avoid loops...
	  if (nscope != top) {
	    top->inherits[inh] = nscope;
	    continue;
	  }
	  warn("Failed to lookup inherit %O (loop).\n"
	       "  Top: %O\n"
	       "  Scope: %O\n"
	       "  Path: %O\n"
	       "  NewScope: %O\n"
	       "  Stack: %O\n",
	       inh, top, scope, path, nscope, stack);
	} else {
	  warn("Failed to lookup inherit %O.\n"
	       "  Top: %O\n"
	       "  Scope: %O\n"
	       "  Path: %O\n"
	       "  NewScope: %O\n"
	       "  Stack: %O\n"
	       "  Top->Symbols: %O\n",
	       inh, top, scope, path, nscope, stack,
	       indices(top->symbols));
	}
	m_delete(top->inherits, inh);
      }
    }
    foreach(indices(top->imports||(<>)); ; string|NScope scope) {
      if (stringp(scope)) {
	string path = [string]scope;
	top->imports[path] = 0;
	int(1..1)|NScope nscope = lookup(path);
	if (objectp(nscope)) {
	  // Avoid loops...
	  if (nscope != top) {
	    top->imports[nscope] = 1;
	    continue;
	  }
	  warn("Failed to lookup import %O (loop).\n"
	       "  Top: %O\n"
	       "  Scope: %O\n"
	       "  NewScope: %O\n"
	       "  Stack: %O\n",
	       path, top, scope, nscope, stack);
	} else {
	  warn("Failed to lookup import %O.\n"
	       "  Top: %O\n"
	       "  Scope: %O\n"
	       "  NewScope: %O\n"
	       "  Stack: %O\n"
	       "  Top->Symbols: %O\n",
	       path, top, scope, nscope, stack,
	       indices(top->symbols));
	}
      }
    }
    array(NScope) old_stack = stack;
    NScope old_top = top;
    foreach(top->inherits||([]); string inh; NScope scope) {
      if (sizeof(filter(scope->inherits||({}), stringp))) {
	// We've inherited a scope with unresolved inherits.
	// We need to resolve them before we can resolve
	// stuff in ourselves.
	reset();
	foreach(splitRef(scope->name), string sym) {
	  enter(sym);
	}
	resolveInherits();
      }
    }
    // Note: We need to do the same for imports, since
    //       they may contain inherited symbols.
    foreach(top->imports||(<>); NScope scope; ) {
      if (sizeof(filter(scope->inherits||({}), stringp))) {
	// We've imported a scope with unresolved inherits.
	// We need to resolve them before we can resolve
	// stuff in ourselves.
	reset();
	foreach(splitRef(scope->name), string sym) {
	  enter(sym);
	}
	resolveInherits();
      }
    }
    top = old_top;
    stack = old_stack;
    foreach(top->symbols; string sym; int(1..1)|NScope scope) {
      if (objectp(scope)) {
	enter(sym);
	resolveInherits();
	leave();
      }
    }
  }
}

void doResolveNode(NScopeStack scopestack, SimpleNode tree)
{
  int pop = 0;
  switch(tree->get_any_name()) {
  case "autodoc":
    scopestack->reset();
    pop = 1;
    break;
  case "namespace":
    scopestack->enter(tree->get_attributes()->name + "::");
    pop = 1;
    break;
  case "class":
  case "module":
    scopestack->enter(tree->get_attributes()->name);
    pop = 1;
    break;
  case "docgroup":
    if (tree->get_attributes()["homogen-type"] == "method") {
      string n = tree->get_attributes()["homogen-name"];
      if (n) {
	scopestack->enter(n);
	pop = 1;
      } else {
	array(SimpleNode) methods = tree->get_children();
	methods = filter(methods, methods->get_attributes());
	methods = filter(methods, methods->get_attributes()->name);
	if (sizeof(methods)) {
	  // Enter the first named method.
	  scopestack->enter(methods[0]->get_attributes()->name);
	  pop = 1;
	} else {
	  werror("No named methods in docgroup\n");
	}
      }
    }
    break;
  case "inherit":
    foreach(tree->get_children(), SimpleNode child) {
      mapping(string:string) m;
      if ((child->get_any_name() == "classname") &&
	  (!(m = child->get_attributes())->resolved)) {
	string ref = child->value_of_node();
	NScope ns;
	if (ns = (scopestack->top->inherits[ref])) {
	  string resolution = scopestack->resolve(splitRef(ns->name));
	  if (resolution) {
	    m->resolved = resolution;
	  }
	}
      }
    }
    break;
  case "modifiers":
  case "enum":
  case "doc":
    break;
  case "ref":
  case "classname":
  case "object":
    mapping(string:string) m = tree->get_attributes();
    if (!m->resolved) {
      string ref = m->to || tree->value_of_node();
      string resolution = scopestack->resolve(splitRef(ref));
      if (resolution) {
	m->resolved = resolution;
      } else {
	if (!scopestack->failures[ref]) {
	  scopestack->failures[ref] = ([]);
	}
	scopestack->failures[ref][scopestack->top->name]++;
      }
    }
    return;	// No need to recurse...
  }
  foreach(tree->get_children(), SimpleNode child) {
    doResolveNode(scopestack, child);
  }
  if (pop) {
    scopestack->leave();
  }
}

void resolveRefs(SimpleNode tree, string|void logfile, .Flags|void flags)
{
  if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
    werror("Building the scope structure...\n");
  NScopeStack scopestack = NScopeStack(NScope(tree), logfile, flags);
  if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
    werror("Adding implicit inherits for compatibility modules...\n");
  scopestack->addImplicitInherits();
  if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
    werror("Resolving inherits...\n");
  scopestack->reset();
  scopestack->resolveInherits();
  if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
    werror("Resolving references...\n");
  doResolveNode(scopestack, tree);
  destruct(scopestack);
  if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
    werror("Done.\n");
}

void cleanUndocumented(SimpleNode tree, .Flags|void flags)
{
  int(0..1) check_node(SimpleNode n) {
    array(SimpleNode) children = n->get_children();
    int num = sizeof(children);
    children = filter(children, map(children, check_node));
    if (sizeof(children) != num) {
      n->replace_children(children);
      check_node(n);
    }

    string name = n->get_tag_name();
    if(name!="class" && name!="module") return 1;

    array ch = n->get_elements()->get_tag_name();
    ch -= ({ "modifiers" });
    ch -= ({ "source-position" });
    if(sizeof(ch)) return 1;
    if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
      werror("Removed empty %s %O\n", name, n->get_attributes()->name);
    return 0;
  };

  check_node(tree);
}

//! Perform the last steps on a completed documentation tree.
//!
//! @param root
//!   Root @tt{<autodoc>@} node of the completed documentation tree.
//!
//! Calls @[handleAppears()], @[cleanUndocumented()] and @[resolveRefs()]
//! in turn.
//!
//! @seealso
//!   @[handleAppears()], @[cleanUndocumented()], @[resolveRefs()]
void postProcess(SimpleNode root, string|void logfile, .Flags|void flags) {
  //  werror("handleAppears\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
  handleAppears(root, flags);
  //  werror("cleanUndocumented\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
  cleanUndocumented(root, flags);
  //  werror("resolveRefs\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
  resolveRefs(root, logfile, flags);
  //  werror("done postProcess\n%s%O\n", ctime(time()), Debug.pp_memory_usage());
}
