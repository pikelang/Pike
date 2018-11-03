
#line 1 "rl/json_mapping.rl"
/* vim:syntax=ragel
*/


#line 61 "rl/json_mapping.rl"


static ptrdiff_t _parse_JSON_mapping(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	/* GCC complains about a being used uninitialized. This is clearly wrong, so
	* lets silence this warning */
	struct mapping *m = m;
	int cs;
	int c=0;
	const int validate = !(state->flags&JSON_VALIDATE);
	
	
	static const int JSON_mapping_start = 1;
	static const int JSON_mapping_first_final = 10;
	static const int JSON_mapping_error = 0;
	
	static const int JSON_mapping_en_main = 1;
	
	static const char _JSON_mapping_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON_mapping_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0
	};
	
	static const char _JSON_mapping_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON_mapping_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 72 "rl/json_mapping.rl"
	
	
	/* Check stacks since we have uncontrolled recursion here. */
	check_stack (10);
	check_c_stack (1024);
	
	if (validate) {
		m = debug_allocate_mapping(5);
		push_mapping(m);
	}
	
	
	{
		cs = (int)JSON_mapping_start;
	}
	
	#line 83 "rl/json_mapping.rl"
	
	
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
			case 5:
			goto st_case_5;
			case 6:
			goto st_case_6;
			case 7:
			goto st_case_7;
			case 10:
			goto st_case_10;
			case 8:
			goto st_case_8;
			case 9:
			goto st_case_9;
		}
		goto st_out;
		st_case_1:
		if ( (((int)INDEX_PCHARP(str, p))) == 123 ) {
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
			case 47: {
				goto st9;
			}
			case 125: {
				goto st10;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st2;
		}
		{
			goto st0;
		}
		ctr2:
		{
			#line 29 "rl/json_mapping.rl"
			
			state->level++;
			if (state->flags&JSON_UTF8)
			p = _parse_JSON_string_utf8(str, p, pe, state);
			else
			p = _parse_JSON_string(str, p, pe, state);
			state->level--;
			
			if (state->flags&JSON_ERROR) {
				if (validate) {
					pop_stack(); /* pop mapping */
				}
				return p;
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
			case 47: {
				goto st4;
			}
			case 58: {
				goto st5;
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
		st5:
		p+= 1;
		if ( p == pe )
		goto _test_eof5;
		st_case_5:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st5;
			}
			case 32: {
				goto st5;
			}
			case 34: {
				goto ctr8;
			}
			case 43: {
				goto ctr8;
			}
			case 47: {
				goto st8;
			}
			case 91: {
				goto ctr8;
			}
			case 102: {
				goto ctr8;
			}
			case 110: {
				goto ctr8;
			}
			case 116: {
				goto ctr8;
			}
			case 123: {
				goto ctr8;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 45 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr8;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto st5;
		}
		{
			goto st0;
		}
		ctr8:
		{
			#line 10 "rl/json_mapping.rl"
			
			state->level++;
			p = _parse_JSON(str, p, pe, state);
			state->level--;
			
			if (state->flags&JSON_ERROR) {
				if (validate) {
					pop_2_elems(); /* pop mapping and key */
				}
				return p;
			} else if (validate) {
				mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
				pop_2_elems();
			}
			
			c++;
			{p = (( p))-1;}
			
		}
		
		goto st6;
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st6;
			}
			case 32: {
				goto st6;
			}
			case 44: {
				goto st2;
			}
			case 47: {
				goto st7;
			}
			case 125: {
				goto st10;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st6;
		}
		{
			goto st0;
		}
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st6;
			}
			case 47: {
				goto st6;
			}
		}
		{
			goto st0;
		}
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		{
			#line 60 "rl/json_mapping.rl"
			p--; {p+= 1; cs = 10; goto _out;} }
		{
			goto st0;
		}
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st5;
			}
			case 47: {
				goto st5;
			}
		}
		{
			goto st0;
		}
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
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
		_test_eof5: cs = 5; goto _test_eof; 
		_test_eof6: cs = 6; goto _test_eof; 
		_test_eof7: cs = 7; goto _test_eof; 
		_test_eof10: cs = 10; goto _test_eof; 
		_test_eof8: cs = 8; goto _test_eof; 
		_test_eof9: cs = 9; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 84 "rl/json_mapping.rl"
	
	
	if (cs >= JSON_mapping_first_final) {
		return p;
	}
	
	state->flags |= JSON_ERROR;
	if (validate) {
		if (c & 1) pop_2_elems(); /* pop key and mapping */
		else pop_stack();
	}
	
	return p;
}

