
#line 1 "rl/json5_mapping.rl"
/* vim:syntax=ragel
*/


#line 75 "rl/json5_mapping.rl"


static ptrdiff_t _parse_JSON5_mapping(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	/* GCC complains about a being used uninitialized. This is clearly wrong, so
	* lets silence this warning */
	struct mapping *m = m;
	int cs;
	int c=0;
	struct string_builder s;
	ptrdiff_t eof = pe;
	ptrdiff_t identifier_start;
	
	const int validate = !(state->flags&JSON5_VALIDATE);
	
	
	static const int JSON5_mapping_start = 1;
	static const int JSON5_mapping_first_final = 23;
	static const int JSON5_mapping_error = 0;
	
	static const int JSON5_mapping_en_main = 1;
	
	static const char _JSON5_mapping_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON5_mapping_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0
	};
	
	static const char _JSON5_mapping_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON5_mapping_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 90 "rl/json5_mapping.rl"
	
	
	/* Check stacks since we have uncontrolled recursion here. */
	check_stack (10);
	check_c_stack (1024);
	
	if (validate) {
		m = debug_allocate_mapping(5);
		push_mapping(m);
	}
	
	
	{
		cs = (int)JSON5_mapping_start;
	}
	
	#line 101 "rl/json5_mapping.rl"
	
	
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
			case 8:
			goto st_case_8;
			case 9:
			goto st_case_9;
			case 10:
			goto st_case_10;
			case 11:
			goto st_case_11;
			case 12:
			goto st_case_12;
			case 13:
			goto st_case_13;
			case 23:
			goto st_case_23;
			case 14:
			goto st_case_14;
			case 15:
			goto st_case_15;
			case 16:
			goto st_case_16;
			case 17:
			goto st_case_17;
			case 18:
			goto st_case_18;
			case 19:
			goto st_case_19;
			case 20:
			goto st_case_20;
			case 21:
			goto st_case_21;
			case 22:
			goto st_case_22;
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
			case 39: {
				goto ctr2;
			}
			case 47: {
				goto st18;
			}
			case 95: {
				goto ctr4;
			}
			case 125: {
				goto st23;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
				goto st2;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 90 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 122 ) {
				goto ctr4;
			}
		} else {
			goto ctr4;
		}
		{
			goto st0;
		}
		ctr2:
		{
			#line 29 "rl/json5_mapping.rl"
			
			state->level++;
			if (state->flags&JSON5_UTF8)
			p = _parse_JSON5_string_utf8(str, p, pe, state);
			else
			p = _parse_JSON5_string(str, p, pe, state);
			state->level--;
			
			if (state->flags&JSON5_ERROR) {
				if (validate) {
					pop_stack(); /* pop mapping */
				}
				return p;
			}
			
			c++;
			{p = (( p))-1;}
			
		}
		
		goto st3;
		ctr25:
		{
			#line 52 "rl/json5_mapping.rl"
			
			ptrdiff_t len = p - identifier_start;
			if (validate) {
				init_string_builder(&s, 0);
				string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
				push_string(finish_string_builder(&s));
			}
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
				goto st8;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st3;
		}
		{
			goto st0;
		}
		ctr26:
		{
			#line 52 "rl/json5_mapping.rl"
			
			ptrdiff_t len = p - identifier_start;
			if (validate) {
				init_string_builder(&s, 0);
				string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
				push_string(finish_string_builder(&s));
			}
		}
		
		goto st4;
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st5;
			}
			case 47: {
				goto st7;
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
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st6;
		}
		{
			goto st5;
		}
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st6;
			}
			case 47: {
				goto st3;
			}
		}
		{
			goto st5;
		}
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st3;
		}
		{
			goto st7;
		}
		ctr28:
		{
			#line 52 "rl/json5_mapping.rl"
			
			ptrdiff_t len = p - identifier_start;
			if (validate) {
				init_string_builder(&s, 0);
				string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
				push_string(finish_string_builder(&s));
			}
		}
		
		goto st8;
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st8;
			}
			case 32: {
				goto st8;
			}
			case 34: {
				goto ctr12;
			}
			case 39: {
				goto ctr12;
			}
			case 43: {
				goto ctr12;
			}
			case 47: {
				goto st14;
			}
			case 73: {
				goto ctr12;
			}
			case 78: {
				goto ctr12;
			}
			case 91: {
				goto ctr12;
			}
			case 102: {
				goto ctr12;
			}
			case 110: {
				goto ctr12;
			}
			case 116: {
				goto ctr12;
			}
			case 123: {
				goto ctr12;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 45 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto ctr12;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto st8;
		}
		{
			goto st0;
		}
		ctr12:
		{
			#line 10 "rl/json5_mapping.rl"
			
			state->level++;
			p = _parse_JSON5(str, p, pe, state);
			state->level--;
			
			if (state->flags&JSON5_ERROR) {
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
		
		goto st9;
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st9;
			}
			case 32: {
				goto st9;
			}
			case 44: {
				goto st2;
			}
			case 47: {
				goto st10;
			}
			case 125: {
				goto st23;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st9;
		}
		{
			goto st0;
		}
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st11;
			}
			case 47: {
				goto st13;
			}
		}
		{
			goto st0;
		}
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st12;
		}
		{
			goto st11;
		}
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st12;
			}
			case 47: {
				goto st9;
			}
		}
		{
			goto st11;
		}
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st9;
		}
		{
			goto st13;
		}
		st23:
		p+= 1;
		if ( p == pe )
		goto _test_eof23;
		st_case_23:
		{
			#line 74 "rl/json5_mapping.rl"
			p--; {p+= 1; cs = 23; goto _out;} }
		{
			goto st0;
		}
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st15;
			}
			case 47: {
				goto st17;
			}
		}
		{
			goto st0;
		}
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st16;
		}
		{
			goto st15;
		}
		st16:
		p+= 1;
		if ( p == pe )
		goto _test_eof16;
		st_case_16:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st16;
			}
			case 47: {
				goto st8;
			}
		}
		{
			goto st15;
		}
		st17:
		p+= 1;
		if ( p == pe )
		goto _test_eof17;
		st_case_17:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st8;
		}
		{
			goto st17;
		}
		st18:
		p+= 1;
		if ( p == pe )
		goto _test_eof18;
		st_case_18:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st19;
			}
			case 47: {
				goto st21;
			}
		}
		{
			goto st0;
		}
		st19:
		p+= 1;
		if ( p == pe )
		goto _test_eof19;
		st_case_19:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st20;
		}
		{
			goto st19;
		}
		st20:
		p+= 1;
		if ( p == pe )
		goto _test_eof20;
		st_case_20:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st20;
			}
			case 47: {
				goto st2;
			}
		}
		{
			goto st19;
		}
		st21:
		p+= 1;
		if ( p == pe )
		goto _test_eof21;
		st_case_21:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st2;
		}
		{
			goto st21;
		}
		ctr4:
		{
			#line 48 "rl/json5_mapping.rl"
			
			identifier_start = p;
		}
		
		goto st22;
		st22:
		p+= 1;
		if ( p == pe )
		goto _test_eof22;
		st_case_22:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr25;
			}
			case 32: {
				goto ctr25;
			}
			case 47: {
				goto ctr26;
			}
			case 58: {
				goto ctr28;
			}
			case 95: {
				goto st22;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) < 48 ) {
			if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
				goto ctr25;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 57 ) {
			if ( (((int)INDEX_PCHARP(str, p))) > 90 ) {
				if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 122 ) {
					goto st22;
				}
			} else if ( (((int)INDEX_PCHARP(str, p))) >= 65 ) {
				goto st22;
			}
		} else {
			goto st22;
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
		_test_eof8: cs = 8; goto _test_eof; 
		_test_eof9: cs = 9; goto _test_eof; 
		_test_eof10: cs = 10; goto _test_eof; 
		_test_eof11: cs = 11; goto _test_eof; 
		_test_eof12: cs = 12; goto _test_eof; 
		_test_eof13: cs = 13; goto _test_eof; 
		_test_eof23: cs = 23; goto _test_eof; 
		_test_eof14: cs = 14; goto _test_eof; 
		_test_eof15: cs = 15; goto _test_eof; 
		_test_eof16: cs = 16; goto _test_eof; 
		_test_eof17: cs = 17; goto _test_eof; 
		_test_eof18: cs = 18; goto _test_eof; 
		_test_eof19: cs = 19; goto _test_eof; 
		_test_eof20: cs = 20; goto _test_eof; 
		_test_eof21: cs = 21; goto _test_eof; 
		_test_eof22: cs = 22; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 102 "rl/json5_mapping.rl"
	
	if (cs >= JSON5_mapping_first_final) {
		return p;
	}
	state->flags |= JSON5_ERROR;
	if (validate) {
		if (c & 1) pop_2_elems(); /* pop key and mapping */
		else pop_stack();
	}
	
	return p;
}

