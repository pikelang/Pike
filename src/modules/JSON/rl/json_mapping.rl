// vim:syntax=ragel


%%{
    machine JSON_mapping;
    alphtype int;
    include JSOND "json_defaults.rl";

    action parse_value {
		state->level++;
		i = _parse_JSON(fpc, pe, state);
		state->level--;

		if (i == NULL) {
			goto failure;
		} else if (!state->validate) {
			mapping_insert(m, &(Pike_sp[-2]), &(Pike_sp[-1]));
			pop_2_elems();
		}

		c++;
		fexec i;
    }

    action parse_key {
		state->level++;
		i = _parse_JSON_string(fpc, pe, state);
		state->level--;

		if (i == NULL) {
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

static p_wchar2 *_parse_JSON_mapping(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 *i = p;
    struct mapping *m;
    int cs;
    int c = 0;

    %% write data;

    if (!state->validate) {
		m = debug_allocate_mapping(5);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_mapping_first_final) {
		if (!state->validate) {
			push_mapping(m);
		}
		return p;
    }

    push_int((INT_TYPE)p);

failure:
    if (!state->validate) {
		if (c & 1) stack_pop_keep_top(); // remove key
		free_mapping(m);
    }

    return NULL;
}

