// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: HTML.pmod,v 1.18 2001/08/17 19:30:41 per Exp $

// Filter for text/html

inherit Search.Filter.Base;

constant contenttypes = ({ "text/html" });
constant fields = ({ "body", "title", "keywords", "description", "robots" });

string _sprintf()
{
  return "Search.Filter.HTML";
}

Output filter(Standards.URI uri, string|Stdio.File data,
	      string content_type,
	      mapping headers,
	      string|void default_charset )
{
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
  
  string parse_meta(Parser.HTML p, mapping m )
  {
    string n = m["http-equiv"]||m->name;
    switch(lower_case(n))
    {
      case "description": 
	res->fields->description=
	  Parser.parse_html_entities(m->contents||m->content||m->data||"");
	break;
      case "keywords":
	res->fields->keywords=
	  Parser.parse_html_entities(m->contents||m->content||m->data||"");
	break;
      case "robots":
	res->fields->robots = m->contents||m->content||m->data||"";
	break;
    }
    return "";
  };

  if( headers["description"] )
    res->fields->description=Parser.parse_html_entities(headers["description"]);

  if( headers["keywords"] )
    res->fields->keywords=Parser.parse_html_entities(headers["keywords"]);
  
  _WhiteFish.LinkFarm lf = _WhiteFish.LinkFarm();
  function ladd = lf->add;

  string parse_title(Parser.HTML p, mapping m, string c) {
    res->fields->title=Parser.parse_html_entities(c);
    return "";
  };
  string parse_a(Parser.HTML p, mapping m, string c)  {
    if(m->href) ladd( m->href );
    return c;
  };
  string parse_frame(Parser.HTML p, mapping m, string c)  {
    if(m->src) ladd( m->src );
    return c;
  };
  string parse_img( Parser.HTML p, mapping m, string c)  {
    if( m->alt ) return m->alt+c;
    return c;
  };

  String.Buffer databuf=String.Buffer();
  Parser.HTML parser = Parser.HTML();

  parser->case_insensitive_tag(1);

  parser->match_tag(0);
  parser->add_tag("meta",parse_meta );
  //  parser->add_container("rank",parse_rank);
  parser->add_container("title",parse_title);
  parser->add_container("a",parse_a);
  parser->add_container("frame",parse_frame);
  parser->add_container("frameset",parse_frame);
  parser->add_container("img",parse_img);

  constant ignore_tags=({"noindex","script","style","no-index",});
  parser->add_containers(mkmapping(ignore_tags,({""})*sizeof(ignore_tags)));

  function dadd = databuf->add;
  parser->_set_data_callback(lambda(object p, string data) {
			       dadd(data);
			     });

  res->fields->title="";
  res->fields->description="";
  res->fields->keywords="";

  parser->feed(data);
  parser->finish();

  res->links = Parser.parse_html_entities(lf->read()*"\0")/"\0";
  res->fields->body=Parser.parse_html_entities(databuf->get());
  res->fix_relative_links(uri);
  return res;
}
