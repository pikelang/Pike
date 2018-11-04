
#line 1 "rl/json5_string_utf8.rl"
/* vim:syntax=ragel
*/
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 179 "rl/json5_string_utf8.rl"


static ptrdiff_t _parse_JSON5_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
	unsigned char *p = (unsigned char*)(str.ptr) + pos;
	unsigned char *pe = (unsigned char*)(str.ptr) + end;
	ptrdiff_t start = pos;
	unsigned char *mark=0;
	struct string_builder s;
	int cs;
	ONERROR handle;
	int hexchr0, hexchr1;
	const int validate = !(state->flags&JSON5_VALIDATE);
	p_wchar2 unicode=0;
	
	
	static const int JSON5_string_start = 1;
	static const int JSON5_string_first_final = 40;
	static const int JSON5_string_error = 0;
	
	static const int JSON5_string_en_main = 1;
	static const int JSON5_string_en_main_hex1 = 28;
	static const int JSON5_string_en_main_hex3 = 34;
	
	static const char _JSON5_string_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON5_string_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0
	};
	
	static const char _JSON5_string_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON5_string_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 194 "rl/json5_string_utf8.rl"
	
	
	if (validate) {
		init_string_builder(&s, 0);
		SET_ONERROR(handle, free_string_builder, &s);
	}
	
	
	{
		cs = (int)JSON5_string_start;
	}
	
	#line 201 "rl/json5_string_utf8.rl"
	
	
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
			case 40: goto st40;
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
			case 41: goto st41;
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
			case 28: goto st28;
			case 29: goto st29;
			case 30: goto st30;
			case 31: goto st31;
			case 32: goto st32;
			case 33: goto st33;
			case 34: goto st34;
			case 35: goto st35;
			case 36: goto st36;
			case 37: goto st37;
			case 38: goto st38;
			case 39: goto st39;
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
			case 40:
			goto st_case_40;
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
			case 41:
			goto st_case_41;
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
			case 28:
			goto st_case_28;
			case 29:
			goto st_case_29;
			case 30:
			goto st_case_30;
			case 31:
			goto st_case_31;
			case 32:
			goto st_case_32;
			case 33:
			goto st_case_33;
			case 34:
			goto st_case_34;
			case 35:
			goto st_case_35;
			case 36:
			goto st_case_36;
			case 37:
			goto st_case_37;
			case 38:
			goto st_case_38;
			case 39:
			goto st_case_39;
		}
		goto st_out;
		st1:
		p+= 1;
		if ( p == pe )
		goto _test_eof1;
		st_case_1:
		switch( ( (*( p))) ) {
			case 34u: {
				goto st2;
			}
			case 39u: {
				goto st15;
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
		switch( ( (*( p))) ) {
			case 34u: {
				goto ctr4;
			}
			case 92u: {
				goto ctr5;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( 32u <= ( (*( p))) && ( (*( p))) <= 127u ) {
				goto ctr3;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr8;
				}
			} else {
				goto ctr7;
			}
		} else {
			goto ctr6;
		}
		{
			goto st0;
		}
		ctr3:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
		goto st3;
		ctr15:
		{
			#line 82 "rl/json5_string_utf8.rl"
			
			if (validate) switch((( (*( p))))) {
				case '\n':
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
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr20:
		cs = 3;
		{
			#line 14 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		{
			#line 19 "rl/json5_string_utf8.rl"
			
			if (IS_HIGH_SURROGATE (hexchr0)) {
				/* Chars outside the BMP can be expressed as two hex
				* escapes that codes a surrogate pair, so see if we can
				* read a second one. */
				cs = 28;}
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
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto _again;
		ctr21:
		{
			#line 111 "rl/json5_string_utf8.rl"
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80)); }
		{
			#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr23:
		{
			#line 115 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
				goto failure;	
			}
		}
		{
			#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr26:
		{
			#line 126 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if (unicode < 0x010000 || unicode > 0x10ffff) {
				goto failure;
			}
		}
		{
			#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		ctr56:
		{
			#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		{
			#line 45 "rl/json5_string_utf8.rl"
			
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
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st3;
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( ( (*( p))) ) {
			case 34u: {
				goto ctr10;
			}
			case 92u: {
				goto ctr11;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( 32u <= ( (*( p))) && ( (*( p))) <= 127u ) {
				goto st3;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr14;
				}
			} else {
				goto ctr13;
			}
		} else {
			goto ctr12;
		}
		{
			goto st0;
		}
		ctr4:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st40;
		ctr10:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st40;
		st40:
		p+= 1;
		if ( p == pe )
		goto _test_eof40;
		st_case_40:
		{
			#line 161 "rl/json5_string_utf8.rl"
			p--; {p+= 1; cs = 40; goto _out;} }
		{
			goto st0;
		}
		ctr5:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st4;
		ctr11:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
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
			case 10u: {
				goto ctr15;
			}
			case 34u: {
				goto ctr15;
			}
			case 39u: {
				goto ctr15;
			}
			case 47u: {
				goto ctr15;
			}
			case 92u: {
				goto ctr15;
			}
			case 98u: {
				goto ctr15;
			}
			case 102u: {
				goto ctr15;
			}
			case 110u: {
				goto ctr15;
			}
			case 114u: {
				goto ctr15;
			}
			case 116u: {
				goto ctr15;
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
			#line 10 "rl/json5_string_utf8.rl"
			
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
			#line 14 "rl/json5_string_utf8.rl"
			
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
		ctr19:
		{
			#line 14 "rl/json5_string_utf8.rl"
			
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
				goto ctr20;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr20;
			}
		} else {
			goto ctr20;
		}
		{
			goto st0;
		}
		ctr6:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
		goto st9;
		ctr12:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
		goto st9;
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr21;
		}
		{
			goto st0;
		}
		ctr7:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
		goto st10;
		ctr13:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
		goto st10;
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr22;
		}
		{
			goto st0;
		}
		ctr22:
		{
			#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
		goto st11;
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr23;
		}
		{
			goto st0;
		}
		ctr8:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
		goto st12;
		ctr14:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
		goto st12;
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr24:
		{
			#line 123 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 12; }
		
		goto st13;
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr25;
		}
		{
			goto st0;
		}
		ctr25:
		{
			#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
		goto st14;
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr26;
		}
		{
			goto st0;
		}
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		switch( ( (*( p))) ) {
			case 39u: {
				goto ctr28;
			}
			case 92u: {
				goto ctr29;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( ( (*( p))) > 33u ) {
				if ( 35u <= ( (*( p))) && ( (*( p))) <= 127u ) {
					goto ctr27;
				}
			} else if ( ( (*( p))) >= 32u ) {
				goto ctr27;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr32;
				}
			} else {
				goto ctr31;
			}
		} else {
			goto ctr30;
		}
		{
			goto st0;
		}
		ctr27:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
		goto st16;
		ctr39:
		{
			#line 82 "rl/json5_string_utf8.rl"
			
			if (validate) switch((( (*( p))))) {
				case '\n':
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
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st16;
		ctr45:
		{
			#line 111 "rl/json5_string_utf8.rl"
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80)); }
		{
			#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st16;
		ctr47:
		{
			#line 115 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
				goto failure;	
			}
		}
		{
			#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st16;
		ctr50:
		{
			#line 126 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if (unicode < 0x010000 || unicode > 0x10ffff) {
				goto failure;
			}
		}
		{
			#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		{
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st16;
		ctr44:
		cs = 16;
		{
			#line 60 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		{
			#line 65 "rl/json5_string_utf8.rl"
			
			if (IS_HIGH_SURROGATE (hexchr0)) {
				/* Chars outside the BMP can be expressed as two hex 
				* escapes that codes a surrogate pair, so see if we can
				* read a second one. */
				cs = 34;}
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
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto _again;
		ctr62:
		{
			#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		{
			#line 45 "rl/json5_string_utf8.rl"
			
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
			#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
		goto st16;
		st16:
		p+= 1;
		if ( p == pe )
		goto _test_eof16;
		st_case_16:
		switch( ( (*( p))) ) {
			case 39u: {
				goto ctr34;
			}
			case 92u: {
				goto ctr35;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( ( (*( p))) > 33u ) {
				if ( 35u <= ( (*( p))) && ( (*( p))) <= 127u ) {
					goto st16;
				}
			} else if ( ( (*( p))) >= 32u ) {
				goto st16;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr38;
				}
			} else {
				goto ctr37;
			}
		} else {
			goto ctr36;
		}
		{
			goto st0;
		}
		ctr28:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st41;
		ctr34:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st41;
		st41:
		p+= 1;
		if ( p == pe )
		goto _test_eof41;
		st_case_41:
		{
			#line 178 "rl/json5_string_utf8.rl"
			p--; {p+= 1; cs = 41; goto _out;} }
		switch( ( (*( p))) ) {
			case 39u: {
				goto ctr34;
			}
			case 92u: {
				goto ctr35;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( ( (*( p))) > 33u ) {
				if ( 35u <= ( (*( p))) && ( (*( p))) <= 127u ) {
					goto st16;
				}
			} else if ( ( (*( p))) >= 32u ) {
				goto st16;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto ctr38;
				}
			} else {
				goto ctr37;
			}
		} else {
			goto ctr36;
		}
		{
			goto st0;
		}
		ctr29:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st17;
		ctr35:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
		goto st17;
		st17:
		p+= 1;
		if ( p == pe )
		goto _test_eof17;
		st_case_17:
		switch( ( (*( p))) ) {
			case 10u: {
				goto ctr39;
			}
			case 39u: {
				goto ctr39;
			}
			case 47u: {
				goto ctr39;
			}
			case 92u: {
				goto ctr39;
			}
			case 98u: {
				goto ctr39;
			}
			case 102u: {
				goto ctr39;
			}
			case 110u: {
				goto ctr39;
			}
			case 114u: {
				goto ctr39;
			}
			case 116u: {
				goto ctr39;
			}
			case 117u: {
				goto st18;
			}
		}
		{
			goto st0;
		}
		st18:
		p+= 1;
		if ( p == pe )
		goto _test_eof18;
		st_case_18:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr41;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr41;
			}
		} else {
			goto ctr41;
		}
		{
			goto st0;
		}
		ctr41:
		{
			#line 56 "rl/json5_string_utf8.rl"
			
			hexchr0 = HEX2DEC((( (*( p)))));
		}
		
		goto st19;
		st19:
		p+= 1;
		if ( p == pe )
		goto _test_eof19;
		st_case_19:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr42;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr42;
			}
		} else {
			goto ctr42;
		}
		{
			goto st0;
		}
		ctr42:
		{
			#line 60 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
		goto st20;
		st20:
		p+= 1;
		if ( p == pe )
		goto _test_eof20;
		st_case_20:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr43;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr43;
			}
		} else {
			goto ctr43;
		}
		{
			goto st0;
		}
		ctr43:
		{
			#line 60 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
		goto st21;
		st21:
		p+= 1;
		if ( p == pe )
		goto _test_eof21;
		st_case_21:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr44;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr44;
			}
		} else {
			goto ctr44;
		}
		{
			goto st0;
		}
		ctr30:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
		goto st22;
		ctr36:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
		goto st22;
		st22:
		p+= 1;
		if ( p == pe )
		goto _test_eof22;
		st_case_22:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr45;
		}
		{
			goto st0;
		}
		ctr31:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
		goto st23;
		ctr37:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
		goto st23;
		st23:
		p+= 1;
		if ( p == pe )
		goto _test_eof23;
		st_case_23:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr46;
		}
		{
			goto st0;
		}
		ctr46:
		{
			#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
		goto st24;
		st24:
		p+= 1;
		if ( p == pe )
		goto _test_eof24;
		st_case_24:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr47;
		}
		{
			goto st0;
		}
		ctr32:
		{
			#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
		goto st25;
		ctr38:
		{
			#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		{
			#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
		goto st25;
		st25:
		p+= 1;
		if ( p == pe )
		goto _test_eof25;
		st_case_25:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr48;
		}
		{
			goto st0;
		}
		ctr48:
		{
			#line 123 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 12; }
		
		goto st26;
		st26:
		p+= 1;
		if ( p == pe )
		goto _test_eof26;
		st_case_26:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr49;
		}
		{
			goto st0;
		}
		ctr49:
		{
			#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
		goto st27;
		st27:
		p+= 1;
		if ( p == pe )
		goto _test_eof27;
		st_case_27:
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto ctr50;
		}
		{
			goto st0;
		}
		st28:
		p+= 1;
		if ( p == pe )
		goto _test_eof28;
		st_case_28:
		if ( ( (*( p))) == 92u ) {
			goto st29;
		}
		{
			goto st0;
		}
		st29:
		p+= 1;
		if ( p == pe )
		goto _test_eof29;
		st_case_29:
		if ( ( (*( p))) == 117u ) {
			goto st30;
		}
		{
			goto st0;
		}
		st30:
		p+= 1;
		if ( p == pe )
		goto _test_eof30;
		st_case_30:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr53;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr53;
			}
		} else {
			goto ctr53;
		}
		{
			goto st0;
		}
		ctr53:
		{
			#line 36 "rl/json5_string_utf8.rl"
			
			hexchr1 = HEX2DEC((( (*( p)))));
		}
		
		goto st31;
		st31:
		p+= 1;
		if ( p == pe )
		goto _test_eof31;
		st_case_31:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr54;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr54;
			}
		} else {
			goto ctr54;
		}
		{
			goto st0;
		}
		ctr54:
		{
			#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
		goto st32;
		st32:
		p+= 1;
		if ( p == pe )
		goto _test_eof32;
		st_case_32:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr55;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr55;
			}
		} else {
			goto ctr55;
		}
		{
			goto st0;
		}
		ctr55:
		{
			#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
		goto st33;
		st33:
		p+= 1;
		if ( p == pe )
		goto _test_eof33;
		st_case_33:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr56;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr56;
			}
		} else {
			goto ctr56;
		}
		{
			goto st0;
		}
		st34:
		p+= 1;
		if ( p == pe )
		goto _test_eof34;
		st_case_34:
		if ( ( (*( p))) == 92u ) {
			goto st35;
		}
		{
			goto st0;
		}
		st35:
		p+= 1;
		if ( p == pe )
		goto _test_eof35;
		st_case_35:
		if ( ( (*( p))) == 117u ) {
			goto st36;
		}
		{
			goto st0;
		}
		st36:
		p+= 1;
		if ( p == pe )
		goto _test_eof36;
		st_case_36:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr59;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr59;
			}
		} else {
			goto ctr59;
		}
		{
			goto st0;
		}
		ctr59:
		{
			#line 36 "rl/json5_string_utf8.rl"
			
			hexchr1 = HEX2DEC((( (*( p)))));
		}
		
		goto st37;
		st37:
		p+= 1;
		if ( p == pe )
		goto _test_eof37;
		st_case_37:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr60;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr60;
			}
		} else {
			goto ctr60;
		}
		{
			goto st0;
		}
		ctr60:
		{
			#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
		goto st38;
		st38:
		p+= 1;
		if ( p == pe )
		goto _test_eof38;
		st_case_38:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr61;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr61;
			}
		} else {
			goto ctr61;
		}
		{
			goto st0;
		}
		ctr61:
		{
			#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
		goto st39;
		st39:
		p+= 1;
		if ( p == pe )
		goto _test_eof39;
		st_case_39:
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto ctr62;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto ctr62;
			}
		} else {
			goto ctr62;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof1: cs = 1; goto _test_eof; 
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof40: cs = 40; goto _test_eof; 
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
		_test_eof41: cs = 41; goto _test_eof; 
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
		_test_eof28: cs = 28; goto _test_eof; 
		_test_eof29: cs = 29; goto _test_eof; 
		_test_eof30: cs = 30; goto _test_eof; 
		_test_eof31: cs = 31; goto _test_eof; 
		_test_eof32: cs = 32; goto _test_eof; 
		_test_eof33: cs = 33; goto _test_eof; 
		_test_eof34: cs = 34; goto _test_eof; 
		_test_eof35: cs = 35; goto _test_eof; 
		_test_eof36: cs = 36; goto _test_eof; 
		_test_eof37: cs = 37; goto _test_eof; 
		_test_eof38: cs = 38; goto _test_eof; 
		_test_eof39: cs = 39; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 202 "rl/json5_string_utf8.rl"
	
	
	if (cs >= JSON5_string_first_final) {
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
	
	state->flags |= JSON5_ERROR;
	if (p == pe) {
		err_msg="Unterminated string";
		return start;
	}
	return p - (unsigned char*)(str.ptr);
}

#undef HEX2DEC
