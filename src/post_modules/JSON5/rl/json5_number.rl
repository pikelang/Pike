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

    end = [\]},:]|myspace;
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
               push_string(make_shared_binary_pcharp(ADD_PCHARP(str, i), p-i));
            } else if (h == 1) {
              // TODO handle errors better, handle possible bignums and possible better approach.
              push_string(make_shared_binary_pcharp(ADD_PCHARP(str, i), p-i));
              push_text("%x");
              f_sscanf(2);
	      if(PIKE_TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY)
                printf("not an array\n");
	      else if(Pike_sp[-1].u.array->size != 1)
                 printf("not the right number of elements!\n");
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

