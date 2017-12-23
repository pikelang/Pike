
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
static const int JSON5_number_start = 21;
static const int JSON5_number_first_final = 21;
static const int JSON5_number_error = 0;

static const int JSON5_number_en_main = 21;


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
case 21:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 46: goto tr23;
		case 47: goto tr24;
		case 48: goto st27;
		case 58: goto tr22;
		case 69: goto tr27;
		case 73: goto tr28;
		case 78: goto tr29;
		case 93: goto tr22;
		case 101: goto tr27;
		case 125: goto tr22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 43 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto tr20;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 45 ) {
		if ( 49 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st28;
	} else
		goto st23;
	goto st0;
st0:
cs = 0;
	goto _out;
tr20:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 22; goto _out;}
    }
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 81 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st22;
		case 32: goto st22;
		case 47: goto st1;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st22;
	goto st0;
tr24:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 1; goto _out;}
    }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 101 "json5_number.c"
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
		case 47: goto st22;
	}
	goto st2;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st22;
	goto st4;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 46: goto tr23;
		case 47: goto tr24;
		case 48: goto st27;
		case 58: goto tr22;
		case 69: goto tr27;
		case 73: goto tr28;
		case 78: goto tr29;
		case 93: goto tr22;
		case 101: goto tr27;
		case 125: goto tr22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 49 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st28;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr20;
	goto st0;
tr22:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 24; goto _out;}
    }
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 166 "json5_number.c"
	goto st0;
tr23:
#line 20 "rl/json5_number.rl"
	{d = 1;}
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 176 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 47: goto tr24;
		case 58: goto tr22;
		case 69: goto tr27;
		case 93: goto tr22;
		case 101: goto tr27;
		case 125: goto tr22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st25;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr20;
	goto st0;
tr27:
#line 18 "rl/json5_number.rl"
	{d = 1;}
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 202 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 43: goto st6;
		case 45: goto st6;
	}
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st26;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st26;
	goto st0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 47: goto tr24;
		case 58: goto tr22;
		case 93: goto tr22;
		case 125: goto tr22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st26;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr20;
	goto st0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 46: goto tr23;
		case 47: goto tr24;
		case 58: goto tr22;
		case 69: goto tr27;
		case 88: goto tr32;
		case 93: goto tr22;
		case 101: goto tr27;
		case 120: goto tr32;
		case 125: goto tr22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st28;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr20;
	goto st0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 46: goto tr23;
		case 47: goto tr24;
		case 58: goto tr22;
		case 69: goto tr27;
		case 93: goto tr22;
		case 101: goto tr27;
		case 125: goto tr22;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st28;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr20;
	goto st0;
tr32:
#line 19 "rl/json5_number.rl"
	{h = 1;}
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 290 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st29;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto st29;
	} else
		goto st29;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr33;
		case 32: goto tr33;
		case 44: goto tr34;
		case 47: goto tr35;
		case 58: goto tr34;
		case 93: goto tr34;
		case 125: goto tr34;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 48 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto tr33;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 57 ) {
		if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
				goto st29;
		} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 65 )
			goto st29;
	} else
		goto st29;
	goto st0;
tr33:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 30; goto _out;}
    }
	goto st30;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
#line 336 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr33;
		case 32: goto tr33;
		case 44: goto tr22;
		case 47: goto tr35;
		case 58: goto tr22;
		case 93: goto tr22;
		case 125: goto tr22;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr33;
	goto st0;
tr35:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 8; goto _out;}
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 360 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st9;
		case 47: goto st11;
	}
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st10;
	goto st9;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st10;
		case 47: goto st30;
	}
	goto st9;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st30;
	goto st11;
tr34:
#line 11 "rl/json5_number.rl"
	{
      p--;
      {p++; cs = 31; goto _out;}
    }
	goto st31;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
#line 400 "json5_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr20;
		case 32: goto tr20;
		case 44: goto tr22;
		case 47: goto tr24;
		case 58: goto tr22;
		case 93: goto tr22;
		case 125: goto tr22;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr20;
	goto st0;
tr28:
#line 21 "rl/json5_number.rl"
	{s = 1;}
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 421 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) == 110 )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 102 )
		goto st14;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 105 )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 110 )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 105 )
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 116 )
		goto st18;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 121 )
		goto st31;
	goto st0;
tr29:
#line 21 "rl/json5_number.rl"
	{s = 1;}
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 475 "json5_number.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) == 97 )
		goto st20;
	goto st0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 78 )
		goto st31;
	goto st0;
	}
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof24: cs = 24; goto _test_eof; 
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 36 "rl/json5_number.rl"
    if (cs >= JSON5_number_first_final) {
	if (!(state->flags&JSON5_VALIDATE)) {
            if (s == 1) {
               push_string(make_shared_binary_pcharp(ADD_PCHARP(str, i), p-i));
            } else if (h == 1) {
              // TODO handle errors better, handle possible bignums and possible better approach.
              push_string(make_shared_binary_pcharp(ADD_PCHARP(str, i), p-i));
              push_text("%x");
              f_sscanf(2);
	      if(PIKE_TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY)
                printf("not an array\n");
	      else if(Pike_sp[-1].u.array->size != 1)
                 printf("not the right number of elements!\n");
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

