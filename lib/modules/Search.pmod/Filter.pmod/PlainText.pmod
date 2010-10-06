// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: PlainText.pmod,v 1.9 2004/08/07 15:27:00 js Exp $

// Filter for text/plain

inherit .Base;

constant contenttypes = ({ "text/plain" });
constant fields = ({ "body" });

.Output filter(Standards.URI uri, string|Stdio.File data, string content_type)
{
  .Output res=.Output();

  if(objectp(data))
    data=data->read();
  
  res->fields->body=data;

  return res;  
}
