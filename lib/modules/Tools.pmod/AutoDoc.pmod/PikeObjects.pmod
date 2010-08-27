#pike __REAL_VERSION__

//! This module contains classes to represent Pike objects such as
//! classes, modules, methods, variables ...
//! The classes can produce XML representations of themselves.

// #pragma strict_types
#include "./debug.h"

protected inherit "module.pmod";
protected inherit Parser.XML.Tree;


//========================================================================
// REPRESENTATION OF TYPES
//========================================================================

//! The base class for representing types.
class Type(string name) {
  //! @returns
  //!   Returns a string with a Pike-syntax representation of the type.
  string print() { return name; }

  //! @returns
  //!   Returns a string with an XML representation of the type.
  string xml() { return xmltag(name);}
}

//! The class for representing array types.
//!
//! @seealso
//!   @[Type]
class ArrayType {
  //!
  inherit Type;

  //! The @[Type] of the array elements.
  Type valuetype;

  //!
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

//! The class for representing program (aka class) types.
//!
//! @seealso
//!   @[Type]
class ProgramType {
  //!
  inherit Type;

  //! The name of the class (if any).
  string classname;

  string print() {
    if(classname)
      return "program(" + classname + ")";
    else
      return "program";
  }
  // FIXME xml() needs to be overridden.

  //!
  void create() { ::create("program"); }
}

//! The class for representing the float type.
//!
//! @seealso
//!   @[Type]
class FloatType {
  //!
  inherit Type;

  //!
  void create() { ::create("float"); }
}

//! The class for representing integer types.
//!
//! @seealso
//!   @[Type]
class IntType {
  //!
  inherit Type;

  //! The minimum and maximum range limits for the integer type.
  string min, max;

  //!
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

//! The class for representing string types.
//!
//! @seealso
//!   @[Type]
class StringType {
  //!
  inherit Type;

  //! The minimum value for characters in the string.
  string min;

  //! The maximum value for characters in the string.
  string max;

  //!
  void create() { ::create("string"); }

  string print() {
    if (min||max)
      return "string(" + (min ? min : "")
        + ".." + (max ? max : "") + ")";
    else
      return "string";
  }
  string xml() {
    if (min||max)
      return xmltag("string", 
                    xmltag("min", min) + 
                    xmltag("max", max));
    return xmltag("string");
  }
}

//! The class for representing the mixed type.
//!
//! @seealso
//!   @[Type]
class MixedType {
  //!
  inherit Type;

  //!
  void create() { ::create("mixed"); }
}

//! The class for representing function types.
//!
//! @seealso
//!   @[Type]
class FunctionType {
  //!
  inherit Type;

  //! An array with types for the arguments to the function.
  array(Type) argtypes;

  //! The type for the return value of the function.
  Type returntype;

  //!
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

//! The class for representing mapping types.
//!
//! @seealso
//!   @[Type]
class MappingType {
  //!
  inherit Type;

  //! The types for the indices and values of the mapping.
  Type indextype, valuetype;

  //!
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

//! The class for representing multiset types.
//!
//! @seealso
//!   @[Type]
class MultisetType {
  //!
  inherit Type;

  //! The type for the indices of the multiset.
  Type indextype;

  //!
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

//! The class for representing object types.
//!
//! @seealso
//!   @[Type]
class ObjectType {
  //!
  inherit Type;

  //! The name of the class for the object.
  string classname;

  //!
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

//! The class for representing type types.
//!
//! @seealso
//!   @[Type]
class TypeType {
  //!
  inherit Type;

  //! The subtype of the type.
  Type subtype = MixedType();

  //!
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

//! The class for representing the void type.
//!
//! @seealso
//!   @[Type]
class VoidType {
  //!
  inherit Type;

  //!
  void create() { ::create("void"); }
}

//! The class for representing the zero type.
//!
//! @seealso
//!   @[Type]
class ZeroType {
  //!
  inherit Type;

  //!
  void create() { ::create("zero"); }
}

//! The class for representing union types.
//!
//! @seealso
//!   @[Type]
class OrType {
  //!
  inherit Type;

  //! An array with the types that are member of the union.
  array(Type) types;

  //!
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

//! The class for representing varargs types.
//!
//! @seealso
//!   @[Type]
class VarargsType {
  //!
  inherit Type;

  //! The type that is varargs.
  Type type;

  //!
  void create(Type t) { ::create("varargs"); type = t; }

  string print() { return type->print() + " ..."; }
  string xml() { return xmltag("varargs", type->xml()); }
}

//! The class for representing attributed types.
//!
//! @seealso
//!   @[Type]
class AttributeType {
  //!
  inherit Type;

  //! The name of the attribute.
  string attribute;

  //! The type that is attributed.
  Type subtype;

  //! Flag indicating:
  //! @int
  //!   @value 0
  //!     The attribute is on the type.
  //!   @value 1
  //!     The attribute is a prefix and acts as a modifier.
  //!     This is only used for functions.
  //! @endint
  int prefix;

  //!
  void create() { ::create("__attribute__"); }

  string print() {
    if (attribute == "\"deprecated\"") {
      return prefix?"__deprecated__ " + subtype->print():
	"__deprecated__(" + subtype->print() + ")";
    } else {
      return prefix?"__attribute__(" + attribute + ") " + subtype->print():
	"__attribute__(" + attribute + ", " + subtype->print() + ")";
    }
  }
  string xml() {
    return xmltag("attribute",
		  (prefix?xmltag("prefix"):"") +
		  xmltag("attribute", attribute) +
		  xmltag("subtype", subtype->xml()));
  }
}

//========================================================================
// DOCUMENTATION OBJECT
//========================================================================

//! The base class for documentation strings.
//!
//! @seealso
//!   @[DocGroup]
class Documentation(string|void text, string|void xml,
		    SourcePosition|void position)
{}

//! The empty @[Documentation].
protected Documentation EmptyDoc =
  Documentation("", "\n", SourcePosition(__FILE__, __LINE__, __LINE__));

//! A class associating a piece of @[Documentation]
//! with a set of @[PikeObject]s.
class DocGroup {
  //! The set of @[PikeObject]s that are documented.
  array(PikeObject) objects = ({ });

  //! The @[Documentation] for the @[objects].
  Documentation documentation = 0;

  //!
  protected void create(array(PikeObject) objs, Documentation doc) {
    documentation = doc;
    objects = objs;
  }

  //! Relocation information.
  string appears = 0;
  string belongs = 0;

  //! @returns
  //!   Returns a string with an XML representation of the documentation.
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

//! Base class for representing a documentable Pike lexical entity.
//!
//! This class is inherited by classes for representing
//! classes, functions, variables, etc.
//!
//! @seealso
//!   @[Inherit], @[Import], @[Class], @[Module], @[NameSpace], @[AutoDoc],
//!   @[Modifier], @[Method], @[Constant], @[Typedef], @[EnumConstant],
//!   @[Enum], @[Variable]
class PikeObject {
  //! The set of modifiers affecting this entity.
  array(string) modifiers = ({ });

  //! The name of the entity.
  string name;

  //! The object type identifier.
  constant objtype = "pikeobject";

  //! The source position where the entity was found.
  SourcePosition position;

  //! Relocation information.
  string appears;
  string belongs;

  //!
  Documentation squeezedInDoc;

  protected string standardTags() {
    string s = "";
    if (position)
      s += position->xml();
    if (sizeof(modifiers))
      s += xmltag("modifiers", map(modifiers, xmltag) * "");
    return s;
  }

  //! @returns
  //!   Returns a string with an XML representation of the entity.
  string xml() {
    return standardStart() + standardEnd();
  }

  protected mapping(string:string) standardAttributes() {
    mapping(string:string) m = ([]);
    if (name)    m->name = name;
    if (appears) m->appears = appears;
    if (belongs) m->belongs = belongs;
    return m;
  }

  protected string standardStart() { return opentag(objtype, standardAttributes()); }
  protected string standardEnd() { return closetag(objtype); }

  protected string printModifiers() {
    return modifiers * " " + (sizeof(modifiers) ? " " : "");
  }

  //! @returns
  //!   Returns a string with a Pike syntax representation of the entity.
  string print() { return printModifiers() + objtype; }
}

//! Representation of an inherit.
class Inherit {
  //!
  inherit PikeObject;

  //!
  constant objtype = "inherit";

  //! Name of the class that is inherited.
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

//! Representation of an import.
class Import {
  //!
  inherit Inherit;

  //!
  constant objtype = "import";

  string print() {
    return ::print() + " " + classname;
  }
}

//! Base class for representing classes, modules and namespaces.
//!
//! @seealso
//!   @[Class], @[Module], @[NameSpace], @[AutoDoc], @[Modifier]
class _Class_or_Module {
  //!
  inherit PikeObject;

  string directory = 0;
  string file = 0;

  //! @note
  //!   The documentation appears as a child of the <class> or <module>
  Documentation documentation;

  //! @[Inherit]s and @[Import]s that affect the symbol lookup
  //! for the entity.
  array(Inherit|Import) inherits = ({ });

  //! Entities that are children to this entity.
  array(_Class_or_Module) children = ({ });

  //! Documented entities that are children to this entity.
  array(DocGroup) docGroups = ({ });

  //! @returns
  //!   Returns @expr{1@} if there is any documentation
  //!   at all for this entity.
  int containsDoc() { return documentation != 0 || sizeof(docGroups) > 0; }

  //! Adds @[p] to the set of @[inherits].
  void AddInherit(PikeObject p) {
    inherits += ({ p });
  }

  //! Adds @[c] to the set of @[children].
  void AddChild(_Class_or_Module c) { children += ({ c }); }

  //! @returns
  //!   Returns the first child with the name @[name] if any.
  PikeObject findChild(string name) {
    int a = Array.search_array(children,
                               lambda(PikeObject o, string n) {
                                 return o->name == n;
                               }, name);
    if (a < 0) return 0;
    return children[a];
  }

  //! @returns
  //!   Returns the first @[DocGroup] that documents an entity
  //!   with the name @[name] if any.
  DocGroup findObject(string name) {
    int a = Array.search_array(docGroups,
                               lambda(DocGroup d, string n) {
                                 return (search(d->objects->name, n) >= 0);
                               }, name);
    if (a < 0) return 0;
    return docGroups[a];
  }

  //! Adds @[d] to the set of @[docGroups].
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

  protected string _sprintf(int c)
  {
    switch(c) {
    case 's': return xml();
    case 'O': return sprintf("%O(%O)", this_program, name);
    }
  }
}

//! Represents a class.
class Class {
  //!
  inherit _Class_or_Module;

  //!
  constant objtype = "class";
}

//! Represents a module.
class Module {
  //!
  inherit _Class_or_Module;

  //!
  constant objtype = "module";
}

//! Represents a name space, eg: @expr{predef::@} or @expr{lfun::@}.
class NameSpace {
  //!
  inherit _Class_or_Module;

  //!
  constant objtype = "namespace";
}

//! The top-level container.
//! This container should only contain namespaces,
//! and they in turn contain modules etc.
class AutoDoc {
  //!
  inherit _Class_or_Module;

  //!
  constant objtype = "autodoc";
}

//! A modifier range, e.g.:
//! @code
//! static private {
//!   ...
//!   <<declarations>>
//!   ...
//! }
//! @endcode
class Modifier {
  //!
  inherit _Class_or_Module;

  //!
  constant objtype = "modifier";
}

//! Represents a function.
class Method {
  //!
  inherit PikeObject;

  //! The names of the arguments.
  array(string) argnames;

  //! The types for the arguments.
  array(Type) argtypes;

  //! The return type for the function.
  Type returntype;

  //!
  constant objtype = "method";

  string xml() {
    string s = standardTags() + "\n";
    string args = "";
    for(int i = 0; i < sizeof(argnames); ++i) {
      if (argtypes[i])
        args += xmltag("argument",
                       argnames[i]&&([ "name" : argnames[i] ]),
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

//! Represents a named constant.
class Constant {
  //!
  inherit PikeObject;

  //!
  constant objtype = "constant";

  //! Typedef @[Type] if it is a typedef.
  Type typedefType = 0;

  string xml() {
    return standardStart() + standardTags()
      + (typedefType ? xmltag("typevalue", typedefType->xml()) : "")
      + standardEnd();
  }
  string print() {
    return ::print() + " " + name;
  }
}

//! Represents a typedef.
class Typedef {
  //!
  inherit PikeObject;

  //!
  constant objtype = "typedef";

  //! Typedef @[Type].
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

//! The values inside @expr{enum Foo { ... }@}
class EnumConstant {
  //!
  inherit PikeObject;

  //!
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

//! The enum container.
class Enum {
  //!
  inherit PikeObject;

  //!
  constant objtype = "enum";

  //! Mimic the @expr{class { ... }@} behaviour.
  Documentation documentation;

  //! The set of children.
  array(DocGroup) children = ({ });

  //! Adds @[c] to the set of @[children].
  void addChild(DocGroup c) { children += ({ c }); }

  string xml() {

    // need some special handling to make this look as if it
    // were produced by some other stuff
    string s =  standardStart() + standardTags();
    array(SimpleNode) inDocGroups = ({});

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

      SimpleNode doc =
	simple_parse_input("<doc>"+documentation->xml+"</doc>")->
	get_first_element();

      foreach (doc->get_children(), SimpleNode group) {
        if (group->get_node_type() == XML_ELEMENT
            && group->get_any_name() == "group")
        {
          int constants = 0;
          string homogenName = 0;
          SimpleNode docGroupNode =
	    SimpleNode(XML_ELEMENT, "docgroup",
		       ([ "homogen-type" : "constant" ]), 0);
          SimpleNode text = 0;
          foreach (group->get_children(), SimpleNode child)
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
              SimpleNode d = SimpleNode(XML_ELEMENT, "doc", ([]), 0);
              d->add_child(text);
              docGroupNode->add_child(d);
            }
            inDocGroups += ({ docGroupNode });
          }
        }
      }
      s += doc->html_of_node();
    }

    foreach (children, DocGroup docGroup)
      s += docGroup->xml();
    foreach (inDocGroups, SimpleNode n)
      s += n->html_of_node();

    s += standardEnd();
    return s;
  }

  string print() {
    return "enum";
  }
}

//! Represents a variable.
class Variable {
  //!
  inherit PikeObject;

  //!
  constant objtype = "variable";

  //! @[Type] of the variable.
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
