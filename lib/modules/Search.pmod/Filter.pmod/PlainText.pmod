// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: PlainText.pmod,v 1.6 2001/06/23 00:21:09 js Exp $

// Filter for text/plain

inherit Search.Filter.Base;

constant contenttypes = ({ "text/plain" });
constant fields = ({ "body"});

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
