// vim:syntax=ragel


%%{
    machine JSON_mapping;
    alphtype char;
    include JSOND "json_defaults.rl";

    action parse_value {
		state->level++;
		i = _parse_JSON_utf8(fpc, pe, state);
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
		i = _parse_JSON_string_utf8(fpc, pe, state);
		state->level--;

		if (i == NULL) {
			goto failure;
		}

		c++;
		fexec i;
    }

    main := ('{' . myspace* . (
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
		       )) %*{ fpc--; fbreak; };
}%%

static char *_parse_JSON_mapping_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
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


failure:
    if (!state->validate) {
		if (c & 1) stack_pop_keep_top(); // remove key
		free_mapping(m);
    }

    push_int((INT_TYPE)p);
    return NULL;
}

