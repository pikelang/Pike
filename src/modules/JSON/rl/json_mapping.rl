// vim:syntax=ragel


%%{
    machine JSON_mapping;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_value {
		state->level++;
		i = _parse_JSON(str, fpc, pe, state);
		state->level--;

		if (state->flags&JSON_ERROR) {
			goto failure;
		} else if (!(state->flags&JSON_VALIDATE)) {
			mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
			pop_2_elems();
		}

		c++;
		fexec i;
    }

    action parse_key {
		state->level++;
		if (state->flags&JSON_UTF8)
		    i = _parse_JSON_string_utf8(str, fpc, pe, state);
		else
		    i = _parse_JSON_string(str, fpc, pe, state);
		state->level--;

		if (state->flags&JSON_ERROR) {
			goto failure;
		}

		c++;
		fexec i;
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
    ptrdiff_t i = p;
    struct mapping *m;
    int cs;
    int c = 0;

    %% write data;

    if (!(state->flags&JSON_VALIDATE)) {
		m = debug_allocate_mapping(5);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_mapping_first_final) {
		if (!(state->flags&JSON_VALIDATE)) {
			push_mapping(m);
		}
		return p;
    }

failure:
    state->flags |= JSON_ERROR;
    if (!(state->flags&JSON_VALIDATE)) {
		if (c & 1) stack_pop_keep_top(); // remove key
		free_mapping(m);
    }

    return p;
}

