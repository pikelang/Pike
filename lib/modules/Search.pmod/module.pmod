// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: module.pmod,v 1.17 2003/01/15 13:09:24 anders Exp $

private mapping filter_mimetypes;
private multiset filter_fields;

program search_filter;

private void get_filters()
{
  if (!search_filter)
    search_filter = master()->resolv("Search.Filter");
  filter_mimetypes=([]);
  filter_fields=(<>);
  foreach(values(search_filter), object filter)
  {
    foreach(filter->contenttypes || ({ }), string mime)
      filter_mimetypes[mime]=filter;

    foreach(filter->fields || ({ }), string field)
      filter_fields[field]=1;
  }
}

object get_filter(string mime_type)
{
  if(!filter_mimetypes)
    get_filters();
  if(!filter_mimetypes[mime_type]) return 0;
  return filter_mimetypes[mime_type];
}

mapping(string:object) get_filter_mime_types()
{
  if(!filter_mimetypes)
    get_filters();
  return filter_mimetypes;
}

array(string) get_filter_fields()
{
  if(!filter_fields)
    get_filters();
  return indices(filter_fields);
}
