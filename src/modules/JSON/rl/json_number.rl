// vim:syntax=ragel

%%{
    machine JSON_number;
    alphtype int;
    include JSOND "json_defaults.rl";
    # we could be much less specific here.. but i guess its ok to ensure the 
    # format, not correctness in the sense of sscanf 
    # 
    action break {
		fpc--; fbreak;
    }
    getkey ((int)INDEX_PCHARP(str, fpc));

    end = [\]},:]|myspace;
    exp = [eE] >{d = 1;}. [+\-]? . digit+ . (end >break)?;
    float = '.' >{d = 1;} . digit+ . (end >break | exp)?;
    main := '-' ? . (('0' | ([1-9] . digit*)) . (end >break | float | exp)?) | float; 
}%%

static ptrdiff_t _parse_JSON_number(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    ptrdiff_t i = p;
    int cs;
    int d = 0;

    %% write data;

    %% write init;
    %% write exec;

    printf("test\n");

    if (cs >= JSON_number_first_final) {
		if (!(state->flags&JSON_VALIDATE)) {
			if (d == 1) {
				printf("pushing float\n");
				push_float((FLOAT_TYPE)STRTOD_PCHARP(ADD_PCHARP(str, i), NULL));
			} else {
				printf("pushing int\n");
				pcharp_to_svalue_inumber(Pike_sp++, ADD_PCHARP(str, i), NULL, 10, p - i + 2);
			}
		}

		return p;
    }

    state->flags |= JSON_ERROR;
    return p;
}

