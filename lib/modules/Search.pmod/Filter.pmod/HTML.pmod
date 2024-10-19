#pike __REAL_VERSION__

// Cannot dump this because of the #if constant(Roxen.xxx) check below.
constant dont_dump_module = 1;

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

protected int(0..0) return_zero(mixed ... args) { return 0; }

protected Parser.HTML parser;
protected Parser.HTML cleaner;
protected mapping entities;

protected void create() {
  parser = Parser.HTML();
  parser->case_insensitive_tag(1);
  parser->lazy_entity_end(1);
  parser->ignore_unknown(1);
  parser->match_tag(0);

  parser->add_quote_tag("?", return_zero, "?");

  parser->_set_tag_callback(lambda(Parser.HTML p, string data) {
			      //  Do nothing! Callback still needed so that
			      //  unknown tags aren't sent to
			      //  _set_data_callback.
			    });

  constant ignore_tags = ({ "script", "style", });
  parser->add_containers(mkmapping(ignore_tags, ({""})*sizeof(ignore_tags)));

  cleaner = Parser.html_entity_parser(1);
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

protected string clean(string data) {
  return cleaner->finish(data)->read();
}

void parse_http_header(string header, string value, .Output res)
{
  switch(lower_case(header))
  {
  case "robots":
    res->fields->robots = (stringp(res->fields->robots)?
			   res->fields->robots+",": "") +
      value;
    break;

  case "last-modified":
    catch {
#if constant(Roxen.parse_since)
      // Roxen.parse_since() supports multiple time formats.
      res->fields->mtime = (string)Roxen.parse_since(value)[0];
#elif constant(Protocols.HTTP.Server.http_decode_date)
      // Protocols.HTTP.Server.http_decode_date() currently
      // only supports the format specified by the RFC.
      res->fields->mtime =
	(string)Protocols.HTTP.Server.http_decode_date(value);
#else
      // Fallback for Pike 7.4.
      Calendar.ISO_UTC.Second s=
	Calendar.ISO_UTC.parse("%e, %D %M %Y %h:%m:%s GMT", value);
      res->fields->mtime = (string)(s && s->unix_time());
#endif
    };
    // FALL_THROUGH

  case "mtime":
  case "description":
  case "keywords":
  case "modified":
  case "author":
#ifdef INTRAWISE
  case "intrawise.folderid":
  case "intrawise.type":
#endif
  default:
    res->fields[lower_case(header)] = value;
    break;
  }
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

  foreach(headers; string header; string value)
  {
    parse_http_header(header, value, res);
  }

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

  array parse_meta(Parser.HTML p, mapping m, mapping e)
  {
    if (e->noindex)
      return ({ });
    parse_http_header(m->name||m["http-equiv"]||"",
		      m->contents||m->content||m->data||"", res);
    return ({});
  };

  _WhiteFish.LinkFarm lf = _WhiteFish.LinkFarm();
  function low_ladd = lf->add;

  void ladd(string html_href)
  {
    low_ladd(Parser.parse_html_entities(html_href, 1));
  };

  array(string) parse_title(Parser.HTML p, mapping m, string c, mapping e) {
    if (e->noindex)
      return ({ });
    if (!res->fields->title)
      res->fields->title = clean(c);
    return ({c});
  };

  // FIXME: Push the a contents to the description field of the
  // document referenced to by this tag.
  array parse_a(Parser.HTML p, mapping m, mapping e)  {
    // FIXME: We should try to decode the source with the
    // charset indicated in m->charset.
    // FIXME: We should set the document language to the
    // language indicated in m->hreflang.
    if(m->href && !e->nofollow) ladd( m->href );

    // FIXME: Push the value of m->title to the title field of
    // the referenced document.
    //    if(m->title)
    //      dadd(" ", m->title, " ");
    return ({});
  };

  // FIXME: The longdesc information should be pushed to the
  // description field of the frame src URL when it is indexed.
  array parse_frame(Parser.HTML p, mapping m, mapping e)  {
    if(m->src && !e->nofollow) ladd( m->src );
    return ({});
  };

  // FIXME: This information should be pushed to the body field
  // of the image file, if it is indexed.
  array parse_img(Parser.HTML p, mapping m, mapping e)  {
    if( !e->noindex ) {
      if( m->alt && sizeof(m->alt) )
	dadd(" ", clean(m->alt));
      if( m->title && sizeof(m->title) )
	dadd(" ", clean(m->title));
    }
    return ({});
  };

  array parse_applet(Parser.HTML p, mapping m, mapping e) {
    // FIXME: The alt information should be pushed to the body field
    // of all the resources linked from this tag.
    if( m->src && !e->nofollow ) ladd( m->src );
    if( m->archive && !e->nofollow )
      ladd( m->archive); // URL to a GNU-ZIP file with classes needed
			 // by the applet.
    if( m->code && !e->nofollow ) ladd( m->code ); // URL to the applets code/class.
    if( m->codebase && !e->nofollow ) ladd( m->codebase );
    if( m->alt && sizeof(m->alt) )
      if( !e->noindex )
	dadd(" ", clean(m->alt));
    return ({});
  };

  // <area>, <bgsound>
  array parse_src_alt(Parser.HTML p, mapping m, mapping e) {
    // FIXME: The alt information should be pushed to the body field
    // of all the resources linked from this tag.
    if( m->src && !e->nofollow ) ladd( m->src );
    if( m->alt && sizeof(m->alt) )
      if( !e->noindex )
	dadd(" ", clean(m->alt));
    return ({});
  };

  array parse_background(Parser.HTML p, mapping m, mapping e) {
    if( m->background && !e->nofollow ) ladd( m->background );
    return ({});
  };

  array parse_embed(Parser.HTML p, mapping m, mapping e) {
    if( m->pluginspage && !e->nofollow )
      ladd( m->pluginspage ); // Where the required plugin can be downloaded.
    if( m->pluginurl && !e->nofollow )
      ladd( m->pluginurl ); // Similar to pluginspage, but for java archives.
    if( m->src && !e->nofollow ) ladd( m->src );
    return ({});
  };

  array parse_layer(Parser.HTML p, mapping m, mapping e) {
    if( m->background && !e->nofollow ) ladd( m->background );
    if( m->src && !e->nofollow ) ladd( m->src );
    return ({});
  };

  array parse_object(Parser.HTML p, mapping m, mapping e) {
    if( m->archive && !e->nofollow ) ladd( m->archive );
    if( m->classid && !e->nofollow ) ladd( m->classid );
    if( m->code && !e->nofollow ) ladd( m->code );
    if( m->codebase && !e->nofollow ) ladd( m->codebase );
    if( m->data && !e->nofollow ) ladd( m->data );
    if( m->standby && sizeof(m->standby) )
      if ( !e->noindex )
	dadd(" ", clean(m->standby) );
    return ({});
  };

  array parse_base(Parser.HTML p, mapping m)
  {
    if(m->href)
      catch(uri = Standards.URI(m->href));
    return ({});
  };

  array parse_q(Parser.HTML p, mapping m, mapping e) {
    if( m->cite && !e->nofollow ) ladd( m->cite );
    return ({});
  };

  array parse_xml(Parser.HTML p, mapping m, mapping e) {
    if( m->ns && !e->nofollow ) ladd( m->ns );
    if( m->src && !e->nofollow ) ladd( m->src );
    return ({});
  };

  array parse_headline(Parser.HTML p, mapping m, string c, mapping e)
  {
    if (e->noindex)
      return ({ });
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

  // <!-- robots:noindex -->...<!-- /robots:noindex -->
  mixed parse_comment(Parser.HTML p, string c, mapping extra)
  {
    if(has_value(c, "/robots:noindex"))
      extra->noindex--;
    else if(has_value(c, "robots:noindex"))
      extra->noindex++;

    if(has_value(c, "/robots:nofollow"))
      extra->nofollow--;
    else if(has_value(c, "robots:nofollow"))
      extra->nofollow++;

    return ({""});
  };

  String.Buffer databuf=String.Buffer(sizeof(data));
  Parser.HTML parser = parser->clone();

  mapping extra = ([]);
  parser->set_extra(extra);
  parser->add_quote_tag("!--", parse_comment, "--");
  parser->add_quote_tag("![CDATA[",
			lambda(Parser.HTML p, string data, mapping e) {
			  if(!e->noindex)
			    dadd(data);
			}, "]]");

  //  parser->add_container("rank",parse_rank);
  parser->add_containers( ([ "title":parse_title,
			     "h1": parse_headline,
			     "h2": parse_headline,
			     "h3": parse_headline,
			     "noindex":  parse_noindex,
			     "no-index": parse_noindex,
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
  parser->_set_data_callback(lambda(Parser.HTML p, string data, mapping e) {
			       if (!e->noindex) {
				 if(space) dadd(" ");
				 dadd(data);
				 space = 1;
			       }
			     });
  parser->_set_entity_callback(lambda(Parser.HTML p, string data, mapping e) {
				 if(!e->noindex) {
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
				 }
			       });

  parser->finish(data);

  res->links = lf->read();
  res->fields->body=databuf->get();
  res->fix_relative_links(uri);

  res->fields->title = res->fields->title || "";
  res->fields->description = res->fields->description || "";
  res->fields->keywords = res->fields->keywords || "";

  return res;
}
