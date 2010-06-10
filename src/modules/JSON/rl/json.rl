//vim: syntax=ragel

#include "mapping.h"
#include "operators.h"
#include "object.h"
#include "array.h"
#include "builtin_functions.h"
#include "gc.h"

#include "json.h"

#include "json_utf8.c"

static p_wchar2 *_parse_JSON(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state);

#include "json_string.c"
#include "json_number.c"
#include "json_array.c"
#include "json_mapping.c"

%%{
    machine JSON;
    alphtype int;
    include JSOND "json_defaults.rl";

    action parse_string { PARSE(string, fpc); fexec i; }
    action parse_number { PARSE(number, fpc); fexec i; }
    action parse_mapping { PARSE(mapping, fpc); fexec i; }
    action parse_array { PARSE(array, fpc); fexec i; }

    action push_true { PUSH_SPECIAL("true"); }
    action push_false { PUSH_SPECIAL("false"); }
    action push_null { PUSH_SPECIAL("null"); }

    main := myspace* . (number_start >parse_number |
			string_start >parse_string |
			mapping_start >parse_mapping |
			array_start >parse_array |
			json_true @push_true |
			json_false @push_false |
			json_null @push_null
			) . myspace*;
}%%

static p_wchar2 *_parse_JSON(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 *i = p;
    int cs;
    int c = 0;
    %% write data;

    %% write init;
    %% write exec;

    if (cs >= JSON_first_final) {
		return p;
    }

    if (!state->validate && c > 0) pop_n_elems(c);

    push_int((INT_TYPE)p);
    return NULL;
}
