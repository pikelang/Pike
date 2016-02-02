#pike __REAL_VERSION__

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
    if( !filter->contenttypes ) continue;
    foreach(filter->contenttypes, string mime)
      filter_mimetypes[mime]=filter;

    foreach(filter->fields || ({ }), string field)
      filter_fields[field]=1;
  }
}

//! @decl Search.Filer.Base get_filter(string mime_type)
//!
//! Returns the appropriate filter object for the given mime
//! type. This will be one of the objects in @[Search.Filter].
object get_filter(string mime_type)
{
  if(!filter_mimetypes)
    get_filters();
  return filter_mimetypes[mime_type];
}

//! @decl mapping(string:Search.Filter.Base) get_filter_mime_types()
//!
//! Returns a mapping from mime-type to filter objects. The filter
//! objects are from @[Search.Filter].
mapping(string:object) get_filter_mime_types()
{
  if(!filter_mimetypes)
    get_filters();
  return filter_mimetypes;
}

//! Returns an array of field types supported by the available set of
//! media plugins.
array(string) get_filter_fields()
{
  if(!filter_fields)
    get_filters();
  return indices(filter_fields);
}
