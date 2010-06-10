
#line 1 "rl/json_string.rl"
// vim:syntax=ragel
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)


#line 70 "rl/json_string.rl"


static p_wchar2 *_parse_JSON_string(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 temp = 0;
    p_wchar2 *mark = 0;
    struct string_builder s;
    int cs;

    
#line 18 "json_string.c"
static const int JSON_string_start = 1;
static const int JSON_string_first_final = 9;
static const int JSON_string_error = 0;

static const int JSON_string_en_main = 1;


#line 79 "rl/json_string.rl"

    if (!state->validate)
		init_string_builder(&s, 0);

    
#line 32 "json_string.c"
	{
	cs = JSON_string_start;
	}

#line 84 "rl/json_string.rl"
    
#line 39 "json_string.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 34 )
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
		case 34: goto tr3;
		case 92: goto tr4;
	}
	if ( (*p) > 55159 ) {
		if ( 57344 <= (*p) && (*p) <= 1114111 )
			goto tr2;
	} else if ( (*p) >= 32 )
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
		if (!state->validate) switch((*p)) {
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
#line 45 "rl/json_string.rl"
	{ mark = p + 1; }
	goto st3;
tr13:
#line 13 "rl/json_string.rl"
	{
		if (!state->validate) {
			temp *= 16;
			temp += HEX2DEC((*p));

			if (IS_NUNICODE(temp)) {
				p--; {p++; cs = 3; goto _out;}
			}
		}
    }
#line 24 "rl/json_string.rl"
	{
		if (!state->validate) string_builder_putchar(&s, temp);
    }
#line 45 "rl/json_string.rl"
	{ mark = p + 1; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 112 "json_string.c"
	switch( (*p) ) {
		case 34: goto tr6;
		case 92: goto tr7;
	}
	if ( (*p) > 55159 ) {
		if ( 57344 <= (*p) && (*p) <= 1114111 )
			goto st3;
	} else if ( (*p) >= 32 )
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
            //string_builder_binary_strcat(s, mark, (ptrdiff_t)(fpc - mark));

			// looking for the lowest possible magnitude here may be worth it. i am not entirely
			// sure if i want to do the copying here.
			// use string_builder_binary_strcat here
			if (!state->validate)
				string_builder_append(&s, MKPCHARP(mark, 2), (ptrdiff_t)(p - mark));
        }
    }
	goto st9;
tr6:
#line 47 "rl/json_string.rl"
	{
		if (p - mark > 0) {
            //string_builder_binary_strcat(s, mark, (ptrdiff_t)(fpc - mark));

			// looking for the lowest possible magnitude here may be worth it. i am not entirely
			// sure if i want to do the copying here.
			// use string_builder_binary_strcat here
			if (!state->validate)
				string_builder_append(&s, MKPCHARP(mark, 2), (ptrdiff_t)(p - mark));
        }
    }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 69 "rl/json_string.rl"
	{ p--; {p++; cs = 9; goto _out;} }
#line 161 "json_string.c"
	goto st0;
tr4:
#line 41 "rl/json_string.rl"
	{
		mark = p;
    }
#line 47 "rl/json_string.rl"
	{
		if (p - mark > 0) {
            //string_builder_binary_strcat(s, mark, (ptrdiff_t)(fpc - mark));

			// looking for the lowest possible magnitude here may be worth it. i am not entirely
			// sure if i want to do the copying here.
			// use string_builder_binary_strcat here
			if (!state->validate)
				string_builder_append(&s, MKPCHARP(mark, 2), (ptrdiff_t)(p - mark));
        }
    }
	goto st4;
tr7:
#line 47 "rl/json_string.rl"
	{
		if (p - mark > 0) {
            //string_builder_binary_strcat(s, mark, (ptrdiff_t)(fpc - mark));

			// looking for the lowest possible magnitude here may be worth it. i am not entirely
			// sure if i want to do the copying here.
			// use string_builder_binary_strcat here
			if (!state->validate)
				string_builder_append(&s, MKPCHARP(mark, 2), (ptrdiff_t)(p - mark));
        }
    }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 199 "json_string.c"
	switch( (*p) ) {
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
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr10;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr10;
	} else
		goto tr10;
	goto st0;
tr10:
#line 9 "rl/json_string.rl"
	{
		if (!state->validate) temp = HEX2DEC((*p));
    }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 235 "json_string.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr11;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr11;
	} else
		goto tr11;
	goto st0;
tr11:
#line 13 "rl/json_string.rl"
	{
		if (!state->validate) {
			temp *= 16;
			temp += HEX2DEC((*p));

			if (IS_NUNICODE(temp)) {
				p--; {p++; cs = 7; goto _out;}
			}
		}
    }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 262 "json_string.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr12;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr12;
	} else
		goto tr12;
	goto st0;
tr12:
#line 13 "rl/json_string.rl"
	{
		if (!state->validate) {
			temp *= 16;
			temp += HEX2DEC((*p));

			if (IS_NUNICODE(temp)) {
				p--; {p++; cs = 8; goto _out;}
			}
		}
    }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 289 "json_string.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr13;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
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

#line 85 "rl/json_string.rl"

    if (cs < JSON_string_first_final) {
		if (!state->validate) {
			free_string_builder(&s);
		}

		push_int((INT_TYPE)p);
		return NULL;
    }

    if (!state->validate)
		push_string(finish_string_builder(&s));

    return p;
}

#undef HEX2DEC
