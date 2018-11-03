
#line 1 "rl/json_parser.rl"
/* vim: syntax=ragel
*/

#include "mapping.h"
#include "operators.h"
#include "object.h"
#include "array.h"
#include "builtin_functions.h"
#include "gc.h"


static ptrdiff_t _parse_JSON(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state);

#include "json_string_utf8.c"
#include "json_string.c"
#include "json_number.c"
#include "json_array.c"
#include "json_mapping.c"


#line 43 "rl/json_parser.rl"


static ptrdiff_t _parse_JSON(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	ptrdiff_t i = p;
	int cs;
	int c=0;
	
	static const int JSON_start = 1;
	static const int JSON_first_final = 14;
	static const int JSON_error = 0;
	
	static const int JSON_en_main = 1;
	
	static const char _JSON_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static const char _JSON_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 50 "rl/json_parser.rl"
	
	
	
	{
		cs = (int)JSON_start;
	}
	
	#line 52 "rl/json_parser.rl"
	
	
	{
		if ( p == pe )
		goto _test_eof;
		switch ( cs )
		{
			case 1:
			goto st_case_1;
			case 0:
			goto st_case_0;
			case 14:
			goto st_case_14;
			case 2:
			goto st_case_2;
			case 3:
			goto st_case_3;
			case 4:
			goto st_case_4;
			case 5:
			goto st_case_5;
			case 6:
			goto st_case_6;
			case 7:
			goto st_case_7;
			case 8:
			goto st_case_8;
			case 9:
			goto st_case_9;
			case 10:
			goto st_case_10;
			case 11:
			goto st_case_11;
			case 12:
			goto st_case_12;
			case 13:
			goto st_case_13;
		}
		goto st_out;
		st1:
		p+= 1;
		if ( p == pe )
		goto _test_eof1;
		st_case_1:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st1;
			}
			case 32: {
				goto st1;
			}
			case 34: {
				goto ctr2;
			}
			case 43: {
				goto ctr3;
			}
			case 47: {
				goto st3;
			}
			case 91: {
				goto ctr5;
			}
			case 102: {
				goto st4;
			}
			case 110: {
				goto st8;
			}
			case 116: {
				goto st11;
			}
			case 123: {
				goto ctr9;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 45 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr3;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto st1;
		}
		{
			goto st0;
		}
		st_case_0:
		st0:
		cs = 0;
		goto _out;
		ctr2:
		{
			#line 26 "rl/json_parser.rl"
			PARSE_STRING(p); {p = (( i))-1;}
		}
		
		goto st14;
		ctr3:
		{
			#line 27 "rl/json_parser.rl"
			PARSE(number, p); {p = (( i))-1;}
		}
		
		goto st14;
		ctr5:
		{
			#line 29 "rl/json_parser.rl"
			PARSE(array, p); {p = (( i))-1;}
		}
		
		goto st14;
		ctr9:
		{
			#line 28 "rl/json_parser.rl"
			PARSE(mapping, p); {p = (( i))-1;}
		}
		
		goto st14;
		ctr14:
		{
			#line 32 "rl/json_parser.rl"
			PUSH_SPECIAL(false, push_int(0)); }
		
		goto st14;
		ctr17:
		{
			#line 33 "rl/json_parser.rl"
			PUSH_SPECIAL(null, push_undefined()); }
		
		goto st14;
		ctr20:
		{
			#line 31 "rl/json_parser.rl"
			PUSH_SPECIAL(true, push_int(1)); }
		
		goto st14;
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st14;
			}
			case 32: {
				goto st14;
			}
			case 47: {
				goto st2;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st14;
		}
		{
			goto st0;
		}
		st2:
		p+= 1;
		if ( p == pe )
		goto _test_eof2;
		st_case_2:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st14;
			}
			case 47: {
				goto st14;
			}
		}
		{
			goto st0;
		}
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st1;
			}
			case 47: {
				goto st1;
			}
		}
		{
			goto st0;
		}
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		if ( (((int)INDEX_PCHARP(str, p))) == 97 ) {
			goto st5;
		}
		{
			goto st0;
		}
		st5:
		p+= 1;
		if ( p == pe )
		goto _test_eof5;
		st_case_5:
		if ( (((int)INDEX_PCHARP(str, p))) == 108 ) {
			goto st6;
		}
		{
			goto st0;
		}
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		if ( (((int)INDEX_PCHARP(str, p))) == 115 ) {
			goto st7;
		}
		{
			goto st0;
		}
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		if ( (((int)INDEX_PCHARP(str, p))) == 101 ) {
			goto ctr14;
		}
		{
			goto st0;
		}
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto st9;
		}
		{
			goto st0;
		}
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		if ( (((int)INDEX_PCHARP(str, p))) == 108 ) {
			goto st10;
		}
		{
			goto st0;
		}
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		if ( (((int)INDEX_PCHARP(str, p))) == 108 ) {
			goto ctr17;
		}
		{
			goto st0;
		}
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		if ( (((int)INDEX_PCHARP(str, p))) == 114 ) {
			goto st12;
		}
		{
			goto st0;
		}
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto st13;
		}
		{
			goto st0;
		}
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		if ( (((int)INDEX_PCHARP(str, p))) == 101 ) {
			goto ctr20;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof1: cs = 1; goto _test_eof; 
		_test_eof14: cs = 14; goto _test_eof; 
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
		
		_test_eof: {}
		_out: {}
	}
	
	#line 53 "rl/json_parser.rl"
	
	
	if (cs >= JSON_first_final) {
		return p;
	}
	
	if ( (state->flags & JSON_FIRST_VALUE) && (c==1) )
	{
		state->flags &= ~JSON_ERROR;
		return p-1;
	}
	
	if (!(state->flags&JSON_VALIDATE) && c > 0) pop_n_elems(c);
	
	state->flags |= JSON_ERROR;
	return p;
}
