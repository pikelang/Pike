/* vim:syntax=c
 */
#include <stdio.h>
#include "stralloc.h"
#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "parse.h"

%%{
    machine PDF;
    alphtype unsigned char;
    variable top c->stack.top;
    variable stack c->stack.stack;
    prepush {
	prepush(&c->stack);
    }
#getkey ((int)INDEX_PCHARP(str, fpc))
    
    action mark {
	mark = fpc;
	fprintf(stderr, "marking %p '%c'\n", fpc, fc);
    }

    action mark_next {
	mark = fpc + 1;
    }

    action string_append {
	if (fpc - mark > 0) {
	    string_builder_binary_strcat0(&c->b, mark, fpc - mark);
	}
    }

    action save_state {
	c->pos = STR0(s) - fpc;
	if (mark) c->mark = STR0(s) - mark;
	c->state = fcurs;
    }

    whitespace = [\0\t\n\r \f];
    delimiter = [()<>\[\]{}\/%];
    regular = any - (whitespace|delimiter);

    stop = delimiter | whitespace;

    my_space = (whitespace | ( '%' . [^\n]* . '\n' ));

    true = 'true';
    false = 'false';

    action append_octal {
	do {
	    int ch = 0;
	    while (mark < fpc) {
		ch <<= 3;
		ch |= *mark - '0';
		mark ++;
	    }
	    /* TODO: limit to 0-255 */
	    string_builder_putchar(&c->b, ch);
	} while (0);
    }

    action append_quoted {
	do {
	    int ch;
	    switch (fc) {
	    case 'n':
		ch = '\n';
		break;
	    case 'r':
		ch = '\r';
		break;
	    case 't':
		ch = '\t';
		break;
	    case 'b':
		ch = '\b';
		break;
	    case 'f':
		ch = '\f';
		break;
	    case '(':
	    case ')':
	    case '\\':
		ch = fc;
		break;
	    }
	    string_builder_putchar(&c->b, ch);
	} while (0);
    }

    action start_string {
	fprintf(stderr, "start string\n");
	init_string_builder(&c->b, 0);
    }

    action finish_string {
	if (fpc - mark > 0) {
	    fprintf(stderr, "appending %ld characters from %p (%s)\n", fpc-mark, mark, mark);
	    string_builder_binary_strcat0(&c->b, mark, fpc - mark);
	}
	fprintf(stderr, "finishing string\n");
	SET_SVAL(*stack_top(&c->stack), PIKE_T_STRING, 0, string, finish_string_builder(&c->b));
    }

    action paren_incr {
	fprintf(stderr, "increase paren at %c from %u\n", fc, c->pc);
	c->pc++;
    }

    action paren_decr {
	fprintf(stderr, "decrease paren at %c from %u\n", fc, c->pc);
	c->pc--;
    }

    action paren_test {
	c->pc
    }

    literal_parenthesis = '(' $paren_incr | ')' when paren_test $paren_decr;
    literal_chars = ((any - [()\\]) | literal_parenthesis )+;
    literal_quote = '\\' . ([0-7]{1,3} >mark %append_octal | [nrtbf()\\] >append_quoted );
    literal_string = '(' >start_string . (
				    (literal_chars >mark | literal_quote >string_append )**
		         ) %finish_string <: ')';

    # integer

    action finish_integer {
	pcharp_to_svalue_inumber(stack_top(&c->stack), MKPCHARP(mark, 0), NULL, 10, fpc-mark);
	fprintf(stderr, "parsing integer in %ld characters from %p (%s)\n", fpc-mark, mark, mark);
    }

    integer = ([\-+]? . digit+) >mark %finish_integer;

    # reference

    action finish_reference {
	SET_SVAL(*stack_top(&c->stack), PIKE_T_OBJECT, 0, object, create_reference(mark, fpc));	
    }

    reference = digit+ >mark . ' ' . digit+ . ' R' %finish_reference;
    
    # real

    action finish_real {
	SET_SVAL(*stack_top(&c->stack), PIKE_T_FLOAT, 0, float_number,
		 (FLOAT_TYPE)STRTOD_PCHARP(MKPCHARP(mark, 0), NULL));

	fprintf(stderr, "parsing real in %ld characters from %p (%s)\n", fpc-mark, mark, mark);
    }

    real = ([\-+]? . (digit* . '.' . digit+)|(digit+ . '.' . digit*) ) >mark %finish_real;

    # name

    action start_name {
	fprintf(stderr, "start name\n");
	init_string_builder(&c->b, 0);
    }

    action add_name_enc {
	string_builder_putchar(&c->b, hex2char(fpc) | hex2char(fpc-1)<<4);
	fprintf(stderr, "%c %c\n", *(fpc-1), *fpc);
    }

    action finish_name {
	if (fpc - mark > 0) {
	    fprintf(stderr, "appending %ld characters from %p (%s)\n", fpc-mark, mark, mark);
	    string_builder_binary_strcat0(&c->b, mark, fpc - mark);
	}
	fprintf(stderr, "finishing name\n");
	SET_SVAL(*stack_top(&c->stack), PIKE_T_STRING, 0, string, finish_string_builder(&c->b));
    }

    name_enc = '#' . xdigit{2};
    name_reg = (regular - '#')+;
    name = '/' >start_name . ( name_enc >string_append @add_name_enc | name_reg >mark )** %finish_name;

    action ret_object {
	fprintf(stderr, "returning %d at %p\n", cs, fpc);
    }

    main := my_space* . (literal_string | name | integer | real | reference) %ret_object . my_space*;

}%%

static int parse_object(struct parse_context * c, struct pike_string * s) {
    const unsigned char * mark = NULL;
    const unsigned char * p = STR0(s) + c->pos;
    const unsigned char * pe = STR0(s) + s->len;
    const unsigned char * eof = pe;
    int cs;

    %% write data;

    if (c->mark != -1) {
	mark = STR0(s) + c->mark;
    }

    cs = %%{ write start; }%% ;

    %% write init nocs;

    %% write exec;

    fprintf(stderr, "cs: %d\n", cs);
    fprintf(stderr, "at %p pe: %p\n", p, pe);
    fprintf(stderr, "at: '[..%c]%s'\n", p[-1], p);

    if (cs >= %%{ write first_final; }%% ) {
	return 0;
    } else {
	return 1;
    }
}
