
#line 1 "rl/json_string.rl"
/* -*- mode: C; c-basic-offset: 4; -*-
* vim:syntax=ragel
*/
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 99 "rl/json_string.rl"


static ptrdiff_t _parse_JSON_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	int hexchr0 = 0, hexchr1 = 0;
	ptrdiff_t start = p, mark=0;
	struct string_builder s;
	int cs;
	ONERROR handle;
	const int validate = !(state->flags&JSON_VALIDATE);
	
	
	static const int JSON_string_start = 1;
	static const int JSON_string_first_final = 15;
	static const int JSON_string_error = 0;
	
	static const int JSON_string_en_main = 1;
	static const int JSON_string_en_main_hex1 = 9;
	
	static const char _JSON_string_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON_string_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0
	};
	
	static const char _JSON_string_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON_string_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 109 "rl/json_string.rl"
	
	
	if (validate) {
		init_string_builder(&s, 0);
		SET_ONERROR (handle, free_string_builder, &s);
	}
	
	
	{
		cs = (int)JSON_string_start;
	}
	
	#line 116 "rl/json_string.rl"
	
	
	{
		if ( p == pe )
		goto _test_eof;
		goto _resume;
		
		_again:
		switch ( cs ) {
			case 1: goto st1;
			case 0: goto st0;
			case 2: goto st2;
			case 3: goto st3;
			case 15: goto st15;
			case 4: goto st4;
			case 5: goto st5;
			case 6: goto st6;
			case 7: goto st7;
			case 8: goto st8;
			case 9: goto st9;
			case 10: goto st10;
			case 11: goto st11;
			case 12: goto st12;
			case 13: goto st13;
			case 14: goto st14;
		}
		
		_resume:
		switch ( cs )
		{
			case 1:
			goto st_case_1;
			case 0:
			goto st_case_0;
			case 2:
			goto st_case_2;
			case 3:
			goto st_case_3;
			case 15:
			goto st_case_15;
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
		}
		goto st_out;
		st1:
		p+= 1;
		if ( p == pe )
		goto _test_eof1;
		st_case_1:
		if ( (((int)INDEX_PCHARP(str, p))) == 34 ) {
			goto st2;
		}
		{
			goto st0;
		}
		st_case_0:
		st0:
		cs = 0;
		goto _out;
		st2:
		p+= 1;
		if ( p == pe )
		goto _test_eof2;
		st_case_2:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto ctr3;
			}
			case 92: {
				goto ctr4;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto ctr2;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto ctr2;
		}
		{
			goto st0;
		}
		ctr2:
		{
			#line 71 "rl/json_string.rl"
			
			mark = p;
		}
		
		goto st3;
		ctr8:
		{
			#line 57 "rl/json_string.rl"
			
			if (validate) switch(((((int)INDEX_PCHARP(str, p))))) {
				case '\'':
				case '"':
				case '/':
				case '\\':      string_builder_putchar(&s, ((((int)INDEX_PCHARP(str, p))))); break;
				case 'b':       string_builder_putchar(&s, '\b'); break;
				case 'f':       string_builder_putchar(&s, '\f'); break;
				case 'n':       string_builder_putchar(&s, '\n'); break;
				case 'r':       string_builder_putchar(&s, '\r'); break;
				case 't':       string_builder_putchar(&s, '\t'); break;
			}
		}
		{
			#line 75 "rl/json_string.rl"
			mark = p + 1; }
		
		goto st3;
		ctr13:
		cs = 3;
		{
			#line 15 "rl/json_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		{
			#line 20 "rl/json_string.rl"
			
			if (IS_HIGH_SURROGATE (hexchr0)) {
				/* Chars outside the BMP can be expressed as two hex
				* escapes that codes a surrogate pair, so see if we can
				* read a second one. */
				cs = 9;}
			else {
				if (IS_NUNICODE(hexchr0)) {
					p--; {p+= 1; goto _out;}
				}
				if (validate) {
					string_builder_putchar(&s, hexchr0);
				}
			}
		}
		{
			#line 75 "rl/json_string.rl"
			mark = p + 1; }
		
		goto _again;
		ctr19:
		{
			#line 41 "rl/json_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		{
			#line 46 "rl/json_string.rl"
			
			if (!IS_LOW_SURROGATE (hexchr1)) {
				p--; {p+= 1; cs = 3; goto _out;}
			}
			if (validate) {
				int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
				0x10000;
				string_builder_putchar(&s, cp);
			}
		}
		{
			#line 75 "rl/json_string.rl"
			mark = p + 1; }
		
		goto st3;
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto ctr6;
			}
			case 92: {
				goto ctr7;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto st3;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto st3;
		}
		{
			goto st0;
		}
		ctr3:
		{
			#line 71 "rl/json_string.rl"
			
			mark = p;
		}
		{
			#line 77 "rl/json_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st15;
		ctr6:
		{
			#line 77 "rl/json_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st15;
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		{
			#line 97 "rl/json_string.rl"
			p--; {p+= 1; cs = 15; goto _out;} }
		{
			goto st0;
		}
		ctr4:
		{
			#line 71 "rl/json_string.rl"
			
			mark = p;
		}
		{
			#line 77 "rl/json_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st4;
		ctr7:
		{
			#line 77 "rl/json_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st4;
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto ctr8;
			}
			case 39: {
				goto ctr8;
			}
			case 47: {
				goto ctr8;
			}
			case 92: {
				goto ctr8;
			}
			case 98: {
				goto ctr8;
			}
			case 102: {
				goto ctr8;
			}
			case 110: {
				goto ctr8;
			}
			case 114: {
				goto ctr8;
			}
			case 116: {
				goto ctr8;
			}
			case 117: {
				goto st5;
			}
		}
		{
			goto st0;
		}
		st5:
		p+= 1;
		if ( p == pe )
		goto _test_eof5;
		st_case_5:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr10;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr10;
			}
		} else {
			goto ctr10;
		}
		{
			goto st0;
		}
		ctr10:
		{
			#line 11 "rl/json_string.rl"
			
			hexchr0 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st6;
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr11;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr11;
			}
		} else {
			goto ctr11;
		}
		{
			goto st0;
		}
		ctr11:
		{
			#line 15 "rl/json_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st7;
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr12;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr12;
			}
		} else {
			goto ctr12;
		}
		{
			goto st0;
		}
		ctr12:
		{
			#line 15 "rl/json_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st8;
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr13;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr13;
			}
		} else {
			goto ctr13;
		}
		{
			goto st0;
		}
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		if ( (((int)INDEX_PCHARP(str, p))) == 92 ) {
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
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
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
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr16;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr16;
			}
		} else {
			goto ctr16;
		}
		{
			goto st0;
		}
		ctr16:
		{
			#line 37 "rl/json_string.rl"
			
			hexchr1 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st12;
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr17;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr17;
			}
		} else {
			goto ctr17;
		}
		{
			goto st0;
		}
		ctr17:
		{
			#line 41 "rl/json_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st13;
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr18;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr18;
			}
		} else {
			goto ctr18;
		}
		{
			goto st0;
		}
		ctr18:
		{
			#line 41 "rl/json_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st14;
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr19;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr19;
			}
		} else {
			goto ctr19;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof1: cs = 1; goto _test_eof; 
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof15: cs = 15; goto _test_eof; 
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
		
		_test_eof: {}
		_out: {}
	}
	
	#line 117 "rl/json_string.rl"
	
	
	if (cs < JSON_string_first_final) {
		if (validate) {
			UNSET_ONERROR(handle);
			free_string_builder(&s);
		}
		
		state->flags |= JSON_ERROR;
		if (p == pe) {
			err_msg="Unterminated string";
			return start;
		}
		return p;
	}
	
	if (validate) {
		push_string(finish_string_builder(&s));
		UNSET_ONERROR(handle);
	}
	
	return p;
}

#undef HEX2DEC
