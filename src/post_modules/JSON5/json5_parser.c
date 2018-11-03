
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
	int c=0;
	ptrdiff_t eof = pe;
	
	static const int JSON5_start = 1;
	static const int JSON5_first_final = 20;
	static const int JSON5_error = 0;
	
	static const int JSON5_en_main = 1;
	
	static const char _JSON5_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON5_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0
	};
	
	static const char _JSON5_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON5_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 50 "rl/json5_parser.rl"
	
	
	
	{
		cs = (int)JSON5_start;
	}
	
	#line 52 "rl/json5_parser.rl"
	
	
	{
		if ( p == pe )
		goto _test_eof;
		switch ( cs )
		{
			case 1:
			goto st_case_1;
			case 0:
			goto st_case_0;
			case 20:
			goto st_case_20;
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
			case 14:
			goto st_case_14;
			case 15:
			goto st_case_15;
			case 16:
			goto st_case_16;
			case 17:
			goto st_case_17;
			case 18:
			goto st_case_18;
			case 19:
			goto st_case_19;
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
			case 39: {
				goto ctr2;
			}
			case 43: {
				goto ctr3;
			}
			case 47: {
				goto st6;
			}
			case 73: {
				goto ctr3;
			}
			case 78: {
				goto ctr3;
			}
			case 91: {
				goto ctr5;
			}
			case 102: {
				goto st10;
			}
			case 110: {
				goto st14;
			}
			case 116: {
				goto st17;
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
			#line 26 "rl/json5_parser.rl"
			PARSE_STRING(p); {p = (( i))-1;}
		}
		
		goto st20;
		ctr3:
		{
			#line 27 "rl/json5_parser.rl"
			PARSE(number, p); {p = (( i))-1;}
		}
		
		goto st20;
		ctr5:
		{
			#line 29 "rl/json5_parser.rl"
			PARSE(array, p); {p = (( i))-1;}
		}
		
		goto st20;
		ctr9:
		{
			#line 28 "rl/json5_parser.rl"
			PARSE(mapping, p); {p = (( i))-1;}
		}
		
		goto st20;
		ctr20:
		{
			#line 31 "rl/json5_parser.rl"
			PUSH_SPECIAL(false); }
		
		goto st20;
		ctr23:
		{
			#line 32 "rl/json5_parser.rl"
			PUSH_SPECIAL(null); }
		
		goto st20;
		ctr26:
		{
			#line 30 "rl/json5_parser.rl"
			PUSH_SPECIAL(true); }
		
		goto st20;
		st20:
		p+= 1;
		if ( p == pe )
		goto _test_eof20;
		st_case_20:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st20;
			}
			case 32: {
				goto st20;
			}
			case 47: {
				goto st2;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st20;
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
				goto st3;
			}
			case 47: {
				goto st5;
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
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st4;
		}
		{
			goto st3;
		}
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st4;
			}
			case 47: {
				goto st20;
			}
		}
		{
			goto st3;
		}
		st5:
		p+= 1;
		if ( p == pe )
		goto _test_eof5;
		st_case_5:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st20;
		}
		{
			goto st5;
		}
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st7;
			}
			case 47: {
				goto st9;
			}
		}
		{
			goto st0;
		}
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st8;
		}
		{
			goto st7;
		}
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st8;
			}
			case 47: {
				goto st1;
			}
		}
		{
			goto st7;
		}
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st1;
		}
		{
			goto st9;
		}
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		if ( (((int)INDEX_PCHARP(str, p))) == 97 ) {
			goto st11;
		}
		{
			goto st0;
		}
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		if ( (((int)INDEX_PCHARP(str, p))) == 108 ) {
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
		if ( (((int)INDEX_PCHARP(str, p))) == 115 ) {
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
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto st15;
		}
		{
			goto st0;
		}
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		if ( (((int)INDEX_PCHARP(str, p))) == 108 ) {
			goto st16;
		}
		{
			goto st0;
		}
		st16:
		p+= 1;
		if ( p == pe )
		goto _test_eof16;
		st_case_16:
		if ( (((int)INDEX_PCHARP(str, p))) == 108 ) {
			goto ctr23;
		}
		{
			goto st0;
		}
		st17:
		p+= 1;
		if ( p == pe )
		goto _test_eof17;
		st_case_17:
		if ( (((int)INDEX_PCHARP(str, p))) == 114 ) {
			goto st18;
		}
		{
			goto st0;
		}
		st18:
		p+= 1;
		if ( p == pe )
		goto _test_eof18;
		st_case_18:
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto st19;
		}
		{
			goto st0;
		}
		st19:
		p+= 1;
		if ( p == pe )
		goto _test_eof19;
		st_case_19:
		if ( (((int)INDEX_PCHARP(str, p))) == 101 ) {
			goto ctr26;
		}
		{
			goto st0;
		}
		st_out:
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
