
#line 1 "rl/json_array.rl"
/* vim:syntax=ragel
*/


#line 41 "rl/json_array.rl"


static ptrdiff_t _parse_JSON_array(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	/* GCC complains about a being used uninitialized. This is clearly wrong, so
	* lets silence this warning */
	struct array *a = a;
	int cs;
	int c=0;
	const int validate = !(state->flags&JSON_VALIDATE);
	
	
	static const int JSON_array_start = 1;
	static const int JSON_array_first_final = 6;
	static const int JSON_array_error = 0;
	
	static const int JSON_array_en_main = 1;
	
	static const char _JSON_array_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON_array_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static const char _JSON_array_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON_array_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 52 "rl/json_array.rl"
	
	
	/* Check stacks since we have uncontrolled recursion here. */
	check_stack (10);
	check_c_stack (1024);
	
	if (validate) {
		a = low_allocate_array(0,5);
		push_array(a);
	}
	
	
	{
		cs = (int)JSON_array_start;
	}
	
	#line 63 "rl/json_array.rl"
	
	
	{
		if ( p == pe )
		goto _test_eof;
		switch ( cs )
		{
			case 1:
			goto st_case_1;
			case 0:
			goto st_case_0;
			case 2:
			goto st_case_2;
			case 3:
			goto st_case_3;
			case 4:
			goto st_case_4;
			case 6:
			goto st_case_6;
			case 5:
			goto st_case_5;
		}
		goto st_out;
		st_case_1:
		if ( (((int)INDEX_PCHARP(str, p))) == 91 ) {
			goto st2;
		}
		{
			goto st0;
		}
		st_case_0:
		st0:
		cs = 0;
		goto _out;
		st2:
		p+= 1;
		if ( p == pe )
		goto _test_eof2;
		st_case_2:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st2;
			}
			case 32: {
				goto st2;
			}
			case 34: {
				goto ctr2;
			}
			case 43: {
				goto ctr2;
			}
			case 47: {
				goto st5;
			}
			case 91: {
				goto ctr2;
			}
			case 93: {
				goto st6;
			}
			case 102: {
				goto ctr2;
			}
			case 110: {
				goto ctr2;
			}
			case 116: {
				goto ctr2;
			}
			case 123: {
				goto ctr2;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 45 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr2;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto st2;
		}
		{
			goto st0;
		}
		ctr2:
		{
			#line 10 "rl/json_array.rl"
			
			
			state->level++;
			p = _parse_JSON(str, p, pe, state);
			state->level--;
			
			if (state->flags&JSON_ERROR) {
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
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st3;
			}
			case 32: {
				goto st3;
			}
			case 44: {
				goto st2;
			}
			case 47: {
				goto st4;
			}
			case 93: {
				goto st6;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st3;
		}
		{
			goto st0;
		}
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st3;
			}
			case 47: {
				goto st3;
			}
		}
		{
			goto st0;
		}
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		{
			#line 40 "rl/json_array.rl"
			p--; {p+= 1; cs = 6; goto _out;} }
		{
			goto st0;
		}
		st5:
		p+= 1;
		if ( p == pe )
		goto _test_eof5;
		st_case_5:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st2;
			}
			case 47: {
				goto st2;
			}
		}
		{
			goto st0;
		}
		st_out:
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof4: cs = 4; goto _test_eof; 
		_test_eof6: cs = 6; goto _test_eof; 
		_test_eof5: cs = 5; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 64 "rl/json_array.rl"
	
	
	if (cs >= JSON_array_first_final) {
		return p;
	}
	
	state->flags |= JSON_ERROR;
	
	if (validate) {
		pop_stack();
	}
	
	return p;
}
