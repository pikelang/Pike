array(Standards.URI) index_document(Search.Database.MySQL db,
				    string|Standards.URI uri,
				    string|Stdio.File data,
				    string content_type,
				    void|string language)
{
  Search.Filter.Base filter=Search.get_filter(content_type);
  if(!filter)
    error("No indexer for content type "+content_type);

//   int h = gethrtime();

  Search.Filter.Base.Output filteroutput=
    filter->filter(uri, data, content_type);

//   werror("filter: %dms\n", (gethrtime()-h)/1000 );

  db->remove_document( uri, language );
  // Tokenize and normalize all the non-anchor fields

  foreach(indices(filteroutput->fields), string field)
  {
    if( strlen(filteroutput->fields[field] ) )
    {
//       h = gethrtime();
      string q =Search.Utils.normalize(filteroutput->fields[field]);
//       werror("normalize: %dms\n", (gethrtime()-h)/1000 );
    
//       h = gethrtime();
      array words = Search.Utils.tokenize(q);
//       werror("tokenize: %dms\n", (gethrtime()-h)/1000 );
      db->insert_words(uri, language, field, words);
    }
  }

  // Tokenize any anchor fields
  int source_hash=hash((string)uri)&0xf;
  foreach(indices(filteroutput->uri_anchors || ({ })), string link_uri)
  {
    array(string) words=
      Search.Utils.tokenize(Search.Utils.normalize
			    (filteroutput->uri_anchors[link_uri]));
    db->insert_words(link_uri, 0, "anchor", words, source_hash);
  }
  return filteroutput->links;
}

array(Standards.URI) extract_links(Search.Database.MySQL db,
				   string|Standards.URI uri,
				   string|Stdio.File data,
				   string content_type)
{
  Search.Filter.Base filter=Search.get_filter(content_type);
  if(!filter)
    throw("No indexer for content type "+content_type);

  Search.Filter.Base.Output filteroutput=filter->filter(uri, data, content_type);
  return filteroutput->links;
}

void remove_document(Search.Database.MySQL db,
		     string|Standards.URI|string uri,
		     void|string language)
{
  db->remove_document(uri, language);
}

array(Standards.URI) test_index(Search.Database.MySQL db, string uri)
{
  object request=Protocols.HTTP.get_url(uri);

  return index_document(db, uri, request->data(),
			request->headers["content-type"]);
}
