#define JSON_UTF8	1
#define JSON_ERROR	2
#define JSON_VALIDATE	4

#define PUSH_SPECIAL(X) do {if (!(state->flags&JSON_VALIDATE)) {						\
    push_text("Standards.JSON." X); 								\
    APPLY_MASTER("resolv", 1);											\
    if (((struct svalue*)(Pike_sp - 1))->type != PIKE_T_OBJECT) { 		\
	Pike_error("Could not resolv program '%s'\n.", 						\
		   "Standards.JSON." X );  								\
    } 																	\
} c++; if (state->level > 0) return p+1; } while(0)

#define PARSE(X, FPC) do {												\
    state->level++;														\
    i = _parse_JSON_##X(str, FPC, pe, state);								\
    state->level--;														\
    if (state->flags&JSON_ERROR) {													\
	    return i;													\
    } else if (state->level > 0) return i;											\
    c++;																\
} while(0)								

#define PARSE_STRING(FPC) do {												\
    state->level++;														\
    if (state->flags&JSON_UTF8)\
	i = _parse_JSON_string_utf8(str, FPC, pe, state);								\
    else\
	i = _parse_JSON_string(str, FPC, pe, state);								\
    state->level--;														\
    if (state->flags&JSON_ERROR) {													\
	    return i;													\
    } else if (state->level > 0) return i;											\
    c++;																\
} while(0)								

#define IS_NUNICODE(x)	((x) < 0 || ((x) > 0xd7ff && (x) < 0xe000) || (x) > 0x10ffff)
#define IS_NUNICODE1(x)	((x) < 0 || ((x) > 0xd7ff && (x) < 0xe000))

#define CHECK_UNICODE(x) do {											\
    switch ((x)->size_shift) {											\
    case 0:																\
		break;															\
    case 1: {															\
		p_wchar1 *s = STR1((x));										\
		p_wchar1 *se = s + (x)->len;									\
																		\
		for (;s < se; s++)												\
			if (IS_NUNICODE1(*s)) {										\
				Pike_error("String contains non-unicode char.\n");		\
			}															\
		break;	}														\
    case 2: {															\
		p_wchar2 *s = STR2((x));										\
		p_wchar2 *se = s + (x)->len;									\
																		\
		for (;s < se; s++) 												\
			if (IS_NUNICODE(*s)) {										\
				Pike_error("String contains non-unicode char.\n");		\
			}															\
		break;}															\
    default:															\
		Pike_error("Bad string shift.\n");								\
    }} while (0)


struct parser_state {
    unsigned int level;
    int flags;
    struct pike_string *data;
};

