
#line 1 "rl/json_string_utf8.rl"
// vim:syntax=ragel
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)

#include "global.h"


#line 96 "rl/json_string_utf8.rl"


static char *_parse_JSON_string_utf8(char *p, char *pe, struct parser_state *state) {
    char *mark = 0;
    struct string_builder s;
    int cs;
    p_wchar2 temp = 0;
    p_wchar2 unicode = 0;

    
#line 21 "json_string_utf8.c"
static const int JSON_string_start = 1;
static const int JSON_string_first_final = 15;
static const int JSON_string_error = 0;

static const int JSON_string_en_main = 1;


#line 106 "rl/json_string_utf8.rl"

    if (!state->validate)
		init_string_builder(&s, 0);

    
#line 35 "json_string_utf8.c"
	{
	cs = JSON_string_start;
	}

#line 111 "rl/json_string_utf8.rl"
    
#line 42 "json_string_utf8.c"
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
		case 34: goto tr6;
		case 92: goto tr7;
	}
	if ( (*p) < -32 ) {
		if ( -62 <= (*p) && (*p) <= -33 )
			goto tr2;
	} else if ( (*p) > -17 ) {
		if ( (*p) > -12 ) {
			if ( 32 <= (*p) )
				goto tr5;
		} else if ( (*p) >= -16 )
			goto tr4;
	} else
		goto tr3;
	goto st0;
tr2:
#line 40 "rl/json_string_utf8.rl"
	{
		mark = p;
    }
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
#line 53 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st3;
tr9:
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
#line 53 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & (0xdf-0xc0))) << 6; }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 105 "json_string_utf8.c"
	if ( (*p) <= -65 )
		goto tr8;
	goto st0;
tr5:
#line 40 "rl/json_string_utf8.rl"
	{
		mark = p;
    }
	goto st4;
tr8:
#line 54 "rl/json_string_utf8.rl"
	{ unicode |= (p_wchar2)((*p) & (0xbf-0x80)); }
#line 76 "rl/json_string_utf8.rl"
	{
		if (!state->validate) { 
			string_builder_putchar(&s, unicode); 
		}
    }
#line 44 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st4;
tr16:
#line 58 "rl/json_string_utf8.rl"
	{ 
	unicode |= (p_wchar2)((*p) & (0xbf-0x80));
		if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
			goto failure;	
		}
    }
#line 76 "rl/json_string_utf8.rl"
	{
		if (!state->validate) { 
			string_builder_putchar(&s, unicode); 
		}
    }
#line 44 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st4;
tr19:
#line 69 "rl/json_string_utf8.rl"
	{ 
		unicode |= (p_wchar2)((*p) & (0xbf-0x80));
		if (unicode < 0x010000 || unicode > 0x10ffff) {
			goto failure;
		}
    }
#line 76 "rl/json_string_utf8.rl"
	{
		if (!state->validate) { 
			string_builder_putchar(&s, unicode); 
		}
    }
#line 44 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st4;
tr20:
#line 27 "rl/json_string_utf8.rl"
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
#line 44 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st4;
tr25:
#line 15 "rl/json_string_utf8.rl"
	{
		temp *= 16;
		temp += HEX2DEC((*p));
    }
#line 20 "rl/json_string_utf8.rl"
	{
		if (IS_NUNICODE(temp)) {
			goto failure;	
		}
		if (!state->validate) string_builder_putchar(&s, temp);
    }
#line 44 "rl/json_string_utf8.rl"
	{ mark = p + 1; }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 198 "json_string_utf8.c"
	switch( (*p) ) {
		case 34: goto tr13;
		case 92: goto tr14;
	}
	if ( (*p) < -32 ) {
		if ( -62 <= (*p) && (*p) <= -33 )
			goto tr9;
	} else if ( (*p) > -17 ) {
		if ( (*p) > -12 ) {
			if ( 32 <= (*p) )
				goto st4;
		} else if ( (*p) >= -16 )
			goto tr11;
	} else
		goto tr10;
	goto st0;
tr3:
#line 40 "rl/json_string_utf8.rl"
	{
		mark = p;
    }
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
#line 56 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st5;
tr10:
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
#line 56 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x0f)) << 12; }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 245 "json_string_utf8.c"
	if ( (*p) <= -65 )
		goto tr15;
	goto st0;
tr15:
#line 57 "rl/json_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 257 "json_string_utf8.c"
	if ( (*p) <= -65 )
		goto tr16;
	goto st0;
tr4:
#line 40 "rl/json_string_utf8.rl"
	{
		mark = p;
    }
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
#line 65 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st7;
tr11:
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
#line 65 "rl/json_string_utf8.rl"
	{ unicode = ((p_wchar2)((*p) & 0x07)) << 18; }
	goto st7;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 291 "json_string_utf8.c"
	if ( (*p) <= -65 )
		goto tr17;
	goto st0;
tr17:
#line 66 "rl/json_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 12; }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 303 "json_string_utf8.c"
	if ( (*p) <= -65 )
		goto tr18;
	goto st0;
tr18:
#line 57 "rl/json_string_utf8.rl"
	{ unicode |= ((p_wchar2)((*p) & (0xbf-0x80))) << 6; }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 315 "json_string_utf8.c"
	if ( (*p) <= -65 )
		goto tr19;
	goto st0;
tr6:
#line 40 "rl/json_string_utf8.rl"
	{
		mark = p;
    }
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st15;
tr13:
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st15;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
#line 95 "rl/json_string_utf8.rl"
	{ p--; {p++; cs = 15; goto _out;} }
#line 347 "json_string_utf8.c"
	goto st0;
tr7:
#line 40 "rl/json_string_utf8.rl"
	{
		mark = p;
    }
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st10;
tr14:
#line 46 "rl/json_string_utf8.rl"
	{
		if (p - mark > 0) {
			if (!state->validate)
				string_builder_binary_strcat(&s, mark, (ptrdiff_t)(p - mark));
        }
    }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 375 "json_string_utf8.c"
	switch( (*p) ) {
		case 34: goto tr20;
		case 47: goto tr20;
		case 92: goto tr20;
		case 98: goto tr20;
		case 102: goto tr20;
		case 110: goto tr20;
		case 114: goto tr20;
		case 116: goto tr20;
		case 117: goto st11;
	}
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr22;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr22;
	} else
		goto tr22;
	goto st0;
tr22:
#line 11 "rl/json_string_utf8.rl"
	{
		temp = HEX2DEC((*p));
    }
	goto st12;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 411 "json_string_utf8.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr23;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr23;
	} else
		goto tr23;
	goto st0;
tr23:
#line 15 "rl/json_string_utf8.rl"
	{
		temp *= 16;
		temp += HEX2DEC((*p));
    }
	goto st13;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
#line 432 "json_string_utf8.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr24;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr24;
	} else
		goto tr24;
	goto st0;
tr24:
#line 15 "rl/json_string_utf8.rl"
	{
		temp *= 16;
		temp += HEX2DEC((*p));
    }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 453 "json_string_utf8.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr25;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr25;
	} else
		goto tr25;
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 112 "rl/json_string_utf8.rl"

    if (cs >= JSON_string_first_final) {
		if (!state->validate)
			push_string(finish_string_builder(&s));

		return p;
    }

failure:

    if (!state->validate) {
		free_string_builder(&s);
    }

    push_int((INT_TYPE)p);
    return NULL;
}

#undef HEX2DEC
