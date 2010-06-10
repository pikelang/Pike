// vim:syntax=ragel
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)

%%{
    machine JSON_string;
    alphtype int;
    include JSOND "json_defaults.rl";

    action hex0 {
		if (!state->validate) temp = HEX2DEC(fc);
    }

    action hex1 {
		if (!state->validate) {
			temp *= 16;
			temp += HEX2DEC(fc);

			if (IS_NUNICODE(temp)) {
				fpc--; fbreak;
			}
		}
    }

    action hex2 {
		if (!state->validate) string_builder_putchar(&s, temp);
    }

    action add_unquote {
		if (!state->validate) switch(fc) {
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
            //string_builder_binary_strcat(s, mark, (ptrdiff_t)(fpc - mark));

			// looking for the lowest possible magnitude here may be worth it. i am not entirely
			// sure if i want to do the copying here.
			// use string_builder_binary_strcat here
			if (!state->validate)
				string_builder_append(&s, MKPCHARP(mark, 2), (ptrdiff_t)(fpc - mark));
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

static p_wchar2 *_parse_JSON_string(p_wchar2 *p, p_wchar2 *pe, struct parser_state *state) {
    p_wchar2 temp = 0;
    p_wchar2 *mark = 0;
    struct string_builder s;
    int cs;

    %% write data;

    if (!state->validate)
		init_string_builder(&s, 0);

    %% write init;
    %% write exec;

    if (cs < JSON_string_first_final) {
		if (!state->validate) {
			free_string_builder(&s);
		}

		push_int((INT_TYPE)p);
		return NULL;
    }

    if (!state->validate)
		push_string(finish_string_builder(&s));

    return p;
}

#undef HEX2DEC
