// vim:syntax=ragel

%%{
    machine JSON_array;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_value {
		state->level++;
		i = _parse_JSON(str, fpc, pe, state);
		state->level--;

		if (state->flags&JSON_ERROR) {
			if (!(state->flags&JSON_VALIDATE)) { 
				free_array(a);
			}
			return i;
		} else if (!(state->flags&JSON_VALIDATE)) {
			a = array_insert(a, &(Pike_sp[-1]), c);
			pop_stack();
		}

		c++;
		fexec i;
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
    ptrdiff_t i = p;
    struct array *a;
    int cs;
    int c = 0;

    %% write data;

    if (!(state->flags&JSON_VALIDATE)) {
	a = low_allocate_array(0,5);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_array_first_final) {
	if (!(state->flags&JSON_VALIDATE)) {
	    push_array(a);
	}
	return p;
    }

    state->flags |= JSON_ERROR;

    if (!(state->flags&JSON_VALIDATE)) {
	free_array(a);
    }

    return p;
}
