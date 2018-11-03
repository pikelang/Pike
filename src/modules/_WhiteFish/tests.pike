int failed;

constant log_msg = Tools.Testsuite.log_msg;
constant log_status = Tools.Testsuite.log_status;

/* Ensure the sets contains the same indices */
void ensure( mixed t, string msg )
{
  if( !t )
  {
    log_msg(msg+"\n");
    failed++;
  }
}

void ensure_i( object r1, object r2, string msg )
{
  if( !equal( column((array)r1,0), column((array)r2,0) ) )
  {
    log_msg(msg+"\n"
	    "OP1: %{%d, %}\n"
	    "OP2: %{%d, %}\n",
	    column((array)r1,0),
	    column((array)r2,0));
    failed++;
  }
}

/* Ensure the sets contains the same indices _and_ values */
void ensure_v( object r1, object r2, string msg )
{
  if( !equal( (array)r1, (array)r2 ) )
  {
    log_msg(msg+"\n"
	    "Got     : %{%d, %} : %{%d, %}\n"
	    "Expected: %{%d, %} : %{%d, %}\n",
	    column((array)r1,0), column((array)r1,1),
	    column((array)r2,0), column((array)r2,1));
    failed++;
  }
}

#define SET_OP( X, Y ) ensure_i(X,Y,"Test "+(test++)+" failed: "+#X+" != "+#Y);
#define OP( X, Y ) ensure_v(X,Y,"Test "+(test++)+" failed: "+#X+" != "+#Y);
#define ENSURE( X ) ensure(X,"Test "+(test++)+" failed: "+#X+" is not true");

int test;

void resultset_tests()
{
  object r0 = _WhiteFish.ResultSet();
  object r1 = _WhiteFish.ResultSet( ({ 1,    3,    5 }));
  object r2 = _WhiteFish.ResultSet( ({    2,    4,    6 }));
  object r3 = _WhiteFish.ResultSet( ({ 1, 2, 3, 4, 5, 6 }) );


  // 1: Check size with friends.

  ENSURE( r0->size() == 0 );  ENSURE( r1->size() == 3 );
  ENSURE( r2->size() == 3 );  ENSURE( r3->size() == 6 );

  log_status("r0->memsize(): %d\r", r0->memsize());
  ENSURE( r0->memsize() );
  log_status("r1->memsize(): %d\r", r1->memsize());
  ENSURE( r0->memsize() <= r1->memsize() );
  log_status("r2->memsize(): %d\r", r2->memsize());
  ENSURE( r1->memsize() == r2->memsize() );
  log_status("r3->memsize(): %d\r", r3->memsize());
  ENSURE( r2->memsize() <= r3->memsize() );

  log_status("r0->overhead(): %d\r", r0->overhead());
  ENSURE( r0->overhead() );
  log_status("r1->overhead(): %d\r", r1->overhead());
  log_status("r2->overhead(): %d\r", r2->overhead());
  ENSURE( r1->overhead() == r2->overhead() );
  log_status("r3->overhead(): %d\r", r3->overhead());
  ENSURE( r2->overhead() > r3->overhead() );


  log_status("r1->dup()->overhead: %d\r", r1->dup()->overhead());
  ENSURE( r1->overhead() >= r1->dup()->overhead() );
  log_status("r3->dup()->overhead: %d\r", r3->dup()->overhead());
  ENSURE( r2->overhead() >= r3->dup()->overhead() );
  ENSURE( r3->overhead() >= r3->dup()->overhead() );
  ENSURE( r0->overhead() >= r3->dup()->overhead() );
  
  // 2: Check set _indices_

  SET_OP( (r0 & r1), r0 );  SET_OP( (r1 & r2), r0 );
  SET_OP( (r3 & r1), r1 );  SET_OP( (r3 & r2), r2 );
  
  SET_OP( (r0 | r1), r1 );  SET_OP( (r1 | r2), r3 );
  SET_OP( (r3 | r1), r3 );  SET_OP( (r3 | r2), r3 );
  
  SET_OP( (r0 + r1), r1 );  SET_OP( (r1 + r2), r3 );
  SET_OP( (r3 + r1), r3 );  SET_OP( (r3 + r2), r3 );
  
  SET_OP( (r0 - r1), r0 );  SET_OP( (r1 - r2), r1 );
  SET_OP( (r3 - r1), r2 );  SET_OP( (r3 - r2), r1 );


  // 3: Check result _values_ 

  object r3_r2 = _WhiteFish.ResultSet(
    ({ 1, ({ 2, 2 }), 3, ({ 4, 2 }), 5, ({ 6, 2 }), }) );
  
  object r3_r1 = _WhiteFish.ResultSet(
    ({ ({ 1, 2 }), 2, ({ 3, 2 }),  4,  ({ 5, 2 }),  6 }) );
  object r3_r3 = _WhiteFish.ResultSet(
    ({ ({ 1, 2 }), ({ 2, 2 }), ({ 3, 2 }),
       ({ 4, 2 }), ({ 5, 2 }), ({ 6, 2 }), }) );

  object r1_r1 = _WhiteFish.ResultSet( ({ ({1, 2}), ({ 3, 2 }), ({ 5,2 }) }) );
  object r2_r2 = _WhiteFish.ResultSet( ({ ({2, 2}), ({ 4, 2 }), ({ 6,2 }) }) );


  /* |: Ranking should be added. */
  OP((r0|r0), r0); OP((r0|r1), r1 );
  OP((r0|r2), r2); OP((r0|r3), r3 );

  OP((r1|r0), r1); OP((r1|r1),r1_r1 );
  OP((r1|r2), r3); OP((r1|r3),r3_r1 );

  OP((r2|r0), r2); OP((r2|r1),r3 );
  OP((r2|r2), r2_r2);OP((r2|r3),r3_r2 );

  OP((r3|r0), r3); OP((r3|r1),r3_r1 );
  OP((r3|r2), r3_r2);OP((r3|r3),r3_r3 );


  /* &: Ranking should be minimum. */
  OP( r3_r3&r3, r3 );         OP( r3_r2&r2, r2 );
  OP( r3_r3&r3_r2, r3_r2 );   OP( r3_r3&r1, r1 );
  OP( r3_r3&r0, r0 );

  /* -: Ranking should be ignored in RHS. */
  OP( r3_r3-r3, r0 );  OP( r3-r2, r1 );  OP( r3-r1, r2 );
  OP( r3_r3-r1, r3_r2-r1 );

  /* add_ranking: Ranking should be added to the left, new indices
   * from the right ignored. */

  OP( r0->add_ranking( r0 ), r0 );
  OP( r0->add_ranking( r1 ), r0 );
  OP( r0->add_ranking( r2 ), r0 );
  OP( r0->add_ranking( r3 ), r0 );

  OP( r1->add_ranking( r0 ), r1 );
  OP( r1->add_ranking( r1 ), r1_r1 );
  OP( r1->add_ranking( r2 ), r1 );
  OP( r1->add_ranking( r3 ), r1_r1 );
  
  OP( r2->add_ranking( r0 ), r2 );
  OP( r2->add_ranking( r1 ), r2 );
  OP( r2->add_ranking( r2 ), r2_r2 );
  OP( r2->add_ranking( r3 ), r2_r2 );

  OP( r3->add_ranking( r0 ), r3 );
  OP( r3->add_ranking( r1 ), r3_r1 );
  OP( r3->add_ranking( r2 ), r3_r2 );
  OP( r3->add_ranking( r3 ), r3_r3 );
}

void blob_tests()
{
  object bs1 = _WhiteFish.Blobs();
  object bs2 = _WhiteFish.Blobs();

  log_status("Adding words to blobs...\r");
  test++;
  for(int i=0; i < 1024; i++) {
    // Six binary words per document...
    array(string) words = sprintf("%09b", ((i*521) & 511))/3;
    bs1->add_words(i, words, 0);
    bs1->add_words(i, sprintf("%09b", i & 511)/3, 1);

    words = sprintf("%09b", ((i*523) & 511))/3;
    bs2->add_words(i, words, 0);
    bs2->add_words(i, sprintf("%09b", i & 511)/3, 1);
  }

  log_status("Reading blobs [1]...\r");
  test++;
  mapping(string:_WhiteFish.Blob) blobs1 = ([]);
  int btest = test;
  while(1) {
    array(string) word_hits = bs1->read();
    if (!word_hits[0]) {
      if (sizeof(blobs1) != 8) {
	log_msg("Test %d: Unexpected number of words in blobs #1!\n", btest);
	failed++;
      }
      break;
    }
    log_status("Blobs #1, Word: %O\r", word_hits[0]);
    test++;
    if (blobs1[word_hits[0]]) {
      log_msg("Test %d: Blob for word %O already existed!\n",
	      test, word_hits[0]);
      failed++;
    }
    // NB: Direct init.
    blobs1[word_hits[0]] = _WhiteFish.Blob(word_hits[1]);
    string probe = blobs1[word_hits[0]]->data();
    test++;
    if (word_hits[1] != probe) {
      log_msg("Test %d: probe mismatch: %O != %O\n", test, probe, word_hits[1]);
      failed++;
    }
    blobs1[word_hits[0]]->merge(probe);
  }

  log_status("Reading blobs [2]...\r");
  test++;
  mapping(string:_WhiteFish.Blob) blobs2 = ([]);
  btest = test;
  while(1) {
    array(string) word_hits = bs2->read();
    if (!word_hits[0]) {
      if (sizeof(blobs2) != 8) {
	log_msg("Test %d: Unexpected number of words in blobs #2!\n", btest);
	failed++;
      }
      break;
    }
    log_status("Blobs #2, Word: %O\r", word_hits[0]);
    test++;
    if (blobs2[word_hits[0]]) {
      log_msg("Test %d: Blob for word %O already existed!\n",
	      test, word_hits[0]);
      failed++;
    }
    // NB: Empty init followed by merge.
    blobs2[word_hits[0]] = _WhiteFish.Blob();
    blobs2[word_hits[0]]->merge(word_hits[1]);
    string probe = blobs2[word_hits[0]]->data();
    test++;
    if (word_hits[1] != probe) {
      log_msg("Test %d: probe mismatch: %O != %O\n", test, probe, word_hits[1]);
      failed++;
    }
    blobs2[word_hits[0]]->merge(probe);
  }

  log_status("Merging blob 1 and 2...\r");
  foreach(blobs1; string word; _WhiteFish.Blob b1) {
    test++;
    _WhiteFish.Blob b2 = m_delete(blobs2, word);
    if (!b2) {
      log_msg("Test %d: No blob #2 for word %O.\n", test, word);
      failed++;
      b2 = _WhiteFish.Blob();
    }
    test++;
    log_status("Merging blobs for word %O...\r", word);
    b1->merge(b2->data());
  }

  test++;
  if (sizeof(blobs2)) {
    log_msg("Test %d: Blobs #2 still contains %d unmerged words.\n",
	    test, sizeof(blobs2));
    failed++;
  }
}

int main()
{
  resultset_tests();

  blob_tests();

  Tools.Testsuite.report_result(test-failed, failed);
  return !!failed;
}
