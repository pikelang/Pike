// Roxen Intraseek 3 main pike module
// Roxen Pathfinder
// Roxen Finder
// Pike Seek
// IKEA Leta
//
// Copyright © 2000, Roxen IS.

#include "types.h"

private mapping filters=([]);

void create() {

  // Load filters
  werror("Load filters\n");
  array tmp=__FILE__/"/";
  tmp=tmp[0..sizeof(tmp)-2];
  string path=tmp*"/"+"/filters/";
  //  catch {
    array(string) f=get_dir( path );
    foreach(glob("*.pike",f), string file) {
      //      mixed error = catch {
	werror("Try with %s\n", path+file);
	object l=(object)(path+file);
	array(string) mimes = l->contenttypes;
	foreach(mimes, string mime)
	  filters[mime]=l;
	//      };
	//      if(error) werror("Failed to load filters/%s\n",file);
    }
    //  };
  if(!sizeof(filters))
    werror("No filters loaded\n");
  else
    werror("Loaded %d filters\n", sizeof(filters));
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

int rank(mapping word) {
  return rank_list[word->type];
}

class Filter {
  void set_content(string);
  array(array(string)) get_anchors();
  void add_content(string, int);
  array(array) get_filtered_content();
  string get_title();
  string get_keywords();
  string get_description();
  // string normalization(string);
}

Filter get_filter(string mime_type) {
  if(!filters[mime_type]) return 0;
  return filters[mime_type]->Filter();
}

array(string) get_filter_mime_types() {
  return indices(filters);
}

array(mapping) splitter(array(string) text, array(int) context, array(int) offset,
			function(string:string) post_normalization,
			function(mapping:int) ranking) {
  if(sizeof(text)!=sizeof(context) ||
     sizeof(text)!=sizeof(offset) ) return 0;

  array(mapping) result=({});
  for(int i=0; i<sizeof(text); i++) {
    array words=text[i]/" ";
    int inc=0, oldinc;
    foreach(words, string word) {
      oldinc=inc;
      inc+=sizeof(word)+1;
      word=post_normalization(word);
      if(!sizeof(word)) continue;
      mapping n_word=([ "word":word,
			"type":context[i],
			"offset":offset[i]+oldinc, // This might be destroyed by pre_normalization
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
