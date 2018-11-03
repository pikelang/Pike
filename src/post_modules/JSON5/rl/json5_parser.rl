/* vim: syntax=ragel
 */

#include "mapping.h"
#include "operators.h"
#include "object.h"
#include "array.h"
#include "builtin_functions.h"
#include "gc.h"


static ptrdiff_t _parse_JSON5(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state);

#include "json5_string_utf8.c"
#include "json5_string.c"
#include "json5_number.c"
#include "json5_array.c"
#include "json5_mapping.c"

%%{
    machine JSON5;
    alphtype int;
    include JSON5D "json5_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action parse_string { PARSE_STRING(fpc); fexec i; }
    action parse_number { PARSE(number, fpc); fexec i; }
    action parse_mapping { PARSE(mapping, fpc); fexec i; }
    action parse_array { PARSE(array, fpc); fexec i; }
    action push_true { PUSH_SPECIAL(true); }
    action push_false { PUSH_SPECIAL(false); }
    action push_null { PUSH_SPECIAL(null); }
 
    main := myspace* .  (number_start >parse_number |
			string_start >parse_string |
			mapping_start >parse_mapping |
			array_start >parse_array |
			json5_true @push_true |
			json5_false @push_false |
			json5_null @push_null
			) . myspace*;
}%%

static ptrdiff_t _parse_JSON5(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    ptrdiff_t i = p;
    int cs;
    int c = 0;
    ptrdiff_t eof = pe;
    %% write data;

    %% write init;
    %% write exec;

    if (cs >= JSON5_first_final) {
	return p;
    }

    if ( (state->flags & JSON5_FIRST_VALUE) && (c==1) )
    {
      state->flags &= ~JSON5_ERROR;
      return p-1;
    }

    if (!(state->flags&JSON5_VALIDATE) && c > 0) pop_n_elems(c);

    state->flags |= JSON5_ERROR;
    return p;
}
