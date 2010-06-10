// vim:syntax=ragel

static char *_parse_JSON_utf8(char *p, char *pe, struct parser_state *state); 

#include "json_string_utf8.c"
#include "json_number_utf8.c"
#include "json_array_utf8.c"
#include "json_mapping_utf8.c"

%%{
    machine JSON;
    alphtype char;
    include JSOND "json_defaults.rl";

    action parse_string { PARSE(string_utf8, fpc); fexec i; }
    action parse_number { PARSE(number_utf8, fpc); fexec i; }
    action parse_mapping { PARSE(mapping_utf8, fpc); fexec i; }
    action parse_array { PARSE(array_utf8, fpc); fexec i; }

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

static char *_parse_JSON_utf8(char *p, char *pe, struct parser_state *state) {
    char *i = p;
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
