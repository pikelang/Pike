
#line 1 "rl/json_number.rl"
// vim:syntax=ragel

#include <stdio.h>
#include "global.h"


#line 21 "rl/json_number.rl"


static p_wchar2 *_parse_JSON_number(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 *i = p;
    int cs;
    int d = 0;

    
#line 19 "json_number.c"
static const int JSON_number_start = 1;
static const int JSON_number_first_final = 6;
static const int JSON_number_error = 0;

static const int JSON_number_en_main = 1;


#line 29 "rl/json_number.rl"

    
#line 30 "json_number.c"
	{
	cs = JSON_number_start;
	}

#line 31 "rl/json_number.rl"
    
#line 37 "json_number.c"
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
#line 13 "rl/json_number.rl"
	{
		p--; {p++; cs = 7; goto _out;}
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 92 "json_number.c"
	goto st0;
tr2:
#line 19 "rl/json_number.rl"
	{d = 1;}
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 102 "json_number.c"
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
#line 18 "rl/json_number.rl"
	{d = 1;}
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 134 "json_number.c"
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

#line 32 "rl/json_number.rl"

    if (cs >= JSON_number_first_final) {
		if (!state->validate) {
			PCHARP tmp = MKPCHARP(i, 2);
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

