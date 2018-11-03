#pike __REAL_VERSION__

inherit Parser._parser.XML;

//! XML parsing made easy.
//!
//! @returns
//!   A hierarchical structure of nested mappings and arrays representing the
//!   XML structure starting at @expr{rootnode@} using a minimal depth.
//!
//! @mapping
//!   @member string ""
//!     The text content of the node.
//!   @member mapping "/"
//!     The arguments on this node.
//!   @member string "..."
//!     The text content of a simple subnode.
//!   @member array "..."
//!     A list of subnodes.
//!   @member mapping "..."
//!     A complex subnode (recurse).
//! @endmapping
//!
//! @example
//!   @code
//!   Parser.XML.node_to_struct(Parser.XML.NSTree.parse_input("<foo>bar</foo>"));
//!   @endcode
mapping(string:string|array|mapping)
 node_to_struct(.NSTree.NSNode|.Tree.Node rootnode) {
  return low_node_to_struct(rootnode)[1];
}

private array low_node_to_struct(.NSTree.NSNode|.Tree.Node n) {
  array|string|mapping ret = ([]);
  {
    mapping m = n->get_attributes();
    {
      mapping n = n->get_short_attributes();
      if (n)
        if (m)
          m += n;
        else
          m = n;
    }
    if (m && sizeof(m)) {
      string ns, value;
      foreach (({"type", "base", "element"}); ; string atn) {
        if (m[atn] && 2 == sscanf(m[atn], "%s:%s", ns, value))
          m[atn] = ({n->get_defined_nss()[ns], value});
      }
      ret["/"] = m;
    }
  }
  string text = n->get_text();
  if (sizeof(text))
    if (stringp(ret))
      ret += text;
    else if (sizeof(ret))
      ret[""] = text;
    else
      ret = text;
  else
    foreach (n->get_children();; object child) {
      mixed r = low_node_to_struct(child);
      text = r[0];
      mixed v = stringp(ret)||arrayp(ret) ? ret : ret[text];
      r = r[1];
      if (!sizeof(text))
        if (stringp(ret))
          ret += r;
        else if (arrayp(ret))
          ret += ({r});
        else if (mappingp(ret) && ret["/"])
          ret[""] = r;
        else if (sizeof(ret))
          ret = ({ret, r});
        else
          ret = r;
      else if (arrayp(ret))
        ret += ({ ([text:r]) });
      else if (stringp(ret))
        ret = ({ ret, ([text:r]) });
      else if (arrayp(v))
        ret[text] += ({r});
      else
        ret[text] = zero_type(v) ? r : ({v, r});
    }
  return ({n->get_full_name(), sizeof(ret) ? ret : ""});
}

