// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Base.pike,v 1.9 2001/08/01 15:52:24 js Exp $

//! The MIME content types this class can filter.
constant contenttypes = ({ });

class Output
{
  // Wide strings here

  mapping(string:string) fields=([]);
  // body, title, description, keywords

  int document_size;
  
  mapping(string:string) uri_anchors=([]);
  // Maps un-normalized URLs to raw text
  // ([ "http://www.roxen.com": "the Roxen web-server" ])

  array(Standards.URI|string) links=({});

  void fix_relative_links(Standards.URI base_uri)
  {
    for(int i=0; i<sizeof(links); i++)
    {
      links[i]=Standards.URI(links[i], base_uri);
      if(links[i]->fragment)
	links[i]->fragment=0;
    }
  }
}

Output filter(Standards.URI uri, string|Stdio.File data,
	      string content_type, mixed ... more);

string tmp_filename()
{
  return sprintf("../var/tmp/search.tmp.%d.%d",random(0x7fffffff),getpid());
}
