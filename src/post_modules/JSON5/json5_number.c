
#line 1 "rl/json5_number.rl"
/* vim:syntax=ragel
*/


#line 24 "rl/json5_number.rl"


static ptrdiff_t _parse_JSON5_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	ptrdiff_t i = p;
	ptrdiff_t eof = pe;
	int cs;
	int d=0, h=0, s=0;
	
	
	static const int JSON5_number_start = 24;
	static const int JSON5_number_first_final = 24;
	static const int JSON5_number_error = 0;
	
	static const int JSON5_number_en_main = 24;
	
	static const char _JSON5_number_nfa_targs[] = {
		0, 0
	};
	
	static const char _JSON5_number_nfa_offsets[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0
	};
	
	static const char _JSON5_number_nfa_push_actions[] = {
		0, 0
	};
	
	static const char _JSON5_number_nfa_pop_trans[] = {
		0, 0
	};
	
	
	#line 33 "rl/json5_number.rl"
	
	
	
	{
		cs = (int)JSON5_number_start;
	}
	
	#line 35 "rl/json5_number.rl"
	
	
	{
		if ( p == pe )
		goto _test_eof;
		switch ( cs )
		{
			case 24:
			goto st_case_24;
			case 0:
			goto st_case_0;
			case 25:
			goto st_case_25;
			case 1:
			goto st_case_1;
			case 2:
			goto st_case_2;
			case 3:
			goto st_case_3;
			case 4:
			goto st_case_4;
			case 26:
			goto st_case_26;
			case 27:
			goto st_case_27;
			case 28:
			goto st_case_28;
			case 29:
			goto st_case_29;
			case 5:
			goto st_case_5;
			case 6:
			goto st_case_6;
			case 30:
			goto st_case_30;
			case 31:
			goto st_case_31;
			case 32:
			goto st_case_32;
			case 7:
			goto st_case_7;
			case 33:
			goto st_case_33;
			case 34:
			goto st_case_34;
			case 35:
			goto st_case_35;
			case 8:
			goto st_case_8;
			case 9:
			goto st_case_9;
			case 10:
			goto st_case_10;
			case 36:
			goto st_case_36;
			case 37:
			goto st_case_37;
			case 38:
			goto st_case_38;
			case 11:
			goto st_case_11;
			case 39:
			goto st_case_39;
			case 40:
			goto st_case_40;
			case 41:
			goto st_case_41;
			case 12:
			goto st_case_12;
			case 13:
			goto st_case_13;
			case 42:
			goto st_case_42;
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
			case 23:
			goto st_case_23;
		}
		goto st_out;
		st_case_24:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 46: {
				goto ctr27;
			}
			case 47: {
				goto ctr28;
			}
			case 48: {
				goto st31;
			}
			case 58: {
				goto ctr26;
			}
			case 69: {
				goto ctr31;
			}
			case 73: {
				goto ctr32;
			}
			case 78: {
				goto ctr33;
			}
			case 93: {
				goto ctr26;
			}
			case 101: {
				goto ctr31;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) < 43 ) {
			if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
				goto ctr24;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 45 ) {
			if ( 49 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st32;
			}
		} else {
			goto st26;
		}
		{
			goto st0;
		}
		st_case_0:
		st0:
		cs = 0;
		goto _out;
		ctr24:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 25; goto _out;}
		}
		
		goto st25;
		st25:
		p+= 1;
		if ( p == pe )
		goto _test_eof25;
		st_case_25:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto st25;
			}
			case 32: {
				goto st25;
			}
			case 47: {
				goto st1;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto st25;
		}
		{
			goto st0;
		}
		st1:
		p+= 1;
		if ( p == pe )
		goto _test_eof1;
		st_case_1:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st2;
			}
			case 47: {
				goto st4;
			}
		}
		{
			goto st0;
		}
		st2:
		p+= 1;
		if ( p == pe )
		goto _test_eof2;
		st_case_2:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st3;
		}
		{
			goto st2;
		}
		st3:
		p+= 1;
		if ( p == pe )
		goto _test_eof3;
		st_case_3:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st3;
			}
			case 47: {
				goto st25;
			}
		}
		{
			goto st2;
		}
		st4:
		p+= 1;
		if ( p == pe )
		goto _test_eof4;
		st_case_4:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st25;
		}
		{
			goto st4;
		}
		st26:
		p+= 1;
		if ( p == pe )
		goto _test_eof26;
		st_case_26:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 46: {
				goto ctr27;
			}
			case 47: {
				goto ctr28;
			}
			case 48: {
				goto st31;
			}
			case 58: {
				goto ctr26;
			}
			case 69: {
				goto ctr31;
			}
			case 73: {
				goto ctr32;
			}
			case 78: {
				goto ctr33;
			}
			case 93: {
				goto ctr26;
			}
			case 101: {
				goto ctr31;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 49 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st32;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr26:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 27; goto _out;}
		}
		
		goto st27;
		st27:
		p+= 1;
		if ( p == pe )
		goto _test_eof27;
		st_case_27:
		{
			goto st0;
		}
		ctr27:
		{
			#line 20 "rl/json5_number.rl"
			d = 1;}
		
		goto st28;
		st28:
		p+= 1;
		if ( p == pe )
		goto _test_eof28;
		st_case_28:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 47: {
				goto ctr28;
			}
			case 58: {
				goto ctr26;
			}
			case 69: {
				goto ctr31;
			}
			case 93: {
				goto ctr26;
			}
			case 101: {
				goto ctr31;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st28;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr28:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 29; goto _out;}
		}
		
		goto st29;
		st29:
		p+= 1;
		if ( p == pe )
		goto _test_eof29;
		st_case_29:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st2;
			}
			case 47: {
				goto st4;
			}
		}
		{
			goto st0;
		}
		ctr31:
		{
			#line 18 "rl/json5_number.rl"
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
			goto st30;
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
			goto st30;
		}
		{
			goto st0;
		}
		st30:
		p+= 1;
		if ( p == pe )
		goto _test_eof30;
		st_case_30:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 47: {
				goto ctr28;
			}
			case 58: {
				goto ctr26;
			}
			case 93: {
				goto ctr26;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st30;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		st31:
		p+= 1;
		if ( p == pe )
		goto _test_eof31;
		st_case_31:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 46: {
				goto ctr27;
			}
			case 47: {
				goto ctr28;
			}
			case 58: {
				goto ctr26;
			}
			case 69: {
				goto ctr31;
			}
			case 88: {
				goto ctr36;
			}
			case 93: {
				goto ctr26;
			}
			case 101: {
				goto ctr31;
			}
			case 120: {
				goto ctr36;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st32;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		st32:
		p+= 1;
		if ( p == pe )
		goto _test_eof32;
		st_case_32:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 46: {
				goto ctr27;
			}
			case 47: {
				goto ctr28;
			}
			case 58: {
				goto ctr26;
			}
			case 69: {
				goto ctr31;
			}
			case 93: {
				goto ctr26;
			}
			case 101: {
				goto ctr31;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st32;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) >= 9 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr36:
		{
			#line 19 "rl/json5_number.rl"
			h = 1;}
		
		goto st7;
		st7:
		p+= 1;
		if ( p == pe )
		goto _test_eof7;
		st_case_7:
		if ( (((int)INDEX_PCHARP(str, p))) < 65 ) {
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57 ) {
				goto st33;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
				goto st33;
			}
		} else {
			goto st33;
		}
		{
			goto st0;
		}
		st33:
		p+= 1;
		if ( p == pe )
		goto _test_eof33;
		st_case_33:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr37;
			}
			case 32: {
				goto ctr37;
			}
			case 44: {
				goto ctr38;
			}
			case 47: {
				goto ctr39;
			}
			case 58: {
				goto ctr38;
			}
			case 93: {
				goto ctr38;
			}
			case 125: {
				goto ctr38;
			}
		}
		if ( (((int)INDEX_PCHARP(str, p))) < 48 ) {
			if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
				goto ctr37;
			}
		} else if ( (((int)INDEX_PCHARP(str, p))) > 57 ) {
			if ( (((int)INDEX_PCHARP(str, p))) > 70 ) {
				if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102 ) {
					goto st33;
				}
			} else if ( (((int)INDEX_PCHARP(str, p))) >= 65 ) {
				goto st33;
			}
		} else {
			goto st33;
		}
		{
			goto st0;
		}
		ctr37:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 34; goto _out;}
		}
		
		goto st34;
		st34:
		p+= 1;
		if ( p == pe )
		goto _test_eof34;
		st_case_34:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr37;
			}
			case 32: {
				goto ctr37;
			}
			case 44: {
				goto ctr26;
			}
			case 47: {
				goto ctr40;
			}
			case 58: {
				goto ctr26;
			}
			case 93: {
				goto ctr26;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto ctr37;
		}
		{
			goto st0;
		}
		ctr40:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 35; goto _out;}
		}
		
		goto st35;
		st35:
		p+= 1;
		if ( p == pe )
		goto _test_eof35;
		st_case_35:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st8;
			}
			case 47: {
				goto st10;
			}
		}
		{
			goto st0;
		}
		st8:
		p+= 1;
		if ( p == pe )
		goto _test_eof8;
		st_case_8:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st9;
		}
		{
			goto st8;
		}
		st9:
		p+= 1;
		if ( p == pe )
		goto _test_eof9;
		st_case_9:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st9;
			}
			case 47: {
				goto st34;
			}
		}
		{
			goto st8;
		}
		st10:
		p+= 1;
		if ( p == pe )
		goto _test_eof10;
		st_case_10:
		if ( (((int)INDEX_PCHARP(str, p))) == 10 ) {
			goto st34;
		}
		{
			goto st10;
		}
		ctr38:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 36; goto _out;}
		}
		
		goto st36;
		st36:
		p+= 1;
		if ( p == pe )
		goto _test_eof36;
		st_case_36:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 44: {
				goto ctr26;
			}
			case 47: {
				goto ctr28;
			}
			case 58: {
				goto ctr26;
			}
			case 93: {
				goto ctr26;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr39:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 37; goto _out;}
		}
		
		goto st37;
		st37:
		p+= 1;
		if ( p == pe )
		goto _test_eof37;
		st_case_37:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr24;
			}
			case 32: {
				goto ctr24;
			}
			case 42: {
				goto st8;
			}
			case 44: {
				goto ctr26;
			}
			case 47: {
				goto ctr41;
			}
			case 58: {
				goto ctr26;
			}
			case 93: {
				goto ctr26;
			}
			case 125: {
				goto ctr26;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto ctr24;
		}
		{
			goto st0;
		}
		ctr41:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 38; goto _out;}
		}
		
		goto st38;
		st38:
		p+= 1;
		if ( p == pe )
		goto _test_eof38;
		st_case_38:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto st34;
			}
			case 42: {
				goto st11;
			}
		}
		{
			goto st10;
		}
		st11:
		p+= 1;
		if ( p == pe )
		goto _test_eof11;
		st_case_11:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto st39;
			}
			case 42: {
				goto st13;
			}
		}
		{
			goto st11;
		}
		ctr42:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 39; goto _out;}
		}
		
		goto st39;
		st39:
		p+= 1;
		if ( p == pe )
		goto _test_eof39;
		st_case_39:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 13: {
				goto ctr42;
			}
			case 32: {
				goto ctr42;
			}
			case 42: {
				goto st3;
			}
			case 44: {
				goto ctr43;
			}
			case 47: {
				goto ctr44;
			}
			case 58: {
				goto ctr43;
			}
			case 93: {
				goto ctr43;
			}
			case 125: {
				goto ctr43;
			}
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10 ) {
			goto ctr42;
		}
		{
			goto st2;
		}
		ctr43:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 40; goto _out;}
		}
		
		goto st40;
		st40:
		p+= 1;
		if ( p == pe )
		goto _test_eof40;
		st_case_40:
		if ( (((int)INDEX_PCHARP(str, p))) == 42 ) {
			goto st3;
		}
		{
			goto st2;
		}
		ctr44:
		{
			#line 11 "rl/json5_number.rl"
			
			p--;
			{p+= 1; cs = 41; goto _out;}
		}
		
		goto st41;
		st41:
		p+= 1;
		if ( p == pe )
		goto _test_eof41;
		st_case_41:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st12;
			}
			case 47: {
				goto st11;
			}
		}
		{
			goto st2;
		}
		st12:
		p+= 1;
		if ( p == pe )
		goto _test_eof12;
		st_case_12:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 42: {
				goto st9;
			}
			case 47: {
				goto st25;
			}
		}
		{
			goto st8;
		}
		st13:
		p+= 1;
		if ( p == pe )
		goto _test_eof13;
		st_case_13:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto st39;
			}
			case 42: {
				goto st13;
			}
			case 47: {
				goto st42;
			}
		}
		{
			goto st11;
		}
		st42:
		p+= 1;
		if ( p == pe )
		goto _test_eof42;
		st_case_42:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 9: {
				goto st42;
			}
			case 10: {
				goto st34;
			}
			case 13: {
				goto st42;
			}
			case 32: {
				goto st42;
			}
			case 47: {
				goto st14;
			}
		}
		{
			goto st10;
		}
		st14:
		p+= 1;
		if ( p == pe )
		goto _test_eof14;
		st_case_14:
		switch( (((int)INDEX_PCHARP(str, p))) ) {
			case 10: {
				goto st34;
			}
			case 42: {
				goto st11;
			}
		}
		{
			goto st10;
		}
		ctr32:
		{
			#line 21 "rl/json5_number.rl"
			s = 1;}
		
		goto st15;
		st15:
		p+= 1;
		if ( p == pe )
		goto _test_eof15;
		st_case_15:
		if ( (((int)INDEX_PCHARP(str, p))) == 110 ) {
			goto st16;
		}
		{
			goto st0;
		}
		st16:
		p+= 1;
		if ( p == pe )
		goto _test_eof16;
		st_case_16:
		if ( (((int)INDEX_PCHARP(str, p))) == 102 ) {
			goto st17;
		}
		{
			goto st0;
		}
		st17:
		p+= 1;
		if ( p == pe )
		goto _test_eof17;
		st_case_17:
		if ( (((int)INDEX_PCHARP(str, p))) == 105 ) {
			goto st18;
		}
		{
			goto st0;
		}
		st18:
		p+= 1;
		if ( p == pe )
		goto _test_eof18;
		st_case_18:
		if ( (((int)INDEX_PCHARP(str, p))) == 110 ) {
			goto st19;
		}
		{
			goto st0;
		}
		st19:
		p+= 1;
		if ( p == pe )
		goto _test_eof19;
		st_case_19:
		if ( (((int)INDEX_PCHARP(str, p))) == 105 ) {
			goto st20;
		}
		{
			goto st0;
		}
		st20:
		p+= 1;
		if ( p == pe )
		goto _test_eof20;
		st_case_20:
		if ( (((int)INDEX_PCHARP(str, p))) == 116 ) {
			goto st21;
		}
		{
			goto st0;
		}
		st21:
		p+= 1;
		if ( p == pe )
		goto _test_eof21;
		st_case_21:
		if ( (((int)INDEX_PCHARP(str, p))) == 121 ) {
			goto st36;
		}
		{
			goto st0;
		}
		ctr33:
		{
			#line 21 "rl/json5_number.rl"
			s = 1;}
		
		goto st22;
		st22:
		p+= 1;
		if ( p == pe )
		goto _test_eof22;
		st_case_22:
		if ( (((int)INDEX_PCHARP(str, p))) == 97 ) {
			goto st23;
		}
		{
			goto st0;
		}
		st23:
		p+= 1;
		if ( p == pe )
		goto _test_eof23;
		st_case_23:
		if ( (((int)INDEX_PCHARP(str, p))) == 78 ) {
			goto st36;
		}
		{
			goto st0;
		}
		st_out:
		_test_eof25: cs = 25; goto _test_eof; 
		_test_eof1: cs = 1; goto _test_eof; 
		_test_eof2: cs = 2; goto _test_eof; 
		_test_eof3: cs = 3; goto _test_eof; 
		_test_eof4: cs = 4; goto _test_eof; 
		_test_eof26: cs = 26; goto _test_eof; 
		_test_eof27: cs = 27; goto _test_eof; 
		_test_eof28: cs = 28; goto _test_eof; 
		_test_eof29: cs = 29; goto _test_eof; 
		_test_eof5: cs = 5; goto _test_eof; 
		_test_eof6: cs = 6; goto _test_eof; 
		_test_eof30: cs = 30; goto _test_eof; 
		_test_eof31: cs = 31; goto _test_eof; 
		_test_eof32: cs = 32; goto _test_eof; 
		_test_eof7: cs = 7; goto _test_eof; 
		_test_eof33: cs = 33; goto _test_eof; 
		_test_eof34: cs = 34; goto _test_eof; 
		_test_eof35: cs = 35; goto _test_eof; 
		_test_eof8: cs = 8; goto _test_eof; 
		_test_eof9: cs = 9; goto _test_eof; 
		_test_eof10: cs = 10; goto _test_eof; 
		_test_eof36: cs = 36; goto _test_eof; 
		_test_eof37: cs = 37; goto _test_eof; 
		_test_eof38: cs = 38; goto _test_eof; 
		_test_eof11: cs = 11; goto _test_eof; 
		_test_eof39: cs = 39; goto _test_eof; 
		_test_eof40: cs = 40; goto _test_eof; 
		_test_eof41: cs = 41; goto _test_eof; 
		_test_eof12: cs = 12; goto _test_eof; 
		_test_eof13: cs = 13; goto _test_eof; 
		_test_eof42: cs = 42; goto _test_eof; 
		_test_eof14: cs = 14; goto _test_eof; 
		_test_eof15: cs = 15; goto _test_eof; 
		_test_eof16: cs = 16; goto _test_eof; 
		_test_eof17: cs = 17; goto _test_eof; 
		_test_eof18: cs = 18; goto _test_eof; 
		_test_eof19: cs = 19; goto _test_eof; 
		_test_eof20: cs = 20; goto _test_eof; 
		_test_eof21: cs = 21; goto _test_eof; 
		_test_eof22: cs = 22; goto _test_eof; 
		_test_eof23: cs = 23; goto _test_eof; 
		
		_test_eof: {}
		_out: {}
	}
	
	#line 36 "rl/json5_number.rl"
	
	if (cs >= JSON5_number_first_final) {
		if (!(state->flags&JSON5_VALIDATE)) {
			if (s == 1) {
				s = (int)INDEX_PCHARP(str, i);
				if(s == '+') s=0, i++; 
				else if(s == '-') s = -1, i++;
				// -symbol
				if(s == -1) {
					s = (int)INDEX_PCHARP(str, i);
					if(s == 'I') {
						push_float(-MAKE_INF());
					} else if(s == 'N') {
						push_float(MAKE_NAN()); /* note sign on -NaN has no meaning */
					} else { /* should never get here */
						state->flags |= JSON5_ERROR; 
						return p;
					}
				} else {
					/* if we had a + sign, look at the next digit, otherwise use the value. */
					if(s == 0) 
					s = (int)INDEX_PCHARP(str, i);
					
					if(s == 'I') {
						push_float(MAKE_INF());
					} else if(s == 'N') {
						push_float(MAKE_NAN()); /* note sign on -NaN has no meaning */
					} else { /* should never get here */
						state->flags |= JSON5_ERROR; 
						return p;
					}
				}
			} else if (h == 1) {
				// TODO handle errors better, handle possible bignums and possible better approach.
				push_string(make_shared_binary_pcharp(ADD_PCHARP(str, i), p-i));
				push_text("%x");
				f_sscanf(2);
				if((PIKE_TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY) || (Pike_sp[-1].u.array->size != 1)) {
					state->flags |= JSON5_ERROR;
					return p;
				}
				else {
					push_int(ITEM(Pike_sp[-1].u.array)[0].u.integer);
					stack_swap();
					pop_stack();
				}
			} else if (d == 1) {
				push_float((FLOAT_TYPE)STRTOD_PCHARP(ADD_PCHARP(str, i), NULL));
			} else {
				pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 10, p - i);
			}
		}
		
		return p;
	}
	state->flags |= JSON5_ERROR;
	return p;
}

