
// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Indexer.pmod,v 1.18 2004/08/08 14:22:53 js Exp $

//!
void index_document(Search.Database.Base db,
		    string|Standards.URI uri,
		    void|string language,
		    mapping fields)
{
  db->remove_document( uri, language );
  int mtime = (int)fields->mtime;
  foreach(indices(fields), string field)
  {
    string f;
    if( strlen(f = fields[field] ) )
    {
      array words=Search.Utils.tokenize_and_normalize( f );
      db->insert_words(uri, language, field, words );
    }
  }
  if( mtime )
      db->set_lastmodified( uri, language, mtime );
// Tokenize any anchor fields
//    int source_hash=hash((string)uri)&0xf;
//    foreach(indices(uri_anchors|| ({ })), string link_uri)
//    {
//      array(string) words=
//        Search.Utils.tokenize_and_normalize(uri_anchors[link_uri]);
//      db->insert_words(link_uri, 0, "anchor", words, source_hash);
//    }
}

//!
string extension_to_type(string extension)
{
   return MIME.ext_to_media_type(extension) || "application/octet-stream";
}

//!
string filename_to_type(string filename)
{
   array v=filename/".";
   if (sizeof(v)<2) return extension_to_type("default");
   return extension_to_type(v[-1]);
}

//!
Search.Filter.Output filter_and_index(Search.Database.Base db,
				      string|Standards.URI uri,
				      void|string language,
				      string|Stdio.File data,
				      string content_type,
				      void|mapping headers,
				      void|string default_charset )
{
  Search.Filter.Base filter=Search.get_filter(content_type);
  if(!filter)
    return 0;

  Search.Filter.Output filteroutput=
    filter->filter(uri, data, content_type,
		   headers, default_charset);
  index_document(db, uri, language, filteroutput->fields);
  return filteroutput;
}

//!
void remove_document(Search.Database.Base db,
		     string|Standards.URI uri,
		     void|string language)
{
  db->remove_document(uri, language);
}
