// $Id: RDF.pike,v 1.5 2002/09/14 13:11:05 nilsson Exp $

//! Represents an RDF domain which can contain any number of complete statements.

static int(1..) node_counter = 1;
static mapping(string:Resource) uris = ([]);

//! Instances of this class represents resources as defined in RDF:
//! All things being described by RDF expressions are called resources. A
//! resource may be an entire Web page; such as the HTML document
//! "http://www.w3.org/Overview.html" for example. A resource may be a
//! part of a Web page; e.g. a specific HTML or XML element within the
//! document source. A resource may also be a whole collection of pages;
//! e.g. an entire Web site. A resource may also be an object that is
//! not directly accessible via the Web; e.g. a printed book. Resources
//! are always named by URIs plus optional anchor ids.
//! Anything can have a URI; the extensibility of URIs allows the
//! introduction of identifiers for any entity imaginable.
//!
//! @note
//!   Resources instantiated from this class should not be used in
//!   other RDF domain objects.
class Resource {
  static int(1..) number;
  static int(0..1) name_is_uri;
  static string name;

  //! @decl void create()
  //! @decl void create(string id, int(0..1) is_uri)
  //! A resource can be created both as an anonymous resource or with
  //! an id. The resource id can be either a literal or a URI, which
  //! then is signified by setting the second argument, @[is_uri], to
  //! true.
  //! @throws
  //!   Throws an error if another resource with the
  //!   same URI already exists in the RDF domain.
  void create(void|string id, void|int(0..1) is_uri) {
    if(id) {
      if(is_uri)
	set_uri(id);
      else
	set_literal(id);
    }
    number = node_counter++;
  }

  //! Sets the node value to be a literal.
  void set_literal(string in) {
    name = in;
    name_is_uri = 0;
  }

  //! Sets the node value to be a URI. The
  //! provided URI should already be normalized.
  //! @throws
  //!   Throws an error if another resource with the
  //!   same URI already exists in the RDF domain.
  void set_uri(string in) {
    if(uris[in])
      error("A resource with URI %s already exists in the RDF domain.\n", in);
    uris[in] = this_object();
    name = in;
    name_is_uri = 1;
  }

  //! Returns the node URI value or zero.
  string get_uri() {
    if(name_is_uri) return name;
    return 0;
  }

  //! Returns the node literal value or zero.
  string get_literal() {
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
    return "_:Resource"+number;
  }

  string _sprintf(int t) {
    if(t=='t') return "RDF.Resource";
    if(t=='O') return "RDF.Resource(" + get_n_triple_name() + ")";
    error("Can not represent RDF.Resource as %c.\n", t);
  }
}

//
//
//

static mapping(Resource:ADT.Relation.Binary) statements = ([]);

//! Adds a statement to the RDF set.
void add_statement(Resource subj, Resource pred, Resource obj) {
  ADT.Relation.Binary rel = statements[pred];
  if(!rel) {
    rel = ADT.Relation.Binary(pred);
    statements[pred] = rel;
  }

  rel->add(subj, obj);
}

//! Returns an RDF node with the given URI as identifier,
//! or zero.
Resource get_node(string uri) {
  if(uris[uri]) return uris[uri];
  return 0;
}

//! Returns an N-triples serialization of all the statements in
//! the RDF set.
string get_n_triples() {
  string ret = "";

  foreach(statements; Resource n; ADT.Relation.Binary rel) {
    string rel_name = n->get_n_triple_name();
    foreach(rel; Resource left; Resource right) {
      ret += left->get_n_triple_name() + " " +
	rel_name + " " + right->get_n_triple_name() + " .\n";
    }
  }

  return ret;
}

//! Parses an N-triples string and adds the found statements
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

string _sprintf(int t) {
  if(t=='t') return "RDF";
  if(t=='O') return "RDF(" + sizeof(statements) + ")";
  error("Can not represent RDF as %c.\n", t);
}
