
static int(1..) node_counter = 1;

//!
class Node {
  static int(1..) number;
  static int(0..1) name_is_uri;
  static string name;

  //! @decl void create()
  //! @decl void create(string id, int(0..1) is_uri)
  void create(void|string id, void|int(0..1) is_uri) {
    if(id) {
      if(is_uri)
	set_uri(id);
      else
	set_string(id);
    }
    number = node_counter++;
  }

  //! Sets the node value to be a string.
  void set_string(string in) {
    name = in;
    name_is_uri = 0;
  }

  //! Sets the node value to be a URI. The
  //! provided URI should already be normalized.
  void set_uri(string in) {
    // FIXME: URI normalization
    name = in;
    name_is_uri = 1;
  }

  //! Returns the node URI value or zero.
  string get_uri() {
    if(name_is_uri) return name;
    return 0;
  }

  //! Returns the node string value or zero.
  string get_string() {
    if(name_is_uri) return 0;
    return name;
  }

  //! Returns the nodes' N-triple serialized ID.
  string get_n_triple_name() {
    if(name) {
      if(name_is_uri)
	return "<" + name + ">";
      return "\"" + name + "\"";
    }
    return "_:Node"+number;
  }

  string _sprintf(int t) {
    if(t=='t') return "RDF.Node";
    if(t=='O') return "RDF.Node(" + get_n_triple_name() + ")";
    error("Can not represent RDF.Node as %c.\n", t);
  }
}

//
//
//

static mapping(Node:ADT.Relation.Binary) relations = ([]);
static mapping(string:Node) uris = ([]);

static void update_uri_table(Node entity) {
  string uri = entity->get_uri();
  if(uri) {
    if( uris[uri] && uris[uri]!=entity )
      error("Node has different Node object but same URI.\n");
    else
      uris[uri] = entity;
  }
}

//! Adds a relation to the RDF set.
void add_relation(Node subj, Node pred, Node obj) {
  ADT.Relation.Binary rel = relations[pred];
  if(!rel) {
    rel = ADT.Relation.Binary(pred);
    relations[pred] = rel;
  }

  update_uri_table(subj);
  update_uri_table(pred);
  update_uri_table(obj);

  rel->add(subj, obj);
}

//! Returns an RDF node with the given URI as identifier,
//! or zero.
Node get_node(string uri) {
  if(uris[uri]) return uris[uri];
  return 0;
}

//! Returns an N-triples serialization of the RDF set.
string get_n_triples() {
  string ret = "";

  foreach(relations; Node n; ADT.Relation.Binary rel) {
    string rel_name = n->get_n_triple_name();
    foreach(rel; Node left; Node right) {
      ret += left->get_n_triple_name() + " " +
	rel_name + " " + right->get_n_triple_name() + " .\n";
    }
  }

  return ret;
}

//! Parses an N-triples string and adds the found relations
//! to the RDF set.
//! @throws
//!   The parser will throw errors on invalid N-triple input.
void parse_n_triples(string in) {

  array(string) tokens = ({});
  int pos;
  while(pos<sizeof(in)) {
    int start = pos;
    switch(in[pos]) {

    case ' ':
    case '\t':
    case '\n':
    case '\r':
      // Well, we are breaking the BNF, but so is w3c in
      // their examples in the RDF primer.
      pos++;
      continue;

    case '#':
      // Comment. Ignore them.
      while( !(< '\r', '\n' >)[in[++pos]] );
      continue;

    case 'x':
      // Assume "xml\"" (xmlString)
      pos += 3;
      // fallthrough
    case '\"':
      // N-triple string
      while( in[++pos]!='\"' )
	if( in[pos]=='\\' ) pos++;
      string str = decode_n_triple_string( in[start..pos-1] );
      start = pos;
      if( in[pos]=='-' )
	while( !(< ' ', '\t', '\r', '\n' >)[in[++pos]] );
      tokens += ({ str+in[start..pos-1] });
      continue;

    case '<':
      // URI
      while( in[++pos]!='>' );
      tokens += ({ decode_n_triple_string( in[start..pos-1] ) });
      continue;

    case '.':
      pos++;
      break;
    }

    tokens += ({ in[start..pos-1] });
  }

  int last;
  while( (last=search(tokens, ".", last))!=-1 ) {

  }
}

//! Decodes a string that has been encoded for N-triples
//! serialization.
//! @bugs
//!   Doesn't correctly decode backslashes that has been
//!   encoded with with \u- or \U-notation.
string decode_n_triple_string(string in) {

  string build = "";
  while( sscanf(in, "%s\\u%4x%s", string a, int b, in)==3 )
    build += a + sprintf("%c", b);

  in = build;
  build = "";
  while( sscanf(in, "%s\\U%8x%s", string a, int b, in)==3 )
    build += a + sprintf("%c", b);

  build = replace(build, ([ "\\\\" : "\\",
			    "\\\"" : "\"",
			    "\\n" : "\n",
			    "\\r" : "\r",
			    "\\t" : "\t" ]) );
  return build;
}

//! Encodes a string for use as tring in N-triples serialization.
string encode_n_triple_string(string in) {

  string build = "";
  foreach(in/1, string c) {
    switch(c[0]) {
    case 0..8:
    case 0xB..0xC:
    case 0xE..0x1F:
    case 0x7F..0xFFFF:
      build += sprintf("\\u%4x", c[0]);
      continue;
    case 0x10000..0x10FFFF:
      build += sprintf("\\U%8x", c[0]);
      continue;

    case 9:
      build += "\\t";
      continue;
    case 10:
      build += "\\n";
      continue;
    case 13:
      build += "\\r";
      continue;
    case 34:
      build += "\\\"";
      continue;
    case 92:
      build += "\\\\";
      continue;

    default:
      build += c;
    }
  }

  return build;
}
