#line 1 "rl/json5_string_utf8.rl"
/* -*- mode: C; c-basic-offset: 4; -*-
* vim:syntax=ragel
*/
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 180 "rl/json5_string_utf8.rl"


static ptrdiff_t _parse_JSON5_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
	unsigned char *p = (unsigned char*)(str.ptr) + pos;
	unsigned char *pe = (unsigned char*)(str.ptr) + end;
	ptrdiff_t start = pos;
	unsigned char *mark = 0;
	struct string_builder s;
	int cs;
	ONERROR handle;
	int hexchr0 = 0, hexchr1 = 0;
	const int validate = !(state->flags&JSON5_VALIDATE);
	p_wchar2 unicode = 0;
	
	
#line 24 "json5_string_utf8.c"
	static const int JSON5_string_start = 1;
	static const int JSON5_string_first_final = 40;
	static const int JSON5_string_error = 0;
	
	static const int JSON5_string_en_main = 1;
	static const int JSON5_string_en_main_hex1 = 28;
	static const int JSON5_string_en_main_hex3 = 34;
	
	
#line 193 "rl/json5_string_utf8.rl"
	
	
	if (validate) {
		init_string_builder(&s, 0);
		SET_ONERROR(handle, free_string_builder, &s);
	}
	
	
#line 43 "json5_string_utf8.c"
	{
		cs = (int)JSON5_string_start;
	}
	
#line 200 "rl/json5_string_utf8.rl"
	
	
#line 51 "json5_string_utf8.c"
	{
		goto _resume;
		
		_again: {}
		switch ( cs ) {
			case 1: goto _st1;
			case 0: goto _st0;
			case 2: goto _st2;
			case 3: goto _st3;
			case 40: goto _st40;
			case 4: goto _st4;
			case 5: goto _st5;
			case 6: goto _st6;
			case 7: goto _st7;
			case 8: goto _st8;
			case 9: goto _st9;
			case 10: goto _st10;
			case 11: goto _st11;
			case 12: goto _st12;
			case 13: goto _st13;
			case 14: goto _st14;
			case 15: goto _st15;
			case 16: goto _st16;
			case 41: goto _st41;
			case 17: goto _st17;
			case 18: goto _st18;
			case 19: goto _st19;
			case 20: goto _st20;
			case 21: goto _st21;
			case 22: goto _st22;
			case 23: goto _st23;
			case 24: goto _st24;
			case 25: goto _st25;
			case 26: goto _st26;
			case 27: goto _st27;
			case 28: goto _st28;
			case 29: goto _st29;
			case 30: goto _st30;
			case 31: goto _st31;
			case 32: goto _st32;
			case 33: goto _st33;
			case 34: goto _st34;
			case 35: goto _st35;
			case 36: goto _st36;
			case 37: goto _st37;
			case 38: goto _st38;
			case 39: goto _st39;
		}
		
		_resume: {}
		switch ( cs ) {
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
		_st1:
		p+= 1;
		st_case_1:
		if ( p == pe )
			goto _out1;
		switch( ( (*( p))) ) {
			case 34u: {
				goto _st2;
			}
			case 39u: {
				goto _st15;
			}
		}
		goto _st0;
		_st0:
		st_case_0:
		goto _out0;
		_st2:
		p+= 1;
		st_case_2:
		if ( p == pe )
			goto _out2;
		switch( ( (*( p))) ) {
			case 34u: {
				goto _ctr5;
			}
			case 92u: {
				goto _ctr6;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( 32u <= ( (*( p))) && ( (*( p))) <= 127u ) {
				goto _ctr4;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto _ctr9;
				}
			} else {
				goto _ctr8;
			}
		} else {
			goto _ctr7;
		}
		goto _st0;
		_ctr4:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 241 "json5_string_utf8.c"
		
		goto _st3;
		_ctr17:
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
		
#line 262 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 268 "json5_string_utf8.c"
		
		goto _st3;
		_ctr25:
		cs = 3;
		{
#line 14 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
#line 280 "json5_string_utf8.c"
		
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
		
#line 300 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 306 "json5_string_utf8.c"
		
		goto _again;
		_ctr27:
		{
#line 111 "rl/json5_string_utf8.rl"
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80)); }
		
#line 314 "json5_string_utf8.c"
		
		{
#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		
#line 324 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 330 "json5_string_utf8.c"
		
		goto _st3;
		_ctr31:
		{
#line 115 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
				goto failure;	
			}
		}
		
#line 343 "json5_string_utf8.c"
		
		{
#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		
#line 353 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 359 "json5_string_utf8.c"
		
		goto _st3;
		_ctr37:
		{
#line 126 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if (unicode < 0x010000 || unicode > 0x10ffff) {
				goto failure;
			}
		}
		
#line 372 "json5_string_utf8.c"
		
		{
#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		
#line 382 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 388 "json5_string_utf8.c"
		
		goto _st3;
		_ctr81:
		{
#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
#line 399 "json5_string_utf8.c"
		
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
		
#line 414 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 420 "json5_string_utf8.c"
		
		goto _st3;
		_st3:
		p+= 1;
		st_case_3:
		if ( p == pe )
			goto _out3;
		switch( ( (*( p))) ) {
			case 34u: {
				goto _ctr11;
			}
			case 92u: {
				goto _ctr12;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( 32u <= ( (*( p))) && ( (*( p))) <= 127u ) {
				goto _st3;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto _ctr15;
				}
			} else {
				goto _ctr14;
			}
		} else {
			goto _ctr13;
		}
		goto _st0;
		_ctr5:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 459 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 470 "json5_string_utf8.c"
		
		goto _st40;
		_ctr11:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 483 "json5_string_utf8.c"
		
		goto _st40;
		_st40:
		p+= 1;
		st_case_40:
		if ( p == pe )
			goto _out40;
		{
#line 161 "rl/json5_string_utf8.rl"
			p--; {p+= 1; cs = 40; goto _out;} }
		
#line 495 "json5_string_utf8.c"
		
		goto _st0;
		_ctr6:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 505 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 516 "json5_string_utf8.c"
		
		goto _st4;
		_ctr12:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 529 "json5_string_utf8.c"
		
		goto _st4;
		_st4:
		p+= 1;
		st_case_4:
		if ( p == pe )
			goto _out4;
		switch( ( (*( p))) ) {
			case 10u: {
				goto _ctr17;
			}
			case 34u: {
				goto _ctr17;
			}
			case 39u: {
				goto _ctr17;
			}
			case 47u: {
				goto _ctr17;
			}
			case 92u: {
				goto _ctr17;
			}
			case 98u: {
				goto _ctr17;
			}
			case 102u: {
				goto _ctr17;
			}
			case 110u: {
				goto _ctr17;
			}
			case 114u: {
				goto _ctr17;
			}
			case 116u: {
				goto _ctr17;
			}
			case 117u: {
				goto _st5;
			}
		}
		goto _st0;
		_st5:
		p+= 1;
		st_case_5:
		if ( p == pe )
			goto _out5;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr19;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr19;
			}
		} else {
			goto _ctr19;
		}
		goto _st0;
		_ctr19:
		{
#line 10 "rl/json5_string_utf8.rl"
			
			hexchr0 = HEX2DEC((( (*( p)))));
		}
		
#line 597 "json5_string_utf8.c"
		
		goto _st6;
		_st6:
		p+= 1;
		st_case_6:
		if ( p == pe )
			goto _out6;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr21;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr21;
			}
		} else {
			goto _ctr21;
		}
		goto _st0;
		_ctr21:
		{
#line 14 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
#line 625 "json5_string_utf8.c"
		
		goto _st7;
		_st7:
		p+= 1;
		st_case_7:
		if ( p == pe )
			goto _out7;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr23;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr23;
			}
		} else {
			goto _ctr23;
		}
		goto _st0;
		_ctr23:
		{
#line 14 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
#line 653 "json5_string_utf8.c"
		
		goto _st8;
		_st8:
		p+= 1;
		st_case_8:
		if ( p == pe )
			goto _out8;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr25;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr25;
			}
		} else {
			goto _ctr25;
		}
		goto _st0;
		_ctr7:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 680 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 691 "json5_string_utf8.c"
		
		{
#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
#line 697 "json5_string_utf8.c"
		
		goto _st9;
		_ctr13:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 710 "json5_string_utf8.c"
		
		{
#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
#line 716 "json5_string_utf8.c"
		
		goto _st9;
		_st9:
		p+= 1;
		st_case_9:
		if ( p == pe )
			goto _out9;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr27;
		}
		goto _st0;
		_ctr8:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 735 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 746 "json5_string_utf8.c"
		
		{
#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
#line 752 "json5_string_utf8.c"
		
		goto _st10;
		_ctr14:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 765 "json5_string_utf8.c"
		
		{
#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
#line 771 "json5_string_utf8.c"
		
		goto _st10;
		_st10:
		p+= 1;
		st_case_10:
		if ( p == pe )
			goto _out10;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr29;
		}
		goto _st0;
		_ctr29:
		{
#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
#line 788 "json5_string_utf8.c"
		
		goto _st11;
		_st11:
		p+= 1;
		st_case_11:
		if ( p == pe )
			goto _out11;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr31;
		}
		goto _st0;
		_ctr9:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 807 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 818 "json5_string_utf8.c"
		
		{
#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
#line 824 "json5_string_utf8.c"
		
		goto _st12;
		_ctr15:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 837 "json5_string_utf8.c"
		
		{
#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
#line 843 "json5_string_utf8.c"
		
		goto _st12;
		_st12:
		p+= 1;
		st_case_12:
		if ( p == pe )
			goto _out12;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr33;
		}
		goto _st0;
		_ctr33:
		{
#line 123 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 12; }
		
#line 860 "json5_string_utf8.c"
		
		goto _st13;
		_st13:
		p+= 1;
		st_case_13:
		if ( p == pe )
			goto _out13;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr35;
		}
		goto _st0;
		_ctr35:
		{
#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
#line 877 "json5_string_utf8.c"
		
		goto _st14;
		_st14:
		p+= 1;
		st_case_14:
		if ( p == pe )
			goto _out14;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr37;
		}
		goto _st0;
		_st15:
		p+= 1;
		st_case_15:
		if ( p == pe )
			goto _out15;
		switch( ( (*( p))) ) {
			case 39u: {
				goto _ctr39;
			}
			case 92u: {
				goto _ctr40;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( ( (*( p))) > 33u ) {
				if ( 35u <= ( (*( p))) && ( (*( p))) <= 127u ) {
					goto _ctr38;
				}
			} else if ( ( (*( p))) >= 32u ) {
				goto _ctr38;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto _ctr43;
				}
			} else {
				goto _ctr42;
			}
		} else {
			goto _ctr41;
		}
		goto _st0;
		_ctr38:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 929 "json5_string_utf8.c"
		
		goto _st16;
		_ctr51:
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
		
#line 950 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 956 "json5_string_utf8.c"
		
		goto _st16;
		_ctr61:
		{
#line 111 "rl/json5_string_utf8.rl"
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80)); }
		
#line 964 "json5_string_utf8.c"
		
		{
#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		
#line 974 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 980 "json5_string_utf8.c"
		
		goto _st16;
		_ctr65:
		{
#line 115 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
				goto failure;	
			}
		}
		
#line 993 "json5_string_utf8.c"
		
		{
#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		
#line 1003 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 1009 "json5_string_utf8.c"
		
		goto _st16;
		_ctr71:
		{
#line 126 "rl/json5_string_utf8.rl"
			
			unicode |= (p_wchar2)((( (*( p)))) & (0xbf-0x80));
			if (unicode < 0x010000 || unicode > 0x10ffff) {
				goto failure;
			}
		}
		
#line 1022 "json5_string_utf8.c"
		
		{
#line 139 "rl/json5_string_utf8.rl"
			
			if (validate) { 
				string_builder_putchar(&s, unicode); 
			}
		}
		
#line 1032 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 1038 "json5_string_utf8.c"
		
		goto _st16;
		_ctr59:
		cs = 16;
		{
#line 60 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
#line 1050 "json5_string_utf8.c"
		
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
		
#line 1070 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 1076 "json5_string_utf8.c"
		
		goto _again;
		_ctr91:
		{
#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
#line 1087 "json5_string_utf8.c"
		
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
		
#line 1102 "json5_string_utf8.c"
		
		{
#line 101 "rl/json5_string_utf8.rl"
			mark = p + 1; }
		
#line 1108 "json5_string_utf8.c"
		
		goto _st16;
		_st16:
		p+= 1;
		st_case_16:
		if ( p == pe )
			goto _out16;
		switch( ( (*( p))) ) {
			case 39u: {
				goto _ctr45;
			}
			case 92u: {
				goto _ctr46;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( ( (*( p))) > 33u ) {
				if ( 35u <= ( (*( p))) && ( (*( p))) <= 127u ) {
					goto _st16;
				}
			} else if ( ( (*( p))) >= 32u ) {
				goto _st16;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto _ctr49;
				}
			} else {
				goto _ctr48;
			}
		} else {
			goto _ctr47;
		}
		goto _st0;
		_ctr39:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 1151 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1162 "json5_string_utf8.c"
		
		goto _st41;
		_ctr45:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1175 "json5_string_utf8.c"
		
		goto _st41;
		_st41:
		p+= 1;
		st_case_41:
		if ( p == pe )
			goto _out41;
		{
#line 178 "rl/json5_string_utf8.rl"
			p--; {p+= 1; cs = 41; goto _out;} }
		
#line 1187 "json5_string_utf8.c"
		
		switch( ( (*( p))) ) {
			case 39u: {
				goto _ctr45;
			}
			case 92u: {
				goto _ctr46;
			}
		}
		if ( ( (*( p))) < 194u ) {
			if ( ( (*( p))) > 33u ) {
				if ( 35u <= ( (*( p))) && ( (*( p))) <= 127u ) {
					goto _st16;
				}
			} else if ( ( (*( p))) >= 32u ) {
				goto _st16;
			}
		} else if ( ( (*( p))) > 223u ) {
			if ( ( (*( p))) > 239u ) {
				if ( ( (*( p))) <= 244u ) {
					goto _ctr49;
				}
			} else {
				goto _ctr48;
			}
		} else {
			goto _ctr47;
		}
		goto _st0;
		_ctr40:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 1224 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1235 "json5_string_utf8.c"
		
		goto _st17;
		_ctr46:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1248 "json5_string_utf8.c"
		
		goto _st17;
		_st17:
		p+= 1;
		st_case_17:
		if ( p == pe )
			goto _out17;
		switch( ( (*( p))) ) {
			case 10u: {
				goto _ctr51;
			}
			case 39u: {
				goto _ctr51;
			}
			case 47u: {
				goto _ctr51;
			}
			case 92u: {
				goto _ctr51;
			}
			case 98u: {
				goto _ctr51;
			}
			case 102u: {
				goto _ctr51;
			}
			case 110u: {
				goto _ctr51;
			}
			case 114u: {
				goto _ctr51;
			}
			case 116u: {
				goto _ctr51;
			}
			case 117u: {
				goto _st18;
			}
		}
		goto _st0;
		_st18:
		p+= 1;
		st_case_18:
		if ( p == pe )
			goto _out18;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr53;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr53;
			}
		} else {
			goto _ctr53;
		}
		goto _st0;
		_ctr53:
		{
#line 56 "rl/json5_string_utf8.rl"
			
			hexchr0 = HEX2DEC((( (*( p)))));
		}
		
#line 1313 "json5_string_utf8.c"
		
		goto _st19;
		_st19:
		p+= 1;
		st_case_19:
		if ( p == pe )
			goto _out19;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr55;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr55;
			}
		} else {
			goto _ctr55;
		}
		goto _st0;
		_ctr55:
		{
#line 60 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
#line 1341 "json5_string_utf8.c"
		
		goto _st20;
		_st20:
		p+= 1;
		st_case_20:
		if ( p == pe )
			goto _out20;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr57;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr57;
			}
		} else {
			goto _ctr57;
		}
		goto _st0;
		_ctr57:
		{
#line 60 "rl/json5_string_utf8.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC((( (*( p)))));
		}
		
#line 1369 "json5_string_utf8.c"
		
		goto _st21;
		_st21:
		p+= 1;
		st_case_21:
		if ( p == pe )
			goto _out21;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr59;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr59;
			}
		} else {
			goto _ctr59;
		}
		goto _st0;
		_ctr41:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 1396 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1407 "json5_string_utf8.c"
		
		{
#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
#line 1413 "json5_string_utf8.c"
		
		goto _st22;
		_ctr47:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1426 "json5_string_utf8.c"
		
		{
#line 110 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & (0xdf-0xc0))) << 6; }
		
#line 1432 "json5_string_utf8.c"
		
		goto _st22;
		_st22:
		p+= 1;
		st_case_22:
		if ( p == pe )
			goto _out22;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr61;
		}
		goto _st0;
		_ctr42:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 1451 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1462 "json5_string_utf8.c"
		
		{
#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
#line 1468 "json5_string_utf8.c"
		
		goto _st23;
		_ctr48:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1481 "json5_string_utf8.c"
		
		{
#line 113 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x0f)) << 12; }
		
#line 1487 "json5_string_utf8.c"
		
		goto _st23;
		_st23:
		p+= 1;
		st_case_23:
		if ( p == pe )
			goto _out23;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr63;
		}
		goto _st0;
		_ctr63:
		{
#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
#line 1504 "json5_string_utf8.c"
		
		goto _st24;
		_st24:
		p+= 1;
		st_case_24:
		if ( p == pe )
			goto _out24;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr65;
		}
		goto _st0;
		_ctr43:
		{
#line 97 "rl/json5_string_utf8.rl"
			
			mark = p;
		}
		
#line 1523 "json5_string_utf8.c"
		
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1534 "json5_string_utf8.c"
		
		{
#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
#line 1540 "json5_string_utf8.c"
		
		goto _st25;
		_ctr49:
		{
#line 103 "rl/json5_string_utf8.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
			}
		}
		
#line 1553 "json5_string_utf8.c"
		
		{
#line 122 "rl/json5_string_utf8.rl"
			unicode = ((p_wchar2)((( (*( p)))) & 0x07)) << 18; }
		
#line 1559 "json5_string_utf8.c"
		
		goto _st25;
		_st25:
		p+= 1;
		st_case_25:
		if ( p == pe )
			goto _out25;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr67;
		}
		goto _st0;
		_ctr67:
		{
#line 123 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 12; }
		
#line 1576 "json5_string_utf8.c"
		
		goto _st26;
		_st26:
		p+= 1;
		st_case_26:
		if ( p == pe )
			goto _out26;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr69;
		}
		goto _st0;
		_ctr69:
		{
#line 114 "rl/json5_string_utf8.rl"
			unicode |= ((p_wchar2)((( (*( p)))) & (0xbf-0x80))) << 6; }
		
#line 1593 "json5_string_utf8.c"
		
		goto _st27;
		_st27:
		p+= 1;
		st_case_27:
		if ( p == pe )
			goto _out27;
		if ( 128u <= ( (*( p))) && ( (*( p))) <= 191u ) {
			goto _ctr71;
		}
		goto _st0;
		_st28:
		p+= 1;
		st_case_28:
		if ( p == pe )
			goto _out28;
		if ( ( (*( p))) == 92u ) {
			goto _st29;
		}
		goto _st0;
		_st29:
		p+= 1;
		st_case_29:
		if ( p == pe )
			goto _out29;
		if ( ( (*( p))) == 117u ) {
			goto _st30;
		}
		goto _st0;
		_st30:
		p+= 1;
		st_case_30:
		if ( p == pe )
			goto _out30;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr75;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr75;
			}
		} else {
			goto _ctr75;
		}
		goto _st0;
		_ctr75:
		{
#line 36 "rl/json5_string_utf8.rl"
			
			hexchr1 = HEX2DEC((( (*( p)))));
		}
		
#line 1647 "json5_string_utf8.c"
		
		goto _st31;
		_st31:
		p+= 1;
		st_case_31:
		if ( p == pe )
			goto _out31;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr77;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr77;
			}
		} else {
			goto _ctr77;
		}
		goto _st0;
		_ctr77:
		{
#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
#line 1675 "json5_string_utf8.c"
		
		goto _st32;
		_st32:
		p+= 1;
		st_case_32:
		if ( p == pe )
			goto _out32;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr79;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr79;
			}
		} else {
			goto _ctr79;
		}
		goto _st0;
		_ctr79:
		{
#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
#line 1703 "json5_string_utf8.c"
		
		goto _st33;
		_st33:
		p+= 1;
		st_case_33:
		if ( p == pe )
			goto _out33;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr81;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr81;
			}
		} else {
			goto _ctr81;
		}
		goto _st0;
		_st34:
		p+= 1;
		st_case_34:
		if ( p == pe )
			goto _out34;
		if ( ( (*( p))) == 92u ) {
			goto _st35;
		}
		goto _st0;
		_st35:
		p+= 1;
		st_case_35:
		if ( p == pe )
			goto _out35;
		if ( ( (*( p))) == 117u ) {
			goto _st36;
		}
		goto _st0;
		_st36:
		p+= 1;
		st_case_36:
		if ( p == pe )
			goto _out36;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr85;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr85;
			}
		} else {
			goto _ctr85;
		}
		goto _st0;
		_ctr85:
		{
#line 36 "rl/json5_string_utf8.rl"
			
			hexchr1 = HEX2DEC((( (*( p)))));
		}
		
#line 1765 "json5_string_utf8.c"
		
		goto _st37;
		_st37:
		p+= 1;
		st_case_37:
		if ( p == pe )
			goto _out37;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr87;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr87;
			}
		} else {
			goto _ctr87;
		}
		goto _st0;
		_ctr87:
		{
#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
#line 1793 "json5_string_utf8.c"
		
		goto _st38;
		_st38:
		p+= 1;
		st_case_38:
		if ( p == pe )
			goto _out38;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr89;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr89;
			}
		} else {
			goto _ctr89;
		}
		goto _st0;
		_ctr89:
		{
#line 40 "rl/json5_string_utf8.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC((( (*( p)))));
		}
		
#line 1821 "json5_string_utf8.c"
		
		goto _st39;
		_st39:
		p+= 1;
		st_case_39:
		if ( p == pe )
			goto _out39;
		if ( ( (*( p))) < 65u ) {
			if ( 48u <= ( (*( p))) && ( (*( p))) <= 57u ) {
				goto _ctr91;
			}
		} else if ( ( (*( p))) > 70u ) {
			if ( 97u <= ( (*( p))) && ( (*( p))) <= 102u ) {
				goto _ctr91;
			}
		} else {
			goto _ctr91;
		}
		goto _st0;
		_out1: cs = 1; goto _out; 
		_out0: cs = 0; goto _out; 
		_out2: cs = 2; goto _out; 
		_out3: cs = 3; goto _out; 
		_out40: cs = 40; goto _out; 
		_out4: cs = 4; goto _out; 
		_out5: cs = 5; goto _out; 
		_out6: cs = 6; goto _out; 
		_out7: cs = 7; goto _out; 
		_out8: cs = 8; goto _out; 
		_out9: cs = 9; goto _out; 
		_out10: cs = 10; goto _out; 
		_out11: cs = 11; goto _out; 
		_out12: cs = 12; goto _out; 
		_out13: cs = 13; goto _out; 
		_out14: cs = 14; goto _out; 
		_out15: cs = 15; goto _out; 
		_out16: cs = 16; goto _out; 
		_out41: cs = 41; goto _out; 
		_out17: cs = 17; goto _out; 
		_out18: cs = 18; goto _out; 
		_out19: cs = 19; goto _out; 
		_out20: cs = 20; goto _out; 
		_out21: cs = 21; goto _out; 
		_out22: cs = 22; goto _out; 
		_out23: cs = 23; goto _out; 
		_out24: cs = 24; goto _out; 
		_out25: cs = 25; goto _out; 
		_out26: cs = 26; goto _out; 
		_out27: cs = 27; goto _out; 
		_out28: cs = 28; goto _out; 
		_out29: cs = 29; goto _out; 
		_out30: cs = 30; goto _out; 
		_out31: cs = 31; goto _out; 
		_out32: cs = 32; goto _out; 
		_out33: cs = 33; goto _out; 
		_out34: cs = 34; goto _out; 
		_out35: cs = 35; goto _out; 
		_out36: cs = 36; goto _out; 
		_out37: cs = 37; goto _out; 
		_out38: cs = 38; goto _out; 
		_out39: cs = 39; goto _out; 
		_out: {}
	}
	
#line 201 "rl/json5_string_utf8.rl"
	
	
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
		err_msg = "Unterminated string";
		return start;
	}
	return p - (unsigned char*)(str.ptr);
}

#undef HEX2DEC
