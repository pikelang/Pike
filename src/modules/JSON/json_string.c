
#line 1 "rl/json_string.rl"
/* vim:syntax=ragel */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 65 "rl/json_string.rl"


static ptrdiff_t _parse_JSON_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    int temp = 0;
    ptrdiff_t start = p, mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;

    
#line 19 "json_string.c"
static const int JSON_string_start = 1;
static const int JSON_string_first_final = 9;
static const int JSON_string_error = 0;

static const int JSON_string_en_main = 1;


#line 75 "rl/json_string.rl"

    if (!(state->flags&JSON_VALIDATE)) {
	init_string_builder(&s, 0);
	SET_ONERROR (handle, free_string_builder, &s);
    }

    
#line 35 "json_string.c"
	{
	cs = JSON_string_start;
	}

#line 82 "rl/json_string.rl"
    
#line 42 "json_string.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
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
#line 41 "rl/json_string.rl"
	{
	mark = p;
    }
	goto st3;
tr8:
#line 28 "rl/json_string.rl"
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
#line 45 "rl/json_string.rl"
	{ mark = p + 1; }
	goto st3;
tr13:
#line 14 "rl/json_string.rl"
	{
	temp *= 16;
	temp += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
#line 19 "rl/json_string.rl"
	{
	if (IS_NUNICODE(temp)) {
	    p--; {p++; cs = 3; goto _out;}
	}
	if (!(state->flags&JSON_VALIDATE)) {
	    string_builder_putchar(&s, temp);
	}
    }
#line 45 "rl/json_string.rl"
	{ mark = p + 1; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 114 "json_string.c"
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
#line 41 "rl/json_string.rl"
	{
	mark = p;
    }
#line 47 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st9;
tr6:
#line 47 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 64 "rl/json_string.rl"
	{ p--; {p++; cs = 9; goto _out;} }
#line 153 "json_string.c"
	goto st0;
tr4:
#line 41 "rl/json_string.rl"
	{
	mark = p;
    }
#line 47 "rl/json_string.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), p - mark);
        }
    }
	goto st4;
tr7:
#line 47 "rl/json_string.rl"
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
#line 181 "json_string.c"
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
	temp = HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 217 "json_string.c"
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
	temp *= 16;
	temp += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 238 "json_string.c"
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
	temp *= 16;
	temp += HEX2DEC(( ((int)INDEX_PCHARP(str, p))));
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 259 "json_string.c"
	if ( ( ((int)INDEX_PCHARP(str, p))) < 65 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr13;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 70 ) {
		if ( 97 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 102 )
			goto tr13;
	} else
		goto tr13;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 83 "rl/json_string.rl"

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
