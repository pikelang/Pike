// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Base.pike,v 1.10 2001/08/08 15:08:09 noring Exp $

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

private string tmp_unique = sprintf("%d.%d", time(), getpid());
private int tmp_sequence = 0;
string tmp_filename()
{
  return sprintf("../var/tmp/search.tmp.%s.%d", tmp_unique, tmp_sequence++);
}

string my_popen(array(string) args)
  // A smarter version of Process.popen: No need to quote arguments.
{    
  Stdio.File pipe0 = Stdio.File();
  Stdio.File pipe1 = pipe0->pipe(Stdio.PROP_IPC);
  if(!pipe1)
    if(!pipe1) error("my_popen failed (couldn't create pipe).\n");
  Process.create_process(args, ([ "env":getenv(), "stdout":pipe1 ]));
  pipe1->close();
  string result = pipe0->read();
  if(!result)
    error("my_popen failed with error "+pipe0->errno()+".\n");
  pipe0->close();
  return result;
}
