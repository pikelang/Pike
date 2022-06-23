/* vim:syntax=ragel
 */

%%{
    machine JSON5_number;
    alphtype int;
    include JSON5D "json5_defaults.rl";
    # we could be much less specific here.. but i guess its ok to ensure the 
    # format, not correctness in the sense of sscanf 
    # 
    action break {
      fpc--;
      fbreak;
    }
    getkey ((int)INDEX_PCHARP(str, fpc));

    end = [\]},:/]|myspace;
    exp = [eE] >{d = 1;}. [+\-]? . digit+ . (end >break)?;
    hex = ('0' . [xX]  >{h = 1;}) . xdigit+ . (end > break)?;
    float = '.' >{d = 1;} . digit* . (end >break | exp)?;
    symbolic_numbers = number_identifiers >{s = 1;} ;
    main := ('+' | '-') ? . ((symbolic_numbers . (end > break)?) | ((hex) . (end > break)?) | (((digit*) . (end >break | float | exp)?) | float)); 

}%%

static ptrdiff_t _parse_JSON5_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    ptrdiff_t i = p;
    ptrdiff_t eof = pe;
    int cs;
    int d = 0, h = 0, s = 0;

    %% write data;

    %% write init;
    %% write exec;
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

