// Roxen Whitefish main pike module
//
// Copyright © 2000, Roxen IS.

#include "types.h"

class Document
{
  //! The placeholder for document metadata.
  
  string title;
  string description;
  int last_changed;
  int size;
  string content_type;
}

private mapping filters=([]);

void create()
{
  werror("Loading filters\n");
  foreach(values(Search.Filter), Search.Filter filter)
    foreach(filter->contenttypes || ({ }), string mime)
      filters[mime]=filter;
  
  if(!sizeof(filters))
    werror("No filters loaded\n");
  else
    werror("Loaded %d filters\n", sizeof(filters));
}

Filter get_filter(string mime_type)
{
  if(!filters[mime_type]) return 0;
  return filters[mime_type]->Filter();
}

array(string) get_filter_mime_types()
{
  return indices(filters);
}

array(mapping) splitter(array(string) text, array(int) context,
			function(string:string) post_normalization,
			function(mapping:int) ranking)
{
  if(sizeof(text)!=sizeof(context))
    return 0;

  array(mapping) result=({});
  for(int i=0; i<sizeof(text); i++)
  {
    array words=text[i]/" ";
    int inc=0, oldinc;
    foreach(words, string word)
    {
      oldinc=inc;
      inc+=sizeof(word)+1;
      word=post_normalization(word);
      if(!sizeof(word)) continue;
      mapping n_word=([ "word":word,
			"type":context[i],
			//			"offset":offset[i]+oldinc,
			// This might be destroyed by pre_normalization
      ]);
      n_word->rank=ranking(n_word);
      result+=({ n_word });
    }
  }
  
  return result;
}


// ---------- Anchor database -------------

class Anchor_database {

  void add(string page, string href, string text) {
  }

  array(string) get_texts(string page) {
    return ({});
  }

}


// --- Page Ranking Algorithms ------------

float entropy(array(string) page_words) {
  mapping(string:int) words=([]);
  foreach(page_words, string word)
    words[word]=1;
  return (float)sizeof(words)/(float)sizeof(page_words);
}


private constant rank_list = ([
  T_TITLE    : 1,
  T_KEYWORDS : 2,
  T_EXT_A    : 3,
  T_H1       : 4,
  T_H2       : 5,
  T_H3       : 6,
  T_DESC     : 7,
  T_H4       : 8,
  T_TH       : 9,
  T_B        : 10,
  T_I        : 11,
  T_A        : 12,
  T_NONE     : 13,
  T_H5       : 14,
  T_H6       : 15 ]);

int rank(mapping word)
{
  return rank_list[word->type];
}

// A normal page has an entropy value around 0.5, so the result x should probably be
// remapped to abs(x-0.5) or even 1-abs(x-0.5)

