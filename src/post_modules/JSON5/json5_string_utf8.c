
#line 1 "rl/json5_string_utf8.rl"
/* vim:syntax=ragel
 */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 179 "rl/json5_string_utf8.rl"


static ptrdiff_t _parse_JSON5_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
    unsigned char *p = (unsigned char*)(str.ptr) + pos;
    unsigned char *pe = (unsigned char*)(str.ptr) + end;
    ptrdiff_t start = pos;
    unsigned char *mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;
    int hexchr0, hexchr1;
    const int validate = !(state->flags&JSON5_VALIDATE);
    p_wchar2 unicode = 0;

    
#line 25 "json5_string_utf8.c"
static const int JSON5_string_start = 1;
static const int JSON5_string_first_final = 40;
static const int JSON5_string_error = 0;

static const int JSON5_string_en_main = 1;
static const int JSON5_string_en_main_hex1 = 28;
static const int JSON5_string_en_main_hex3 = 34;


#line 194 "rl/json5_string_utf8.rl"

    if (validate) {
	init_string_builder(&s, 0);
	SET_ONERROR(handle, free_string_builder, &s);
    }

    
#line 43 "json5_string_utf8.c"
	{
	cs = JSON5_string_start;
	}

#line 201 "rl/json5_string_utf8.rl"
    
#line 50 "json5_string_utf8.c"
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
	default: break;
	}

	if ( ++p == pe )
		goto _test_eof;
_resume:
	switch ( cs )
	{
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 34u: goto st2;
		case 39u: goto st15;
	}
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 34u: goto tr4;
		case 92u: goto tr5;
	}
	if ( (*p) < 194u ) {
		if ( 32u <= (*p) && (*p) <= 127u )
			goto tr3;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr8;
		} else if ( (*p) >= 224u )
			goto tr7;
	} else
		goto tr6;
	goto st0;
tr3:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
	goto st3;
tr15:
#line 82 "rl/json5_string_utf8.rl"
	{
	if (validate) switch((*p)) {
	    case '\n':
	    case '\'':
	    case '"':
	    case '/':
	    case '\\':      string_builder_putchar(&s, (*p)); break;
	    case 'b':       string_builder_putchar(&s, '\b'); break;
	    case 'f':       string_builder_putchar(&s, '\f'); break;
	    case 'n':       string_builder_putchar(&s, '\n'); break;
	    case 'r':       string_builder_putchar(&s, '\r'); break;
	    case 't':       string_builder_putchar(&s, '\t'); break;
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr20:
	cs = 3;
#line 14 "rl/json5_string_utf8.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC((*p));
    }
#line 19 "rl/json5_string_utf8.rl"
	{
	if (IS_HIGH_SURROGATE (hexchr0)) {
	    /* Chars outside the BMP can be expressed as two hex
	     * escapes that codes a surrogate pair, so see if we can
	     * read a second one. */
	    cs = 28;
	}
	else {
	    if (IS_NUNICODE(hexchr0)) {
		goto failure;
	    }
	    if (validate) {
		string_builder_putchar(&s, hexchr0);
	    }
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto _again;
tr21:
#line 111 "rl/json5_string_utf8.rl"
	{ unicode |= (p_wchar2)((*p) & (0xbf-0x80)); }
#line 139 "rl/json5_string_utf8.rl"
	{
	if (validate) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr23:
#line 115 "rl/json5_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
	if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
	    goto failure;	
	}
    }
#line 139 "rl/json5_string_utf8.rl"
	{
	if (validate) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr26:
#line 126 "rl/json5_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
	if (unicode < 0x010000 || unicode > 0x10ffff) {
	    goto failure;
	}
    }
#line 139 "rl/json5_string_utf8.rl"
	{
	if (validate) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr56:
#line 40 "rl/json5_string_utf8.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC((*p));
    }
#line 45 "rl/json5_string_utf8.rl"
	{
	if (!IS_LOW_SURROGATE (hexchr1)) {
	    goto failure;
	}
	if (validate) {
	    int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
		0x10000;
	    string_builder_putchar(&s, cp);
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 262 "json5_string_utf8.c"
	switch( (*p) ) {
		case 34u: goto tr10;
		case 92u: goto tr11;
	}
	if ( (*p) < 194u ) {
		if ( 32u <= (*p) && (*p) <= 127u )
			goto st3;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr14;
		} else if ( (*p) >= 224u )
			goto tr13;
	} else
		goto tr12;
	goto st0;
tr4:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st40;
tr10:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st40;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
#line 161 "rl/json5_string_utf8.rl"
	{ p--; {p++; cs = 40; goto _out;} }
#line 307 "json5_string_utf8.c"
	goto st0;
tr5:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st4;
tr11:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 335 "json5_string_utf8.c"
	switch( (*p) ) {
		case 10u: goto tr15;
		case 34u: goto tr15;
		case 39u: goto tr15;
		case 47u: goto tr15;
		case 92u: goto tr15;
		case 98u: goto tr15;
		case 102u: goto tr15;
		case 110u: goto tr15;
		case 114u: goto tr15;
		case 116u: goto tr15;
		case 117u: goto st5;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr17;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr17;
	} else
		goto tr17;
	goto st0;
tr17:
#line 10 "rl/json5_string_utf8.rl"
	{
	hexchr0 = HEX2DEC((*p));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 373 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr18;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr18;
	} else
		goto tr18;
	goto st0;
tr18:
#line 14 "rl/json5_string_utf8.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC((*p));
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 394 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr19;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr19;
	} else
		goto tr19;
	goto st0;
tr19:
#line 14 "rl/json5_string_utf8.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC((*p));
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 415 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr20;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr20;
	} else
		goto tr20;
	goto st0;
tr6:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 110 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st9;
tr12:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 110 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 455 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr21;
	goto st0;
tr7:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 113 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st10;
tr13:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 113 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 489 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr22;
	goto st0;
tr22:
#line 114 "rl/json5_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 501 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr23;
	goto st0;
tr8:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 122 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st12;
tr14:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 122 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 535 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr24;
	goto st0;
tr24:
#line 123 "rl/json5_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 12; }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 547 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr25;
	goto st0;
tr25:
#line 114 "rl/json5_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 559 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr26;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	switch( (*p) ) {
		case 39u: goto tr28;
		case 92u: goto tr29;
	}
	if ( (*p) < 194u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 127u )
				goto tr27;
		} else if ( (*p) >= 32u )
			goto tr27;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr32;
		} else if ( (*p) >= 224u )
			goto tr31;
	} else
		goto tr30;
	goto st0;
tr27:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
	goto st16;
tr39:
#line 82 "rl/json5_string_utf8.rl"
	{
	if (validate) switch((*p)) {
	    case '\n':
	    case '\'':
	    case '"':
	    case '/':
	    case '\\':      string_builder_putchar(&s, (*p)); break;
	    case 'b':       string_builder_putchar(&s, '\b'); break;
	    case 'f':       string_builder_putchar(&s, '\f'); break;
	    case 'n':       string_builder_putchar(&s, '\n'); break;
	    case 'r':       string_builder_putchar(&s, '\r'); break;
	    case 't':       string_builder_putchar(&s, '\t'); break;
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st16;
tr45:
#line 111 "rl/json5_string_utf8.rl"
	{ unicode |= (p_wchar2)((*p) & (0xbf-0x80)); }
#line 139 "rl/json5_string_utf8.rl"
	{
	if (validate) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st16;
tr47:
#line 115 "rl/json5_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
	if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
	    goto failure;	
	}
    }
#line 139 "rl/json5_string_utf8.rl"
	{
	if (validate) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st16;
tr50:
#line 126 "rl/json5_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
	if (unicode < 0x010000 || unicode > 0x10ffff) {
	    goto failure;
	}
    }
#line 139 "rl/json5_string_utf8.rl"
	{
	if (validate) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st16;
tr44:
	cs = 16;
#line 60 "rl/json5_string_utf8.rl"
	{
        hexchr0 *= 16;
        hexchr0 += HEX2DEC((*p));
    }
#line 65 "rl/json5_string_utf8.rl"
	{
        if (IS_HIGH_SURROGATE (hexchr0)) {
            /* Chars outside the BMP can be expressed as two hex 
             * escapes that codes a surrogate pair, so see if we can
             * read a second one. */
            cs = 34;
        }
        else { 
            if (IS_NUNICODE(hexchr0)) {
                goto failure;
            }
            if (validate) {
               string_builder_putchar(&s, hexchr0);
            }
        }
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto _again;
tr62:
#line 40 "rl/json5_string_utf8.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC((*p));
    }
#line 45 "rl/json5_string_utf8.rl"
	{
	if (!IS_LOW_SURROGATE (hexchr1)) {
	    goto failure;
	}
	if (validate) {
	    int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
		0x10000;
	    string_builder_putchar(&s, cp);
	}
    }
#line 101 "rl/json5_string_utf8.rl"
	{ mark = p + 1; }
	goto st16;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
#line 708 "json5_string_utf8.c"
	switch( (*p) ) {
		case 39u: goto tr34;
		case 92u: goto tr35;
	}
	if ( (*p) < 194u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 127u )
				goto st16;
		} else if ( (*p) >= 32u )
			goto st16;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr38;
		} else if ( (*p) >= 224u )
			goto tr37;
	} else
		goto tr36;
	goto st0;
tr28:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st41;
tr34:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st41;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
#line 178 "rl/json5_string_utf8.rl"
	{ p--; {p++; cs = 41; goto _out;} }
#line 756 "json5_string_utf8.c"
	switch( (*p) ) {
		case 39u: goto tr34;
		case 92u: goto tr35;
	}
	if ( (*p) < 194u ) {
		if ( (*p) > 33u ) {
			if ( 35u <= (*p) && (*p) <= 127u )
				goto st16;
		} else if ( (*p) >= 32u )
			goto st16;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr38;
		} else if ( (*p) >= 224u )
			goto tr37;
	} else
		goto tr36;
	goto st0;
tr29:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st17;
tr35:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st17;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
#line 802 "json5_string_utf8.c"
	switch( (*p) ) {
		case 10u: goto tr39;
		case 39u: goto tr39;
		case 47u: goto tr39;
		case 92u: goto tr39;
		case 98u: goto tr39;
		case 102u: goto tr39;
		case 110u: goto tr39;
		case 114u: goto tr39;
		case 116u: goto tr39;
		case 117u: goto st18;
	}
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr41;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr41;
	} else
		goto tr41;
	goto st0;
tr41:
#line 56 "rl/json5_string_utf8.rl"
	{
        hexchr0 = HEX2DEC((*p));
    }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 839 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr42;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr42;
	} else
		goto tr42;
	goto st0;
tr42:
#line 60 "rl/json5_string_utf8.rl"
	{
        hexchr0 *= 16;
        hexchr0 += HEX2DEC((*p));
    }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 860 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr43;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr43;
	} else
		goto tr43;
	goto st0;
tr43:
#line 60 "rl/json5_string_utf8.rl"
	{
        hexchr0 *= 16;
        hexchr0 += HEX2DEC((*p));
    }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 881 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr44;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr44;
	} else
		goto tr44;
	goto st0;
tr30:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 110 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st22;
tr36:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 110 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st22;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
#line 921 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr45;
	goto st0;
tr31:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 113 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st23;
tr37:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 113 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st23;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
#line 955 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr46;
	goto st0;
tr46:
#line 114 "rl/json5_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st24;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
#line 967 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr47;
	goto st0;
tr32:
#line 97 "rl/json5_string_utf8.rl"
	{
	mark = p;
    }
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 122 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st25;
tr38:
#line 103 "rl/json5_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 122 "rl/json5_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 1001 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr48;
	goto st0;
tr48:
#line 123 "rl/json5_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 12; }
	goto st26;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
#line 1013 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr49;
	goto st0;
tr49:
#line 114 "rl/json5_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 1025 "json5_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr50;
	goto st0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	if ( (*p) == 92u )
		goto st29;
	goto st0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	if ( (*p) == 117u )
		goto st30;
	goto st0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr53;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr53;
	} else
		goto tr53;
	goto st0;
tr53:
#line 36 "rl/json5_string_utf8.rl"
	{
	hexchr1 = HEX2DEC((*p));
    }
	goto st31;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
#line 1066 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr54;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr54;
	} else
		goto tr54;
	goto st0;
tr54:
#line 40 "rl/json5_string_utf8.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC((*p));
    }
	goto st32;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
#line 1087 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr55;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr55;
	} else
		goto tr55;
	goto st0;
tr55:
#line 40 "rl/json5_string_utf8.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC((*p));
    }
	goto st33;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
#line 1108 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr56;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr56;
	} else
		goto tr56;
	goto st0;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	if ( (*p) == 92u )
		goto st35;
	goto st0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( (*p) == 117u )
		goto st36;
	goto st0;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr59;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr59;
	} else
		goto tr59;
	goto st0;
tr59:
#line 36 "rl/json5_string_utf8.rl"
	{
	hexchr1 = HEX2DEC((*p));
    }
	goto st37;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
#line 1155 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr60;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr60;
	} else
		goto tr60;
	goto st0;
tr60:
#line 40 "rl/json5_string_utf8.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC((*p));
    }
	goto st38;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
#line 1176 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr61;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr61;
	} else
		goto tr61;
	goto st0;
tr61:
#line 40 "rl/json5_string_utf8.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC((*p));
    }
	goto st39;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
#line 1197 "json5_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr62;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr62;
	} else
		goto tr62;
	goto st0;
	}
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
	err_msg = "Unterminated string";
	return start;
    }
    return p - (unsigned char*)(str.ptr);
}

#undef HEX2DEC
