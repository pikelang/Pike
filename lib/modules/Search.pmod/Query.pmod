mapping blob_done=([]);

static function(int:string) blobfeeder(Search.Database.Base db, array word_ids)
{
  mapping state = mkmapping(word_ids,allocate(sizeof(word_ids)));
  return lambda( int word )
	 {
	   return db->get_blob(word, state[word]++);
	 };
}


Search.ResultSet do_query_or(Search.Database.Base db,
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

Search.ResultSet do_query_and(Search.Database.Base db,
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

Search.ResultSet do_query_phrase(Search.Database.Base db,
				 array(string) words,
				 Search.RankingProfile ranking)
{
  array(int) word_ids=map(words, db->hash_word);
  return _WhiteFish.do_query_phrase(word_ids,
				    ranking->field_ranking,
				    //    ranking->cutoff,
				    blobfeeder(db, word_ids));
}



// Execute a parsed, optimized and validated query by a parser

static void lowlevel(string s, mixed ... args) {
  werror(s, @args);
  werror("\n");
}

static constant ParseNode = Search.Grammar.ParseNode;

void execute(ParseNode q) {
  switch (q->op) {
    case "and":
      {
      int first = 1;
      foreach (q->children, ParseNode child)
        if (child->op != "date") {
          execute(child);
          if (!first)
            lowlevel("AND");
          else
            first = 0;
        }
      foreach (q->children, ParseNode child)
        if (child->op == "date")
          execute(child);
      }
      break;
    case "or":
      int first = 1;
      foreach (q->children, ParseNode child) {
        execute(child);
        if (!first)
          lowlevel("OR");
        else
          first = 0;
      }
      break;
    case "date":
      lowlevel("DATE_FILTER %O", q->date);
      break;
    case "text":
      {
      int hasPlus = sizeof(q->plusWords) || sizeof(q->plusPhrases);
      int hasOrdinary = sizeof(q->words) || sizeof(q->phrases);
      int hasMinus = sizeof(q->minusWords) || sizeof(q->minusPhrases);
      if (hasPlus) {
        int first = 1;
        if (sizeof(q->plusWords)) {
          lowlevel("QUERY_AND     field:%O %{ %O%}", q->field, q->plusWords);
          first = 0;
        }
        foreach (q->plusPhrases, array(string) ph) {
          lowlevel("QUERY_PHRASE  field:%O %{ %O%}", q->field, ph);
          if (first)
            first = 0;
          else
            lowlevel("AND");
        }
      }
      if (hasOrdinary) {
        int first = 1;
        if (sizeof(q->words)) {
          lowlevel("QUERY_OR      field:%O %{ %O%}", q->field, q->words);
          first = 0;
        }
        foreach (q->phrases, array(string) ph) {
          lowlevel("QUERY_PHRASE  field:%O %{ %O%}", q->field, ph);
          if (first)
            first = 0;
          else
            lowlevel("OR");
        }
      }

      if (hasPlus && hasOrdinary)
        lowlevel("UPRANK");          // the XXX operation :)

      if (hasMinus) {
        int first = 1;
        if (sizeof(q->minusWords)) {
          lowlevel("QUERY_OR      field:%O %{ %O%}", q->field, q->minusWords);
          first = 0;
        }
        foreach (q->minusPhrases, array(string) ph) {
          lowlevel("QUERY_PHRASE  field:%O %{ %O%}", q->field, ph);
          if (first)
            first = 0;
          else
            lowlevel("OR");
        }
        lowlevel("SUB"); // Only - words are not allowed
      }

      break;
      }
  } // switch (q->op)
}


/* Test stuff */
Search.ResultSet test_query(Search.Database.Base db, array(string) words)
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

_WhiteFish.ResultSet test_query2(Search.Database.Base db, array(string) words)
{
  array(int) field_ranking=allocate(66);
  field_ranking[0]=17;
  field_ranking[2]=47;
  return _WhiteFish.do_query_phrase(map(words, hash),
				    field_ranking,
				    blobfeeder(db, map(words,hash) ));
}
