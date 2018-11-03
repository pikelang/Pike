
#line 1 "rl/json_number.rl"
/* vim:syntax=ragel
*/


#line 20 "rl/json_number.rl"


static ptrdiff_t _parse_JSON_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	ptrdiff_t i = p;
	int cs;
	int d=0;
	
	
	static const int JSON_number_start = 1;
	static const int JSON_number_first_final = 7;
	static const int JSON_number_error = 0;
	
	static const int JSON_number_en_main = 1;
	
	static const char _JSON_number_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON_number_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0
	};
	
	static const char _JSON_number_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON_number_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 28 "rl/json_number.rl"
	
	
	
	{
		cs = (int)JSON_number_start;
	}
	
	#line 30 "rl/json_number.rl"
	
	
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
			case 7:
			goto st_case_7;
			case 8:
			goto st_case_8;
			case 3:
			goto st_case_3;
			case 9:
			goto st_case_9;
			case 4:
			goto st_case_4;
			case 5:
			goto st_case_5;
			case 6:
			goto st_case_6;
			case 10:
			goto st_case_10;
			case 11:
			goto st_case_11;
		}
		goto st_out;
		st_case_1:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 45: {
				goto st2;
			}
			case 46: {
				goto ctr2;
			}
			case 48: {
				goto st7;
			}
		}
		if ( 49 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
			goto st11;
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
		if ( (((int)INDEX_PCHARP(str, p))) == 48 ) {
			goto st7;
		}
		if ( 49 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
			goto st11;
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
			case 13: {
				goto ctr9;
			}
			case 32: {
				goto ctr9;
			}
			case 44: {
				goto ctr9;
			}
			case 46: {
				goto ctr2;
			}
			case 47: {
				goto ctr10;
			}
			case 58: {
				goto ctr9;
			}
			case 69: {
				goto ctr11;
			}
			case 93: {
				goto ctr9;
			}
			case 101: {
				goto ctr11;
			}
			case 125: {
				goto ctr9;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto ctr9;
		}
		{
			goto st0;
		}
		ctr9:
		{
			#line 11 "rl/json_number.rl"
			
			p--; {p+= 1; cs = 8; goto _out;}
		}
		
		goto st8;
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		{
			goto st0;
		}
		ctr2:
		{
			#line 18 "rl/json_number.rl"
			d = 1;}
		
		goto st3;
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
			goto st9;
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
			case 13: {
				goto ctr9;
			}
			case 32: {
				goto ctr9;
			}
			case 44: {
				goto ctr9;
			}
			case 47: {
				goto ctr10;
			}
			case 58: {
				goto ctr9;
			}
			case 69: {
				goto ctr11;
			}
			case 93: {
				goto ctr9;
			}
			case 101: {
				goto ctr11;
			}
			case 125: {
				goto ctr9;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st9;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr9;
		}
		{
			goto st0;
		}
		ctr10:
		{
			#line 11 "rl/json_number.rl"
			
			p--; {p+= 1; cs = 4; goto _out;}
		}
		
		goto st4;
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st8;
			}
			case 47: {
				goto st8;
			}
		}
		{
			goto st0;
		}
		ctr11:
		{
			#line 17 "rl/json_number.rl"
			d = 1;}
		
		goto st5;
		st5:
		p+= 1;
		if ( p == pe )
		goto _test_eof5;
		st_case_5:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 43: {
				goto st6;
			}
			case 45: {
				goto st6;
			}
		}
		if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
			goto st10;
		}
		{
			goto st0;
		}
		st6:
		p+= 1;
		if ( p == pe )
		goto _test_eof6;
		st_case_6:
		if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
			goto st10;
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
			case 13: {
				goto ctr9;
			}
			case 32: {
				goto ctr9;
			}
			case 44: {
				goto ctr9;
			}
			case 47: {
				goto ctr10;
			}
			case 58: {
				goto ctr9;
			}
			case 93: {
				goto ctr9;
			}
			case 125: {
				goto ctr9;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st10;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr9;
		}
		{
			goto st0;
		}
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr9;
			}
			case 32: {
				goto ctr9;
			}
			case 44: {
				goto ctr9;
			}
			case 46: {
				goto ctr2;
			}
			case 47: {
				goto ctr10;
			}
			case 58: {
				goto ctr9;
			}
			case 69: {
				goto ctr11;
			}
			case 93: {
				goto ctr9;
			}
			case 101: {
				goto ctr11;
			}
			case 125: {
				goto ctr9;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st11;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr9;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof7: cs = 7; goto _test_eof; 
		_test_eof8: cs = 8; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof9: cs = 9; goto _test_eof; 
		_test_eof4: cs = 4; goto _test_eof; 
		_test_eof5: cs = 5; goto _test_eof; 
		_test_eof6: cs = 6; goto _test_eof; 
		_test_eof10: cs = 10; goto _test_eof; 
		_test_eof11: cs = 11; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 31 "rl/json_number.rl"
	
	
	if (cs >= JSON_number_first_final) {
		if (!(state->flags&JSON_VALIDATE)) {
			if (d == 1) {
				push_float((FLOAT_TYPE)STRTOD_PCHARP(ADD_PCHARP(str, i), NULL));
			} else {
				pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 10, p - i);
			}
		}
		
		return p;
	}
	
	state->flags |= JSON_ERROR;
	return p;
}

