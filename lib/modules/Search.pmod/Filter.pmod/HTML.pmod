// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: HTML.pmod,v 1.36 2004/06/08 12:28:50 anders Exp $

// Filter for text/html

inherit Search.Filter.Base;

constant contenttypes = ({ "text/html" });
constant fields = ({ "body", "title", "keywords", "description", "robots", "headline",
		     "intrawise.folderid", "modified", "author", "intrawise.type",
		     "summary"});

string _sprintf()
{
  return "Search.Filter.HTML";
}

Output filter(Standards.URI uri, string|Stdio.File data,
	      string content_type,
	      mapping headers,
	      string|void default_charset )
{
  function(string:void) dadd;
  Output res=Output();


  if(objectp(data))
    data=data->read();

  data = .Charset.decode_http( data, headers, default_charset );

  /*
  int(0..0) parse_rank(Parser.HTML p, mapping m, string c)
  {
    if(!m->name)
      return 0;
    
    if(res->fields[m->name])
      res->fields[m->name] += " "+c;
    else
      res->fields[m->name] = c;
    return 0;
  };
  */
  
  array(string) parse_meta(Parser.HTML p, mapping m )
  {
    string n = m->name||m["http-equiv"];
    switch(lower_case(n))
    {
      case "description": 
      case "keywords":
      case "intrawise.folderid":
      case "modified":
      case "author":
      case "intrawise.type":
	res->fields[lower_case(n)] =
	  Parser.parse_html_entities(m->contents||m->content||m->data||"");
	break;
      case "robots":
	res->fields->robots = (stringp(res->fields->robots)? res->fields->robots+",": "")+
			      (m->contents||m->content||m->data||"");
	break;
    }
    return ({ "" });
  };

  _WhiteFish.LinkFarm lf = _WhiteFish.LinkFarm();
  function low_ladd = lf->add;

  void ladd(string html_href)
  {
    low_ladd(Parser.parse_html_entities(html_href));
  };

  array(string) parse_title(Parser.HTML p, mapping m, string c) {
    res->fields->title=Parser.parse_html_entities(c);
    return ({ "" });
  };

  // FIXME: Push the a contents to the description field of the
  // document referenced to by this tag.
  array(string) parse_a(Parser.HTML p, mapping m)  {
    // FIXME: We should try to decode the source with the
    // charset indicated in m->charset.
    // FIXME: We should set the document language to the
    // language indicated in m->hreflang.
    if(m->href) ladd( m->href );
    // FIXME: Push the value of m->title to the title field of
    // the referenced document instead.
    if(m->title)
      dadd(" " + m->title + " ");
    return ({ "" });
  };

  array(string) parse_a_noindex(Parser.HTML p, mapping m) {
    //  Special version of parse_a which only cares about href
    if(m->href) ladd( m->href );
    return ({ "" });
  };

  // FIXME: The longdesc information should be pushed to the
  // description field of the frame src URL when it is indexed.
  array(string) parse_frame(Parser.HTML p, mapping m)  {
    if(m->src) ladd( m->src );
    if(m->longdesc) ladd( m->longdesc );
    return ({ "" });
  };

  // FIXME: This information should be pushed to the body field
  // of the image file, if it is indexed.
  array(string) parse_img(Parser.HTML p, mapping m)  {
//      if( m->alt )
//        dadd(" " + m->alt + " ");
    return ({ " " });
  };

  array(string) parse_applet(Parser.HTML p, mapping m) {
    // FIXME: The alt information should be pushed to the body field
    // of all the resources linked from this tag.
    if( m->src ) ladd( m->src );
    if( m->archive ) ladd( m->archive); // URL to a GNU-ZIP file with classes needed by the applet.
    if( m->code ) ladd( m->code ); // URL to the applets code/class.
    if( m->codebase ) ladd( m->codebase );
    if( m->alt ) return ({ m->alt });
    return ({ " " });
  };

  // <area>, <bgsound>
  array(string) parse_src_alt(Parser.HTML p, mapping m) {
    // FIXME: The alt information should be pushed to the body field
    // of all the resources linked from this tag.
    if( m->src ) ladd( m->src );
    if( m->alt ) return ({ m->alt });
    return ({ "" });
  };

  array(string) parse_background(Parser.HTML p, mapping m) {
    if( m->background ) ladd( m->background );
    return ({ " " });
  };

  array(string) parse_embed(Parser.HTML p, mapping m) {
    if( m->pluginspage ) ladd( m->pluginspage ); // Where the required plugin can be downloaded.
    if( m->pluginurl ) ladd( m->pluginurl ); // Similar to pluginspage, but for java archives.
    if( m->src ) ladd( m->src );
    return ({ " " });
  };

  array(string) parse_layer(Parser.HTML p, mapping m) {
    if( m->background ) ladd( m->background );
    if( m->src ) ladd( m->src );
    return ({ " " });
  };

  array(string) parse_object(Parser.HTML p, mapping m) {
    if( m->archive ) ladd( m->archive );
    if( m->classid ) ladd( m->classid );
    if( m->code ) ladd( m->code );
    if( m->codebase ) ladd( m->codebase );
    if( m->data ) ladd( m->data );
    if( m->standby )
      dadd(" " + m->standby + " ");
    return ({ " " });
  };

  array(string) parse_base(Parser.HTML p, mapping m)
  {
    if(m->href)
      catch(uri = Standards.URI(m->href));
    return ({ "" });
  };

  array(string) parse_q(Parser.HTML p, mapping m) {
    if( m->cite ) ladd( m->cite );
    return ({ "" });
  };

  array(string) parse_xml(Parser.HTML p, mapping m) {
    if( m->ns ) ladd( m->ns );
    if( m->src ) ladd( m->src );
    return ({ " " });
  };

  array(string) parse_headline(Parser.HTML p, mapping m, string c)
  {
    if(!res->fields->headline)
      res->fields->headline = "";
    res->fields->headline += " " + c;
    return ({ " " });
  };

  array(string) parse_noindex(Parser.HTML p, mapping m, string c)
  {
    if(m->nofollow)
      return ({ "" });
    Parser.HTML parser = Parser.HTML();
    parser->add_tags( (["a":parse_a_noindex,
			"base": parse_base,
			"link":parse_a,
			"frame":parse_frame,
			"iframe":parse_frame,
			"layer":parse_layer,
			"ilayer":parse_layer,
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
			"xml":parse_xml, ]) );
    parser->feed(c);
    parser->finish();
    return ({""});
  };

  String.Buffer databuf=String.Buffer();
  Parser.HTML parser = Parser.HTML();

  parser->case_insensitive_tag(1);
  parser->lazy_entity_end(1);
  parser->ignore_unknown(1);
  parser->match_tag(0);
  
  int(0..0) return_zero() {return 0;};
  parser->add_quote_tag("!--", return_zero, "--");
  parser->add_quote_tag("![CDATA[", return_zero, "]]");
  parser->add_quote_tag("?", return_zero, "?");
  
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

  constant ignore_tags = ({ "script", "style", });
  parser->add_containers(mkmapping(ignore_tags, ({""})*sizeof(ignore_tags)));

  dadd = databuf->add;
  parser->_set_data_callback(lambda(Parser.HTML p, string data) {
			       dadd(" " + data + " ");
			     });
  parser->_set_tag_callback(lambda(Parser.HTML p, string data) {
			      //  Do nothing! Callback still needed so that
			      //  unknown tags aren't sent to
			      //  _set_data_callback.
			    });
  
  res->fields->title="";
  res->fields->description="";
  res->fields->keywords="";

  parser->feed(data);
  parser->finish();

  res->links = lf->read();
  res->fields->body=Parser.parse_html_entities(databuf->get(), 1);
  res->fix_relative_links(uri);

  return res;
}
