// $Id$
#pike __REAL_VERSION__

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
