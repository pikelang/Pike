mapping blob_done=([]);

static function(int:string) blobfeeder(Search.Database.MySQL db, array word_ids)
{
  mapping state = mkmapping(word_ids,allocate(sizeof(word_ids)));
  return lambda( int word )
	 {
	   return db->get_blob(word, state[word]++);
	 };
}


Search.ResultSet do_query_or(Search.Database.MySQL db,
			     array(string) words,
			     Search.RankingProfile ranking)
{
  array(int) word_ids=map(Array.uniq(words), db->hash_word);
  return _WhiteFish.do_query_or(word_ids,
				ranking->field_ranking,
				ranking->prox_ranking,
				ranking->cutoff,
				blobfeeder(db, word_ids));
}

Search.ResultSet do_query_and(Search.Database.MySQL db,
			      array(string) words,
			      Search.RankingProfile ranking)
{
  array(int) word_ids=map(Array.uniq(words), db->hash_word);
  return _WhiteFish.do_query_or(word_ids,
				ranking->field_ranking,
				ranking->prox_ranking,
				ranking->cutoff,
				blobfeeder(db, word_ids));
}

Search.ResultSet do_query_phrase(Search.Database.MySQL db,
				 array(string) words,
				 Search.RankingProfile ranking)
{
  array(int) word_ids=map(words, db->hash_word);
  return _WhiteFish.do_query_phrase(word_ids,
				    ranking->field_ranking,
				    //    ranking->cutoff,
				    blobfeeder(db, word_ids));
}


/* Test stuff */
Search.ResultSet test_query(Search.Database.MySQL db, array(string) words)
{
  array(int) field_ranking=allocate(66);
  field_ranking[0]=17;
  field_ranking[2]=147;

  array(int) prox_ranking=allocate(8);
  for(int i=0; i<8; i++)
    prox_ranking[i]=8-i;

  return _WhiteFish.do_query_and(map(words, hash),
				   field_ranking,
				   prox_ranking,
				   8,
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
