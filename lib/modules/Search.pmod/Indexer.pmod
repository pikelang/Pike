void index_document(Search.Database.Base db,
				    string|Standards.URI uri,
				    void|string language,
				    mapping fields,
				    mapping uri_anchors)
{
  db->remove_document( uri, language );

  foreach(indices(fields), string field)
  {
    string f;
    if( strlen(f = fields[field] ) )
    {
      array words=Search.Utils.tokenize(Search.Utils.normalize(f));
      db->insert_words(uri, language, field, words );
    }
  }
  // Tokenize any anchor fields
  
  int source_hash=hash((string)uri)&0xf;
  foreach(indices(uri_anchors|| ({ })), string link_uri)
  {
    array(string) words=
      Search.Utils.tokenize(Search.Utils.normalize(uri_anchors[link_uri]));
    db->insert_words(link_uri, 0, "anchor", words, source_hash);
  }
  
  h = gethrtime();
  int source_hash=hash((string)uri)&0xf;
  foreach(indices(filteroutput->uri_anchors || ({ })), string link_uri)
  {
    array(string) words=
      Search.Utils.tokenize(Search.Utils.normalize
			    (filteroutput->uri_anchors[link_uri]));
    db->insert_words(link_uri, 0, "anchor", words, source_hash);
  }
}


array(Standards.URI) filter_and_extract_links(Search.Database.Base db,
					      string|Standards.URI uri,
					      void|string language,
					      string|Stdio.File data,
					      string content_type)
{
  Search.Filter.Base filter=Search.get_filter(content_type);
  if(!filter)
    throw("No indexer for content type "+content_type);

  Search.Filter.Base.Output filteroutput=filter->filter(uri, data, content_type);
  index_document(db, uri, language, filteroutput->fields, filteroutput->uri_anchors);
  return filteroutput->links;
}

void remove_document(Search.Database.Base db,
		     string|Standards.URI|string uri,
		     void|string language)
{
  db->remove_document(uri, language);
}

array(Standards.URI) test_index(Search.Database.Base db, string uri)
{
  object request=Protocols.HTTP.get_url(uri);

  return filter_and_index_document(db, uri, 0, request->data(),
				   request->headers["content-type"]);
}
