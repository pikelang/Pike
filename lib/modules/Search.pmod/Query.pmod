

// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Query.pmod,v 1.30 2004/12/29 13:27:22 anders Exp $

static function(string,int:string) blobfeeder(Search.Database.Base db,
                                              array words)
{
  mapping state = mkmapping(words,allocate(sizeof(words)));
  mapping(string:mapping(int:string)) blobcache = ([ ]);
  return lambda( string word, int foo )
         {
           return db->get_blob(word, state[word]++, blobcache);
         };
}

static array(string) uniq_preserve_order(array(string) a) {
  array(string) result = ({});
  foreach (a, string s)
    if (search(result, s) < 0)
      result += ({ s });
  return result;
}

Search.ResultSet do_query_or(Search.Database.Base db,
                             array(string) words,
                             Search.RankingProfile ranking)
{
  Search.ResultSet result =
    _WhiteFish.do_query_or(words,
                           ranking->field_ranking,
                           ranking->proximity_ranking,
                           ranking->cutoff,
                           blobfeeder(db, words));
  return result;
}

Search.ResultSet do_query_and(Search.Database.Base db,
                              array(string) words,
                              Search.RankingProfile ranking)
{
  Search.ResultSet result =
    _WhiteFish.do_query_and(words,
                            ranking->field_ranking,
                            ranking->proximity_ranking,
                            ranking->cutoff,
                            blobfeeder(db, words));
  return result;
}

Search.ResultSet do_query_phrase(Search.Database.Base db,
                                 array(string) words,
                                 Search.RankingProfile ranking)
{
  Search.ResultSet result =
    _WhiteFish.do_query_phrase(words,
                               ranking->field_ranking,
                               //    ranking->cutoff,
                               blobfeeder(db, words));
  return result;
}

enum search_order
{
  RELEVANCE=1, DATE_ASC, DATE_DESC, NONE
};

static Search.ResultSet sort_resultset(Search.ResultSet resultset,
                                       search_order order,
                                       Search.Database.Base db)
{
  
}


//! @param query
//!   The query string entered by user.
//! @param db
//!   The search database.
//! @param defaultRanking
//!   Used when searching in the field "any:".
//!
//! @returns
//!   An array with two elements:
//!   @array
//!     @elem Search.ResultSet 0
//!       The ResultSet containing the hits.
//!     @elem array(string) 1
//!       All wanted words in the query. (I.e. not the words that were
//!       preceded by minus.)
//!   @endarray
//!
array(Search.ResultSet|array(string)) execute(Search.Database.Base db,
                                              Search.Grammar.AbstractParser parser,
                                              string query,
                                              Search.RankingProfile ranking,
                                              void|array(string) stop_words,
                                              void|search_order order)
{
  Search.Grammar.ParseNode q = parser->parse(query);
  if (stop_words && sizeof(stop_words))
    Search.Grammar.remove_stop_words(q, stop_words);

  q = Search.Grammar.optimize(q);
  
  if (!q)                                  // The query was a null query
    return ({ Search.ResultSet(), ({}) }); // so return an empty resultset

  string error = Search.Grammar.validate(q);
  if (error)
    throw (error);

  array(Search.ResultSet|array(string)) res =  class {
    static Search.RankingProfile defaultRanking;
    static Search.Database.Base db;

    // Used when search is limited to another field than "any:".
    static Search.RankingProfile specialRanking;

    static void create(Search.Database.Base _db, Search.RankingProfile _defaultRanking) {
      db = _db;
      defaultRanking = _defaultRanking;
      specialRanking = defaultRanking->copy();
      pop = stack->pop;
      push = stack->push;
    }

    static array(array(string)) split_words(array(string) words)
    {
      array a=({}),b=({});
      foreach(words, string word)
        if(has_value(word, "*") || has_value(word, "?"))
          b+=({ word });
        else
          a+=({ word });
      return ({ a, b });
    }
    
    static constant ParseNode = Search.Grammar.ParseNode;

    static array(array(string)|string) words = ({ });
    static ADT.Stack stack = ADT.Stack();
    static function(Search.ResultSet:void) push;
    static function(void:Search.ResultSet) pop;

    array(Search.ResultSet|array(string)) execute(ParseNode q) {
      exec(q);
      if (sizeof(stack) != 1)
        error("Stack should have exactly one item!");
      return ({ pop(), words });
    }


    void exec(ParseNode q) {
      int max_globs = 100;
      switch (q->op) {
        case "and":
        {
          int first = 1;
          foreach (q->children, ParseNode child)
          {
            exec(child);
            if (!first) {
              Search.ResultSet r2 = pop();
              Search.ResultSet r1 = pop();
              push(r1 & r2);
            }
            else
              first = 0;
          }
        }
        break;
        case "or":
          {
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
          }
          break;
        case "date":
          _WhiteFish.DateSet global_dateset = db->get_global_dateset();

          if(!sizeof(global_dateset))
          {
            push(global_dateset);
            break;
          }

          int t_low, t_high;
	  catch {
	    t_low = t_high = Calendar.ISO.dwim_day(String.trim_whites(q->date))->unix_time();
	    t_high += 24*60*60-1;   // Add 24 h to end of the day.
	  };

	  // Fix to allow year-month "%04d-%02d" timerange.
	  if(!t_low && sscanf(q->date, "%4d-%2d", int y, int m) == 2)
	    catch {
	      Calendar.ISO.Month month = Calendar.ISO.Month(y, m);
	      t_low  = month->unix_time();
	      t_high = month->next()->unix_time()-1;
	    };
	  // Fix to allow year "%04d" timerange.
	  if(!t_low && sscanf(q->date, "%4d", int y))
	    catch {
	      Calendar.ISO.Year year = Calendar.ISO.Year(y);
	      t_low  = year->unix_time();
	      t_high = year->next()->unix_time()-1;
	    };
	  
	  if(t_low <= 0 || t_high <= 0 ||
	     object_program(t_low) || object_program(t_high))
	    // Guard against out-of-bounds and bignums.
	  {
	    push(_WhiteFish.DateSet());
	    break;
	  }
	  
	  _WhiteFish.DateSet restriction;
	  switch(q->operator[1])
	  {
	    case "=":
	      restriction =
		global_dateset->between(t_low-1, t_high+1)->finalize();
	      break;
	    case "<>":
	    case "!=":
	      restriction =
		global_dateset->not_between(t_low, t_high)->finalize();
	      break;
	    case "<=":
	      restriction = global_dateset->before(t_high+1)->finalize();
	      break;
	    case ">=":
	      restriction = global_dateset->after(t_low-1)->finalize();
	      break;
	    case "<":
	      restriction = global_dateset->before(t_low)->finalize();
	      break;
	    case ">":
	      restriction = global_dateset->after(t_high)->finalize();
	      break;
	  }
	  push(restriction || _WhiteFish.DateSet());
          break;
          
        case "text":
          {
            Search.RankingProfile ranking = defaultRanking;

            if (q->field != "any")
            {
              ranking = specialRanking;
              int fieldID = db->get_field_id(q->field, 1);
              if (!fieldID && q->field != "body")
              {
                // There was no such field, so we push an empty ResultSet !
                push(Search.ResultSet());
                break;
              }
              ranking->field_ranking = allocate(65);
              ranking->field_ranking[fieldID] = 1;
            }

            [array plusWords, array plusWordGlobs] = split_words(q->plusWords);
            [array ordinaryWords, array ordinaryWordGlobs] = split_words(q->words);
            [array minusWords, array minusWordGlobs] = split_words(q->minusWords);

//        werror("[%-10s] plus: %-15s   ordinary: %-15s   minus: %-15s\n", q->field, q>plusWords*", ", q->words*", ", q->minusWords*", ");
            
            int hasPlus = sizeof(q->plusWords) || sizeof(q->plusPhrases);
            int hasOrdinary = sizeof(q->words) || sizeof(q->phrases);
            int hasMinus = sizeof(q->minusWords) || sizeof(q->minusPhrases);

            if(hasPlus)
            {
              int first = 1;
              if(sizeof(plusWords))
              {
                words += plusWords;
                push(do_query_and(db, plusWords, ranking));
                first = 0;
              }
              foreach(plusWordGlobs, string plusWordGlob)
              {
                push(do_query_or(db, db->expand_word_glob(plusWordGlob, max_globs), ranking));
                if (!first)
                {
                  Search.ResultSet r2 = pop();
                  Search.ResultSet r1 = pop();
                  push(r1 & r2);
                }
                first = 0;
              }
              foreach (q->plusPhrases, array(string) ph)
              {
                words += ph;
                push(do_query_phrase(db, ph, ranking));
                if (!first)
                {
                  Search.ResultSet r2 = pop();
                  Search.ResultSet r1 = pop();
                  push(r1 & r2);
                }
                first = 0;
              }
            }
            
            if(hasOrdinary)
            {
              int first = 1;
              if (sizeof(ordinaryWords))
              {
                words += ordinaryWords;
                push(do_query_or(db, ordinaryWords, ranking));
                first = 0;
              }
              foreach(ordinaryWordGlobs, string ordinaryWordGlob)
              {
                push(do_query_or(db, db->expand_word_glob(ordinaryWordGlob, max_globs), ranking));
                if (!first)
                {
                  Search.ResultSet r2 = pop();
                  Search.ResultSet r1 = pop();
                  push(r1 | r2);
                }
                first = 0;
              }
              foreach (q->phrases, array(string) ph)
              {
                words += ph;
                push(do_query_phrase(db, ph, ranking));
                if(!first)
                {
                  Search.ResultSet r2 = pop();
                  Search.ResultSet r1 = pop();
                  push(r1 | r2);
                }
                first = 0;
              }
            }
            
            if(hasPlus && hasOrdinary)
            {
              Search.ResultSet r2 = pop();
              Search.ResultSet r1 = pop();
              // If a document contains must-have words AND ALSO may-have words,
              // it's ranking is increased.
              push(r1->add_ranking(r2));
            }
            
            if((hasPlus || hasOrdinary) && hasMinus)
            {
              int first = 1;
              if (sizeof(q->minusWords))
              {
                push(do_query_or(db, q->minusWords, ranking));
                first = 0;
              }
              foreach(minusWordGlobs, string minusWordGlob)
              {
                push(do_query_or(db, db->expand_word_glob(minusWordGlob, max_globs), ranking));
                if(!first)
                {
                  Search.ResultSet r2 = pop();
                  Search.ResultSet r1 = pop();
                  push(r1 | r2);
                }
                first = 0;
              }
              foreach (q->minusPhrases, array(string) ph)
              {
                push(do_query_phrase(db, ph, ranking));
                if (!first)
                {
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

  } (db, ranking)->execute(q);

  res[0] -= db->get_deleted_documents();

  if(!order)
    order = RELEVANCE;
  
  if(order!=NONE)
    switch(order)
    {
      case RELEVANCE:
        res[0]->sort();
        break;
      case DATE_ASC:
      case DATE_DESC:
        res[0] = res[0]->finalize()->add_ranking(db->get_global_dateset());
        if(order==DATE_DESC)
          res[0]->sort();
        else
          res[0]->sort_rev();
      case NONE:
    }

  return res;
}
