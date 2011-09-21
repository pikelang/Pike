/* vim:syntax=ragel */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)

%%{
    machine JSON_string;
    alphtype int;
    include JSOND "json_defaults.rl";
    getkey ((int)INDEX_PCHARP(str, fpc));

    action hex0beg {
	hexchr0 = HEX2DEC(fc);
    }

    action hex0mid {
	hexchr0 *= 16;
	hexchr0 += HEX2DEC(fc);
    }

    action hex0end {
	if (IS_HIGH_SURROGATE (hexchr0)) {
	    /* Chars outside the BMP can be expressed as two hex
	     * escapes that codes a surrogate pair, so see if we can
	     * read a second one. */
	    fnext hex1;
	}
	else {
	    if (IS_NUNICODE(hexchr0)) {
		fpc--; fbreak;
	    }
	    if (!(state->flags&JSON_VALIDATE)) {
		string_builder_putchar(&s, hexchr0);
	    }
	}
    }

    action hex1beg {
	hexchr1 = HEX2DEC(fc);
    }

    action hex1mid {
	hexchr1 *= 16;
	hexchr1 += HEX2DEC(fc);
    }

    action hex1end {
	if (!IS_LOW_SURROGATE (hexchr1)) {
	    fpc--; fbreak;
	}
	if (!(state->flags&JSON_VALIDATE)) {
	    int cp = (((hexchr0 - 0xd800) << 10) | (hexchr1 - 0xdc00)) +
		0x10000;
	    string_builder_putchar(&s, cp);
	}
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
		       'u' . xdigit >hex0beg . (xdigit{3} $hex0mid) @hex0end -> start
		   ) @mark_next,
		   hex1: (
		       '\\u' . xdigit >hex1beg . (xdigit{3} $hex1mid) @hex1end -> start
		   ) @mark_next
		  ) >mark %*{ fpc--; fbreak; };
}%%

static ptrdiff_t _parse_JSON_string(PCHARP str, ptrdiff_t p, ptrdiff_t pe, struct parser_state *state) {
    int hexchr0, hexchr1;
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
