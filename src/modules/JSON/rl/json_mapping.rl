// vim:syntax=ragel


%%{
    machine JSON_mapping;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_value {
	state->level++;
	p = _parse_JSON(str, fpc, pe, state);
	state->level--;

	if (state->flags&JSON_ERROR) {
	    if (!(state->flags&JSON_VALIDATE)) {
		pop_2_elems(); // pop mapping and key
	    }
	    return p;
	} else if (!(state->flags&JSON_VALIDATE)) {
	    mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
	    pop_2_elems();
	}

	c++;
	fexec p;
    }

    action parse_key {
	state->level++;
	if (state->flags&JSON_UTF8)
	    p = _parse_JSON_string_utf8(str, fpc, pe, state);
	else
	    p = _parse_JSON_string(str, fpc, pe, state);
	state->level--;

	if (state->flags&JSON_ERROR) {
	    if (!(state->flags&JSON_VALIDATE)) {
		pop_stack(); // pop mapping
	    }
	    return p;
	}

	c++;
	fexec p;
    }

    main := '{' . myspace* . (
			start: (
			    '}' -> final |
			    '"' >parse_key . myspace* . ':' -> value
			),
			value: (
			    myspace* . value_start >parse_value . myspace* -> repeat
		        ),
			repeat: (
			    ',' . myspace* -> start |
			    '}' -> final
			)
		       ) %*{ fpc--; fbreak; };
}%%

static ptrdiff_t _parse_JSON_mapping(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    struct mapping *m;
    int cs;
    int c = 0;

    %% write data;

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (!(state->flags&JSON_VALIDATE)) {
	m = debug_allocate_mapping(5);
	push_mapping(m);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_mapping_first_final) {
	return p;
    }

    state->flags |= JSON_ERROR;
    if (!(state->flags&JSON_VALIDATE)) {
	if (c & 1) pop_2_elems(); // pop key and mapping
	else pop_stack();
    }

    return p;
}

