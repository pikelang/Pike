// vim:syntax=ragel

#include <stdio.h>
#include "global.h"

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

    end = [\]},:]|myspace;
    exp = [eE] >{d = 1;}. [+\-]? . digit+ . (end >break)?;
    float = '.' >{d = 1;} . digit+ . (end >break | exp)?;
    main := '-' ? . (('0' | ([1-9] . digit*)) . (end >break | float | exp)?) | float; 
}%%

static p_wchar2 *_parse_JSON_number(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 *i = p;
    int cs;
    int d = 0;

    %% write data;

    %% write init;
    %% write exec;

    if (cs >= JSON_number_first_final) {
		if (!state->validate) {
			PCHARP tmp = MKPCHARP(i, 2);
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

