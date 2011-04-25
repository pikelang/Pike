// $Id$

#pike __REAL_VERSION__

inherit .RDF;

constant rdfs_ns = "http://www.w3.org/2000/01/rdf-schema#";

void create() {
  namespaces[rdfs_ns] = "rdfs";
}

class RDFSResource {
  inherit URIResource;

  void create(string id) {
    ::create(rdfs_ns+id);
  }
}

RDFSResource rdfs_Class         = RDFSResource("Class");
RDFSResource rdfs_subClassOf    = RDFSResource("subClassOf");
RDFSResource rdfs_Literal       = RDFSResource("Literal");
RDFSResource rdfs_subPropertyOf = RDFSResource("subPropertyOf");
RDFSResource rdfs_domain        = RDFSResource("domain");
RDFSResource rdfs_range         = RDFSResource("range");


void add_Class(Resource c) {
  add_statement(c, rdf_type, rdfs_Class);
}

void add_subClassOf(Resource a, Resource b) {
  add_statement(a, rdfs_subClassOf, b);
}

void add_subPropertyOf(Resource a, Resource b) {
  add_statement(a, rdfs_subPropertyOf, b);
}
