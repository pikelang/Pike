/* vim:syntax=ragel
 */
#include <stdio.h>
#include "stralloc.h"
#include "global.h"
#include "svalue.h"
#include "interpret.h"
#include "parse.h"
#include "mapping.h"
#include "array.h"

//#define PDF_TRACE


%%{
    machine PDF;
    alphtype unsigned char;
    variable top c->stack.top;
    variable stack c->stack.stack;
    prepush {
	prepush(&c->stack);
    }
    
    action mark {
	mark = fpc;
#ifdef PDF_TRACE
	fprintf(stderr, "marking %p '%c'\n", fpc, fc);
#endif
    }

    action mark_next {
	mark = fpc + 1;
    }

    action return {
#ifdef PDF_TRACE
	fprintf(stderr, "returning SP[%d] from state %u at pos %p (%c)\n", c->stack.top, cs, fpc, fc);
#endif
	fret;
    }


    action string_append {
	if (fpc - mark > 0) {
	    string_builder_binary_strcat0(&c->b, mark, fpc - mark);
	}
    }

    whitespace = [\0\t\n\r \f];
    delimiter = [()<>\[\]{}\/%];
    regular = any - (whitespace|delimiter);

    stop = delimiter | whitespace;

    my_space = (whitespace | ( '%' . [^\n]* . '\n' ));
#my_space = whitespace*;

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
	    int ch = 0;
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
#ifdef PDF_TRACE
	fprintf(stderr, "start string\n");
#endif
	init_string_builder(&c->b, 0);
    }

    action finish_string {
	if (fpc - mark > 0) {
#ifdef PDF_TRACE
	    fprintf(stderr, "appending %ld characters from %p (%s)\n", fpc-mark, mark, mark);
#endif
	    string_builder_binary_strcat0(&c->b, mark, fpc - mark);
	}
#ifdef PDF_TRACE
	fprintf(stderr, "finishing string\n");
#endif
	SET_SVAL(SP[0], PIKE_T_STRING, 0, string, finish_string_builder(&c->b));
    }

    action paren_incr {
#ifdef PDF_TRACE
	fprintf(stderr, "increase paren at %c from %u\n", fc, c->pc);
#endif
	c->pc++;
    }

    action paren_decr {
#ifdef PDF_TRACE
	fprintf(stderr, "decrease paren at %c from %u\n", fc, c->pc);
#endif
	c->pc--;
    }

    action paren_test {
	c->pc
    }

    literal_parenthesis = '(' $paren_incr | ')' when paren_test $paren_decr;
    literal_chars = ((any - [()\\]) | literal_parenthesis )+;
    literal_quote = '\\' . ([0-7]{1,3} >mark %append_octal | [nrtbf()\\] >append_quoted );
    literal_string := (literal_chars >mark | literal_quote >string_append )**
			    >start_string %finish_string <: ')' @return;

    # hex string

    action finish_hex_string {
	init_string_builder_alloc(&c->b, (p - mark + 3)/2, 0);

	p--;
	while (mark < p) {
	    int ch = hex2char(mark++)<<8;
	    ch |= hex2char(mark++);
	    string_builder_putchar(&c->b, ch);
	}

	if (mark == p) {
	    string_builder_putchar(&c->b, hex2char(mark)<<8);
	}
	p++;
	SET_SVAL(SP[0], PIKE_T_STRING, 0, string, finish_string_builder(&c->b));
    }
    
    hex_string := xdigit* >mark . '>' @finish_hex_string @return;
    # integer

    action finish_integer {
#ifdef PDF_TRACE
	fprintf(stderr, "parsing integer in %ld characters from %p (%s)\n", fpc-mark, mark, mark);
#endif
	pcharp_to_svalue_inumber(SP, MKPCHARP(mark, 0), NULL, 10, fpc-mark);
    }

    integer = ([\-+]? . digit+) >mark %finish_integer;

    # reference

    action finish_reference {
	SET_SVAL(SP[0], PIKE_T_OBJECT, 0, object, create_reference(mark, fpc));	
    }

    reference = digit+ >mark . ' ' . digit+ . ' R' %finish_reference;
    
    # real

    action finish_real {
#ifdef PDF_TRACE
	fprintf(stderr, "parsing real in %ld characters from %p (%s)\n", fpc-mark, mark, mark);
#endif
	SET_SVAL(SP[0], PIKE_T_FLOAT, 0, float_number,
		 (FLOAT_TYPE)STRTOD_PCHARP(MKPCHARP(mark, 0), NULL));
    }

    real = ([\-+]? . (digit* . '.' . digit+)|(digit+ . '.' . digit*) ) >mark %finish_real;

    number := (reference | integer | real) <: any >{ fhold; } >return ;

    # name

    action start_name {
#ifdef PDF_TRACE
	fprintf(stderr, "start name\n");
#endif
	init_string_builder(&c->b, 0);
    }

    action add_name_enc {
	string_builder_putchar(&c->b, hex2char(fpc) | hex2char(fpc-1)<<4);
#ifdef PDF_TRACE
	fprintf(stderr, "%c %c\n", *(fpc-1), *fpc);
#endif
    }

    action finish_name {
	if (fpc - mark > 0) {
#ifdef PDF_TRACE
	    fprintf(stderr, "appending %ld characters from %p (%s)\n", fpc-mark, mark, mark);
#endif
	    string_builder_binary_strcat0(&c->b, mark, fpc - mark);
	}
#ifdef PDF_TRACE
	fprintf(stderr, "finishing name\n");
#endif
	SET_SVAL(SP[0], PIKE_T_STRING, 0, string, finish_string_builder(&c->b));
    }

    name_enc = '#' . xdigit{2};
    name_reg = (regular - '#')+;
    name := ( name_enc >string_append @add_name_enc | name_reg >mark )**
		  >start_name %finish_name %{ fhold; } %return <: any;

    action start_dict {
#ifdef PDF_TRACE
	fprintf(stderr, "starting dictionary\n");
#endif
	SET_SVAL(SP[0], PIKE_T_MAPPING, 0, mapping, allocate_mapping(16));
    }

    action finish_dict {
#ifdef PDF_TRACE
	fprintf(stderr, "finishing dict\n");
#endif
    }

    action store_key {
	{
	    struct pike_string *key = (SP+1)->u.string;
#ifdef PDF_TRACE
	    fprintf(stderr, "storing name: %s\n", key->str);
#endif
	}
	c->key = (SP+1)->u.string;
    }

    action call_dictionary {
#ifdef PDF_TRACE
	fprintf(stderr, "calling dictionary from SP[%d]\n", c->stack.top);
#endif
	fcall dictionary;
    }

    action call_array {
#ifdef PDF_TRACE
	fprintf(stderr, "calling array from SP[%d]\n", c->stack.top);
#endif
	fcall array;
    }

    action call_name {
#ifdef PDF_TRACE
	fprintf(stderr, "calling name\n");
#endif
	fcall name;
    }

    action call_lstring {
#ifdef PDF_TRACE
	fprintf(stderr, "calling literal_string\n");
#endif
	fcall literal_string;
    }

    action call_number {
	fhold;
#ifdef PDF_TRACE
	fprintf(stderr, "calling number\n");
#endif
	fcall number;
    }

    action call_hexstring {
	fhold;
#ifdef PDF_TRACE
	fprintf(stderr, "calling number\n");
#endif
	fcall hex_string;
    }
    
    action store_entry {
	{
	    struct svalue key;
	    SET_SVAL(key, PIKE_T_STRING, 0, string, c->key);
	    low_mapping_insert(SP[0].u.mapping, &key, SP+1, 2);
	    free_svalue(SP+1);
#ifdef PDF_TRACE
	    fprintf(stderr, "storing entry at '%c'\n", fc);
#endif
	}

    }

    action finish_null {
	SET_SVAL(SP[1], PIKE_T_OBJECT, 0, object, get_val_null());
    }

    action finish_true {
	SET_SVAL(SP[1], PIKE_T_OBJECT, 0, object, get_val_true());
    }

    action finish_false {
	SET_SVAL(SP[1], PIKE_T_OBJECT, 0, object, get_val_false());
    }

    object = '<<' @call_dictionary |
	     '<' . xdigit >call_hexstring |
	     '(' @call_lstring |
	     '/' @call_name |
	     '[' @call_array |
	     'null' @finish_null |
	     'false' @finish_false |
	     'true' @finish_true |
	     (digit|[+\-]) >call_number;

    dictionary := (
		   (my_space* . '/' @call_name %store_key . my_space* . object . my_space*) %store_entry
		  )* >start_dict
		  . '>>' @finish_dict <: any @return;

    action start_array {
#ifdef PDF_TRACE
	fprintf(stderr, "starting array\n");
#endif
	SET_SVAL(SP[0], PIKE_T_ARRAY, 0, array, low_allocate_array(0, 8));
    }

    action array_add {
	{
	    struct array * a = SP[0].u.array;
	    move_svalue(ITEM(a) + a->size++, SP+1);
	}
    }

    action finish_array {
#ifdef PDF_TRACE
	fprintf(stderr, "finishing array\n");
#endif
    }

    action create_object {
	SET_SVAL(SP[0], PIKE_T_OBJECT, 0, object, low_clone(pdf_object_program));
    }

    action add_objid {
	{
	    PCHARP end = MKPCHARP(fpc, 0);
	    long int id = STRTOL_PCHARP(MKPCHARP(mark, 0), &end, 10);
	    OBJ2_PDF_OBJECT(SP[0].u.object)->id = (INT_TYPE)id;
	}
    }

    action add_objrev {
	{
	    PCHARP end = MKPCHARP(fpc, 0);
	    long int rev = STRTOL_PCHARP(MKPCHARP(mark, 0), &end, 10);
	    OBJ2_PDF_OBJECT(SP[0].u.object)->rev = (INT_TYPE)rev;
	}
    }

    action add_objdata {
	{
	    move_svalue(&OBJ2_PDF_OBJECT(SP[0].u.object)->data, SP+1);
	}
    }

    array := ( my_space* . object %array_add . my_space*)* >start_array . ']' @finish_array @return;
    
    direct_object = digit+ >mark >create_object %add_objid . my_space+ .
		    digit+ >mark %add_objrev . my_space+ .
		    'obj' . my_space* . object . my_space* . 'endobj' @add_objdata;

    main := my_space* . direct_object @{ fbreak;};
}%%

#define SP (stack_top(&c->stack))

static void parse_context_init(struct parse_context * c) {
    stack_init(&c->stack);
    c->state = %%{ write start; }%% ;
    c->mark = 0;
    c->pos = 0;
    c->key = NULL;
    c->pc = 0;
}

static const int PDF_error_state = %%{ write error; }%%;

static int parse_object(struct parse_context * c, struct pike_string * s) {
    const unsigned char * mark = STR0(s) + c->mark;
    const unsigned char * p = STR0(s) + c->pos;
    const unsigned char * pe = STR0(s) + s->len;
    const unsigned char * eof = pe;

    int cs = c->state;

    %% write data;

#ifdef PDF_TRACE
    fprintf(stderr, "starting parsing at '%4s..' in state %d at SP[%u]\n", p, cs, c->stack.top);
#endif

    if (mark > p) {
	Pike_error("mark > p! \n");
    }
    if (p >= pe) {
	Pike_error("p >= pe! \n");
    }


    %% write exec;

#ifdef PDF_TRACE
    fprintf(stderr, "finishing parsing at '%4s..' in state %d at SP[%u]\n\n", p, cs, c->stack.top);
#endif
    
    c->mark = mark - STR0(s);
    c->pos = p - STR0(s);

    if (cs >= %%{ write first_final; }%% && !c->stack.top) {
	c->state = %%{ write start; }%% ;
	return 1;
    } else {
	c->state = cs;
	return 0;
    }
}
