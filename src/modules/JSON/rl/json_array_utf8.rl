// vim:syntax=ragel

%%{
    machine JSON_array;
    alphtype char;
    include JSOND "json_defaults.rl";

    action parse_value {
	state->level++;
	i = _parse_JSON_utf8(fpc, pe, state);
	state->level--;

	if (i == NULL) {
	    if (!state->validate) { 
		free_array(a);
	    }
	    return NULL;
	} else if (!state->validate) {
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
			     ) )%*{ fpc--; fbreak; };
}%%

static char *_parse_JSON_array_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
    struct array *a;
    int cs;
    int c = 0;

    %% write data;

    if (!state->validate) {
	a = low_allocate_array(0,5);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_array_first_final) {
	if (!state->validate) {
	    push_array(a);
	}
	return p;
    }

    if (!state->validate) {
	free_array(a);
    }


    push_int((INT_TYPE)p);
    return NULL;
}
