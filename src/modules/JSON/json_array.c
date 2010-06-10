
#line 1 "rl/json_array.rl"
// vim:syntax=ragel


#line 39 "rl/json_array.rl"


static ptrdiff_t _parse_JSON_array(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    struct array *a;
    int cs;
    int c = 0;

    
#line 16 "json_array.c"
static const int JSON_array_start = 1;
static const int JSON_array_first_final = 4;
static const int JSON_array_error = 0;

static const int JSON_array_en_main = 1;


#line 47 "rl/json_array.rl"

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (!(state->flags&JSON_VALIDATE)) {
	a = low_allocate_array(0,5);
	push_array(a);
    }

    
#line 36 "json_array.c"
	{
	cs = JSON_array_start;
	}

#line 58 "rl/json_array.rl"
    
#line 43 "json_array.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 91 )
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
		case 43: goto tr2;
		case 91: goto tr2;
		case 93: goto st4;
		case 102: goto tr2;
		case 110: goto tr2;
		case 116: goto tr2;
		case 123: goto tr2;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) < 45 ) {
		if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
			goto st2;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) > 46 ) {
		if ( 48 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr2;
	} else
		goto tr2;
	goto st0;
tr2:
#line 9 "rl/json_array.rl"
	{

	state->level++;
	p = _parse_JSON(str, p, pe, state);
	state->level--;

	if (state->flags&JSON_ERROR) {
	    if (!(state->flags&JSON_VALIDATE)) { 
		pop_stack();
	    }
	    return p;
	} else if (!(state->flags&JSON_VALIDATE)) {
	    Pike_sp[-2].u.array = a = array_insert(a, &(Pike_sp[-1]), c);
	    pop_stack();
	}

	c++;
	{p = (( p))-1;}
    }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 107 "json_array.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st3;
		case 32: goto st3;
		case 44: goto st2;
		case 93: goto st4;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st3;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 38 "rl/json_array.rl"
	{ p--; {p++; cs = 4; goto _out;} }
#line 123 "json_array.c"
	goto st0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 59 "rl/json_array.rl"

    if (cs >= JSON_array_first_final) {
	return p;
    }

    state->flags |= JSON_ERROR;

    if (!(state->flags&JSON_VALIDATE)) {
	pop_stack();
    }

    return p;
}
