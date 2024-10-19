
#line 1 "rl/json_string_utf8.rl"
/* -*- mode: C; c-basic-offset: 4; -*-
* vim:syntax=ragel
*/
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 130 "rl/json_string_utf8.rl"


static ptrdiff_t _parse_JSON_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
	unsigned char *p = (unsigned char*)(str.ptr) + pos;
	unsigned char *pe = (unsigned char*)(str.ptr) + end;
	ptrdiff_t start = pos;
	unsigned char *mark=0;
	struct string_builder s;
	int cs;
	ONERROR handle;
	int hexchr0 = 0, hexchr1 = 0;
	const int validate = !(state->flags&JSON_VALIDATE);
	p_wchar2 unicode=0;
	
	
	static const int JSON_string_start = 1;
	static const int JSON_string_first_final = 21;
	static const int JSON_string_error = 0;
	
	static const int JSON_string_en_main = 1;
	static const int JSON_string_en_main_hex1 = 15;
	
	static const char _JSON_string_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON_string_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0
	};
	
	static const char _JSON_string_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON_string_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 144 "rl/json_string_utf8.rl"
	
	
	if (validate) {
		init_string_builder(&s, 0);
		SET_ONERROR(handle, free_string_builder, &s);
	}
	
	
	{
		cs = (int)JSON_string_start;
	}
	
	#line 151 "rl/json_string_utf8.rl"
	
	
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
			case 21: goto st21;
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
			case 15: goto st15;
			case 16: goto st16;
			case 17: goto st17;
			case 18: goto st18;
			case 19: goto st19;
			case 20: goto st20;
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
			case 21:
			goto st_case_21;
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
			case 20:
			goto st_case_20;
		}
		goto st_out;
		st1:
		p+= 1;
		if ( p == pe )
		goto _test_eof1;
		st_case_1:
		if ( ( (*( p))) == 34u ) {
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
		switch( ( (*( p))) ) {
			case 34u: {
				goto ctr3;
			}
			case 92u: {
				goto ctr4;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( 32u <= ( (*( p))) && ( (*( p))) <= 127u ) {
				goto ctr2;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr7;
				}
			} else {
				goto ctr6;
			}
		} else {
			goto ctr5;
		}
		{
			goto st0;
		}
		ctr2:
		{
			#line 70 "rl/json_string_utf8.rl"
			
			mark = p;
		}
		
		goto st3;
		ctr14:
		{
			#line 56 "rl/json_string_utf8.rl"
			
			if (validate) switch((( (*( p))))) {
				case '\'':
				case '"':
				case '/':
				case '\\':      string_builder_putchar(&s, (( (*( p))))); break;
				case 'b':       string_builder_putchar(&s, '\b'); break;
				case 'f':       string_builder_putchar(&s, '\f'); break;
				case 'n':       string_builder_putchar(&s, '\n'); break;
				case 'r':       string_builder_putchar(&s, '\r'); break;
				case 't':       string_builder_putchar(&s, '\t'); break;
			}
		}
		{
			#line 74 "rl/json_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr19:
		cs = 3;
		{
			#line 14 "rl/json_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		{
			#line 19 "rl/json_string_utf8.rl"
			
			if (IS_HIGH_SURROGATE (hexchr0)) {
				/* Chars outside the BMP can be expressed as two hex
				* escapes that codes a surrogate pair, so see if we can
				* read a second one. */
				cs = 15;}
			else {
				if (IS_NUNICODE(hexchr0)) {
					goto failure;
				}
				if (validate) {
					string_builder_putchar(&s, hexchr0);
				}
			}
		}
		{
			#line 74 "rl/json_string_utf8.rl"
			mark = p + 1; }
		
		goto _again;
		ctr20:
		{
			#line 84 "rl/json_string_utf8.rl"
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80)); }
		{
			#line 106 "rl/json_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 74 "rl/json_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr22:
		{
			#line 88 "rl/json_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
				goto failure;	
			}
		}
		{
			#line 106 "rl/json_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 74 "rl/json_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr25:
		{
			#line 99 "rl/json_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if (unicode < 0x010000 || unicode > 0x10ffff) {
				goto failure;
			}
		}
		{
			#line 106 "rl/json_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 74 "rl/json_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr31:
		{
			#line 40 "rl/json_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		{
			#line 45 "rl/json_string_utf8.rl"
			
			if (!IS_LOW_SURROGATE (hexchr1)) {
				goto failure;
			}
			if (validate) {
				int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
				0x10000;
				string_builder_putchar(&s, cp);
			}
		}
		{
			#line 74 "rl/json_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( ( (*( p))) ) {
			case 34u: {
				goto ctr9;
			}
			case 92u: {
				goto ctr10;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( 32u <= ( (*( p))) && ( (*( p))) <= 127u ) {
				goto st3;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr13;
				}
			} else {
				goto ctr12;
			}
		} else {
			goto ctr11;
		}
		{
			goto st0;
		}
		ctr3:
		{
			#line 70 "rl/json_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st21;
		ctr9:
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st21;
		st21:
		p+= 1;
		if ( p == pe )
		goto _test_eof21;
		st_case_21:
		{
			#line 128 "rl/json_string_utf8.rl"
			p--; {p+= 1; cs = 21; goto _out;} }
		{
			goto st0;
		}
		ctr4:
		{
			#line 70 "rl/json_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st4;
		ctr10:
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st4;
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		switch( ( (*( p))) ) {
			case 34u: {
				goto ctr14;
			}
			case 39u: {
				goto ctr14;
			}
			case 47u: {
				goto ctr14;
			}
			case 92u: {
				goto ctr14;
			}
			case 98u: {
				goto ctr14;
			}
			case 102u: {
				goto ctr14;
			}
			case 110u: {
				goto ctr14;
			}
			case 114u: {
				goto ctr14;
			}
			case 116u: {
				goto ctr14;
			}
			case 117u: {
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
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr16;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
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
			#line 10 "rl/json_string_utf8.rl"
			
			hexchr0 = HEX2DEC((( (*( p)))));
		}
		
		goto st6;
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr17;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
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
			#line 14 "rl/json_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
		goto st7;
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr18;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
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
			#line 14 "rl/json_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
		goto st8;
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr19;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr19;
			}
		} else {
			goto ctr19;
		}
		{
			goto st0;
		}
		ctr5:
		{
			#line 70 "rl/json_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 83 "rl/json_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
		goto st9;
		ctr11:
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 83 "rl/json_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
		goto st9;
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr20;
		}
		{
			goto st0;
		}
		ctr6:
		{
			#line 70 "rl/json_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 86 "rl/json_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
		goto st10;
		ctr12:
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 86 "rl/json_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
		goto st10;
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr21;
		}
		{
			goto st0;
		}
		ctr21:
		{
			#line 87 "rl/json_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
		goto st11;
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr22;
		}
		{
			goto st0;
		}
		ctr7:
		{
			#line 70 "rl/json_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 95 "rl/json_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
		goto st12;
		ctr13:
		{
			#line 76 "rl/json_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 95 "rl/json_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
		goto st12;
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr23;
		}
		{
			goto st0;
		}
		ctr23:
		{
			#line 96 "rl/json_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 12; }
		
		goto st13;
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr24:
		{
			#line 87 "rl/json_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
		goto st14;
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr25;
		}
		{
			goto st0;
		}
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		if ( ( (*( p))) == 92u ) {
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
		if ( ( (*( p))) == 117u ) {
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
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr28;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr28;
			}
		} else {
			goto ctr28;
		}
		{
			goto st0;
		}
		ctr28:
		{
			#line 36 "rl/json_string_utf8.rl"
			
			hexchr1 = HEX2DEC((( (*( p)))));
		}
		
		goto st18;
		st18:
		p+= 1;
		if ( p == pe )
		goto _test_eof18;
		st_case_18:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr29;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
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
			#line 40 "rl/json_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
		goto st19;
		st19:
		p+= 1;
		if ( p == pe )
		goto _test_eof19;
		st_case_19:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr30;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
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
			#line 40 "rl/json_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
		goto st20;
		st20:
		p+= 1;
		if ( p == pe )
		goto _test_eof20;
		st_case_20:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr31;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr31;
			}
		} else {
			goto ctr31;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof1: cs = 1; goto _test_eof; 
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof21: cs = 21; goto _test_eof; 
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
		_test_eof20: cs = 20; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 152 "rl/json_string_utf8.rl"
	
	
	if (cs >= JSON_string_first_final) {
		if (validate) {
			push_string(finish_string_builder(&s));
			UNSET_ONERROR(handle);
		}
		
		return p - (unsigned char*)(str.ptr);
	}
	
	failure:
	
	if (validate) {
		UNSET_ONERROR(handle);
		free_string_builder(&s);
	}
	
	state->flags |= JSON_ERROR;
	if (p == pe) {
		err_msg="Unterminated string";
		return start;
	}
	return p - (unsigned char*)(str.ptr);
}

#undef HEX2DEC
