#pike __REAL_VERSION__

/*
 * $Id: Tree.pmod,v 1.11 2004/03/15 16:43:08 jonasw Exp $
 *
 */

constant STOP_WALK = -1;
constant XML_ROOT     = 0x0001;
constant XML_ELEMENT  = 0x0002;
constant XML_TEXT     = 0x0004;
constant XML_HEADER   = 0x0008;
constant XML_PI       = 0x0010;
constant XML_COMMENT  = 0x0020;
constant XML_DOCTYPE  = 0x0040;
constant XML_ATTR     = 0x0080;    //  Attribute nodes are created on demand
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

string text_quote(string data, void|int preserve_roxen_entities)
{
  if (preserve_roxen_entities) {
    string out = "";
    int pos = 0;
    while ((pos = search(data, "&")) >= 0) {
      if ((sscanf(data[pos..], "&%[^\n\r\t <>;&];%*s", string entity) == 2) &&
	  search(entity, ".") >= 0) {
	out += text_quote(data[..pos - 1], 0) + "&" + entity + ";";
	data = data[pos + strlen(entity) + 2..];
      } else {
	out += text_quote(data[..pos], 0);
	data = data[pos + 1..];
      }
    }
    return out + text_quote(data, 0);
  } else {
    data = replace(data, "&", "&amp;");
    data = replace(data, "<", "&lt;");
    data = replace(data, ">", "&gt;");
    return data;
  }
}

string attribute_quote(string data, void|int preserve_roxen_entities)
{
  data = text_quote(data, preserve_roxen_entities);
  data = replace(data, "\"", "&quot;");
  data = replace(data, "'",  "&apos;");
  return data;
}

void throw_error(mixed ...args)
{
  //  Put message in debug log and throw exception
  array rep_args = args;
  rep_args[0] = "Parser.XML.Tree: " + rep_args[0];
  if (sizeof(args) == 1)
	throw(args[0]);
  else
	throw(sprintf(@args));
}

class AbstractNode {
  //  Private member variables
  /*private*/ AbstractNode           mParent = 0;
  /*private*/ array(AbstractNode)    mChildren = ({ });
  
  //  Public methods
  void set_parent(AbstractNode p)    { mParent = p; }
  AbstractNode get_parent()          { return (mParent); }
  array(AbstractNode) get_children() { return (mChildren); }
  int count_children()           { return (sizeof(mChildren)); }
 
  
  AbstractNode get_root()
  {
    AbstractNode  parent, node;
    
    parent = this_object();
    while (node = parent->mParent)
      parent = node;
    return (parent);
  }

  AbstractNode get_last_child()
  {
    if (!sizeof(mChildren))
      return 0;
    else
      return (mChildren[-1]);
  }
  
  AbstractNode `[](mixed pos)
  {
    //  The [] operator indexes among the node children
    if (intp(pos)) {
      //  Treat pos as index into array
      if ((pos < 0) || (pos > sizeof(mChildren) - 1))
	return 0;
      return (mChildren[pos]);
    } else
      //  Unknown indexing
      return 0;
  }

  AbstractNode add_child(AbstractNode c)
  {
    //  Add to array and update parent reference for this new node
    mChildren += ({ c });
    c->mParent = this_object();
	
    //  Let caller get the new node back for easy chaining of calls
    return (c);
  }

  void remove_child(AbstractNode c)
  {
    //  Remove all entries from array and update parent reference for
    //  altered child node.
    mChildren -= ({ c });
    c->mParent = 0;
  }

  int|void walk_preorder(function(AbstractNode, mixed ...:int|void) callback,
			 mixed ... args)
  {
    //  Root node first, then subtrees left->right
    if (callback(this_object(), @args) == STOP_WALK)
      return STOP_WALK;
    foreach(mChildren, AbstractNode c)
      if (c->walk_preorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
  }
  
  int|void walk_preorder_2(function(AbstractNode, mixed ...:int|void) callback_1,
			   function(AbstractNode, mixed ...:int|void) callback_2,
			   mixed ... args)
  {
    int  res;
	
    //  Root node first, then subtrees left->right. For each node we call
    //  callback_1 before iterating through children, and then callback_2
    //  (which always gets called even if the walk is aborted earlier).
    res = callback_1(this_object(), @args);
    if (!res)
      foreach(mChildren, AbstractNode c)
	res = res || c->walk_preorder_2(callback_1, callback_2, @args);
    return (callback_2(this_object(), @args) || res);
  }

  int|void walk_inorder(function(AbstractNode, mixed ...:int|void) callback,
			mixed ... args)
  {
    //  Left subtree first, then root node, and finally remaining subtrees
    if (sizeof(mChildren) > 0)
      if (mChildren[0]->walk_inorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
    if (callback(this_object(), @args) == STOP_WALK)
      return STOP_WALK;
    foreach(mChildren[1..], AbstractNode c)
      if (c->walk_inorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
  }

  int|void walk_postorder(function(AbstractNode, mixed ...:int|void) callback,
			  mixed ... args)
  {
    //  First subtrees left->right, then root node
    foreach(mChildren, AbstractNode c)
      if (c->walk_postorder(callback, @args) == STOP_WALK)
	return STOP_WALK;
    if (callback(this_object(), @args) == STOP_WALK)
      return STOP_WALK;
  }

  int|void iterate_children(function(AbstractNode, mixed ...:int|void) callback,
			    mixed ... args)
  {
    foreach(mChildren, AbstractNode c)
      if (callback(c, @args) == STOP_WALK)
	return STOP_WALK;
  }

  array(AbstractNode) get_preceding_siblings()
  {
    array  siblings;
    int    pos;

    //  Get parent list of children and locate ourselves
    if (!mParent)
      return ({ });
    siblings = mParent->get_children();
    pos = search(siblings, this_object());

    //  Return array in reverse order not including self
    return (reverse(siblings[..(pos - 1)]));
  }

  array(AbstractNode) get_following_siblings()
  {
    array  siblings;
    int    pos;

    //  Get parent list of children and locate ourselves
    if (!mParent)
      return ({ });
    siblings = mParent->get_children();
    pos = search(siblings, this_object());

    //  Select array range
    return (siblings[(pos + 1)..]);
  }

  array(AbstractNode) get_siblings()
  {
    //  If not found we return ourself only
    if (!mParent)
      return ({ this_object() });
    return (mParent->get_children());
  }

  array(AbstractNode) get_ancestors(int include_self)
  {
    array     res;
    AbstractNode  node;
	
    //  Repeat until we reach the top
    res = include_self ? ({ this_object() }) : ({ });
    node = this_object();
    while (node = node->get_parent())
      res += ({ node });
    return (res);
  }

  array(AbstractNode) get_descendants(int include_self)
  {
    array   res;
	
    //  Walk subtrees in document order and add to resulting list
    res = include_self ? ({ this_object() }) : ({ });
    foreach(mChildren, AbstractNode child) {
      res += child->get_descendants(1);
    }
    return (res);
  }

  array(AbstractNode) get_preceding()
  {
    AbstractNode   node, root, self;
    array      res = ({ });
	
    //  Walk tree from root until we find ourselves and add all preceding
    //  nodes. We should return the nodes in reverse document order.
    self = this_object();
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
    root = this_object();
    while (node = root->get_parent()) {
      root = node;
      res -= ({ node });
    }
    return (res);
  }

  array(AbstractNode) get_following()
  {
    array      siblings;
    AbstractNode   node;
    array      res = ({ });
	
    //  Add subtrees from right-hand siblings and keep walking towards
    //  the root of the tree.
    node = this_object();
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


//  Node in XML tree
class Node {
  inherit AbstractNode;

  //  Member variables for this node type
  private int            mNodeType;
  private string         mTagName;
//   private int            mTagCode;
  private mapping        mAttributes;
  private array(Node) mAttrNodes;   //  created on demand
  private string         mText;
  private int            mDocOrder;

  //  This can be accessed directly by various methods to cache parsing
  //  info for faster processing. Some use it for storing flags and others
  //  use it to cache reference objects.
  public mixed           mNodeData = 0;
  
  //  Public methods
  mapping get_attributes()   { return (mAttributes); }
  int get_node_type()        { return (mNodeType); }
  string get_text()          { return (mText); }
  int get_doc_order()        { return (mDocOrder); }
  void set_doc_order(int o)  { mDocOrder = o; }
  
//   int get_tag_code()
//   {
//     //  Fake ATTR nodes query their parent
//     return ((mNodeType == XML_ATTR) ? get_parent()->get_tag_code() : mTagCode);
//   }
  
  string get_tag_name()
  {
    //  Fake ATTR nodes query their parent
    return ((mNodeType == XML_ATTR) ? get_parent()->get_tag_name() : mTagName);
  }

  string get_any_name()
  {
    //  Return name of tag or name of fake attribute
    return (mTagName);
  }
  
  string get_attr_name()
  {
    //  Only works for fake ATTR nodes
    return ((mNodeType == XML_ATTR) ? mTagName : "");
  }
  
  void create(int type, string name, mapping attr, string text)
  {
    mNodeType = type;
    mTagName = name;
//     mTagCode = kTagMapping[name] || kUnsupportedTagMapping[name];
    mAttributes = attr;
    mText = text;
    mAttrNodes = 0;
  }

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

  string html_of_node(void|int preserve_roxen_entities)
  {
    string  data = "";
	
    //  Walk sub-tree and create HTML representation
    walk_preorder_2(
		    lambda(Node n) {
		      switch(n->get_node_type()) {
		      case XML_TEXT:
                        data += text_quote(n->get_text(),
					   preserve_roxen_entities);
			break;

		      case XML_ELEMENT:
			if (!strlen(n->get_tag_name()))
			  break;
			data += "<" + n->get_tag_name();
			if (mapping attr = n->get_attributes()) {
                          foreach(indices(attr), string a)
                            data += " " + a + "='"
                              + attribute_quote(attr[a],
						preserve_roxen_entities) + "'";
			}
			if (n->count_children())
			  data += ">";
			else
			  data += "/>";
			break;

		      case XML_HEADER:
			data += "<?xml";
			if (mapping attr = n->get_attributes()) {
                          foreach(indices(attr), string a)
                            data += " " + a + "='"
                              + attribute_quote(attr[a],
						preserve_roxen_entities) + "'";
			}
			data += "?>\n";
			break;

		      case XML_PI:
			data += "<?" + n->get_tag_name();
			string text = n->get_text();
			if (strlen(text))
			  data += " " + text;
			data += "?>";
			break;
			
		      case XML_COMMENT:
			data += "<!--" + n->get_text() + "-->";
			break;
		      }
		    },
		    lambda(Node n) {
		      if (n->get_node_type() == XML_ELEMENT)
			if (n->count_children())
			  if (strlen(n->get_tag_name()))
			    data += "</" + n->get_tag_name() + ">";
		    });
	
    return (data);
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

    //  Create array of new nodes; they will NOT be added as proper
    //  children to the parent node since that would only cause
    //  unnecessary slowdowns when walking that tree. However, we
    //  set the parent link so upwards traversal is made possible.
    //
    //  After creating these nodes we need to give them node numbers
    //  which harmonize with the existing numbers. Fortunately we
    //  inserted a gap in the series when first numbering the original
    //  nodes.
    mAttrNodes = ({ });
    node_num = get_doc_order() + 1;
    foreach(indices(mAttributes), string attr) {
      node = Node(XML_ATTR, attr, 0, mAttributes[attr]);
      node->set_parent(this_object());
      node->set_doc_order(node_num++);
      mAttrNodes += ({ node });
    }
    return (mAttrNodes);
  }

  string _sprintf() {
    return sprintf("Node(#%d: %d, %s)", mDocOrder, get_node_type(), get_any_name());
  }
};


mixed parse_xml_callback(string type, string name,
			 mapping attr, string|array contents,
			 mixed location, mixed ...extra)
{
  Node   node;

  switch (type) {
  case "":
  case "<![CDATA[":
    //  Create text node
    return (Node(XML_TEXT, "", 0, contents));

  case "<!--":
    //  Create comment node
    return (Node(XML_COMMENT, "", 0, contents));

  case "<?xml":
    //  XML header tag
    return (Node(XML_HEADER, "", attr, ""));

  case "<?":
    //  XML processing instruction
    return (Node(XML_PI, name, attr, contents));

  case "<>":
    //  Create new tag node. Convert tag and attribute names to lowercase
    //  if requested.
    if (arrayp(extra) && sizeof(extra) &&
	mappingp(extra[0]) && extra[0]->force_lc) {
      name = lower_case(name);
      attr = mkmapping(map(indices(attr), lower_case), values(attr));
    }
    return (Node(XML_ELEMENT, name, attr, ""));

  case ">":
    //  Create tree node for this container. Convert tag and attribute
    //  names to lowercase if requested.
    if (arrayp(extra) && sizeof(extra) &&
	mappingp(extra[0]) && extra[0]->force_lc) {
      name = lower_case(name);
      attr = mkmapping(map(indices(attr), lower_case), values(attr));
    }
    node = Node(XML_ELEMENT, name, attr, "");
	
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
	if (strlen(buffer_text)) {
	  node->add_child(Node(XML_TEXT, "", 0, buffer_text));
	  buffer_text = "";
	}
	node->add_child(child);
      }
    }
    if (strlen(buffer_text))
      node->add_child(Node(XML_TEXT, "", 0, buffer_text));
    return (node);

  case "error":
    //  Error message present in contents. If "location" is present in the
    //  "extra" mapping we encode that value in the message string so the
    //  handler for this throw() can display the proper error context.
    if (location && mappingp(location))
      throw_error(contents + " [Offset: " + location->location + "]\n");
    else
      throw_error(contents + "\n");

  case "<":
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

Node parse_input(string data, void|int no_fallback, void|int force_lowercase,
		 void|mapping predefined_entities)
{
  object xp = spider.XML();
  Node mRoot;
  
  xp->allow_rxml_entities(1);
  
  //  Init parser with predefined entities
  if (predefined_entities)
    foreach(indices(predefined_entities), string entity)
      xp->define_entity_raw(entity, predefined_entities[entity]);
  
  //  Construct tree from string
  mixed err = catch
  {
    mRoot = Node(XML_ROOT, "", ([ ]), "");
    foreach(xp->parse(data, parse_xml_callback,
		      force_lowercase && ([ "force_lc" : 1 ]) ),
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
      throw(err);
    }
  }
  else
    return mRoot;
}
  
Node parse_file(string path)
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
    return parse_input(data);
}
