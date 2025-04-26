#pike __REAL_VERSION__

//!
class DOMException(int code) {

  // ExceptionCode
  constant INDEX_SIZE_ERR               = 1;
  constant DOMSTRING_SIZE_ERR           = 2;
  constant HIERARCHY_REQUEST_ERR        = 3;
  constant WRONG_DOCUMENT_ERR           = 4;
  constant INVALID_CHARACTER_ERR        = 5;
  constant NO_DATA_ALLOWED_ERR          = 6;
  constant NO_MODIFICATION_ALLOWED_ERR  = 7;
  constant NOT_FOUND_ERR                = 8;
  constant NOT_SUPPORTED_ERR            = 9;
  constant INUSE_ATTRIBUTE_ERR          = 10;

  constant is_generic_error = 1;
  constant is_dom_exception = 1;

  protected array backtrace = predef::backtrace()[..<2];

  protected constant symbolic = ([
    INDEX_SIZE_ERR: "INDEX_SIZE_ERR",
    DOMSTRING_SIZE_ERR: "DOMSTRING_SIZE_ERR",
    HIERARCHY_REQUEST_ERR: "HIERARCHY_REQUEST_ERR",
    WRONG_DOCUMENT_ERR: "WRONG_DOCUMENT_ERR",
    INVALID_CHARACTER_ERR: "INVALID_CHARACTER_ERR",
    NO_DATA_ALLOWED_ERR: "NO_DATA_ALLOWED_ERR",
    NO_MODIFICATION_ALLOWED_ERR: "NO_MODIFICATION_ALLOWED_ERR",
    NOT_FOUND_ERR: "NOT_FOUND_ERR",
    NOT_SUPPORTED_ERR: "NOT_SUPPORTED_ERR",
    INUSE_ATTRIBUTE_ERR: "INUSE_ATTRIBUTE_ERR",
  ]);

  protected constant human_readable = ([
    INDEX_SIZE_ERR: "index out of range",
    DOMSTRING_SIZE_ERR: "specified range of text does not fit into DOMString",
    HIERARCHY_REQUEST_ERR: "node inserted somewhere it doesn't belong",
    WRONG_DOCUMENT_ERR: "node used in a document that doesn't support it",
    INVALID_CHARACTER_ERR: "an invalid or illegal character was specified",
    NO_DATA_ALLOWED_ERR: "data was specified for a node which does not support data",
    NO_MODIFICATION_ALLOWED_ERR: "an unallowed attempt was made to modify an object",
    NOT_FOUND_ERR: "node does not exist in this context",
    NOT_SUPPORTED_ERR: "implementation does not support the type of object requested",
    INUSE_ATTRIBUTE_ERR: "attempted to add an attribute that is already in use",
  ]);

  protected string|array `[] (int i)
  {
    switch (i) {
      case 0:
	return "DOMException: " +
	  (human_readable[code] || ("unknown exception "+code)) + "\n";
      case 1:
	return backtrace;
    }
  }

  protected string _sprintf(int mode)
  {
    return mode == 'O' && sprintf("%O(%s)", this_program,
				  (string)(symbolic[code]||code) );
  }

};

//!
class DOMImplementation
{
  int has_feature(string feature, string|void version)
  {
    return lower_case(feature)=="xml" && (version == "1.0" || !version);
  }

  Document create_document(string namespace_uri, string qualified_name,
			   DocumentType|void doctype)
  {
    return Document(this, namespace_uri, qualified_name, doctype);
  }

  DocumentType create_document_type(string qualified_name,
				    string|void public_id,
				    string|void system_id)
  {
    return DocumentType(this, qualified_name, public_id, system_id);
  }
}

//!
class NodeList
{
  protected array(Node) nodes;

  protected NodeList `+(NodeList|array(Node) nl) {
    return NodeList(values(this)+values(nl));
  }
  protected Node `[](int index) { return item(index); }
  protected int _sizeof() { return get_length(); }
  protected array(Node) cast(string to)
  {
    if(to == "array")
      return values(this);
    return UNDEFINED;
  }

  Node item(int index)
  {
    return index >= 0 && index < sizeof(nodes) && nodes[index];
  }

  int get_length()
  {
    return sizeof(nodes);
  }

  protected array(int) _indices() { return indices(nodes); }
  protected array(Node) _values() { return copy_value(nodes); }
  protected Array.Iterator _get_iterator() { return Array.Iterator(nodes); }

  protected void create(array(Node)|void elts) { nodes = elts || ({}); }
}

//!
class NamedNodeMap
{
  protected Document owner_document;
  protected mapping(string:Node) map = ([]);

  protected int is_readonly() { return 0; }
  protected void bind(Node n) { }
  protected void unbind(Node n) { }

  Node get_named_item(string name) { return map[name]; }

  object(Node)|zero set_named_item(Node arg)
  {
    if(arg->get_owner_document() != owner_document)
      throw(DOMException(DOMException.WRONG_DOCUMENT_ERR));
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    Node old = map[arg->get_node_name()];
    if(old == arg)
      return 0;
    bind(arg);
    map[arg->get_node_name()] = arg;
    if(old)
      unbind(old);
    return old;
  }

  Node remove_named_item(string name)
  {
    Node old = map[name];
    if(!old)
      throw(DOMException(DOMException.NOT_FOUND_ERR));
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    m_delete(map, name);
    unbind(old);
    return old;
  }

  protected Node `[](int|string index)
  {
    return stringp(index)? get_named_item(index) : item(index);
  }

  protected int _sizeof() { return get_length(); }
  protected mapping(string:Node) cast(string to)
  {
    if(to == "mapping")
      return copy_value(map);
    return UNDEFINED;
  }

  Node item(int index)
  {
    return values(this)[index];
  }

  int get_length()
  {
    return sizeof(map);
  }

  protected array(string) _indices() { return indices(map); }
  protected array(Node) _values() { return values(map); }
  protected Mapping.Iterator _get_iterator() { return Mapping.Iterator(map); }

  protected void create(Document owner)
  {
    owner_document = owner;
  }
}

//!
class Node
{
  // NodeType
  constant ELEMENT_NODE                 = 1;
  constant ATTRIBUTE_NODE               = 2;
  constant TEXT_NODE                    = 3;
  constant CDATA_SECTION_NODE           = 4;
  constant ENTITY_REFERENCE_NODE        = 5;
  constant ENTITY_NODE                  = 6;
  constant PROCESSING_INSTRUCTION_NODE  = 7;
  constant COMMENT_NODE                 = 8;
  constant DOCUMENT_NODE                = 9;
  constant DOCUMENT_TYPE_NODE           = 10;
  constant DOCUMENT_FRAGMENT_NODE       = 11;
  constant NOTATION_NODE                = 12;

  protected class NodeNodeList {
    inherit NodeList;

    protected int search(Node n) { return predef::search(nodes, n); }
    protected void insert_at(int pos, array(Node) ns) {
      nodes = nodes[..pos-1] + ns + nodes[pos..];
    }
    protected void delete_at(int pos) {
      nodes = nodes[..pos-1]+nodes[pos+1..];
    }
  }

  protected Node parent_node;
  protected NodeNodeList child_nodes;
  protected Document owner_document;

  protected int is_readonly() {
    return parent_node && parent_node->is_readonly();
  }

  string|zero get_node_value() { return 0; }
  void set_node_value(string value) { }

  string get_node_name();
  int get_node_type();
  Node get_parent_node() { return parent_node; }

  NodeList get_child_nodes()
  {
    return child_nodes || (child_nodes = NodeNodeList());
  }

  Node get_first_child()
  {
    return child_nodes && sizeof(child_nodes) && child_nodes[0];
  }

  Node get_last_child()
  {
    return child_nodes && sizeof(child_nodes) && child_nodes[-1];
  }

  object(Node)|zero get_previous_sibling(Node|void node) {
    if(!node)
      return parent_node && parent_node->get_previous_sibling(this);
    if(!child_nodes)
      return 0;
    int pos = child_nodes->search(node);
    return pos > 0 && child_nodes[pos-1];
  }

  object(Node)|zero get_next_sibling(Node|void node) {
    if(!node)
      return parent_node && parent_node->get_next_sibling(this);
    if(!child_nodes)
      return 0;
    int pos = child_nodes->search(node);
    return pos >= 0 && pos < sizeof(child_nodes)-1 && child_nodes[pos+1];
  }

  object(NamedNodeMap)|zero get_attributes() { return 0; }
  Document get_owner_document() { return owner_document; }

  protected int child_is_allowed(Node child) { return 0; }

  protected int is_ancestor(Node node)
  {
    Node loc = this;
    while(loc) {
      if(node == loc)
	return 1;
      loc = loc->get_parent_node();
    }
    return 0;
  }

  protected void _set_parent(Node new_parent)
  {
    if(new_parent && parent_node)
	parent_node->remove_child(this);
    parent_node = new_parent;
  }

  Node insert_before(Node new_child, object(Node)|zero ref_child)
  {
    int pos = ref_child?
      (child_nodes? child_nodes->search(ref_child) : -1) :
      child_nodes && sizeof(child_nodes);
    if(pos<0)
      throw(DOMException(DOMException.NOT_FOUND_ERR));
    if(is_ancestor(new_child))
      throw(DOMException(DOMException.HIERARCHY_REQUEST_ERR));
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    if(new_child->get_owner_document() != owner_document)
      if(get_node_type() == DOCUMENT_NODE &&
	 new_child->get_node_type() == DOCUMENT_TYPE_NODE &&
	 !new_child->get_owner_document() &&
	 functionp(new_child->_set_owner_document))
	new_child->_set_owner_document(this);
      else
	throw(DOMException(DOMException.WRONG_DOCUMENT_ERR));

    if(new_child->get_node_type() == DOCUMENT_FRAGMENT_NODE) {
      array(Node) new_children = values(new_child->get_child_nodes());
      foreach(new_children, Node nc)
	if(!child_is_allowed(nc))
	  throw(DOMException(DOMException.HIERARCHY_REQUEST_ERR));
      foreach(new_children, Node nc)
	nc->_set_parent(this);
      get_child_nodes()->insert_at(pos, new_children);
    } else {
      if(!child_is_allowed(new_child))
	throw(DOMException(DOMException.HIERARCHY_REQUEST_ERR));
      new_child->_set_parent(this);
      get_child_nodes()->insert_at(pos, ({new_child}));
    }
    return new_child;
  }

  Node replace_child(Node new_child, Node old_child)
  {
    insert_before(new_child, old_child);
    return remove_child(old_child);
  }

  Node remove_child(Node old_child)
  {
    int pos = child_nodes? child_nodes->search(old_child) : -1;
    if(pos<0)
       throw(DOMException(DOMException.NOT_FOUND_ERR));
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    get_child_nodes()->delete_at(pos);
    old_child->_set_parent(0);
    return old_child;
  }

  Node append_child(Node new_child) { return insert_before(new_child, 0); }

  int has_child_nodes() { return child_nodes && sizeof(child_nodes) > 0; }

  Node clone_node(int|void deep) {
    throw(DOMException(DOMException.NOT_SUPPORTED_ERR));
  }

  protected string _sprintf(int mode)
  {
    return mode == 'O' &&
      sprintf("%O(%s)", this_program, get_node_name());
  }
}

//!
class DocumentFragment
{
  inherit Node;

  int get_node_type() { return DOCUMENT_FRAGMENT_NODE; }

  string get_node_name() { return "#document-fragment"; }

  Node clone_node(int|void deep) {
    Node new = owner_document->create_document_fragment();
    if(deep)
      foreach((array(Node))get_child_nodes(), Node n)
	new->append_child(n->clone_node(1));
    return new;
  }

  protected int child_is_allowed(Node child)
  {
    return (<ELEMENT_NODE,PROCESSING_INSTRUCTION_NODE,
	     COMMENT_NODE,TEXT_NODE,CDATA_SECTION_NODE,
	     ENTITY_REFERENCE_NODE>)[child->get_node_type()];
  }

  protected void create(Document owner)
  {
    owner_document = owner;
  }
}

//!
class Document
{
  inherit Node;

  protected program ElementImpl = Element;
  protected program DocumentFragmentImpl = DocumentFragment;
  protected program TextImpl = Text;
  protected program CommentImpl = Comment;
  protected program CDATASectionImpl = CDATASection;
  protected program ProcessingInstructionImpl = ProcessingInstruction;
  protected program AttrImpl = Attr;
  protected program EntityReferenceImpl = EntityReference;

  protected DOMImplementation implementation;

  // NB: Stored, but not used.
  protected string namespace_uri, qualified_name;

  int get_node_type() { return DOCUMENT_NODE; }
  string get_node_name() { return "#document"; }
  object(Document)|zero get_owner_document() { return 0; }
  DOMImplementation get_implementation() { return implementation; }

  object(DocumentType)|zero get_doctype()
  {
    foreach(values(get_child_nodes()), Node cn)
      if(cn->get_node_type() == DOCUMENT_TYPE_NODE)
	return cn;
    return 0;
  }

  object(Element)|zero get_document_element()
  {
    foreach(values(get_child_nodes()), Node cn)
      if(cn->get_node_type() == ELEMENT_NODE)
	return cn;
    return 0;
  }

  Element create_element(string tag_name)
  {
    return ElementImpl(this, tag_name);
  }

  DocumentFragment create_document_fragment()
  {
    return DocumentFragmentImpl(this);
  }

  Text create_text_node(string data)
  {
    return TextImpl(this, data);
  }

  Comment create_comment(string data)
  {
    return CommentImpl(this, data);
  }

  CDATASection create_cdata_section(string data)
  {
    return CDATASectionImpl(this, data);
  }

  ProcessingInstruction create_processing_instruction(string target, string data)
  {
    return ProcessingInstructionImpl(this, target, data);
  }

  Attr create_attribute(string name, string|void default_value)
  {
    return AttrImpl(this, name, default_value);
  }

  EntityReference create_entity_reference(string name)
  {
    return EntityReferenceImpl(this, name);
  }

  NodeList get_elements_by_tag_name(string tagname)
  {
    return get_document_element()->get_elements_by_tag_name(tagname);
  }

  protected int child_is_allowed(Node child)
  {
    if(child->get_node_type() == ELEMENT_NODE)
      return !get_document_element();
    else
      return (<PROCESSING_INSTRUCTION_NODE,COMMENT_NODE,
	       DOCUMENT_TYPE_NODE>)[child->get_node_type()];
  }

  protected void create(DOMImplementation i, string ns, string qn,
		     DocumentType|void doctype)
  {
    implementation = i;
    namespace_uri = ns;
    qualified_name = qn;
    owner_document = this;
    if(doctype)
      append_child(doctype);
  }
}

//!
class CharacterData
{
  inherit Node;

  protected string node_value;

  string get_node_value() { return node_value; }
  void set_node_value(string data)
  {
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    node_value=data;
  }
  string get_data() { return get_node_value(); }
  void set_data(string data) { set_node_value(data); }
  int get_length() { return sizeof(get_data()); }

  string substring_data(int offset, int count)
  {
    if(offset < 0 || offset > get_length() || count < 0)
      throw(DOMException(DOMException.INDEX_SIZE_ERR));
    return get_data()[offset..offset+count-1];
  }

  void append_data(string arg)
  {
    set_data(get_data()+arg);
  }

  void insert_data(int offset, string arg)
  {
    set_data(substring_data(0, offset) + arg +
	     substring_data(offset, get_length()));
  }

  void delete_data(int offset, int count)
  {
    if(offset > get_length() || count < 0)
      throw(DOMException(DOMException.INDEX_SIZE_ERR));
    if(offset + count >= get_length())
      set_data(substring_data(0, offset));
    else
      set_data(substring_data(0, offset) +
	       substring_data(offset+count, get_length()));
  }

  void replace_data(int offset, int count, string arg)
  {
    if(offset > get_length() || count < 0)
      throw(DOMException(DOMException.INDEX_SIZE_ERR));
    if(offset + count >= get_length())
      set_data(substring_data(0, offset) + arg);
    else
      set_data(substring_data(0, offset) + arg +
	       substring_data(offset+count, get_length()));
  }

  protected string cast(string to)
  {
    if(to == "string")
      return get_data();
    return UNDEFINED;
  }
}

//!
class Attr
{
  inherit Node;

  protected string name;
  protected int specified = 1;
  protected Element bound_to;

  int get_node_type() { return ATTRIBUTE_NODE; }
  string get_node_name() { return get_name(); }
  string get_name() { return name; }
  string get_value() { return get_node_value(); }
  void set_value(string value) { set_node_value(value); }
  int get_specified() { return specified; }

  void set_node_value(string value)
  {
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    specified = 1;
    foreach((array(Node))get_child_nodes(), Node n)
      remove_child(n);
    append_child(owner_document->create_text_node(value));
  }

  string get_node_value()
  {
    return ((array(string))get_child_nodes())*"";
  }

  Node clone_node(int|void deep) {
    Node new = owner_document->create_attribute(get_name());
    if(deep)
      foreach((array(Node))get_child_nodes(), Node n)
	new->append_child(n->clone_node(1));
    return new;
  }

  protected void _set_parent(Node new_parent) { }

  protected void _bind(Element new_element)
  {
    if(new_element && bound_to)
      throw(DOMException(DOMException.INUSE_ATTRIBUTE_ERR));
    bound_to = new_element;
  }

  protected int child_is_allowed(Node child)
  {
    return child->get_node_type() == TEXT_NODE ||
      child->get_node_type() == ENTITY_REFERENCE_NODE;
  }

  protected void create(Document owner, string _name, string|void default_value)
  {
    owner_document = owner;
    name = _name;
    if(default_value) {
      set_node_value(default_value);
      specified = 0;
    }
  }

}

//!
class Element
{
  inherit Node : node;

  protected string tag_name;
  protected NamedNodeMap attributes;

  protected NamedNodeMap create_attributes()
  {
    return class {

      inherit NamedNodeMap;

      protected int is_readonly() { return node::is_readonly(); }
      protected void bind(Node n)
      {
	n->_bind(function_object(this_program));
      }
      protected void unbind(Node n) {
	n->_bind(0);
      }

    }(owner_document);
  }

  int get_node_type() { return ELEMENT_NODE; }
  string get_node_name() { return get_tag_name(); }
  string get_tag_name() { return tag_name; }

  NamedNodeMap get_attributes() { return attributes; }

  string get_attribute(string name)
  {
    Attr a = get_attribute_node(name);
    return (a && a->get_value()) || "";
  }

  void set_attribute(string name, string value)
  {
    Attr a = get_attribute_node(name);

    if(a)
      a->set_value(value);
    else {
      a = owner_document->create_attribute(name);
      a->set_value(value);
      set_attribute_node(a);
    }
  }

  void remove_attribute(string name)
  {
    get_attributes()->remove_named_item(name);
    /* reinstate default ... */
  }

  Attr get_attribute_node(string name)
  {
    return get_attributes()->get_named_item(name);
  }

  Attr set_attribute_node(Attr new_attr)
  {
    return get_attributes()->set_named_item(new_attr);
  }

  Attr remove_attribute_node(Attr old_attr)
  {
    if(get_attribute_node(old_attr->get_name())!=old_attr)
      throw(DOMException(DOMException.NOT_FOUND_ERR));
    remove_attribute(old_attr->get_name());
    return old_attr;
  }

  NodeList get_elements_by_tag_name(string name)
  {
    NodeList res = NodeList((name == "*" || name == get_tag_name()) &&
			    ({ this }));
    foreach((array(Node))get_child_nodes(), Node n)
      if(n->get_node_type() == ELEMENT_NODE)
	res += values(n->get_elements_by_tag_name(name));
    return res;
  }

  protected void low_normalize(Node n)
  {
    while(n) {
      Node s = n->get_next_sibling();
      if(n->get_node_type() == TEXT_NODE &&
	 s && s->get_node_type() == TEXT_NODE) {
	s->set_data(n->get_data()+s->get_data());
	n->get_parent_node()->remove_child(n);
      }	else if(n->get_node_type() == ELEMENT_NODE)
	n->normalize();
      n = s;
    }
  }

  void normalize()
  {
    foreach(values(get_attributes()), Attr a)
      low_normalize(a->get_first_child());
    low_normalize(get_first_child());
  }

  Node clone_node(int|void deep) {
    Node new = owner_document->create_element(get_tag_name());
    if(deep)
      foreach((array(Node))get_child_nodes(), Node n)
	new->append_child(n->clone_node(1));
    return new;
  }

  protected int child_is_allowed(Node child)
  {
    return (<ELEMENT_NODE,PROCESSING_INSTRUCTION_NODE,
	     COMMENT_NODE,TEXT_NODE,CDATA_SECTION_NODE,
	     ENTITY_REFERENCE_NODE>)[child->get_node_type()];
  }

  protected void create(Document owner, string name)
  {
    owner_document = owner;
    tag_name = name;
    attributes = create_attributes();
  }
}

//!
class Text
{
  inherit CharacterData;

  int get_node_type() { return TEXT_NODE; }
  string get_node_name() { return "#text"; }

  Text split_text(int offset)
  {
    Text new = clone_node();
    new->set_data(substring_data(offset, get_length()));
    delete_data(offset, get_length());
    return (parent_node? parent_node->insert_before(new, get_next_sibling()) :
	    new);
  }

  Node clone_node(int|void deep) {
    return owner_document->create_text_node(get_data());
  }

  protected void create(Document owner, string data)
  {
    owner_document = owner;
    set_data(data);
  }
}

//!
class Comment
{
  inherit CharacterData;

  int get_node_type() { return COMMENT_NODE; }
  string get_node_name() { return "#comment"; }

  Node clone_node(int|void deep) {
    return owner_document->create_comment(get_data());
  }

  protected void create(Document owner, string data)
  {
    owner_document = owner;
    set_data(data);
  }
}

//!
class CDATASection
{
  inherit Text;

  int get_node_type() { return CDATA_SECTION_NODE; }
  string get_node_name() { return "#cdata-section"; }

  Node clone_node(int|void deep) {
    return owner_document->create_cdata_section(get_data());
  }

  protected void create(Document owner, string data)
  {
    owner_document = owner;
    set_data(data);
  }
}

//!
class DocumentType
{
  inherit Node;

  protected string name, public_id, system_id;
  protected NamedNodeMap entities, notations;
  protected DOMImplementation implementation;

  int get_node_type() { return DOCUMENT_TYPE_NODE; }
  string get_node_name() { return get_name(); }
  string get_name() { return name; }
  string get_public_id() { return public_id; }
  string get_system_id() { return system_id; }
  NamedNodeMap get_entities() { return entities || create_entities(); }
  NamedNodeMap get_notations() { return notations || create_notations(); }

  protected void _set_owner_document(Document d) { owner_document = d; }

  protected NamedNodeMap create_entities()
  {
    return entities = NamedNodeMap(owner_document);
  }

  protected NamedNodeMap create_notations()
  {
    return notations = NamedNodeMap(owner_document);
  }

  protected void create(DOMImplementation i, string qn,
		     string|void pubid, string|void sysid)
  {
    implementation = i;
    name = qn;
    public_id = pubid;
    system_id = sysid;
  }
}

//!
class Notation
{
  inherit Node;

  protected string name, public_id, system_id;

  int get_node_type() { return NOTATION_NODE; }
  string get_node_name() { return name; }
  string get_public_id() { return public_id; }
  string get_system_id() { return system_id; }
  protected int is_readonly() { return 1; }

  protected void create(Document owner, string _name, string p_id, string s_id)
  {
    owner_document = owner;
    name = _name;
    public_id = p_id;
    system_id = s_id;
  }
}

//!
class Entity
{
  inherit Node;

  protected string name;
  protected string|zero public_id, system_id, notation_name;

  int get_node_type() { return ENTITY_NODE; }
  string get_node_name() { return name; }
  string|zero get_public_id() { return public_id; }
  string|zero get_system_id() { return system_id; }
  string|zero get_notation_name() { return notation_name; }
  protected int is_readonly() { return name != 0; }

  protected string cast(string to)
  {
    if(to == "string")
      return ((array(string))get_child_nodes())*"";
    return UNDEFINED;
  }

  protected int child_is_allowed(Node child)
  {
    return (<ELEMENT_NODE,PROCESSING_INSTRUCTION_NODE,
	     COMMENT_NODE,TEXT_NODE,CDATA_SECTION_NODE,
	     ENTITY_REFERENCE_NODE>)[child->get_node_type()];
  }

  protected void create(Document owner, string _name,
			string|zero p_id, string|zero s_id,
			string|zero n_name, DocumentFragment|void value)
  {
    owner_document = owner;
    public_id = p_id;
    system_id = s_id;
    notation_name = n_name;
    if(value)
      append_child(value);
    name = _name;
  }
}

//!
class EntityReference
{
  inherit Node;

  protected string name;
  protected Entity entity;

  int get_node_type() { return ENTITY_REFERENCE_NODE; }
  string get_node_name() { return name; }
  protected int is_readonly() { return 1; }

  Node clone_node(int|void deep) {
    return owner_document->create_entity_reference(name);
  }

  protected string cast(string to)
  {
    if(to == "string")
      return (entity? (string)entity : "&"+name+";");
    return UNDEFINED;
  }

  NodeList get_child_nodes() { return entity && entity->get_child_nodes(); }
  Node get_first_child() { return entity && entity->get_first_child(); }
  Node get_last_child() { return entity && entity->get_last_child(); }

  Node get_previous_sibling(Node|void node) {
    return node? entity && entity->get_previous_sibling(node) :
      ::get_previous_sibling();
  }

  Node get_next_sibling(Node|void node) {
    return node? entity && entity->get_next_sibling(node) :
      ::get_next_sibling();
  }

  int has_child_nodes() { return entity && entity->has_child_nodes(); }

  protected void create(Document owner, string _name, Entity|void _entity)
  {
    owner_document = owner;
    name = _name;
    entity = _entity;
  }
}

//!
class ProcessingInstruction
{
  inherit Node;

  protected string target, node_value;

  int get_node_type() { return PROCESSING_INSTRUCTION_NODE; }
  string get_node_name() { return get_target(); }
  string get_target() { return target; }
  string get_node_value() { return node_value; }
  void set_node_value(string data)
  {
    if(is_readonly())
      throw(DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR));
    node_value=data;
  }
  string get_data() { return get_node_value(); }
  void set_data(string data) { set_node_value(data); }

  Node clone_node(int|void deep) {
    return owner_document->create_processing_instruction(target, get_data());
  }

  protected void create(Document owner, string _target, string data)
  {
    owner_document = owner;
    target = _target;
    set_data(data);
  }
}

//!
class ParseException
{
  protected string message, pubid, sysid;
  protected int loc;

  constant is_generic_error = 1;

  protected array backtrace = predef::backtrace()[..<2];

  int get_location() { return loc; }
  string get_public_id() { return pubid; }
  string get_system_id() { return sysid; }
  string get_message() { return message; }

  protected string|array `[] (int i)
  {
    switch (i) {
      case 0:
	return "ParseException: " + sysid + ":" + loc + ": " + message+"\n";
      case 1:
	return backtrace;
    }
  }

  protected string _sprintf(int mode)
  {
    return mode == 'O' &&
      sprintf("%O(%O /* %O char %d */)",
	      this_program, message, sysid, loc);
  }

  protected void create(string _message, string|void _sysid, string|void _pubid,
		     int|void _loc)
  {
    message = _message;
    sysid = _sysid;
    pubid = _pubid;
    loc = _loc;
  }
}

//!
class InputSource {

  protected string sysid, pubid, encoding;
  protected Stdio.File file;

  string get_public_id() { return pubid; }
  string get_system_id() { return sysid; }
  string get_encoding() { return encoding; }
  void set_public_id(string id) { pubid = id; }
  void set_system_id(string id) { sysid = id; }
  void set_encoding(string enc) { encoding = enc; }

  Stdio.File get_file() { return file; }
  void set_file(Stdio.File f) { file = f; }

  string get_data()
  {
    string data = get_file()->read();
    return data;
  }

  protected Stdio.File get_external_file(string sysid, string|void pubid)
  {
    Stdio.File f = Stdio.File();
    if(!f->open(sysid, "r"))
      throw(ParseException(sysid+": file not found", sysid, pubid));
    return f;
  }

  protected void create(Stdio.File|string|void input)
  {
    if(input)
      if(stringp(input)) {
	set_system_id(input);
	set_file(get_external_file(input));
	} else
	  set_file(input);
  }
}

//!
class AbstractDOMParser
{
  protected class ErrorHandler {
    void error(ParseException exception);
    void fatal_error(ParseException exception);
    void warning(ParseException exception);
  };

  protected ErrorHandler error_handler = this;

  void error(ParseException exception) { throw(exception); }
  void fatal_error(ParseException exception) { throw(exception); }
  void warning(ParseException exception) { }

  void set_error_handler(ErrorHandler handler) { error_handler = handler; }

  protected Document document;
  protected Node current_node;
  protected array(Node) node_stack;

  Document get_document() { return document; }

  protected DOMImplementation get_dom_implementation()
  {
    return DOMImplementation();
  }

  protected Document create_document(InputSource s)
  {
    // NB: Argument s ignored.
    return get_dom_implementation()->create_document("", "", 0);
  }

  protected object|void parse_callback(string ty, string name, mapping attributes,
				    array|string contents, mapping info,
				    InputSource input)
  {
    switch(ty) {
     case "<!--":
       current_node->append_child(document->create_comment(contents));
       break;
     case "<":
     case "<>":
       Element e = document->create_element(name);
       foreach(indices(attributes), string att_name)
	 e->set_attribute(att_name, attributes[att_name]);
       current_node->append_child(e);
       node_stack += ({ current_node });
       current_node = e ;
       if(ty == "<")
	 break;
     case ">":
       current_node = node_stack[-1];
       node_stack = node_stack[..<1];
       break;
     case "":
       current_node->append_child(document->create_text_node(contents));
       break;
     case "<![CDATA[":
       current_node->append_child(document->create_cdata_section(contents));
       break;
     case "<?":
       current_node->append_child(document->
				  create_processing_instruction(name,
								contents));
       break;
     case "<?xml":
       break;
     case "<!DOCTYPE":
       {
         DocumentType doctype = document->get_implementation()->
	   create_document_type(name, attributes->PUBLIC,
				attributes->SYSTEM);

	 current_node->append_child(doctype);

	 if(contents)
	   foreach(contents, object elt)
	     if(elt)
	       switch(elt->get_node_type()) {
		case Node.NOTATION_NODE:
		  doctype->get_notations()->set_named_item(elt);
		  break;
		case Node.ENTITY_NODE:
		  doctype->get_entities()->set_named_item(elt);
		  break;
	       }
	 break;
       }
     case "<!NOTATION":
       return Notation(document, name, attributes->PUBLIC, attributes->SYSTEM);
       break;
     case "<!ENTITY":
       if(name[0] != '%')
	 if(contents) {
	   DocumentFragment frag = document->create_document_fragment();
	   frag->append_child(document->create_text_node(contents));
	   return Entity(document, name, 0, 0, 0, frag);
	 } else
	   return Entity(document, name, attributes->PUBLIC,
			 attributes->SYSTEM, attributes->NDATA);
       break;
     case "<!ELEMENT":
     case "<!ATTLIST":
       break;
     case "error":
       throw(ParseException(contents, input->get_system_id(),
			    input->get_public_id(), info->location));
    }
  }

  protected void _parse(string data, function cb, InputSource input);

  void parse(InputSource|string source)
  {
    if(stringp(source))
      source = InputSource(source);
    current_node = document = create_document(source);
    node_stack = ({});
    _parse(source->get_data(), parse_callback, source);
  }
}

//!
class NonValidatingDOMParser
{
  inherit AbstractDOMParser;
  protected inherit .Simple : xml;

  protected string autoconvert(string data, InputSource input)
  {
    mixed err = catch{ return xml::autoconvert(data); };
    throw(ParseException(err[0], input->get_system_id(),
			 input->get_public_id(), 0));
  }

  protected void _parse(string data, function cb, InputSource input)
  {
    xml::parse(autoconvert(data, input), cb, input);
  }
}

//!
class DOMParser
{
  inherit AbstractDOMParser;
  protected inherit Parser.XML.Validating : xml;

  protected string autoconvert(string data, InputSource input)
  {
    mixed err = catch{ return xml::autoconvert(data); };
    throw(ParseException(err[0]-"\n", input->get_system_id(),
			 input->get_public_id(), 0));
  }

  string get_external_entity(string sysid, string|void pubid,
			     int|void unparsed, InputSource input)
  {
    InputSource is =
      InputSource(combine_path(combine_path(input->get_system_id(), ".."),
			       sysid));
    return (unparsed? is->get_data() : autoconvert(is->get_data(), is));
  }

  protected void _parse(string data, function cb, InputSource input)
  {
    xml::parse(autoconvert(data, input), cb, input);
  }
}
