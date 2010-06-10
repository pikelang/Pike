// vim: syntax=ragel

#include "mapping.h"
#include "operators.h"
#include "object.h"
#include "array.h"
#include "builtin_functions.h"
#include "gc.h"


static ptrdiff_t _parse_JSON(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state);

#include "json_string_utf8.c"
#include "json_string.c"
#include "json_number.c"
#include "json_array.c"
#include "json_mapping.c"

%%{
    machine JSON;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_string { PARSE_STRING(fpc); fexec i; }
    action parse_number { PARSE(number, fpc); fexec i; }
    action parse_mapping { PARSE(mapping, fpc); fexec i; }
    action parse_array { PARSE(array, fpc); fexec i; }

    action push_true { PUSH_SPECIAL(true); }
    action push_false { PUSH_SPECIAL(false); }
    action push_null { PUSH_SPECIAL(null); }

    main := myspace* . (number_start >parse_number |
			string_start >parse_string |
			mapping_start >parse_mapping |
			array_start >parse_array |
			json_true @push_true |
			json_false @push_false |
			json_null @push_null
			) . myspace*;
}%%

static ptrdiff_t _parse_JSON(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    p_wchar2 i = p;
    int cs;
    int c = 0;
    %% write data;

    %% write init;
    %% write exec;

    if (cs >= JSON_first_final) {
	return p;
    }

    if (!(state->flags&JSON_VALIDATE) && c > 0) pop_n_elems(c);

    state->flags |= JSON_ERROR;
    return p;
}
