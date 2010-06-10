
#line 1 "rl/json_number_utf8.rl"
// vim:syntax=ragel


#line 18 "rl/json_number_utf8.rl"


static char *_parse_JSON_number_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
    int cs;
    INT_TYPE d = 0;
    FLOAT_TYPE f;

    
#line 17 "json_number_utf8.c"
static const int JSON_number_start = 1;
static const int JSON_number_first_final = 6;
static const int JSON_number_error = 0;

static const int JSON_number_en_main = 1;


#line 27 "rl/json_number_utf8.rl"

    
#line 28 "json_number_utf8.c"
	{
	cs = JSON_number_start;
	}

#line 29 "rl/json_number_utf8.rl"
    
#line 35 "json_number_utf8.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 45: goto st2;
		case 46: goto tr2;
		case 48: goto st6;
	}
	if ( 49 <= (*p) && (*p) <= 57 )
		goto st10;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( (*p) == 48 )
		goto st6;
	if ( 49 <= (*p) && (*p) <= 57 )
		goto st10;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( (*p) ) {
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
	if ( 9 <= (*p) && (*p) <= 10 )
		goto tr8;
	goto st0;
tr8:
#line 10 "rl/json_number_utf8.rl"
	{
	p--; {p++; cs = 7; goto _out;}
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 90 "json_number_utf8.c"
	goto st0;
tr2:
#line 16 "rl/json_number_utf8.rl"
	{d = 1;}
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 100 "json_number_utf8.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( (*p) ) {
		case 13: goto tr8;
		case 32: goto tr8;
		case 44: goto tr8;
		case 58: goto tr8;
		case 69: goto tr9;
		case 93: goto tr8;
		case 101: goto tr9;
		case 125: goto tr8;
	}
	if ( (*p) > 10 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st8;
	} else if ( (*p) >= 9 )
		goto tr8;
	goto st0;
tr9:
#line 15 "rl/json_number_utf8.rl"
	{d = 1;}
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 132 "json_number_utf8.c"
	switch( (*p) ) {
		case 43: goto st5;
		case 45: goto st5;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st9;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st9;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 13: goto tr8;
		case 32: goto tr8;
		case 44: goto tr8;
		case 58: goto tr8;
		case 93: goto tr8;
		case 125: goto tr8;
	}
	if ( (*p) > 10 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st9;
	} else if ( (*p) >= 9 )
		goto tr8;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
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
	if ( (*p) > 10 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st10;
	} else if ( (*p) >= 9 )
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

#line 30 "rl/json_number_utf8.rl"

    if (cs >= JSON_number_first_final) {
	
		if (!state->validate) {
			PCHARP tmp = MKPCHARP(i, 0);
			if (d == 1) {
				push_float((FLOAT_TYPE)STRTOD_PCHARP(tmp, NULL));
			} else {
				struct svalue *v = Pike_sp++;
				pcharp_to_svalue_inumber(v, tmp, NULL, 10, p - i);
			}
		}

		return p;
    }

    push_int((INT_TYPE)p);
    return NULL;
}

