
#line 1 "rl/json5_string.rl"
/* vim:syntax=ragel
*/
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 147 "rl/json5_string.rl"


static ptrdiff_t _parse_JSON5_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	int hexchr0, hexchr1;
	ptrdiff_t start = p, mark=0;
	struct string_builder s;
	int cs;
	ONERROR handle;
	const int validate = !(state->flags&JSON5_VALIDATE);
	
	static const int JSON5_string_start = 1;
	static const int JSON5_string_first_final = 28;
	static const int JSON5_string_error = 0;
	
	static const int JSON5_string_en_main = 1;
	static const int JSON5_string_en_main_hex1 = 16;
	static const int JSON5_string_en_main_hex3 = 22;
	
	static const char _JSON5_string_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON5_string_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0
	};
	
	static const char _JSON5_string_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON5_string_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 157 "rl/json5_string.rl"
	
	
	if (validate) {
		init_string_builder(&s, 0);
		SET_ONERROR (handle, free_string_builder, &s);
	}
	
	
	{
		cs = (int)JSON5_string_start;
	}
	
	#line 164 "rl/json5_string.rl"
	
	
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
			case 28: goto st28;
			case 4: goto st4;
			case 5: goto st5;
			case 6: goto st6;
			case 7: goto st7;
			case 8: goto st8;
			case 9: goto st9;
			case 10: goto st10;
			case 29: goto st29;
			case 11: goto st11;
			case 12: goto st12;
			case 13: goto st13;
			case 14: goto st14;
			case 15: goto st15;
			case 16: goto st16;
			case 17: goto st17;
			case 18: goto st18;
			case 19: goto st19;
			case 20: goto st20;
			case 21: goto st21;
			case 22: goto st22;
			case 23: goto st23;
			case 24: goto st24;
			case 25: goto st25;
			case 26: goto st26;
			case 27: goto st27;
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
			case 28:
			goto st_case_28;
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
			case 29:
			goto st_case_29;
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
			case 20:
			goto st_case_20;
			case 21:
			goto st_case_21;
			case 22:
			goto st_case_22;
			case 23:
			goto st_case_23;
			case 24:
			goto st_case_24;
			case 25:
			goto st_case_25;
			case 26:
			goto st_case_26;
			case 27:
			goto st_case_27;
		}
		goto st_out;
		st1:
		p+= 1;
		if ( p == pe )
		goto _test_eof1;
		st_case_1:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto st2;
			}
			case 39: {
				goto st9;
			}
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
				goto ctr4;
			}
			case 92: {
				goto ctr5;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto ctr3;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto ctr3;
		}
		{
			goto st0;
		}
		ctr3:
		{
			#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
		goto st3;
		ctr9:
		{
			#line 83 "rl/json5_string.rl"
			
			if (validate) switch(((((int)INDEX_PCHARP(str, p))))) {
				case '\n':
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
			#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
		goto st3;
		ctr14:
		cs = 3;
		{
			#line 15 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		{
			#line 20 "rl/json5_string.rl"
			
			if (IS_HIGH_SURROGATE (hexchr0)) {
				/* Chars outside the BMP can be expressed as two hex
				* escapes that codes a surrogate pair, so see if we can
				* read a second one. */
				cs = 16;}
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
			#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
		goto _again;
		ctr32:
		{
			#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		{
			#line 46 "rl/json5_string.rl"
			
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
			#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
		goto st3;
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto ctr7;
			}
			case 92: {
				goto ctr8;
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
		ctr4:
		{
			#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st28;
		ctr7:
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st28;
		st28:
		p+= 1;
		if ( p == pe )
		goto _test_eof28;
		st_case_28:
		{
			#line 130 "rl/json5_string.rl"
			p--; {p+= 1; cs = 28; goto _out;} }
		{
			goto st0;
		}
		ctr5:
		{
			#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st4;
		ctr8:
		{
			#line 104 "rl/json5_string.rl"
			
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
			case 10: {
				goto ctr9;
			}
			case 34: {
				goto ctr9;
			}
			case 39: {
				goto ctr9;
			}
			case 47: {
				goto ctr9;
			}
			case 92: {
				goto ctr9;
			}
			case 98: {
				goto ctr9;
			}
			case 102: {
				goto ctr9;
			}
			case 110: {
				goto ctr9;
			}
			case 114: {
				goto ctr9;
			}
			case 116: {
				goto ctr9;
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
			#line 11 "rl/json5_string.rl"
			
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
			#line 15 "rl/json5_string.rl"
			
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
		ctr13:
		{
			#line 15 "rl/json5_string.rl"
			
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
				goto ctr14;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr14;
			}
		} else {
			goto ctr14;
		}
		{
			goto st0;
		}
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 39: {
				goto ctr16;
			}
			case 92: {
				goto ctr17;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto ctr15;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto ctr15;
		}
		{
			goto st0;
		}
		ctr15:
		{
			#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
		goto st10;
		ctr21:
		{
			#line 83 "rl/json5_string.rl"
			
			if (validate) switch(((((int)INDEX_PCHARP(str, p))))) {
				case '\n':
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
			#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
		goto st10;
		ctr26:
		cs = 10;
		{
			#line 61 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		{
			#line 66 "rl/json5_string.rl"
			
			if (IS_HIGH_SURROGATE (hexchr0)) {
				/* Chars outside the BMP can be expressed as two hex
				* escapes that codes a surrogate pair, so see if we can
				* read a second one. */
				cs = 22;}
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
			#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
		goto _again;
		ctr38:
		{
			#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		{
			#line 46 "rl/json5_string.rl"
			
			if (!IS_LOW_SURROGATE (hexchr1)) {
				p--; {p+= 1; cs = 10; goto _out;}
			}
			if (validate) {
				int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
				0x10000;
				string_builder_putchar(&s, cp);
			}
		}
		{
			#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
		goto st10;
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 39: {
				goto ctr19;
			}
			case 92: {
				goto ctr20;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto st10;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto st10;
		}
		{
			goto st0;
		}
		ctr16:
		{
			#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st29;
		ctr19:
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st29;
		st29:
		p+= 1;
		if ( p == pe )
		goto _test_eof29;
		st_case_29:
		{
			#line 145 "rl/json5_string.rl"
			p--; {p+= 1; cs = 29; goto _out;} }
		{
			goto st0;
		}
		ctr17:
		{
			#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st11;
		ctr20:
		{
			#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
		goto st11;
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto ctr21;
			}
			case 39: {
				goto ctr21;
			}
			case 47: {
				goto ctr21;
			}
			case 92: {
				goto ctr21;
			}
			case 98: {
				goto ctr21;
			}
			case 102: {
				goto ctr21;
			}
			case 110: {
				goto ctr21;
			}
			case 114: {
				goto ctr21;
			}
			case 116: {
				goto ctr21;
			}
			case 117: {
				goto st12;
			}
		}
		{
			goto st0;
		}
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr23;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr23;
			}
		} else {
			goto ctr23;
		}
		{
			goto st0;
		}
		ctr23:
		{
			#line 57 "rl/json5_string.rl"
			
			hexchr0 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st13;
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr24;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr24;
			}
		} else {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr24:
		{
			#line 61 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st14;
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr25;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr25;
			}
		} else {
			goto ctr25;
		}
		{
			goto st0;
		}
		ctr25:
		{
			#line 61 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st15;
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr26;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr26;
			}
		} else {
			goto ctr26;
		}
		{
			goto st0;
		}
		st16:
		p+= 1;
		if ( p == pe )
		goto _test_eof16;
		st_case_16:
		if ( (((int)INDEX_PCHARP(str, p))) == 92 ) {
			goto st17;
		}
		{
			goto st0;
		}
		st17:
		p+= 1;
		if ( p == pe )
		goto _test_eof17;
		st_case_17:
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
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
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr29;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr29;
			}
		} else {
			goto ctr29;
		}
		{
			goto st0;
		}
		ctr29:
		{
			#line 37 "rl/json5_string.rl"
			
			hexchr1 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st19;
		st19:
		p+= 1;
		if ( p == pe )
		goto _test_eof19;
		st_case_19:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr30;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr30;
			}
		} else {
			goto ctr30;
		}
		{
			goto st0;
		}
		ctr30:
		{
			#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st20;
		st20:
		p+= 1;
		if ( p == pe )
		goto _test_eof20;
		st_case_20:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr31;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr31;
			}
		} else {
			goto ctr31;
		}
		{
			goto st0;
		}
		ctr31:
		{
			#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st21;
		st21:
		p+= 1;
		if ( p == pe )
		goto _test_eof21;
		st_case_21:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr32;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr32;
			}
		} else {
			goto ctr32;
		}
		{
			goto st0;
		}
		st22:
		p+= 1;
		if ( p == pe )
		goto _test_eof22;
		st_case_22:
		if ( (((int)INDEX_PCHARP(str, p))) == 92 ) {
			goto st23;
		}
		{
			goto st0;
		}
		st23:
		p+= 1;
		if ( p == pe )
		goto _test_eof23;
		st_case_23:
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto st24;
		}
		{
			goto st0;
		}
		st24:
		p+= 1;
		if ( p == pe )
		goto _test_eof24;
		st_case_24:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr35;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr35;
			}
		} else {
			goto ctr35;
		}
		{
			goto st0;
		}
		ctr35:
		{
			#line 37 "rl/json5_string.rl"
			
			hexchr1 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st25;
		st25:
		p+= 1;
		if ( p == pe )
		goto _test_eof25;
		st_case_25:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr36;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr36;
			}
		} else {
			goto ctr36;
		}
		{
			goto st0;
		}
		ctr36:
		{
			#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st26;
		st26:
		p+= 1;
		if ( p == pe )
		goto _test_eof26;
		st_case_26:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr37;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr37;
			}
		} else {
			goto ctr37;
		}
		{
			goto st0;
		}
		ctr37:
		{
			#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
		goto st27;
		st27:
		p+= 1;
		if ( p == pe )
		goto _test_eof27;
		st_case_27:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr38;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto ctr38;
			}
		} else {
			goto ctr38;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof1: cs = 1; goto _test_eof; 
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof28: cs = 28; goto _test_eof; 
		_test_eof4: cs = 4; goto _test_eof; 
		_test_eof5: cs = 5; goto _test_eof; 
		_test_eof6: cs = 6; goto _test_eof; 
		_test_eof7: cs = 7; goto _test_eof; 
		_test_eof8: cs = 8; goto _test_eof; 
		_test_eof9: cs = 9; goto _test_eof; 
		_test_eof10: cs = 10; goto _test_eof; 
		_test_eof29: cs = 29; goto _test_eof; 
		_test_eof11: cs = 11; goto _test_eof; 
		_test_eof12: cs = 12; goto _test_eof; 
		_test_eof13: cs = 13; goto _test_eof; 
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
		_test_eof24: cs = 24; goto _test_eof; 
		_test_eof25: cs = 25; goto _test_eof; 
		_test_eof26: cs = 26; goto _test_eof; 
		_test_eof27: cs = 27; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 165 "rl/json5_string.rl"
	
	
	if (cs < JSON5_string_first_final) {
		if (validate) {
			UNSET_ONERROR(handle);
			free_string_builder(&s);
		}
		
		state->flags |= JSON5_ERROR;
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
