#ifndef PDF_PARSE_H
#define PDF_PARSE_H

#include "svalue.h"
#include "stralloc.h"

struct parse_stack {
    unsigned INT32 top, size;
    struct svalue * sv;
    int * stack;
};

struct parse_context {
    struct parse_stack stack;
    ptrdiff_t mark, pos;
    struct string_builder b;
    /* keeps the name in dictionaries */
    struct pike_string * key;
    /* paranthesis count for literal strings */
    unsigned int pc;
    int state;
};

static INLINE void stack_init(struct parse_stack * s) {
    s->top = 0;
    s->size = 64;
    s->sv = (struct svalue*)xalloc(64 * sizeof(struct svalue));
    s->stack = (int*)xalloc(64 * sizeof(int));
}

static INLINE void prepush(struct parse_stack * s) {
    if (s->top == s->size) {
	s->size *= 2;
	s->sv = (struct svalue*)xrealloc(s->sv, s->size * sizeof(struct svalue));
	s->stack = (int*)xrealloc(s->stack, s->size * sizeof(int));
    }
}

static INLINE struct svalue * stack_top(struct parse_stack * s) {
    return s->sv + s->top;
}

static INLINE int hex2char(const unsigned char * s) {
    int c = * s;

    if (c >= 'A' && c <= 'F') {
	return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
	return 10 + c - 'a';
    } else {
	return c - '0';
    }
}

#endif
