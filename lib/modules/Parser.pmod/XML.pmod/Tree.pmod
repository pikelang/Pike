#pike __REAL_VERSION__

/*
 * $Id: Tree.pmod,v 1.40 2004/04/20 19:25:22 grubba Exp $
 *
 */

//! XML parser that generates a node-tree.
//!
//! Has some support for XML namespaces
//! @url{http://www.w3.org/TR/REC-xml-names/@}
//! RFC 2518 23.4.

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
#define  XML_NODE     (XML_ROOT | XML_ELEMENT | XML_TEXT |    \
					   XML_PI | XML_COMMENT | XML_ATTR)

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
	search(entity, ".") >= 0) {
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
string attribute_quote(string data)
{
  return replace(data, ([ "\"":"&quot;",
			  "'":"&apos;",
			  "&":"&amp;",
			  "<":"&lt;",
			  ">":"&gt;" ]) );
}

//! Quotes strings just like @[attribute_quote], but entities in the
//! form @tt{&foo.bar;@} will not be quoted.
string roxen_attribute_quote(string data)
{
  return replace(roxen_text_quote(data),
		 ([ "\"":"&quot;",
		    "'":"&apos;" ]));
}

void throw_error(mixed ...args)
{
  //  Put message in debug log and throw exception
  args[0] = "Parser.XML.Tree: " + args[0];
  if (sizeof(args) == 1)
	error(args[0]);
  else
	error(sprintf(@args));
}

//! Namespace aware parser.
class XMLNSParser {
  ADT.Stack namespace_stack = ADT.Stack();

  void create()
  {
    namespace_stack->push(([]));	// Sentinel.
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
	namespaces[0] = val;
      } else if (has_prefix(attr, "xmlns:")) {
	if (val == "") error("Bad namespace specification (%O=\"\")\n", attr);
	namespaces[attr[6..]] = val;
      }
    }
    namespace_stack->push(namespaces);
    // Now that we know what the namespaces are, we
    // can expand the namespaces in the other attributes.
    mapping(string:string) result = ([]);
    foreach(attrs; string attr; string val) {
      int i;
      if (!has_prefix(attr, "xmlns:") &&
	  (i = search(attr, ":")) >= 0) {
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

//!
class AbstractNode {
  //  Private member variables
  /* static */ AbstractNode           mParent = 0;
  /* static */ array(AbstractNode)    mChildren = ({ });
  
  //  Public methods

  //! Sets the parent node to @[parent].
  void set_parent(AbstractNode parent)    { mParent = parent; }

  //! Returns the parent node.
  AbstractNode get_parent()          { return (mParent); }

  //! Returns all the nodes children.
  array(AbstractNode) get_children() { return (mChildren); }

  //! Returns the number of children of the node.
  int count_children()           { return (sizeof(mChildren)); }
 
  //! Returns the corresponding node in a clone of the tree.
  AbstractNode clone(void|int(-1..1) direction) {
    AbstractNode n = AbstractNode();
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

  //! Returns the last childe node or zero.
  AbstractNode get_last_child()
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
  AbstractNode `[](mixed pos)
  {
    if (intp(pos)) {
      //  Treat pos as index into array
      if ((pos < 0) || (pos > sizeof(mChildren) - 1))
	return 0;
      return (mChildren[pos]);
    } else
      //  Unknown indexing
      return 0;
  }

  //! Adds a child node to this node. The child node is
  //! added last in the child list and its parent reference
  //! is updated.
  //! @returns
  //! The updated child node is returned.
  AbstractNode add_child(AbstractNode c)
  {
    mChildren += ({ c });
    c->mParent = this;
	
    //  Let caller get the new node back for easy chaining of calls
    return (c);
  }

  //! Removes all occurrences of the provided node from the called nodes
  //! list of children. The removed nodes parent reference is set to null.
  void remove_child(AbstractNode c)
  {
    mChildren -= ({ c });
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
    mChildren = children;
    foreach(mChildren, AbstractNode c)
      c->mParent = this;
  }


  //! Replaces the first occurrence of the old node child with
  //! the new node child. All parent references are updated.
  //! @returns
  //!   Returns the new child node.
  AbstractNode replace_child(AbstractNode old, AbstractNode new)
  {
    int index = search(mChildren, old);
    if (index < 0)
      return 0;
    mChildren[index] = new;
    new->mParent = this;
    old->mParent = 0;
    return new;
  }

  //! Replaces this node with the provided one.
  //! @returns
  //!   Returns the new node.
  AbstractNode replace_node(AbstractNode new) {
    mParent->replace_child(this, new);
    return new;
  }

  //! Traverse the node subtree in preorder, root node first, then
  //! subtrees from left to right, calling the callback function
  //! for every node. If the callback function returns @[STOP_WALK]
  //! the traverse is promptly aborted and @[STOP_WALK] is returned.
  int walk_preorder(function(AbstractNode, mixed ...:int|void) callback,
		    mixed ... args)
  {
    if (callback(this, @args) == STOP_WALK)
      return STOP_WALK;
    foreach(mChildren, AbstractNode c)
      if (c->walk_preorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
  }
  
  //! Traverse the node subtree in preorder, root node first, then
  //! subtrees from left to right. For each node we call callback_1
  //! before iterating through children, and then callback_2
  //! (which always gets called even if the walk is aborted earlier).
  //! If the callback function returns @[STOP_WALK] the traverse
  //! decend is aborted and @[STOP_WALK] is returned once all waiting
  //! callback_2 functions has been called.
  int walk_preorder_2(function(AbstractNode, mixed ...:int|void) callback_1,
		      function(AbstractNode, mixed ...:int|void) callback_2,
		      mixed ... args)
  {
    int  res;
	
    res = callback_1(this, @args);
    if (!res)
      foreach(mChildren, AbstractNode c)
	res = res || c->walk_preorder_2(callback_1, callback_2, @args);
    return (callback_2(this, @args) || res);
  }

  //! Traverse the node subtree in inorder, left subtree first, then
  //! root node, and finally the remaining subtrees, calling the callback
  //! function for every node. If the callback function returns
  //! @[STOP_WALK] the traverse is promptly aborted and @[STOP_WALK]
  //! is returned.
  int walk_inorder(function(AbstractNode, mixed ...:int|void) callback,
		   mixed ... args)
  {
    if (sizeof(mChildren) > 0)
      if (mChildren[0]->walk_inorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
    if (callback(this, @args) == STOP_WALK)
      return STOP_WALK;
    foreach(mChildren[1..], AbstractNode c)
      if (c->walk_inorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
  }

  //! Traverse the node subtree in postorder, first subtrees from left to
  //! right, then the root node, calling the callback function for every
  //! node. If the callback function returns @[STOP_WALK] the traverse
  //! is promptly aborted and @[STOP_WALK] is returned.
  int walk_postorder(function(AbstractNode, mixed ...:int|void) callback,
		     mixed ... args)
  {
    foreach(mChildren, AbstractNode c)
      if (c->walk_postorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
    if (callback(this, @args) == STOP_WALK)
      return STOP_WALK;
  }

  //! Iterates over the nodes children from left to right, calling the callback
  //! function for every node. If the callback function returns @[STOP_WALK]
  //! the iteration is promptly aborted and @[STOP_WALK] is returned.
  int iterate_children(function(AbstractNode, mixed ...:int|void) callback,
		       mixed ... args)
  {
    foreach(mChildren, AbstractNode c)
      if (callback(c, @args) == STOP_WALK)
	return STOP_WALK;
  }

  //! Returns all preceding siblings, i.e. all siblings present before this node
  //! in the parents children list.
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

  //! Returns all following siblings, i.e. all siblings present after this node
  //! in the parents children list.
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

  //! Returns a list of all descendants in document order. Includes
  //! this node if @[include_self] is set.
  array(AbstractNode) get_descendants(int(0..1) include_self)
  {
    array   res;
	
    //  Walk subtrees in document order and add to resulting list
    res = include_self ? ({ this }) : ({ });
    foreach(mChildren, AbstractNode child) {
      res += child->get_descendants(1);
    }
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
};


//!  Node in XML tree
class Node {
  inherit AbstractNode;

  //  Member variables for this node type
  static int            mNodeType;
  static string		mShortNamespace = "";	// Namespace prefix
  static string		mNamespace;	// Resolved namespace
  static string         mTagName;
//   private int            mTagCode;
  static mapping        mAttributes;		// Resolved attributes
  static mapping	mShortAttributes;	// Shortened attributes
  static array(Node) mAttrNodes;   //  created on demand
  static string         mText;
  static int            mDocOrder;

  static Node low_clone()
  {
    return Node(get_node_type(), get_full_name(),
		get_attributes(), get_text());
  }

  //! Clones the node, optionally connected to parts of the tree.
  //! If direction is -1 the cloned nodes parent will be set, if
  //! direction is 1 the clone nodes childen will be set.
  Node clone(void|int(-1..1) direction)
  {
    Node n = low_clone();

    if(direction!=1) {
      Node p = get_parent();
      if(p)
	n->set_parent( p->clone(-1) );
    }

    if(direction!=-1)
      foreach(get_children(), Node child)
	n->add_child( child->clone(1) );

    return n;
  }

  //  This can be accessed directly by various methods to cache parsing
  //  info for faster processing. Some use it for storing flags and others
  //  use it to cache reference objects.
  public mixed           mNodeData = 0;
  
  //  Public methods
  //! Returns this nodes attributes, which can be altered
  //! destructivly to alter the nodes attributes.
  mapping get_attributes()   { return (mAttributes); }

  //! Returns this nodes name-space adjusted attributes.
  //!
  //! @note
  //!   @[set_short_namespaces()] must have been called before
  //!   calling this function.
  mapping get_short_attributes()   { return (mShortAttributes); }

  //! Returns the node type. See defined node type constants.
  int get_node_type()        { return (mNodeType); }

  //! Returns text content in node.
  string get_text()          { return (mText); }

  //!
  int get_doc_order()        { return (mDocOrder); }

  //!
  void set_doc_order(int o)  { mDocOrder = o; }
  
//   int get_tag_code()
//   {
//     //  Fake ATTR nodes query their parent
//     return ((mNodeType == XML_ATTR) ? get_parent()->get_tag_code() : mTagCode);
//   }
  

  //! Returns the name of the element node, or the nearest element above if
  //! an attribute node.
  string get_tag_name()
  {
    //  Fake ATTR nodes query their parent
    return ((mNodeType == XML_ATTR) ? get_parent()->get_tag_name() : mTagName);
  }

  //! Return name of tag or name of attribute node.
  string get_any_name()
  {
    return (mTagName);
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

  //! Returns the name of the attribute node.
  string get_attr_name()
  {
    //  Only works for fake ATTR nodes
    return ((mNodeType == XML_ATTR) ? mTagName : "");
  }
  
  //!
  static void create(int type, string name, mapping attr, string text)
  {
    if (name) {
      sscanf(reverse(name), "%[^/:]", mTagName);
      mTagName=reverse(mTagName);
      mNamespace=name[..sizeof(name)-(1+sizeof(mTagName))];
    }
    mNodeType = type;
//     mTagCode = kTagMapping[name] || kUnsupportedTagMapping[name];
    mAttributes = attr;
    mText = text;
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
  AbstractNode get_first_element(string|void name, int(0..1)|void full) {
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
  string html_of_node(void|int(0..1) preserve_roxen_entities)
  {
    return render_xml(preserve_roxen_entities);
  }

  //! It is possible to cast a node to a string, which will return
  //! @[render_xml()] for that node.
  mixed cast(string to) {
    if(to=="object") return this;
    if(to=="string") return render_xml();
    error( "Can not case Node to "+to+".\n" );
  }

  static void low_render_xml(String.Buffer data, Node n,
			     function(string:string) textq,
			     function(string:string) attrq,
			     void|mapping(string:string) namespace_lookup)
  {
    string tagname;
    switch(n->get_node_type()) {
    case XML_TEXT:
      data->add(textq(n->get_text()));
      break;

    case XML_ELEMENT:
      if (!sizeof(tagname = n->get_short_name(namespace_lookup)))
	break;
      
      data->add("<", tagname);
      if (mapping attr = n->get_short_attributes()) {
	foreach(indices(attr), string a) {
	  data->add(" ", a, "='", attrq(attr[a]), "'");
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
	foreach(indices(attr), string a)
	  data->add(" ", a, "='", attrq(attr[a]), "'");
      }
      data->add("?>\n");
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
    }

    array(Node) children = n->get_children();
    foreach(children, Node n) {
      low_render_xml(data, n, textq, attrq);
    }

    if (n->get_node_type() == XML_ELEMENT) {
      if (n->count_children())
	if (sizeof(tagname))
	  data->add("</", tagname, ">");
    }
  }

  // Get the encoding from the XML-header (if any).
  //
  // Create a new XML-header if there's none.
  //
  // Add an encoding attribute if there is none.
  static string get_encoding()
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
      forward_lookup = ([]);
      backward_lookup = ([]);
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
    // Then set the short namespace for this tag.
    mShortNamespace = "";
    if (sizeof(mNamespace)) {
      if (!(mShortNamespace = forward_lookup[mNamespace])) {
	// We need to allocate a short namespace symbol.
	// FIXME: This is O(n²).
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
    // Then set the short namespaces for any attributes.
    foreach(indices(attrs), string attr_name) {
      if (!has_prefix(attr_name, "xmlns:")) {
	int i = search(attr_name, ":");
	int j = search(attr_name, "/");
	if (j > i) {
	  i = j;
	}
	if (i >= 0) {
	  string ns = attr_name[..i-1];
	  string prefix;
	  if (!(prefix = forward_lookup[ns])) {
	    // We need to allocate a short namespace symbol.
	    // FIXME: This is O(n²).
	    int i;
	    while(backward_lookup[prefix = ("NS"+i+":")]) {
	      i++;
	    }
	    backward_lookup[mShortNamespace] = ns;
	    forward_lookup[mNamespace] = prefix;
	    attrs["xmlns:NS"+i] = ns;
	    short_attrs["xmlns:NS"+i] = ns;
	  }
	  m_delete(short_attrs, attr_name);
	  short_attrs[prefix + ns] = attrs[attr_name];
	}
      }
    }
    mShortAttributes = short_attrs;
    // And then do it for all the children.
    get_children()->set_short_namespaces(forward_lookup, backward_lookup);
  }

  //! Creates an XML representation of the node sub tree. If the
  //! flag @[preserve_roxen_entities] is set, entities on the form
  //! @tt{&foo.bar;@} will not be escaped.
  //!
  //! @param namespace_lookup
  //!   Mapping from namespace prefix to namespace symbol prefix.
  string render_xml(void|int(0..1) preserve_roxen_entities)
  {
    String.Buffer data = String.Buffer();
    string encoding = get_encoding();
    set_short_namespaces();
    if(preserve_roxen_entities)
      low_render_xml(data, this, roxen_text_quote,
		     roxen_attribute_quote);
    else
      low_render_xml(data, this, text_quote, attribute_quote);
    return Locale.Charset.encoder(encoding)->feed((string)data)->drain();
  }

  //! Creates an XML representation fo the node sub tree and streams
  //! the output to the file @[f]. If the flag @[preserve_roxen_entities]
  //! is set, entities on the form @tt{&foo.bar;@} will not be escaped.
  void render_to_file(Stdio.File f,
		      void|int(0..1) preserve_roxen_entities) {
    object data = class (Stdio.File f, object encoder) {
      void add(string ... args) {
	encoder->feed(args[*]);
	f->write(encoder->drain());
      }
    } (f, Locale.Charset.encoder(get_encoding()));
    set_short_namespaces();
    if(preserve_roxen_entities)
      low_render_xml(data, this, roxen_text_quote,
		     roxen_attribute_quote);
    else
      low_render_xml(data, this, text_quote, attribute_quote);
  }

  //  Override AbstractNode::`[]
  Node `[](mixed pos)
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

  string _sprintf(int t) {
    return t=='O' && sprintf("%O(#%d:%d,%s)", this_program, mDocOrder,
			     get_node_type(), get_full_name()||"NULL");
  }
};

class RootNode
{
  inherit Node;
  static Node low_clone()
  {
    return RootNode();
  }
  static void create()
  {
    ::create(XML_ROOT, "", 0, "");
  }
}

class TextNode
{
  inherit Node;
  static Node low_clone()
  {
    return TextNode(get_text());
  }
  static void create(string text)
  {
    ::create(XML_TEXT, "", 0, text);
  }
}

class CommentNode
{
  inherit Node;
  static Node low_clone()
  {
    return CommentNode(get_text());
  }
  static void create(string text)
  {
    ::create(XML_COMMENT, "", 0, text);
  }
}

class HeaderNode
{
  inherit Node;
  static Node low_clone()
  {
    return HeaderNode(get_attributes());
  }
  static void create(mapping(string:string) attrs)
  {
    ::create(XML_HEADER, "", attrs, "");
  }
}

class PINode
{
  inherit Node;
  static Node low_clone()
  {
    return PINode(get_full_name(), get_attributes(), get_text());
  }
  static void create(string name, mapping(string:string) attrs,
		     string contents)
  {
    ::create(XML_PI, name, attrs, contents);
  }
}

class ElementNode
{
  inherit Node;
  static Node low_clone()
  {
    return ElementNode(get_full_name(), get_attributes());
  }
  static void create(string name, mapping(string:string) attrs)
  {
    ::create(XML_ELEMENT, name, attrs, "");
  }
}

class AttributeNode
{
  inherit Node;
  static Node low_clone()
  {
    return AttributeNode(get_full_name(), get_text());
  }
  static void create(string name, string value)
  {
    ::create(XML_ATTR, name, 0, value);
  }
}

private Node|int(0..0)
  parse_xml_callback(string type, string name,
		     mapping attr, string|array contents,
		     mixed location, mixed ...extra)
{
  Node node;

  switch (type) {
  case "":
  case "<![CDATA[":
    //  Create text node
    return TextNode(contents);

  case "<!--":
    //  Create comment node
    return CommentNode(contents);

  case "<?xml":
    //  XML header tag
    return HeaderNode(attr);

  case "<?":
    //  XML processing instruction
    return PINode(name, attr, contents);

  case "<>":
    //  Create new tag node.
    if (arrayp(extra) && sizeof(extra) &&
	mappingp(extra[0])) {
      //  Convert tag and attribute names to lowercase
      //  if requested.
      if (extra[0]->force_lc) {
	name = lower_case(name);
	attr = mkmapping(map(indices(attr), lower_case), values(attr));
      }
      //  Parse namespace information of available.
      if (extra[0]->xmlns) {
	XMLNSParser xmlns = extra[0]->xmlns;
	attr = xmlns->Enter(attr);
	name = xmlns->Decode(name);
	xmlns->Leave();
      }
    }
    return ElementNode(name, attr);

  case ">":
    //  Create tree node for this container
    if (arrayp(extra) && sizeof(extra) && mappingp(extra[0])) {
      //  Convert tag and attribute names to lowercase
      //  if requested.
      if (extra[0]->force_lc) {
	name = lower_case(name);
	attr = mkmapping(map(indices(attr), lower_case), values(attr));
      }
      //  Parse namespace information of available.
      if (extra[0]->xmlns) {
	XMLNSParser xmlns = extra[0]->xmlns;
	name = xmlns->Decode(name);
	xmlns->Leave();
      }
    }
    node = ElementNode(name, attr);
	
    //  Add children to our tree node. We need to merge consecutive text
    //  children since two text elements can't be neighbors according to
    //  the W3 spec.
    string buffer_text = "";
    foreach(contents, Node child) {
      if (child->get_node_type() == XML_TEXT) {
	//  Add this text string to buffer
	buffer_text += child->get_text();
      } else {
	//  Process buffered text before this child is added
	if (sizeof(buffer_text)) {
	  node->add_child(TextNode(buffer_text));
	  buffer_text = "";
	}
	node->add_child(child);
      }
    }
    if (sizeof(buffer_text))
      node->add_child(TextNode(buffer_text));
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
    }
  case "<!DOCTYPE":
  default:
    return 0;
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

//! Takes a XML string and produces a node tree.
Node parse_input(string data, void|int(0..1) no_fallback,
		 void|int(0..1) force_lowercase,
		 void|mapping(string:string) predefined_entities,
		 void|int(0..1) parse_namespaces)
{
  object xp = spider.XML();
  Node mRoot;
  
  xp->allow_rxml_entities(1);
  
  //  Init parser with predefined entities
  if (predefined_entities)
    foreach(indices(predefined_entities), string entity)
      xp->define_entity_raw(entity, predefined_entities[entity]);
  
  // Construct tree from string
  mixed err = catch
  {
    mapping(string:mixed) extras = ([]);
    if (force_lowercase) {
      extras->force_lc = 1;
    }
    if (parse_namespaces) {
      extras->xmlns = XMLNSParser();
    }
    mRoot = RootNode();
    catch( data=xp->autoconvert(data) );
    foreach(xp->parse(data, parse_xml_callback,
		      sizeof(extras) && extras),
	    Node child)
      mRoot->add_child(child);
  };

  if(err)
  {
    //  If string msg is found we propagate the error. If error message
    //  contains " [Offset: 4711]" we add the input data to the string.
    if (stringp(err) && no_fallback)
    {
      if (sscanf(err, "%s [Offset: %d]", err, int ofs) == 2)
	err += report_error_context(data, ofs);
    }
    throw(err);
  }
  else
    return mRoot;
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
