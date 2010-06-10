
#line 1 "rl/json_number.rl"
// vim:syntax=ragel


#line 19 "rl/json_number.rl"


static ptrdiff_t _parse_JSON_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    ptrdiff_t i = p;
    int cs;
    int d = 0;

    
#line 16 "json_number.c"
static const int JSON_number_start = 1;
static const int JSON_number_first_final = 6;
static const int JSON_number_error = 0;

static const int JSON_number_en_main = 1;


#line 27 "rl/json_number.rl"

    
#line 27 "json_number.c"
	{
	cs = JSON_number_start;
	}

#line 29 "rl/json_number.rl"
    
#line 34 "json_number.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 45: goto st2;
		case 46: goto tr2;
		case 48: goto st6;
	}
	if ( 49 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st10;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 48 )
		goto st6;
	if ( 49 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st10;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr8;
		case 32: goto tr8;
		case 44: goto tr8;
		case 46: goto tr2;
		case 58: goto tr8;
		case 69: goto tr9;
		case 93: goto tr8;
		case 101: goto tr9;
		case 125: goto tr8;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto tr8;
	goto st0;
tr8:
#line 10 "rl/json_number.rl"
	{
		p--; {p++; cs = 7; goto _out;}
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 89 "json_number.c"
	goto st0;
tr2:
#line 17 "rl/json_number.rl"
	{d = 1;}
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 99 "json_number.c"
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr8;
		case 32: goto tr8;
		case 44: goto tr8;
		case 58: goto tr8;
		case 69: goto tr9;
		case 93: goto tr8;
		case 101: goto tr9;
		case 125: goto tr8;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st8;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr8;
	goto st0;
tr9:
#line 16 "rl/json_number.rl"
	{d = 1;}
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 131 "json_number.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 43: goto st5;
		case 45: goto st5;
	}
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st9;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
		goto st9;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr8;
		case 32: goto tr8;
		case 44: goto tr8;
		case 58: goto tr8;
		case 93: goto tr8;
		case 125: goto tr8;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st9;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr8;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto tr8;
		case 32: goto tr8;
		case 44: goto tr8;
		case 46: goto tr2;
		case 58: goto tr8;
		case 69: goto tr9;
		case 93: goto tr8;
		case 101: goto tr9;
		case 125: goto tr8;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto st10;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto tr8;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 30 "rl/json_number.rl"

    if (cs >= JSON_number_first_final) {
		if (!(state->flags&JSON_VALIDATE)) {
			if (d == 1) {
				push_float((FLOAT_TYPE)STRTOD_PCHARP(ADD_PCHARP(str, i), NULL));
			} else {
				pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 10, p - i + 1);
			}
		}

		return p;
    }

    state->flags |= JSON_ERROR;
    return p;
}

