// $Id: RSS.pmod,v 1.1 2003/11/07 15:45:36 nilsson Exp $

#pike __REAL_VERSION__

//! Represents a RSS (RDF Site Summary) file.

static constant ns = "http://purl.org/rss/1.0/";

static class Thing {
  static .RDF rdf;
  static mapping attributes = ([]);
  .RDF.Resource me;
  constant thing = "";

  mixed `[](string i) {
    if(!zero_type(attributes[i])) return attributes[i];
    return ::`[](i);
  }

  void `[]=(string i, string v) {
    if(!zero_type(attributes[i])) {
      attributes[i] = v;
      .RDF.Resource attr = rdf->make_resource(ns+thing);
      foreach(rdf->find_statements(me, attr, 0), array s)
	rdf->remove_statement(@s);
      rdf->add_statement(me, attr, rdf->LiteralResource(v));
    } else
      error("No %O variable.\n", i);
  }

  function `-> = `[];
  function `->= = `[]=;

  void create(.RDF _rdf, string about, mapping _attr) {
    rdf = _rdf;
    me = rdf->make_resource(about);
    foreach(indices(attributes), string i) {
      string v = _attr[i];
      if(!v) continue;

      attributes[i] = v;
      rdf->add_statement(me, rdf->make_resource(ns+thing),
			 rdf->LiteralResource(v));
    }
  }

  .RDF.Resource get_id() { return me; }
}

class Image {
  inherit Thing;
  constant thing = "image";
  static mapping attributes = ([
    "title" : 0,
    "url" : 0,
    "link" : 0
  ]);
}

class Item {
  inherit Thing;
  constant thing = "item";
  static mapping attributes = ([
    "title" : 0,
    "link" : 0,
    "description" : 0
  ]);
}

class Textinput {
  inherit Thing;
  constant thing = "textinput";
  static mapping attributes = ([
    "title" : 0,
    "description" : 0,
    "name" : 0,
    "link" : 0,
  ]);
}

class Channel {
  inherit Thing;
  static constant thing = "channel";
  static mapping attributes = ([
    "title" : 0,
    "link" : 0,
    "description" : 0,
    "image" : 0,
    "items" : 0,
    "textinput" : 0,
  ]);

  void `[]=(string i, mixed v) {
    if( i=="image" || i=="textinput" ) {
      if(objectp(v))
	attributes[i] = v->me->get_uri();
      else if(stringp(v)) {
	attributes[i] = v;
	v = rdf->make_resource(v);
      }
      else
	error("Wrong type. Expected string or Image/Textinput.\n");

      .RDF.Resource attr = rdf->make_resource(ns+i);
      foreach(rdf->find_statements(me, attr, 0), array s)
	rdf->remove_statement(@s);
      rdf->add_statement(me, attr, v);
      return;
    }
    if( i=="textinput" ) {
      if(!arrayp(i)) error("Wrong type. Expected array(Item).\n");
      // FIXME: Stuff here
      return;
    }
    ::`[]=(i, v);
  }

  void add_item(Item i) {
  }

  void remove_item(Item i) {
  }
}

class Index {
}

Index parse_xml(string|Parser.XML.Tree.Node n, void|string base) {
}

