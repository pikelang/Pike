
//! This object is returned from @[Search.Filter] plugins.

constant contenttypes = ({ }); // Hide this program.

// Wide strings here

//! Data extracted from input, grouped by type. Standard fields are
//! @expr{"body"@}, @expr{"title"@}, @expr{"description"@} and
//! @expr{"keywords"@}.
mapping(string:string) fields=([]);
// body, title, description, keywords

//! The size of the document.
int document_size;

//! Maps un-normalized URLs to raw text, e.g.
//! @expr{ ([ "http://pike.ida.liu.se": "Pike language" ]) @}.
mapping(string:string) uri_anchors=([]);

//! All links collected from the document.
array(Standards.URI|string) links=({});

//! Modifies relative links in @[links] to be relative to @[base_uri].
void fix_relative_links(Standards.URI base_uri) {
  for(int i=0; i<sizeof(links); i++)
  {
    links[i]=Standards.URI(links[i], base_uri);
    if(links[i]->fragment)
      links[i]->fragment=0;
  }
}
