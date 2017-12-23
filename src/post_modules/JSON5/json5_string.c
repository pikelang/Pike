
#line 1 "rl/json5_string.rl"
/* vim:syntax=ragel
 */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 148 "rl/json5_string.rl"


static ptrdiff_t _parse_JSON5_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    int hexchr0, hexchr1;
    ptrdiff_t start = p, mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;
    const int validate = !(state->flags&JSON5_VALIDATE);
    
#line 20 "json5_string.c"
static const int JSON5_string_start = 1;
static const int JSON5_string_first_final = 28;
static const int JSON5_string_error = 0;

static const int JSON5_string_en_main = 1;
static const int JSON5_string_en_main_hex1 = 16;
static const int JSON5_string_en_main_hex3 = 22;


#line 158 "rl/json5_string.rl"

    if (validate) {
	init_string_builder(&s, 0);
	SET_ONERROR (handle, free_string_builder, &s);
    }

    
#line 38 "json5_string.c"
	{
	cs = JSON5_string_start;
	}

#line 165 "rl/json5_string.rl"
    
#line 45 "json5_string.c"
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
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 34: goto st2;
		case 39: goto st9;
	}
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 34: goto tr4;
		case 92: goto tr5;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 55295 ) {
		if ( 57344 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 1114111 )
			goto tr3;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 32 )
		goto tr3;
	goto st0;
tr3:
#line 98 "rl/json5_string.rl"
	{
	mark = p;
    }
	goto st3;
tr9:
#line 83 "rl/json5_string.rl"
	{
	if (validate) switch(( ((int)INDEX_PCHARP(str, p)))) {
            case '\n':
	    case '\'':
	    case '"':
	    case '/':
	    case '\\':      string_builder_putchar(&s, ( ((int)INDEX_PCHARP(str, p)))); break;
	    case 'b':       string_builder_putchar(&s, '\b'); break;
	    case 'f':       string_builder_putchar(&s, '\f'); break;
	    case 'n':       string_builder_putchar(&s, '\n'); break;
	    case 'r':       string_builder_putchar(&s, '\r'); break;
	    case 't':       string_builder_putchar(&s, '\t'); break;
	}
    }
#line 102 "rl/json5_string.rl"
	{ mark = p + 1; }
	goto st3;
tr14:
	cs = 3;
#line 15 "rl/json5_string.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 20 "rl/json5_string.rl"
	{
	if (IS_HIGH_SURROGATE (hexchr0)) {
	    /* Chars outside the BMP can be expressed as two hex
	     * escapes that codes a surrogate pair, so see if we can
	     * read a second one. */
	    cs = 16;
	}
	else {
	    if (IS_NUNICODE(hexchr0)) {
		p--; {p++; goto _out;}
	    }
	    if (validate) {
		string_builder_putchar(&s, hexchr0);
	    }
	}
    }
#line 102 "rl/json5_string.rl"
	{ mark = p + 1; }
	goto _again;
tr32:
#line 41 "rl/json5_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 46 "rl/json5_string.rl"
	{
	if (!IS_LOW_SURROGATE (hexchr1)) {
	    p--; {p++; cs = 3; goto _out;}
	}
	if (validate) {
	    int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
		0x10000;
	    string_builder_putchar(&s, cp);
	}
    }
#line 102 "rl/json5_string.rl"
	{ mark = p + 1; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 193 "json5_string.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 34: goto tr7;
		case 92: goto tr8;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 55295 ) {
		if ( 57344 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 1114111 )
			goto st3;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 32 )
		goto st3;
	goto st0;
tr4:
#line 98 "rl/json5_string.rl"
	{
	mark = p;
    }
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st28;
tr7:
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st28;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
#line 130 "rl/json5_string.rl"
	{ p--; {p++; cs = 28; goto _out;} }
#line 232 "json5_string.c"
	goto st0;
tr5:
#line 98 "rl/json5_string.rl"
	{
	mark = p;
    }
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st4;
tr8:
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 260 "json5_string.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 10: goto tr9;
		case 34: goto tr9;
		case 39: goto tr9;
		case 47: goto tr9;
		case 92: goto tr9;
		case 98: goto tr9;
		case 102: goto tr9;
		case 110: goto tr9;
		case 114: goto tr9;
		case 116: goto tr9;
		case 117: goto st5;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr11;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr11;
	} else
		goto tr11;
	goto st0;
tr11:
#line 11 "rl/json5_string.rl"
	{
	hexchr0 = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 298 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr12;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr12;
	} else
		goto tr12;
	goto st0;
tr12:
#line 15 "rl/json5_string.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 319 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr13;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr13;
	} else
		goto tr13;
	goto st0;
tr13:
#line 15 "rl/json5_string.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 340 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr14;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr14;
	} else
		goto tr14;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 39: goto tr16;
		case 92: goto tr17;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 55295 ) {
		if ( 57344 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 1114111 )
			goto tr15;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 32 )
		goto tr15;
	goto st0;
tr15:
#line 98 "rl/json5_string.rl"
	{
	mark = p;
    }
	goto st10;
tr21:
#line 83 "rl/json5_string.rl"
	{
	if (validate) switch(( ((int)INDEX_PCHARP(str, p)))) {
            case '\n':
	    case '\'':
	    case '"':
	    case '/':
	    case '\\':      string_builder_putchar(&s, ( ((int)INDEX_PCHARP(str, p)))); break;
	    case 'b':       string_builder_putchar(&s, '\b'); break;
	    case 'f':       string_builder_putchar(&s, '\f'); break;
	    case 'n':       string_builder_putchar(&s, '\n'); break;
	    case 'r':       string_builder_putchar(&s, '\r'); break;
	    case 't':       string_builder_putchar(&s, '\t'); break;
	}
    }
#line 102 "rl/json5_string.rl"
	{ mark = p + 1; }
	goto st10;
tr26:
	cs = 10;
#line 61 "rl/json5_string.rl"
	{
        hexchr0 *= 16;
        hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 66 "rl/json5_string.rl"
	{
        if (IS_HIGH_SURROGATE (hexchr0)) {
            /* Chars outside the BMP can be expressed as two hex
             * escapes that codes a surrogate pair, so see if we can
             * read a second one. */
            cs = 22;
        }
        else {
            if (IS_NUNICODE(hexchr0)) {
                p--; {p++; goto _out;}
            }
            if (validate) {
                string_builder_putchar(&s, hexchr0);
            }
        }
    }
#line 102 "rl/json5_string.rl"
	{ mark = p + 1; }
	goto _again;
tr38:
#line 41 "rl/json5_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 46 "rl/json5_string.rl"
	{
	if (!IS_LOW_SURROGATE (hexchr1)) {
	    p--; {p++; cs = 10; goto _out;}
	}
	if (validate) {
	    int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
		0x10000;
	    string_builder_putchar(&s, cp);
	}
    }
#line 102 "rl/json5_string.rl"
	{ mark = p + 1; }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 440 "json5_string.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 39: goto tr19;
		case 92: goto tr20;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 55295 ) {
		if ( 57344 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 1114111 )
			goto st10;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 32 )
		goto st10;
	goto st0;
tr16:
#line 98 "rl/json5_string.rl"
	{
	mark = p;
    }
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st29;
tr19:
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st29;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
#line 146 "rl/json5_string.rl"
	{ p--; {p++; cs = 29; goto _out;} }
#line 479 "json5_string.c"
	goto st0;
tr17:
#line 98 "rl/json5_string.rl"
	{
	mark = p;
    }
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st11;
tr20:
#line 104 "rl/json5_string.rl"
	{
	if (p - mark > 0) {
	    if (validate)
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 507 "json5_string.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 10: goto st10;
		case 34: goto tr21;
		case 39: goto tr21;
		case 47: goto tr21;
		case 92: goto tr21;
		case 98: goto tr21;
		case 102: goto tr21;
		case 110: goto tr21;
		case 114: goto tr21;
		case 116: goto tr21;
		case 117: goto st12;
	}
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr23;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr23;
	} else
		goto tr23;
	goto st0;
tr23:
#line 57 "rl/json5_string.rl"
	{
        hexchr0 = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 545 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr24;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr24;
	} else
		goto tr24;
	goto st0;
tr24:
#line 61 "rl/json5_string.rl"
	{
        hexchr0 *= 16;
        hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 566 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr25;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr25;
	} else
		goto tr25;
	goto st0;
tr25:
#line 61 "rl/json5_string.rl"
	{
        hexchr0 *= 16;
        hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 587 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr26;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr26;
	} else
		goto tr26;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 92 )
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 117 )
		goto st18;
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr29;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr29;
	} else
		goto tr29;
	goto st0;
tr29:
#line 37 "rl/json5_string.rl"
	{
	hexchr1 = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st19;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
#line 634 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr30;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr30;
	} else
		goto tr30;
	goto st0;
tr30:
#line 41 "rl/json5_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 655 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr31;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr31;
	} else
		goto tr31;
	goto st0;
tr31:
#line 41 "rl/json5_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st21;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
#line 676 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr32;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr32;
	} else
		goto tr32;
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 92 )
		goto st23;
	goto st0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 117 )
		goto st24;
	goto st0;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr35;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr35;
	} else
		goto tr35;
	goto st0;
tr35:
#line 37 "rl/json5_string.rl"
	{
	hexchr1 = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st25;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
#line 723 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr36;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr36;
	} else
		goto tr36;
	goto st0;
tr36:
#line 41 "rl/json5_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st26;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
#line 744 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr37;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr37;
	} else
		goto tr37;
	goto st0;
tr37:
#line 41 "rl/json5_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 765 "json5_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr38;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr38;
	} else
		goto tr38;
	goto st0;
	}
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

#line 166 "rl/json5_string.rl"

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
