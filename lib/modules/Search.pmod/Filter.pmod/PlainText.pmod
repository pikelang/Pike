// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: PlainText.pmod,v 1.5 2001/06/22 01:28:35 nilsson Exp $

// Filter for text/plain

inherit Search.Filter.Base;

constant contenttypes = ({ "text/plain" });

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
