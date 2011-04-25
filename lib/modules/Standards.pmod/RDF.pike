// $Id$

#pike __REAL_VERSION__

//! Represents an RDF domain which can contain any number of complete
//! statements.

static int(1..) node_counter = 1;
static mapping(string:Resource) uris = ([]);

//! Instances of this class represents resources as defined in RDF:
//! All things being described by RDF expressions are called resources. A
//! resource may be an entire Web page; such as the HTML document
//! "http://www.w3.org/Overview.html" for example. A resource may be a
//! part of a Web page; e.g. a specific HTML or XML element within the
//! document source. A resource may also be a whole collection of pages;
//! e.g. an entire Web site. A resource may also be an object that is
//! not directly accessible via the Web; e.g. a printed book.
//! This general resource is anonymous and has no URI or literal id.
//!
//! @note
//!   Resources instantiated from this class should not be used in
//!   other RDF domain objects.
//!
//! @seealso
//!   @[URIResource], @[LiteralResource]
class Resource {
  static int(1..) number;

  void create() {
    number = node_counter++;
  }

  //! Returns the nodes' N-triple serialized ID.
  string get_n_triple_name() {
    return "_:Resource"+number;
  }

  static string __sprintf(string c, int t) {
    if(t=='t') return "RDF."+c;
    if(t=='O') return "RDF."+c+"(" + get_n_triple_name() + ")";
  }

  string _sprintf(int t) { return __sprintf("Resource", t); }
}

//! Resource identified by literal.
class LiteralResource {
  inherit Resource;
  constant is_literal_resource = 1;
  static string id;

  //! The resource will be identified by @[literal].
  void create(string literal) {
    id = literal;
    ::create();
  }

  string get_n_triple_name() {
    return "\"" + encode_n_triple_string(id) + "\"";
  }

  string _sprintf(int t) { return __sprintf("LiteralResource", t); }
}

//! Resource identified by URI.
class URIResource {
  inherit Resource;
  constant is_uri_resource = 1;
  static string id;

  //! Creates an URI resource with the @[uri] as identifier.
  //! @throws
  //!   Throws an error if another resource with the
  //!   same URI already exists in the RDF domain.
  void create(string uri) {
    if(uris[uri])
      error("A resource with URI %s already exists in the RDF domain.\n", uri);
    uris[uri] = this_object();
    id = uri;
    ::create();
  }

  string get_n_triple_name() {
    return "<" + id + ">";
  }

  string _sprintf(int t) { return __sprintf("URIResource", t); }
}


//
// General RDF set modification
//

static mapping(Resource:ADT.Relation.Binary) statements = ([]);

//! Adds a statement to the RDF set.
void add_statement(Resource subj, Resource pred, Resource obj) {
  ADT.Relation.Binary rel = statements[pred];
  if(!rel) {
    rel = ADT.Relation.Binary();
    statements[pred] = rel;
  }

  rel->add(subj, obj);
}

//! Returns an RDF resource with the given URI as identifier,
//! or zero.
Resource get_resource(string uri) {
  if(uris[uri]) return uris[uri];
  return 0;
}

//! Returns an array with the statements that matches the given
//! subject @[subj], predicate @[pred] and object @[obj]. Any
//! and all of the resources may be zero to disregard from matching
//! that part of the statement, i.e. find_statements(0,0,0) returns
//! all statements in the domain.
//!
//! @returns
//!   An array with arrays of three elements.
//!   @array
//!     @elem Resource 0
//!       The subject of the statement
//!     @elem Resource 1
//!       The predicate of the statement
//!     @elem Resource 2
//!       The object of the statement
//!   @endarray
array(array(Resource)) find_statements(Resource|int(0..0) subj,
				       Resource|int(0..0) pred,
				       Resource|int(0..0) obj) {

  array(array(Resource)) find_subj_obj(Resource subj, Resource pred,
				       Resource obj, ADT.Relation.Binary rel) {
    if(subj && obj) {
      if(rel(subj,obj)) return ({ ({ subj, pred, obj }) });
      return ({});
    }

    array ret = ({});
    foreach(rel; Resource left; Resource right) {
      if(subj && subj!=left) continue;
      if(obj && obj!=right) continue;
      ret += ({ ({ left, pred, right }) });
    }
    return ret;
  };

  if(pred)
    return find_subj_obj(subj, pred, obj, statements[pred]);
  array ret = ({});
  foreach(statements; Resource pred; ADT.Relation.Binary rel)
    ret += find_subj_obj(subj, pred, obj, rel);
  return ret;
}


//
// N-triple code
//

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
//! to the RDF set. Returns the number of added relations.
//! @throws
//!   The parser will throw errors on invalid N-triple input.
int parse_n_triples(string in) {

  class Temp(string id) {
    constant type = "";
    string _sprintf(int t) { return t=='O' && sprintf("%s(%O)", type, id); }
  };
  class TempURI {
    inherit Temp;
    constant type = "TempURI";
  };
  class TempNode {
    inherit Temp;
    constant type = "TempNode";
  };
  class TempLiteral {
    inherit Temp;
    constant type = "TempLiteral";
  };

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
      string str = decode_n_triple_string( in[start+1..pos-1] );
      tokens += ({ TempLiteral(str) });

      // language (ignored)
      start = pos;
      if( in[pos]=='-' ) {
	while( !(< ' ', '\t', '\r', '\n' >)[in[++pos]] );
      }

      pos++;
      continue;

    case '<':
      // uriref
      while( in[++pos]!='>' );
      tokens += ({ TempURI(decode_n_triple_string( in[start+1..pos-1] )) });
      pos++;
      continue;

    case '_':
      // nodeID
      if(in[pos+1]!=':')
	error("No ':' in nodeID (position %s).\n", pos);
      while( !(< ' ', '\t', '\r', '\n' >)[in[++pos]] );
      tokens += ({ TempNode( in[start+2..pos-1] ) });
      continue;

    case '.':
      tokens += ({ "." });
      pos++;
      break;

    default:
      error("Malformed n-triples input (position %d).\n", pos);
    }
  }

  mapping(string:Resource) anonymous = ([]);

  Resource make_resource(Temp res) {
    switch(res->type) {
    case "TempURI":
      Resource ret;
      if(ret=uris[res->id])
	return ret;
      return URIResource( res->id );

    case "TempLiteral": return LiteralResource( res->id );

    case "TempNode":
      if(ret=anonymous[res->id])
	return ret;
      else
	return anonymous[res->id]=Resource();
    }
    error("Unknown temp container. Strange indeed.\n");
  };

  int adds;
  foreach(tokens/({"."}), array rel) {
    if(!sizeof(rel)) continue;
    if(sizeof(rel)!=3) error("N-triple isn't a triple.\n");
    add_statement( make_resource( rel[0] ),
		   make_resource( rel[1] ),
		   make_resource( rel[2] ) );
    adds++;
  }
  return adds;
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

  in = build+in;
  build = "";
  while( sscanf(in, "%s\\U%8x%s", string a, int b, in)==3 )
    build += a + sprintf("%c", b);

  build = replace(build+in, ([ "\\\\" : "\\",
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


//
// lfuns
//

int _sizeof() {
  if(!sizeof(statements)) return 0;
  return `+( @sizeof(values(statements)[*]) );
}

string _sprintf(int t) {
  return t=='O' && sprintf("%O(%d)", this_program, _sizeof());
}
