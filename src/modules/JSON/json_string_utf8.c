
#line 1 "rl/json_string_utf8.rl"
/* vim:syntax=ragel */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 94 "rl/json_string_utf8.rl"


static ptrdiff_t _parse_JSON_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
    unsigned char *p = (unsigned char*)(str.ptr) + pos;
    unsigned char *pe = (unsigned char*)(str.ptr) + end;
    ptrdiff_t start = pos;
    unsigned char *mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;
    p_wchar2 temp = 0;
    p_wchar2 unicode = 0;

    
#line 23 "json_string_utf8.c"
static const int JSON_string_start = 1;
static const int JSON_string_first_final = 15;
static const int JSON_string_error = 0;

static const int JSON_string_en_main = 1;


#line 108 "rl/json_string_utf8.rl"

    if (!(state->flags&JSON_VALIDATE)) {
	init_string_builder(&s, 0);
	SET_ONERROR(handle, free_string_builder, &s);
    }

    
#line 39 "json_string_utf8.c"
	{
	cs = JSON_string_start;
	}

#line 115 "rl/json_string_utf8.rl"
    
#line 46 "json_string_utf8.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 34u )
		goto st2;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 34u: goto tr3;
		case 92u: goto tr4;
	}
	if ( (*p) < 194u ) {
		if ( 32u <= (*p) && (*p) <= 127u )
			goto tr2;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr7;
		} else if ( (*p) >= 224u )
			goto tr6;
	} else
		goto tr5;
	goto st0;
tr2:
#line 38 "rl/json_string_utf8.rl"
	{
	mark = p;
    }
	goto st3;
tr14:
#line 25 "rl/json_string_utf8.rl"
	{
	if (!(state->flags&JSON_VALIDATE)) switch((*p)) {
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
#line 42 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr19:
#line 13 "rl/json_string_utf8.rl"
	{
	temp *= 16;
	temp += HEX2DEC((*p));
    }
#line 18 "rl/json_string_utf8.rl"
	{
	if (IS_NUNICODE(temp)) {
		goto failure;	
	}
	if (!(state->flags&JSON_VALIDATE)) string_builder_putchar(&s, temp);
    }
#line 42 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr20:
#line 52 "rl/json_string_utf8.rl"
	{ unicode |= (p_wchar2)((*p) & (0xbf-0x80)); }
#line 74 "rl/json_string_utf8.rl"
	{
	if (!(state->flags&JSON_VALIDATE)) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 42 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr22:
#line 56 "rl/json_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
	if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
	    goto failure;	
	}
    }
#line 74 "rl/json_string_utf8.rl"
	{
	if (!(state->flags&JSON_VALIDATE)) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 42 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
tr25:
#line 67 "rl/json_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
	if (unicode < 0x010000 || unicode > 0x10ffff) {
	    goto failure;
	}
    }
#line 74 "rl/json_string_utf8.rl"
	{
	if (!(state->flags&JSON_VALIDATE)) { 
	    string_builder_putchar(&s, unicode); 
	}
    }
#line 42 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 168 "json_string_utf8.c"
	switch( (*p) ) {
		case 34u: goto tr9;
		case 92u: goto tr10;
	}
	if ( (*p) < 194u ) {
		if ( 32u <= (*p) && (*p) <= 127u )
			goto st3;
	} else if ( (*p) > 223u ) {
		if ( (*p) > 239u ) {
			if ( 240u <= (*p) && (*p) <= 244u )
				goto tr13;
		} else if ( (*p) >= 224u )
			goto tr12;
	} else
		goto tr11;
	goto st0;
tr3:
#line 38 "rl/json_string_utf8.rl"
	{
	mark = p;
    }
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st15;
tr9:
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 93 "rl/json_string_utf8.rl"
	{ p--; {p++; cs = 15; goto _out;} }
#line 213 "json_string_utf8.c"
	goto st0;
tr4:
#line 38 "rl/json_string_utf8.rl"
	{
	mark = p;
    }
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st4;
tr10:
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 241 "json_string_utf8.c"
	switch( (*p) ) {
		case 34u: goto tr14;
		case 47u: goto tr14;
		case 92u: goto tr14;
		case 98u: goto tr14;
		case 102u: goto tr14;
		case 110u: goto tr14;
		case 114u: goto tr14;
		case 116u: goto tr14;
		case 117u: goto st5;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr16;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr16;
	} else
		goto tr16;
	goto st0;
tr16:
#line 9 "rl/json_string_utf8.rl"
	{
	temp = HEX2DEC((*p));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 277 "json_string_utf8.c"
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
#line 13 "rl/json_string_utf8.rl"
	{
	temp *= 16;
	temp += HEX2DEC((*p));
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 298 "json_string_utf8.c"
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
#line 13 "rl/json_string_utf8.rl"
	{
	temp *= 16;
	temp += HEX2DEC((*p));
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 319 "json_string_utf8.c"
	if ( (*p) < 65u ) {
		if ( 48u <= (*p) && (*p) <= 57u )
			goto tr19;
	} else if ( (*p) > 70u ) {
		if ( 97u <= (*p) && (*p) <= 102u )
			goto tr19;
	} else
		goto tr19;
	goto st0;
tr5:
#line 38 "rl/json_string_utf8.rl"
	{
	mark = p;
    }
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 51 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st9;
tr11:
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 51 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 359 "json_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr20;
	goto st0;
tr6:
#line 38 "rl/json_string_utf8.rl"
	{
	mark = p;
    }
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 54 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st10;
tr12:
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 54 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 393 "json_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr21;
	goto st0;
tr21:
#line 55 "rl/json_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 405 "json_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr22;
	goto st0;
tr7:
#line 38 "rl/json_string_utf8.rl"
	{
	mark = p;
    }
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 63 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st12;
tr13:
#line 44 "rl/json_string_utf8.rl"
	{
	if (p - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(p - mark));
        }
    }
#line 63 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 439 "json_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr23;
	goto st0;
tr23:
#line 64 "rl/json_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 12; }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 451 "json_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr24;
	goto st0;
tr24:
#line 55 "rl/json_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 463 "json_string_utf8.c"
	if ( 128u <= (*p) && (*p) <= 191u )
		goto tr25;
	goto st0;
	}
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

#line 116 "rl/json_string_utf8.rl"

    if (cs >= JSON_string_first_final) {
	if (!(state->flags&JSON_VALIDATE)) {
	    push_string(finish_string_builder(&s));
	    UNSET_ONERROR(handle);
	}

	return p - (unsigned char*)(str.ptr);
    }

failure:

    if (!(state->flags&JSON_VALIDATE)) {
	UNSET_ONERROR(handle);
	free_string_builder(&s);
    }

    state->flags |= JSON_ERROR;
    if (p == pe) {
	err_msg = "Unterminated string";
	return start;
    }
    return p - (unsigned char*)(str.ptr);
}

#undef HEX2DEC
