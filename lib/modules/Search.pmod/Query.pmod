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
                                ranking->proximity_ranking,
				ranking->cutoff,
				blobfeeder(db, word_ids));
}

Search.ResultSet do_query_and(Search.Database.Base db,
			      array(string) words,
			      Search.RankingProfile ranking)
{
  array(int) word_ids=map(Array.uniq(words), db->hash_word);
  return _WhiteFish.do_query_and(word_ids,
                                 ranking->field_ranking,
                                 ranking->proximity_ranking,
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

//! @param query
//!   The query string entered by user.
//! @param db
//!   The search database.
//! @param defaultRanking
//!   Used when searching in the field "any:".
Search.ResultSet execute(Search.Database.Base db,
                         Search.Grammar.AbstractParser parser,
                         string query,
                         Search.RankingProfile defaultRanking)
{

  Search.Grammar.ParseNode q = parser->parse(query);
  q = Search.Grammar.optimize(q);
  string error = Search.Grammar.validate(q);
  if (error)
    throw (error);

  return class {
    static Search.RankingProfile defaultRanking;
    static Search.Database.Base db;

    // Used when search is limited to another field than "any:".
    static Search.RankingProfile specialRanking;

    static void create(Search.Database.Base _db, Search.RankingProfile _defaultRanking) {
      db = _db;
      defaultRanking = _defaultRanking;
      specialRanking = defaultRanking->copy();
    }

    static constant ParseNode = Search.Grammar.ParseNode;

    static array(Search.ResultSet) stack = ({ });
    static void push(Search.ResultSet r) {
      werror("---PUSH\n");
      stack = ({ r }) + stack;
    }
    static Search.ResultSet pop() {
      werror("---POP\n");
      if (!sizeof(stack))
        error("Very bad!");
      Search.ResultSet r = stack[0];
      stack = stack[1 .. ];
      return r;
    }

    Search.ResultSet execute(ParseNode q) {
      exec(q);
      if (sizeof(stack) != 1)
        throw ("Stack should have exactly one item!");
      return pop();
    }

    void exec(ParseNode q) {
      werror("EXEC %s\n", q->op);
      switch (q->op) {
        case "and":
          {
          int first = 1;
          foreach (q->children, ParseNode child)
            if (child->op != "date") {
              exec(child);
              if (!first) {
                Search.ResultSet r2 = pop();
                Search.ResultSet r1 = pop();
                push(r1 & r2);
              }
              else
                first = 0;
            }
          // ( DATE: limitations not implemented yet... )
          //
          // foreach (q->children, ParseNode child)
          //   if (child->op == "date")
          //     exec(child);
          }
          break;
        case "or":
          int first = 1;
          foreach (q->children, ParseNode child) {
            exec(child);
            if (!first) {
              Search.ResultSet r2 = pop();
              Search.ResultSet r1 = pop();
              push(r1 | r2);
            }
            else
              first = 0;
          }
          break;
        case "date":
          // NOT IMPLEMENTED YET
          break;
        case "text":
          {
          Search.RankingProfile ranking = defaultRanking;

          if (q->field != "any") {
            ranking = specialRanking;
            int fieldID = db->get_field_id(q->field, 1);
            if (!fieldID && q->field != "body") {
              // There was no such field, so we push an empty ResultSet !
              push(Search.ResultSet());
              break;
            }
            ranking->field_ranking = allocate(66);

            ranking->field_ranking[fieldID] = defaultRanking->field_ranking[fieldID];
            // ranking->field_ranking[fieldID] = 1;
          }

          int hasPlus = sizeof(q->plusWords) || sizeof(q->plusPhrases);
          int hasOrdinary = sizeof(q->words) || sizeof(q->phrases);
          int hasMinus = sizeof(q->minusWords) || sizeof(q->minusPhrases);
          if (hasPlus) {
            int first = 1;
            if (sizeof(q->plusWords)) {
              push(do_query_and(db, q->plusWords, ranking));
              first = 0;
            }
            foreach (q->plusPhrases, array(string) ph) {
              push(do_query_phrase(db, ph, ranking));
              if (!first) {
                Search.ResultSet r2 = pop();
                Search.ResultSet r1 = pop();
                push(r1 & r2);
              }
              first = 0;
            }
          }
          if (hasOrdinary) {
            int first = 1;
            if (sizeof(q->words)) {
              push(do_query_and(db, q->words, ranking));
              first = 0;
            }
            foreach (q->phrases, array(string) ph) {
              push(do_query_phrase(db, ph, ranking));
              if (!first) {
                Search.ResultSet r2 = pop();
                Search.ResultSet r1 = pop();
                push(r1 | r2);
              }
              first = 0;
            }
          }

          if (hasPlus && hasOrdinary) {
            Search.ResultSet r2 = pop();
            Search.ResultSet r1 = pop();
            push(r1->add(r2));
          }

          if ((hasPlus || hasOrdinary) && hasMinus) {
            int first = 1;
            if (sizeof(q->minusWords)) {
              push(do_query_or(db, q->minusWords, ranking));
              first = 0;
            }
            foreach (q->minusPhrases, array(string) ph) {
              push(do_query_phrase(db, ph, ranking));
              if (!first) {
                Search.ResultSet r2 = pop();
                Search.ResultSet r1 = pop();
                push(r1 | r2);
              }
              first = 0;
            }
            Search.ResultSet r2 = pop();
            Search.ResultSet r1 = pop();
            push(r1 - r2);
          }

          }
          break;
        default:
          error("Unknown type of ParseNode!");
      } // switch (q->op)
    }

  } (db, defaultRanking)->execute(q);
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
