// $Id: RDF.pike,v 1.42 2004/04/05 22:52:02 nilsson Exp $

#pike __REAL_VERSION__

//! Represents an RDF domain which can contain any number of complete
//! statements.

constant rdf_ns = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";

// These should be in some generic namespace module. Perhaps.
constant default_ns = ([
  "http://www.w3.org/2002/07/owl#" : "owl",
  "http://www.w3.org/2000/01/rdf-schema#" : "rdfs",
  "http://www.w3.org/2001/XMLSchema#" : "xsd",
]);

static int(1..) node_counter = 1;
static mapping(string:Resource) uris = ([]);

// Returns ({ namespace, object })
static array(string) uri_parts(string uri) {
  string obj,ns;
  if(sscanf(uri, "%s#%s", ns,obj)==2)
    return ({ ns+"#", obj });
  if(sscanf(reverse(uri), "%s/%s", obj,ns)==2) {
    if(reverse(ns)=="http:/")
      return ({ 0, "http://"+reverse(obj) });
    return ({ reverse(ns)+"/", reverse(obj) });
  }
  return ({ 0,0 });
}

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

  static void create() {
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

  static int __hash() { return number; }
}

//! Resource identified by literal.
class LiteralResource {
  inherit Resource;
  constant is_literal_resource = 1;
  static string id;

  //! Used to contain rdf:datatype value.
  string datatype;

  //! The resource will be identified by @[literal].
  static void create(string literal) {
    id = literal;
    ::create();
  }

  string get_n_triple_name() {
    string ret = "\"" + encode_n_triple_string(id) + "\"";
    if(datatype) ret += "^^<" + datatype + ">";
    return ret;
  }

  string get_3_tuple_name() {
    return get_n_triple_name();
  }

  //! Returns the literal as an XML string.
  string get_xml() {
    return id; // FIXME: XML quote.
  }

  //! Returns the literal string.
  string get_literal() {
    return id;
  }

  int(0..1) _equal(mixed r) {
    if(!objectp(r) || !r->is_literal_resource) return 0;
    return r->id==id;
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
  static void create(string uri) {
    if(uris[uri])
      error("A resource with URI %s already exists in the RDF domain.\n", uri);
    uris[uri] = this;
    id = uri;
    ::create();
  }

  //! Returns the URI the resource references to.
  string get_uri() {
    return id;
  }

  //! Returns the qualifying name, or zero.
  string get_qname(void|string ns) {
    fix_namespaces();
    if(!ns) ns=common_ns;
    string nns,obj;
    [ nns, obj ] = uri_parts(id);
    if(!obj) error("Could not produce qname.\n");
    if(!nns || nns==ns) return obj;
    return namespaces[nns]+":"+obj;
  }

  //! Returns the namespace this resource URI references to.
  string get_namespace() {
    return uri_parts(id)[0];
  }

  string get_n_triple_name() {
    return "<" + id + ">";
  }

  string get_3_tuple_name() {
    return "[" + id + "]";
  }

  int(0..1) _equal(mixed r) {
    if(!objectp(r) || !r->is_uri_resource) return 0;
    return r->id==id;
  }

  string _sprintf(int t) { return __sprintf("URIResource", t); }
}

//! Resource used for RDF-technical reasons like reification.
class RDFResource {
  inherit URIResource;

  //! The resource will be identified by the identifier @[rdf_id]
  static void create(string rdf_id) {
    ::create(rdf_ns + rdf_id);
  }

  //! Returns the qualifying name.
  string get_qname(void|string ns) {
    if(!ns) ns = common_ns;
    string rid;
    sscanf(id, rdf_ns+"%s", rid);
    if(ns==rdf_ns) return rid;
    return "rdf:"+rid;
  }
}

RDFResource rdf_Statement = RDFResource("Statement"); //! Statement resource.
RDFResource rdf_predicate = RDFResource("predicate"); //! predicate resource.
RDFResource rdf_subject   = RDFResource("subject"); //! subject resource.
RDFResource rdf_object    = RDFResource("object"); //! object resource.
RDFResource rdf_type      = RDFResource("type"); //! type resource.

RDFResource rdf_Seq       = RDFResource("Seq"); //! Seq resource.

RDFResource rdf_first     = RDFResource("first"); //! first resource.
RDFResource rdf_rest      = RDFResource("rest"); //! rest resource.
RDFResource rdf_nil       = RDFResource("nil"); //! nil resource.

static int(0..1) is_resource(mixed res) {
  if(!objectp(res)) return 0;
  return res->is_resource;
}


//
// General RDF set modification
//

// predicate : Relation( subject, object )
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

//! Returns a mapping with all the domains subject resources as
//! indices and a mapping with that subjects predicates and objects
//! as value.
mapping(Resource:mapping(Resource:array(Resource))) get_subject_map() {
  mapping subs = ([]);
  foreach(statements; Resource pred; ADT.Relation.Binary rel)
    foreach(rel; Resource subj; Resource obj)
      if(subs[subj]) {
	if(subs[subj][pred])
	  subs[subj][pred] += ({ obj });
	else
	  subs[subj][pred] = ({ obj });
      }
      else
	subs[subj] = ([ pred:({obj}) ]);
  return subs;
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

//! Returns @expr{1@} if resource @[r] is used as a subject, otherwise
//! @expr{0@}.
int(0..1) is_subject(Resource r) {
  return !!sizeof(find_statements(r,0,0));
}

//! Returns @expr{1@} if resource @[r] is used as a predicate,
//! otherwise @expr{0@}.
int(0..1) is_predicate(Resource r) {
  return !!sizeof(find_statements(0,r,0));
}

//! Returns @expr{1@} if resource @[r] is used as an object, otherwise
//! @expr{0@}.
int(0..1) is_object(Resource r) {
  return !!sizeof(find_statements(0,0,r));
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
    foreach(rel; Resource left; Resource right)
      ret->add( left->get_n_triple_name(), " ", rel_name,
		" ", right->get_n_triple_name(), " .\n" );
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
    string datatype;
  };

  array(string|Temp) tokens = ({});
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
      if( in[pos+1]=='^' ) {
	pos += 3; // "^^
	start = pos+1;
	while(in[++pos]!='>');
	tokens[-1]->datatype = in[start..pos-1];
      }

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

    case "TempLiteral":
      ret = LiteralResource( res->id );
      ret->datatype = res->datatype;
      return ret;

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

static int dirty_namespaces = 1;
static mapping(string:string) namespaces = ([]); // url-prefix:name
static string common_ns = ""; // The most common namespace

// W3C must die!
static Node add_xml_children(Node p, string rdfns, string base) {
  mapping rdf_m = p->get_ns_attributes(rdf_ns);
  base = p->get_ns_attributes("xml")->base || base;
  if(rdf_m->about && rdf_m->ID)
    error("Both rdf:about and rdf:ID defined on the same element.\n");

  string subj_uri = rdf_m->about;
  Resource subj;
  if(rdf_m->about)
    subj = make_resource(rdf_m->about);
  else if(rdf_m->ID) {
    subj = make_resource(rdf_m->ID);
    add_statement(subj, rdf_type,
		  make_resource(p->get_ns()+p->get_any_name()) );
  } else
    subj = Resource();

  if(rdfns && p->get_ns()!=rdfns) {
    add_statement( subj, rdf_type,
		   make_resource(p->get_ns()+p->get_any_name()) );
  }
  else
    rdfns = p->get_ns();

  if(rdfns==rdf_ns) {
    string name = p->get_any_name();
    if( name == "Seq" )
      add_statement(subj, rdf_type, rdf_Seq);
  }

  // Handle attribute abbreviation (2.2.2. Basic Abbreviated Syntax)
  mapping m = p->get_ns_attributes();
  foreach(m; string ns; mapping m) {
    if(ns==rdf_ns) continue;
    foreach(m; string pred; string obj)
      add_statement( subj, make_resource(ns+pred), LiteralResource(obj) );
  }

  int li_counter; // FIXME: Restart for every collection?

  // Handle subnodes
  foreach(p->get_elements(), Node c) {
    if(c->get_ns()==rdf_ns) {
      string name = c->get_any_name();
      if(name=="type" || name=="first") {
	string obj_uri = c->get_ns_attributes(rdfns)->resource;
	if(!obj_uri) error("rdf:%s missing resource attribute.\n", name);
	add_statement( subj, this["rdf_"+name], make_resource(obj_uri) );
	continue;
      }
      if(name=="rest") {
	string obj_uri = c->get_ns_attributes(rdfns)->resource;
	if(obj_uri) add_statement( subj, rdf_rest, make_resource(obj_uri) );
	array(Node) dcs = c->get_elements();
	foreach(dcs, Node dc)
	  add_statement( subj, rdf_rest, add_xml_children(dc, rdfns, base) );
	continue;
      }
      else if(sscanf(name, "_%*d")) {
	string obj_uri = c->get_ns_attributes(rdfns)->resource;
	if(!obj_uri) error("rdf:_n missing resource attribute.\n");
	add_statement( subj, make_resource(rdf_ns+name),
		       make_resource(obj_uri) );
	continue;
      }
      else if(name=="li") {
	string obj_uri = c->get_ns_attributes(rdfns)->resource;
	if(!obj_uri) error("rdf:li missing resource attribute.\n");
	add_statement( subj, make_resource(rdf_ns+"_"+(++li_counter)),
		       make_resource(obj_uri) );
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
      string ptype = c->get_ns_attributes(rdfns)->parseType;
      if( !(< "Literal", "Resource", "Collection", 0 >)[ptype] )
	error("Illegal parserType value %O.\n", ptype);

      if(!ptype || ptype=="Resource") {
	array(Node) dcs = c->get_elements();
	if(sizeof(dcs)) {
	  foreach(dcs, Node dc)
	    add_statement( subj, make_resource(pred_uri),
			   add_xml_children(dc, rdfns, base) );
	  continue;
	}
      }
      else if(ptype=="Collection") {
	// FIXME: Empty lists?
	array(Node) dcs = c->get_elements();
	Resource n = Resource();
	add_statement( subj, make_resource(pred_uri), n );
	foreach(dcs; int pos; Node dc) {
	  add_statement( n, rdf_first, add_xml_children(dc, rdfns, base) );
	  if(pos<sizeof(dcs)-1)
	    add_statement( n, rdf_rest, n=Resource() );
	}
	add_statement( n, rdf_rest, rdf_nil );
	continue;
      }

      // ptype == "Literal"

      obj = LiteralResource((array(string))c->get_children()*"");
      obj->datatype = c->get_ns_attributes(rdfns)->datatype;
    }
    add_statement( subj, make_resource(pred_uri), obj );
  }

  return subj;
}

//! @decl Web.RDF parse_xml(string|Parser.XML.NSTree.NSNode in, @
//!                         void|string base)
//! Adds the statements represented by the string or tree @[in] to the
//! RDF domain. If @[in] is a tree the in-node should be the @tt{RDF@}
//! node of the XML serialization. RDF documents take its default
//! namespace from the URI of the document, so if the RDF document relies
//! such ingenious mechanisms, pass the document URI in the @[base]
//! variable.
this_program parse_xml(string|Node in, void|string base) {
  Node n;
  if(stringp(in)) {
    n = Parser.XML.NSTree.parse_input(in);
    n = n->get_first_element("RDF");
  }
  else
    n = in;

  // Determine the document base.
  base = n->get_ns_attributes("xml")->base || base || "";

  // FIXME: Namespaces defined under the rdf-element will not be used
  // in serialization.
  mapping nss = n->get_defined_nss();
  foreach(values(nss), string name)
    if( !namespaces[name] ) {
      namespaces = mkmapping(values(nss),indices(nss))|namespaces;
      dirty_namespaces = 1;
      break;
    }

  string rdfns = n->get_ns();

  foreach(n->get_elements(), Node c)
    add_xml_children(c, rdfns, base);

  return this;
}

static void fix_namespaces() {
  if(!dirty_namespaces) return;
  mapping(string:string) new = ([]);
  mapping(string:int) stat = ([]);
  int i=1;
  foreach(indices(uris), string uri) {
    string obj;
    [ uri, obj ] = uri_parts(uri);
    if(!uri) continue;
    if( new[uri])
      ;
    else if( namespaces[uri] )
      new[uri] = namespaces[uri];
    else {
      string ns;
      do {
	ns = default_ns[uri] || Locale.Language.nld.number(i++);
      }
      while( has_value(new, ns) || has_value(namespaces, ns) );
      new[uri] = ns;
    }
    stat[uri]++;
  }
  namespaces = new;

  array nss = indices(stat);
  array occur = values(stat);
  sort(occur,nss);
  common_ns = nss[-1];

  dirty_namespaces = 0;
}

static class XML {

  String.Buffer buf = String.Buffer();
  mapping subjects = get_subject_map();
  mapping ns = ([]);
  int ind;

  void add_ns(Resource r) {
    string s=r->get_namespace();
    ns[s] = namespaces[s];
  }

  void low_add_Description( mapping(Resource:array(Resource)) rel ) {
    ind++;
    array group = ({});
    foreach(rel; Resource left; array(Resource) rights) {
      foreach(rights, Resource right) {
	if(right->is_literal_resource) {
	  if(ind) buf->add("  "*ind);
	  buf->add("<", left->get_qname());
	  if(right->datatype)
	    buf->add(" rdf:datatype='", right->datatype, "'");
	  buf->add(">", right->get_xml(), "</", left->get_qname(), ">\n");
	}
	else if(right->is_uri_resource) {
	  if(ind) buf->add("  "*ind);
	  buf->add("<", left->get_qname());
	  buf->add(" rdf:resource='", right->get_uri(), "'/>\n");
	}
	else
	  group += ({ right });
      }
      if(sizeof(group)) {
	if(ind) buf->add("  "*ind);
	buf->add("<", left->get_qname(), ">\n");
	ind++;
	foreach(group, Resource right)
	  add_Description(right, m_delete(subjects, right)||([]));
	group = ({});
	ind--;
	if(ind) buf->add("  "*ind);
	buf->add("</", left->get_qname(), ">\n");
      }
      add_ns(left); // We must add_ns after get_qname to fix_namespaces.
    }
    ind--;
  }

  string make_prop_attr(mapping(Resource:array(Resource)) rel,
			int nl, int i2) {
    foreach(rel; Resource left; array(Resource) rights) {
      if(!left->is_uri_resource) continue;
      foreach(rights; int p; Resource right) {
	if(!right->is_literal_resource) continue;
	if(right->datatype) continue;
	if(has_value(right->get_xml(), "\n")) continue;
	if(nl++)
	  buf->add("\n", "  "*ind, " "*i2);
	buf->add(left->get_qname(), "='", right->get_xml(), "'");
	add_ns(left);
	rights[p]=0;
      }
      rights -= ({ 0 });
      if(!sizeof(rights))
	m_delete(rel, left);
      else
	rel[left] = rights;
    }
  }

  void add_Description(Resource n,
		       mapping(Resource:array(Resource)) rel) {
    if(n->is_literal_resource)
      error("Can not serialize literal resource as subject.\n");

    // Can we make a <foo></foo> instead of
    // <Description><rdf:type rdf:resource="foo"/></Description>
    if(rel[rdf_type]) {
      Resource c = rel[rdf_type][0];
      if(sizeof(rel[rdf_type])>1)
	rel[rdf_type] = rel[rdf_type][1..];
      else
	m_delete(rel, rdf_type);
      if(ind) buf->add("  "*ind);
      buf->add("<", c->get_qname());
      add_ns(c);
      if(n->is_uri_resource) {
	buf->add(" rdf:about='", n->get_uri(), "'");
	make_prop_attr(rel, 1, sizeof(c->get_qname())+2);
      }
      else {
	buf->add(" ");
	make_prop_attr(rel, 0, sizeof(c->get_qname())+2);
      }
      buf->add(">\n");
      low_add_Description(rel);
      if(ind) buf->add("  "*ind);
      buf->add("</", c->get_qname(), ">\n");
    }
    else {
      if(ind) buf->add("  "*ind);
      if(n->is_uri_resource) {
	buf->add("<rdf:Description rdf:about='", n->get_uri(), "'");
	make_prop_attr(rel, 1, 17);
      }
      else {
	buf->add("<rdf:Description ");
	make_prop_attr(rel, 0, 17);
      }

      if(!sizeof(rel)) {
	buf->add("/>\n");
	if(!ind) buf->add("\n");
	return;
      }

      buf->add(">\n");

      low_add_Description(rel);
      if(ind) buf->add("  "*ind);
      buf->add("</rdf:Description>\n");
    }
    if(!ind) buf->add("\n");
  }

  string render() {

    fix_namespaces();

    // First all root resources.
    foreach(subjects; Resource n;
	    mapping(Resource:array(Resource)) rel) {
      if(is_object(n) || is_predicate(n)) continue;
      m_delete(subjects, n);
      add_Description(n, rel);
    }

    // Then all named resources.
    foreach(subjects; Resource n;
	    mapping(Resource:array(Resource)) rel) {
      if(!n->is_uri_resource) continue;
      m_delete(subjects, n);
      add_Description(n, rel);
    }

    // Cyclic/unreferenced unnamed resources and literals.
    if(sizeof(subjects))
      error("Unserialized resources left.\n");

    String.Buffer ret = String.Buffer();
    ret->add("<?xml version='1.0'?>\n"
	     "<rdf:RDF\nxmlns='" + common_ns +"'\n"
	     "xmlns:rdf='" +rdf_ns + "'\n");
    foreach(ns; string url; string name) {
      if(url==common_ns) continue;
      ret->add("xmlns:", name, "='", url, "'\n");
    }
    ret->add(">\n");
    ret->add( (string)buf );
    ret->add("</rdf:RDF>\n");

    return (string)ret;
  }
}

//! Serialize the RDF domain as an XML string.
string get_xml() {
  return XML()->render();
}


//
// lfuns
//

//! Returns the number of statements in the RDF domain.
static int _sizeof() {
  if(!sizeof(statements)) return 0;
  return `+( @sizeof(values(statements)[*]) );
}

static string _sprintf(int t) {
  return t=='O' && sprintf("%O(%d)", this_program, _sizeof());
}

//! @decl Web.RDF `|(Web.RDF x)
//! Modifies the current object to create a union of the current object
//! and the object @[x].
static this_program `|(mixed data) {
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
    if(!functionp(r->get_uri)) return r;
    return make_resource(r->get_uri());
  };

  foreach(data->find_statements(),
	  [Resource subj, Resource pred, Resource obj])
    add_statement(normalize(subj), normalize(pred), normalize(obj));
  return this;
}
