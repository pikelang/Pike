
#line 1 "rl/json_mapping.rl"
// vim:syntax=ragel



#line 61 "rl/json_mapping.rl"


static ptrdiff_t _parse_JSON_mapping(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    struct mapping *m;
    int cs;
    int c = 0;

    
#line 17 "json_mapping.c"
static const int JSON_mapping_start = 1;
static const int JSON_mapping_first_final = 6;
static const int JSON_mapping_error = 0;

static const int JSON_mapping_en_main = 1;


#line 69 "rl/json_mapping.rl"

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (!(state->flags&JSON_VALIDATE)) {
	m = debug_allocate_mapping(5);
	push_mapping(m);
    }

    
#line 37 "json_mapping.c"
	{
	cs = JSON_mapping_start;
	}

#line 80 "rl/json_mapping.rl"
    
#line 44 "json_mapping.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 123 )
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
		case 13: goto st2;
		case 32: goto st2;
		case 34: goto tr2;
		case 125: goto st6;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st2;
	goto st0;
tr2:
#line 29 "rl/json_mapping.rl"
	{
	state->level++;
	if (state->flags&JSON_UTF8)
	    p = _parse_JSON_string_utf8(str, p, pe, state);
	else
	    p = _parse_JSON_string(str, p, pe, state);
	state->level--;

	if (state->flags&JSON_ERROR) {
	    if (!(state->flags&JSON_VALIDATE)) {
		pop_stack(); // pop mapping
	    }
	    return p;
	}

	c++;
	{p = (( p))-1;}
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 95 "json_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st3;
		case 32: goto st3;
		case 58: goto st4;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st3;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
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
	if ( ( ((int)INDEX_PCHARP(str, p))) < 45 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto st4;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 46 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr6;
	} else
		goto tr6;
	goto st0;
tr6:
#line 10 "rl/json_mapping.rl"
	{
	state->level++;
	p = _parse_JSON(str, p, pe, state);
	state->level--;

	if (state->flags&JSON_ERROR) {
	    if (!(state->flags&JSON_VALIDATE)) {
		pop_2_elems(); // pop mapping and key
	    }
	    return p;
	} else if (!(state->flags&JSON_VALIDATE)) {
	    mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
	    pop_2_elems();
	}

	c++;
	{p = (( p))-1;}
    }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 153 "json_mapping.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st5;
		case 32: goto st5;
		case 44: goto st2;
		case 125: goto st6;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st5;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 60 "rl/json_mapping.rl"
	{ p--; {p++; cs = 6; goto _out;} }
#line 169 "json_mapping.c"
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

#line 81 "rl/json_mapping.rl"

    if (cs >= JSON_mapping_first_final) {
	return p;
    }

    state->flags |= JSON_ERROR;
    if (!(state->flags&JSON_VALIDATE)) {
	if (c & 1) pop_2_elems(); // pop key and mapping
	else pop_stack();
    }

    return p;
}

