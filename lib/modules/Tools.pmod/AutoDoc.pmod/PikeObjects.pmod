#pike __REAL_VERSION__

// This module contains classes to represent Pike objects such as
// classes, modules, methods, variables ...
// The classes can produce XML representations of themselves.

// #pragma strict_types
#include "./debug.h"

static inherit "module.pmod";
static inherit Parser.XML.Tree;


//========================================================================
// REPRESENTATION OF TYPES
//========================================================================

class Type(string name) {
  string print() { return name; }
  string xml() { return xmltag(name);}
}

class ArrayType {
  inherit Type;
  Type valuetype;
  void create() { ::create("array"); }
  string print() {
    if (valuetype)
      return "array(" + valuetype->print() + ")";
    else
      return "array";
  }
  string xml() {
    if (!valuetype)
      return ::xml();
    return xmltag("array",
                  xmltag("valuetype", valuetype->xml()));
  }
}

class ProgramType {
  inherit Type;
  string classname;
  string print() {
    if(classname)
      return "program(" + classname + ")";
    else
      return "program";
  }
  // FIXME xml() needs to be overridden.
  void create() { ::create("program"); }
}

class FloatType {
  inherit Type;
  void create() { ::create("float"); }
}

class IntType {
  inherit Type;
  string min, max;
  void create() { ::create("int"); }
  string print() {
    if (min || max)
      return "int(" + (min ? min : "")
        + ".." + (max ? max : "") + ")";
    else
      return "int";
  }
  string xml() {
    if (min || max)
      return xmltag("int",
                    xmltag("min", min) + xmltag("max", max));
    return xmltag("int");
  }
}

class StringType {
  inherit Type;
  void create() { ::create("string"); }
}

class MixedType {
  inherit Type;
  void create() { ::create("mixed"); }
}

class FunctionType {
  inherit Type;
  array(Type) argtypes;
  Type returntype;
  void create() { ::create("function"); }
  string print() {
    if (argtypes && returntype) {
      return "function("
        + map(argtypes, lambda(Type t) {
                        return t->print();
                      }) * ", "
        + " : "
        + returntype->print()
        + ")";
    } else
      return "function";
  }
  string xml() {
    string s = "";
    if (argtypes)
      foreach(argtypes, Type t)
        s += xmltag("argtype", t->xml());
    if (returntype)
      s += xmltag("returntype", returntype->xml());
    return xmltag("function", s);
  }
}

class MappingType {
  inherit Type;
  Type indextype, valuetype;
  void create() { ::create("mapping"); }
  string print() {
    if (indextype && valuetype) {
      return "mapping("
        + indextype->print()
        + " : "
        + valuetype->print()
        + ")";
    }
    else
      return "mapping";
  }
  string xml() {
    if (indextype && valuetype)
      return xmltag("mapping",
                    xmltag("indextype", indextype->xml())
                    + xmltag("valuetype", valuetype->xml()));
    else
      return xmltag("mapping");
  }
}

class MultisetType {
  inherit Type;
  Type indextype;
  void create() { ::create("multiset"); }
  string print() {
    if (indextype) {
      return "multiset("
        + indextype->print()
        + ")";
    }
    else
      return "multiset";
  }
  string xml() {
    if (indextype)
      return xmltag("multiset",
                    xmltag("indextype", indextype->xml()));
    return xmltag("multiset");
  }
}

class ObjectType {
  inherit Type;
  string classname;
  void create() { ::create("object"); }
  string print() {
    if (classname)
      return "object(" + classname + ")";
    else
      return "object";
  }
  string xml() {
    return xmltag("object", classname);
  }
}

class TypeType {
  inherit Type;
  Type subtype = MixedType();
  void create() { ::create("type"); }
  string print() {
    if (subtype->name != "mixed") {
      return "type(" + subtype->print() + ")";
    }
    return "type";
  }
  string xml() {
    return xmltag("type", subtype->xml());
  }
}

class VoidType {
  inherit Type;
  void create() { ::create("void"); }
}

class ZeroType {
  inherit Type;
  void create() { ::create("zero"); }
}

class OrType {
  inherit Type;
  array(Type) types;
  void create() { ::create("or"); }
  string print() {
    return map(types, lambda(Type t) { return t->print(); }) * " | ";
  }
  string xml() {
    string s = "";
    foreach(types, Type t)
      s += t->xml();
    return xmltag("or", s);
  }
}

class VarargsType {
  inherit Type;
  Type type;
  void create(Type t) { ::create("varargs"); type = t; }
  string print() { return type->print() + " ..."; }
  string xml() { return xmltag("varargs", type->xml()); }
}

//========================================================================
// DOCUMENTATION OBJECT
//========================================================================

class Documentation(string|void text, string|void xml,
		    SourcePosition|void position)
{}

static Documentation EmptyDoc =
  Documentation("", "\n", SourcePosition(__FILE__, __LINE__, __LINE__));

class DocGroup {
  array(PikeObject) objects = ({ });
  Documentation documentation = 0;
  static void create(array(PikeObject) objs, Documentation doc) {
    documentation = doc;
    objects = objs;
  }
  string appears = 0;
  string belongs = 0;

  string xml() {
    mapping(string:string) m = ([]);
    if (appears) m->appears = appears;
    if (belongs) m->belongs = belongs;

    // Check if homogeneous
    mapping (string:int) types = ([]);
    mapping (string:int) names = ([]);
    foreach(objects, PikeObject obj) {
      types[obj->objtype] = 1;
      if (obj->name)
        names[obj->name] = 1;
    }
    if (sizeof(types) == 1) {
      m["homogen-type"] = indices(types)[0];
      if (sizeof(names) == 1)
        m["homogen-name"] = objects[0]->name;
    }

    string res = opentag("docgroup", m);
    if (documentation)
      res += xmltag("doc", documentation->xml) + "\n";
    foreach(objects, PikeObject obj)
      res += obj->xml() + "\n";
    return res + closetag("docgroup") + "\n";
  }
}

//========================================================================
// REPRESENTATION OF PIKES LEXICAL OBJECTS (class, function, variable ...)
//========================================================================

class PikeObject {
  array(string) modifiers = ({ });
  string name;
  constant objtype = "pikeobject";

  SourcePosition position;
  string appears;
  string belongs;

  Documentation squeezedInDoc;

  static string standardTags() {
    string s = "";
    if (position)
      s += position->xml();
    if (sizeof(modifiers))
      s += xmltag("modifiers", map(modifiers, xmltag) * "");
    return s;
  }

  string xml() {
    return standardStart() + standardEnd();
  }

  static mapping(string:string) standardAttributes() {
    mapping(string:string) m = ([]);
    if (name)    m->name = name;
    if (appears) m->appears = appears;
    if (belongs) m->belongs = belongs;
    return m;
  }

  static string standardStart() { return opentag(objtype, standardAttributes()); }
  static string standardEnd() { return closetag(objtype); }

  static string printModifiers() {
    return modifiers * " " + (sizeof(modifiers) ? " " : "");
  }
  string print() { return printModifiers() + objtype; }
}

class Inherit {
  inherit PikeObject;
  constant objtype = "inherit";

  string classname;
  string xml() {
    return standardStart() +
      standardTags() +
      xmltag("classname", classname) +
      standardEnd();
  }
  string print() {
    return ::print() + " " + classname + (name ? "" : " : " + name);
  }
}

class Import {
  inherit PikeObject;
  constant objtype = "import";
}

class _Class_or_Module {
  inherit PikeObject;

  string directory = 0;
  string file = 0;

  // NB: The documentation appears as a child of the <class> or <module>
  Documentation documentation;

  array(Inherit) inherits = ({ });
  array(_Class_or_Module) children = ({ });
  array(DocGroup) docGroups = ({ });

  int containsDoc() { return documentation != 0 || sizeof(docGroups) > 0; }

  void AddInherit(PikeObject p) { inherits += ({ p }); }
  void AddChild(_Class_or_Module c) { children += ({ c }); }

  PikeObject findChild(string name) {
    int a = Array.search_array(children,
                               lambda(PikeObject o, string n) {
                                 return o->name == n;
                               }, name);
    if (a < 0) return 0;
    return children[a];
  }

  void AddGroup(DocGroup d) {
    docGroups += ({ d });
  }

  string xml() {
    string contents = standardTags();
    if (documentation && documentation->xml != "")
      contents += xmltag("doc", documentation->xml);
    children -= ({ 0 }); // FIXME
    foreach (children, _Class_or_Module c)
      contents += c->xml();
    foreach (inherits, Inherit in) {
      // Wrap the undocumented inherits in docgroups.
      DocGroup dg = DocGroup(({in}), EmptyDoc);
      contents += dg->xml();
    }
    foreach (docGroups, DocGroup dg)
      contents += dg->xml();
    mapping(string:string) m = standardAttributes();
    if (file) m["file"] = file;
    if (directory) m["directory"] = directory;
    return "\n\n" + opentag(objtype, m) + contents + standardEnd();
  }

  string print() {
    string s = ::print() + " " + name + " {\n";
    foreach(docGroups, DocGroup dg) {
      if (dg->documentation->position) {
	s += sprintf("doc [%d..%d]:\n",
		     dg->documentation->position->firstline,
		     dg->documentation->position->lastline);
      } else {
	s += "doc [..]:\n";
      }
      s += dg->documentation->text + "\n";
      foreach(dg->objects, PikeObject p) {
	if (p->position) {
	  s += sprintf("%s   [%d..%d]\n", p->print(),
		       p->position->firstline,
		       p->position->lastline);
	} else {
	  s += sprintf("%s   [..]\n", p->print());
	}
      }
    }
    return s + "\n}";
  }

  static string _sprintf(int c)
  {
    switch(c) {
    case 's': return xml();
    case 'O': return sprintf("%O(%O)", this_program, name);
    }
  }
}

class Class {
  inherit _Class_or_Module;
  constant objtype = "class";

  array(Type) createArgTypes;
  array(string) createArgNames;
  array(array(string)) createArgModifiers;
}

class Module {
  inherit _Class_or_Module;
  constant objtype = "module";
}

// A name space, eg: predef:: or lfun::
class NameSpace {
  inherit _Class_or_Module;
  constant objtype = "namespace";
}

// The top-level container.
// This container should only contain namespaces,
// and they in turn contain modules etc.
class AutoDoc {
  inherit _Class_or_Module;
  constant objtype = "autodoc";
}

// A modifier range, e.g.:
// static private {
//   ...
//   <<declarations>>
//   ...
// }
class Modifier {
  inherit _Class_or_Module;
  constant objtype = "modifier";
}

class Method {
  inherit PikeObject;
  array(string) argnames;
  array(Type) argtypes;
  Type returntype;
  constant objtype = "method";
  string xml() {
    string s = standardTags() + "\n";
    string args = "";
    for(int i = 0; i < sizeof(argnames); ++i) {
      if (argtypes[i])
        args += xmltag("argument",
                       (["name" : argnames[i] ]),
                       xmltag("type", argtypes[i]->xml()));
      else
        args += xmltag("argument",
                       xmltag("value", argnames[i]));
    }
    s += xmltag("arguments", args) + "\n" +
      xmltag("returntype", returntype->xml()) + "\n";
    return standardStart() + s + standardEnd();
  }
  string print() {
    array(string) args = ({ });
    for(int i = 0; i < sizeof(argnames); ++i)
      args += ({ argtypes[i]->print() + " " + argnames[i] });
    return printModifiers() + name + "(" + args * ", " + ")";
  }
}

class Constant {
  inherit PikeObject;
  constant objtype = "constant";
  Type typedefType = 0;   // if it is a typedef
  string xml() {
    return standardStart() + standardTags()
      + (typedefType ? xmltag("typevalue", typedefType->xml()) : "")
      + standardEnd();
  }
  string print() {
    return ::print() + " " + name;
  }
}

class Typedef {
  inherit PikeObject;
  constant objtype = "typedef";
  Type type = 0;
  string xml() {
    return standardStart() + standardTags()
      + xmltag("type", type->xml())
      + standardEnd();
  }
  string print() {
    return ::print() + (type ? " " + type->print() + " " : "") + name;
  }
}

// The values inside enum Foo { ... }
class EnumConstant {
  inherit PikeObject;
  constant objtype = "constant";
  string xml() {
    mapping m = ([]) + standardAttributes();
    return opentag(objtype, m) + standardTags()
      + standardEnd();
  }
  string print() {
    return "constant";  // for now...
  }
}

// The enum container
class Enum {
  inherit PikeObject;
  constant objtype = "enum";

  // mimic the class { ... } behaviour
  Documentation documentation;

  array(DocGroup) children = ({ });
  void addChild(DocGroup c) { children += ({ c }); }
  string xml() {

    // need some special handling to make this look as if it
    // were produced by some other stuff
    string s =  standardStart() + standardTags();
    array(Node) inDocGroups = ({});

    if (documentation && documentation->xml != "") {

      // Have to handle all xpath:enum/doc/group[constant]
      // differently, replacing them according to:
      //
      // <group>
      //   <constant name="foo"/>
      //   <text>....</text>
      // </group>
      //
      // Becomes:
      //
      // <docgroup homogen-type="constant" homogen-name="foo">
      //   <constant name="foo"/>
      //   <doc>
      //     <text>
      //     </text>
      //   </doc>
      // </docgroup>

      Node doc = parse_input(documentation->xml);

      foreach (doc->get_children(), Node group) {
        if (group->get_node_type() == XML_ELEMENT
            && group->get_any_name() == "group")
        {
          int constants = 0;
          string homogenName = 0;
          Node docGroupNode = Node(XML_ELEMENT, "docgroup",
                                   ([ "homogen-type" : "constant" ]), 0);
          Node text = 0;
          foreach (group->get_children(), Node child)
            if (child->get_node_type() == XML_ELEMENT)
              if (child->get_any_name() == "constant") {
                ++constants;
                if (constants == 1)
                  homogenName = child->get_attributes()["name"];
                else
                  homogenName = 0;
                docGroupNode->add_child(child);
              }
              else if (child->get_any_name() == "text") {
                if (text)
                  throw("<group> had more than one <text> child!!!!");
                text = child;
              }
          if (constants) {
            // <group> had at least one <constant> child.
            // Then it should be transformed into a <docgroup> child
            // of the <enum> node.
            doc->remove_child(group);
            if (homogenName)
              docGroupNode->get_attributes() ["homogen-name"] = homogenName;

            if (text) {
              // wrap the <text> node in a <doc>,
              // and then put it inside the <docgroup>
              // together with the <constant>s
              Node d = Node(XML_ELEMENT, "doc", ([]), 0);
              d->add_child(text);
              docGroupNode->add_child(d);
            }
            inDocGroups += ({ docGroupNode });
          }
        }
      }
      s += xmltag("doc", doc->html_of_node());
    }

    foreach (children, DocGroup docGroup)
      s += docGroup->xml();
    foreach (inDocGroups, Node n)
      s += n->html_of_node();

    s += standardEnd();
    return s;
  }

  string print() {
    return "enum";
  }
}

class Variable {
  inherit PikeObject;
  constant objtype = "variable";
  Type type;
  string xml() {
    return standardStart() +
      standardTags() +
      xmltag("type", type->xml()) +
      standardEnd();
  }
  string print() {
    return printModifiers() + type->print() + " " +  name;
  }
}
