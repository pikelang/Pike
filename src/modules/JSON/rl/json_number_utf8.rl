// vim:syntax=ragel

%%{
    machine JSON_number;
    alphtype char;
    include JSOND "json_defaults.rl";
    # we could be much less specific here.. but i guess its ok to ensure the 
    # format, not correctness in the sense of sscanf 
    # 
    action break {
	fpc--; fbreak;
    }

    end = [\]},:]|myspace;
    exp = [eE] >{d = 1;}. [+\-]? . digit+ . (end >break)?;
    float = '.' >{d = 1;} . digit+ . (end >break | exp)?;
    main := '-' ? . (('0' | ([1-9] . digit*)) . (end >break | float | exp)?) | float; 
}%%

static char *_parse_JSON_number_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
    int cs;
    INT_TYPE d = 0;
    FLOAT_TYPE f;

    %% write data;

    %% write init;
    %% write exec;

    if (cs >= JSON_number_first_final) {
	
		if (!state->validate) {
			PCHARP tmp = MKPCHARP(i, 0);
			if (d == 1) {
				push_float((FLOAT_TYPE)STRTOD_PCHARP(tmp, NULL));
			} else {
				struct svalue *v = Pike_sp++;
				pcharp_to_svalue_inumber(v, tmp, NULL, 10, p - i);
			}
		}

		return p;
    }

    push_int((INT_TYPE)p);
    return NULL;
}

