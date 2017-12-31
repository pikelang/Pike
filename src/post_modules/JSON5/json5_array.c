
#line 1 "rl/json5_array.rl"
/* vim:syntax=ragel
 */


#line 41 "rl/json5_array.rl"


static ptrdiff_t _parse_JSON5_array(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    /* GCC complains about a being used uninitialized. This is clearly wrong, so
     * lets silence this warning */
    struct array *a = a;
    int cs;
    int c = 0;
    ptrdiff_t eof = pe;
    const int validate = !(state->flags&JSON5_VALIDATE);

    
#line 21 "json5_array.c"
static const int JSON5_array_start = 1;
static const int JSON5_array_first_final = 12;
static const int JSON5_array_error = 0;

static const int JSON5_array_en_main = 1;


#line 53 "rl/json5_array.rl"

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (validate) {
	a = low_allocate_array(0,5);
	push_array(a);
    }

    
#line 41 "json5_array.c"
	{
	cs = JSON5_array_start;
	}

#line 64 "rl/json5_array.rl"
    
#line 48 "json5_array.c"
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
		case 39: goto tr2;
		case 43: goto tr2;
		case 47: goto st8;
		case 73: goto tr2;
		case 78: goto tr2;
		case 91: goto tr2;
		case 93: goto st12;
		case 102: goto tr2;
		case 110: goto tr2;
		case 116: goto tr2;
		case 123: goto tr2;
	}
	if ( ( ((int)INDEX_PCHARP(str, p))) > 10 ) {
		if ( 45 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 57 )
			goto tr2;
	} else if ( ( ((int)INDEX_PCHARP(str, p))) >= 9 )
		goto st2;
	goto st0;
tr2:
#line 10 "rl/json5_array.rl"
	{

	state->level++;
	p = _parse_JSON5(str, p, pe, state);
	state->level--;

	if (state->flags&JSON5_ERROR) {
	    if (validate) { 
		pop_stack();
	    }
	    return p;
	} 
        if (validate) {
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
#line 114 "json5_array.c"
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 13: goto st3;
		case 32: goto st3;
		case 44: goto st2;
		case 47: goto st4;
		case 93: goto st12;
	}
	if ( 9 <= ( ((int)INDEX_PCHARP(str, p))) && ( ((int)INDEX_PCHARP(str, p))) <= 10 )
		goto st3;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st5;
		case 47: goto st7;
	}
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st6;
	goto st5;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st6;
		case 47: goto st3;
	}
	goto st5;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st3;
	goto st7;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
#line 40 "rl/json5_array.rl"
	{ p--; {p++; cs = 12; goto _out;} }
#line 163 "json5_array.c"
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st9;
		case 47: goto st11;
	}
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 42 )
		goto st10;
	goto st9;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( ( ((int)INDEX_PCHARP(str, p))) ) {
		case 42: goto st10;
		case 47: goto st2;
	}
	goto st9;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( ( ((int)INDEX_PCHARP(str, p))) == 10 )
		goto st2;
	goto st11;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 65 "rl/json5_array.rl"

    if (cs >= JSON5_array_first_final) {
	return p;
    }

    state->flags |= JSON5_ERROR;

    if (validate) {
	pop_stack();
    }

    return p;
}
