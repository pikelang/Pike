#charset iso-8859-1
#pike __REAL_VERSION__

/*
 */

//! XML parser that generates node-trees.
//!
//! Has some support for XML namespaces
//! @url{http://www.w3.org/TR/REC-xml-names/@} @rfc{2518:23.4@}.
//!
//! @note
//!   This module defines two sets of node trees;
//!   the @[SimpleNode]-based, and the @[Node]-based.
//!   The main difference between the two, is that
//!   the @[Node]-based trees have parent pointers,
//!   which tend to generate circular data references
//!   and thus garbage.
//!
//!   There are some more subtle differences between
//!   the two. Please read the documentation carefully.

//!
constant STOP_WALK = -1;

//!
constant XML_ROOT     = 0x0001;

//!
constant XML_ELEMENT  = 0x0002;

//!
constant XML_TEXT     = 0x0004;

//!
constant XML_HEADER   = 0x0008;

//!
constant XML_PI       = 0x0010;

//!
constant XML_COMMENT  = 0x0020;

//!
constant XML_DOCTYPE  = 0x0040;

//! Attribute nodes are created on demand
constant XML_ATTR     = 0x0080;    //  Attribute nodes are created on demand

//!
constant DTD_ENTITY   = 0x0100;

//!
constant DTD_ELEMENT  = 0x0200;

//!
constant DTD_ATTLIST  = 0x0400;

//!
constant DTD_NOTATION = 0x0800;

//!
constant XML_NODE     = (XML_ROOT | XML_ELEMENT | XML_TEXT |
                       XML_PI | XML_COMMENT | XML_ATTR);
#define STOP_WALK  -1
#define  XML_ROOT     0x0001
#define  XML_ELEMENT  0x0002
#define  XML_TEXT     0x0004
#define  XML_HEADER   0x0008
#define  XML_PI       0x0010
#define  XML_COMMENT  0x0020
#define  XML_DOCTYPE  0x0040
#define  XML_ATTR     0x0080     //  Attribute nodes are created on demand
#define  DTD_ENTITY   0x0100
#define  DTD_ELEMENT  0x0200
#define  DTD_ATTLIST  0x0400
#define  DTD_NOTATION 0x0800
#define  XML_NODE     (XML_ROOT | XML_ELEMENT | XML_TEXT |    \
					   XML_PI | XML_COMMENT | XML_ATTR)

constant type_names = ([
    XML_ROOT : "ROOT",
    XML_ELEMENT : "ELEMENT",
    XML_TEXT : "TEXT",
    XML_HEADER : "HEADER",
    XML_PI : "PI",
    XML_COMMENT : "COMMENT",
    XML_DOCTYPE : "DOCTYPE",
    XML_ATTR : "ATTR",
    DTD_ENTITY: "!ENTITY",
    DTD_ELEMENT: "!ELEMENT",
    DTD_ATTLIST:  "!ATTLIST",
    DTD_NOTATION: "!NOTATION",
]);

string get_type_name(int type)
{
    if( type_names[type] ) return type_names[type];
    return (string)type;
}

//! Flags used together with @[simple_parse_input()] and
//! @[simple_parse_file()].
enum ParseFlags {
  PARSE_WANT_ERROR_CONTEXT =		0x1,
#define PARSE_WANT_ERROR_CONTEXT	0x1
  PARSE_FORCE_LOWERCASE =		0x2,
#define PARSE_FORCE_LOWERCASE		0x2
  PARSE_ENABLE_NAMESPACES =		0x4,
#define PARSE_ENABLE_NAMESPACES		0x4
  // Negated flag for compatibility.
  PARSE_DISALLOW_RXML_ENTITIES =	0x8,
#define PARSE_DISALLOW_RXML_ENTITIES	0x8

  PARSE_COMPAT_ALLOW_ERRORS_7_2 =	0x10,
#define PARSE_COMPAT_ALLOW_ERRORS_7_2	0x10
  PARSE_COMPAT_ALLOW_ERRORS_7_6 =	0x20,
#define PARSE_COMPAT_ALLOW_ERRORS_7_6	0x20
  // The following exists for compatibility only.
  PARSE_CHECK_ALL_ERRORS =		0,
}

//! Quotes the string given in @[data] by escaping &, < and >.
string text_quote(string data)
{
  return replace(data, ([ "&":"&amp;",
			  "<":"&lt;",
			  ">":"&gt;" ]) );
}

//! Quotes strings just like @[text_quote], but entities in the form
//! @tt{&foo.bar;@} will not be quoted.
string roxen_text_quote(string data) {
  string out = "";
  int pos, opos;
  while ((pos = search(data, "&", pos)) >= 0) {
    if ((sscanf(data[pos..], "&%[^\n\r\t <>;&];%*s", string entity) == 2) &&
	search(entity, ".") >= 1 &&
	!has_suffix(entity, ".")) {
      out += text_quote(data[opos..pos - 1]) + "&" + entity + ";";
      pos += sizeof(entity) + 2;
    } else {
      out += text_quote(data[opos..pos]);
      pos++;
    }
    opos = pos;
  }
  return out + text_quote(data[opos..]);
}

//! Quotes the string given in @[data] by escaping &, <, >, ' and ".
string attribute_quote(string data, void|string ignore)
{
  mapping m = ([ "\"":"&quot;",
                 "'":"&apos;",
                 "&":"&amp;",
                 "\n":"&#10;",
                 "\r":"&#13;",
                 "<":"&lt;",
                 ">":"&gt;" ]);
  if( ignore ) m_delete(m, ignore);
  return replace(data, m);
}

//! Quotes strings just like @[attribute_quote], but entities in the
//! form @tt{&foo.bar;@} will not be quoted.
string roxen_attribute_quote(string data, void|string ignore)
{
  mapping m = ([ "\"":"&quot;",
                 "'":"&apos;" ]);
  if( ignore ) m_delete(m, ignore);
  return replace(roxen_text_quote(data), m);
}

void throw_error(string str, mixed ... args)
{
  //  Put message in debug log and throw exception
  str = "Parser.XML.Tree: " + str;
  error(str, @args);
}

//! Namespace aware parser.
class XMLNSParser {
  ADT.Stack namespace_stack = ADT.Stack();

  protected void create()
  {
    // Sentinel and default namespaces.
    namespace_stack->push(([
			    "xml":"http://www.w3.org/XML/1998/namespace",
			    "xmlns":"http://www.w3.org/2000/xmlns/",
			  ]));
  }

  //! Check @[attrs] for namespaces.
  //!
  //! @returns
  //!   Returns the namespace expanded version of @[attrs].
  mapping(string:string) Enter(mapping(string:string) attrs)
  {
    mapping(string:string) namespaces = namespace_stack->top() + ([]);
    foreach(attrs; string attr; string val) {
      if (attr == "xmlns") {
	if (val == "") error("Bad namespace specification (%O=\"\")\n", attr);
	if ((val == "http://www.w3.org/2000/xmlns/") ||
	    (val == "http://www.w3.org/XML/1998/namespace")) {
	  error("Invalid namespace declaration.\n");
	}
	namespaces[0] = val;
      } else if (has_prefix(attr, "xmlns:")) {
	if (val == "") error("Bad namespace specification (%O=\"\")\n", attr);
	if ((val == "http://www.w3.org/2000/xmlns/") ||
	    (val == "http://www.w3.org/XML/1998/namespace") ||
	    (attr == "xmlns:xml") || (attr == "xmlns:xmlns")) {
	  if ((attr != "xmlns:xml") ||
	      (val != "http://www.w3.org/XML/1998/namespace")) {
	    error("Invalid namespace declaration.\n");
	  }
	}
	namespaces[attr[6..]] = val;
      }
    }
    namespace_stack->push(namespaces);
    // Now that we know what the namespaces are, we
    // can expand the namespaces in the other attributes.
    mapping(string:string) result = ([]);
    foreach(attrs; string attr; string val) {
      int i;
      if (!has_prefix(attr, "xmlns:") && (i = search(attr, ":")) >= 0) {
	string key = attr[..i-1];
	attr = attr[i+1..];
	string prefix = namespaces[key];
	if (!prefix) {
	  error("Unknown namespace %O for attribute %O\n",
		key, attr);
	}
	attr = prefix+attr;
      }
      result[attr] = val;
    }
    return result;
  }

  string Decode(string name)
  {
    int i = search(name, ":");
    string key;
    if (i >= 0) {
      key = name[..i-1];
      name = name[i+1..];
    }
    if ((key || name) == "xmlns") {
      if (key) name = key + ":" + name;
      return name;
    }
    string prefix = namespace_stack->top()[key];
    if (!prefix) {
      if (key) {
	error("Unknown namespace %O for tag %O\n",
	      key, name);
      } else {
	error("No default namespace, and tag without namespace: %O\n",
	      name);
      }
    }
    return prefix + name;
  }

  string Encode(string name)
  {
    string longest;
    string best;
    foreach(namespace_stack->top(); string ns; string prefix) {
      if (has_prefix(name, prefix) &&
	  (!longest || sizeof(prefix) > sizeof(longest))) {
	longest = prefix;
	best = ns;
      }
    }
    if (!longest) {
      error("No namespace containing tag %O found.\n", name);
    }
    name = name[sizeof(longest)..];
    if (best) return best + ":" + name;
    return name;
  }

  void Leave()
  {
    namespace_stack->pop();
  }
}

//! Base class for nodes.
class AbstractSimpleNode {
  //  Private member variables
  /* protected */ array(AbstractSimpleNode) mChildren = ({ });

  //! Returns all the nodes children.
  array(AbstractSimpleNode) get_children() { return (mChildren); }

  //! Returns the number of children of the node.
  int count_children()           { return (sizeof(mChildren)); }

  //! Returns an initialized copy of the node.
  //! @note
  //!   The returned node has no children.
  optional AbstractSimpleNode low_clone() {
    return AbstractSimpleNode();
  }

  //! Returns a clone of the sub-tree rooted in the node.
  optional AbstractSimpleNode clone() {
    AbstractSimpleNode n = low_clone();
    foreach(mChildren, AbstractSimpleNode child)
      n->add_child( child->clone() );
    return n;
  }

  //! Returns the last child node or zero.
  object(AbstractSimpleNode)|zero get_last_child()
  {
    if (!sizeof(mChildren))
      return 0;
    else
      return (mChildren[-1]);
  }

  //! The [] operator indexes among the node children, so
  //! @expr{node[0]@} returns the first node and @expr{node[-1]@} the last.
  //! @note
  //!   The [] operator will select a node from all the nodes children,
  //!   not just its element children.
  protected object(AbstractSimpleNode)|zero `[](int pos)
  {
    //  Treat pos as index into array
    if ((pos < 0) || (pos > sizeof(mChildren) - 1))
      return 0;
    return (mChildren[pos]);
  }

  //! Adds the given node to the list of children of this node. The
  //! new node is added last in the list.
  //!
  //! @note
  //!   The return value differs from the one returned
  //!   by @[Node()->add_child()].
  //!
  //! @returns
  //!   The current node.
  AbstractSimpleNode add_child(AbstractSimpleNode c)
  {
    mChildren += ({ c });

    //  Let caller get the node back for easy chaining of calls
    return this;
  }

  //! Adds the node @[c] to the list of children of this node. The
  //! node is added before the node @[old], which is assumed to be an
  //! existing child of this node. The node is added last if @[old] is
  //! zero.
  //!
  //! @returns
  //!   The current node.
  AbstractSimpleNode add_child_before (AbstractSimpleNode c,
				       AbstractSimpleNode old)
  {
    if (old) {
      int index = search (mChildren, old);
      mChildren = mChildren[..index - 1] + ({c}) + mChildren[index..];
    }
    else
      mChildren += ({c});
    return this;
  }

  //! Adds the node @[c] to the list of children of this node. The
  //! node is added after the node @[old], which is assumed to be an
  //! existing child of this node. The node is added first if @[old]
  //! is zero.
  //!
  //! @returns
  //!   The current node.
  AbstractSimpleNode add_child_after (AbstractSimpleNode c,
				      AbstractSimpleNode old)
  {
    if (old) {
      int index = search (mChildren, old);
      mChildren = mChildren[..index] + ({c}) + mChildren[index + 1..];
    }
    else
      mChildren = ({c}) + mChildren;
    return this;
  }

  //! Removes all occurrences of the provided node from the list of
  //! children of this node.
  void remove_child(AbstractSimpleNode c)
  {
    mChildren -= ({ c });
  }

  //! Replaces the nodes children with the provided ones.
  void replace_children(array(AbstractSimpleNode) children) {
    mChildren = children;
  }


  //! Replaces the first occurrence of the old node child with
  //! the new node child or children.
  //! @note
  //!   The return value differs from the one returned
  //!   by @[Node()->replace_child()].
  //! @returns
  //!   Returns the current node on success, and @expr{0@} (zero)
  //!   if the node @[old] wasn't found.
  object(AbstractSimpleNode)|zero
    replace_child(AbstractSimpleNode old,
                  AbstractSimpleNode|array(AbstractSimpleNode) new)
  {
    int index = search(mChildren, old);
    if (index < 0)
      return 0;
    if( arrayp(new) )
      mChildren = mChildren[..index-1] + new + mChildren[index+1..];
    else
      mChildren[index] = new;
    return this;
  }

  //! Destruct the tree recursively. When the inheriting
  //! @[AbstractNode] or @[Node] is used, which have parent pointers,
  //! this function should be called for every tree that no longer is
  //! in use to avoid frequent garbage collector runs.
  void zap_tree()
  {
    if (mChildren)
      // Avoid mChildren->zap_tree() since applying an array causes
      // pike to recurse more heavily on the C stack than a normal
      // function call.
      foreach (mChildren, AbstractSimpleNode child)
	child && child->zap_tree();
    destruct (this);
  }

  //! Traverse the node subtree in preorder, root node first, then
  //! subtrees from left to right, calling the callback function
  //! for every node. If the callback function returns @[STOP_WALK]
  //! the traverse is promptly aborted and @[STOP_WALK] is returned.
  int walk_preorder(function(AbstractSimpleNode, mixed ...:int|void) callback,
		    mixed ... args)
  {
    if (callback(this, @args) == STOP_WALK)
      return STOP_WALK;
    foreach(mChildren, AbstractSimpleNode c)
      if (c->walk_preorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
  }

  //! Traverse the node subtree in preorder, root node first, then
  //! subtrees from left to right. For each node we call @[cb_1]
  //! before iterating through children, and then @[cb_2]
  //! (which always gets called even if the walk is aborted earlier).
  //! If the callback function returns @[STOP_WALK] the traverse
  //! decend is aborted and @[STOP_WALK] is returned once all waiting
  //! @[cb_2] functions have been called.
  int walk_preorder_2(function(AbstractSimpleNode, mixed ...:int|void) cb_1,
		      function(AbstractSimpleNode, mixed ...:int|void) cb_2,
		      mixed ... args)
  {
    int  res;

    res = cb_1(this, @args);
    if (!res)
      foreach(mChildren, AbstractSimpleNode c)
	res = res || c->walk_preorder_2(cb_1, cb_2, @args);
    return (cb_2(this, @args) || res);
  }

  //! Traverse the node subtree in inorder, left subtree first, then
  //! root node, and finally the remaining subtrees, calling the function
  //! @[callback] for every node. If the function @[callback] returns
  //! @[STOP_WALK] the traverse is promptly aborted and @[STOP_WALK]
  //! is returned.
  int walk_inorder(function(AbstractSimpleNode, mixed ...:int|void) callback,
		   mixed ... args)
  {
    if (sizeof(mChildren) > 0)
      if (mChildren[0]->walk_inorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
    if (callback(this, @args) == STOP_WALK)
      return STOP_WALK;
    foreach(mChildren[1..], AbstractSimpleNode c)
      if (c->walk_inorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
  }

  //! Traverse the node subtree in postorder, first subtrees from left
  //! to right, then the root node, calling the function @[callback]
  //! for every node. If the function @[callback] returns @[STOP_WALK]
  //! the traverse is promptly aborted and @[STOP_WALK] is returned.
  int walk_postorder(function(AbstractSimpleNode,
                              mixed ...:int|void) callback,
		     mixed ... args)
  {
    foreach(mChildren, AbstractSimpleNode c)
      if (c->walk_postorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
    if (callback(this, @args) == STOP_WALK)
      return STOP_WALK;
  }

  //! Iterates over the nodes children from left to right, calling the
  //! function @[callback] for every node. If the callback function
  //! returns @[STOP_WALK] the iteration is promptly aborted and
  //! @[STOP_WALK] is returned.
  int iterate_children(function(AbstractSimpleNode,
                                mixed ...:int|void) callback,
		       mixed ... args)
  {
    foreach(mChildren, AbstractSimpleNode c)
      if (callback(c, @args) == STOP_WALK)
	return STOP_WALK;
  }

  //! Returns a list of all descendants in document order. Includes
  //! this node if @[include_self] is set.
  array(AbstractSimpleNode) get_descendants(int(0..1) include_self)
  {
    array   res;

    //  Walk subtrees in document order and add to resulting list
    res = include_self ? ({ this }) : ({ });
    foreach(mChildren, AbstractSimpleNode child) {
      res += child->get_descendants(1);
    }
    return (res);
  }

  //! Optional factory for creating contained nodes.
  //!
  //! @param type
  //!   Type of node to create. One of:
  //!   @int
  //!     @value XML_TEXT
  //!       XML text. @[text] contains a string with the text.
  //!     @value XML_COMMENT
  //!       XML comment. @[text] contains a string with the comment text.
  //!     @value XML_HEADER
  //!       @tt{<?xml?>@}-header @[attr] contains a mapping with
  //!       the attributes.
  //!     @value XML_PI
  //!       XML processing instruction. @[name] contains the name of the
  //!       processing instruction and @[text] the remainder.
  //!     @value XML_ELEMENT
  //!       XML element tag. @[name] contains the name of the tag and
  //!       @[attr] the attributes.
  //!     @value XML_DOCTYPE
  //!     @value DTD_ENTITY
  //!     @value DTD_ELEMENT
  //!     @value DTD_ATTLIST
  //!     @value DTD_NOTATION
  //!       DTD information.
  //!   @endint
  //!
  //! @param name
  //!   Name of the tag if applicable.
  //!
  //! @param attr
  //!   Attributes for the tag if applicable.
  //!
  //! @param text
  //!   Contained text of the tab if any.
  //!
  //! This function is called during parsning to create the various
  //! XML nodes.
  //!
  //! Define this function to provide application-specific XML nodes.
  //!
  //! @returns
  //!   Returns one of
  //!   @mixed
  //!     @type AbstractSimpleNode
  //!       A node object representing the XML tag.
  //!     @type int(0..0)
  //!       @expr{0@} (zero) if the subtree rooted here should be cut.
  //!     @type zero
  //!       @expr{UNDEFINED@} to fall back to the next level of parser
  //!       (ie behave as if this function does not exist).
  //!   @endmixed
  //!
  //! @note
  //!   This function is only relevant for @[XML_ELEMENT] nodes.
  //!
  //! @note
  //!   This function is not available in Pike 7.6 and earlier.
  //!
  //! @note
  //!   In Pike 8.0 and earlier this function was only called in
  //!   root nodes.
  optional this_program node_factory(int type, string name,
				     mapping attr, string text);
}

//! Base class for nodes with parent pointers.
class AbstractNode {
  inherit AbstractSimpleNode;
  //  Private member variables
  /* protected */ object(AbstractNode)|zero           mParent = 0;

  @Pike.Annotations.Implements(AbstractSimpleNode);

  // Instruct Pike.count_memory to search three steps: mChildren (in
  // VirtualNode also mAttrNodes) -> array value -> mParent.
  constant pike_cycle_depth = 3;

  //  Public methods

  //! Sets the parent node to @[parent].
  void set_parent(AbstractNode parent)    { mParent = parent; }

  //! Returns the parent node.
  AbstractNode get_parent()          { return (mParent); }

#if 0
  protected void create()
  {
    error("Creating a plain AbstractNode.\n");
  }
#endif /* 0 */

  //! Returns an initialized copy of the node.
  //! @note
  //!   The returned node has no children, and no parent.
  optional AbstractNode low_clone()
  {
    return AbstractNode();
  }

  //! Clones the node, optionally connected to parts of the tree.
  //! If direction is -1 the cloned nodes parent will be set, if
  //! direction is 1 the clone nodes childen will be set.
  AbstractNode clone(void|int(-1..1) direction) {
    AbstractNode n = low_clone();
    if(mParent && direction!=1)
      n->set_parent( mParent->clone(-1) );
    if(direction!=-1)
      foreach(mChildren, AbstractNode child)
	n->add_child( child->clone(1) );
    return n;
  }

  //! Follows all parent pointers and returns the root node.
  AbstractNode get_root()
  {
    AbstractNode  parent, node;

    parent = this;
    while (node = parent->mParent)
      parent = node;
    return (parent);
  }

  //! Adds the node @[c] to the list of children of this node. The
  //! node is added before the node @[old], which is assumed to be an
  //! existing child of this node. The node is added first if @[old]
  //! is zero.
  //!
  //! @note
  //!   Returns the new child node, NOT the current node.
  //!
  //! @returns
  //! The new child node is returned.
  AbstractNode add_child(AbstractNode c)
  {
    c->mParent = ::add_child(c);

    //  Let caller get the new node back for easy chaining of calls
    return (c);
  }

  //! Adds the node @[c] to the list of children of this node. The
  //! node is added before the node @[old], which is assumed to be an
  //! existing child of this node. The node is added last if @[old] is
  //! zero.
  //!
  //! @returns
  //!   The current node.
  AbstractNode add_child_before (AbstractNode c, AbstractNode old)
  {
    return c->mParent = ::add_child_before (c, old);
  }

  //! Adds the node @[c] to the list of children of this node. The
  //! node is added after the node @[old], which is assumed to be an
  //! existing child of this node. The node is added first if @[old]
  //! is zero.
  //!
  //! @returns
  //!   The current node.
  AbstractNode add_child_after (AbstractNode c, AbstractNode old)
  {
    return c->mParent = ::add_child_after (c, old);
  }

  //! Variants of @[add_child], @[add_child_before] and
  //! @[add_child_after] that doesn't set the parent pointer in the
  //! newly added children.
  //!
  //! This is useful while building a node tree, to get efficient
  //! refcount garbage collection if the build stops abruptly.
  //! @[fix_tree] has to be called on the root node when the building
  //! is done.
  AbstractNode tmp_add_child(AbstractNode c)
  {
    ::add_child (c); return c;
  }
  AbstractNode tmp_add_child_before (AbstractNode c, AbstractNode old)
  {
    return ::add_child_before (c, old);
  }
  AbstractNode tmp_add_child_after (AbstractNode c, AbstractNode old)
  {
    return ::add_child_after (c, old);
  }

  //! Fix all parent pointers recursively in a tree that has been
  //! built with @[tmp_add_child].
  void fix_tree()
  {
    foreach (mChildren, AbstractNode c)
      if (c->mParent != this) {
	c->mParent = this;
	c->fix_tree();
      }
  }

  //! Removes all occurrences of the provided node from the called nodes
  //! list of children. The removed nodes parent reference is set to null.
  void remove_child(AbstractNode c)
  {
    ::remove_child(c);
    c->mParent = 0;
  }

  //! Removes this node from its parent. The parent reference is set to null.
  void remove_node() {
    mParent->remove_child(this);
  }

  //! Replaces the nodes children with the provided ones. All parent
  //! references are updated.
  void replace_children(array(AbstractNode) children) {
    foreach(mChildren, AbstractNode c)
      c->mParent = 0;
    ::replace_children(children);
    foreach(mChildren, AbstractNode c)
      c->mParent = this;
  }


  //! Replaces the first occurrence of the old node child with the new
  //! node child or children. All parent references are updated.
  //! @note
  //!   The returned value is NOT the current node.
  //! @returns
  //!   Returns the new child node.
  object(AbstractNode)|zero
    replace_child(AbstractNode old,
                  AbstractNode|array(AbstractNode) new)
  {
    if (!::replace_child(old, new)) return 0;
    new->mParent = this;
    old->mParent = 0;
    return new;
  }

  //! Replaces this node with the provided one.
  //! @returns
  //!   Returns the new node.
  AbstractNode|array(AbstractNode)
    replace_node(AbstractNode|array(AbstractNode) new)
  {
    mParent->replace_child(this, new);
    return new;
  }

  //! Returns all preceding siblings, i.e. all siblings present before
  //! this node in the parents children list.
  array(AbstractNode) get_preceding_siblings()
  {
    array  siblings;
    int    pos;

    //  Get parent list of children and locate ourselves
    if (!mParent)
      return ({ });
    siblings = mParent->get_children();
    pos = search(siblings, this);

    //  Return array in reverse order not including self
    return (reverse(siblings[..(pos - 1)]));
  }

  //! Returns all following siblings, i.e. all siblings present after
  //! this node in the parents children list.
  array(AbstractNode) get_following_siblings()
  {
    array  siblings;
    int    pos;

    //  Get parent list of children and locate ourselves
    if (!mParent)
      return ({ });
    siblings = mParent->get_children();
    pos = search(siblings, this);

    //  Select array range
    return (siblings[(pos + 1)..]);
  }

  //! Returns all siblings, including this node.
  array(AbstractNode) get_siblings()
  {
    //  If not found we return ourself only
    if (!mParent)
      return ({ this });
    return (mParent->get_children());
  }

  //! Returns a list of all ancestors, with the top node last.
  //! The list will start with this node if @[include_self] is set.
  array(AbstractNode) get_ancestors(int(0..1) include_self)
  {
    array     res;
    AbstractNode  node;

    //  Repeat until we reach the top
    res = include_self ? ({ this }) : ({ });
    node = this;
    while (node = node->get_parent())
      res += ({ node });
    return (res);
  }

  //! Returns all preceding nodes, excluding this nodes ancestors.
  array(AbstractNode) get_preceding()
  {
    AbstractNode   node, root, self;
    array      res = ({ });

    //  Walk tree from root until we find ourselves and add all preceding
    //  nodes. We should return the nodes in reverse document order.
    self = this;
    root = get_root();
    root->walk_preorder(
			lambda(AbstractNode n) {
			  //  Have we reached our own node?
			  if (n == self)
			    return STOP_WALK;
			  else
			    res = ({ n }) + res;
			});

    //  Finally remove all of our ancestors
    root = this;
    while (node = root->get_parent()) {
      root = node;
      res -= ({ node });
    }
    return (res);
  }

  //! Returns all the nodes that follows after the current one.
  array(AbstractNode) get_following()
  {
    array      siblings;
    AbstractNode   node;
    array      res = ({ });

    //  Add subtrees from right-hand siblings and keep walking towards
    //  the root of the tree.
    node = this;
    do {
      siblings = node->get_following_siblings();
      foreach(siblings, AbstractNode n) {
	n->walk_preorder(
			 lambda(AbstractNode n) {
			   //  Add to result
			   res += ({ n });
			 });
      }

      node = node->get_parent();
    } while (node);
    return (res);
  }
}

//!  Node in XML tree
protected class VirtualNode {
  //  Member variables for this node type
  protected int            mNodeType;
  protected string		mShortNamespace = "";	// Namespace prefix
  protected string		mNamespace;	// Resolved namespace
  protected string         mTagName;
  protected mapping(string:string) mAttributes;		// Resolved attributes
  protected mapping(string:string)	mShortAttributes;	// Shortened attributes
  protected array(Node) mAttrNodes;   //  created on demand
  protected string|zero    mText;
  protected int            mDocOrder;

  // Functions implemented via multiple inheritance.
  array(AbstractNode) get_children();
  int walk_preorder(function(AbstractSimpleNode, mixed ...:int|void) callback,
		    mixed ... args);

  protected VirtualNode low_clone()
  {
    return this_program(get_node_type(), get_full_name(),
			get_attributes(), get_text());
  }

  //  This can be accessed directly by various methods to cache parsing
  //  info for faster processing. Some use it for storing flags and others
  //  use it to cache reference objects.
  public mixed           mNodeData = 0;

  //  Public methods
  //! Returns this nodes attributes, which can be altered
  //! destructivly to alter the nodes attributes.
  //!
  //! @seealso
  //!   @[replace_attributes()]
  mapping(string:string) get_attributes()   { return mAttributes||([]); }

  //! Replace the entire set of attributes.
  //!
  //! @seealso
  //!   @[get_attributes()]
  void replace_attributes(mapping(string:string) attrs)
  {
    if (mShortAttributes && mShortAttributes == mAttributes)
      mShortAttributes = attrs;
    mAttributes = attrs;
  }

  //! Returns this nodes name-space adjusted attributes.
  //!
  //! @note
  //!   @[set_short_namespaces()] or @[set_short_attributes()] must
  //!   have been called before calling this function.
  mapping get_short_attributes()   { return (mShortAttributes); }

  //! Sets this nodes name-space adjusted attributes.
  void set_short_attributes(mapping short_attrs)
  {
    mShortAttributes = short_attrs;
  }

  //! Returns the node type. See defined node type constants.
  int get_node_type()        { return (mNodeType); }

  //! Returns text content in node.
  string|zero get_text()          { return (mText); }

  //! Change the text content destructively.
  string set_text(string txt) {
    if( mNodeType & (XML_TEXT|XML_ATTR|XML_PI|XML_COMMENT|DTD_ENTITY) )
      mText = txt;
  }

  //!
  int get_doc_order()        { return (mDocOrder); }

  //!
  void set_doc_order(int o)  { mDocOrder = o; }

  //! Returns the name of the element node, or the nearest element above if
  //! an attribute node.
  string get_tag_name()
  {
    return mTagName;
  }

  //! Return name of tag or name of attribute node.
  string get_any_name()
  {
    return (mTagName);
  }

  //! Change the tag name destructively. Can only be used on element and
  //! processing-instruction nodes.
  void set_tag_name(string name)
  {
    if (mNodeType & (XML_ELEMENT | XML_PI))
      mTagName = name;
  }

  //! Return the (resolved) namespace for this node.
  string get_namespace()
  {
    return mNamespace;
  }

  //! Return fully qualified name of the element node.
  string get_full_name()
  {
    return mNamespace + mTagName;
  }

  string get_short_name()
  {
    return mShortNamespace + mTagName;
  }

  //!
  protected void create(int type, string|zero name, mapping|zero attr,
			string|zero text)
  {
    if (name) {
      if (has_value(name, ":") && sscanf (name, "%*[^/:]%*c") == 2) {
	sscanf(reverse(name), "%[^/:]", mTagName);
	mTagName=reverse(mTagName);
	mShortNamespace = mNamespace = name[..<sizeof(mTagName)];
      }
      else {
	mTagName = name;
	mNamespace = "";
      }
    }
    mNodeType = type;
    mAttributes = attr;
    if( mNodeType & (XML_TEXT|XML_ATTR|XML_PI|XML_COMMENT|DTD_ENTITY) ) {
      if (!text) {
	error("Text missing for node of type %s.\n",
	      get_type_name(mNodeType));
      }
      mText = text;
    } else {
      if (text && (text != "")) {
	error("Nodes of type %s do not support text.\n",
	      get_type_name(mNodeType));
      }
      mText = UNDEFINED;
    }
    mAttrNodes = 0;
  }

  //! If the node is an attribute node or a text node, its value is returned.
  //! Otherwise the child text nodes are concatenated and returned.
  string value_of_node()
  {
    string  str = "";

    switch (mNodeType) {
    case XML_ATTR:
    case XML_TEXT:
      //  If attribute node we return attribute value. For text nodes we
      //  return (you guessed it!) text.
      return (mText);

    default:
      //  Concatenate text children
      walk_preorder(lambda(Node n) {
		      if (n->get_node_type() == XML_TEXT)
			str += n->get_text();
		    });
      return (str);
    }
  }

  //! Returns the first element child to this node.
  //!
  //! @param name
  //!   If provided, the first element child with that name is returned.
  //!
  //! @param full
  //!   If specified, name matching will be done against the full name.
  //!
  //! @returns
  //!   Returns the first matching node, and 0 if no such node was found.
  object(AbstractNode)|zero
    get_first_element(string|void name, int(0..1)|void full)
  {
    if (!name) {
      foreach(get_children(), AbstractNode c)
	if(c->get_node_type()==XML_ELEMENT)
	  return c;
    } else if (!full) {
      foreach(get_children(), AbstractNode c)
	if(c->get_node_type()==XML_ELEMENT &&
	   c->get_tag_name()==name)
	  return c;
    } else {
      foreach(get_children(), AbstractNode c)
	if(c->get_node_type()==XML_ELEMENT &&
	   c->get_full_name()==name)
	  return c;
    }
    return 0;
  }

  //! Returns all element children to this node.
  //!
  //! @param name
  //!   If provided, only elements with that name is returned.
  //!
  //! @param full
  //!   If specified, name matching will be done against the full name.
  //!
  //! @returns
  //!   Returns an array with matching nodes.
  array(AbstractNode) get_elements(string|void name, int(0..1)|void full) {
    if (!name) {
      return filter(get_children(),
		    lambda(Node n) {
		      return n->get_node_type()==XML_ELEMENT;
		    } );
    } else if (!full) {
      return filter(get_children(),
		    lambda(Node n) {
		      return n->get_node_type()==XML_ELEMENT &&
			n->get_tag_name()==name;
		    } );
    } else {
      return filter(get_children(),
		    lambda(Node n) {
		      return n->get_node_type()==XML_ELEMENT &&
			n->get_full_name()==name;
		    } );
    }
  }

  // It doesn't produce html, and not of the node only.
  // Note: Returns wide data!
  string html_of_node(void|int(0..1) preserve_roxen_entities)
  {
    String.Buffer data = String.Buffer();
    set_short_namespaces();
    if(preserve_roxen_entities)
      low_render_xml(data, this, roxen_text_quote, roxen_attribute_quote);
    else
      low_render_xml(data, this, text_quote, attribute_quote);
    return (string)data;
  }

  //! It is possible to cast a node to a string, which will return
  //! @[render_xml()] for that node.
  protected mixed cast(string to) {
    if(to=="string") return render_xml();
    return UNDEFINED;
  }

  // FIXME: Consider moving this to the corresponding base node classes?
  protected void low_render_xml(String.Buffer data, Node n,
			     function(string:string) textq,
                             function(string,void|string:string) attrq,
                             void|mapping(string:string) namespace_lookup,
                             void|int(0..3) quote_mode)
  {
    string tagname;
    switch(n->get_node_type()) {
    case XML_TEXT:
      data->add(textq(n->get_text()));
      break;

    case XML_ELEMENT:
      if (!sizeof(tagname = n->get_short_name()))
	break;

      data->add("<", tagname);
      if (mapping attr = n->get_short_attributes()) {
        foreach(sort(indices(attr)), string a) {
          int quote = quote_mode & 1; // 0=', 1="
          if( !(quote_mode & 2) ) {
            if( !quote && has_value(attr[a], "\'") )
              quote = 1;
            else if( quote && has_value(attr[a], "\"") )
              quote = 0;
          }
          if( quote )
            data->add(" ", a, "=\"", attrq(attr[a], "'"), "\"");
          else
            data->add(" ", a, "='", attrq(attr[a], "\""), "'");
        }
      }
      if (n->count_children())
	data->add(">");
      else
	data->add("/>");
      break;

    case XML_HEADER:
      data->add("<?xml");
      if (mapping attr = n->get_attributes() + ([])) {
	// version and encoding must come in the correct order.
	// version must always be present.
	if (attr->version)
	  data->add(" version='", attrq(attr->version), "'");
	else
	  data->add(" version='1.0'");
	m_delete(attr, "version");
	if (attr->encoding)
	  data->add(" encoding='", attrq(attr->encoding), "'");
	m_delete(attr, "encoding");
	foreach(sort(indices(attr)), string a)
	  data->add(" ", a, "='", attrq(attr[a]), "'");
      }
      data->add("?>");
      break;

    case XML_PI:
      data->add("<?", n->get_short_name());
      string text = n->get_text();
      if (sizeof(text))
	data->add(" ", text);
      data->add("?>");
      break;

    case XML_COMMENT:
      data->add("<!--", n->get_text(), "-->");
      break;

    case XML_DOCTYPE:
      data->add("<!DOCTYPE ", n->get_short_name());
      mapping attrs = n->get_attributes();
      if (attrs->PUBLIC) {
	data->sprintf(" PUBLIC %O %O",
                      attrs->PUBLIC, attrs->SYSTEM || "");
      } else if (attrs->SYSTEM) {
	data->sprintf(" SYSTEM %O", attrs->SYSTEM);
      }
      if (n->count_children()) {
	// Use the raw internal subset if available.
	if (attrs->internal_subset) {
	  data->add(" [", attrs->internal_subset, "]>");
	  return;
	} else {
	  // We need to render the DTD by hand.
	  // NOTE: Any formatting (including PEReferences) of the
	  //       originating DTD will be lost.
	  data->add(" [\n");
	}
      } else {
	data->add(">");
      }
      break;

    case DTD_ENTITY:
      tagname = n->get_short_name();
      if (tagname[0] == '%') {
	data->add("  <!ENTITY % ", tagname[1..], " ");
      } else {
	data->add("  <!ENTITY ", tagname, " ");
      }
      data->add("\"", attribute_quote(n->get_text()), "\" >\n");
      break;

    case DTD_ELEMENT:
      data->add("  <!ELEMENT ", n->get_short_name(), " ");
      n->render_expression(data);
      data->add(" >\n");
      break;
    }

    // FIXME: The following code is probably only relevant to
    //        XML_ELEMENT and XML_DOCTYPE nodes. Consider moving
    //        it to the corresponding cases above.

    array(Node) children = n->get_children();
    foreach(children, Node n) {
      low_render_xml(data, n, textq, attrq, namespace_lookup, quote_mode);
    }

    if (n->get_node_type() == XML_ELEMENT) {
      if (n->count_children())
	if (sizeof(tagname))
	  data->add("</", tagname, ">");
    } else if ((n->get_node_type() == XML_DOCTYPE) && (n->count_children())) {
      data->add(" ]>");
    }
  }

  // Get the encoding from the XML-header (if any).
  //
  // Create a new XML-header if there's none.
  //
  // Add an encoding attribute if there is none.
  protected string get_encoding()
  {
    Node xml_header;
    if (sizeof(get_children()) &&
	(xml_header = get_children()[0])->get_node_type()==XML_HEADER) {
      string encoding;
      if (encoding = xml_header->get_attributes()->encoding) {
	return encoding;
      }
      xml_header->get_attributes()->encoding = "utf-8";
    }
    return "utf-8";
  }

  void set_short_namespaces(void|mapping(string:string) forward_lookup,
			    void|mapping(string:string) backward_lookup)
  {
    if (!mTagName) return;
    if (!forward_lookup) {
      forward_lookup = ([
	"http://www.w3.org/XML/1998/namespace":"xml:",
	"http://www.w3.org/2000/xmlns/":"xmlns:",
      ]);
      backward_lookup = ([
	"xml:":"http://www.w3.org/XML/1998/namespace",
	"xmlns:":"http://www.w3.org/2000/xmlns/",
      ]);
    } else {
      // Make sure changes aren't propagated backwards...
      forward_lookup += ([]);
      backward_lookup += ([]);
    }
    // First check if any namespaces are defined by this tag.
    mapping attrs = get_attributes() || ([]);
    mapping short_attrs = attrs + ([]);
    foreach(indices(attrs), string attr_name) {
      if (has_prefix(attr_name, "xmlns")) {
	string short_prefix = "";
	if (has_prefix(attr_name, "xmlns:")) {
	  short_prefix = attr_name[6..] + ":";
	}
	if (backward_lookup[short_prefix]) {
	  m_delete(forward_lookup, backward_lookup[short_prefix]);
	}
	backward_lookup[short_prefix] = attrs[attr_name];
	forward_lookup[attrs[attr_name]] = short_prefix;
      }
    }
    if (!mShortAttributes) {
      // Then set the short namespace for this tag.
      mShortNamespace = "";
      if (sizeof(mNamespace)) {
	if (!(mShortNamespace = forward_lookup[mNamespace])) {
#if 0
	  werror("Forward_lookup: %O\n"
		 "Backward_lookup: %O\n"
		 "mNamespace:%O\n",
		 forward_lookup,
		 backward_lookup,
		 mNamespace);
#endif /* 0 */

	  string found;
	  string full_name = get_full_name();
	  // Check if there are any longer namespaces that might match.
	  foreach(forward_lookup; string long;) {
	    if (has_prefix(full_name, long) &&
		(!found || (sizeof(found) < sizeof(long)))) {
	      found = long;
	      break;
	    }
	  }

	  if (found) {
#if 0
	    werror("Found: NS: %O <%s%s/>\n",
		   found, forward_lookup[found], full_name[sizeof(found)..]);
#endif /* 0 */
	    mTagName = full_name[sizeof(found)..];
	    mNamespace = found;
	    mShortNamespace = forward_lookup[found];
	  } else {
#if 0
	    werror("No suitable namespace found for %O.\n",
		   full_name);
#endif /* 0 */
	    // We need to allocate a short namespace symbol.
	    // FIXME: This is O(n�).
	    int i;
	    while(backward_lookup[mShortNamespace = ("NS"+i+":")]) {
	      i++;
	    }
	    backward_lookup[mShortNamespace] = mNamespace;
	    forward_lookup[mNamespace] = mShortNamespace;
	    attrs["xmlns:NS"+i] = mNamespace;
	    short_attrs["xmlns:NS"+i] = mNamespace;
	  }
	}
      }
#if 0
      werror("attrs: %O\n"
	     "short attrs: %O\n",
	     attrs, short_attrs);
#endif /* 0 */
      // Then set the short namespaces for any attributes.
      foreach(indices(attrs), string attr_name) {
	if (!has_prefix(attr_name, "xmlns:")) {
	  int i = -1;
	  int j;
	  while ((j = search(attr_name, ":", i + 1)) >= 0) {
	    i = j;
	  }
	  while ((j = search(attr_name, "/", i + 1)) >= 0) {
	    i = j;
	  }
	  if (i >= 0) {
	    string ns = attr_name[..i];
	    string prefix;

	    // Check if we already have some namespace that is a longer
	    // prefix of this attribute than ns. This isn't only for
	    // looks; there are broken XML parsers that require the
	    // break between the namespace and the attribute name to be
	    // at a specific spot, e.g. the one used in the WebDAV
	    // client in MS XP Pro.
	    foreach (forward_lookup; string long;)
	      if (sizeof (long) > sizeof (ns) &&
		  has_prefix (attr_name, long)) {
		ns = long;
		i = sizeof (long) - 1;
		break;
	      }

	    if (!(prefix = forward_lookup[ns])) {
	      // We need to allocate a short namespace symbol.
#if 0
	      werror("Namespace %O not found in %O\n",
		     ns, forward_lookup);
#endif /* 0 */
	      // FIXME: This is O(n�).
	      int i;
	      while(backward_lookup[prefix = ("NS"+i+":")]) {
		i++;
	      }
	      backward_lookup[prefix] = ns;
	      forward_lookup[mNamespace] = prefix;
	      attrs["xmlns:NS"+i] = ns;
	      short_attrs["xmlns:NS"+i] = ns;
#if 0
	      werror("New namespace: %O %O\n", prefix, ns);
	      werror("Forward_lookup: %O\n"
		     "Backward_lookup: %O\n"
		     "mNamespace:%O\n",
		     forward_lookup,
		     backward_lookup,
		     mNamespace);
#endif /* 0 */
	    }
	    m_delete(short_attrs, attr_name);
	    short_attrs[prefix + attr_name[i+1..]] = attrs[attr_name];
	  }
	}
      }
      mShortAttributes = short_attrs;
    }
    // And then do it for all the children.
    get_children()->set_short_namespaces(forward_lookup, backward_lookup);
  }

  protected Charset.Encoder get_encoder(string encoding)
  {
    return Charset.encoder(encoding)->set_replacement_callback(lambda(string c)
      {
        return sprintf("&#x%x;", c[0]);
      });
  }

  //! Creates an XML representation of the node sub tree. If the
  //! flag @[preserve_roxen_entities] is set, entities on the form
  //! @tt{&foo.bar;@} will not be escaped.
  //!
  //! @param namespace_lookup
  //!   Mapping from namespace prefix to namespace symbol prefix.
  //!
  //! @param encoding
  //!   Force a specific output character encoding. By default the
  //!   encoding set in the document XML processing instruction will
  //!   be used, with UTF-8 as a fallback. Setting this value will
  //!   change the XML processing instruction, if present.
  //!
  //! @param quote_mode
  //!   @int
  //!     @value 0
  //!       Defaults to single quote, but use double quote if it
  //!       avoids escaping.
  //!     @value 1
  //!       Defaults to double quote, but use single quote if it
  //!       avoids escaping.
  //!     @value 2
  //!       Use only single quote.
  //!     @value 3
  //!       Use only double quote.
  //!   @endint
  string render_xml(void|int(0..1) preserve_roxen_entities,
                    void|mapping(string:string) namespace_lookup,
                    void|string encoding, void|int(0..3) quote_mode)
  {
    String.Buffer data = String.Buffer();
    if( encoding )
    {
      Node xml_header;
      if (sizeof(get_children()) &&
          (xml_header = get_children()[0])->get_node_type()==XML_HEADER)
        xml_header->get_attributes()->encoding=encoding;
    }
    else
      encoding = get_encoding();

    set_short_namespaces();
    if(preserve_roxen_entities)
      low_render_xml(data, this, roxen_text_quote,
		     roxen_attribute_quote,
                     namespace_lookup, quote_mode);
    else
      low_render_xml(data, this, text_quote, attribute_quote,
                     namespace_lookup, quote_mode);
    return get_encoder(encoding)->feed((string)data)->drain();
  }

  //! Creates an XML representation for the node sub tree and streams
  //! the output to the file @[f]. If the flag @[preserve_roxen_entities]
  //! is set, entities on the form @tt{&foo.bar;@} will not be escaped.
  void render_to_file(Stdio.File f,
		      void|int(0..1) preserve_roxen_entities) {
    object data = class (Stdio.File f, object encoder) {
      void add(string ... args) {
	encoder->feed(args[*]);
	f->write(encoder->drain());
      }
    } (f, get_encoder(get_encoding()));
    set_short_namespaces();
    if(preserve_roxen_entities)
      low_render_xml(data, this, roxen_text_quote,
                     roxen_attribute_quote);
    else
      low_render_xml(data, this, text_quote, attribute_quote);
  }

  /*protected*/ void _add_to_text (string str)
  // Only to be used internally from the parse callback.
  {
    mText += str;
  }

  protected string _sprintf(int t) {
    return t=='O' && sprintf("%O(#%d:%s,%O)", this_program, mDocOrder,
			     get_type_name(get_node_type()), get_full_name());
  }
}

//! XML node without parent pointers and attribute nodes.
class SimpleNode
{
  inherit AbstractSimpleNode;
  inherit VirtualNode;

  @Pike.Annotations.Implements(AbstractSimpleNode);
  @Pike.Annotations.Implements(VirtualNode);

  // Needed for cross-overloading
  optional SimpleNode low_clone()
  {
    return VirtualNode::low_clone();
  }
}


//! XML node with parent pointers.
class Node
{
  inherit AbstractNode;
  inherit VirtualNode;

  @Pike.Annotations.Implements(AbstractNode);
  @Pike.Annotations.Implements(VirtualNode);

  // Needed for cross-overloading
  optional Node low_clone()
  {
    return VirtualNode::low_clone();
  }

  //! Returns the name of the element node, or the nearest element above if
  //! an attribute node.
  string get_tag_name()
  {
    //  Fake ATTR nodes query their parent
    return ((mNodeType == XML_ATTR) ? get_parent()->get_tag_name() : mTagName);
  }

  //! Returns the name of the attribute node.
  string get_attr_name()
  {
    //  Only works for fake ATTR nodes
    return ((mNodeType == XML_ATTR) ? mTagName : "");
  }

  //! Creates and returns an array of new nodes; they will not be
  //! added as proper children to the parent node, but the parent
  //! link in the nodes are set so that upwards traversal is made
  //! possible.
  array(Node) get_attribute_nodes()
  {
    Node   node;
    int       node_num;

    //  Return if already computed
    if (mAttrNodes)
      return (mAttrNodes);

    //  Only applicable for XML_ROOT and XML_ELEMENT
    if ((mNodeType != XML_ROOT) && (mNodeType != XML_ELEMENT))
      return ({ });

    //  After creating these nodes we need to give them node numbers
    //  which harmonize with the existing numbers. Fortunately we
    //  inserted a gap in the series when first numbering the original
    //  nodes.
    mAttrNodes = ({ });
    node_num = get_doc_order() + 1;
    foreach(indices(mAttributes), string attr) {
      node = AttributeNode(attr, mAttributes[attr]);
      node->set_parent(this);
      node->set_doc_order(node_num++);
      mAttrNodes += ({ node });
    }
    return (mAttrNodes);
  }

  //  Override AbstractNode::`[]
  protected Node `[](string|int pos)
  {
    //  If string indexing we find attributes which match the string
    if (stringp(pos)) {
      //  Make sure attribute node list is instantiated
      array(Node) attr = get_attribute_nodes();

      //  Find attribute name
      foreach(attr, Node n)
	if (n->get_attr_name() == pos)
	  return (n);
      return (0);
    } else
      //  Call inherited method
      return (AbstractNode::`[](pos));
  }
}

string report_error_context(string data, int ofs)
{
  string pre = reverse(data[..ofs - 1]);
  string post = data[ofs..];
  sscanf(pre, "%s\n", pre);
  pre = reverse(pre);
  sscanf(post, "%s\n", post);
  return "\nContext: " + pre + post + "\n";
}

// Used for debugging...
class WrappedSimple
{
  inherit .Simple;

  string lookup_entity(string entity)
  {
    string ret = ::lookup_entity(entity);
    werror("lookup_entity(%O) ==> %O\n", entity, ret);
    return ret;
  }

  void define_entity_raw(string entity, string raw)
  {
    werror("define_entity_raw(%O, %O)\n", entity, raw);
    ::define_entity_raw(entity, raw);
  }
}

//! Mixin for parsing XML.
//!
//! Uses @[Parser.XML.Simple] to perform
//! the actual parsing.
class XMLParser
{
  this_program add_child(this_program);
  void create(int, string, mapping, string);

  this_program doctype_node;

  protected ADT.Stack container_stack = ADT.Stack();

  void parse(string data,
             void|mapping predefined_entities,
             ParseFlags|void flags,
	     string|void default_namespace)
  {
    //.Simple xp = WrappedSimple();
    .Simple xp = .Simple();

    if (!(flags & PARSE_DISALLOW_RXML_ENTITIES))
      xp->allow_rxml_entities(1);

    if (flags & PARSE_COMPAT_ALLOW_ERRORS_7_2)
      xp->compat_allow_errors ("7.2");
    else if (flags & PARSE_COMPAT_ALLOW_ERRORS_7_6)
      xp->compat_allow_errors ("7.6");

    //  Init parser with predefined entities
    if (predefined_entities)
      foreach(indices(predefined_entities), string entity)
        xp->define_entity_raw(entity, predefined_entities[entity]);

    // Construct tree from string
    mixed err = catch
    {
      mapping(string:mixed) extras = ([]);
      if (flags & PARSE_FORCE_LOWERCASE) {
        extras->force_lc = 1;
      }
      if (flags & PARSE_ENABLE_NAMESPACES) {
        extras->xmlns = XMLNSParser();
	if (default_namespace) {
	  // Set the default namespace.
	  extras->xmlns->namespace_stack->top()[0] = default_namespace;
	}
      }
      catch( data=xp->autoconvert(data) );
      container_stack = ADT.Stack();
      foreach(xp->parse(data, parse_xml_callback,
                        sizeof(extras) && extras),
              this_program child)
        add_child(child);
    };

    if(err)
    {
      //  If string msg is found we propagate the error. If error message
      //  contains " [Offset: 4711]" we add the input data to the string.
      if (stringp(err) && (flags & PARSE_WANT_ERROR_CONTEXT))
      {
        if (sscanf(err, "%s [Offset: %d]", err, int ofs) == 2)
          err += report_error_context(data, ofs);
      }
      throw(err);
    }
  }

  //! Factory for creating nodes.
  //!
  //! @param type
  //!   Type of node to create. One of:
  //!   @int
  //!     @value XML_TEXT
  //!       XML text. @[text] contains a string with the text.
  //!     @value XML_COMMENT
  //!       XML comment. @[text] contains a string with the comment text.
  //!     @value XML_HEADER
  //!       @tt{<?xml?>@}-header @[attr] contains a mapping with
  //!       the attributes.
  //!     @value XML_PI
  //!       XML processing instruction. @[name] contains the name of the
  //!       processing instruction and @[text] the remainder.
  //!     @value XML_ELEMENT
  //!       XML element tag. @[name] contains the name of the tag and
  //!       @[attr] the attributes.
  //!     @value XML_DOCTYPE
  //!     @value DTD_ENTITY
  //!     @value DTD_ELEMENT
  //!     @value DTD_ATTLIST
  //!     @value DTD_NOTATION
  //!       DTD information.
  //!   @endint
  //!
  //! @param name
  //!   Name of the tag if applicable.
  //!
  //! @param attr
  //!   Attributes for the tag if applicable.
  //!
  //! @param text
  //!   Contained text of the tab if any.
  //!
  //! This function is called during parsning to create the various
  //! XML nodes.
  //!
  //! Overload this function to provide application-specific XML nodes.
  //!
  //! @returns
  //!   Returns a node object representing the XML tag,
  //!   or @expr{0@} (zero) if the subtree rooted in the
  //!   tag should be cut.
  //!
  //! @note
  //!   This function is not available in Pike 7.6 and earlier.
  //!
  //! @seealso
  //!   @[node_factory_dispatch()], @[AbstractSimpleNode()->node_factory()]
  protected AbstractSimpleNode node_factory(int type, string name,
					    mapping attr, string text);

  //! Dispatcher of @[node_factory()].
  //!
  //! This function finds a suitable @[node_factory()] given the
  //! current parser context to call with the same arguments.
  protected AbstractSimpleNode node_factory_dispatch(int type, string name,
						     mapping|zero attr, string text)
  {
    foreach(reverse(values(container_stack)), AbstractNode n) {
      if (!n || !n->node_factory) continue;
      AbstractSimpleNode res = n->node_factory(type, name, attr, text);
      if (!undefinedp(res)) return res;
    }
    return node_factory(type, name, attr, text);
  }

  protected AbstractSimpleNode|zero
    parse_xml_callback(string type, string name,
                       mapping attr, string|array contents,
                       mixed location, mixed ...extra)
  {
    AbstractSimpleNode node;
    mapping|zero short_attr = attr;

    switch (type) {
    case "":
    case "<![CDATA[":
      //  Create text node
      return node_factory_dispatch(XML_TEXT, "", 0, contents);

    case "<!--":
      //  Create comment node
      return node_factory_dispatch(XML_COMMENT, "", 0, contents);

    case "<?xml":
      //  XML header tag
      return node_factory_dispatch(XML_HEADER, "", attr, "");

    case "<!ENTITY":
      return node_factory_dispatch(DTD_ENTITY, name, attr, contents);
    case "<!ELEMENT":
      return node_factory_dispatch(DTD_ELEMENT, name, 0, contents);
    case "<!ATTLIST":
      return node_factory_dispatch(DTD_ATTLIST, name, attr, contents);
    case "<!NOTATION":
      return node_factory_dispatch(DTD_NOTATION, name, attr, contents);
    case "<!DOCTYPE":
      return node_factory_dispatch(XML_DOCTYPE, name, attr, contents);

    case "<?":
      //  XML processing instruction
      return node_factory_dispatch(XML_PI, name, attr, contents);

    case "<>":
      //  Create new tag node.
      if (arrayp(extra) && sizeof(extra) && mappingp(extra[0])) {
        //  Convert tag and attribute names to lowercase
        //  if requested.
        if (extra[0]->force_lc) {
          name = lower_case(name);
          attr = mkmapping(map(indices(attr), lower_case),
                           values(attr));
	  short_attr = attr;
        }
        //  Parse namespace information of available.
        if (extra[0]->xmlns) {
          XMLNSParser xmlns = extra[0]->xmlns;
          attr = xmlns->Enter(attr);
          name = xmlns->Decode(name);
          xmlns->Leave();
	  short_attr = UNDEFINED;
	}
      }
      node = node_factory_dispatch(XML_ELEMENT, name, attr, "");
      if (node && short_attr) node->set_short_attributes([mapping]short_attr);
      return node;

    case ">":
      //  Create tree node for this container
      if (arrayp(extra) && sizeof(extra) && mappingp(extra[0])) {
        //  Convert tag and attribute names to lowercase
        //  if requested.
        if (extra[0]->force_lc) {
          name = lower_case(name);
          attr = mkmapping(map(indices(attr), lower_case), values(attr));
	  short_attr = attr;
        }
        //  Parse namespace information if available.
        if (extra[0]->xmlns) {
          XMLNSParser xmlns = extra[0]->xmlns;
          name = xmlns->Decode(name);
	  attr = mkmapping(map(indices(attr), xmlns->Decode), values(attr));
          xmlns->Leave();
	  short_attr = UNDEFINED;
        }
      }
      node = container_stack->pop();

      if (node) {
	// FIXME: Validate that the node has the expected content.

	//  Add children to our tree node. We need to merge consecutive text
	//  children since two text elements can't be neighbors according to
	//  the W3 spec. This is necessary since CDATA sections are
	//  converted to text nodes which might need to be concatenated
	//  with neighboring text nodes.
	Node text_node;
	int(0..1) modified;

	if (short_attr) node->set_short_attributes([mapping]short_attr);

	foreach(contents; int i; Node child) {
	  if (child->get_node_type() == XML_TEXT) {
	    if (text_node) {
	      //  Add this text string to the previous text node.
	      text_node->_add_to_text (child->get_text());
	      contents[i]=0;
	      modified=1;
	    }
	    else
	      text_node = child;
	  } else
	    text_node = 0;
	}

	if( modified )
	  contents -= ({ 0 });
	node->replace_children( contents );
      }
      return (node);

    case "error":
      //  Error message present in contents. If "location" is present in the
      //  "extra" mapping we encode that value in the message string so the
      //  handler for this throw() can display the proper error context.
      if (name) {
        // We append the name of the tag that caused the error to be triggered.
        contents += sprintf(" [Tag: %O]", name);
      }
      if (location && mappingp(location))
        throw_error(contents + " [Offset: " + location->location + "]\n");
      else
        throw_error(contents + "\n");

    case "<":
      if (arrayp(extra) && sizeof(extra) && mappingp(extra[0]) &&
          extra[0]->xmlns) {
        XMLNSParser xmlns = extra[0]->xmlns;
        attr = xmlns->Enter(attr);
	name = xmlns->Decode(name);
      }
      container_stack->push(node_factory_dispatch(XML_ELEMENT, name, attr, ""));
      return 0;

    default:
      // werror("Unknown XML type: %O: %O, %O\n", type, attr, contents);
      return 0;
    }
  }
}

//
// --- Compatibility below this point
//

//! Takes an XML string and produces a @[SimpleNode] tree.
SimpleRootNode simple_parse_input(string data,
				  void|mapping predefined_entities,
				  ParseFlags|void flags,
				  string|void default_namespace)
{
  return SimpleRootNode(data, predefined_entities, flags,
			default_namespace);
}

//! Loads the XML file @[path], creates a @[SimpleNode] tree representation and
//! returns the root node.
SimpleRootNode simple_parse_file(string path,
				 void|mapping predefined_entities,
				 ParseFlags|void flags,
				 string|void default_namespace)
{
  Stdio.File  file = Stdio.File(path, "r");
  string      data;

  //  Try loading file and parse its contents
  if(catch {
    data = file->read();
    file->close();
  })
    throw_error("Could not read XML file %O.\n", path);
  else
    return simple_parse_input(data, predefined_entities, flags,
			      default_namespace);
}

//! Takes an XML string and produces a node tree.
//!
//! @note
//! @[flags] is not used for @[PARSE_WANT_ERROR_CONTEXT],
//! @[PARSE_FORCE_LOWERCASE] or @[PARSE_ENABLE_NAMESPACES] since they
//! are covered by the separate flag arguments.
RootNode parse_input(string data, void|int(0..1) no_fallback,
		     void|int(0..1) force_lowercase,
		     void|mapping(string:string) predefined_entities,
		     void|int(0..1) parse_namespaces,
		     ParseFlags|void flags)
{
    if(no_fallback)
        flags |= PARSE_WANT_ERROR_CONTEXT;

    if(force_lowercase)
        flags |= PARSE_FORCE_LOWERCASE;

    if(parse_namespaces)
        flags |= PARSE_ENABLE_NAMESPACES;

    return RootNode(data, predefined_entities, flags);
}

//! Loads the XML file @[path], creates a node tree representation and
//! returns the root node.
Node parse_file(string path, int(0..1)|void parse_namespaces)
{
  Stdio.File  file = Stdio.File(path, "r");
  string      data;

  //  Try loading file and parse its contents
  if(catch {
    data = file->read();
    file->close();
  })
    throw_error("Could not read XML file %O.\n", path);
  else
    return parse_input(data, 0, 0, 0, parse_namespaces);
}

protected class DTDElementHelper
{
  array expression;
  array get_expression()
  {
    return expression;
  }

  void low_render_expression(String.Buffer data, array|string expr)
  {
    if (stringp(expr)) {
      data->add(expr);
    } else if ((<"?", "*", "+">)[expr[0]]) {
      // Postfix operator.
      low_render_expression(data, expr[1]);
      data->add(expr[0]);
    } else if (expr[0] == "#PCDATA") {
      // Special case...
      if (sizeof(expr) == 1) {
	data->add("(#PCDATA)");
      } else {
	data->add("(#PCDATA");
	foreach(expr[1..], string e) {
	  data->add(" | ", e);
	}
	data->add(")*");
      }
    } else {
      // Infix operator.
      data->add("(");
      foreach(expr[1..]; int ind; array|string e) {
	if (ind) data->add(" ", expr[0], " ");
	low_render_expression(data, e);
      }
      data->add(")");
    }
  }

  void render_expression(String.Buffer data)
  {
    low_render_expression(data, expression);
  }
}

// Convenience stuff for creation of @[SimpleNode]s.

//! The root node of an XML-tree consisting of @[SimpleNode]s.
class SimpleRootNode
{
  inherit SimpleNode;
  inherit XMLParser;

  @Pike.Annotations.Implements(SimpleNode);
  // @Pike.Annotations.Implements(XMLParser);

  protected mapping(string:SimpleElementNode) node_ids;

  //! Find the element with the specified id.
  //!
  //! @param id
  //!   The XML id of the node to search for.
  //!
  //! @param force
  //!   Force a regeneration of the id lookup cache.
  //!   Needed the first time after the node tree has been
  //!   modified by adding or removing element nodes, or
  //!   by changing the id attribute of an element node.
  //!
  //! @returns
  //!   Returns the element node with the specified id
  //!   if any. Returns @[UNDEFINED] otherwise.
  //!
  //! @seealso
  //!   @[flush_node_id_cache]
  SimpleElementNode get_element_by_id(string id, int|void force)
  {
    if (!node_ids || force) {
      mapping(string:SimpleNode) new_lookup = ([]);
      walk_preorder(lambda(SimpleNode node,
			   mapping(string:SimpleElementNode) new_lookup) {
		      if (node->get_node_type() != XML_ELEMENT) return 0;
		      if (string id = node->get_attributes()->id) {
			if (new_lookup[id]) {
			  error("Multiple nodes with the same id "
				"(id: %O, nodes: %O, %O)\n",
				id, new_lookup[id], node);
			}
			new_lookup[id] = node;
		      }
		      return 0;
		    }, new_lookup);
      node_ids = new_lookup;
    }
    return node_ids[id];
  }

  void flush_node_id_cache()
  //! Clears the node id cache built and used by @[get_element_by_id].
  {
    node_ids = 0;
  }

  protected SimpleNode low_clone()
  {
    return SimpleRootNode();
  }

  protected SimpleNode node_factory(int type, string name,
				    mapping attr, string|array text)
  {
    switch(type) {
    case XML_TEXT: return SimpleTextNode(text);
    case XML_COMMENT: return SimpleCommentNode(text);
    case XML_HEADER: return SimpleHeaderNode(attr);
    case XML_PI: return SimplePINode(name, attr, text);
    case XML_ELEMENT: return SimpleElementNode(name, attr);
    case XML_DOCTYPE: return SimpleDoctypeNode(name, attr, text);
    case DTD_ENTITY: return SimpleDTDEntityNode(name, attr, text);
    case DTD_ELEMENT: return SimpleDTDElementNode(name, text);
    case DTD_ATTLIST: return SimpleDTDAttlistNode(name, attr, text);
    case DTD_NOTATION: return SimpleDTDNotationNode(name, attr, text);
    default: return SimpleNode(type, name, attr, text);
    }
  }

  //!
  protected void create(string|void data,
		     mapping|void predefined_entities,
		     ParseFlags|void flags,
		     string|void default_namespace)
  {
    ::create(XML_ROOT, "", 0, "");
    if (data) {
      parse(data, predefined_entities, flags, default_namespace);
    }
  }
}

//!
class SimpleTextNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleTextNode(get_text());
  }

  //!
  protected void create(string text)
  {
    ::create(XML_TEXT, "", 0, text);
  }
}

//!
class SimpleCommentNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleCommentNode(get_text());
  }

  //!
  protected void create(string comment)
  {
    ::create(XML_COMMENT, "", 0, comment);
  }
}

//!
class SimpleHeaderNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleHeaderNode(get_attributes());
  }

  //!
  protected void create(mapping(string:string) attrs)
  {
    ::create(XML_HEADER, "", attrs, "");
  }
}

//!
class SimplePINode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimplePINode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(XML_PI, name, attrs, contents);
  }
}

//!
class SimpleDoctypeNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleDoctypeNode(get_full_name(), get_attributes(), 0);
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     array|zero contents)
  {
    ::create(XML_DOCTYPE, name, attrs, "");
    if (contents) {
      replace_children(contents);
    }
  }
}

//!
class SimpleDTDEntityNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleDTDEntityNode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(DTD_ENTITY, name, attrs, contents);
  }
}

//!
class SimpleDTDElementNode
{
  inherit SimpleNode;
  inherit DTDElementHelper;

  @Pike.Annotations.Implements(SimpleNode);
  @Pike.Annotations.Implements(DTDElementHelper);

  protected SimpleNode low_clone()
  {
    return SimpleDTDElementNode(get_full_name(), get_expression());
  }

  //!
  protected void create(string name, array expression)
  {
    this::expression = expression;
    ::create(DTD_ELEMENT, name, 0, "");
  }
}

//!
class SimpleDTDAttlistNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleDTDAttlistNode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(DTD_ATTLIST, name, attrs, contents);
  }
}

//!
class SimpleDTDNotationNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleDTDNotationNode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(DTD_NOTATION, name, attrs, contents);
  }
}

//!
class SimpleElementNode
{
  inherit SimpleNode;

  @Pike.Annotations.Implements(SimpleNode);

  protected SimpleNode low_clone()
  {
    return SimpleElementNode(get_full_name(), get_attributes());
  }

  //!
  protected void create(string name, mapping(string:string) attrs)
  {
    ::create(XML_ELEMENT, name, attrs, "");
  }
}

// Convenience stuff for creation of @[Node]s.

//! The root node of an XML-tree consisting of @[Node]s.
class RootNode
{
  inherit Node;
  inherit XMLParser;

  @Pike.Annotations.Implements(Node);
  // @Pike.Annotations.Implements(XMLParser);

  protected mapping(string:ElementNode) node_ids;

  //! Find the element with the specified id.
  //!
  //! @param id
  //!   The XML id of the node to search for.
  //!
  //! @param force
  //!   Force a regeneration of the id lookup cache.
  //!   Needed the first time after the node tree has been
  //!   modified by adding or removing element nodes, or
  //!   by changing the id attribute of an element node.
  //!
  //! @returns
  //!   Returns the element node with the specified id
  //!   if any. Returns @[UNDEFINED] otherwise.
  //!
  //! @seealso
  //!   @[flush_node_id_cache]
  ElementNode get_element_by_id(string id, int|void force)
  {
    if (!node_ids || force) {
      mapping(string:ElementNode) new_lookup = ([]);
      walk_preorder(lambda(SimpleNode node,
			   mapping(string:ElementNode) new_lookup) {
		      if (node->get_node_type() != XML_ELEMENT) return 0;
		      if (string id = node->get_attributes()->id) {
			if (new_lookup[id]) {
			  error("Multiple nodes with the same id "
				"(id: %O, nodes: %O, %O)\n",
				id, new_lookup[id], node);
			}
			new_lookup[id] = node;
		      }
		      return 0;
		    }, new_lookup);
      node_ids = new_lookup;
    }
    return node_ids[id];
  }

  void flush_node_id_cache()
  //! Clears the node id cache built and used by @[get_element_by_id].
  {
    node_ids = 0;
  }

  protected Node low_clone()
  {
    return RootNode();
  }

  protected Node node_factory(int type, string name,
			      mapping attr, string|array text)
  {
    switch(type) {
    case XML_TEXT: return TextNode(text);
    case XML_COMMENT: return CommentNode(text);
    case XML_HEADER: return HeaderNode(attr);
    case XML_PI: return PINode(name, attr, text);
    case XML_ELEMENT: return ElementNode(name, attr);
    case XML_DOCTYPE: return DoctypeNode(name, attr, text);
    case DTD_ENTITY: return DTDEntityNode(name, attr, text);
    case DTD_ELEMENT: return DTDElementNode(name, text);
    case DTD_ATTLIST: return DTDAttlistNode(name, attr, text);
    case DTD_NOTATION: return DTDNotationNode(name, attr, text);
    default: return Node(type, name, attr, text);
    }
  }

  //!
  protected void create(string|void data,
		     mapping|void predefined_entities,
		     ParseFlags|void flags)
  {
    ::create(XML_ROOT, "", 0, "");
    if (data) {
      parse(data, predefined_entities, flags);
    }
  }
}

//!
class TextNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return TextNode(get_text());
  }

  //!
  protected void create(string text)
  {
    ::create(XML_TEXT, "", 0, text);
  }
}

//!
class CommentNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return CommentNode(get_text());
  }

  //!
  protected void create(string text)
  {
    ::create(XML_COMMENT, "", 0, text);
  }
}

//!
class HeaderNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return HeaderNode(get_attributes());
  }

  //!
  protected void create(mapping(string:string) attrs)
  {
    ::create(XML_HEADER, "", attrs, "");
  }
}

//!
class PINode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return PINode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(XML_PI, name, attrs, contents);
  }
}

//!
class DoctypeNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return DoctypeNode(get_full_name(), get_attributes(), 0);
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     array|zero contents)
  {
    ::create(XML_DOCTYPE, name, attrs, "");
    if (contents) {
      replace_children(contents);
    }
  }
}

//!
class DTDEntityNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return DTDEntityNode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(DTD_ENTITY, name, attrs, contents);
  }
}

//!
class DTDElementNode
{
  inherit Node;
  inherit DTDElementHelper;

  @Pike.Annotations.Implements(Node);
  @Pike.Annotations.Implements(DTDElementHelper);

  protected Node low_clone()
  {
    return DTDElementNode(get_full_name(), get_expression());
  }

  //!
  protected void create(string name, array expression)
  {
    this::expression = expression;
    ::create(DTD_ELEMENT, name, 0, "");
  }
}

//!
class DTDAttlistNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return DTDAttlistNode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(DTD_ATTLIST, name, attrs, contents);
  }
}

//!
class DTDNotationNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return DTDNotationNode(get_full_name(), get_attributes(), get_text());
  }

  //!
  protected void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(DTD_NOTATION, name, attrs, contents);
  }
}

//!
class ElementNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return ElementNode(get_full_name(), get_attributes());
  }

  //!
  protected void create(string name, mapping(string:string) attrs)
  {
    ::create(XML_ELEMENT, name, attrs, "");
  }
}

//!
class AttributeNode
{
  inherit Node;

  @Pike.Annotations.Implements(Node);

  protected Node low_clone()
  {
    return AttributeNode(get_full_name(), get_text());
  }

  //!
  protected void create(string name, string value)
  {
    ::create(XML_ATTR, name, 0, value);
  }
}
