// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: module.pmod,v 1.16 2001/06/23 00:21:09 js Exp $

private mapping filter_mimetypes;
private multiset filter_fields;

private void get_filters()
{
  filter_mimetypes=([]);
  filter_fields=(<>);
  foreach(values(Search.Filter), object filter)
  {
    foreach(filter->contenttypes || ({ }), string mime)
      filter_mimetypes[mime]=filter;

    foreach(filter->fields || ({ }), string field)
      filter_fields[field]=1;
  }
}

Search.Filter.Base get_filter(string mime_type)
{
  if(!filter_mimetypes)
    get_filters();
  if(!filter_mimetypes[mime_type]) return 0;
  return filter_mimetypes[mime_type];
}

mapping(string:Search.Filter.Base) get_filter_mime_types()
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
