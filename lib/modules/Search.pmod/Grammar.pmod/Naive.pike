// Really simple search grammar
// Copyright © 2000, Roxen IS.

array(mapping) do_query(string q) {

  array(mapping) result=({});

#ifdef Webserver
  if(result=cache_lookup("searchfoo", q)
     return result;
#endif

  array(string) words=({ ({}),({}) });
  array(string) urls=({ ({}),({}) });

  array(string) in=({});
  array(string) temp=q/"\"";

  for(int i=0; i<sizeof(temp); i++) {
    if(i%2)
      in+=({ temp[i] });
    else
      in+=temp[i]/" ";
  }

  in-=({""});

  int neg;
  foreach(in, string word) {
    if(sizeof(word)<2) continue; // Nop, we do not support character search.

    if(lower_case(word)=="and") continue;

    neg=0;
    if(word[0]=='-') {
      neg=1;
      word=word[1..];
    }
    if(word[0]=='+') {
      neg=0;
      word=word[1..];
    }
    if(word[0..3]=="url:") {
      if(sizeof(word)>4) urls[neg]+=({ word[4..] });
      continue;
    }
    words[neg]+=({ word });
  }

  if(!sizeof(word[0])) return ({});

  array(mapping) pos=index->query(words[0]);
  array(mapping) neg=({});
  if(sizeof(words[1]))
    neg=index->query(words[1]);
  multiset filter=(multiset)neg->url;

  foreach(pos, mapping res) {
    if(filter[res->url]) continue;

    if(sizeof(urls[0])) {
      int fail;
      foreach(urls[0], string url)
	if(!has_value(res->url, url)) {
	  fail=1;
	  break;
	}
      if(fail) continue;
    }
    if(sizeof(urls[1])) {
      int fail;
      foreach(urls[1], string url)
	if(has_value(res->url, url)) {
	  fail=1;
	  break;
	}
      if(fail) continue;
    }

    result=({ res });
  }

#ifdef Webserver
  cache_set("searchfoo", q, result);
#endif

  return result;
}
