// vim:syntax=ragel
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)

%%{
    machine JSON_string;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action hex0 {
	if (!(state->flags&JSON_VALIDATE)) temp = HEX2DEC(fc);
    }

    action hex1 {
	if (!(state->flags&JSON_VALIDATE)) {
	    temp *= 16;
	    temp += HEX2DEC(fc);

	    if (IS_NUNICODE(temp)) {
		fpc--; fbreak;
	    }
	}
    }

    action hex2 {
	if (!(state->flags&JSON_VALIDATE)) string_builder_putchar(&s, temp);
    }

    action add_unquote {
	if (!(state->flags&JSON_VALIDATE)) switch(fc) {
	    case '"':
	    case '/':
	    case '\\':      string_builder_putchar(&s, fc); break;
	    case 'b':       string_builder_putchar(&s, '\b'); break;
	    case 'f':       string_builder_putchar(&s, '\f'); break;
	    case 'n':       string_builder_putchar(&s, '\n'); break;
	    case 'r':       string_builder_putchar(&s, '\r'); break;
	    case 't':       string_builder_putchar(&s, '\t'); break;
	}
    }

    action mark {
	mark = fpc;
    }

    action mark_next { mark = fpc + 1; }

    action string_append {
	if (fpc - mark > 0) {
	    if (!(state->flags&JSON_VALIDATE))
		    string_builder_append(&s, ADD_PCHARP(str, mark), fpc - mark);
        }
    }

    main := '"' . (
		   start: (
		       '"' >string_append -> final |
		       '\\' >string_append -> unquote |
		       (unicode - [\\"]) -> start
		   ),
		   unquote: (
		       ["\\/bfnrt] >add_unquote -> start |
		       'u' . xdigit >hex0 . (xdigit{3} $hex1) @hex2 -> start
		   ) @mark_next
		  ) >mark %*{ fpc--; fbreak; };
}%%

static ptrdiff_t _parse_JSON_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    int temp = 0;
    ptrdiff_t start = p, mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;

    %% write data;

    if (!(state->flags&JSON_VALIDATE)) {
	init_string_builder(&s, 0);
	SET_ONERROR (handle, free_string_builder, &s);
    }

    %% write init;
    %% write exec;

    if (cs < JSON_string_first_final) {
	if (!(state->flags&JSON_VALIDATE)) {
	    UNSET_ONERROR(handle);
	    free_string_builder(&s);
	}

	state->flags |= JSON_ERROR;
	if (p == pe) {
	    err_msg = "Unterminated string";
	    return start;
	}
	return p;
    }

    if (!(state->flags&JSON_VALIDATE)) {
	push_string(finish_string_builder(&s));
	UNSET_ONERROR(handle);
    }

    return p;
}

#undef HEX2DEC
