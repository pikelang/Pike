// vim:syntax=ragel
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)

%%{
    machine JSON_string;
    alphtype char;
    include JSOND "json_defaults.rl";

    action hex0 {
	temp = HEX2DEC(fc);
    }

    action hex1 {
	temp *= 16;
	temp += HEX2DEC(fc);
    }

    action hex2 {
	if (IS_NUNICODE(temp)) {
		goto failure;	
	}
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
		string_builder_binary_strcat(&s, mark, (ptrdiff_t)(fpc - mark));
        }
    }

    action utf_2_1 { unicode = ((p_wchar2)(fc & (0xdf-0xc0))) << 6; }
    action utf_2_2 { unicode |= (p_wchar2)(fc & (0xbf-0x80)); }

    action utf_3_1 { unicode = ((p_wchar2)(fc & 0x0f)) << 12; }
    action utf_3_2 { unicode |= ((p_wchar2)(fc & (0xbf-0x80))) << 6; }
    action utf_3_3 { 
	unicode |= (p_wchar2)(fc & (0xbf-0x80));
	if ((unicode < 0x0800 || unicode > 0xd7ff) && (unicode < 0xe000 || unicode > 0xffff)) {
	    goto failure;	
	}
    }

    action utf_4_1 { unicode = ((p_wchar2)(fc & 0x07)) << 18; }
    action utf_4_2 { unicode |= ((p_wchar2)(fc & (0xbf-0x80))) << 12; }

#action utf_4_3 { unicode |= ((p_wchar2)(fc & (0xbf-0x80))) << 6; }
    action utf_4_4 { 
	unicode |= (p_wchar2)(fc & (0xbf-0x80));
	if (unicode < 0x010000 || unicode > 0x10ffff) {
	    goto failure;
	}
    }

    action finish {
	if (!(state->flags&JSON_VALIDATE)) { 
	    string_builder_putchar(&s, unicode); 
	}
    }

    main := '"' . (
		   start: (
		       '"' >string_append -> final |
		       '\\' >string_append -> unquote |
			(0x20..0x7f - (0x00..0x1f | '"' | '\\')) -> start |
		       0xc2..0xdf >string_append >utf_2_1 . 0x80..0xbf >utf_2_2 >finish @mark_next -> start |
		       0xe0..0xef >string_append >utf_3_1 . 0x80..0xbf >utf_3_2 . 0x80..0xbf >utf_3_3 >finish @mark_next -> start |
		       0xf0..0xf4 >string_append >utf_4_1 . 0x80..0xbf >utf_4_2 . 0x80..0xbf >utf_3_2 . 0x80..0xbf >utf_4_4 >finish @mark_next -> start
		   ),
		   unquote: (
		       ["\\/bfnrt] >add_unquote -> start |
		       'u' . xdigit >hex0 . (xdigit{3} $hex1) @hex2 -> start
		   ) @mark_next
		  ) >mark %*{ fpc--; fbreak; };
}%%

static ptrdiff_t _parse_JSON_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
    char *p = (char*)(str.ptr) + pos;
    char *pe = (char*)(str.ptr) + end;
    ptrdiff_t start = pos;
    char *mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;
    p_wchar2 temp = 0;
    p_wchar2 unicode = 0;

    %% write data;

    if (!(state->flags&JSON_VALIDATE)) {
	init_string_builder(&s, 0);
	SET_ONERROR(handle, free_string_builder, &s);
    }

    %% write init;
    %% write exec;

    if (cs >= JSON_string_first_final) {
	if (!(state->flags&JSON_VALIDATE)) {
	    push_string(finish_string_builder(&s));
	    UNSET_ONERROR(handle);
	}

	return p - (char*)(str.ptr);
    }

failure:

    if (!(state->flags&JSON_VALIDATE)) {
	UNSET_ONERROR(handle);
	free_string_builder(&s);
    }

    state->flags |= JSON_ERROR;
    if (p == pe) {
	err_msg = "Unterminated string";
	return start;
    }
    return p - (char*)(str.ptr);
}

#undef HEX2DEC
