
#line 1 "rl/json_string.rl"
/* vim:syntax=ragel */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 96 "rl/json_string.rl"


static ptrdiff_t _parse_JSON_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    int hexchr0, hexchr1;
    ptrdiff_t start = p, mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;

    
#line 19 "json_string.c"
static const int JSON_string_start = 1;
static const int JSON_string_first_final = 15;
static const int JSON_string_error = 0;

static const int JSON_string_en_main = 1;
static const int JSON_string_en_main_hex1 = 9;


#line 106 "rl/json_string.rl"

    if (!(state->flags&JSON_VALIDATE)) {
	init_string_builder(&s, 0);
	SET_ONERROR (handle, free_string_builder, &s);
    }

    
#line 36 "json_string.c"
	{
	cs = JSON_string_start;
	}

#line 113 "rl/json_string.rl"
    
#line 43 "json_string.c"
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
	if ( ( ((int)INDEX_PCHARP(str, p))) == 34 )
		goto st2;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 34: goto tr3;
		case 92: goto tr4;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 55159 ) {
		if ( 57344 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 1114111 )
			goto tr2;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 32 )
		goto tr2;
	goto st0;
tr2:
#line 69 "rl/json_string.rl"
	{
	mark = p;
    }
	goto st3;
tr8:
#line 56 "rl/json_string.rl"
	{
	if (!(state->flags&JSON_VALIDATE)) switch(( ((int)INDEX_PCHARP(str, p)))) {
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
#line 73 "rl/json_string.rl"
	{ mark = p + 1; }
	goto st3;
tr13:
	cs = 3;
#line 14 "rl/json_string.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 19 "rl/json_string.rl"
	{
	if (IS_HIGH_SURROGATE (hexchr0)) {
	    /* Chars outside the BMP can be expressed as two hex
	     * escapes that codes a surrogate pair, so see if we can
	     * read a second one. */
	    cs = 9;
	}
	else {
	    if (IS_NUNICODE(hexchr0)) {
		p--; {p++; goto _out;}
	    }
	    if (!(state->flags&JSON_VALIDATE)) {
		string_builder_putchar(&s, hexchr0);
	    }
	}
    }
#line 73 "rl/json_string.rl"
	{ mark = p + 1; }
	goto _again;
tr19:
#line 40 "rl/json_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 45 "rl/json_string.rl"
	{
	if (!IS_LOW_SURROGATE (hexchr1)) {
	    p--; {p++; cs = 3; goto _out;}
	}
	if (!(state->flags&JSON_VALIDATE)) {
	    int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
		0x10000;
	    string_builder_putchar(&s, cp);
	}
    }
#line 73 "rl/json_string.rl"
	{ mark = p + 1; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 173 "json_string.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 34: goto tr6;
		case 92: goto tr7;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 55159 ) {
		if ( 57344 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 1114111 )
			goto st3;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 32 )
		goto st3;
	goto st0;
tr3:
#line 69 "rl/json_string.rl"
	{
	mark = p;
    }
#line 75 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st15;
tr6:
#line 75 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 95 "rl/json_string.rl"
	{ p--; {p++; cs = 15; goto _out;} }
#line 212 "json_string.c"
	goto st0;
tr4:
#line 69 "rl/json_string.rl"
	{
	mark = p;
    }
#line 75 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st4;
tr7:
#line 75 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 240 "json_string.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 34: goto tr8;
		case 47: goto tr8;
		case 92: goto tr8;
		case 98: goto tr8;
		case 102: goto tr8;
		case 110: goto tr8;
		case 114: goto tr8;
		case 116: goto tr8;
		case 117: goto st5;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr10;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr10;
	} else
		goto tr10;
	goto st0;
tr10:
#line 10 "rl/json_string.rl"
	{
	hexchr0 = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 276 "json_string.c"
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
#line 14 "rl/json_string.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 297 "json_string.c"
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
#line 14 "rl/json_string.rl"
	{
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 318 "json_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr13;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr13;
	} else
		goto tr13;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 92 )
		goto st10;
	goto st0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 117 )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr16;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr16;
	} else
		goto tr16;
	goto st0;
tr16:
#line 36 "rl/json_string.rl"
	{
	hexchr1 = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 365 "json_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr17;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr17;
	} else
		goto tr17;
	goto st0;
tr17:
#line 40 "rl/json_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 386 "json_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr18;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr18;
	} else
		goto tr18;
	goto st0;
tr18:
#line 40 "rl/json_string.rl"
	{
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 407 "json_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr19;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr19;
	} else
		goto tr19;
	goto st0;
	}
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

#line 114 "rl/json_string.rl"

    if (cs < JSON_string_first_final) {
	if (!(state->flags&JSON_VALIDATE)) {
	    UNSET_ONERROR(handle);
	    free_string_builder(&s);
	}

	state->flags |= JSON_ERROR;
	if (p == pe) {
	    err_msg = "Unterminated string";
	    return start;
	}
	return p;
    }

    if (!(state->flags&JSON_VALIDATE)) {
	push_string(finish_string_builder(&s));
	UNSET_ONERROR(handle);
    }

    return p;
}

#undef HEX2DEC
