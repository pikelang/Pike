mapping blob_done=([]);

function(int:string) blobfeeder(Search.Database.MySQL db, array word_ids)
{
  mapping state = mkmapping(word_ids,allocate(sizeof(word_ids)));
  return lambda( int word )
	 {
	   return db->get_blob(word, state[word]++);
	 };
}

_WhiteFish.ResultSet test_query(Search.Database.MySQL db, array(string) words)
{
  array(int) field_ranking=allocate(66);
  field_ranking[0]=17;
  field_ranking[2]=47;

  array(int) prox_ranking=allocate(8);
  for(int i=0; i<8; i++)
    prox_ranking[i]=8-i;

  return _WhiteFish.do_query_merge(map(words, hash),
				   field_ranking,
				   prox_ranking,
				   blobfeeder(db, map(words,hash) ));
}

_WhiteFish.ResultSet test_query2(Search.Database.MySQL db, array(string) words)
{
  array(int) field_ranking=allocate(66);
  field_ranking[0]=17;
  field_ranking[2]=47;
  return _WhiteFish.do_query_phrase(map(words, hash),
				    field_ranking,
				    blobfeeder(db, map(words,hash) ));
}
