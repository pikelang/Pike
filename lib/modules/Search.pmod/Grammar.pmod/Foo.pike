// Really simple search grammar
// Copyright © 2000, Roxen IS.

object indexer;

void create(object _indexer) {
  indexer=_indexer;
}

array(mapping) do_query(string q) {

#ifdef Webserver
  if(result=cache_lookup("searchfoo", q))
     return result;
#endif

  array(string) positive_and = ({});
  array(string) positive_or  = ({});
  array(string) negative_and = ({});
  array(string) in = q/" " - ({""});

  foreach(in, string word) {
    if(sizeof(word)<2) continue; // Nop, we do not support character search.

    if(word[0]=='-') {
      negative_and += ({ word[1..] });
    }
    else if(word[0]=='+') {
      positive_and += ({ word[1..] });
    }
    positive_or += ({ word });
  }

  if(!sizeof(positive_and) && !sizeof(positive_or)) return ({});

  array(mapping) pos_or;
  array(mapping) pos_and;

  if(positive_and) pos_and = indexer->lookup_words_and(positive_and);
  if(positive_or)  pos_or  = indexer->lookup_words_or(positive_or);

  if(!sizeof(pos_and) && !sizeof(pos_or)) {
#ifdef Webserver
    cache_set("searchfoo", q, result);
#endif
    return ({});
  }

  array(mapping) result = pos_and + pos_or;

  if(negative_and) {
    array(mapping) neg_and = indexer->lookup_words_and(negative_and);
    multiset filter=(multiset)neg->url;
    array(mapping) temp = ({});

    foreach(temp, mapping res) {
      if(filter[res->url]) continue;

      temp += ({ res });
    }
    result = temp;
  }

#ifdef Webserver
  cache_set("searchfoo", q, result);
#endif

  return result;
}
