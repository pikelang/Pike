#define PUSH_SPECIAL(X) do {if (!state->validate) {						\
    push_text("Standards.JSON." X); 								\
    APPLY_MASTER("resolv", 1);											\
    if (((struct svalue*)(Pike_sp - 1))->type != PIKE_T_OBJECT) { 		\
	Pike_error("Could not resolv program '%s'\n.", 						\
		   "Standards.JSON." X );  								\
    } 																	\
} c++; if (state->level > 0) return p+1; } while(0)

#define PARSE(X, FPC) do {												\
    state->level++;														\
    i = _parse_JSON_##X(FPC, pe, state);								\
    state->level--;														\
    if (i == NULL) {													\
		return NULL;													\
    } else if (state->level > 0) return i;											\
    c++;																\
} while(0)								

#define JSON_CONVERT(a,b) do {											\
	if (a->len < 0 || (a->len << 2) >> 2 != a->len) {		\
		Pike_error("Value passed to malloc would overflowed.\n");		\
	}																	\
    switch (a->size_shift) {											\
    case 0:																\
		b=(p_wchar2 *)malloc(sizeof(p_wchar2) * a->len);		\
		if (b == NULL) {												\
			SIMPLE_OUT_OF_MEMORY_ERROR (state.validate == 1 			\
						? "validate" : "parse", sizeof(p_wchar2) * a->len); \
		}																\
        convert_0_to_2(b,STR0(a), a->len);								\
		break;															\
    case 1:																\
		b=(p_wchar2 *)malloc(sizeof(p_wchar2) * a->len);				\
		if (b == NULL) {												\
			SIMPLE_OUT_OF_MEMORY_ERROR (state.validate == 1 			\
						? "validate" : "parse", sizeof(p_wchar2) * a->len); \
		}																\
		convert_1_to_2(b,STR1(a), a->len);								\
		break;															\
    case 2:																\
		b = STR2(a);													\
		break;															\
    default:															\
		Pike_error("Bad string shift.\n");								\
    } } while (0)

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
    short validate;
    struct pike_string *data;
};

