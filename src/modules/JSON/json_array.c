
#line 1 "rl/json_array.rl"
// vim:syntax=ragel


#line 37 "rl/json_array.rl"


static p_wchar2 *_parse_JSON_array(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 *i = p;
    struct array *a;
    int cs;
    int c = 0;

    
#line 17 "json_array.c"
static const int JSON_array_start = 1;
static const int JSON_array_first_final = 4;
static const int JSON_array_error = 0;

static const int JSON_array_en_main = 1;


#line 46 "rl/json_array.rl"

    if (!state->validate) {
	a = low_allocate_array(0,5);
    }

    
#line 32 "json_array.c"
	{
	cs = JSON_array_start;
	}

#line 52 "rl/json_array.rl"
    
#line 39 "json_array.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 91 )
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
		case 13: goto st2;
		case 32: goto st2;
		case 34: goto tr2;
		case 43: goto tr2;
		case 91: goto tr2;
		case 93: goto st4;
		case 102: goto tr2;
		case 110: goto tr2;
		case 116: goto tr2;
		case 123: goto tr2;
	}
	if ( (*p) < 45 ) {
		if ( 9 <= (*p) && (*p) <= 10 )
			goto st2;
	} else if ( (*p) > 46 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else
		goto tr2;
	goto st0;
tr2:
#line 8 "rl/json_array.rl"
	{
		state->level++;
		i = _parse_JSON(p, pe, state);
		state->level--;

		if (i == NULL) {
			if (!state->validate) { 
				free_array(a);
			}
			return NULL;
		} else if (!state->validate) {
			a = array_insert(a, &(Pike_sp[-1]), c);
			pop_stack();
		}

		c++;
		{p = (( i))-1;}
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 102 "json_array.c"
	switch( (*p) ) {
		case 13: goto st3;
		case 32: goto st3;
		case 44: goto st2;
		case 93: goto st4;
	}
	if ( 9 <= (*p) && (*p) <= 10 )
		goto st3;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 36 "rl/json_array.rl"
	{ p--; {p++; cs = 4; goto _out;} }
#line 118 "json_array.c"
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 53 "rl/json_array.rl"

    if (cs >= JSON_array_first_final) {
	if (!state->validate) {
	    push_array(a);
	}
	return p;
    }

    if (!state->validate) {
	free_array(a);
    }


    push_int((INT_TYPE)p);
    return NULL;
}
