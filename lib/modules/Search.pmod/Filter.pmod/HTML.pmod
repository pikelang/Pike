// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: HTML.pmod,v 1.37 2004/08/07 15:26:59 js Exp $

// Filter for text/html

#define INTRAWISE

inherit .Base;

constant contenttypes = ({ "text/html" });
constant fields = ({ "body", "title", "keywords", "description", "robots",
		     "headline", "modified", "author", "summary",
#ifdef INTRAWISE
		     "intrawise.folderid", "intrawise.type",
#endif
		  });

static int(0..0) return_zero(mixed ... args) { return 0; }

static Parser.HTML parser;
static Parser.HTML cleaner;
static mapping entities;

static void create() {
  parser = Parser.HTML();
  parser->case_insensitive_tag(1);
  parser->lazy_entity_end(1);
  parser->ignore_unknown(1);
  parser->match_tag(0);

  parser->add_quote_tag("!--", return_zero, "--");
  parser->add_quote_tag("?", return_zero, "?");

  parser->_set_tag_callback(lambda(Parser.HTML p, string data) {
			      //  Do nothing! Callback still needed so that
			      //  unknown tags aren't sent to
			      //  _set_data_callback.
			    });

  constant ignore_tags = ({ "script", "style", });
  parser->add_containers(mkmapping(ignore_tags, ({""})*sizeof(ignore_tags)));

#if __VERSION__ > 7.4
  cleaner = Parser.html_entity_parser(1);
#else
  cleaner = Parser.html_entity_parser();
  cleaner->_set_entity_callback(
	   lambda(Parser.HTML p,string ent)
	   {
	     string chr = Parser.decode_numeric_xml_entity(p->tag_name());
	     if (!chr)
	       return 0;
	     return ({chr});
	   });
#endif
  cleaner->case_insensitive_tag(1);
  cleaner->lazy_entity_end(1);
  cleaner->ignore_unknown(1);
  cleaner->match_tag(0);

  cleaner->add_quote_tag("!--", return_zero, "--");
  cleaner->add_quote_tag("?", return_zero, "?");
  cleaner->_set_tag_callback(lambda(Parser.HTML p, string data) {
			      return ({ "" });
			    });

  entities = ([]);
  foreach(Parser.html_entities; string i; string v)
    entities["&"+i+";"] = v;
}

static string clean(string data) {
  return cleaner->finish(data)->read();
}

.Output filter(Standards.URI uri, string|Stdio.File data,
	       string content_type,
	       mapping headers,
	       string|void default_charset )
{
  function(string...:void) dadd;
  .Output res=.Output();


  if(objectp(data))
    data=data->read();

  data = .Charset.decode_http( data, headers, default_charset );

#if 0
  array parse_rank(Parser.HTML p, mapping m, string c)
  {
    if(!m->name)
      return ({});
    
    if(res->fields[m->name])
      res->fields[m->name] += " " + clean(c);
    else
      res->fields[m->name] = clean(c);
    return ({});
  };
#endif
  
  array parse_meta(Parser.HTML p, mapping m )
  {
    string n = m->name||m["http-equiv"];
    switch(lower_case(n))
    {
      case "description": 
      case "keywords":
      case "modified":
      case "author":
#ifdef INTRAWISE
      case "intrawise.folderid":
      case "intrawise.type":
#endif
	res->fields[lower_case(n)] = m->contents||m->content||m->data||"";
	break;
      case "robots":
	res->fields->robots = (stringp(res->fields->robots)?
			       res->fields->robots+",": "") +
			      (m->contents||m->content||m->data||"");
	break;
    }
    return ({});
  };

  _WhiteFish.LinkFarm lf = _WhiteFish.LinkFarm();
  function ladd = lf->add;

  array(string) parse_title(Parser.HTML p, mapping m, string c) {
    res->fields->title = clean(c);
    return ({c});
  };

  // FIXME: Push the a contents to the description field of the
  // document referenced to by this tag.
  array parse_a(Parser.HTML p, mapping m)  {
    // FIXME: We should try to decode the source with the
    // charset indicated in m->charset.
    // FIXME: We should set the document language to the
    // language indicated in m->hreflang.
    if(m->href) ladd( m->href );

    // FIXME: Push the value of m->title to the title field of
    // the referenced document.
    //    if(m->title)
    //      dadd(" ", m->title, " ");
    return ({});
  };

  // FIXME: The longdesc information should be pushed to the
  // description field of the frame src URL when it is indexed.
  array parse_frame(Parser.HTML p, mapping m)  {
    if(m->src) ladd( m->src );
    return ({});
  };

  // FIXME: This information should be pushed to the body field
  // of the image file, if it is indexed.
  array parse_img(Parser.HTML p, mapping m)  {
    if( m->alt && sizeof(m->alt) )
      dadd(" ", clean(m->alt));
    if( m->title && sizeof(m->title) )
      dadd(" ", clean(m->title));
    return ({});
  };

  array parse_applet(Parser.HTML p, mapping m) {
    // FIXME: The alt information should be pushed to the body field
    // of all the resources linked from this tag.
    if( m->src ) ladd( m->src );
    if( m->archive )
      ladd( m->archive); // URL to a GNU-ZIP file with classes needed
			 // by the applet.
    if( m->code ) ladd( m->code ); // URL to the applets code/class.
    if( m->codebase ) ladd( m->codebase );
    if( m->alt && sizeof(m->alt) ) dadd(" ", clean(m->alt));
    return ({});
  };

  // <area>, <bgsound>
  array parse_src_alt(Parser.HTML p, mapping m) {
    // FIXME: The alt information should be pushed to the body field
    // of all the resources linked from this tag.
    if( m->src ) ladd( m->src );
    if( m->alt && sizeof(m->alt) ) dadd(" ", clean(m->alt));
    return ({});
  };

  array parse_background(Parser.HTML p, mapping m) {
    if( m->background ) ladd( m->background );
    return ({});
  };

  array parse_embed(Parser.HTML p, mapping m) {
    if( m->pluginspage )
      ladd( m->pluginspage ); // Where the required plugin can be downloaded.
    if( m->pluginurl )
      ladd( m->pluginurl ); // Similar to pluginspage, but for java archives.
    if( m->src ) ladd( m->src );
    return ({});
  };

  array parse_layer(Parser.HTML p, mapping m) {
    if( m->background ) ladd( m->background );
    if( m->src ) ladd( m->src );
    return ({});
  };

  array parse_object(Parser.HTML p, mapping m) {
    if( m->archive ) ladd( m->archive );
    if( m->classid ) ladd( m->classid );
    if( m->code ) ladd( m->code );
    if( m->codebase ) ladd( m->codebase );
    if( m->data ) ladd( m->data );
    if( m->standby && sizeof(m->standby) )
      dadd(" ", clean(m->standby) );
    return ({});
  };

  array parse_base(Parser.HTML p, mapping m)
  {
    if(m->href)
      catch(uri = Standards.URI(m->href));
    return ({});
  };

  array parse_q(Parser.HTML p, mapping m) {
    if( m->cite ) ladd( m->cite );
    return ({});
  };

  array parse_xml(Parser.HTML p, mapping m) {
    if( m->ns ) ladd( m->ns );
    if( m->src ) ladd( m->src );
    return ({});
  };

  array parse_headline(Parser.HTML p, mapping m, string c)
  {
    if(!res->fields->headline)
      res->fields->headline = "";
    res->fields->headline += " " + clean(c);
    return ({});
  };

  array parse_noindex(Parser.HTML p, mapping m, string c)
  {
    if(m->nofollow) return ({});

    Parser.HTML parser = p->clone();
    parser->_set_data_callback(return_zero);
    parser->_set_entity_callback(return_zero);
    parser->add_tags( ([ "title" : return_zero,
			 "h1" : return_zero,
			 "h2" : return_zero,
			 "h3" : return_zero,
    ]) );
    function odadd = dadd;
    dadd = return_zero;
    parser->finish(c);
    dadd = odadd;
    return ({});
  };

  String.Buffer databuf=String.Buffer(sizeof(data));
  Parser.HTML parser = parser->clone();
  parser->add_quote_tag("![CDATA[",
			lambda(Parser.HTML p, string data) {
			  dadd(data);
			}, "]]");

  //  parser->add_container("rank",parse_rank);
  parser->add_containers( ([ "title":parse_title,
			     "h1": parse_headline,
			     "h2": parse_headline,
			     "h3": parse_headline, 
			     "noindex": parse_noindex,
			     "no-index":parse_noindex,
			     "no_index": parse_noindex,  ]) );
			     
  parser->add_tags( ([ "meta":parse_meta,
		       "a":parse_a,
		       "base": parse_base,
		       "link":parse_a,
		       "frame":parse_frame,
		       "iframe":parse_frame,
		       "layer":parse_layer,
		       "ilayer":parse_layer,
		       "img":parse_img,
		       "applet":parse_applet,
		       "area":parse_src_alt,
		       "bgsound":parse_src_alt,
		       "sound":parse_src_alt,
		       "body":parse_background,
		       "table":parse_background,
		       "td":parse_background,
		       "object": parse_object,
		       "q":parse_q,
		       "embed":parse_embed,
		       "xml":parse_xml
  ]) );

  dadd = databuf->add;
  int space;
  parser->_set_data_callback(lambda(Parser.HTML p, string data) {
			       if(space) dadd(" ");
  			       dadd(data);
			       space = 1;
			     });
  parser->_set_entity_callback(lambda(Parser.HTML p, string data) {
				 if(entities[data]) {
				   dadd(entities[data]);
				   space = 0;
				   return;
				 }
				 string c = Parser.
				   decode_numeric_xml_entity(data);
				 if(c) {
				   space = 0;
				   dadd(c);
				 }
			       });
  
  res->fields->title="";
  res->fields->description="";
  res->fields->keywords="";

  parser->finish(data);

  res->links = lf->read();
  res->fields->body=databuf->get();
  res->fix_relative_links(uri);

  return res;
}
