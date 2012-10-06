/* vim:syntax=ragel */
#define HEX2DEC(x) ((x) <= '9' ? (x) - '0' : ((x) < 'G') ? (x) - 'A' + 10 : (x) - 'a' + 10)

%%{
    machine JSON_string;
    alphtype unsigned char;
    include JSOND "json_defaults.rl";

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
		goto failure;
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
	    goto failure;
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
		string_builder_binary_strcat(&s, (char *)mark, (ptrdiff_t)(fpc - mark));
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
		       'u' . xdigit >hex0beg . (xdigit{3} $hex0mid) @hex0end -> start
		   ) @mark_next,
		   hex1: (
		       '\\u' . xdigit >hex1beg . (xdigit{3} $hex1mid) @hex1end -> start
		   ) @mark_next
		  ) >mark %*{ fpc--; fbreak; };
}%%

static ptrdiff_t _parse_JSON_string_utf8(PCHARP str, ptrdiff_t pos, ptrdiff_t end, struct parser_state *state) {
    unsigned char *p = (unsigned char*)(str.ptr) + pos;
    unsigned char *pe = (unsigned char*)(str.ptr) + end;
    ptrdiff_t start = pos;
    unsigned char *mark = 0;
    struct string_builder s;
    int cs;
    ONERROR handle;
    int hexchr0, hexchr1;
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

	return p - (unsigned char*)(str.ptr);
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
    return p - (unsigned char*)(str.ptr);
}

#undef HEX2DEC
