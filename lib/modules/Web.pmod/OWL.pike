// $Id: OWL.pike,v 1.6 2004/03/09 14:12:30 nilsson Exp $

#pike __REAL_VERSION__
#define Node Parser.XML.NSTree.NSNode

//! Represents an RDF tuple set from an OWL perspective.

inherit .RDFS; // RDFS in turn inherits RDF

constant owl_ns = "http://www.w3.org/2002/07/owl#";

void create()
{
  namespaces[owl_ns] = "owl";
  ::create();
}

class OWLResource
{
  inherit URIResource;

  void create(string id)
  {
    ::create(owl_ns+id);
  }
}

OWLResource owl_Class           = OWLResource("Class");
OWLResource owl_Thing           = OWLResource("Thing");
OWLResource owl_Nothing         = OWLResource("Nothing");
OWLResource owl_oneOf           = OWLResource("oneOf");
OWLResource owl_sameAs          = OWLResource("sameAs");
OWLResource owl_differentFrom   = OWLResource("differentFrom");
OWLResource owl_intersectionOf  = OWLResource("intersectionOf");
OWLResource owl_unionOf         = OWLResource("unionOf");
OWLResource owl_complementOf    = OWLResource("complementOf");
OWLResource owl_equivalentClass = OWLResource("equivalentClass");
OWLResource owl_disjointWith    = OWLResource("disjointWith");
OWLResource owl_Restriction     = OWLResource("Restriction");

this_program parse_owl(string|Node in, void|string base) {
  return parse_xml(in, base);
}

void add_Class(Resource c)
{
  add_statement(c, rdf_type, owl_Class);
}

void add_Thing(Resource c)
{
  add_statement(c, rdf_type, owl_Thing);
}

static Resource _add_list(Resource ... list_members)
{
  Resource list = rdf_nil;

  foreach(reverse(list_members), Resource m)
  {
    Resource new = Resource();
    add_statement(new, rdf_first, m);
    add_statement(new, rdf_rest, list);
    list = new;
  }

  return list;
}

void add_intersectionOf(Resource c, Resource ... members)
{
  add_statement(c, owl_intersectionOf, _add_list(@members));
}

void add_unionOf(Resource c, Resource ... members)
{
  add_statement(c, owl_unionOf, _add_list(@members));
}

void add_complementOf(Resource c, Resource other)
{
  add_statement(c, owl_complementOf, other);
}
