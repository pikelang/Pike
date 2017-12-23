/* vim:syntax=ragel
 */

%%{
    machine JSON5_array;
    alphtype int;
    include JSON5D "json5_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_value {

	state->level++;
	p = _parse_JSON5(str, fpc, pe, state);
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
	fexec p;
    }

    main := ('[' . myspace* . (
			      start: (
				']' -> final |
				value_start >parse_value . myspace* -> more
			      ),
			      more: (
				']' -> final |
				',' . myspace* -> start 
			      )
			     ) %*{ fpc--; fbreak; }) ;
}%%

static ptrdiff_t _parse_JSON5_array(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    /* GCC complains about a being used uninitialized. This is clearly wrong, so
     * lets silence this warning */
    struct array *a = a;
    int cs;
    int c = 0;
    ptrdiff_t eof = pe;
    const int validate = !(state->flags&JSON5_VALIDATE);

    %% write data;

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (validate) {
	a = low_allocate_array(0,5);
	push_array(a);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON5_array_first_final) {
	return p;
    }

    state->flags |= JSON5_ERROR;

    if (validate) {
	pop_stack();
    }

    return p;
}
