
#line 1 "rl/json5_parser.rl"
/* vim: syntax=ragel
 */

#include "mapping.h"
#include "operators.h"
#include "object.h"
#include "array.h"
#include "builtin_functions.h"
#include "gc.h"


static ptrdiff_t _parse_JSON5(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state);

#include "json5_string_utf8.c"
#include "json5_string.c"
#include "json5_number.c"
#include "json5_array.c"
#include "json5_mapping.c"


#line 42 "rl/json5_parser.rl"


static ptrdiff_t _parse_JSON5(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    ptrdiff_t i = p;
    int cs;
    int c = 0;
    ptrdiff_t eof = pe;
    
#line 33 "json5_parser.c"
static const int JSON5_start = 1;
static const int JSON5_first_final = 20;
static const int JSON5_error = 0;

static const int JSON5_en_main = 1;


#line 50 "rl/json5_parser.rl"

    
#line 44 "json5_parser.c"
	{
	cs = JSON5_start;
	}

#line 52 "rl/json5_parser.rl"
    
#line 51 "json5_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st1;
		case 32: goto st1;
		case 34: goto tr2;
		case 39: goto tr2;
		case 43: goto tr3;
		case 47: goto st6;
		case 73: goto tr3;
		case 78: goto tr3;
		case 91: goto tr5;
		case 102: goto st10;
		case 110: goto st14;
		case 116: goto st17;
		case 123: goto tr9;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 45 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr3;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto st1;
	goto st0;
st0:
cs = 0;
	goto _out;
tr2:
#line 26 "rl/json5_parser.rl"
	{ PARSE_STRING(p); {p = (( i))-1;} }
	goto st20;
tr3:
#line 27 "rl/json5_parser.rl"
	{ PARSE(number, p); {p = (( i))-1;} }
	goto st20;
tr5:
#line 29 "rl/json5_parser.rl"
	{ PARSE(array, p); {p = (( i))-1;} }
	goto st20;
tr9:
#line 28 "rl/json5_parser.rl"
	{ PARSE(mapping, p); {p = (( i))-1;} }
	goto st20;
tr20:
#line 31 "rl/json5_parser.rl"
	{ PUSH_SPECIAL(false); }
	goto st20;
tr23:
#line 32 "rl/json5_parser.rl"
	{ PUSH_SPECIAL(null); }
	goto st20;
tr26:
#line 30 "rl/json5_parser.rl"
	{ PUSH_SPECIAL(true); }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 117 "json5_parser.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st20;
		case 32: goto st20;
		case 47: goto st2;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st20;
	goto st0;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st3;
		case 47: goto st5;
	}
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st4;
	goto st3;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st4;
		case 47: goto st20;
	}
	goto st3;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st20;
	goto st5;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st7;
		case 47: goto st9;
	}
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st8;
	goto st7;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st8;
		case 47: goto st1;
	}
	goto st7;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st1;
	goto st9;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 97 )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 108 )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 115 )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 101 )
		goto tr20;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 117 )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 108 )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 108 )
		goto tr23;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 114 )
		goto st18;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 117 )
		goto st19;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 101 )
		goto tr26;
	goto st0;
	}
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
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
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 53 "rl/json5_parser.rl"

    if (cs >= JSON5_first_final) {
	return p;
    }

    if ( (state->flags & JSON5_FIRST_VALUE) && (c==1) )
    {
      state->flags &= ~JSON5_ERROR;
      return p-1;
    }

    if (!(state->flags&JSON5_VALIDATE) && c > 0) pop_n_elems(c);

    state->flags |= JSON5_ERROR;
    return p;
}
