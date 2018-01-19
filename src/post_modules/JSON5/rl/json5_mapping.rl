/* vim:syntax=ragel
 */

%%{
    machine JSON5_mapping;
    alphtype int;
    include JSON5D "json5_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_value {
	state->level++;
	p = _parse_JSON5(str, fpc, pe, state);
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
	fexec p;
    }

    action parse_key {
	state->level++;
	if (state->flags&JSON5_UTF8)
	    p = _parse_JSON5_string_utf8(str, fpc, pe, state);
	else
	    p = _parse_JSON5_string(str, fpc, pe, state);
	state->level--;

	if (state->flags&JSON5_ERROR) {
	    if (validate) {
		pop_stack(); /* pop mapping */
	    }
	    return p;
	}

	c++;
	fexec p;
    }

    action identifier_start {
      identifier_start = fpc;
    }
 
    action identifier_end {
      ptrdiff_t len = fpc - identifier_start;
      if (validate) {
       init_string_builder(&s, 0);
       string_builder_append(&s, ADD_PCHARP(str, identifier_start), len);
        push_string(finish_string_builder(&s));
      }
    }

    main := '{' . myspace* . (
			start: (
			    '}' -> final |
			    identifier >identifier_start %identifier_end . myspace* . ':' -> value |
			    ('"'|'\'') >parse_key . myspace* . ':' -> value
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

static ptrdiff_t _parse_JSON5_mapping(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    /* GCC complains about a being used uninitialized. This is clearly wrong, so
     * lets silence this warning */
    struct mapping *m = m;
    int cs;
    int c = 0;
    struct string_builder s;
    ptrdiff_t eof = pe;
    ptrdiff_t identifier_start;

    const int validate = !(state->flags&JSON5_VALIDATE);

    %% write data;

    /* Check stacks since we have uncontrolled recursion here. */
    check_stack (10);
    check_c_stack (1024);

    if (validate) {
	m = debug_allocate_mapping(5);
	push_mapping(m);
    }

    %% write init;
    %% write exec;
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

