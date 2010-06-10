
#line 1 "rl/json_mapping_utf8.rl"
// vim:syntax=ragel



#line 51 "rl/json_mapping_utf8.rl"


static char *_parse_JSON_mapping_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
    struct mapping *m;
    int cs;
    int c = 0;

    
#line 18 "json_mapping_utf8.c"
static const int JSON_mapping_start = 1;
static const int JSON_mapping_first_final = 6;
static const int JSON_mapping_error = 0;

static const int JSON_mapping_en_main = 1;


#line 60 "rl/json_mapping_utf8.rl"

    if (!state->validate) {
		m = debug_allocate_mapping(5);
    }

    
#line 33 "json_mapping_utf8.c"
	{
	cs = JSON_mapping_start;
	}

#line 66 "rl/json_mapping_utf8.rl"
    
#line 40 "json_mapping_utf8.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 123 )
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
		case 125: goto st6;
	}
	if ( 9 <= (*p) && (*p) <= 10 )
		goto st2;
	goto st0;
tr2:
#line 25 "rl/json_mapping_utf8.rl"
	{
		state->level++;
		i = _parse_JSON_string_utf8(p, pe, state);
		state->level--;

		if (i == NULL) {
			goto failure;
		}

		c++;
		{p = (( i))-1;}
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 85 "json_mapping_utf8.c"
	switch( (*p) ) {
		case 13: goto st3;
		case 32: goto st3;
		case 58: goto st4;
	}
	if ( 9 <= (*p) && (*p) <= 10 )
		goto st3;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 13: goto st4;
		case 32: goto st4;
		case 34: goto tr6;
		case 43: goto tr6;
		case 91: goto tr6;
		case 102: goto tr6;
		case 110: goto tr6;
		case 116: goto tr6;
		case 123: goto tr6;
	}
	if ( (*p) < 45 ) {
		if ( 9 <= (*p) && (*p) <= 10 )
			goto st4;
	} else if ( (*p) > 46 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr6;
	} else
		goto tr6;
	goto st0;
tr6:
#line 9 "rl/json_mapping_utf8.rl"
	{
		state->level++;
		i = _parse_JSON_utf8(p, pe, state);
		state->level--;

		if (i == NULL) {
			goto failure;
		} else if (!state->validate) {
			mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
			pop_2_elems();
		}

		c++;
		{p = (( i))-1;}
    }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 140 "json_mapping_utf8.c"
	switch( (*p) ) {
		case 13: goto st5;
		case 32: goto st5;
		case 44: goto st2;
		case 125: goto st6;
	}
	if ( 9 <= (*p) && (*p) <= 10 )
		goto st5;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 50 "rl/json_mapping_utf8.rl"
	{ p--; {p++; cs = 6; goto _out;} }
#line 156 "json_mapping_utf8.c"
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 67 "rl/json_mapping_utf8.rl"

    if (cs >= JSON_mapping_first_final) {
		if (!state->validate) {
			push_mapping(m);
		}
		return p;
    }


failure:
    if (!state->validate) {
		if (c & 1) stack_pop_keep_top(); // remove key
		free_mapping(m);
    }

    push_int((INT_TYPE)p);
    return NULL;
}

