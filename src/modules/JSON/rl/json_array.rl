// vim:syntax=ragel

%%{
    machine JSON_array;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_value {

	state->level++;
	p = _parse_JSON(str, fpc, pe, state);
	state->level--;

	if (state->flags&JSON_ERROR) {
	    if (!(state->flags&JSON_VALIDATE)) { 
		pop_stack();
	    }
	    return p;
	} else if (!(state->flags&JSON_VALIDATE)) {
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

static ptrdiff_t _parse_JSON_array(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    struct array *a;
    int cs;
    int c = 0;

    %% write data;

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (!(state->flags&JSON_VALIDATE)) {
	a = low_allocate_array(0,5);
	push_array(a);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_array_first_final) {
	return p;
    }

    state->flags |= JSON_ERROR;

    if (!(state->flags&JSON_VALIDATE)) {
	pop_stack();
    }

    return p;
}
