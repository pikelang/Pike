// $Id: RDF.pike,v 1.14 2003/08/11 20:36:44 nilsson Exp $

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
  constant is_resource = 1;

  void create() {
    number = node_counter++;
  }

  //! Returns the nodes' N-triple serialized ID.
  string get_n_triple_name() {
    return "_:Resource"+number;
  }

  //! Returns the nodes' 3-tuple serialized ID.
  string get_3_tuple_name() {
    return "RDF:_"+number;
  }

  static string __sprintf(string c, int t) {
    if(t=='t') return "RDF."+c;
    if(t=='O') return "RDF."+c+"(" + get_n_triple_name() + ")";
  }

  string _sprintf(int t) { return __sprintf("Resource", t); }
}

//! Resource used for RDF-technical reasons like reification.
class RDFResource {
  inherit Resource;
  static string id;

  //! The resource will be identified by the identifier @[rdf_id]
  void create(string rdf_id) {
    id = rdf_id;
  }

  string get_n_triple_name() {
    return "_:"+id;
  }

  string get_3_tuple_name() {
    return "RDF:"+id;
  }
}

RDFResource rdf_Statement = RDFResource("Statement");
RDFResource rdf_predicate = RDFResource("predicate");
RDFResource rdf_subject   = RDFResource("subject");
RDFResource rdf_object    = RDFResource("object");
RDFResource rdf_type      = RDFResource("type");


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

  string get_3_tuple_name() {
    return get_n_triple_name();
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

  //! Returns the URI the resource references to.
  string get_uri() {
    return id;
  }

  string get_n_triple_name() {
    return "<" + id + ">";
  }

  string get_3_tuple_name() {
    return "[" + id + "]";
  }

  string _sprintf(int t) { return __sprintf("URIResource", t); }
}

static int(0..1) is_resource(mixed res) {
  if(!objectp(res)) return 0;
  return res->is_resource;
}


//
// General RDF set modification
//

static mapping(Resource:ADT.Relation.Binary) statements = ([]);

//! Adds a statement to the RDF set.
//! @throws
//!   Throws an exception if any argument isn't a Resouce object.
void add_statement(Resource subj, Resource pred, Resource obj) {
  if(!is_resource(subj) || !is_resource(pred) || !is_resource(obj))
    error("Non-resource argument to add_statement");
  ADT.Relation.Binary rel = statements[pred];
  if(!rel) {
    rel = ADT.Relation.Binary();
    statements[pred] = rel;
  }

  rel->add(subj, obj);
}

//! Returns 1 if the RDF domain contains the relation {subj, pred, obj},
//! otherwise 0.
int(0..1) has_statement(Resource subj, Resource pred, Resource obj) {
  ADT.Relation.Binary rel = statements[pred];
  if(!rel) return 0;
  return rel->contains(subj,obj);
}

//! Removes the relation from the RDF set. Returns 1 if the relation
//! did exist in the RDF set.
int(0..1) remove_statement(Resource subj, Resource pred, Resource obj) {
  if(!has_statement(subj, pred, obj)) return 0;
  ADT.Relation.Binary rel = statements[pred];
  rel->contains(subj,obj);
  rel->remove(subj,obj);
  if(!sizeof(statements[pred])) m_delete(statements, pred);
  return 1;
}

//! Reifies the statement @tt{{ pred, subj, obj }@} and returns
//! the resource that denotes the reified statement. There will
//! not be any check to see if the unreified statement is already
//! in the domain, making it possible to define the relation twice.
//! The original statement will not be removed.
//! @returns
//!   The subject of the reified statement.
Resource reify_low(Resource subj, Resource pred, Resource obj) {
  Resource r = Resource();
  add_statement(r, rdf_predicate, pred);
  add_statement(r, rdf_subject, subj);
  add_statement(r, rdf_object, obj);
  add_statement(r, rdf_type, rdf_Statement);
  return r;
}

//! Returns a resource that is the subject of the reified statement
//! {subj, pred, obj}, if such a resource exists in the RDF domain.
Resource get_reify(Resource subj, Resource pred, Resource obj) {
  array rs = find_statements(0, rdf_predicate, pred)[*][0] &
    find_statements(0, rdf_subject, subj)[*][0] &
    find_statements(0, rdf_object, obj)[*][0];
  // Any one of rs is a reified statment of ({ subj, pred, obj }).
  if(sizeof(rs)) return rs[0];
  return 0;
}

//! Returns the result of @[get_reify], if any. Otherwise calls
//! @[reify_low] followed by @[remove_statement] of the provided
//! statement {subj, pred, obj}.
//! @returns
//!   The subject of the reified statement.
Resource reify(Resource subj, Resource pred, Resource obj) {
  Resource r = get_reify(subj, pred, obj);
  if(r) return r;
  r = reify_low(subj, pred, obj);
  remove_statement(subj, pred, obj);
  return r;
}

//! Turns the reified statement @[r] into a normal statement, if possible.
//! @returns
//!   1 for success, 0 for failure.
int(0..1) dereify(Resource r) {
  if(sizeof(find_statements(0,0,r))) return 0;
  if(sizeof(find_statements(0,r,0))) return 0;
  array statements = find_statements(r,0,0);
  mapping statement = mkmapping( column(statements,1), column(statements,2) );
  if(sizeof(statement)!=4) return 0;
  add_statement( statement[rdf_subject], statement[rdf_predicate],
		 statement[rdf_object] );
  foreach( statements, array statement )
    remove_statement(@statement);
  return 1;
}

//! Dereifies as many statements as possible. Returns the number of
//! dereified statements.
int(0..) dereify_all() {
  int total, dereified=1;
  while(dereified) {
    dereified=0;
    array rs = find_statements(0, rdf_type, rdf_Statement)[*][0];
    foreach(rs, Resource r)
      dereified = dereify(r);
    total += dereified;
  }
  return total;
}

//! Returns all properties in the domain, e.g. all resources that
//! has been used as predicates.
array(Resource) get_properties() {
  return indices(statements);
}

//! Returns an RDF resource with the given URI as identifier,
//! or zero.
URIResource get_resource(string uri) {
  if(uris[uri]) return uris[uri];
  return 0;
}

//! Returns an RDF resource with the given URI as identifier,
//! or if no such resrouce exists, creates it and returns it.
URIResource make_resource(string uri) {
  return get_resource(uri) || URIResource(uri);
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
  array ret = ({});

  // Optimize the case when all search predicates are 0.
  if(!subj && !pred && !obj) {
    foreach(statements; Resource pred; ADT.Relation.Binary rel)
      foreach(rel; Resource subj; Resource obj)
	ret += ({ ({ subj, pred, obj }) });
    return ret;
  }

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

  if(statements[pred])
    return find_subj_obj(subj, pred, obj, statements[pred]);
  if(pred) return ({});

  foreach(statements; Resource pred; ADT.Relation.Binary rel)
    ret += find_subj_obj(subj, pred, obj, rel);
  return ret;
}


//
// 3-tuple code
//

//! Returns a 3-tuple serialization of all the statements in
//! the RDF set.
string get_3_tuples() {
  String.Buffer ret = String.Buffer();

  foreach(statements; Resource n; ADT.Relation.Binary rel) {
    string rel_name = n->get_3_tuple_name();
    foreach(rel; Resource left; Resource right) {
      ret->add( "{", left->get_3_tuple_name(), ", ", rel_name,
		", ", right->get_3_tuple_name(), "}\n" );
    }
  }

  return (string)ret;
}


//
// N-triple code
//

//! Returns an N-triples serialization of all the statements in
//! the RDF set.
string get_n_triples() {
  String.Buffer ret = String.Buffer();

  foreach(statements; Resource n; ADT.Relation.Binary rel) {
    string rel_name = n->get_n_triple_name();
    foreach(rel; Resource left; Resource right) {
      ret->add( left->get_n_triple_name(), " ", rel_name,
		" ", right->get_n_triple_name(), " .\n" );
    }
  }

  return (string)ret;
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
// XML code
//

#define Node Parser.XML.NSTree.NSNode

static Node add_xml_children(Node p, string rdfns) {
  string subj_uri = p->get_attributes()->about;
  Resource subj;
  if(!subj_uri) {
    subj = Resource();
  }
  else
    subj = make_resource(subj_uri);

  if(p->get_ns()!=rdfns) {
    add_statement( subj, rdf_type,
		   make_resource(p->get_ns()+p->get_any_name()) );
  }

  // Handle attribute abbreviation (2.2.2. Basic Abbreviated Syntax)
  mapping m = p->get_ns_attributes();
  foreach(m; string ns; mapping m) {
    if(ns==rdfns) continue;
    foreach(m; string pred; string obj)
      add_statement( subj, make_resource(ns+pred), LiteralResource(obj) );
  }

  // Handle subnodes
  foreach(p->get_elements(), Node c) {
    if(c->get_ns()==rdfns) {
      if(c->get_any_name()=="type") {
	string obj_uri = c->get_attributes()->resource;
	if(!obj_uri) error("rdf:type missing resource attribute.\n");
	add_statement( subj, rdf_type, make_resource(obj_uri) );
	continue;
      }
      else {
	// We are required to ignore unknown rdf elements.
	error("Can not handle rdf:%s\n", c->get_any_name());
      }
    }

    string pred_uri = c->get_ns() + c->get_any_name();
    Resource obj;
    string obj_uri = c->get_ns_attributes()[rdfns] &&
      c->get_ns_attributes()[rdfns]->resource;
    if(obj_uri) {
      obj = make_resource(obj_uri);
      mapping m = c->get_ns_attributes();
      foreach(m; string ns; mapping m) {
	if(ns==rdfns) continue;
	foreach(m; string pred; string subobj)
	  add_statement( obj, make_resource(ns+pred),
			 LiteralResource(subobj) );
      }
    }
    else {
      string ptype = c->get_attributes()->parserType;
      if( !(< "Literal", "Resource", 0 >)[ptype] )
	error("Illegal parserType value %O.\n", ptype);

      if(ptype!="Literal") {
	array(Node) dcs = c->get_elements();
	if(sizeof(dcs)) {
	  foreach(dcs, Node dc)
	    add_statement( subj, make_resource(pred_uri),
			   add_xml_children(dc, rdfns) );
	  continue;
	}
      }

      obj = LiteralResource((array(string))c->get_children()*"");
    }
    add_statement( subj, make_resource(pred_uri), obj );
  }

  return subj;
}

//! @decl Standards.RDF parse_xml(string|Parser.XML.NSTree.NSNode in)
//! Adds the statements represented by the string or tree @[in] to the
//! RDF domain. If @[in] is a tree the in-node should be the @tt{RDF@}
//! node of the XML serialization. RDF documents takes it default
//! namespace from the URI of the document, so if the RDF document relies
//! such ingenious mechanisms, pass the document URI in the @[base]
//! variable.
this_program parse_xml(string|Node in, void|string base) {
  Node n;
  if(base && base[-1]!='/') base += "#";
  if(stringp(in)) {
    n = Parser.XML.NSTree.parse_input(in, base);
    n = n->get_first_element("RDF");
  }
  else
    n = in;

  string rdfns = n->get_ns();

  foreach(n->get_elements(), Node c)
    add_xml_children(c, rdfns);

  return this_object();
}

//
// lfuns
//

//! Returns the number of statements in the RDF domain.
int _sizeof() {
  if(!sizeof(statements)) return 0;
  return `+( @sizeof(values(statements)[*]) );
}

//!
string _sprintf(int t) {
  return t=='O' && sprintf("%O(%d)", this_program, _sizeof());
}

//! @decl Standards.RDF `|(Standards.RDF x)
//! Modifies the current object to create a union of the current object
//! and the object @[x].
this_program `|(mixed data) {
  if(sprintf("%t", data)!="object" ||
     !functionp(data->find_statements))
    error("Can only or an RDF object with another RDF object.\n");

  Resource normalize(Resource r) {
    if(r->is_literal_resource) return r;
    if(r==data->rdf_Statement) return rdf_Statement;
    if(r==data->rdf_predicate) return rdf_predicate;
    if(r==data->rdf_subject) return rdf_subject;
    if(r==data->rdf_object) return rdf_object;
    if(r==data->rdf_type) return rdf_type;
    if(!functionp(r->get_uri)) error("Unknown resource found\n");
    return make_resource(r->get_uri());
  };

  foreach(data->find_statements(),
	  [Resource subj, Resource pred, Resource obj])
    add_statement(normalize(subj), normalize(pred), normalize(obj));
  return this;
}
