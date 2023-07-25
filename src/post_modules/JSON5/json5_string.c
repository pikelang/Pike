#line 1 "rl/json5_string.rl"
/* -*- mode: C; c-basic-offset: 4; -*-
* vim:syntax=ragel
*/
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 148 "rl/json5_string.rl"


static ptrdiff_t _parse_JSON5_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	int hexchr0 = 0, hexchr1 = 0;
	ptrdiff_t start = p, mark = 0;
	struct string_builder s;
	int cs;
	ONERROR handle;
	const int validate = !(state->flags&JSON5_VALIDATE);
	
#line 19 "json5_string.c"
	static const int JSON5_string_start = 1;
	static const int JSON5_string_first_final = 28;
	static const int JSON5_string_error = 0;
	
	static const int JSON5_string_en_main = 1;
	static const int JSON5_string_en_main_hex1 = 16;
	static const int JSON5_string_en_main_hex3 = 22;
	
	
#line 156 "rl/json5_string.rl"
	
	
	if (validate) {
		init_string_builder(&s, 0);
		SET_ONERROR (handle, free_string_builder, &s);
	}
	
	
#line 38 "json5_string.c"
	{
		cs = (int)JSON5_string_start;
	}
	
#line 163 "rl/json5_string.rl"
	
	
#line 46 "json5_string.c"
	{
		goto _resume;
		
		_again: {}
		switch ( cs ) {
			case 1: goto _st1;
			case 0: goto _st0;
			case 2: goto _st2;
			case 3: goto _st3;
			case 28: goto _st28;
			case 4: goto _st4;
			case 5: goto _st5;
			case 6: goto _st6;
			case 7: goto _st7;
			case 8: goto _st8;
			case 9: goto _st9;
			case 10: goto _st10;
			case 29: goto _st29;
			case 11: goto _st11;
			case 12: goto _st12;
			case 13: goto _st13;
			case 14: goto _st14;
			case 15: goto _st15;
			case 16: goto _st16;
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
		_st1:
		p+= 1;
		st_case_1:
		if ( p == pe )
			goto _out1;
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto _st2;
			}
			case 39: {
				goto _st9;
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
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto _ctr5;
			}
			case 92: {
				goto _ctr6;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto _ctr4;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto _ctr4;
		}
		goto _st0;
		_ctr4:
		{
#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
#line 192 "json5_string.c"
		
		goto _st3;
		_ctr11:
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
		
#line 213 "json5_string.c"
		
		{
#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
#line 219 "json5_string.c"
		
		goto _st3;
		_ctr19:
		cs = 3;
		{
#line 15 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 231 "json5_string.c"
		
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
		
#line 251 "json5_string.c"
		
		{
#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
#line 257 "json5_string.c"
		
		goto _again;
		_ctr45:
		{
#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 268 "json5_string.c"
		
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
		
#line 283 "json5_string.c"
		
		{
#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
#line 289 "json5_string.c"
		
		goto _st3;
		_st3:
		p+= 1;
		st_case_3:
		if ( p == pe )
			goto _out3;
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 34: {
				goto _ctr8;
			}
			case 92: {
				goto _ctr9;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto _st3;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto _st3;
		}
		goto _st0;
		_ctr5:
		{
#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
#line 320 "json5_string.c"
		
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 331 "json5_string.c"
		
		goto _st28;
		_ctr8:
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 344 "json5_string.c"
		
		goto _st28;
		_st28:
		p+= 1;
		st_case_28:
		if ( p == pe )
			goto _out28;
		{
#line 130 "rl/json5_string.rl"
			p--; {p+= 1; cs = 28; goto _out;} }
		
#line 356 "json5_string.c"
		
		goto _st0;
		_ctr6:
		{
#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
#line 366 "json5_string.c"
		
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 377 "json5_string.c"
		
		goto _st4;
		_ctr9:
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 390 "json5_string.c"
		
		goto _st4;
		_st4:
		p+= 1;
		st_case_4:
		if ( p == pe )
			goto _out4;
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto _ctr11;
			}
			case 34: {
				goto _ctr11;
			}
			case 39: {
				goto _ctr11;
			}
			case 47: {
				goto _ctr11;
			}
			case 92: {
				goto _ctr11;
			}
			case 98: {
				goto _ctr11;
			}
			case 102: {
				goto _ctr11;
			}
			case 110: {
				goto _ctr11;
			}
			case 114: {
				goto _ctr11;
			}
			case 116: {
				goto _ctr11;
			}
			case 117: {
				goto _st5;
			}
		}
		goto _st0;
		_st5:
		p+= 1;
		st_case_5:
		if ( p == pe )
			goto _out5;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr13;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr13;
			}
		} else {
			goto _ctr13;
		}
		goto _st0;
		_ctr13:
		{
#line 11 "rl/json5_string.rl"
			
			hexchr0 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 458 "json5_string.c"
		
		goto _st6;
		_st6:
		p+= 1;
		st_case_6:
		if ( p == pe )
			goto _out6;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr15;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr15;
			}
		} else {
			goto _ctr15;
		}
		goto _st0;
		_ctr15:
		{
#line 15 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 486 "json5_string.c"
		
		goto _st7;
		_st7:
		p+= 1;
		st_case_7:
		if ( p == pe )
			goto _out7;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr17;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr17;
			}
		} else {
			goto _ctr17;
		}
		goto _st0;
		_ctr17:
		{
#line 15 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 514 "json5_string.c"
		
		goto _st8;
		_st8:
		p+= 1;
		st_case_8:
		if ( p == pe )
			goto _out8;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr19;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr19;
			}
		} else {
			goto _ctr19;
		}
		goto _st0;
		_st9:
		p+= 1;
		st_case_9:
		if ( p == pe )
			goto _out9;
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 39: {
				goto _ctr21;
			}
			case 92: {
				goto _ctr22;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto _ctr20;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto _ctr20;
		}
		goto _st0;
		_ctr20:
		{
#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
#line 562 "json5_string.c"
		
		goto _st10;
		_ctr27:
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
		
#line 583 "json5_string.c"
		
		{
#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
#line 589 "json5_string.c"
		
		goto _st10;
		_ctr35:
		cs = 10;
		{
#line 61 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 601 "json5_string.c"
		
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
		
#line 621 "json5_string.c"
		
		{
#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
#line 627 "json5_string.c"
		
		goto _again;
		_ctr55:
		{
#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 638 "json5_string.c"
		
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
		
#line 653 "json5_string.c"
		
		{
#line 102 "rl/json5_string.rl"
			mark = p + 1; }
		
#line 659 "json5_string.c"
		
		goto _st10;
		_st10:
		p+= 1;
		st_case_10:
		if ( p == pe )
			goto _out10;
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 39: {
				goto _ctr24;
			}
			case 92: {
				goto _ctr25;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 55295 ) {
			if ( 57344 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 1114111 ) {
				goto _st10;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 32 ) {
			goto _st10;
		}
		goto _st0;
		_ctr21:
		{
#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
#line 690 "json5_string.c"
		
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 701 "json5_string.c"
		
		goto _st29;
		_ctr24:
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 714 "json5_string.c"
		
		goto _st29;
		_st29:
		p+= 1;
		st_case_29:
		if ( p == pe )
			goto _out29;
		{
#line 145 "rl/json5_string.rl"
			p--; {p+= 1; cs = 29; goto _out;} }
		
#line 726 "json5_string.c"
		
		goto _st0;
		_ctr22:
		{
#line 98 "rl/json5_string.rl"
			
			mark = p;
		}
		
#line 736 "json5_string.c"
		
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 747 "json5_string.c"
		
		goto _st11;
		_ctr25:
		{
#line 104 "rl/json5_string.rl"
			
			if (p - mark > 0) {
				if (validate)
				string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
			}
		}
		
#line 760 "json5_string.c"
		
		goto _st11;
		_st11:
		p+= 1;
		st_case_11:
		if ( p == pe )
			goto _out11;
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto _ctr27;
			}
			case 39: {
				goto _ctr27;
			}
			case 47: {
				goto _ctr27;
			}
			case 92: {
				goto _ctr27;
			}
			case 98: {
				goto _ctr27;
			}
			case 102: {
				goto _ctr27;
			}
			case 110: {
				goto _ctr27;
			}
			case 114: {
				goto _ctr27;
			}
			case 116: {
				goto _ctr27;
			}
			case 117: {
				goto _st12;
			}
		}
		goto _st0;
		_st12:
		p+= 1;
		st_case_12:
		if ( p == pe )
			goto _out12;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr29;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr29;
			}
		} else {
			goto _ctr29;
		}
		goto _st0;
		_ctr29:
		{
#line 57 "rl/json5_string.rl"
			
			hexchr0 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 825 "json5_string.c"
		
		goto _st13;
		_st13:
		p+= 1;
		st_case_13:
		if ( p == pe )
			goto _out13;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr31;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr31;
			}
		} else {
			goto _ctr31;
		}
		goto _st0;
		_ctr31:
		{
#line 61 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 853 "json5_string.c"
		
		goto _st14;
		_st14:
		p+= 1;
		st_case_14:
		if ( p == pe )
			goto _out14;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr33;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr33;
			}
		} else {
			goto _ctr33;
		}
		goto _st0;
		_ctr33:
		{
#line 61 "rl/json5_string.rl"
			
			hexchr0 *= 16;
			hexchr0 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 881 "json5_string.c"
		
		goto _st15;
		_st15:
		p+= 1;
		st_case_15:
		if ( p == pe )
			goto _out15;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr35;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr35;
			}
		} else {
			goto _ctr35;
		}
		goto _st0;
		_st16:
		p+= 1;
		st_case_16:
		if ( p == pe )
			goto _out16;
		if ( (((int)INDEX_PCHARP(str, p))) == 92 ) {
			goto _st17;
		}
		goto _st0;
		_st17:
		p+= 1;
		st_case_17:
		if ( p == pe )
			goto _out17;
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto _st18;
		}
		goto _st0;
		_st18:
		p+= 1;
		st_case_18:
		if ( p == pe )
			goto _out18;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr39;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr39;
			}
		} else {
			goto _ctr39;
		}
		goto _st0;
		_ctr39:
		{
#line 37 "rl/json5_string.rl"
			
			hexchr1 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 943 "json5_string.c"
		
		goto _st19;
		_st19:
		p+= 1;
		st_case_19:
		if ( p == pe )
			goto _out19;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr41;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr41;
			}
		} else {
			goto _ctr41;
		}
		goto _st0;
		_ctr41:
		{
#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 971 "json5_string.c"
		
		goto _st20;
		_st20:
		p+= 1;
		st_case_20:
		if ( p == pe )
			goto _out20;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr43;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr43;
			}
		} else {
			goto _ctr43;
		}
		goto _st0;
		_ctr43:
		{
#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 999 "json5_string.c"
		
		goto _st21;
		_st21:
		p+= 1;
		st_case_21:
		if ( p == pe )
			goto _out21;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr45;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr45;
			}
		} else {
			goto _ctr45;
		}
		goto _st0;
		_st22:
		p+= 1;
		st_case_22:
		if ( p == pe )
			goto _out22;
		if ( (((int)INDEX_PCHARP(str, p))) == 92 ) {
			goto _st23;
		}
		goto _st0;
		_st23:
		p+= 1;
		st_case_23:
		if ( p == pe )
			goto _out23;
		if ( (((int)INDEX_PCHARP(str, p))) == 117 ) {
			goto _st24;
		}
		goto _st0;
		_st24:
		p+= 1;
		st_case_24:
		if ( p == pe )
			goto _out24;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr49;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr49;
			}
		} else {
			goto _ctr49;
		}
		goto _st0;
		_ctr49:
		{
#line 37 "rl/json5_string.rl"
			
			hexchr1 = HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 1061 "json5_string.c"
		
		goto _st25;
		_st25:
		p+= 1;
		st_case_25:
		if ( p == pe )
			goto _out25;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr51;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr51;
			}
		} else {
			goto _ctr51;
		}
		goto _st0;
		_ctr51:
		{
#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 1089 "json5_string.c"
		
		goto _st26;
		_st26:
		p+= 1;
		st_case_26:
		if ( p == pe )
			goto _out26;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr53;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr53;
			}
		} else {
			goto _ctr53;
		}
		goto _st0;
		_ctr53:
		{
#line 41 "rl/json5_string.rl"
			
			hexchr1 *= 16;
			hexchr1 += HEX2DEC(((((int)INDEX_PCHARP(str, p)))));
		}
		
#line 1117 "json5_string.c"
		
		goto _st27;
		_st27:
		p+= 1;
		st_case_27:
		if ( p == pe )
			goto _out27;
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto _ctr55;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto _ctr55;
			}
		} else {
			goto _ctr55;
		}
		goto _st0;
		_out1: cs = 1; goto _out; 
		_out0: cs = 0; goto _out; 
		_out2: cs = 2; goto _out; 
		_out3: cs = 3; goto _out; 
		_out28: cs = 28; goto _out; 
		_out4: cs = 4; goto _out; 
		_out5: cs = 5; goto _out; 
		_out6: cs = 6; goto _out; 
		_out7: cs = 7; goto _out; 
		_out8: cs = 8; goto _out; 
		_out9: cs = 9; goto _out; 
		_out10: cs = 10; goto _out; 
		_out29: cs = 29; goto _out; 
		_out11: cs = 11; goto _out; 
		_out12: cs = 12; goto _out; 
		_out13: cs = 13; goto _out; 
		_out14: cs = 14; goto _out; 
		_out15: cs = 15; goto _out; 
		_out16: cs = 16; goto _out; 
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
		_out: {}
	}
	
#line 164 "rl/json5_string.rl"
	
	
	if (cs < JSON5_string_first_final) {
		if (validate) {
			UNSET_ONERROR(handle);
			free_string_builder(&s);
		}
		
		state->flags |= JSON5_ERROR;
		if (p == pe) {
			err_msg = "Unterminated string";
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
