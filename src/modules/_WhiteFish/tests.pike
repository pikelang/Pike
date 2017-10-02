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

int main()
{
  int test;
  object r0 = _WhiteFish.ResultSet();
  object r1 = _WhiteFish.ResultSet( ({ 1,    3,    5 }));
  object r2 = _WhiteFish.ResultSet( ({    2,    4,    6 }));
  object r3 = _WhiteFish.ResultSet( ({ 1, 2, 3, 4, 5, 6 }) );


  // 1: Check size with friends.

  ENSURE( r0->size() == 0 );  ENSURE( r1->size() == 3 );
  ENSURE( r2->size() == 3 );  ENSURE( r3->size() == 6 );

  log_status("r0->memsize(): %d\n", r0->memsize());
  ENSURE( r0->memsize() );
  log_status("r1->memsize(): %d\n", r1->memsize());
  ENSURE( r0->memsize() <= r1->memsize() );
  log_status("r2->memsize(): %d\n", r2->memsize());
  ENSURE( r1->memsize() == r2->memsize() );
  log_status("r3->memsize(): %d\n", r3->memsize());
  ENSURE( r2->memsize() <= r3->memsize() );

  log_status("r0->overhead(): %d\n", r0->overhead());
  ENSURE( r0->overhead() );
  log_status("r1->overhead(): %d\n", r1->overhead());
  log_status("r2->overhead(): %d\n", r2->overhead());
  ENSURE( r1->overhead() == r2->overhead() );
  log_status("r3->overhead(): %d\n", r3->overhead());
  ENSURE( r2->overhead() > r3->overhead() );


  log_status("r1->dup()->overhead: %d\n", r1->dup()->overhead());
  ENSURE( r1->overhead() >= r1->dup()->overhead() );
  log_status("r3->dup()->overhead: %d\n", r3->dup()->overhead());
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

  Tools.Testsuite.report_result(test-failed, failed);
  return !!failed;
}
