
#line 1 "rl/json5_number.rl"
/* vim:syntax=ragel
 */


#line 24 "rl/json5_number.rl"


static ptrdiff_t _parse_JSON5_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    ptrdiff_t i = p;
    ptrdiff_t eof = pe;
    int cs;
    int d = 0, h = 0, s = 0;

    
#line 18 "json5_number.c"
static const int JSON5_number_start = 24;
static const int JSON5_number_first_final = 24;
static const int JSON5_number_error = 0;

static const int JSON5_number_en_main = 24;


#line 33 "rl/json5_number.rl"

    
#line 29 "json5_number.c"
	{
	cs = JSON5_number_start;
	}

#line 35 "rl/json5_number.rl"
    
#line 36 "json5_number.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 24:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 46: goto tr27;
		case 47: goto tr28;
		case 48: goto st31;
		case 58: goto tr26;
		case 69: goto tr31;
		case 73: goto tr32;
		case 78: goto tr33;
		case 93: goto tr26;
		case 101: goto tr31;
		case 125: goto tr26;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 43 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto tr24;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 45 ) {
		if ( 49 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st32;
	} else
		goto st26;
	goto st0;
st0:
cs = 0;
	goto _out;
tr24:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 25; goto _out;}
    }
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 81 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st25;
		case 32: goto st25;
		case 47: goto st1;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st25;
	goto st0;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st2;
		case 47: goto st4;
	}
	goto st0;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st3;
	goto st2;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st3;
		case 47: goto st25;
	}
	goto st2;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st25;
	goto st4;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 46: goto tr27;
		case 47: goto tr28;
		case 48: goto st31;
		case 58: goto tr26;
		case 69: goto tr31;
		case 73: goto tr32;
		case 78: goto tr33;
		case 93: goto tr26;
		case 101: goto tr31;
		case 125: goto tr26;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 49 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st32;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr24;
	goto st0;
tr26:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 27; goto _out;}
    }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 158 "json5_number.c"
	goto st0;
tr27:
#line 20 "rl/json5_number.rl"
	{d = 1;}
	goto st28;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
#line 168 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 47: goto tr28;
		case 58: goto tr26;
		case 69: goto tr31;
		case 93: goto tr26;
		case 101: goto tr31;
		case 125: goto tr26;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st28;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr24;
	goto st0;
tr28:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 29; goto _out;}
    }
	goto st29;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
#line 197 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st2;
		case 47: goto st4;
	}
	goto st0;
tr31:
#line 18 "rl/json5_number.rl"
	{d = 1;}
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 211 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 43: goto st6;
		case 45: goto st6;
	}
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st30;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st30;
	goto st0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 47: goto tr28;
		case 58: goto tr26;
		case 93: goto tr26;
		case 125: goto tr26;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st30;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr24;
	goto st0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 46: goto tr27;
		case 47: goto tr28;
		case 58: goto tr26;
		case 69: goto tr31;
		case 88: goto tr36;
		case 93: goto tr26;
		case 101: goto tr31;
		case 120: goto tr36;
		case 125: goto tr26;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st32;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr24;
	goto st0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 46: goto tr27;
		case 47: goto tr28;
		case 58: goto tr26;
		case 69: goto tr31;
		case 93: goto tr26;
		case 101: goto tr31;
		case 125: goto tr26;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st32;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr24;
	goto st0;
tr36:
#line 19 "rl/json5_number.rl"
	{h = 1;}
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 299 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st33;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto st33;
	} else
		goto st33;
	goto st0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr37;
		case 32: goto tr37;
		case 44: goto tr38;
		case 47: goto tr39;
		case 58: goto tr38;
		case 93: goto tr38;
		case 125: goto tr38;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 48 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto tr37;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 57 ) {
		if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
				goto st33;
		} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 65 )
			goto st33;
	} else
		goto st33;
	goto st0;
tr37:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 34; goto _out;}
    }
	goto st34;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
#line 345 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr37;
		case 32: goto tr37;
		case 44: goto tr26;
		case 47: goto tr40;
		case 58: goto tr26;
		case 93: goto tr26;
		case 125: goto tr26;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr37;
	goto st0;
tr40:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 35; goto _out;}
    }
	goto st35;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
#line 369 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st8;
		case 47: goto st10;
	}
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st9;
	goto st8;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st9;
		case 47: goto st34;
	}
	goto st8;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st34;
	goto st10;
tr38:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 36; goto _out;}
    }
	goto st36;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
#line 409 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 44: goto tr26;
		case 47: goto tr28;
		case 58: goto tr26;
		case 93: goto tr26;
		case 125: goto tr26;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr24;
	goto st0;
tr39:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 37; goto _out;}
    }
	goto st37;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
#line 433 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr24;
		case 32: goto tr24;
		case 42: goto st8;
		case 44: goto tr26;
		case 47: goto tr41;
		case 58: goto tr26;
		case 93: goto tr26;
		case 125: goto tr26;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr24;
	goto st0;
tr41:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 38; goto _out;}
    }
	goto st38;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
#line 458 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 10: goto st34;
		case 42: goto st11;
	}
	goto st10;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 10: goto st39;
		case 42: goto st13;
	}
	goto st11;
tr42:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 39; goto _out;}
    }
	goto st39;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
#line 484 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr42;
		case 32: goto tr42;
		case 42: goto st3;
		case 44: goto tr43;
		case 47: goto tr44;
		case 58: goto tr43;
		case 93: goto tr43;
		case 125: goto tr43;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr42;
	goto st2;
tr43:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 40; goto _out;}
    }
	goto st40;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
#line 509 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st3;
	goto st2;
tr44:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 41; goto _out;}
    }
	goto st41;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
#line 524 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st12;
		case 47: goto st11;
	}
	goto st2;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st9;
		case 47: goto st25;
	}
	goto st8;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 10: goto st39;
		case 42: goto st13;
		case 47: goto st42;
	}
	goto st11;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 9: goto st42;
		case 10: goto st34;
		case 13: goto st42;
		case 32: goto st42;
		case 47: goto st14;
	}
	goto st10;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 10: goto st34;
		case 42: goto st11;
	}
	goto st10;
tr32:
#line 21 "rl/json5_number.rl"
	{s = 1;}
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 578 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) == 110 )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 102 )
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 105 )
		goto st18;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 110 )
		goto st19;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 105 )
		goto st20;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 116 )
		goto st21;
	goto st0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 121 )
		goto st36;
	goto st0;
tr33:
#line 21 "rl/json5_number.rl"
	{s = 1;}
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 632 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) == 97 )
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 78 )
		goto st36;
	goto st0;
	}
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof32: cs = 32; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof33: cs = 33; goto _test_eof; 
	_test_eof34: cs = 34; goto _test_eof; 
	_test_eof35: cs = 35; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof37: cs = 37; goto _test_eof; 
	_test_eof38: cs = 38; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof39: cs = 39; goto _test_eof; 
	_test_eof40: cs = 40; goto _test_eof; 
	_test_eof41: cs = 41; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof42: cs = 42; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 36 "rl/json5_number.rl"
    if (cs >= JSON5_number_first_final) {
	if (!(state->flags&JSON5_VALIDATE)) {
            if (s == 1) {
               s = (int)INDEX_PCHARP(str, i);
               if(s == '+') s = 0, i++; 
               else if(s == '-') s = -1, i++;
               // -symbol
               if(s == -1) {
                  s = (int)INDEX_PCHARP(str, i);
                  if(s == 'I') {
                    push_float(-MAKE_INF());
                  } else if(s == 'N') {
                    push_float(MAKE_NAN()); /* note sign on -NaN has no meaning */
                  } else { /* should never get here */
                    state->flags |= JSON5_ERROR; 
                    return p;
                  }
               } else {
                 /* if we had a + sign, look at the next digit, otherwise use the value. */
		 if(s == 0) 
                   s = (int)INDEX_PCHARP(str, i);
                   
                 if(s == 'I') {
                    push_float(MAKE_INF());
                 } else if(s == 'N') {
                   push_float(MAKE_NAN()); /* note sign on -NaN has no meaning */
                 } else { /* should never get here */
                   state->flags |= JSON5_ERROR; 
                   return p;
                 }
              }
            } else if (h == 1) {
              // TODO handle errors better, handle possible bignums and possible better approach.
              push_string(make_shared_binary_pcharp(ADD_PCHARP(str, i), p-i));
              push_text("%x");
              f_sscanf(2);
	      if((PIKE_TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY) || (Pike_sp[-1].u.array->size != 1)) {
                state->flags |= JSON5_ERROR;
                return p;
              }
              else {
                 push_int(ITEM(Pike_sp[-1].u.array)[0].u.integer);
	         stack_swap();
		 pop_stack();
              }
	    } else if (d == 1) {
		push_float((FLOAT_TYPE)STRTOD_PCHARP(ADD_PCHARP(str, i), NULL));
	    } else {
		pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 10, p - i);
	    }
	}

	return p;
    }
    state->flags |= JSON5_ERROR;
    return p;
}

