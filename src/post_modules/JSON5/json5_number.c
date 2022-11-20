static ptrdiff_t _parse_JSON5_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
	ptrdiff_t i = p;
	ptrdiff_t eof = pe;
	int cs;
	int d = 0, h = 0, s = 0;
	
	static const int  JSON5_number_start  = 24;
	static const int  JSON5_number_first_final  = 24;
	static const int  JSON5_number_error  = 0;
	static const int  JSON5_number_en_main  = 24;
	{
		cs = ( int ) JSON5_number_start;
		
	}
	{
		switch ( cs  ) {
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
		p+= 1;
		st_case_24:
		if ( p == pe  )
		goto _out24;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 46:
			{
				goto _ctr35;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 48:
			{
				goto _st31;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 69:
			{
				goto _ctr39;
				
			}
			case 73:
			{
				goto _ctr40;
				
			}
			case 78:
			{
				goto _ctr41;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 101:
			{
				goto _ctr39;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) < 43  )
		{
			if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
			{
				goto _ctr32;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) > 45  )
		{
			if ( 49 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st32;
				
			}
			
			
		}
		
		else
		{
			goto _st26;
			
		}
		
		goto _st0;
		_st0:
		st_case_0:
		goto _out0;
		_ctr32:
		{
			p--;
			{
				p+= 1;
				cs = 25;
				goto _out;
			}
			
			
		}
		
		
		goto _st25;
		_st25:
		p+= 1;
		st_case_25:
		if ( p == pe  )
		goto _out25;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _st25;
				
			}
			case 32:
			{
				goto _st25;
				
			}
			case 47:
			{
				goto _st1;
				
			}
			
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
		{
			goto _st25;
			
		}
		
		goto _st0;
		_st1:
		p+= 1;
		st_case_1:
		if ( p == pe  )
		goto _out1;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st2;
				
			}
			case 47:
			{
				goto _st4;
				
			}
			
		}
		goto _st0;
		_st2:
		p+= 1;
		st_case_2:
		if ( p == pe  )
		goto _out2;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 42  )
		{
			goto _st3;
			
		}
		
		goto _st2;
		_st3:
		p+= 1;
		st_case_3:
		if ( p == pe  )
		goto _out3;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st3;
				
			}
			case 47:
			{
				goto _st25;
				
			}
			
		}
		goto _st2;
		_st4:
		p+= 1;
		st_case_4:
		if ( p == pe  )
		goto _out4;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 10  )
		{
			goto _st25;
			
		}
		
		goto _st4;
		_st26:
		p+= 1;
		st_case_26:
		if ( p == pe  )
		goto _out26;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 46:
			{
				goto _ctr35;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 48:
			{
				goto _st31;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 69:
			{
				goto _ctr39;
				
			}
			case 73:
			{
				goto _ctr40;
				
			}
			case 78:
			{
				goto _ctr41;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 101:
			{
				goto _ctr39;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10  )
		{
			if ( 49 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st32;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) >= 9  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_ctr34:
		{
			p--;
			{
				p+= 1;
				cs = 27;
				goto _out;
			}
			
			
		}
		
		
		goto _st27;
		_st27:
		p+= 1;
		st_case_27:
		if ( p == pe  )
		goto _out27;
		
		goto _st0;
		_ctr35:
		{
			d = 1;
		}
		
		
		goto _st28;
		_st28:
		p+= 1;
		st_case_28:
		if ( p == pe  )
		goto _out28;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 69:
			{
				goto _ctr39;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 101:
			{
				goto _ctr39;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10  )
		{
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st28;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) >= 9  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_ctr36:
		{
			p--;
			{
				p+= 1;
				cs = 29;
				goto _out;
			}
			
			
		}
		
		
		goto _st29;
		_st29:
		p+= 1;
		st_case_29:
		if ( p == pe  )
		goto _out29;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st2;
				
			}
			case 47:
			{
				goto _st4;
				
			}
			
		}
		goto _st0;
		_ctr39:
		{
			d = 1;
		}
		
		
		goto _st5;
		_st5:
		p+= 1;
		st_case_5:
		if ( p == pe  )
		goto _out5;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 43:
			{
				goto _st6;
				
			}
			case 45:
			{
				goto _st6;
				
			}
			
		}
		if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
		{
			goto _st30;
			
		}
		
		goto _st0;
		_st6:
		p+= 1;
		st_case_6:
		if ( p == pe  )
		goto _out6;
		
		if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
		{
			goto _st30;
			
		}
		
		goto _st0;
		_st30:
		p+= 1;
		st_case_30:
		if ( p == pe  )
		goto _out30;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10  )
		{
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st30;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) >= 9  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_st31:
		p+= 1;
		st_case_31:
		if ( p == pe  )
		goto _out31;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 46:
			{
				goto _ctr35;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 69:
			{
				goto _ctr39;
				
			}
			case 88:
			{
				goto _ctr45;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 101:
			{
				goto _ctr39;
				
			}
			case 120:
			{
				goto _ctr45;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10  )
		{
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st32;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) >= 9  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_st32:
		p+= 1;
		st_case_32:
		if ( p == pe  )
		goto _out32;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 46:
			{
				goto _ctr35;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 69:
			{
				goto _ctr39;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 101:
			{
				goto _ctr39;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) > 10  )
		{
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st32;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) >= 9  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_ctr45:
		{
			h = 1;
		}
		
		
		goto _st7;
		_st7:
		p+= 1;
		st_case_7:
		if ( p == pe  )
		goto _out7;
		
		if ( (((int)INDEX_PCHARP(str, p))) < 65  )
		{
			if ( 48 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 57  )
			{
				goto _st33;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) > 70  )
		{
			if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102  )
			{
				goto _st33;
				
			}
			
			
		}
		
		else
		{
			goto _st33;
			
		}
		
		goto _st0;
		_st33:
		p+= 1;
		st_case_33:
		if ( p == pe  )
		goto _out33;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr46;
				
			}
			case 32:
			{
				goto _ctr46;
				
			}
			case 44:
			{
				goto _ctr47;
				
			}
			case 47:
			{
				goto _ctr48;
				
			}
			case 58:
			{
				goto _ctr47;
				
			}
			case 93:
			{
				goto _ctr47;
				
			}
			case 125:
			{
				goto _ctr47;
				
			}
			
		}
		if ( (((int)INDEX_PCHARP(str, p))) < 48  )
		{
			if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
			{
				goto _ctr46;
				
			}
			
			
		}
		
		else if ( (((int)INDEX_PCHARP(str, p))) > 57  )
		{
			if ( (((int)INDEX_PCHARP(str, p))) > 70  )
			{
				if ( 97 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 102  )
				{
					goto _st33;
					
				}
				
				
			}
			
			else if ( (((int)INDEX_PCHARP(str, p))) >= 65  )
			{
				goto _st33;
				
			}
			
			
		}
		
		else
		{
			goto _st33;
			
		}
		
		goto _st0;
		_ctr46:
		{
			p--;
			{
				p+= 1;
				cs = 34;
				goto _out;
			}
			
			
		}
		
		
		goto _st34;
		_st34:
		p+= 1;
		st_case_34:
		if ( p == pe  )
		goto _out34;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr46;
				
			}
			case 32:
			{
				goto _ctr46;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 47:
			{
				goto _ctr49;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
		{
			goto _ctr46;
			
		}
		
		goto _st0;
		_ctr49:
		{
			p--;
			{
				p+= 1;
				cs = 35;
				goto _out;
			}
			
			
		}
		
		
		goto _st35;
		_st35:
		p+= 1;
		st_case_35:
		if ( p == pe  )
		goto _out35;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st8;
				
			}
			case 47:
			{
				goto _st10;
				
			}
			
		}
		goto _st0;
		_st8:
		p+= 1;
		st_case_8:
		if ( p == pe  )
		goto _out8;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 42  )
		{
			goto _st9;
			
		}
		
		goto _st8;
		_st9:
		p+= 1;
		st_case_9:
		if ( p == pe  )
		goto _out9;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st9;
				
			}
			case 47:
			{
				goto _st34;
				
			}
			
		}
		goto _st8;
		_st10:
		p+= 1;
		st_case_10:
		if ( p == pe  )
		goto _out10;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 10  )
		{
			goto _st34;
			
		}
		
		goto _st10;
		_ctr47:
		{
			p--;
			{
				p+= 1;
				cs = 36;
				goto _out;
			}
			
			
		}
		
		
		goto _st36;
		_st36:
		p+= 1;
		st_case_36:
		if ( p == pe  )
		goto _out36;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 47:
			{
				goto _ctr36;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_ctr48:
		{
			p--;
			{
				p+= 1;
				cs = 37;
				goto _out;
			}
			
			
		}
		
		
		goto _st37;
		_st37:
		p+= 1;
		st_case_37:
		if ( p == pe  )
		goto _out37;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr32;
				
			}
			case 32:
			{
				goto _ctr32;
				
			}
			case 42:
			{
				goto _st8;
				
			}
			case 44:
			{
				goto _ctr34;
				
			}
			case 47:
			{
				goto _ctr52;
				
			}
			case 58:
			{
				goto _ctr34;
				
			}
			case 93:
			{
				goto _ctr34;
				
			}
			case 125:
			{
				goto _ctr34;
				
			}
			
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
		{
			goto _ctr32;
			
		}
		
		goto _st0;
		_ctr52:
		{
			p--;
			{
				p+= 1;
				cs = 38;
				goto _out;
			}
			
			
		}
		
		
		goto _st38;
		_st38:
		p+= 1;
		st_case_38:
		if ( p == pe  )
		goto _out38;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 10:
			{
				goto _st34;
				
			}
			case 42:
			{
				goto _st11;
				
			}
			
		}
		goto _st10;
		_st11:
		p+= 1;
		st_case_11:
		if ( p == pe  )
		goto _out11;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 10:
			{
				goto _st39;
				
			}
			case 42:
			{
				goto _st13;
				
			}
			
		}
		goto _st11;
		_ctr54:
		{
			p--;
			{
				p+= 1;
				cs = 39;
				goto _out;
			}
			
			
		}
		
		
		goto _st39;
		_st39:
		p+= 1;
		st_case_39:
		if ( p == pe  )
		goto _out39;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 13:
			{
				goto _ctr54;
				
			}
			case 32:
			{
				goto _ctr54;
				
			}
			case 42:
			{
				goto _st3;
				
			}
			case 44:
			{
				goto _ctr55;
				
			}
			case 47:
			{
				goto _ctr56;
				
			}
			case 58:
			{
				goto _ctr55;
				
			}
			case 93:
			{
				goto _ctr55;
				
			}
			case 125:
			{
				goto _ctr55;
				
			}
			
		}
		if ( 9 <= (((int)INDEX_PCHARP(str, p))) && (((int)INDEX_PCHARP(str, p))) <= 10  )
		{
			goto _ctr54;
			
		}
		
		goto _st2;
		_ctr55:
		{
			p--;
			{
				p+= 1;
				cs = 40;
				goto _out;
			}
			
			
		}
		
		
		goto _st40;
		_st40:
		p+= 1;
		st_case_40:
		if ( p == pe  )
		goto _out40;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 42  )
		{
			goto _st3;
			
		}
		
		goto _st2;
		_ctr56:
		{
			p--;
			{
				p+= 1;
				cs = 41;
				goto _out;
			}
			
			
		}
		
		
		goto _st41;
		_st41:
		p+= 1;
		st_case_41:
		if ( p == pe  )
		goto _out41;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st12;
				
			}
			case 47:
			{
				goto _st11;
				
			}
			
		}
		goto _st2;
		_st12:
		p+= 1;
		st_case_12:
		if ( p == pe  )
		goto _out12;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 42:
			{
				goto _st9;
				
			}
			case 47:
			{
				goto _st25;
				
			}
			
		}
		goto _st8;
		_st13:
		p+= 1;
		st_case_13:
		if ( p == pe  )
		goto _out13;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 10:
			{
				goto _st39;
				
			}
			case 42:
			{
				goto _st13;
				
			}
			case 47:
			{
				goto _st42;
				
			}
			
		}
		goto _st11;
		_st42:
		p+= 1;
		st_case_42:
		if ( p == pe  )
		goto _out42;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 9:
			{
				goto _st42;
				
			}
			case 10:
			{
				goto _st34;
				
			}
			case 13:
			{
				goto _st42;
				
			}
			case 32:
			{
				goto _st42;
				
			}
			case 47:
			{
				goto _st14;
				
			}
			
		}
		goto _st10;
		_st14:
		p+= 1;
		st_case_14:
		if ( p == pe  )
		goto _out14;
		
		switch ( (((int)INDEX_PCHARP(str, p))) ) {
			case 10:
			{
				goto _st34;
				
			}
			case 42:
			{
				goto _st11;
				
			}
			
		}
		goto _st10;
		_ctr40:
		{
			s = 1;
		}
		
		
		goto _st15;
		_st15:
		p+= 1;
		st_case_15:
		if ( p == pe  )
		goto _out15;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 110  )
		{
			goto _st16;
			
		}
		
		goto _st0;
		_st16:
		p+= 1;
		st_case_16:
		if ( p == pe  )
		goto _out16;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 102  )
		{
			goto _st17;
			
		}
		
		goto _st0;
		_st17:
		p+= 1;
		st_case_17:
		if ( p == pe  )
		goto _out17;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 105  )
		{
			goto _st18;
			
		}
		
		goto _st0;
		_st18:
		p+= 1;
		st_case_18:
		if ( p == pe  )
		goto _out18;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 110  )
		{
			goto _st19;
			
		}
		
		goto _st0;
		_st19:
		p+= 1;
		st_case_19:
		if ( p == pe  )
		goto _out19;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 105  )
		{
			goto _st20;
			
		}
		
		goto _st0;
		_st20:
		p+= 1;
		st_case_20:
		if ( p == pe  )
		goto _out20;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 116  )
		{
			goto _st21;
			
		}
		
		goto _st0;
		_st21:
		p+= 1;
		st_case_21:
		if ( p == pe  )
		goto _out21;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 121  )
		{
			goto _st36;
			
		}
		
		goto _st0;
		_ctr41:
		{
			s = 1;
		}
		
		
		goto _st22;
		_st22:
		p+= 1;
		st_case_22:
		if ( p == pe  )
		goto _out22;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 97  )
		{
			goto _st23;
			
		}
		
		goto _st0;
		_st23:
		p+= 1;
		st_case_23:
		if ( p == pe  )
		goto _out23;
		
		if ( (((int)INDEX_PCHARP(str, p))) == 78  )
		{
			goto _st36;
			
		}
		
		goto _st0;
		_out24: cs = 24;
		goto _out; 
		_out0: cs = 0;
		goto _out; 
		_out25: cs = 25;
		goto _out; 
		_out1: cs = 1;
		goto _out; 
		_out2: cs = 2;
		goto _out; 
		_out3: cs = 3;
		goto _out; 
		_out4: cs = 4;
		goto _out; 
		_out26: cs = 26;
		goto _out; 
		_out27: cs = 27;
		goto _out; 
		_out28: cs = 28;
		goto _out; 
		_out29: cs = 29;
		goto _out; 
		_out5: cs = 5;
		goto _out; 
		_out6: cs = 6;
		goto _out; 
		_out30: cs = 30;
		goto _out; 
		_out31: cs = 31;
		goto _out; 
		_out32: cs = 32;
		goto _out; 
		_out7: cs = 7;
		goto _out; 
		_out33: cs = 33;
		goto _out; 
		_out34: cs = 34;
		goto _out; 
		_out35: cs = 35;
		goto _out; 
		_out8: cs = 8;
		goto _out; 
		_out9: cs = 9;
		goto _out; 
		_out10: cs = 10;
		goto _out; 
		_out36: cs = 36;
		goto _out; 
		_out37: cs = 37;
		goto _out; 
		_out38: cs = 38;
		goto _out; 
		_out11: cs = 11;
		goto _out; 
		_out39: cs = 39;
		goto _out; 
		_out40: cs = 40;
		goto _out; 
		_out41: cs = 41;
		goto _out; 
		_out12: cs = 12;
		goto _out; 
		_out13: cs = 13;
		goto _out; 
		_out42: cs = 42;
		goto _out; 
		_out14: cs = 14;
		goto _out; 
		_out15: cs = 15;
		goto _out; 
		_out16: cs = 16;
		goto _out; 
		_out17: cs = 17;
		goto _out; 
		_out18: cs = 18;
		goto _out; 
		_out19: cs = 19;
		goto _out; 
		_out20: cs = 20;
		goto _out; 
		_out21: cs = 21;
		goto _out; 
		_out22: cs = 22;
		goto _out; 
		_out23: cs = 23;
		goto _out; 
		_out: {
		
		}
		
	}
	if (cs >= JSON5_number_first_final) {
		if (!(state->flags&JSON5_VALIDATE)) {
			if (s == 1) {
				s = (int)INDEX_PCHARP(str, i);
				if(s == '+') s = 0, i++; 
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
				pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 16, p - i);
			} else if (d == 1) {
				#if SIZEOF_FLOAT_TYPE > SIZEOF_DOUBLE
				push_float((FLOAT_TYPE)STRTOLD_PCHARP(ADD_PCHARP(str, i), NULL));
				#else
				push_float((FLOAT_TYPE)STRTOD_PCHARP(ADD_PCHARP(str, i), NULL));
				#endif
			} else {
				pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 10, p - i);
			}
		}
		
		return p;
	}
	state->flags |= JSON5_ERROR;
	return p;
}

