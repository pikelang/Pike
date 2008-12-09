// $Id: SloppyDOM.pmod,v 1.3 2008/12/09 18:56:23 nilsson Exp $

#pike __REAL_VERSION__

//! A somewhat DOM-like library that implements lazy generation of the
//! node tree, i.e. it's generated from the data upon lookup. There's
//! also a little bit of XPath evaluation to do queries on the node
//! tree.
//!
//! Implementation note: This is generally more pragmatic than
//! @[Parser.XML.DOM], meaning it's not so pretty and compliant, but
//! more efficient.
//!
//! Implementation status: There's only enough implemented to parse a
//! node tree from source and access it, i.e. modification functions
//! aren't implemented. Data hiding stuff like NodeList and
//! NamedNodeMap is not implemented, partly since it's cumbersome to
//! meet the "live" requirement. Also, @[Parser.HTML] is used in XML
//! mode to parse the input. Thus it's too error tolerant to be XML
//! compliant, and it currently doesn't handle DTD elements, like
//! @tt{"<!DOCTYPE"@}, or the XML declaration (i.e. @tt{"<?xml
//! version='1.0'?>"@}.

// Created 2002-02-14 by Martin Stjernholm

Document parse (string source, void|int raw_values)
//! Normally entities are decoded, and @[Node.xml_format] will encode
//! them again. If @[raw_values] is nonzero then all text and attribute
//! values are instead kept in their original form.
{
  return Document (source, raw_values);
}

//class DOMImplementation {}
//class NodeList {}
//class NamedNodeMap {}

class Node
//!  Basic node.
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

  Node parent_node;
  Document owner_document;

  string get_node_value() {return 0;}
  void set_node_value (string value);

  string get_node_name();
  int get_node_type();
  Node get_parent_node() {return parent_node;}
  //NodeList get_child_nodes();
  Node get_first_child() {return 0;}
  Node get_last_child() {return 0;}
  Node get_previous_sibling()
    {return parent_node && parent_node->_get_child_by_pos (pos_in_parent - 1);}
  Node get_next_sibling()
    {return parent_node && parent_node->_get_child_by_pos (pos_in_parent + 1);}

  //NamedNodeMap get_attributes() {return 0;}
  Document get_owner_document() {return owner_document;}

  Node insert_before(Node new_child, Node ref_child);
  Node replace_child(Node new_child, Node old_child);
  Node remove_child(Node old_child);
  Node append_child(Node new_child);
  int has_child_nodes() {return 0;}
  Node clone_node (int|void deep);

  string get_text_content()
  //! If the @tt{raw_values@} flag is set in the owning document, the
  //! text is returned with entities and CDATA blocks intact.
  //!
  //! @seealso
  //! @[parse]
  {
    String.Buffer res = String.Buffer();
    _text_content (res);
    return res->get();
  }

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path (string path, void|int xml_format);
  //! Access a node or a set of nodes through an expression that is a
  //! subset of an XPath RelativeLocationPath in abbreviated form.
  //!
  //! That means one or more Steps separated by "/" or "//". A Step
  //! consists of an AxisSpecifier followed by a NodeTest and then
  //! optionally by one or more Predicate's.
  //!
  //! "/" before a Step causes it to be matched only against the
  //! immediate children of the node(s) selected by the previous Step.
  //! "//" before a Step causes it to be matched against any children
  //! in the tree below the node(s) selected by the previous Step.
  //! The initial selection before the first Step is this element.
  //!
  //! The currently allowed AxisSpecifier NodeTest combinations are:
  //!
  //! @ul
  //! @item
  //!   @tt{name@} to select all elements with the given name. The
  //!   name can be @tt{"*"@} to select all.
  //! @item
  //!   @tt{@@name@} to select all attributes with the given name. The
  //!   name can be @tt{"*"@} to select all.
  //! @item
  //!   @tt{comment()@} to select all comments.
  //! @item
  //!   @tt{text()@} to select all text and CDATA blocks. Note that
  //!   all entity references are also selected, under the assumption
  //!   that they would expand to text only.
  //! @item
  //!   @tt{processing-instruction("name")@} to select all processing
  //!   instructions with the given name. The name can be left out to
  //!   select all. Either @tt{'@} or @tt{"@} may be used to delimit
  //!   the name. For compatibility, it can also occur without
  //!   surrounding quotes.
  //! @item
  //!   @tt{node()@} to select all nodes, i.e. the whole content of an
  //!   element node.
  //! @item
  //!   @tt{.@} to select the currently selected element itself.
  //! @endul
  //!
  //! A Predicate is on the form @tt{[PredicateExpr]@} where
  //! PredicateExpr currently can be in any of the following forms:
  //!
  //! @ul
  //! @item
  //!   An integer indexes one item in the selected set, according to
  //!   the document order. A negative index counts from the end of
  //!   the set.
  //! @item
  //!   A RelativeLocationPath as specified above. It's executed for
  //!   each element in the selected set and those where it yields an
  //!   empty result are filtered out while the rest remain in the
  //!   set.
  //! @item
  //!   A RelativeLocationPath as specified above followed by
  //!   @tt{="value"@}. The path is executed for each element in the
  //!   selected set and those where the text result of it is equal to
  //!   the given value remain in the set. Either @tt{'@} or @tt{"@}
  //!   may be used to delimit the value.
  //! @endul
  //!
  //! If @[xml_format] is nonzero, the return value is an xml
  //! formatted string of all the matched nodes, in document order.
  //! Otherwise the return value is as follows:
  //!
  //! Attributes are returned as one or more index/value pairs in a
  //! mapping. Other nodes are returned as the node objects. If the
  //! expression is on a form that can give at most one answer (i.e.
  //! there's a predicate with an integer index) then a single mapping
  //! or node is returned, or zero if there was no match. If the
  //! expression can give more answers then the return value is an
  //! array containing zero or more attribute mappings and/or nodes.
  //! The array follows document order.
  //!
  //! @note
  //! Not DOM compliant.

  string xml_format()
  //! Returns the formatted XML that corresponds to the node tree.
  //!
  //! @note
  //! Not DOM compliant.
  {
    String.Buffer res = String.Buffer();
    _xml_format (res);
    return res->get();
  }

  // Internals.

  static constant class_name = "Node";

  /*protected*/ int pos_in_parent;

  /*protected*/ Document _get_doc() {return owner_document;}
  /*protected*/ void _text_content (String.Buffer into);
  /*protected*/ void _xml_format (String.Buffer into);
  /*protected*/ void _destruct_tree() {destruct (this_object());}

#define WS "%*[ \t\n\r]"
#define NAME "%[^][ \t\n\r/@(){},=.]"

  void simple_path_error (string msg, mixed... args)
  {
    if (sizeof (args)) msg = sprintf (msg, @args);
    msg += sprintf ("%s node%s.\n", class_name,
		    this_object()->node_name ?
		    " " + this_object()->node_name : "");
    error (msg);
  };

  static array(string) parse_trivial_node_type_path (string path)
  {
    string orig_path = path;
    while (sscanf (path, WS"."WS"%s", string rest) == 3) {
      if (has_prefix (rest, "//")) path = rest[2..];
      else if (has_prefix (rest, "/")) path = rest[1..];
      else break;
    }
    if (sscanf (path, WS""NAME""WS"("WS"%s",
		string node_type, string rest) == 5) {
      string arg;
      if (sscanf (rest, ")"WS"%s", rest) != 2 &&
	  (sscanf (rest, "'%[^']'"WS")"WS"%s", arg, rest) != 4 ||
	   sscanf (rest, "\"%[^\"]\""WS")"WS"%s", arg, rest) != 4))
	simple_path_error ("Invalid argument list after %O in %O in ",
			   node_type, orig_path);
      if (rest != "")
	simple_path_error ("Unexpected expression %O after node type %s in ",
			   rest, node_type);
      if (!(<"node", "comment", "text", "processing-instruction">)[node_type])
	simple_path_error ("Invalid node type %O in %O in ",
			   node_type, orig_path);
      if (arg && node_type != "processing-instruction")
	simple_path_error ("Cannot give an argument to the node type %s "
			   "in %O in ", node_type, orig_path);
      return ({node_type, arg});
    }
    return ({0, 0});
  }

  static string sprintf_name (int flag) {return "";}
  static string sprintf_attr (int flag) {return "";}
  static string sprintf_content (int flag) {return "";}

  string _sprintf (int flag)
  {
    switch (flag) {
      case 'O':
	return "SloppyDOM." + class_name + "(" + sprintf_name ('O') + ")";
      case 'V':
	string res = sprintf_name ('V') + sprintf_attr ('V');
	string c = sprintf_content ('V');
	if (sizeof (c))
	  if (has_value (c, "\n") || sizeof (c) > 50)
	    res += (sizeof (res) ? ":" : "") + "\n  " + replace (c, "\n", "\n  ");
	  else
	    res += (sizeof (res) ? ": " : "") + c;
	return "SloppyDOM." + class_name + "(" + res + ")";
    }
  }
}

#define CHECK_CONTENT							\
  if (stringp (content))						\
    content = sloppy_parse_fragment (content, this_object());
#define NODE_AT(POS) (stringp (content[POS]) ? make_node (POS) : content[POS])

static class NodeWithChildren
{
  inherit Node;

  Node get_first_child()
  {
    CHECK_CONTENT;
    if (content && sizeof (content)) return NODE_AT (0);
    return 0;
  }

  Node get_last_child()
  {
    CHECK_CONTENT;
    if (content && sizeof (content)) return NODE_AT (-1);
    return 0;
  }

  int has_child_nodes() {return content && sizeof (content);}

  // Internals.

  static constant class_name = "NodeWithChildren";

  /*protected*/ string|array(string|Node) content;

  /*protected*/ Node _get_child_by_pos (int pos)
  {
    if (pos < 0) return 0;
    CHECK_CONTENT;
    if (content && pos < sizeof (content)) return NODE_AT (pos);
    return 0;
  }

  /*protected*/ void _destruct_tree()
  {
    if (arrayp (content))
      foreach (content, string|Node child)
	if (objectp (child)) child->_destruct_tree();
    destruct (this_object());
  }

  /*protected*/ Node make_node (int pos)
  {
    Document doc = _get_doc();
    string text = content[pos];
    Node node;
    if (has_prefix (text, "&")) {
      text = text[1..sizeof (text) - 2];
      if (string decoded = !doc->raw_values &&
	  (Parser.html_entities[text] || Parser.decode_numeric_xml_entity (text)))
	node = Text (doc, decoded);
      else
	node = EntityReference (doc, text);
    }
    else if (has_prefix (text, "<!--"))
      node = Comment (doc, text[4..sizeof (text) - 4]);
    else if (has_prefix (text, "<?")) {
      sscanf (text[..sizeof (text) - 3], "<?%[^ \t\n\r]%*[ \t\n\r]%s",
	      string target, string data);
      node = ProcessingInstruction (doc, target, data || "");
    }
    else if (has_prefix (text, "<![CDATA["))
      node = CDATASection (doc, text[9..sizeof (text) - 4]);
    else
      node = Text (doc, text);
    content[pos] = node;
    node->parent_node = this_object();
    node->pos_in_parent = pos;
    return node;
  }

  /*protected*/ void make_all_nodes()
  {
    CHECK_CONTENT;
    if (arrayp (content))
      for (int i = sizeof (content) - 1; i >= 0; i--)
	if (stringp (content[i]))
	  make_node (i);
  }

  static void format_attrs (mapping(string:string) attrs, String.Buffer into)
  {
    if (owner_document->raw_values)
      foreach (indices (attrs), string attr) {
	string var = attrs[attr];
	if (has_value (var, "\""))
	  into->add (" ", attr, "='", var, "'");
	else
	  into->add (" ", attr, "=\"", var, "\"");
      }
    else
      foreach (indices (attrs), string attr)
	into->add (" ", attr, "=\"",
		   // Serial replace's are currently faster than one parallell.
		   replace (replace (replace (attrs[attr],
					      "&", "&amp;"),
				     "<", "&lt;"),
			    "\"", "&quot;"),
		   "\"");
  }

  /*protected*/ void _text_content (String.Buffer into)
  {
    CHECK_CONTENT;
    if (arrayp (content))
      for (int i = 0; i < sizeof (content); i++) {
	string|Node child = content[i];
	if (objectp (child))
	  switch (child->node_type) {
	    case COMMENT_NODE:
	    case PROCESSING_INSTRUCTION_NODE:
	      break;
	    default:
	      child->_text_content (into);
	  }
	else
	  if (has_prefix (child, "&") || has_prefix (child, "<![CDATA["))
	    make_node (i)->_text_content (into);
	  else if (!has_prefix (child, "<!--") && !has_prefix (child, "<?"))
	    into->add (child);
      }
  }

  static void xml_format_children (String.Buffer into)
  {
    if (stringp (content)) into->add (content);
    if (arrayp (content))
      foreach (content, string|Node child)
	if (stringp (child)) into->add (child);
	else child->_xml_format (into);
  }

  static string sprintf_content (int flag)
  {
    if (stringp (content)) return sprintf ("%O", content);
    if (arrayp (content))
      return map (content, lambda (string|Node child) {
			     if (stringp (child)) return sprintf ("%O", child);
			     return child->_sprintf (flag);
			   }) * ",\n";
    return "";
  }
}

#define CHECK_LOOKUP_MAPPING if (!id_prefix) fix_lookup_mapping();

static int last_used_id = 0;

static class NodeWithChildElements
//!  Node with child elements.
{
  inherit NodeWithChildren;

  string get_attribute (string name);

  //NodeList get_elements_by_tag_name (string tag_name);

  array(Element) get_elements (string name)
  //! Lightweight variant of @[get_elements_by_tag_name] that returns
  //! a simple array instead of a fancy live NodeList.
  //!
  //! @note
  //! Not DOM compliant.
  {
    if (name == "*") {
      CHECK_CONTENT;
      if (!content) return ({});
      return filter (content,
		     lambda (string|Node child) {
		       return objectp (child) && child->node_type == ELEMENT_NODE;
		     });
    }
    else {
      CHECK_LOOKUP_MAPPING;
      return owner_document->_lookup_mapping[id_prefix + name] || ({});
    }
  }

  array(Element) get_descendant_elements()
  //! Returns all descendant elements in document order.
  //!
  //! @note
  //! Not DOM compliant.
  {
    CHECK_CONTENT;
    if (!content) return ({});
    array(Element) res = ({});
    foreach (content, string|Node child) {
      if (objectp (child) && child->node_type == ELEMENT_NODE)
	res += ({child}) + child->get_descendant_elements();
    }
    return res;
  }

  array(Node) get_descendant_nodes()
  //! Returns all descendant nodes (except attribute nodes) in
  //! document order.
  //!
  //! @note
  //! Not DOM compliant.
  {
    CHECK_CONTENT;
    if (!content) return ({});
    array(Node) res = ({});
    for (int i = 0; i < sizeof (content); i++) {
      string|Node child = content[i];
      if (!objectp (child)) child = make_node (i);
      res += ({child});
      if (child->node_type == ELEMENT_NODE)
	res += child->get_descendant_nodes();
    }
    return res;
  }

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path (string path, void|int xml_format)
  {
    return simple_path_recur (path, xml_format, 0);
  }

  // Internals.

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path_recur (string path, int xml_format, int rec_search)
  {
    string orig_path = path;
    sscanf (path, ""WS NAME WS"%s", string name, path);

    string num_ord (int nr)
    {
      return sprintf ("%d%s", nr,
		      nr % 10 == 1 && nr != 11 ? "st" :
		      nr % 10 == 2 && nr != 12 ? "nd" :
		      nr % 10 == 3 && nr != 13 ? "rd" :
		      "th");
    };

    mapping(string:string)|Node|array(Node|int) res;

    if (name == "") {
      if (sscanf (path, "@"WS NAME WS"%s", name, path)) {
	if (!sizeof (name))
	  simple_path_error ("No attribute name after @ in ");

	foreach (rec_search ? ({this_object()}) + get_descendant_elements() :
		 ({this_object()}), NodeWithChildElements node) {
	  mapping(string:string) attr = node->attributes;
	  if (!mappingp (attr))
	    simple_path_error ("Cannot access an attribute %O in ", name);
	  if (name == "*") {
	    if (res) res += attr;
	    else res = attr + ([]);
	  }
	  else if (string val = attr[name]) {
	    if (res) res[name] = val;
	    else res = ([name: val]);
	  }
	}

	if (!res)
	  return xml_format && "";
      }

      else if (sscanf (path, "."WS"%s", path))
	res = rec_search ? get_descendant_nodes() : ({this});

      else simple_path_error ("Invalid path %O in ", path);
    }

    else if (has_prefix (path, "(")) {
      string arg;
      // The first syntax below is not correct, but we need to support
      // it for compatibility.
      if (sscanf (path, "("WS NAME WS")"WS"%s", arg, path) != 5 &&
	  sscanf (path, "("WS"'%[^']'"WS")"WS"%s", arg, path) != 5 &&
	  sscanf (path, "("WS"\"%[^\"]\""WS")"WS"%s", arg, path) != 5)
	simple_path_error ("Invalid node type expression in %O in ",
			   name + path);
      if (sizeof (arg) && name != "processing-instruction")
	simple_path_error ("Cannot give an argument %O to the node type %s in ",
			   arg, name);

      if (name == "node") {
	if (rec_search)
	  res = get_descendant_nodes();
	else if (xml_format && !sizeof (path)) {
	  String.Buffer buf = String.Buffer();
	  xml_format_children (buf);
	  return buf->get();
	}
	else {
	  CHECK_CONTENT;
	  res = allocate (sizeof (content));
	  for (int i = sizeof (res) - 1; i >= 0; i--)
	    if (objectp (content[i])) res[i] = content[i];
	    else res[i] = i;
	}
      }

      else {
	CHECK_CONTENT;
	res = ({});

	switch (name) {
	  case "comment":
	    if (rec_search) {
	      foreach (get_descendant_nodes(), Node node)
		if (node->node_type == COMMENT_NODE)
		  res += ({node});
	    }

	    else {
	      for (int i = 0; i < sizeof (content); i++) {
		string|Node child = content[i];
		if (objectp (child)) {
		  if (child->node_type == COMMENT_NODE)
		    res += ({child});
		}
		else
		  if (has_prefix (child, "<!--"))
		    res += ({i});
	      }
	    }
	    break;

	  case "text":
	    if (rec_search) {
	      foreach (get_descendant_nodes(), Node node)
		if ((<TEXT_NODE, ENTITY_REFERENCE_NODE,
		      CDATA_SECTION_NODE>)[node->node_type])
		  res += ({node});
	    }

	    else {
	      //normalize();
	      for (int i = 0; i < sizeof (content); i++) {
		string|Node child = content[i];
		if (objectp (child)) {
		  if ((<TEXT_NODE, ENTITY_REFERENCE_NODE,
			CDATA_SECTION_NODE>)[child->node_type])
		    res += ({child});
		}
		else
		  if (!has_prefix (child, "<!--") && !has_prefix (child, "<?"))
		    res += ({i});
	      }
	    }
	    break;

	  case "processing-instruction":
	    if (sizeof (arg)) {
	      string scanfmt =
		"<?" + replace (arg, "%", "%%") + "%[ \t\n\r]";

	      if (rec_search) {
		foreach (get_descendant_nodes(), Node node)
		  if (node->node_type == PROCESSING_INSTRUCTION_NODE &&
		      node->node_name == arg)
		    res += ({node});
	      }

	      else {
		for (int i = 0; i < sizeof (content); i++) {
		  string|Node child = content[i];
		  if (objectp (child)) {
		    if (child->node_type == PROCESSING_INSTRUCTION_NODE &&
			child->node_name == arg)
		      res += ({child});
		  }
		  else
		    if (sscanf (child, scanfmt, string ws) &&
			(sizeof (ws) || sizeof (child) == sizeof (arg) + 4))
		      res += ({i});
		}
	      }
	    }

	    else {
	      if (rec_search) {
		foreach (get_descendant_nodes(), Node node)
		  if (node->node_type == PROCESSING_INSTRUCTION_NODE)
		    res += ({node});
	      }

	      else {
		for (int i = 0; i < sizeof (content); i++) {
		  string|Node child = content[i];
		  if (objectp (child)) {
		    if (child->node_type == PROCESSING_INSTRUCTION_NODE)
		      res += ({child});
		  }
		  else
		    if (has_prefix (child, "<?"))
		      res += ({i});
		}
	      }
	    }
	    break;

	  default:
	    simple_path_error ("Invalid node type %s in ", name);
	}
      }
    }

    else {
      if (rec_search) {
	if (name == "*")
	  res = get_descendant_elements();
	else {
	  res = ({});
	  // Can't use get_elements here since we need to preserve the
	  // document order.
	  foreach (get_descendant_elements(), NodeWithChildElements node)
	    if (node->node_name == name)
	      res += ({node});
	}
      }
      else
	res = get_elements (name);
    }

    //werror ("%O: path %O before preds: %O\n", this, path, res);

    string preds_path = path;
    for (int pred_num = 1; has_prefix (path, "["); pred_num++) {

      if (sscanf (path, "["WS"%d"WS"]"WS"%s", int index, path) == 5) {
	if (!index)
	  simple_path_error (
	    "Invalid index 0 in %s predicate after %O in ",
	    num_ord (pred_num),
	    orig_path[..sizeof (orig_path) - sizeof (preds_path) - 1]);

	mapping(string:string)|int|Node elem;

	if (index > 0) {
	  if (index > sizeof (res)) return xml_format && "";
	  if (mappingp (res))
	    elem = (mapping) ({((array) res)[index - 1]});
	  else
	    elem = res[index - 1];
	}
	else {
	  if (index < -sizeof (res)) return xml_format && "";
	  if (mappingp (res))
	    elem = (mapping) ({((array) res)[index]});
	  else
	    elem = res[index];
	}

	res = intp (elem) ? make_node (elem) : elem;
      }

      else {
	string pred_expr = "";
	string value;
	int depth = 1;
	path = path[1..];
      find_balanced_brackets:
	while (sscanf (path, "%[^][=]%1s%s",
		       string pre, string delim, path) == 3) {
	  pred_expr += pre;
	  switch (delim) {
	    case "[":
	      depth++;
	      break;
	    case "]":
	      depth--;
	      if (!depth) break find_balanced_brackets;
	      break;
	    case "=":
	      if (depth == 1) {
		if (sscanf (path, WS"'%[^']'"WS"]"WS"%s", value, path) == 5 ||
		    sscanf (path, WS"\"%[^\"]\""WS"]"WS"%s", value, path) == 5){
		  depth = 0;
		  break find_balanced_brackets;
		}
		simple_path_error (
		  "Invalid equals operand in %s predicate after %O in ",
		  num_ord (pred_num),
		  orig_path[..sizeof (orig_path) - sizeof (preds_path) - 1]);
	      }
	      break;
	  }
	  pred_expr += delim;
	}
	if (depth)
	  simple_path_error (
	    "Unbalanced brackets in %s predicate after %O in ",
	    num_ord (pred_num),
	    orig_path[..sizeof (orig_path) - sizeof (preds_path) - 1]);

	sscanf (pred_expr, WS"%s", pred_expr);

	if (mappingp (res)) {
	  if (sscanf (pred_expr, "@"WS NAME WS"%s",
		      string attr_name, string rest)) {
	    if (rest != "")
	      simple_path_error (
		"Invalid %s predicate after %O in ",
		num_ord (pred_num),
		orig_path[..sizeof (orig_path) - sizeof (preds_path) - 1]);
	    if (!(value ? res[attr_name] == value : res[attr_name]))
	      return xml_format && "";
	    res = ([attr_name: res[attr_name]]);
	  }
	  else
	    return xml_format && "";
	}

	else {
	  array filtered_res = ({});

	  if (value) {
	  res_loop:
	    foreach (res, int|Node elem) {
	      if (intp (elem)) elem = make_node (elem);
	      if (mixed pred_res = elem->simple_path_recur (pred_expr, 0, 0)) {
		if (arrayp (pred_res)) {
		  foreach (pred_res, Node pred_node)
		    if (pred_node->get_text_content() == value) {
		      filtered_res += ({elem});
		      continue res_loop;
		    }
		}
		else if (mappingp (pred_res)) {
		  if (values (pred_res)[0] == value) {
		    filtered_res += ({elem});
		    continue res_loop;
		  }
		}
		else
		  if (pred_res->get_text_content() == value) {
		    filtered_res += ({elem});
		    continue res_loop;
		  }
	      }
	    }
	  }

	  else {
	    foreach (res, int|Node elem) {
	      if (intp (elem)) elem = make_node (elem);
	      if (mixed pred_res = elem->simple_path_recur (pred_expr, 0, 0))
		if (objectp (pred_res) || sizeof (pred_res))
		  filtered_res += ({elem});
	    }
	  }

	  res = filtered_res;
	  if (!sizeof (res)) return xml_format ? "" : ({});
	}
      }

      //werror ("%O: path %O after pred %d: %O\n", this, path, pred_num, res);
    }

    if (arrayp (res))
      for (int i = sizeof (res) - 1; i >= 0; i--)
	if (intp (res[i])) res[i] = make_node (res[i]);

    if (sizeof (path)) {
      if (has_prefix (path, "//")) {
	path = path[2..];
	if (arrayp (res) ||
	    (objectp (res) && res->get_elements && (res = ({res})))) {
	  array(Node) collected = ({});
	  // res might contain nodes that are inside other nodes in
	  // res (e.g. when processing the second "//" in expressions
	  // like ".//a//b"). We therefore use |= below to avoid
	  // duplicates. Since res is in document order we can always
	  // keep the first instance to produce a result in document
	  // order (actually, we could remove all nested elements in
	  // res and still get a complete result).
	  foreach (res, Node node) {
	    mapping(string:string)|Node|array(mapping(string:string)|Node)
	      subres = node->simple_path_recur (path, 0, 1);
	    if (objectp (subres))
	      collected |= ({subres});
	    else if (arrayp (subres) && sizeof (subres))
	      collected |= subres;
	  }
	  res = collected;
	}
      }

      else if (has_prefix (path, "/")) {
	path = path[1..];

	if (arrayp (res)) {
	  if (xml_format) {
	    String.Buffer collected = String.Buffer();
	    foreach (res, Node child)
	      if (child->get_elements) {
		string subres = child->simple_path_recur (path, 1, 0);
		collected->add (subres);
	      }
	    return collected->get();
	  }

	  else {
	    array(mapping(string:string)|Node) collected = ({});
	    foreach (res, Node child)
	      if (child->get_elements) {
		mapping(string:string)|Node|array(mapping(string:string)|Node)
		  subres = child->simple_path_recur (path, 0, 0);
		if (objectp (subres) || mappingp (subres))
		  collected += ({subres});
		else if (arrayp (subres))
		  collected += subres;
	      }
	    return collected;
	  }
	}
	else if (objectp (res) && res->get_elements)
	  return res->simple_path_recur (path, xml_format, 0);
	else
	  return xml_format ? "" : ({});
      }

      else
	simple_path_error ("Invalid expression %O after ", path);
    }

    if (xml_format) {
      if (!res) return "";
      String.Buffer collected = String.Buffer();
      if (mappingp (res))
	format_attrs (res, collected);
      else
	// Works both when res is Node and array(Node).
	res->_xml_format (collected);
      string formatted = collected->get();
      //werror ("%O: path %O, leaf result %O\n", this, orig_path, formatted);
      return formatted;
    }

    //werror ("%O: path %O, leaf result %O\n", this, orig_path, res);
    return res;
  }

  static constant class_name = "NodeWithChildElements";

  static string id_prefix;

  static void fix_lookup_mapping()
  {
    id_prefix = (string) ++last_used_id + ":";
    CHECK_CONTENT;
    if (content) {
      mapping lm = _get_doc()->_lookup_mapping;
      foreach (content, string|Node child)
	if (objectp (child) && child->node_type == ELEMENT_NODE)
	  lm[id_prefix + child->node_name] += ({child});
    }
  }
}

//class DocumentFragment {}

class Document
//! @note
//! The node tree is very likely a cyclic structure, so it might be an
//! good idea to destruct it when you're finished with it, to avoid
//! garbage. Destructing the @[Document] object always destroys all
//! nodes in it.
{
  inherit NodeWithChildElements;

  constant node_type = DOCUMENT_NODE;
  int get_node_type() { return DOCUMENT_NODE; }
  string get_node_name() { return "#document"; }

  //DOMImplementation get_implementation();
  //DocumentType get_doctype();

  Element get_document_element()
  {
    if (!document_element) {
      CHECK_CONTENT;
      foreach (content, string|Node node)
	if (objectp (node) && node->node_type == ELEMENT_NODE)
	  return document_element = node;
    }
    return document_element;
  }

#if 0
  // Disabled for now since the tree can't be manipulated anyway.
  Element create_element (string tag_name)
    {return Element (this_object(), tag_name);}
  //DocumentFragment create_document_fragment();
  Text create_text_node (string data)
    {return Text (this_object(), data);}
  Comment create_comment (string data)
    {return Comment (this_object(), data);}
  CDATASection create_cdata_section (string data)
    {return CDATASection (this_object(), data);}
  ProcessingInstruction create_processing_instruction (string target, string data)
    {return ProcessingInstruction (this_object(), target, data);}
  //Attr create_attribute (string name, string|void default_value);
  EntityReference create_entity_reference (string name)
    {return EntityReference (this_object(), name);}
#endif

  //NodeList get_elements_by_tag_name (string tagname);

  array(Element) get_elements (string name)
  //! Note that this one looks among the top level elements, as
  //! opposed to @[get_elements_by_tag_name]. This means that if the
  //! document is correct, you can only look up the single top level
  //! element here.
  //!
  //! @note
  //! Not DOM compliant.
  {
    if (name == "*") {
      CHECK_CONTENT;
      if (!content) return ({});
      return filter (content,
		     lambda (string|Node child) {
		       return objectp (child) && child->node_type == ELEMENT_NODE;
		     });
    }
    else {
      CHECK_LOOKUP_MAPPING;
      return _lookup_mapping[id_prefix + name] || ({});
    }
  }

  int get_raw_values() {return raw_values;}
  //! @note
  //! Not DOM compliant.

  static void create (void|string|array(string|Node) c, void|int raw_vals)
  {
    content = c;
    raw_values = raw_vals;
  }

  // Internals.

  static constant class_name = "Document";

  /*protected*/ int raw_values;
  static Element document_element = 0;
  /*protected*/ mapping(string:array(Node)) _lookup_mapping = ([]);

  /*protected*/ Document _get_doc() {return this_object();}

  /*protected*/ void _xml_format (String.Buffer into) {xml_format_children (into);}

  static void destroy()
  {
    if (arrayp (content))
      foreach (content, string|Node child)
	if (objectp (child)) child->_destruct_tree();
  }
}

//class Attr {}

class Element
{
  inherit NodeWithChildElements;

  string node_name;
  mapping(string:string) attributes;

  constant node_type = ELEMENT_NODE;
  int get_node_type() { return ELEMENT_NODE; }
  string get_node_name() { return node_name; }
  string get_tag_name() { return node_name; }

  //NamedNodeMap get_attributes();

  string get_attribute (string name)
    {return attributes[name] || "";}
  void set_attribute (string name, string value)
    {attributes[name] = value;}
  void remove_attribute (string name)
    {m_delete (attributes, name);}

  //Attr get_attribute_node (string name);
  //Attr set_attribute_node (Attr new_attr);
  //Attr remove_attribute_node (Attr old_attr);

  //void normalize();

  static void create (Document owner, string name, void|mapping(string:string) attr)
  {
    owner_document = owner;
    node_name = name;
    attributes = attr || ([]);
  }

  // Internals.

  static constant class_name = "Element";

  /*protected*/ void _xml_format (String.Buffer into)
  {
    into->add ("<", node_name);
    format_attrs (attributes, into);
    if (content && sizeof (content)) {
      into->add (">");
      xml_format_children (into);
      into->add ("</", node_name, ">");
    }
    else
      into->add (" />");
  }

  static string sprintf_name() {return node_name;}

  static string sprintf_attr()
  {
    if (sizeof (attributes))
      return "(" + map ((array) attributes,
			lambda (array pair) {
			  return sprintf ("%s=%O", pair[0], pair[1]);
			}) * ", " + ")";
    else
      return "";
  }
}

class CharacterData
{
  inherit Node;

  string node_value;

  string get_node_value() { return node_value; }
  void set_node_value(string data) {node_value = data;}
  string get_data() {return node_value;}
  void set_data (string data) {node_value = data;}
  int get_length() {return sizeof (node_value);}

  string substring_data (int offset, int count)
    {return node_value[offset..offset + count - 1];}
  void append_data (string arg)
    {node_value += arg;}
  void insert_data (int offset, string arg)
    {node_value = node_value[..offset - 1] + arg + node_value[offset..];}
  void delete_data (int offset, int count)
    {node_value = node_value[..offset - 1] + node_value[offset + count..];}
  void replace_data (int offset, int count, string arg)
    {node_value = node_value[..offset - 1] + arg + node_value[offset + count..];}

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path (string path, void|int xml_format)
  {
    [string node_type, string arg] = parse_trivial_node_type_path (path);
    if (node_type == "text") return node_value;
    return xml_format && "";
  }

  // Internals.

  static constant class_name = "CharacterData";

  /*protected*/ void _text_content (String.Buffer into)
  {
    if (owner_document->raw_values) {
      // Serial replace's are currently faster than one parallell.
      into->add (replace (replace (replace (node_value,
					    "&", "&amp;"),
				   "<", "&lt;"),
			  ">", "&gt;"));
    }
    else
      into->add (node_value);
  }

  static string sprintf_content (int flag) {return sprintf ("%O", node_value);}
}

class Text
{
  inherit CharacterData;

  constant node_type = TEXT_NODE;
  int get_node_type() { return TEXT_NODE; }
  string get_node_name() { return "#text"; }

  //Text split_text (int offset);

  static void create (Document owner, string data)
  {
    owner_document = owner;
    node_value = data;
  }

  // Internals.

  static constant class_name = "Text";

  /*protected*/ void _xml_format (String.Buffer into)
  {
    // Serial replace's are currently faster than one parallell.
    into->add (replace (replace (replace (node_value,
					  "&", "&amp;"),
				 "<", "&lt;"),
			">", "&gt;"));
  }
}

class Comment
{
  inherit CharacterData;

  constant node_type = COMMENT_NODE;
  int get_node_type() { return COMMENT_NODE; }
  string get_node_name() { return "#comment"; }

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path (string path, void|int xml_format)
  {
    [string node_type, string arg] = parse_trivial_node_type_path (path);
    if (node_type == "comment") return node_value;
    return xml_format && "";
  }

  static void create (Document owner, string data)
  {
    owner_document = owner;
    node_value = data;
  }

  // Internals.

  static constant class_name = "Comment";

  /*protected*/ void _xml_format (String.Buffer into)
  {
    into->add ("<!--", node_value, "-->");
  }
}

class CDATASection
{
  inherit Text;

  constant node_type = CDATA_SECTION_NODE;
  int get_node_type() { return CDATA_SECTION_NODE; }
  string get_node_name() { return "#cdata-section"; }

  static void create (Document owner, string data)
  {
    owner_document = owner;
    node_value = data;
  }

  // Internals.

  static constant class_name = "CDATASection";

  /*protected*/ void _text_content (String.Buffer into)
  {
    if (owner_document->raw_values)
      into->add ("<![CDATA[", node_value, "]]>");
    else
      into->add (node_value);
  }

  /*protected*/ void _xml_format (String.Buffer into)
  {
    into->add ("<![CDATA[", node_value, "]]>");
  }
}

//class DocumentType {}
//class Notation {}
//class Entity {}

class EntityReference
{
  inherit Node;

  string node_name;

  constant node_type = ENTITY_REFERENCE_NODE;
  int get_node_type() { return ENTITY_REFERENCE_NODE; }
  string get_node_name() { return node_name; }

  //NodeList get_child_nodes();
  //Node get_first_child();
  //Node get_last_child();
  //Node get_previous_sibling();
  //Node get_next_sibling();

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path (string path, void|int xml_format)
  {
    [string node_type, string arg] = parse_trivial_node_type_path (path);
    if (node_type == "text") return "&" + node_name + ";";
    return xml_format && "";
  }

  static void create (Document owner, string name)
  {
    owner_document = owner;
    node_name = name;
  }

  // Internals.

  static constant class_name = "EntityReference";

  /*protected*/ void _text_content (String.Buffer into)
  {
    if (owner_document->raw_values)
      into->add ("&", node_name, ";");
    else
      if (string decoded = Parser.html_entities[node_name] ||
	  Parser.decode_numeric_xml_entity (node_name))
	into->add (decoded);
      else
	error ("Cannot decode entity reference %O.\n", node_name);
  }

  static string sprintf_name() {return node_name;}

  /*protected*/ void _xml_format (String.Buffer into)
  {
    into->add ("&", node_name, ";");
  }
}

class ProcessingInstruction
{
  inherit Node;

  string node_name, node_value;

  constant node_type = PROCESSING_INSTRUCTION_NODE;
  int get_node_type() { return PROCESSING_INSTRUCTION_NODE; }
  string get_node_name() { return node_name; }
  string get_target() { return node_name; }
  string get_node_value() { return node_value; }

  void set_node_value (string data) {node_value = data;}
  string get_data() {return node_value;}
  void set_data (string data) {node_value = data;}

  mapping(string:string)|Node|array(mapping(string:string)|Node)|string
    simple_path (string path, void|int xml_format)
  {
    [string node_type, string arg] = parse_trivial_node_type_path (path);
    if (node_type == "processing-instruction")
      if (!arg) return node_value;
      else if (arg == node_name) return node_value;
    return xml_format && "";
  }

  static void create (Document owner, string t, string data)
  {
    owner_document = owner;
    node_name = t;
    node_value = data;
  }

  // Internals.

  static constant class_name = "ProcessingInstruction";

  /*protected*/ void _text_content (String.Buffer into)
  {
    if (owner_document->raw_values) {
      // Serial replace's are currently faster than one parallell.
      into->add (replace (replace (replace (node_value,
					    "&", "&amp;"),
				   "<", "&lt;"),
			  ">", "&gt;"));
    }
    else
      into->add (node_value);
  }

  /*protected*/ void _xml_format (String.Buffer into)
  {
    if (sizeof (node_value))
      into->add ("<?", node_name, " ", node_value, "?>");
    else
      into->add ("<?", node_name, "?>");
  }

  static string sprintf_name() {return node_name;}
  static string sprintf_content (int flag) {return sprintf ("%O", node_value);}
}

// Internals.

static int(0..0) return_zero() {return 0;}

static array sloppy_parser_container_callback (
  Parser.HTML p, mapping(string:string) args, string content, Node cur)
{
  if (Parser.HTML ent_p = p->entity_parser)
    foreach (indices (args), string arg)
      args[arg] = ent_p->finish (args[arg])->read();
  Element element = Element (cur->_get_doc(), p->tag_name(), args);
  element->parent_node = cur;
  element->content = content;
  return ({element});
}

static array|int sloppy_parser_tag_callback (Parser.HTML p, string text, Node cur)
{
  if (text[-2] != '/') {
    sscanf (text, "<%[^ \t\n\r>]", text);
    p->add_container (text, sloppy_parser_container_callback);
    return 1;
  }
  mapping(string:string) args = p->tag_args();
  if (Parser.HTML ent_p = p->entity_parser)
    foreach (indices (args), string arg)
      args[arg] = ent_p->finish (args[arg])->read();
  Element element = Element (cur->_get_doc(), p->tag_name(), args);
  element->parent_node = cur;
  element->content = "";
  return ({element});
}

static array sloppy_parser_entity_callback (Parser.HTML p, string text, Node cur)
{
  text = p->tag_name();
  if (string chr = Parser.decode_numeric_xml_entity (text))
    return ({chr});
  EntityReference ent = EntityReference (cur->_get_doc(), text);
  ent->parent_node = cur;
  return ({ent});
}

static class SloppyParser
{
  inherit Parser.HTML;
  Parser.HTML entity_parser;
}

static SloppyParser sloppy_parser_template =
  lambda() {
    SloppyParser p = SloppyParser();
    p->lazy_entity_end (1);
    p->match_tag (0);
    p->xml_tag_syntax (3);
    p->mixed_mode (1);
    p->add_quote_tag ("!--", return_zero, "--");
    p->add_quote_tag ("![CDATA[", return_zero, "]]");
    p->add_quote_tag ("?", return_zero, "?");
    p->_set_tag_callback (sloppy_parser_tag_callback);
    p->_set_entity_callback ((mixed) return_zero); // Cast due to type inference bug.
    return p;
  }();

static array(string|Node) sloppy_parse_fragment (string frag, Node cur)
{
  Parser.HTML p = sloppy_parser_template->clone();
  if (!cur->_get_doc()->raw_values)
    p->entity_parser = Parser.html_entity_parser();
  p->set_extra (cur);
  array(string|Node) res = p->finish (frag)->read();
  for (int i = sizeof (res) - 1; i >= 0; i--)
    if (objectp (res[i])) res[i]->pos_in_parent = i;
  return res;
}
