// Filter for text/plain
// Copyright © 2000, Roxen IS.
import "../../";

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
