// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: PlainText.pmod,v 1.7 2001/08/07 14:45:42 js Exp $

// Filter for text/plain

inherit Search.Filter.Base;

constant contenttypes = ({ "text/plain" });
constant fields = ({ "body",
		     //FIXME:
		     "author", "external_use"});

Output filter(Standards.URI uri, string|Stdio.File data, string content_type)
{
  Output res=Output();

  if(objectp(data))
    data=data->read();
  
  res->fields->body=data;

  return res;  
}

string _sprintf()
{
  return "Search.Filter.PlainText";
}
