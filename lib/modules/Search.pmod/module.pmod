// Roxen Whitefish main pike module
//
// Copyright © 2000,2001 Roxen IS.



private mapping filters;

// void create()
// {
//   foreach(values(Search.Filter), program filter)
//   {
//     Search.Filter.Base tmp=filter();
//     foreach(tmp->contenttypes || ({ }), string mime)
//       filters[mime]=tmp;
//   }
// }

private void get_filters()
{
  filters=([]);
  foreach(values(Search.Filter), object filter)
    foreach(filter->contenttypes || ({ }), string mime)
      filters[mime]=filter;
}

Search.Filter.Base get_filter(string mime_type)
{
  if(!filters)
    get_filters();
  if(!filters[mime_type]) return 0;
  return filters[mime_type];
}

mapping(string:Search.Filter.Base) get_filter_mime_types()
{
  if(!filters)
    get_filters();
  return filters;
}



// --- Page Ranking Algorithms ------------

float entropy(array(string) page_words) {
  mapping(string:int) words=([]);
  foreach(page_words, string word)
    words[word]=1;
  return (float)sizeof(words)/(float)sizeof(page_words);
}


// A normal page has an entropy value around 0.5, so the result x should probably be
// remapped to abs(x-0.5) or even 1-abs(x-0.5)

