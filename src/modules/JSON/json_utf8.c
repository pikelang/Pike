
#line 1 "rl/json_utf8.rl"
// vim:syntax=ragel

static char *_parse_JSON_utf8(char *p, char *pe, struct parser_state *state); 

#include "json_string_utf8.c"
#include "json_number_utf8.c"
#include "json_array_utf8.c"
#include "json_mapping_utf8.c"


#line 32 "rl/json_utf8.rl"


static char *_parse_JSON_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
    int cs;
    int c = 0;

    
#line 23 "json_utf8.c"
static const int JSON_start = 1;
static const int JSON_first_final = 12;
static const int JSON_error = 0;

static const int JSON_en_main = 1;


#line 40 "rl/json_utf8.rl"

    
#line 34 "json_utf8.c"
	{
	cs = JSON_start;
	}

#line 42 "rl/json_utf8.rl"
    
#line 41 "json_utf8.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 13: goto st1;
		case 32: goto st1;
		case 34: goto tr2;
		case 43: goto tr3;
		case 91: goto tr4;
		case 102: goto st2;
		case 110: goto st6;
		case 116: goto st9;
		case 123: goto tr8;
	}
	if ( (*p) < 45 ) {
		if ( 9 <= (*p) && (*p) <= 10 )
			goto st1;
	} else if ( (*p) > 46 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr3;
	} else
		goto tr3;
	goto st0;
st0:
cs = 0;
	goto _out;
tr2:
#line 15 "rl/json_utf8.rl"
	{ PARSE(string_utf8, p); {p = (( i))-1;} }
	goto st12;
tr3:
#line 16 "rl/json_utf8.rl"
	{ PARSE(number_utf8, p); {p = (( i))-1;} }
	goto st12;
tr4:
#line 18 "rl/json_utf8.rl"
	{ PARSE(array_utf8, p); {p = (( i))-1;} }
	goto st12;
tr8:
#line 17 "rl/json_utf8.rl"
	{ PARSE(mapping_utf8, p); {p = (( i))-1;} }
	goto st12;
tr12:
#line 21 "rl/json_utf8.rl"
	{ PUSH_SPECIAL("false"); }
	goto st12;
tr15:
#line 22 "rl/json_utf8.rl"
	{ PUSH_SPECIAL("null"); }
	goto st12;
tr18:
#line 20 "rl/json_utf8.rl"
	{ PUSH_SPECIAL("true"); }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 106 "json_utf8.c"
	switch( (*p) ) {
		case 13: goto st12;
		case 32: goto st12;
	}
	if ( 9 <= (*p) && (*p) <= 10 )
		goto st12;
	goto st0;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( (*p) == 97 )
		goto st3;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 108 )
		goto st4;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 115 )
		goto st5;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 101 )
		goto tr12;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 117 )
		goto st7;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 108 )
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 108 )
		goto tr15;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 114 )
		goto st10;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( (*p) == 117 )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 101 )
		goto tr18;
	goto st0;
	}
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 43 "rl/json_utf8.rl"

    if (cs >= JSON_first_final) {
	return p;
    }

    if (!state->validate && c > 0) pop_n_elems(c);

    push_int((INT_TYPE)p);
    return NULL;
}
