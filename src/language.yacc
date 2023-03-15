/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

%pure-parser

/* Expect a single shift/reduce conflict (dangling else). */
%expect 2

%token TOK_ARROW "->"

/*
 * Basic value pushing
 */
%token TOK_CONSTANT "constant"
%token TOK_FLOAT "float literal"
%token TOK_STRING "string literal"
%token TOK_NUMBER "integer literal"

/*
 * These are the predefined functions that can be accessed from Pike.
 */

%token TOK_INC "++"
%token TOK_DEC "--"
%token TOK_RETURN "return"

%token TOK_EQ "=="
%token TOK_GE ">="
%token TOK_LE "<="
%token TOK_NE "!="
%token TOK_NOT "!"
%token TOK_LSH "<<"
%token TOK_RSH ">>"
%token TOK_LAND "&&"
%token TOK_LOR "||"

%token TOK_SWITCH "switch"
%token TOK_SSCANF "sscanf"
%token TOK_CATCH "catch"
%token TOK_FOREACH "foreach"

/* This is the end of file marker used by the lexer
 * to enable nicer EOF in error handling.
 */
%token TOK_LEX_EOF "end of file"

%token TOK_ADD_EQ "+="
%token TOK_AND_EQ "&="
%token TOK_ARRAY_ID "array"
%token TOK_ATTRIBUTE_ID "__attribute__"
%token TOK_BREAK "break"
%token TOK_CASE "case"
%token TOK_CLASS "class"
%token TOK_COLON_COLON "::"
%token TOK_CONTINUE "continue"
%token TOK_DEFAULT "default"
%token TOK_DEPRECATED_ID "__deprecated__"
%token TOK_DIV_EQ "/="
%token TOK_DO "do"
%token TOK_DOT_DOT ".."
%token TOK_DOT_DOT_DOT "..."
%token TOK_ELSE "else"
%token TOK_ENUM "enum"
%token TOK_EXTERN "extern"
%token TOK_FLOAT_ID "float"
%token TOK_FOR "for"
%token TOK_FUNCTION_ID "function"
%token TOK_GAUGE "gauge"
%token TOK_GLOBAL "global"
%token TOK_IDENTIFIER "identifier"
%token TOK_RESERVED "reserved identifier"
%token TOK_IF "if"
%token TOK_IMPORT "import"
%token TOK_INHERIT "inherit"
%token TOK_INLINE "inline"
%token TOK_LOCAL_ID "local"
%token TOK_FINAL_ID "final"
%token TOK_FUNCTION_NAME "__func__"
%token TOK_INT_ID "int"
%token TOK_LAMBDA "lambda"
%token TOK_MULTISET_ID "multiset"
%token TOK_MULTISET_END ">)"
%token TOK_MULTISET_START "(<"
%token TOK_LSH_EQ "<<="
%token TOK_MAPPING_ID "mapping"
%token TOK_MIXED_ID "mixed"
%token TOK_MOD_EQ "%="
%token TOK_MULT_EQ "*="
%token TOK_OBJECT_ID "object"
%token TOK_OR_EQ "|="
%token TOK_POW "**"
%token TOK_POW_EQ "**="
%token TOK_PRIVATE "private"
%token TOK_PROGRAM_ID "program"
%token TOK_PROTECTED "protected"
%token TOK_PREDEF "predef"
%token TOK_PUBLIC "public"
%token TOK_RSH_EQ ">>="
%token TOK_STATIC "static"
%token TOK_STATIC_ASSERT "_Static_assert"
%token TOK_STRING_ID "string"
%token TOK_SUB_EQ "-="
%token TOK_TYPEDEF "typedef"
%token TOK_TYPEOF "typeof"
%token TOK_UNKNOWN "__unknown__"
%token TOK_UNUSED "__unused__"
%token TOK_VARIANT "variant"
%token TOK_VERSION "version prefix"
%token TOK_VOID_ID "void"
%token TOK_WEAK "__weak__"
%token TOK_WHILE "while"
%token TOK_XOR_EQ "^="
%token TOK_OPTIONAL "optional"
%token TOK_SAFE_INDEX "->?"
%token TOK_SAFE_START_INDEX "[?"
%token TOK_SAFE_APPLY "(?"
%token TOK_BITS "bits"
%token TOK_AUTO_ID "auto"
%token TOK_ATOMIC_GET_SET "?="


%right '='
%right '?'

%left TOK_LOR
%left TOK_LAND
%left '|'
%left '^'
%left '&'
%left TOK_EQ TOK_NE
%left '>' TOK_GE '<' TOK_LE  /* nonassoc? */
%left TOK_LSH TOK_RSH
%left '+' '-'
%left '*' '%' '/'
%right TOK_NOT '~'
%right TOK_POW
%nonassoc TOK_INC TOK_DEC

%{
/* This is the grammar definition of Pike. */

#include "global.h"
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include "interpret.h"
#include "array.h"
#include "object.h"
#include "mapping.h"
#include "stralloc.h"
#include "las.h"
#include "interpret.h"
#include "program.h"
#include "pike_types.h"
#include "constants.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "docode.h"
#include "pike_embed.h"
#include "opcodes.h"
#include "operators.h"
#include "builtin_functions.h"
#include "bignum.h"

#define YYMAXDEPTH	1000

#ifdef PIKE_DEBUG
#ifndef YYDEBUG
/* May also be defined by machine.h */
#define YYDEBUG 1
#endif /* YYDEBUG */
#endif

/* Get verbose parse error reporting. */
#define YYERROR_VERBOSE	1

/* #define LAMBDA_DEBUG	1 */

/*
 * Prototypes for static functions.
 * NB: Other functions have their prototypes in las.h.
 */
static void yyerror_reserved(const char *keyword);
static void redefine_local(struct compiler_frame *frame, int var,
                           node *def, node *init);
static void mark_lvalue_as_used(node *n);
static void mark_lvalues_as_used(node *n);
static node *lexical_islocal(struct pike_string *);
static node *safe_inc_enum(node *n);
static node *find_versioned_identifier(struct pike_string *identifier,
				       int major, int minor);
static node *set_default_value(int e);
static int call_handle_import(void);
static void update_current_type(void);

static int inherit_depth;
static struct program_state *inherit_state = NULL;

/*
 * Kludge for Bison not using prototypes.
 */
#ifndef __GNUC__
#ifndef __cplusplus
static void __yy_memcpy(char *to, YY_FROM_CONST char *from,
			YY_COUNT_TYPE count);
#endif /* !__cplusplus */
#endif /* !__GNUC__ */

%}

%union
{
  int number;
  FLOAT_TYPE fnum;
  struct node_s *n;
  char *str;
  void *ptr;
}

%{
/* Need to be included after YYSTYPE is defined. */
#define INCLUDED_FROM_LANGUAGE_YACC
#include "lex.h"
#include "pike_compiler.h"
%}

%{
/* Include <stdio.h> our selves, so that we can do our magic
 * without being disturbed... */
#include <stdio.h>
int yylex(YYSTYPE *yylval);
/* Bison is stupid, and tries to optimize for space... */
#ifdef YYBISON
#define short int
#endif /* YYBISON */

%}

%type <fnum> TOK_FLOAT

%type <number> TOK_ARRAY_ID
%type <number> TOK_BREAK
%type <number> TOK_CASE
%type <number> TOK_CATCH
%type <number> TOK_CONTINUE
%type <number> TOK_DEFAULT
%type <number> TOK_DO
%type <number> TOK_ELSE
%type <number> TOK_FLOAT_ID
%type <number> TOK_FOR
%type <number> TOK_FOREACH
%type <number> TOK_FUNCTION_ID
%type <number> TOK_GAUGE
%type <number> TOK_IF
%type <number> TOK_INHERIT
%type <number> TOK_INLINE
%type <number> TOK_INT_ID
%type <number> TOK_LAMBDA
%type <number> TOK_LOCAL_ID
%type <number> TOK_MAPPING_ID
%type <number> TOK_MIXED_ID
%type <number> TOK_MULTISET_ID
%type <number> TOK_OBJECT_ID
%type <number> TOK_PREDEF
%type <number> TOK_PRIVATE
%type <number> TOK_PROGRAM_ID
%type <number> TOK_PROTECTED
%type <number> TOK_PUBLIC
%type <number> TOK_RETURN
%type <number> TOK_SSCANF
%type <number> TOK_STATIC
%type <number> TOK_STRING_ID
%type <number> TOK_AUTO_ID
%type <number> TOK_SWITCH
%type <number> TOK_VOID_ID
%type <number> TOK_WHILE
%type <number> arguments
%type <number> arguments2
%type <number> func_args
%type <number> create_arguments
%type <number> create_arguments2
%type <number> create_arg
%type <number> assign
%type <number> low_assign
%type <number> modifier
%type <number> modifier_list
%type <number> modifiers
%type <number> implicit_modifiers
%type <number> inherit_specifier
%type <number> function_type_list
%type <number> function_type_list2
%type <number> optional_dot_dot_dot
%type <number> optional_comma
%type <number> optional_constant
%type <number> optional_continue
%type <number> optional_create_arguments
%type <number> save_block_level
%type <number> save_locals

%type <str> magic_identifiers
%type <str> magic_identifiers1
%type <str> magic_identifiers2
%type <str> magic_identifiers3

/* The following symbols return type information */

%type <n> number
%type <n> number_or_minint
%type <n> number_or_maxint
%type <n> cast
%type <n> soft_cast
%type <n> real_string_constant
%type <n> real_string_or_identifier
%type <n> string_constant
%type <n> string_or_identifier
%type <n> string_segment
%type <n> string
%type <n> TOK_STRING
%type <n> TOK_NUMBER
%type <n> TOK_BITS
%type <n> optional_default_value
%type <n> optional_rename_inherit
%type <n> optional_identifier
%type <n> implicit_identifier
%type <n> TOK_IDENTIFIER
%type <n> TOK_RESERVED
%type <n> TOK_VERSION
%type <n> annotation
%type <n> annotation_list
%type <n> attribute
%type <n> assoc_pair
%type <n> line_number_info
%type <n> block
%type <n> optional_block
%type <n> failsafe_block
%type <n> safe_apply_with_line_info
%type <n> open_paren_with_line_info
%type <n> open_paren_or_safe_apply_with_line_info
%type <n> open_bracket_with_line_info
%type <n> block_or_semi
%type <n> break
%type <n> case
%type <n> catch
%type <n> catch_arg
%type <n> anon_class
%type <n> named_class
%type <n> enum
%type <n> enum_value
%type <n> range_bound
%type <n> cond
%type <n> continue
%type <n> default
%type <n> do
%type <n> constant_expr
%type <n> safe_assignment_expr
%type <n> splice_expr

%type <n> literal_expr
%type <n> unqualified_id_expr
%type <n> qualified_ident
%type <n> qualified_id_expr
%type <n> id_expr
%type <n> primary_expr
%type <n> postfix_expr
%type <n> pow_expr
%type <n> unary_expr
%type <n> cast_expr
%type <n> mul_expr
%type <n> add_expr
%type <n> shift_expr
%type <n> rel_expr
%type <n> eq_expr
%type <n> and_expr
%type <n> xor_expr
%type <n> or_expr
%type <n> land_expr
%type <n> lor_expr
%type <n> cond_expr
%type <n> assignment_expr
%type <n> comma_expr
%type <n> init_expr
%type <n> safe_init_expr
%type <n> optional_init_expr
%type <n> void_expr
%type <n> optional_void_expr

%type <n> apply
%type <n> expr_list
%type <n> expr_list2
%type <n> for
%type <n> for_expr
%type <n> foreach
%type <n> gauge
%type <n> labeled_statement
%type <n> lambda
%type <n> local_name_list
%type <n> low_id_expr
%type <n> safe_lvalue
%type <n> lvalue
%type <n> lvalue_list
%type <n> low_lvalue_list
%type <n> m_expr_list
%type <n> m_expr_list2
%type <n> new_local_name
%type <n> normal_label_statement
%type <n> optional_else_part
%type <n> optional_label
%type <n> propagated_enum_value
%type <n> return
%type <n> sscanf
%type <n> statement
%type <n> statements
%type <n> statement_with_semicolon
%type <n> switch
%type <n> typeof
%type <n> while
%type <n> low_program_ref
%type <n> inherit_ref
%type <n> local_function
%type <n> local_generator
%type <n> magic_identifier
%type <n> simple_identifier
%type <n> foreach_lvalues
%type <n> foreach_optional_lvalue

%type <ptr> start_function
%type <ptr> start_lambda
%%

all: program { YYACCEPT; }
  | program TOK_LEX_EOF { YYACCEPT; }
/*  | error TOK_LEX_EOF { YYABORT; } */
  ;

program: program def
  | program ';'
  |  /* empty */
  ;

real_string_or_identifier: TOK_IDENTIFIER
  | real_string_constant
  ;

optional_rename_inherit: ':' real_string_or_identifier { $$=$2; }
  | ':' bad_identifier { $$=0; }
  | ':' error { $$=0; }
  | { $$=0; }
  ;

/* NOTE: This rule pushes a string "name" on the stack in addition
 * to resolving the program reference.
 */
low_program_ref: safe_assignment_expr
  {
    node *n = $1;

    STACK_LEVEL_START(0);

    while (n) {
      switch (n->token) {
      case F_EXTERNAL:
      case F_GET_SET:
	$$ = n;
	add_ref(n);
	free_node($1);
	goto got_program_ref;
      case F_APPLY:
	{
	  if ((CAR(n)->token == F_CONSTANT) &&
	      (TYPEOF(CAR(n)->u.sval) == T_FUNCTION) &&
	      (SUBTYPEOF(CAR(n)->u.sval) == FUNCTION_BUILTIN) &&
	      (CAR(n)->u.sval.u.efun->function == debug_f_aggregate)) {
	    /* Disambiguate multiple inherit ::-reference. */
	    node *arg;
	    while(1) {
	      while ((arg = CDR(n))) {
		n = arg;
		if (n->token != F_ARG_LIST) goto found_program_ref;
	      }
	      /* Paranoia. */
	      if ((arg = CAR(n))) {
		n = arg;
		continue;
	      }
	      /* FIXME: Ought to go up a level and try the car there...
	       *        But as this code probably won't be reached, we
	       *        just fail.
	       */
	      yyerror("Failed to get last argument from empty array.");
	      n = NULL;
	      break;
	    }
	  found_program_ref:
	    /* NB: The traditional C grammar requires a statement
	     *     after a label.
	     */
	    continue;
	  }
	}
	/* FALLTHRU */
      default:
	/* Evaluate the expression. */
	break;
      }
      break;
    }

    resolv_constant(n);
    free_node($1);

    if (TYPEOF(Pike_sp[-1]) == T_STRING) {
      if (call_handle_inherit(Pike_sp[-1].u.string)) {
	STACK_LEVEL_CHECK(2);
	$$ = mksvaluenode(Pike_sp-1);
	pop_stack();
      }
      else
	$$ = mknewintnode(0);
      STACK_LEVEL_CHECK(1);
      if($$->name) free_string($$->name);
#ifdef PIKE_DEBUG
      if (TYPEOF(Pike_sp[-1]) != T_STRING) {
	Pike_fatal("Compiler lost track of program name.\n");
      }
#endif /* PIKE_DEBUG */

      add_ref( $$->name=Pike_sp[-1].u.string );
    } else {
      $$ = mksvaluenode(Pike_sp-1);
      pop_stack();

    got_program_ref:
      STACK_LEVEL_CHECK(0);
      if (Pike_compiler->last_identifier) {
	ref_push_string(Pike_compiler->last_identifier);
      } else {
	push_empty_string();
      }
    }

    STACK_LEVEL_DONE(1);
  }
  ;

inherit_ref:
  {
    SET_FORCE_RESOLVE($<number>$);
  }
  low_program_ref
  {
    UNSET_FORCE_RESOLVE($<number>1);
    $$ = $2;
  }
  ;

inheritance: modifiers TOK_INHERIT inherit_ref optional_rename_inherit ';'
  {
    if (($1 & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)) {
      yywarning("Extern declared inherit.");
    }
    if($3)
    {
      struct pike_string *s=Pike_sp[-1].u.string;
      if($4) s=$4->u.sval.u.string;
      compiler_do_inherit($3,$1,s);
    }
    if($4) free_node($4);
    pop_stack();
    if ($3) free_node($3);
  }
  | modifiers TOK_INHERIT inherit_ref error ';'
  {
    if ($3) free_node($3);
    pop_stack();
    yyerrok;
  }
  | modifiers TOK_INHERIT inherit_ref error TOK_LEX_EOF
  {
    if ($3) free_node($3);
    pop_stack();
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | modifiers TOK_INHERIT inherit_ref error '}'
  {
    if ($3) free_node($3);
    pop_stack();
    yyerror("Missing ';'.");
  }
  ;

import: TOK_IMPORT constant_expr ';'
  {
    resolv_constant($2);
    free_node($2);
    if (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING || call_handle_import()) {
      use_module(Pike_sp-1);
      pop_stack();
    }
  }
  ;

constant_name: TOK_IDENTIFIER '=' safe_assignment_expr
  {
    /* This can be made more lenient in the future */

    /* Ugly hack to make sure that $3 is optimized */
    {
      int tmp=Pike_compiler->compiler_pass;
      $3=mknode(F_COMMA_EXPR,$3,0);
      Pike_compiler->compiler_pass=tmp;
    }

    if (Pike_compiler->current_modifiers & ID_EXTERN) {
      int depth = 0;
      struct program_state *state = Pike_compiler;
      node *n = $3;
      while (((n->token == F_COMMA_EXPR) || (n->token == F_ARG_LIST)) &&
	     ((!CAR(n)) ^ (!CDR(n)))) {
	if (CAR(n)) n = CAR(n);
	else n = CDR(n);
      }
      if (n->token == F_EXTERNAL) {
	while (state && (state->new_program->id != n->u.integer.a)) {
	  depth++;
	  state = state->previous;
	}
      }
      if (depth && state) {
	/* Alias for a symbol in a surrounding scope. */
	int id = really_low_reference_inherited_identifier(state, 0,
							   n->u.integer.b);
	define_alias($1->u.sval.u.string, n->type,
		     Pike_compiler->current_modifiers & ~ID_EXTERN,
		     depth, id);
      } else if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
	yyerror("Invalid extern declared constant.");
	add_constant($1->u.sval.u.string, &svalue_undefined,
		     Pike_compiler->current_modifiers & ~ID_EXTERN);
      }
    } else {
      if(!is_const($3)) {
	if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
	  yyerror("Constant definition is not constant.");
	}
	add_constant($1->u.sval.u.string, 0,
		     Pike_compiler->current_modifiers & ~ID_EXTERN);
      } else {
	if(!Pike_compiler->num_parse_error)
	{
	  ptrdiff_t tmp=eval_low($3,1);
	  if(tmp < 1)
	  {
	    yyerror("Error in constant definition.");
	    push_undefined();
	  }else{
            pop_n_elems((INT32)(tmp - 1));
	  }
	} else {
	  push_undefined();
	}
	add_constant($1->u.sval.u.string, Pike_sp-1,
		     Pike_compiler->current_modifiers & ~ID_EXTERN);
	pop_stack();
      }
    }
  const_def_ok:
    if($3) free_node($3);
    free_node($1);
  }
  | bad_identifier '=' safe_assignment_expr { if ($3) free_node($3); }
  | error '=' safe_assignment_expr { if ($3) free_node($3); }
  ;

constant_list: constant_name
  | constant_list ',' constant_name
  ;

constant: modifiers TOK_CONSTANT constant_list ';' {}
  | modifiers TOK_CONSTANT error ';' { yyerrok; }
  | modifiers TOK_CONSTANT error TOK_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | modifiers TOK_CONSTANT error '}' { yyerror("Missing ';'."); }
  ;

block_or_semi: block
  {
    $$ = mknode(F_COMMA_EXPR,$1,mknode(F_RETURN,mkintnode(0),0));
    COPY_LINE_NUMBER_INFO($$, $1);
  }
  | ';' { $$ = NULL; }
  | TOK_LEX_EOF { yyerror("Expected ';'."); $$ = NULL; }
  | error { $$ = NULL; }
  ;


open_paren_with_line_info: '('
  {
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
  ;

safe_apply_with_line_info: TOK_SAFE_APPLY
  {
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
  ;

open_paren_or_safe_apply_with_line_info: open_paren_with_line_info
  | safe_apply_with_line_info
  ;

close_brace_or_missing: '}'
  | /* empty */
  {
    yyerror("Missing '}'.");
  }
  ;

close_brace_or_eof: '}'
  | TOK_LEX_EOF
  {
    yyerror("Missing '}'.");
  }
  ;

open_bracket_with_line_info: '['
  {
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
  ;

close_bracket_or_missing: ']'
  | /* empty */
  {
    yyerror("Missing ']'.");
  }
  ;

/* This variant is used to start a new compiler frame for functions
 * with a declared return type.
 *
 * On entry the return type to be is taken from
 *
 *   compiler_frame->current_type
 *
 * in the current frame (stored there eg via simple_type). This is
 * then copied to
 *
 *   compiler_frame->current_return_type
 *
 * for the new frame.
 */
start_function: /* empty */
  {
    push_compiler_frame(SCOPE_LOCAL);

    if(!Pike_compiler->compiler_frame->previous ||
       !Pike_compiler->compiler_frame->previous->current_type)
    {
      yyerror("Internal compiler error (start_function).");
      copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
                     any_type_string);
    }else{
      copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
		     Pike_compiler->compiler_frame->previous->current_type);
    }

    $$ = Pike_compiler->compiler_frame;
  }
  ;

optional_constant: /* empty */
  {
    $$ = OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT;
  }
  | TOK_CONSTANT
  {
    $$ = 0;
  }
  ;

def: modifiers optional_attributes simple_type optional_constant
     TOK_IDENTIFIER start_function
  '('
  {
    $<number>$ = 0;
    /* Check for the (very special) case of create and create_args. */
    if (Pike_compiler->num_create_args) {
      struct pike_string *create_string = NULL;
      int e;
      MAKE_CONST_STRING(create_string, "create");
      if ($5->u.sval.u.string == create_string) {
	/* Prepend the create arguments. */
	if (Pike_compiler->num_create_args < 0) {
	  /* __create__() has varargs.
	   * Inhibit further arguments in the explicit create().
	   */
	  Pike_compiler->varargs = 1;
	  for (e = 0; e < -Pike_compiler->num_create_args; e++) {
	    struct identifier *id =
	      Pike_compiler->new_program->identifiers + e;
	    add_ref(id->type);
	    add_local_name(empty_pike_string, id->type, 0);
	    /* Note: add_local_name() above will return e. */
            bind_local(NULL, e);
	    Pike_compiler->compiler_frame->local_names[e].flags |=
	      LOCAL_VAR_IS_USED;
	  }
	} else {
	  for (e = 0; e < Pike_compiler->num_create_args; e++) {
	    struct identifier *id =
	      Pike_compiler->new_program->identifiers + e;
	    add_ref(id->type);
	    add_local_name(empty_pike_string, id->type, 0);
	    /* Note: add_local_name() above will return e. */
            bind_local(NULL, e);
	    Pike_compiler->compiler_frame->local_names[e].flags |=
	      LOCAL_VAR_IS_USED;
	  }
	}
	$<number>$ = e;
      }
    }
  }
  arguments ')'
  {
    int e, ee;

    /* Adjust opt_flags in case we've got an optional_constant. */
    Pike_compiler->compiler_frame->opt_flags = $4;

    /* construct the function type */
    push_finished_type(Pike_compiler->compiler_frame->current_return_type);

    if ($1 & ID_GENERATOR) {
      struct pike_string *name;

      /* Adjust the type to be a function that returns
       * a function(mixed|void, function(mixed...:void)|void:X).
       */
      push_type(T_VOID);
      push_type(T_MANY);

      push_type(T_VOID);
      push_type(T_VOID);
      push_type(T_VOID);
      push_type(T_MIXED);
      push_type(T_OR);
      push_type(T_MANY);
      push_type(T_OR);
      push_type(T_FUNCTION);

      push_type(T_VOID);
      push_type(T_MIXED);
      push_type(T_OR);
      push_type(T_FUNCTION);

      /* Entry point variable. */
      add_ref(int_type_string);
      MAKE_CONST_STRING(name, "__generator_entry_point__");
      Pike_compiler->compiler_frame->generator_local =
	add_local_name(name, int_type_string, 0);

      /* Stack contents to restore. */
      add_ref(array_type_string);
      MAKE_CONST_STRING(name, "__generator_stack__");
      add_local_name(name, array_type_string, 0);

      /* Resumption argument. */
      add_ref(mixed_type_string);
      MAKE_CONST_STRING(name, "__generator_argument__");
      add_local_name(name, mixed_type_string, 0);

      /* Resumption callback. */
      add_ref(function_type_string);
      MAKE_CONST_STRING(name, "__generator_callback__");
      add_local_name(name, function_type_string, 0);

      /* All of the above and the arguments are scoped. */
      for (e = 0; e < Pike_compiler->compiler_frame->generator_local + 4; e++) {
        /* Force local #e to be scoped. */
        node *n = mklocalnode(e, -1);
        if (e >= Pike_compiler->compiler_frame->generator_local) {
          /* Mark all generator locals as used. */
          mark_lvalue_as_used(n);
        }
        free_node(n);
      }
    }

    if ($<number>8 && (Pike_compiler->num_create_args < 0) && $9) {
      /* Do not allow further arguments if __create__() is varargs.
       *
       * NB: No need to complain that $9 != 0, as this should already
       *     have triggered a complaint when the corresponding variable
       *     was declared due to varargs being set.
       */
      $9 = 0;
    }
    e = $<number>8 + $9 - 1;
    if (Pike_compiler->varargs) {
      /* NB: This is set when either __create__() or create() has varargs. */
      struct local_name *var;
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      var = Pike_compiler->compiler_frame->local_names + ee;
      push_finished_type(var->def->type);
      e--;
      if (var->def->type->type != T_ARRAY) {
	yywarning("Varargs variable is not an array!! (Internal error)");
      } else {
	if (pop_type_stack(T_ARRAY)) {
	  compiler_discard_top_type();
	}
      }
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e >= $<number>8; e--)
    {
      struct local_name *var;
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      var = Pike_compiler->compiler_frame->local_names + ee;
      push_finished_type(var->def->type);
      if (var->init && (var->def->token == F_LOCAL)) {
	push_type(T_VOID);
	push_type(T_OR);
      }
      push_type(T_FUNCTION);
    }
    if (e >= 0) {
      /* __create__() arguments. */
      int i = FIND_LFUN(Pike_compiler->new_program, LFUN___CREATE__);
      struct identifier *id = ID_FROM_INT(Pike_compiler->new_program, i);
      struct pike_type *t = id->type;
      struct pike_type *trailer = compiler_pop_type();

      while (t && (t->type == T_FUNCTION)) {
	push_finished_type(t->car);
	t = t->cdr;
      }

      push_finished_type(trailer);
      free_type(trailer);

      t = id->type;

      while (t && (t->type == T_FUNCTION)) {
	push_reverse_type(T_FUNCTION);
	t = t->cdr;
      }
    }

    if (Pike_compiler->current_attributes) {
      node *n = Pike_compiler->current_attributes;
      while (n) {
	push_type_attribute(CDR(n)->u.sval.u.string);
	n = CAR(n);
      }
    }

    {
      struct pike_type *s=compiler_pop_type();
      int i = isidentifier($5->u.sval.u.string);

      if (Pike_compiler->compiler_pass != COMPILER_PASS_FIRST) {
	if (i < 0) {
          my_yyerror("Identifier %pS lost after first pass.",
		     $5->u.sval.u.string);
	}
      }

      $<n>$ = mktypenode(s);
      free_type(s);
    }


/*    if(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) */
    {
      /* FIXME:
       * set current_function_number for local functions as well
       */
      Pike_compiler->compiler_frame->current_function_number=
	define_function($5->u.sval.u.string,
			$<n>$->u.sval.u.type,
			$1 & (~ID_EXTERN),
			IDENTIFIER_PIKE_FUNCTION |
			(Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
			0,
			$4);

      Pike_compiler->varargs=0;
    }
  }
  block_or_semi
  {
    int e;
    if($12)
    {
      int f;
      node *check_args = NULL;
      struct compilation *c = THIS_COMPILATION;
      struct pike_string *save_file = c->lex.current_file;
      int save_line  = c->lex.current_line;
      int num_required_args = 0;
      struct identifier *i;
      c->lex.current_file = $5->current_file;
      c->lex.current_line = $5->line_number;

      if (($1 & ID_EXTERN) &&
	  (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)) {
	yywarning("Extern declared function definition.");
      }

      if ($1 & ID_GENERATOR) {
	struct pike_type *generator_type;
	int generator_stack_local;

	ref_push_string($5->u.sval.u.string);
	push_constant_text("\0generator");
	f_add(2);
	if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST &&
	    Pike_compiler->compiler_frame->current_return_type->type == PIKE_T_AUTO) {
	  /* Change "auto" return type to actual return type. */
	  push_finished_type(Pike_compiler->compiler_frame->current_return_type->car);
	} else {
	  push_finished_type(Pike_compiler->compiler_frame->current_return_type);
	}

	/* Adjust the type to be a function that returns
         * a function(mixed|void, function(mixed...:void)|void:X).
	 */
	push_type(T_VOID);
	push_type(T_MANY);

	push_type(T_VOID);
	push_type(T_VOID);
	push_type(T_VOID);
	push_type(T_MIXED);
	push_type(T_OR);
	push_type(T_MANY);
	push_type(T_OR);
	push_type(T_FUNCTION);

	push_type(T_VOID);
	push_type(T_MIXED);
	push_type(T_OR);
	push_type(T_FUNCTION);

	generator_type = compiler_pop_type();
	f = dooptcode(Pike_sp[-1].u.string, $12, generator_type,
		      ID_INLINE|ID_PROTECTED|ID_PRIVATE|ID_USED);
	pop_stack();

	generator_stack_local =
	  Pike_compiler->compiler_frame->generator_local + 1;
	Pike_compiler->compiler_frame->generator_local = -1;
	free_type(Pike_compiler->compiler_frame->current_return_type);
	Pike_compiler->compiler_frame->current_return_type = generator_type;
	$12 = mknode(F_COMMA_EXPR,
		     mknode(F_ASSIGN, mklocalnode(generator_stack_local, 0),
			    mkefuncallnode("aggregate", NULL)),
		     mknode(F_RETURN, mkgeneratornode(f), NULL));
      }

      for(e=$<number>8; e<$<number>8+$9; e++)
      {
        int ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
        struct local_name *var;
        var = Pike_compiler->compiler_frame->local_names + ee;
        if((e >= $<number>8) && (!var->name || !var->name->len) &&
           (var->def->token == F_LOCAL)) {
	  my_yyerror("Missing name for argument %d.", e - $<number>8);
	} else {
          check_args = mknode(F_COMMA_EXPR, check_args, set_default_value(ee));

	  if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
	      /* FIXME: Should probably use some other flag. */
	      if ((runtime_options & RUNTIME_CHECK_TYPES) &&
                  (var->def->type != mixed_type_string)) {
		node *local_node;

		/* fprintf(stderr, "Creating soft cast node for local #%d\n", e);*/

                local_node = mkcastnode(mixed_type_string, mklocalnode(ee, 0));

		/* NOTE: The cast to mixed above is needed to avoid generating
		 *       compilation errors, as well as avoiding optimizations
		 *       in mksoftcastnode().
		 */
		check_args =
		  mknode(F_COMMA_EXPR, check_args,
                         mksoftcastnode(var->def->type, local_node));
	      }
	  }
	}
      }

      if ($<number>8) {
	/* Generate code that calls __create__(). */
	node *args = NULL;
        int ee;
	e = $<number>8;
	if (Pike_compiler->varargs) {
	  e--;
          ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
	  args = mknode(F_PUSH_ARRAY,
                        mklocalnode(ee, 0),
			NULL);
	}
	while (e--) {
          ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
          args = mknode(F_ARG_LIST, mklocalnode(ee, 0), args);
	}

	e = FIND_LFUN(Pike_compiler->new_program, LFUN___CREATE__);
	check_args =
	  mknode(F_COMMA_EXPR,
		 mknode(F_POP_VALUE,
			mknode(F_APPLY,
			       mkidentifiernode(e),
			       args),
			NULL),
		 check_args);
      }

      {
	int l = $12->line_number;
	struct pike_string *f = $12->current_file;
	if (check_args) {
	  /* Prepend the arg checking code. */
	  $12 = mknode(F_COMMA_EXPR, mknode(F_POP_VALUE, check_args, NULL), $12);
	}
	c->lex.current_line = l;
	c->lex.current_file = f;
      }

      f=dooptcode($5->u.sval.u.string, $12, $<n>11->u.sval.u.type, $1);

      i = ID_FROM_INT(Pike_compiler->new_program, f);
      i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

      if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST &&
          Pike_compiler->compiler_frame->current_return_type->type == PIKE_T_AUTO)
      {
        /* Change "auto" return type to actual return type. */
        int ee;
        push_finished_type(Pike_compiler->compiler_frame->current_return_type->car);

        e = $<number>8 + $9 - 1;
        if(Pike_compiler->varargs &&
           (!$<number>8 || (Pike_compiler->num_create_args >= 0)))
        {
          struct local_name *var;
          ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
          var = Pike_compiler->compiler_frame->local_names + ee;
          push_finished_type(var->def->type);
          e--;
          if (pop_type_stack(T_ARRAY)) {
	    compiler_discard_top_type();
	  }
        }else{
          push_type(T_VOID);
        }
        push_type(T_MANY);
        for(; e>=0; e--)
        {
          struct local_name *var;
          ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
          var = Pike_compiler->compiler_frame->local_names + ee;
          push_finished_type(var->def->type);
          if (var->init && (var->def->token == F_LOCAL)) {
            push_type(T_VOID);
            push_type(T_OR);
          }
          push_type(T_FUNCTION);
        }

        if (Pike_compiler->current_attributes)
        {
          node *n = Pike_compiler->current_attributes;
          while (n) {
            push_type_attribute(CDR(n)->u.sval.u.string);
            n = CAR(n);
          }
        }

        free_type( i->type );
        i->type = compiler_pop_type();
      }

#ifdef PIKE_DEBUG
      if(Pike_interpreter.recoveries &&
	 ((Pike_sp - Pike_interpreter.evaluator_stack) <
	  Pike_interpreter.recoveries->stack_pointer))
	Pike_fatal("Stack error (underflow)\n");

      if((Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) &&
	 (f != Pike_compiler->compiler_frame->current_function_number)) {
	fprintf(stderr, "define_function()/do_opt_code() failed for symbol %s\n",
		$5->u.sval.u.string->str);
	dump_program_desc(Pike_compiler->new_program);
	Pike_fatal("define_function screwed up! %d != %d\n",
	      f, Pike_compiler->compiler_frame->current_function_number);
      }
#endif

      c->lex.current_line = save_line;
      c->lex.current_file = save_file;
    } else {
      /* Prototype; don't warn about unused arguments. */
      int ee;
      for (e = Pike_compiler->compiler_frame->current_number_of_locals; e--;) {
        ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
        Pike_compiler->compiler_frame->local_names[ee].flags |= LOCAL_VAR_IS_USED;
      }
      if( Pike_compiler->compiler_frame->current_return_type->type == PIKE_T_AUTO )
          yyerror("'auto' return type not allowed for prototypes.");
      if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
	for(e=0; e<$<number>8+$9; e++)
	{
          node *init;
          ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
          init = Pike_compiler->compiler_frame->local_names[ee].init;
	  if (init) {
	    low_yyreport(REPORT_WARNING, init->current_file, init->line_number,
			 parser_system_string, 0,
			 "Argument default values are not supported in prototypes.");
	  }
	}
      }
    }
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $6) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $6,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    free_node($5);
    free_node($<n>11);
  }
  | modifiers optional_attributes simple_type
  optional_constant TOK_IDENTIFIER start_function
    error
  {
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $6) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $6,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    free_node($5);
  }
  | modifiers optional_attributes simple_type optional_constant bad_identifier
  {
    compiler_discard_type();
  }
    '(' arguments ')' block_or_semi
  {
    if ($10) free_node($10);
  }
  | modifiers optional_attributes simple_type optional_constant name_list ';'
  | inheritance {}
  | import {}
  | constant {}
  | modifiers named_class { free_node($2); }
  | modifiers enum { free_node($2); }
  | annotation ';'
  {
    if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
      $1 = mknode(F_COMMA_EXPR, $1, NULL);
      compiler_add_program_annotations(0, $1);
    }
    free_node($1);
  }
  | '@' TOK_CONSTANT ';'
  {
    Pike_compiler->new_program->flags |= PROGRAM_CONSTANT;
  }
  | typedef {}
  | static_assertion expected_semicolon {}
  | error TOK_LEX_EOF
  {
    reset_type_stack();
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file");
  }
  | error ';'
  {
    reset_type_stack();
    yyerrok;
/*     if(Pike_compiler->num_parse_error>5) YYACCEPT; */
  }
  | error '}'
  {
    reset_type_stack();
    yyerror("Missing ';'.");
   /* yychar = '}';	*/ /* Put the '}' back on the input stream */
  }
  | modifiers
   '{'
    {
      $<number>$=THIS_COMPILATION->lex.pragmas;
      THIS_COMPILATION->lex.pragmas|=$1;
      if (Pike_compiler->current_annotations) {
	yywarning("Annotation blocks are not supported.");
      }
    }
      program
   close_brace_or_eof
    {
      THIS_COMPILATION->lex.pragmas=$<number>3;
    }
  ;

static_assertion: TOK_STATIC_ASSERT '(' assignment_expr ',' assignment_expr ')'
  {
    Pike_compiler->init_node =
      mknode(F_COMMA_EXPR, Pike_compiler->init_node,
	     mkefuncallnode("_Static_assert",
			    mknode(F_ARG_LIST, $3, $5)));
  }
  ;

optional_dot_dot_dot: TOK_DOT_DOT_DOT { $$=1; }
  | TOK_DOT_DOT
  {
    yyerror("Range indicator ('..') where elipsis ('...') expected.");
    $$=1;
  }
  | /* empty */ { $$=0; }
  ;

optional_identifier: TOK_IDENTIFIER
  | bad_identifier { $$=0; }
  | /* empty */ { $$=0; }
  ;

optional_default_value: '=' assignment_expr { $$ = $2; }
  | /* empty */ { $$ = NULL; }
  ;

new_arg_name: full_type optional_dot_dot_dot
  {
    /* This is needed in order to avoid issues with eg using cast
     * in the optional_default_value.
     *
     * An alternative could be to use simple_type instead of full_type.
     */
    type_stack_mark();
  }
  optional_identifier optional_default_value
  {
    int i;

    /* Pop the mark form above. */
    pop_stack_mark();

    if(Pike_compiler->varargs) yyerror("Can't define more arguments after ...");

    if (TEST_COMPAT(8, 0) &&
	!pike_types_le(zero_type_string, peek_type_stack(), 0, 0)) {
      push_type(PIKE_T_ZERO);
      push_type(T_OR);
    }

    if($2)
    {
      push_unlimited_array_type(T_ARRAY);

      Pike_compiler->varargs=1;
    }

    if ($5) {
      if ($2) {
	yyerror("Varargs variable with default value.");
	free_node($5);
	$5 = NULL;
      } else if (!$4) {
	yywarning("Unnamed variable with default value.");
      }
    }

    if(!$4)
    {
      $4 = mkstrnode(empty_pike_string);
    }

    if($4->u.sval.u.string->len &&
       islocal($4->u.sval.u.string) >= 0)
      my_yyerror("Variable %pS appears twice in argument list.",
		 $4->u.sval.u.string);

    i = add_local_name($4->u.sval.u.string, compiler_pop_type(), $5);
    if (i >= 0) {
      /* Don't warn about unused arguments. */
      Pike_compiler->compiler_frame->local_names[i].flags |= LOCAL_VAR_IS_USED;
      if ($5) {
	Pike_compiler->compiler_frame->local_names[i].flags |=
	  LOCAL_VAR_IS_ARGUMENT;
      }
      bind_local(NULL, i);
    }
    free_node($4);
  }
  | open_bracket_with_line_info low_lvalue_list ']' optional_default_value
  {
    node *n = mknode(F_ARRAY_LVALUE, $2, 0);
    node *init;
    int i;
    type_stack_mark();
    fix_type_field(n);
    push_finished_type(n->type);
    if ($4) {
      push_type(T_VOID);
      push_type(T_OR);
      free_type(n->type);
      copy_pike_type(n->type, peek_type_stack());
    }
    i = add_local_name(empty_pike_string, pop_unfinished_type(), NULL);
    Pike_compiler->compiler_frame->local_names[i].flags |= LOCAL_VAR_IS_USED;
    bind_local(NULL, i);
    init = mklocalnode(i, 0);
    if ($4) {
      init = mknode(F_LOR, init, $4);
    }
    /* Redefine local #i as [ $2 ] with an initializer of
     * orig_i || $4.
     */
    redefine_local(NULL, i, n, init);
    free_node($1);
  }
  ;

func_args: '(' arguments ')'
  {
    $$=$2;
  }
  ;

arguments: /* empty */ optional_comma { $$=0; }
  | arguments2 optional_comma
  ;

arguments2: new_arg_name { $$ = 1; }
  | arguments2 ',' new_arg_name { $$ = $1 + 1; }
  | arguments2 ':' new_arg_name
  {
    yyerror("Unexpected ':' in argument list.");
    $$ = $1 + 1;
  }
  ;

modifier:
    TOK_FINAL_ID   { $$ = ID_FINAL | ID_INLINE; }
  | TOK_STATIC     {
    $$ = ID_PROTECTED;
    if( !(THIS_COMPILATION->lex.pragmas & ID_NO_DEPRECATION_WARNINGS) &&
        !TEST_COMPAT(7, 8) &&
	(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) )
      yywarning("Keyword static is deprecated in favour of 'protected'.");
    }
  | TOK_EXTERN     { $$ = ID_EXTERN | ID_OPTIONAL; }
  | TOK_OPTIONAL   { $$ = ID_OPTIONAL; }
  | TOK_PRIVATE    { $$ = ID_PRIVATE | ID_PROTECTED | ID_INLINE; }
  | TOK_LOCAL_ID   { $$ = ID_INLINE; }
  | TOK_PUBLIC     { $$ = ID_PUBLIC; }
  | TOK_PROTECTED  { $$ = ID_PROTECTED; }
  | TOK_INLINE     { $$ = ID_INLINE; }
  | TOK_VARIANT    { $$ = ID_VARIANT; }
  | TOK_WEAK       { $$ = ID_WEAK; }
  | TOK_CONTINUE   { $$ = ID_GENERATOR; }
  | TOK_UNUSED     { $$ = ID_USED; }
  ;

magic_identifiers1:
    TOK_FINAL_ID   { $$ = "final"; }
  | TOK_STATIC     { $$ = "static"; }
  | TOK_EXTERN	   { $$ = "extern"; }
  | TOK_PRIVATE    { $$ = "private"; }
  | TOK_LOCAL_ID   { $$ = "local"; }
  | TOK_PUBLIC     { $$ = "public"; }
  | TOK_PROTECTED  { $$ = "protected"; }
  | TOK_INLINE     { $$ = "inline"; }
  | TOK_OPTIONAL   { $$ = "optional"; }
  | TOK_VARIANT    { $$ = "variant"; }
  | TOK_WEAK       { $$ = "__weak__"; }
  | TOK_UNUSED     { $$ = "__unused__"; }
  | TOK_STATIC_ASSERT	{ $$ = "_Static_assert"; }
  ;

magic_identifiers2:
    TOK_VOID_ID       { $$ = "void"; }
  | TOK_MIXED_ID      { $$ = "mixed"; }
  | TOK_ARRAY_ID      { $$ = "array"; }
  | TOK_ATTRIBUTE_ID  { $$ = "__attribute__"; }
  | TOK_DEPRECATED_ID { $$ = "__deprecated__"; }
  | TOK_MAPPING_ID    { $$ = "mapping"; }
  | TOK_MULTISET_ID   { $$ = "multiset"; }
  | TOK_OBJECT_ID     { $$ = "object"; }
  | TOK_FUNCTION_ID   { $$ = "function"; }
  | TOK_FUNCTION_NAME { $$ = "__func__"; }
  | TOK_PROGRAM_ID    { $$ = "program"; }
  | TOK_STRING_ID     { $$ = "string"; }
  | TOK_FLOAT_ID      { $$ = "float"; }
  | TOK_INT_ID        { $$ = "int"; }
  | TOK_ENUM	      { $$ = "enum"; }
  | TOK_TYPEDEF       { $$ = "typedef"; }
  | TOK_UNKNOWN	      { $$ = "__unknown__"; }
  /* | TOK_AUTO_ID       { $$ = "auto"; } */
  ;

magic_identifiers3:
    TOK_IF         { $$ = "if"; }
  | TOK_DO         { $$ = "do"; }
  | TOK_FOR        { $$ = "for"; }
  | TOK_WHILE      { $$ = "while"; }
  | TOK_ELSE       { $$ = "else"; }
  | TOK_FOREACH    { $$ = "foreach"; }
  | TOK_CATCH      { $$ = "catch"; }
  | TOK_GAUGE      { $$ = "gauge"; }
  | TOK_CLASS      { $$ = "class"; }
  | TOK_BREAK      { $$ = "break"; }
  | TOK_CASE       { $$ = "case"; }
  | TOK_CONSTANT   { $$ = "constant"; }
  | TOK_CONTINUE   { $$ = "continue"; }
  | TOK_DEFAULT    { $$ = "default"; }
  | TOK_IMPORT     { $$ = "import"; }
  | TOK_INHERIT    { $$ = "inherit"; }
  | TOK_LAMBDA     { $$ = "lambda"; }
  | TOK_PREDEF     { $$ = "predef"; }
  | TOK_RETURN     { $$ = "return"; }
  | TOK_SSCANF     { $$ = "sscanf"; }
  | TOK_SWITCH     { $$ = "switch"; }
  | TOK_TYPEOF     { $$ = "typeof"; }
  | TOK_GLOBAL     { $$ = "global"; }
  ;

magic_identifiers: magic_identifiers1 | magic_identifiers2 | magic_identifiers3 ;

magic_identifier: TOK_IDENTIFIER | TOK_RESERVED
  | magic_identifiers
  {
    struct pike_string *tmp=make_shared_string($1);
    $$=mkstrnode(tmp);
    free_string(tmp);
  }
  ;

annotation: '@' constant_expr
  {
    $$ = $2;
  }
  ;

annotation_list: /* empty */ { $$ = NULL; }
  | annotation ':' annotation_list
  {
    $$ = mknode(F_COMMA_EXPR, $1, $3);
  }
  ;

modifiers: annotation_list modifier_list
  {
    free_node(Pike_compiler->current_annotations);
    Pike_compiler->current_annotations = $1;
    $$ = Pike_compiler->current_modifiers = $2 |
      (THIS_COMPILATION->lex.pragmas & ID_MODIFIER_MASK);
  }
  ;

modifier_list: /* empty */ { $$ = 0; }
  | modifier_list modifier { $$ = $1 | $2; }
  ;

attribute: TOK_ATTRIBUTE_ID '(' string_constant optional_comma ')'
  {
    $$ = $3;
  }
  | TOK_DEPRECATED_ID '(' ')'
  {
    struct pike_string *deprecated_string;
    MAKE_CONST_STRING(deprecated_string, "deprecated");
    $$ = mkstrnode(deprecated_string);
  }
  | TOK_DEPRECATED_ID
  {
    struct pike_string *deprecated_string;
    MAKE_CONST_STRING(deprecated_string, "deprecated");
    $$ = mkstrnode(deprecated_string);
  }
  ;

optional_attributes: /* empty */
  {
    if (Pike_compiler->current_attributes) {
      free_node(Pike_compiler->current_attributes);
    }
    if ((Pike_compiler->current_attributes =
	 THIS_COMPILATION->lex.attributes)) {
      add_ref(Pike_compiler->current_attributes);
    }
  }
  | optional_attributes attribute
  {
    if ($2) {
      Pike_compiler->current_attributes =
	mknode(F_ARG_LIST, Pike_compiler->current_attributes, $2);
    }
  }
  ;

cast: open_paren_with_line_info type ')'
    {
      struct pike_type *s = compiler_pop_type();
      $$ = mktypenode(s);
      free_type(s);
      COPY_LINE_NUMBER_INFO($$, $1);
      free_node ($1);
    }
    ;

soft_cast: open_bracket_with_line_info type ']'
    {
      struct pike_type *s = compiler_pop_type();
      $$ = mktypenode(s);
      free_type(s);
      COPY_LINE_NUMBER_INFO($$, $1);
      free_node ($1);
    }
    ;

/* Either a basic_type-prefixed expression, or an identifier type.
 * Value on type stack.
 */
type2: type | identifier_type ;

/* Full type expression.
 * Value moved to compiler_frame->current_type.
 */
simple_type: full_type
  {
    update_current_type();
  }
  ;

/* Basic_type-prefixed expression or an identifier type.
 * Value moved to compiler_frame->current_type.
 */
simple_type2: type2
  {
    update_current_type();
  }
  ;

/* Full type expression. Value on type stack.
 * Typically used in contexts where there must be a type,
 * and expressions are invalid.
 */
full_type: full_type '|' type3 { push_type(T_OR); }
  | type3
  ;

/* Basic_type-prefixed expression. Value on type stack.
 * Typically used in contexts where expressions are valid.
 */
type: type '|' type3 { push_type(T_OR); }
  | basic_type
  ;

/* Either a basic_type or an identifier type. Value on type stack. */
type3: basic_type | identifier_type ;

/* Literal type. Value on type stack. */
basic_type:
    TOK_FLOAT_ID                      { push_type(T_FLOAT); }
  | TOK_VOID_ID                       { push_type(T_VOID); }
  | TOK_MIXED_ID                      { push_type(T_MIXED); }
  | TOK_UNKNOWN			      { push_type(PIKE_T_UNKNOWN); }
  | TOK_AUTO_ID			{ push_type(T_ZERO); push_type(PIKE_T_AUTO); }
  | TOK_STRING_ID   opt_string_width  {}
  | TOK_INT_ID      opt_int_range     {}
  | TOK_MAPPING_ID  opt_mapping_type  {}
  | TOK_FUNCTION_ID opt_function_type {}
  | TOK_OBJECT_ID   opt_program_type  {}
  | TOK_PROGRAM_ID  opt_program_type  { push_type(T_PROGRAM); }
  | TOK_ARRAY_ID    opt_array_type    {}
  | TOK_MULTISET_ID opt_multiset_type { push_type(T_MULTISET); }
  | TOK_ATTRIBUTE_ID '(' string_constant ',' full_type ')'
  {
    push_type_attribute($3->u.sval.u.string);
    free_node($3);
  }
  | TOK_ATTRIBUTE_ID '(' string_constant error ')'
  {
    push_type(T_MIXED);
    push_type_attribute($3->u.sval.u.string);
    free_node($3);
  }
  | TOK_ATTRIBUTE_ID error
  {
    push_type(T_MIXED);
  }
  | TOK_DEPRECATED_ID '(' full_type ')'
  {
    struct pike_string *deprecated_string;
    MAKE_CONST_STRING(deprecated_string, "deprecated");
    push_type_attribute(deprecated_string);
  }
  | TOK_DEPRECATED_ID '(' error ')'
  {
    struct pike_string *deprecated_string;
    MAKE_CONST_STRING(deprecated_string, "deprecated");
    push_type(T_MIXED);
    push_type_attribute(deprecated_string);
  }
  ;

/* Identifier type. Value on type stack.
 *
 * NB: Introduces shift-reduce conflict on TOK_LOCAL_ID.
 */
identifier_type: id_expr
  {
    if ($1) {
      fix_type_field($1);

      if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	  !pike_types_le($1->type, typeable_type_string, 0, 0) &&
	  (THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
	yytype_report(REPORT_WARNING,
		      $1->current_file, $1->line_number, typeable_type_string,
		      $1->current_file, $1->line_number, $1->type,
		      0, "Invalid type.");
      }
    }

    resolv_type($1);

    /* Attempt to name the type. */
    if (Pike_compiler->last_identifier) {
      push_type_name(Pike_compiler->last_identifier);
    }

    free_node($1);
  }
  | typeof
  {
    if ($1 && CAR($1)) {
      fix_type_field($1);
      push_finished_type(CAR($1)->type);
      free_node($1);
    } else {
      push_finished_type(mixed_type_string);
    }
  }
  ;

number: TOK_NUMBER
  | '-' TOK_NUMBER
  {
#ifdef PIKE_DEBUG
    if (($2->token != F_CONSTANT) || (TYPEOF($2->u.sval) != T_INT)) {
      Pike_fatal("Unexpected number in negative int-range.\n");
    }
#endif /* PIKE_DEBUG */
    $$ = mkintnode(-($2->u.sval.u.integer));
    free_node($2);
  }
  ;

number_or_maxint: /* Empty */
  {
    $$ = mkintnode(MAX_INT_TYPE);
  }
  | number;

number_or_minint: /* Empty */
  {
    $$ = mkintnode(MIN_INT_TYPE);
  }
  | number;

expected_dot_dot: TOK_DOT_DOT
  | TOK_DOT_DOT_DOT
  {
    yyerror("Elipsis ('...') where range indicator ('..') expected.");
  }
  ;

safe_int_range_type_low: TOK_BITS
  {
    push_int_type( 0, (1<<$1->u.sval.u.integer)-1 );
    free_node( $1 );
  }
  | number_or_minint expected_dot_dot number_or_maxint
  {
    INT_TYPE min = MIN_INT_TYPE;
    INT_TYPE max = MAX_INT_TYPE;

    /* FIXME: Check that $3 is >= $1. */
    if($3->token == F_CONSTANT) {
      if (TYPEOF($3->u.sval) == T_INT) {
	max = $3->u.sval.u.integer;
      } else if (is_bignum_object_in_svalue(&$3->u.sval)) {
	push_int(0);
	if (is_lt(&$3->u.sval, Pike_sp-1)) {
	  max = MIN_INT_TYPE;
	}
	pop_stack();
      }
    }

    if($1->token == F_CONSTANT) {
      if (TYPEOF($1->u.sval) == T_INT) {
	min = $1->u.sval.u.integer;
      } else if (is_bignum_object_in_svalue(&$1->u.sval)) {
	push_int(0);
	if (is_lt(Pike_sp-1, &$1->u.sval)) {
	  min = MAX_INT_TYPE;
	}
	pop_stack();
      }
    }

    push_int_type(min, max);

    free_node($1);
    free_node($3);
  }
  | number
  {
    INT_TYPE val = MAX_INT_TYPE;

    if($1->token == F_CONSTANT) {
      if (TYPEOF($1->u.sval) == T_INT) {
	val = $1->u.sval.u.integer;
      } else if (is_bignum_object_in_svalue(&$1->u.sval)) {
	push_int(0);
	if (is_lt(&$1->u.sval, Pike_sp-1)) {
	  val = MIN_INT_TYPE;
	}
	pop_stack();
      }
    }

    push_int_type(val, val);

    free_node($1);
  }
  | error
  {
    push_int_type(MIN_INT32, MAX_INT32);
    yyerror("Expected integer range.");
  }
  ;

safe_int_range_type: safe_int_range_type_low
  | safe_int_range_type '|' safe_int_range_type_low
  {
    push_type(PIKE_T_OR);
  }
  ;

opt_int_range: /* Empty */
  {
    push_int_type(MIN_INT_TYPE, MAX_INT_TYPE);
  }
  | '(' safe_int_range_type ')'
  ;

opt_string_width: opt_int_range
  {
    push_unlimited_array_type(T_STRING);
  }
  | '(' safe_int_range_type ':' safe_int_range_type ')'
  {
    push_reverse_type(T_STRING);
  }
  | '(' safe_int_range_type ':' ')'
  {
    push_finished_type(int_type_string);
    push_reverse_type(T_STRING);
  }
  | '(' ':' safe_int_range_type ')'
  {
    push_unlimited_array_type(T_STRING);
  }
  ;

opt_program_type:  /* Empty */ { push_object_type(0, 0); }
  | '(' full_type ')'
  | '(' string_constant ')'
  {
    resolv_type($2);
    push_type_name($2->u.sval.u.string);
    free_node($2);
  }
  | '(' error ')'
  {
    push_object_type(0, 0);
    yyerror("Invalid program subtype.");
  }
  ;

opt_function_type: '('
  {
    type_stack_mark();
  }
  function_type_list optional_dot_dot_dot ':'
  {
    /* Add the many type if there is none. */
    if ($4)
    {
      if (!$3) {
	/* function_type_list ends with a comma, or is empty.
	 * FIXME: Should this be a syntax error or not?
	 */
	if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
	  yyerror("Missing type before ... .");
	}
	push_type(T_MIXED);
      }
    }else{
      push_type(T_VOID);
    }
  }
  full_type ')'
  {
    push_reverse_type(T_MANY);
    Pike_compiler->pike_type_mark_stackp--;
    while (*Pike_compiler->pike_type_mark_stackp+1 <
	   Pike_compiler->type_stackp) {
      push_reverse_type(T_FUNCTION);
    }
  }
  | /* empty */
  {
   push_type(T_MIXED);
   push_type(T_VOID);
   push_type(T_OR);

   push_type(PIKE_T_UNKNOWN);

   push_type(T_MANY);

   if (!(THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES) ||
       TEST_COMPAT(8, 0)) {
     /* For type validation we also need this.
      * Otherwise code like
      *
      *   function foo;
      *   function(string:void) bar = foo;
      *
      * will fail.
      */
     push_type(PIKE_T_UNKNOWN);

     push_type(T_MIXED);

     push_type(T_MANY);

     push_type(T_OR);
   }
  }
  ;

function_type_list: /* Empty */ optional_comma { $$=0; }
  | function_type_list2 optional_comma { $$=!$2; }
  ;

function_type_list2: full_type { $$=1; }
  | function_type_list2 ','
  {
  }
  full_type
  ;

opt_multiset_type: '(' full_type ')'
  |  { push_type(T_MIXED); }
  ;

opt_array_type: '(' full_type ')'
  { push_unlimited_array_type(T_ARRAY); }
  | /* Empty */
  { push_type(T_MIXED); push_unlimited_array_type(T_ARRAY); }
  | '(' safe_int_range_type ':' full_type ')'
  { push_reverse_type(T_ARRAY); }
  | '(' ':' full_type ')'
  { push_unlimited_array_type(T_ARRAY); }
  | '(' safe_int_range_type ':' ')'
  { push_type(T_MIXED); push_reverse_type(T_ARRAY); }
  | '(' safe_int_range_type ')'
  {
    yyerror("Missing ':' after length in array type.");
    push_type(T_MIXED); push_reverse_type(T_ARRAY);
  }
  ;

opt_mapping_type: '('
  {
  }
  full_type ':'
  {
  }
  full_type
  {
    push_reverse_type(T_MAPPING);
  }
  ')'
  | /* empty */
  {
    push_type(T_MIXED);
    push_type(T_MIXED);
    push_type(T_MAPPING);
  }
  ;


name_list: new_name
  | name_list ',' new_name
  ;

new_name: TOK_IDENTIFIER
  {
    struct pike_type *type;
    node *n;

    type = Pike_compiler->compiler_frame->current_type;
    push_finished_type(type);
    if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	(!type || ((type->type != PIKE_T_AUTO) && (type->type != T_FLOAT)))) {
      /* NB: Global variables of type float are initialized to 0.0. */
      if (!pike_types_le(zero_type_string, type, 0, 0)) {
	if (!TEST_COMPAT(8,0) &&
	    (THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
	  ref_push_string($1->u.sval.u.string);
	  yytype_report(REPORT_WARNING, NULL, 0, zero_type_string,
			NULL, 0, type,
			1, "Type does not contain zero for variable without "
			"initializer %s. Type adjusted.");
	}
	push_type(PIKE_T_ZERO);
	push_type(T_OR);
      }
    }
    n = Pike_compiler->current_attributes;
    while(n) {
      push_type_attribute(CDR(n)->u.sval.u.string);
      n = CAR(n);
    }
    type=compiler_pop_type();
    define_variable($1->u.sval.u.string, type,
		    Pike_compiler->current_modifiers);
    free_type(type);
    free_node($1);
  }
  | bad_identifier {}
  | TOK_IDENTIFIER '='
  {
    struct pike_type *type;
    node *n;

    push_finished_type(Pike_compiler->compiler_frame->current_type);
    n = Pike_compiler->current_attributes;
    while(n) {
      push_type_attribute(CDR(n)->u.sval.u.string);
      n = CAR(n);
    }
    type=compiler_pop_type();
    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)) {
      yywarning("Extern declared variable has initializer.");
    }
    $<number>$=define_variable($1->u.sval.u.string, type,
			       Pike_compiler->current_modifiers & (~ID_EXTERN));
    free_type(type);
  }
  assignment_expr
  {
    if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	!TEST_COMPAT(7, 8) && ($4) && ($4->token == F_CONSTANT) &&
	!Pike_compiler->num_parse_error) {
      /* Check if it is zero, in which case we can throw it away.
       *
       * NB: The compat test is due to that this changes the semantics
       *     of calling __INIT() by hand.
       */
      if ((TYPEOF($4->u.sval) == PIKE_T_INT) && !SUBTYPEOF($4->u.sval) &&
	  !$4->u.sval.u.integer &&
	  !IDENTIFIER_IS_ALIAS(ID_FROM_INT(Pike_compiler->new_program,
					   $<number>3)->identifier_flags)) {
	/* NB: Inherited variables get converted into aliases by
	 *     define_variable, and we need to support clearing
	 *     of inherited variables.
	 */
#ifdef PIKE_DEBUG
	if (l_flag > 5) {
	  fprintf(stderr,
		  "Ignoring initialization to zero for variable %s.\n",
		  $1->u.sval.u.string->str);
	}
#endif /* PIKE_DEBUG */
        if(Pike_compiler->compiler_frame->current_type->type == PIKE_T_AUTO)
        {
          // auto variable type needs to be updated.
          fix_type_field( $4 );
          fix_auto_variable_type( $<number>3, $4->type );
        }
	free_node($4);
	$4 = NULL;
      }
    }
    if ($4) {
      node *n;
      // this is done in both passes to get somewhat better handling
      // of auto types.
      //
      // an example is: auto a  = typeof(b); auto b = (["foo":"bar"]);
      // if this is only done in the second pass the type of a will be
      // type(auto), not type(mapping(..))
      if( Pike_compiler->compiler_frame->current_type->type == PIKE_T_AUTO )
      {
        fix_type_field( $4 );
        fix_auto_variable_type( $<number>3, $4->type );
      }
      n = mkcastnode(void_type_string,
		     mknode(F_ASSIGN,
			    mkidentifiernode($<number>3), $4));
      if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
	// This makes sure that #pragma {no_,}deprecation_warnings
	// works as expected.
	optimize_node(n);
      }
      Pike_compiler->init_node=mknode(F_COMMA_EXPR,Pike_compiler->init_node,
				      n);
    }
    free_node($1);
  }
  | TOK_IDENTIFIER '=' error
  {
    free_node($1);
  }
  | TOK_IDENTIFIER '=' TOK_LEX_EOF
  {
    yyerror("Unexpected end of file in variable definition.");
    free_node($1);
  }
  | bad_identifier '=' assignment_expr
  {
    free_node($3);
  }
  ;

/* Type at compiler_frame->current_type. */
new_local_name: TOK_IDENTIFIER
  {
    int id;
    struct pike_type *type;
    if (pike_types_le(zero_type_string,
		      Pike_compiler->compiler_frame->current_type, 0, 0)) {
      copy_pike_type(type, Pike_compiler->compiler_frame->current_type);
    } else {
      struct compilation *c = THIS_COMPILATION;

      type_stack_mark();
      push_finished_type(Pike_compiler->compiler_frame->current_type);
      push_type(PIKE_T_ZERO);
      push_type(T_OR);
      type = pop_unfinished_type();

      if ((Pike_compiler->compiler_pass == COMPILER_PASS_LAST) &&
	  (c->lex.pragmas & ID_STRICT_TYPES) && !TEST_COMPAT(8, 0)) {
	ref_push_string($1->u.sval.u.string);
	yytype_report(REPORT_WARNING, NULL, 0, zero_type_string,
		      NULL, 0, Pike_compiler->compiler_frame->current_type,
		      1, "Type does not contain zero for variable without "
		      "initializer %s. Type adjusted.");
      }
    }
    id = add_local_name($1->u.sval.u.string, type, 0);
    if( type->type == PIKE_T_AUTO )
    {
      /* FIXME: Update type on assign instead! */
      yyerror("auto only valid when the variable is assigned in definition.");
    }

    if (id >= 0) {
      /* FIXME: Consider using mklocalnode(id, -1). */
      $$=mknode(F_ASSIGN, mklocalnode(id,0), mkintnode(0));
    } else
      $$ = 0;
    free_node($1);
  }
  | bad_identifier { $$=0; }
  | TOK_IDENTIFIER '='
  {
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    type_stack_mark();
  }
  assignment_expr
  {
    int id;
    struct pike_type *type;
    pop_stack_mark();
    update_current_type();
    copy_pike_type(type, Pike_compiler->compiler_frame->current_type);
    if( type->type == PIKE_T_AUTO &&
	Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
    {
        free_type( type );
        fix_type_field( $4 );
        copy_pike_type( type, $4->type );
    }
    id = add_local_name($1->u.sval.u.string, type, 0);
    if (id >= 0) {
      if (!(THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
	/* Only warn about unused initialized variables in strict types mode. */
	Pike_compiler->compiler_frame->local_names[id].flags |= LOCAL_VAR_IS_USED;
      }
      $$ = mknode(F_INITIALIZE, mklocalnode(id, 0), $4);
    } else
      $$ = 0;
    free_node($1);
  }
  | bad_identifier '='
  {
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    type_stack_mark();
  }
  assignment_expr
  {
    pop_stack_mark();
    update_current_type();
    free_node($4);
    $$=0;
  }
  | TOK_IDENTIFIER '='
  {
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    type_stack_mark();
  }
  error
  {
    pop_stack_mark();
    update_current_type();
    free_node($1);
    /* No yyerok here since we aren't done yet. */
    $$=0;
  }
  | TOK_IDENTIFIER '='
  {
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    type_stack_mark();
  }
  TOK_LEX_EOF
  {
    pop_stack_mark();
    update_current_type();
    yyerror("Unexpected end of file in local variable definition.");
    free_node($1);
    /* No yyerok here since we aren't done yet. */
    $$=0;
  }
  ;

line_number_info: /* empty */
  {
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
  ;

block:'{'
  {
    $<number>1=Pike_compiler->num_used_modules;
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  line_number_info
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;

    if($<number>$ == -1) /* if 'first block' */
      Pike_compiler->compiler_frame->last_block_level=0; /* all variables */
    else
      Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  {
    $<ptr>$ = Pike_compiler;
  }
  statements end_block
  {
    /* Recover compilation context on syntax errors. */
    while (Pike_compiler != $<ptr>5) {
      struct program *p;
      /* fprintf(stderr, "Compiler context out of sync. Attempting to recover...\n"); */
      if(Pike_compiler->compiler_pass != COMPILER_PASS_LAST)
	p = end_first_pass(0);
      else
	p=end_first_pass(1);

      if (p) free_program(p);
    }

    unuse_modules(Pike_compiler->num_used_modules - $<number>1);
    $6 = pop_local_variables($<number>2, $6);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
    if ($6) COPY_LINE_NUMBER_INFO($6, $3);
    free_node ($3);
    $$=$6;
  }
  ;

/* Node with line number info at $0. */
end_block: '}'
  | TOK_LEX_EOF
  {
    yyerror("Missing '}'.");
    if ($<n>0) {
      low_yyreport(REPORT_ERROR, $<n>0->current_file, $<n>0->line_number,
		   parser_system_string, 0, "Opening '{' was here.");
    }
    yyerror("Unexpected end of file.");
  }
  ;

failsafe_block: block
              | error { $$=0; }
              | TOK_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
              ;

/* Type at compiler_frame->current_type. */
local_name_list: new_local_name
  | local_name_list ',' new_local_name
    { $$ = mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $1), $3); }
  ;


constant_expr: safe_assignment_expr
  {
    /* Ugly hack to make sure that $1 is optimized */
    {
      int tmp = Pike_compiler->compiler_pass;
      $$ = mknode(F_COMMA_EXPR, $1, 0);
      optimize_node($$);
      Pike_compiler->compiler_pass = tmp;
    }

    if(!is_const($$)) {
      if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
	yyerror("Expected constant expression.");
      push_int(0);
    } else {
      ptrdiff_t tmp = eval_low($$, 1);
      if(tmp < 1)
      {
	if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
	  yyerror("Error evaluating constant expression.");
	push_int(0);
      } else {
        pop_n_elems((INT32)(tmp - 1));
      }
    }
    free_node($$);
    $$ = mkconstantsvaluenode(Pike_sp - 1);
    pop_stack();
  }
  ;

local_constant_name: TOK_IDENTIFIER '=' safe_assignment_expr
  {
    /* Ugly hack to make sure that $3 is optimized */
    {
      int tmp=Pike_compiler->compiler_pass;
      $3=mknode(F_COMMA_EXPR,$3,0);
      optimize_node($3);
      Pike_compiler->compiler_pass=tmp;
    }

    if(!is_const($3))
    {
      if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
	yyerror("Constant definition is not constant.");
    }else{
      ptrdiff_t tmp=eval_low($3,1);
      if(tmp < 1)
      {
	yyerror("Error in constant definition.");
      }else{
        pop_n_elems((INT32)(tmp - 1));
	if($3) free_node($3);
	$3=mksvaluenode(Pike_sp-1);
	pop_stack();
      }
    }
    low_add_local_name(NULL, $1->u.sval.u.string, NULL,
		       $3?$3:mknode(F_UNDEFINED, 0, 0), NULL);
    /* Note: Intentionally not marked as used. */
    free_node($1);
  }
  | bad_identifier '=' safe_assignment_expr { if ($3) free_node($3); }
  | error '=' safe_assignment_expr { if ($3) free_node($3); }
  ;

local_constant_list: local_constant_name
  | local_constant_list ',' local_constant_name
  ;

local_constant: TOK_CONSTANT local_constant_list ';'
  | TOK_CONSTANT error ';' { yyerrok; }
  | TOK_CONSTANT error TOK_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_CONSTANT error '}' { yyerror("Missing ';'."); }
  ;


statements: { $$=0; }
  | statements statement
  {
    $$ = mknode(F_COMMA_EXPR, $1, mkcastnode(void_type_string, $2));
  }
  ;

statement_with_semicolon: void_expr expected_semicolon
  ;

normal_label_statement: statement_with_semicolon
  | import { $$=0; }
  | cond
  | return
  | local_constant { $$=0; }
  | block
  | break expected_semicolon
  | continue expected_semicolon
  | error ';' { reset_type_stack(); $$=0; yyerrok; }
  | error TOK_LEX_EOF
  {
    reset_type_stack();
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
    $$=0;
  }
  | error '}'
  {
    reset_type_stack();
    yyerror("Missing ';'.");
/*    yychar = '}'; */	/* Put the '}' back on the input stream. */
    $$=0;
  }
  | ';' { $$=0; }
  ;

statement: normal_label_statement
  {
    Pike_compiler->compiler_frame->opt_flags &= ~OPT_CUSTOM_LABELS;
  }
  | while
  | do
  | for
  | foreach
  | switch
  | case
  | default
  | labeled_statement
  | simple_type2 local_function { $$=mkcastnode(void_type_string, $2); }
  | TOK_CONTINUE simple_type2 local_generator
  {
    /* NB: The alternative of prefixing the local_function rule above with
     *     an optional_continue causes lots of shift/reduce conflicts.
     *     Thus the separate rule for local_generators.
     */
    $$=mkcastnode(void_type_string, $3);
  }
  | implicit_modifiers named_class { $$=mkcastnode(void_type_string, $2); }
  ;

labeled_statement: TOK_IDENTIFIER
  {
    Pike_compiler->compiler_frame->opt_flags &= ~OPT_CUSTOM_LABELS;
  }
  ':' statement
  {
    $$ = mknode(Pike_compiler->compiler_frame->opt_flags & OPT_CUSTOM_LABELS ?
		F_CUSTOM_STMT_LABEL : F_NORMAL_STMT_LABEL,
		$1, $4);

    /* FIXME: This won't be correct if the node happens to be shared.
     * That's an issue to be solved with shared nodes in general,
     * though. */
    COPY_LINE_NUMBER_INFO($$, $1);
  }
  ;

optional_label: TOK_IDENTIFIER
  | /* empty */ {$$ = 0;}
  ;

break: TOK_BREAK optional_label { $$=mknode(F_BREAK,$2,0); } ;
default: TOK_DEFAULT ':'  { $$=mknode(F_DEFAULT,0,0); }
  | TOK_DEFAULT
  {
    $$=mknode(F_DEFAULT,0,0); yyerror("Expected ':' after default.");
  }
  ;

continue: TOK_CONTINUE optional_label { $$=mknode(F_CONTINUE,$2,0); } ;

/* This variant is used to push the compiler context for
 * functions without a declared return type (ie lambdas
 * or implicit lfun::__create__()).
 */
start_lambda: /* empty */
  {
    push_compiler_frame(SCOPE_LOCAL);

    debug_malloc_touch(Pike_compiler->compiler_frame->current_return_type);
    if(Pike_compiler->compiler_frame->current_return_type)
      free_type(Pike_compiler->compiler_frame->current_return_type);
    copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
		   any_type_string);

    $$ = Pike_compiler->compiler_frame;
  }
  ;

implicit_identifier: /* empty */
  {
    struct pike_string *name;
    $$=mkstrnode(name = get_new_name(NULL));
    free_string(name);
  }
  ;

lambda: TOK_LAMBDA line_number_info implicit_identifier start_lambda
  func_args
  {
    struct pike_string *name = $3->u.sval.u.string;
    struct pike_type *type;
    int e;

    $<number>$ = Pike_compiler->varargs;
    Pike_compiler->varargs = 0;

    if (Pike_compiler->compiler_pass == COMPILER_PASS_FIRST) {
      /* Define a tentative prototype for the lambda. */
      int ee;
      push_finished_type(mixed_type_string);
      e=$5-1;
      if($<number>$)
      {
        ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
        push_finished_type(Pike_compiler->compiler_frame->local_names[ee].def->type);
	e--;
	if (pop_type_stack(T_ARRAY)) {
	  compiler_discard_top_type();
	}
      }else{
	push_type(T_VOID);
      }
      Pike_compiler->varargs=0;
      push_type(T_MANY);
      for(; e>=0; e--) {
        struct local_name *var;
        ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
        var = Pike_compiler->compiler_frame->local_names + ee;
        push_finished_type(var->def->type);
        if (var->init && (var->def->token == F_LOCAL)) {
          push_type(T_VOID);
          push_type(T_OR);
        }
	push_type(T_FUNCTION);
      }
      type=compiler_pop_type();
      Pike_compiler->compiler_frame->current_function_number =
	define_function(name, type,
			ID_PROTECTED | ID_PRIVATE | ID_INLINE | ID_USED,
			IDENTIFIER_PIKE_FUNCTION, NULL,
			(unsigned INT16)
			(Pike_compiler->compiler_frame->opt_flags));
      free_type(type);
    } else {
      /* In pass 2 we just reuse the type from pass 1. */
      Pike_compiler->compiler_frame->current_function_number =
	isidentifier(name);
    }
  }
  failsafe_block
  {
    struct pike_type *type;
    int f, e, ee;
    struct pike_string *name;
    struct compilation *c = THIS_COMPILATION;
    struct pike_string *save_file = c->lex.current_file;
    int save_line = c->lex.current_line;
    c->lex.current_file = $2->current_file;
    c->lex.current_line = $2->line_number;

    debug_malloc_touch($7);
    $7=mknode(F_COMMA_EXPR,$7,mknode(F_RETURN,mkintnode(0),0));
    if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
      /* Doing this in pass 1 might induce too strict checks on types
       * in cases where we got placeholders. */
      type=find_return_type($7);
      if (type) {
	push_finished_type(type);
	free_type(type);
      } else {
	yywarning("Failed to determine return type for lambda.");
	push_type(T_ZERO);
      }
    } else {
      /* Tentative return type. */
      push_type(T_MIXED);
    }

    e=$5-1;
    if($<number>6)
    {
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      push_finished_type(Pike_compiler->compiler_frame->local_names[ee].def->type);
      e--;
      if (pop_type_stack(T_ARRAY)) {
	compiler_discard_top_type();
      }
    }else{
      push_type(T_VOID);
    }
    Pike_compiler->varargs=0;
    push_type(T_MANY);
    for(; e>=0; e--) {
      struct local_name *var;
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      var = Pike_compiler->compiler_frame->local_names + ee;
      push_finished_type(var->def->type);
      if (var->init && (var->def->token == F_LOCAL)) {
	push_type(T_VOID);
	push_type(T_OR);
      }
      push_type(T_FUNCTION);

      $7 = mknode(F_COMMA_EXPR, set_default_value(ee), $7);
    }

    type=compiler_pop_type();

    name = $3->u.sval.u.string;

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n%d:   type: ",
	    Pike_compiler->compiler_pass, name->str,
	    (long)Pike_compiler->new_program->id,
	    Pike_compiler->local_class_counter-1,
	    Pike_compiler->compiler_pass);
    simple_describe_type(type);
    fprintf(stderr, "\n");
#endif /* LAMBDA_DEBUG */

    f=dooptcode(name,
		$7,
		type,
		ID_PROTECTED | ID_PRIVATE | ID_INLINE | ID_USED);

#ifdef PIKE_DEBUG
    if (f != Pike_compiler->compiler_frame->current_function_number) {
      Pike_fatal("Lost track of lambda %s.\n", name->str);
    }
#endif /* PIKE_DEBUG */

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d:   lexical_scope: 0x%08x\n",
	    Pike_compiler->compiler_pass,
	    Pike_compiler->compiler_frame->lexical_scope);
#endif /* LAMBDA_DEBUG */

    if(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPED) {
      $$ = mktrampolinenode(f, Pike_compiler->compiler_frame->previous);
    } else {
      $$ = mkidentifiernode(f);
    }
    free_type(type);
    c->lex.current_line = save_line;
    c->lex.current_file = save_file;
    free_node($3);
    free_node ($2);
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $4) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $4,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
  }
  | TOK_LAMBDA line_number_info implicit_identifier start_lambda error
  {
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $4) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $4,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    $$ = mkintnode(0);
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node($3);
    free_node($2);
  }
  ;

local_function: TOK_IDENTIFIER start_function func_args
  {
    struct pike_string *name;
    struct pike_type *type;
    int id, e, ee;
    node *n;
    struct identifier *i=0;

    /***/
    push_finished_type(Pike_compiler->compiler_frame->current_return_type);

    e=$3-1;
    if(Pike_compiler->varargs)
    {
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      push_finished_type(Pike_compiler->compiler_frame->local_names[ee].def->type);
      e--;
      if (pop_type_stack(T_ARRAY)) {
	compiler_discard_top_type();
      }
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--) {
      struct local_name *var;
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      var = Pike_compiler->compiler_frame->local_names + ee;
      push_finished_type(var->def->type);
      if (var->init && (var->def->token == F_LOCAL)) {
	push_type(T_VOID);
	push_type(T_OR);
      }
      push_type(T_FUNCTION);
    }

    type=compiler_pop_type();
    /***/

    name = get_new_name($1->u.sval.u.string);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, name->str,
	    (long)Pike_compiler->new_program->id,
	    Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    if(Pike_compiler->compiler_pass > COMPILER_PASS_FIRST)
    {
      id=isidentifier(name);
    }else{
      id=define_function(name,
			 type,
			 ID_PROTECTED | ID_PRIVATE | ID_INLINE | ID_USED,
			 IDENTIFIER_PIKE_FUNCTION |
			 (Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
			 0,
			 OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
    }
    Pike_compiler->varargs=0;
    Pike_compiler->compiler_frame->current_function_number=id;
    free_type(type);

    n=0;
    i = ID_FROM_INT(Pike_compiler->new_program, id);
    if(i->identifier_flags & IDENTIFIER_SCOPED)
      n = mktrampolinenode(id, Pike_compiler->compiler_frame->previous);
    else
      n = mkidentifiernode(id);

    low_add_local_name(Pike_compiler->compiler_frame->previous,
		       $1->u.sval.u.string, NULL, n, NULL);

    $<number>$=id;
    free_string(name);
  }
  failsafe_block
  {
    int localid;
    struct identifier *i=ID_FROM_INT(Pike_compiler->new_program, $<number>4);
    struct compilation *c = THIS_COMPILATION;
    struct pike_string *save_file = c->lex.current_file;
    int save_line = c->lex.current_line;
    int e = $3 - 1;
    c->lex.current_file = $1->current_file;
    c->lex.current_line = $1->line_number;

    if (Pike_compiler->varargs) e--;
    for(; e>=0; e--) {
      int ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      $5 = mknode(F_COMMA_EXPR, set_default_value(ee), $5);
    }

    $5=mknode(F_COMMA_EXPR,$5,mknode(F_RETURN,mkintnode(0),0));

    debug_malloc_touch($5);
    dooptcode(i->name,
	      $5,
	      i->type,
	      ID_PROTECTED | ID_PRIVATE | ID_INLINE);

    i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

    c->lex.current_line = save_line;
    c->lex.current_file = save_file;
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $2) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $2,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    free_node($1);

    /* WARNING: If the local function adds more variables we are screwed */
    /* WARNING2: if add_local_name stops adding local variables at the end,
     *           this has to be fixed.
     */

    localid=Pike_compiler->compiler_frame->current_number_of_locals-1;
    if(Pike_compiler->compiler_frame->local_names[localid].def)
    {
      $$=copy_node(Pike_compiler->compiler_frame->local_names[localid].def);
    }else{
      if(Pike_compiler->compiler_frame->lexical_scope &
	 (SCOPE_SCOPE_USED | SCOPE_SCOPED))
      {
	$$ = mktrampolinenode($<number>4,Pike_compiler->compiler_frame);
      }else{
	$$ = mkidentifiernode($<number>4);
      }
    }
  }
  | TOK_IDENTIFIER start_function error
  {
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $2) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $2,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    $$=mkintnode(0);
  }
  ;

local_generator: TOK_IDENTIFIER start_function func_args
  {
    struct pike_string *name;
    struct pike_type *type;
    int id, e, ee;
    node *n;
    struct identifier *i=0;

    /***/
    push_finished_type(Pike_compiler->compiler_frame->current_return_type);

    /* Adjust the type to be a function that returns
     * a function(mixed|void, function(mixed...:void)|void:X).
     */
    push_type(T_VOID);
    push_type(T_MANY);

    push_type(T_VOID);
    push_type(T_VOID);
    push_type(T_VOID);
    push_type(T_MIXED);
    push_type(T_OR);
    push_type(T_MANY);
    push_type(T_OR);
    push_type(T_FUNCTION);

    push_type(T_VOID);
    push_type(T_MIXED);
    push_type(T_OR);
    push_type(T_FUNCTION);

    /* Entry point variable. */
    add_ref(int_type_string);
    MAKE_CONST_STRING(name, "__generator_entry_point__");
    Pike_compiler->compiler_frame->generator_local =
      add_local_name(name, int_type_string, 0);

    /* Stack contents to restore. */
    add_ref(array_type_string);
    MAKE_CONST_STRING(name, "__generator_stack__");
    add_local_name(name, array_type_string, 0);

    /* Resumption argument. */
    add_ref(mixed_type_string);
    MAKE_CONST_STRING(name, "__generator_argument__");
    add_local_name(name, mixed_type_string, 0);

    /* Resumption callback. */
    add_ref(function_type_string);
    MAKE_CONST_STRING(name, "__generator_callback__");
    add_local_name(name, function_type_string, 0);

    /* All of the above and the arguments are scoped. */
    for (e = 0; e < Pike_compiler->compiler_frame->generator_local + 4; e++) {
      /* Force local #e to be scoped. */
      node *n = mklocalnode(e, -1);
      if (e >= Pike_compiler->compiler_frame->generator_local) {
        /* Mark all generator locals as used. */
        mark_lvalue_as_used(n);
      }
      free_node(n);
    }

    e=$3-1;
    if(Pike_compiler->varargs)
    {
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      push_finished_type(Pike_compiler->compiler_frame->local_names[ee].def->type);
      e--;
      if (pop_type_stack(T_ARRAY)) {
	compiler_discard_top_type();
      }
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--) {
      struct local_name *var;
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      var = Pike_compiler->compiler_frame->local_names + ee;
      push_finished_type(var->def->type);
      if (var->init && (var->def->token == F_LOCAL)) {
	push_type(T_VOID);
	push_type(T_OR);
      }
      push_type(T_FUNCTION);
    }

    type=compiler_pop_type();
    /***/

    name = get_new_name($1->u.sval.u.string);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, name->str,
	    (long)Pike_compiler->new_program->id,
	    Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    if(Pike_compiler->compiler_pass > COMPILER_PASS_FIRST)
    {
      id=isidentifier(name);
    }else{
      id=define_function(name,
			 type,
			 ID_PROTECTED | ID_PRIVATE | ID_INLINE | ID_USED,
			 IDENTIFIER_PIKE_FUNCTION |
			 (Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
			 0,
			 OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
    }
    Pike_compiler->varargs=0;
    Pike_compiler->compiler_frame->current_function_number=id;
    free_type(type);

    n=0;
    i = ID_FROM_INT(Pike_compiler->new_program, id);
    if(i->identifier_flags & IDENTIFIER_SCOPED)
      n = mktrampolinenode(id, Pike_compiler->compiler_frame->previous);
    else
      n = mkidentifiernode(id);

    low_add_local_name(Pike_compiler->compiler_frame->previous,
		       $1->u.sval.u.string, NULL, n, NULL);

    $<number>$=id;
    free_string(name);
  }
  failsafe_block
  {
    int localid;
    struct identifier *i=ID_FROM_INT(Pike_compiler->new_program, $<number>4);
    struct compilation *c = THIS_COMPILATION;
    struct pike_string *save_file = c->lex.current_file;
    int save_line = c->lex.current_line;
    struct pike_type *generator_type;
    struct pike_string *name;
    int generator_stack_local;
    int f, ee;
    int e = $3 - 1;

    c->lex.current_file = $1->current_file;
    c->lex.current_line = $1->line_number;

    ref_push_string($1->u.sval.u.string);
    push_constant_text("\0generator");
    f_add(2);
    name = get_new_name(Pike_sp[-1].u.string);
    pop_stack();

    if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST &&
	Pike_compiler->compiler_frame->current_return_type->type == PIKE_T_AUTO) {
      /* Change "auto" return type to actual return type. */
      push_finished_type(Pike_compiler->compiler_frame->current_return_type->car);
    } else {
      push_finished_type(Pike_compiler->compiler_frame->current_return_type);
    }

    /* Adjust the type to be a function that returns
     * a function(mixed|void, function(mixed...:void)|void:X).
     */
    push_type(T_VOID);
    push_type(T_MANY);

    push_type(T_VOID);
    push_type(T_VOID);
    push_type(T_VOID);
    push_type(T_MIXED);
    push_type(T_OR);
    push_type(T_MANY);
    push_type(T_OR);
    push_type(T_FUNCTION);

    push_type(T_VOID);
    push_type(T_MIXED);
    push_type(T_OR);
    push_type(T_FUNCTION);

    generator_type = compiler_pop_type();
    f = dooptcode(name, $5, generator_type,
		  ID_INLINE | ID_PROTECTED | ID_PRIVATE | ID_USED);
    free_string(name);

    generator_stack_local =
      Pike_compiler->compiler_frame->generator_local + 1;
    Pike_compiler->compiler_frame->generator_local = -1;
    free_type(Pike_compiler->compiler_frame->current_return_type);
    Pike_compiler->compiler_frame->current_return_type = generator_type;
    $5 = mknode(F_COMMA_EXPR,
		mknode(F_ASSIGN, mklocalnode(generator_stack_local, 0),
		       mkefuncallnode("aggregate", NULL)),
		mknode(F_RETURN, mkgeneratornode(f), NULL));

    if(Pike_compiler->varargs) e--;
    for(; e>=0; e--) {
      ee = Pike_compiler->compiler_frame->local_variables[e] - 1;
      $5 = mknode(F_COMMA_EXPR, set_default_value(ee), $5);
    }

    debug_malloc_touch($5);
    f = dooptcode(i->name,
		  $5,
		  i->type,
		  ID_PROTECTED | ID_PRIVATE | ID_INLINE | ID_USED);

    i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

    c->lex.current_line = save_line;
    c->lex.current_file = save_file;
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $2) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $2,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    free_node($1);

    /* WARNING: If the local function adds more variables we are screwed */
    /* WARNING2: if add_local_name stops adding local variables at the end,
     *           this has to be fixed.
     */

    localid=Pike_compiler->compiler_frame->current_number_of_locals-1;
    if(Pike_compiler->compiler_frame->local_names[localid].def)
    {
      $$=copy_node(Pike_compiler->compiler_frame->local_names[localid].def);
    }else{
      if(Pike_compiler->compiler_frame->lexical_scope &
	 (SCOPE_SCOPE_USED | SCOPE_SCOPED))
      {
	$$ = mktrampolinenode(f, Pike_compiler->compiler_frame);
      }else{
	$$ = mkidentifiernode(f);
      }
    }
  }
  | TOK_IDENTIFIER start_function error
  {
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $2) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $2,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
    $$=mkintnode(0);
  }
  ;

create_arg: modifiers simple_type optional_dot_dot_dot TOK_IDENTIFIER
  optional_default_value
  {
    struct pike_type *type;
    int ref_no;
    int l_no;

    if (Pike_compiler->num_create_args < 0) {
      yyerror("Can't define more variables after ...");
    }

    if ($3) {
      push_finished_type(Pike_compiler->compiler_frame->current_type);
      push_unlimited_array_type(T_ARRAY);
      type = compiler_pop_type();
    } else {
      copy_pike_type(type, Pike_compiler->compiler_frame->current_type);
    }

    /* Add the identifier globally.
     * Note: Since these are the first identifiers (and references)
     *       to be added to the program, they will be numbered in
     *       sequence starting at 0 (zero). This means that the
     *       counter num_create_args is sufficient extra information
     *       to be able to keep track of them.
     */
    ref_no = define_variable($4->u.sval.u.string, type,
			     Pike_compiler->current_modifiers);

    if ((Pike_compiler->num_create_args != ref_no) &&
	(Pike_compiler->num_create_args != -ref_no)) {
      my_yyerror("Multiple definitions of create variable %pS (%d != %d).",
		 $4->u.sval.u.string,
		 Pike_compiler->num_create_args, ref_no);
    }
    if ($3) {
      /* Encode varargs marker as negative number of args. */
      Pike_compiler->num_create_args = -(ref_no + 1);

      if ($5) {
	yyerror("Varargs variable with default value.");
	free_node($5);
	$5 = NULL;
      }
    } else {
      Pike_compiler->num_create_args = ref_no + 1;
    }

    l_no = add_local_name($4->u.sval.u.string, type, $5);
    bind_local(NULL, l_no);
    Pike_compiler->compiler_frame->local_names[l_no].flags |=
      LOCAL_VAR_IS_ARGUMENT | LOCAL_VAR_IS_USED;

    if (ref_no != l_no) {
      my_yyerror("Variables out of sync for __create__().");
    }

    /* free_type(type); */
    free_node($4);
    $$=0;
  }
  | modifiers simple_type bad_identifier { $$=0; }
  ;

create_arguments2: create_arg { $$ = 1; }
  | create_arguments2 ',' create_arg { $$ = $1 + 1; }
  | create_arguments2 ':' create_arg
  {
    yyerror("Unexpected ':' in create argument list.");
    $$ = $1 + 1;
  }
  ;

create_arguments: /* empty */ optional_comma { $$=0; }
  | create_arguments2 optional_comma
  ;

optional_create_arguments: /* empty */ { $$ = 0; }
  | start_lambda '(' create_arguments ')'
  {
    node *n = NULL;
    int e = $3;
    struct pike_type *t;

    type_stack_mark();
    push_type(T_VOID);

    if (Pike_compiler->num_create_args < 0) {
      e--;
      push_finished_type(Pike_compiler->compiler_frame->local_names[e].def->type);
      if (pop_type_stack(T_ARRAY)) {
	compiler_discard_top_type();
      }

      if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
	/* FIXME: Should probably use some other flag. */
	if ((runtime_options & RUNTIME_CHECK_TYPES) &&
	    (Pike_compiler->compiler_frame->local_names[e].def->type !=
	     mixed_type_string)) {
	  node *local_node;

	  /* fprintf(stderr, "Creating soft cast node for local #%d\n", e);*/

	  local_node = mkcastnode(mixed_type_string, mklocalnode(e, 0));

	  /* NOTE: The cast to mixed above is needed to avoid generating
	   *       compilation errors, as well as avoiding optimizations
	   *       in mksoftcastnode().
	   */
	  n =
	    mknode(F_COMMA_EXPR, n,
		   mksoftcastnode(Pike_compiler->compiler_frame->local_names[e].def->type,
				  local_node));
	}
      }
      n =
	mknode(F_COMMA_EXPR, n,
	       mknode(F_POP_VALUE,
		      mknode(F_ASSIGN, mkidentifiernode(e),
			     mklocalnode(e, 0)), NULL));
    } else {
      push_type(T_VOID);
    }
    push_type(T_MANY);

    while (e-- > 0) {
      /* NB: Currently there is no need to go indirectly via local_variables,
       *     as all create_args have been bound directly (there is no support
       *     for having them bound via F_ARRAY_LVALUE yet).
       */
      struct local_name *var = Pike_compiler->compiler_frame->local_names + e;

      push_finished_type(var->def->type);

      n =
	mknode(F_COMMA_EXPR,
	       mknode(F_POP_VALUE,
		      mknode(F_ASSIGN, mkidentifiernode(e),
			     mklocalnode(e, 0)), NULL),
	       n);

      if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
	/* FIXME: Should probably use some other flag. */
	if ((runtime_options & RUNTIME_CHECK_TYPES) &&
            (var->def->type != mixed_type_string)) {
	  node *local_node;

	  /* fprintf(stderr, "Creating soft cast node for local #%d\n", e);*/

	  local_node = mkcastnode(mixed_type_string, mklocalnode(e, 0));

	  /* NOTE: The cast to mixed above is needed to avoid generating
	   *       compilation errors, as well as avoiding optimizations
	   *       in mksoftcastnode().
	   */
	  n =
	    mknode(F_COMMA_EXPR,
                   mksoftcastnode(var->def->type, local_node),
		   n);
	}
      }

      /* NB: var->def->token is currently always F_LOCAL, but forward
       *     compat is trivial here, so.
       */
      if (var->init && (var->def->token == F_LOCAL)) {
	push_type(T_VOID);
	push_type(T_OR);
      }
      push_type(T_FUNCTION);

      n = mknode(F_COMMA_EXPR, set_default_value(e), n);
    }

    n = mknode(F_COMMA_EXPR, n,
	       mknode(F_RETURN, mkintnode(0), NULL));

    t = pop_unfinished_type();

    define_function(lfun_strings[LFUN___CREATE__], t,
		    ID_PROTECTED|ID_LOCAL,
		    IDENTIFIER_PIKE_FUNCTION |
		    ((Pike_compiler->num_create_args < 0)?
		     IDENTIFIER_VARARGS:0),
		    0, OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

    dooptcode(lfun_strings[LFUN___CREATE__], n, t, ID_PROTECTED|ID_LOCAL);

    free_type(t);

#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $1) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $1,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();

    /* NOTE: One more than the number of arguments, so that we
     *       can detect the case of no parenthesis below when
     *       we define the user lfun::create() function.
     */
    $$ = $3 + 1;
  }
  ;

failsafe_program: '{' program { $<n>$ = NULL; } end_block
                | error { yyerrok; }
                | TOK_LEX_EOF
                {
		  yyerror("End of file where program definition expected.");
		}
                ;

/* Modifiers at $0. */
anon_class: TOK_CLASS line_number_info
  {
    struct pike_string *s;
    char buffer[42];
    sprintf(buffer,"__class_%ld_%ld_line_%d",
	    (long)Pike_compiler->new_program->id,
	    (long)Pike_compiler->local_class_counter++,
	    (int) $2->line_number);
    s = make_shared_string(buffer);
    $<n>$ = mkstrnode(s);
    free_string(s);
    $<number>0 |= ID_PROTECTED | ID_PRIVATE | ID_INLINE;
  }
  {
    /* fprintf(stderr, "LANGUAGE.YACC: ANON CLASS start\n"); */
    if(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)
    {
      if ($<number>0 & ID_EXTERN) {
	yywarning("Extern declared class definition.");
      }
      low_start_new_program(0, COMPILER_PASS_FIRST, $<n>3->u.sval.u.string,
			    $<number>0,
			    &$<number>$);

      /* fprintf(stderr, "Pass 1: Program %s has id %d\n",
	 $3->u.sval.u.string->str, Pike_compiler->new_program->id); */

      store_linenumber($2->line_number, $2->current_file);
      debug_malloc_name(Pike_compiler->new_program,
			$2->current_file->str,
			$2->line_number);
    }else{
      int i;
      struct identifier *id;
      int tmp=Pike_compiler->compiler_pass;
      i=isidentifier($<n>3->u.sval.u.string);
      if(i<0)
      {
	/* Seriously broken... */
	yyerror("Pass 2: program not defined!");
	low_start_new_program(0, COMPILER_PASS_LAST, 0,
			      $<number>0,
			      &$<number>$);
      }else{
	id=ID_FROM_INT(Pike_compiler->new_program, i);
	if(IDENTIFIER_IS_CONSTANT(id->identifier_flags))
	{
	  struct svalue *s;
	  if ((id->func.const_info.offset >= 0) &&
	      (TYPEOF(*(s = &PROG_FROM_INT(Pike_compiler->new_program,i)->
			constants[id->func.const_info.offset].sval)) ==
	       T_PROGRAM))
	  {
	    low_start_new_program(s->u.program, COMPILER_PASS_LAST,
				  $<n>3->u.sval.u.string,
				  $<number>0,
				  &$<number>$);

	    /* fprintf(stderr, "Pass 2: Program %s has id %d\n",
	       $3->u.sval.u.string->str, Pike_compiler->new_program->id); */

	  }else{
	    yyerror("Pass 2: constant redefined!");
	    low_start_new_program(0, COMPILER_PASS_LAST, 0,
				  $<number>0,
				  &$<number>$);
	  }
	}else{
	  yyerror("Pass 2: class constant no longer constant!");
	  low_start_new_program(0, COMPILER_PASS_LAST, 0,
				$<number>0,
				&$<number>$);
	}
      }
      Pike_compiler->compiler_pass=tmp;
    }
  }
  {
    /* Clear scoped modifiers. */
    $<number>$ = THIS_COMPILATION->lex.pragmas;
    THIS_COMPILATION->lex.pragmas &= ~ID_MODIFIER_MASK;
  }
  optional_create_arguments failsafe_program
  {
    struct program *p;

    /* Check if we have __create__() but no locally defined create(). */
    if ($6) {
      struct reference *ref = NULL;
      struct identifier *id = NULL;
      int ref_id;
      if (((ref_id = isidentifier(lfun_strings[LFUN_CREATE])) < 0) ||
	  (ref = PTR_FROM_INT(Pike_compiler->new_program, ref_id))->inherit_offset ||
	  ((id = ID_FROM_PTR(Pike_compiler->new_program, ref))->func.offset == -1)) {
	/* There is no create() or it is inherited or it is a prototype. */
	push_compiler_frame(SCOPE_LOCAL);

	ref_id = FIND_LFUN(Pike_compiler->new_program, LFUN___CREATE__);
	ref = PTR_FROM_INT(Pike_compiler->new_program, ref_id);
	id = ID_FROM_PTR(Pike_compiler->new_program, ref);

	ref_id = define_function(lfun_strings[LFUN_CREATE],
				 id->type, ref->id_flags,
				 id->identifier_flags, &id->func,
				 id->opt_flags);

	pop_compiler_frame();
      }
    }

    if(Pike_compiler->compiler_pass != COMPILER_PASS_LAST)
      p=end_first_pass(0);
    else
      p=end_first_pass(1);

    /* fprintf(stderr, "LANGUAGE.YACC: ANON CLASS end\n"); */

    if(p) {
      /* Update the type for the program constant,
       * since we might have a lfun::create(). */
      struct identifier *i;
      struct svalue sv;
      SET_SVAL(sv, T_PROGRAM, 0, program, p);
      i = ID_FROM_INT(Pike_compiler->new_program, $<number>4);
      free_type(i->type);
      i->type = get_type_of_svalue(&sv);
      free_program(p);
    } else if (!Pike_compiler->num_parse_error) {
      /* Make sure code in this class is aware that something went wrong. */
      Pike_compiler->num_parse_error = 1;
    }

    $$=mkidentifiernode($<number>4);

    free_node($2);
    free_node($<n>3);
    check_tree($$,0);
    THIS_COMPILATION->lex.pragmas = $<number>5;
  }
  ;

/* Modifiers at $0. */
named_class: TOK_CLASS line_number_info simple_identifier
  {
    if(!$3)
    {
      struct pike_string *s;
      char buffer[42];
      sprintf(buffer,"__class_%ld_%ld_line_%d",
	      (long)Pike_compiler->new_program->id,
	      (long)Pike_compiler->local_class_counter++,
	      (int) $2->line_number);
      s=make_shared_string(buffer);
      $3=mkstrnode(s);
      free_string(s);
      $<number>0|=ID_PROTECTED | ID_PRIVATE | ID_INLINE;
    }

    /* fprintf(stderr, "LANGUAGE.YACC: CLASS start\n"); */
    if(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)
    {
      if ($<number>0 & ID_EXTERN) {
	yywarning("Extern declared class definition.");
      }
      low_start_new_program(0, COMPILER_PASS_FIRST, $3->u.sval.u.string,
			    $<number>0,
			    &$<number>$);

      /* fprintf(stderr, "Pass 1: Program %s has id %d\n",
	 $3->u.sval.u.string->str, Pike_compiler->new_program->id); */

      store_linenumber($2->line_number, $2->current_file);
      debug_malloc_name(Pike_compiler->new_program,
			$2->current_file->str,
			$2->line_number);
    }else{
      int i;
      struct identifier *id;
      int tmp=Pike_compiler->compiler_pass;
      i=isidentifier($3->u.sval.u.string);
      if(i<0)
      {
	/* Seriously broken... */
	yyerror("Pass 2: program not defined!");
	low_start_new_program(0, COMPILER_PASS_LAST, 0,
			      $<number>0,
			      &$<number>$);
      }else{
	id=ID_FROM_INT(Pike_compiler->new_program, i);
	if(IDENTIFIER_IS_CONSTANT(id->identifier_flags))
	{
	  struct svalue *s;
	  if ((id->func.const_info.offset >= 0) &&
	      (TYPEOF(*(s = &PROG_FROM_INT(Pike_compiler->new_program,i)->
			constants[id->func.const_info.offset].sval)) ==
	       T_PROGRAM))
	  {
	    low_start_new_program(s->u.program, COMPILER_PASS_LAST,
				  $3->u.sval.u.string,
				  $<number>0,
				  &$<number>$);

	    /* fprintf(stderr, "Pass 2: Program %s has id %d\n",
	       $3->u.sval.u.string->str, Pike_compiler->new_program->id); */

	  }else{
	    yyerror("Pass 2: constant redefined!");
	    low_start_new_program(0, COMPILER_PASS_LAST, 0,
				  $<number>0,
				  &$<number>$);
	  }
	}else{
	  yyerror("Pass 2: class constant no longer constant!");
	  low_start_new_program(0, COMPILER_PASS_LAST, 0,
				$<number>0,
				&$<number>$);
	}
      }
      Pike_compiler->compiler_pass=tmp;
    }
  }
  {
    /* Clear scoped modifiers. */
    $<number>$ = THIS_COMPILATION->lex.pragmas;
    THIS_COMPILATION->lex.pragmas &= ~ID_MODIFIER_MASK;
  }
  optional_create_arguments failsafe_program
  {
    struct program *p;

    /* Check if we have __create__() but no locally defined create(). */
    if ($6) {
      struct reference *ref = NULL;
      struct identifier *id = NULL;
      int ref_id;
      if (((ref_id = isidentifier(lfun_strings[LFUN_CREATE])) < 0) ||
	  (ref = PTR_FROM_INT(Pike_compiler->new_program, ref_id))->inherit_offset ||
	  ((id = ID_FROM_PTR(Pike_compiler->new_program, ref))->func.offset == -1)) {
	/* There is no create() or it is inherited or it is a prototype. */
	push_compiler_frame(SCOPE_LOCAL);

	ref_id = FIND_LFUN(Pike_compiler->new_program, LFUN___CREATE__);
	ref = PTR_FROM_INT(Pike_compiler->new_program, ref_id);
	id = ID_FROM_PTR(Pike_compiler->new_program, ref);

	ref_id = define_function(lfun_strings[LFUN_CREATE],
				 id->type, ref->id_flags,
				 id->identifier_flags, &id->func,
				 id->opt_flags);

	pop_compiler_frame();
      }
    }

    /* Update the type for the program constant,
     * since we may have a lfun::create().
     *
     * Do this before end_first_pass(), to keep
     * override_identifier() et al happy.
     */
    {
      struct identifier *i;
      struct svalue sv;
      SET_SVAL(sv, T_PROGRAM, 0, program, Pike_compiler->new_program);
      i = ID_FROM_INT(Pike_compiler->previous->new_program, $<number>4);
      free_type(i->type);
      i->type = get_type_of_svalue(&sv);
      if (Pike_compiler->new_program->flags & PROGRAM_CONSTANT) {
	/* Update, in case of @constant. */
	i->opt_flags = 0;
      }
    }

    if(Pike_compiler->compiler_pass != COMPILER_PASS_LAST)
      p=end_first_pass(0);
    else
      p=end_first_pass(1);

    /* fprintf(stderr, "LANGUAGE.YACC: CLASS end\n"); */

    if(p) {
      free_program(p);
    } else if (!Pike_compiler->num_parse_error) {
      /* Make sure code in this class is aware that something went wrong. */
      Pike_compiler->num_parse_error = 1;
    }

    $$=mkidentifiernode($<number>4);

    free_node($2);
    free_node($3);
    check_tree($$,0);
    THIS_COMPILATION->lex.pragmas = $<number>5;
  }
  ;

simple_identifier: TOK_IDENTIFIER
  | bad_identifier { $$ = 0; }
  ;

enum_value: /* EMPTY */ { $$ = 0; }
  | '=' safe_assignment_expr { $$ = $2; }
  ;

/* Previous enum value at $0. */
enum_def: /* EMPTY */
  | simple_identifier enum_value
  {
    if ($1) {
      if ($2) {
	/* Explicit enum value. */

	/* This can be made more lenient in the future */

	/* Ugly hack to make sure that $2 is optimized */
	{
	  int tmp=Pike_compiler->compiler_pass;
	  $2=mknode(F_COMMA_EXPR,$2,0);
	  Pike_compiler->compiler_pass=tmp;
	}

	if(!is_const($2))
	{
	  if(Pike_compiler->compiler_pass == COMPILER_PASS_LAST)
	    yyerror("Enum definition is not constant.");
	  push_int(0);
	} else {
	  if(!Pike_compiler->num_parse_error)
	  {
	    ptrdiff_t tmp=eval_low($2,1);
	    if(tmp < 1)
	    {
	      yyerror("Error in enum definition.");
	      push_int(0);
	    }else{
              pop_n_elems((INT32)(tmp - 1));
	    }
	  } else {
	    push_int(0);
	  }
	}
	free_node($2);
	free_node($<n>0);
	$<n>0 = mkconstantsvaluenode(Pike_sp-1);
      } else {
	/* Implicit enum value. */
	$<n>0 = safe_inc_enum($<n>0);
	push_svalue(&$<n>0->u.sval);
      }
      add_constant($1->u.sval.u.string, Pike_sp-1,
		   (Pike_compiler->current_modifiers & ~ID_EXTERN) | ID_INLINE);
      /* Update the type. */
      {
	struct pike_type *current = pop_unfinished_type();
	struct pike_type *new = get_type_of_svalue(Pike_sp-1);
	struct pike_type *res = or_pike_types(new, current, 2);
	free_type(current);
	free_type(new);
	type_stack_mark();
	push_finished_type(res);
	free_type(res);
      }
      pop_stack();
      free_node($1);
    } else if ($2) {
      free_node($2);
    }
  }
  ;

/* Previous enum value at $-2 */
propagated_enum_value:
  {
    $$ = $<n>-2;
  }
  ;

/* Previous enum value at $0. */
enum_list: enum_def
  | enum_list ',' propagated_enum_value enum_def { $<n>0 = $3; }
  | error
  ;

/* Modifiers at $0. */
enum: TOK_ENUM
  {
    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)) {
      yywarning("Extern declared enum.");
    }

    type_stack_mark();
    push_type(T_ZERO);	/* Joined type so far. */
  }
  optional_identifier '{'
  {
    push_int(-1);	/* Previous value. */
    $<n>$ = mkconstantsvaluenode(Pike_sp-1);
    pop_stack();
  }
  enum_list { $<n>$ = NULL; } end_block
  {
    struct pike_type *t = pop_unfinished_type();
    free_node($<n>5);
    if ($3) {
      ref_push_type_value(t);
      add_constant($3->u.sval.u.string, Pike_sp-1,
		   (Pike_compiler->current_modifiers & ~ID_EXTERN) | ID_INLINE);
      pop_stack();
      free_node($3);
    }
    $$ = mktypenode(t);
    free_type(t);
  }
  ;

typedef: modifiers TOK_TYPEDEF full_type simple_identifier ';'
  {
    struct pike_type *t = compiler_pop_type();

    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == COMPILER_PASS_FIRST)) {
      yywarning("Extern declared typedef.");
    }

    if ($4) {
      ref_push_type_value(t);
      add_constant($4->u.sval.u.string, Pike_sp-1,
		   (Pike_compiler->current_modifiers & ~ID_EXTERN) | ID_INLINE);
      pop_stack();
      free_node($4);
    }
    free_type(t);
  }
  ;

save_locals: /* empty */
  {
    Pike_compiler->compiler_frame->last_block_level =
      $$ = Pike_compiler->compiler_frame->current_number_of_locals;
  }
  ;

save_block_level: /* empty */
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $$ = Pike_compiler->compiler_frame->last_block_level;
  }
  ;

cond: TOK_IF save_block_level save_locals line_number_info
  '(' safe_init_expr end_cond statement optional_else_part
  {
    $$ = mknode('?', $6,
		mknode(':',
		       mkcastnode(void_type_string, $8),
		       mkcastnode(void_type_string, $9)));
    COPY_LINE_NUMBER_INFO($$, $4);
    $$ = mkcastnode(void_type_string, $$);
    COPY_LINE_NUMBER_INFO($$, $4);
    free_node($4);
    $$ = pop_local_variables($3, $$);
    Pike_compiler->compiler_frame->last_block_level = $2;
  }
  ;

end_cond: ')'
  | '}' { yyerror("Missing ')'."); }
  | TOK_LEX_EOF
  {
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  ;

optional_else_part: { $$=0; }
  | TOK_ELSE statement { $$=$2; }
  ;

safe_lvalue: lvalue
  {
    if (!(THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES) && $1) {
      mark_lvalue_as_used($1);
    }
  }
  | error { $$=0; }
  ;

safe_assignment_expr: assignment_expr
  | TOK_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
  | error { $$=0; }
  ;


foreach_optional_lvalue: /* empty */ { $$=0; }
   | safe_lvalue
   ;

foreach_lvalues:  ',' safe_lvalue { $$=$2; }
  | ';' foreach_optional_lvalue ';' foreach_optional_lvalue
  { $$=mknode(':',$2,$4); }
  ;

foreach: TOK_FOREACH save_block_level save_locals line_number_info
  '(' assignment_expr foreach_lvalues end_cond
  {
  }
  statement
  {
    if ($7) {
      $$=mknode(F_FOREACH,
		mknode(F_FOREACH_VAL_LVAL,$6,$7),
		$10);
    } else {
      /* Error in lvalue */
      $$=mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $6), $10);
    }
    COPY_LINE_NUMBER_INFO($$, $4);
    free_node($4);
    $$ = pop_local_variables($3, $$);
    Pike_compiler->compiler_frame->last_block_level = $2;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;

do: TOK_DO line_number_info statement
  TOK_WHILE '(' safe_init_expr end_cond expected_semicolon
  {
    $$=mknode(F_DO,$6,$3);
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node ($2);
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  | TOK_DO line_number_info statement TOK_WHILE TOK_LEX_EOF
  {
    free_node ($2);
    $$=0;
    yyerror("Missing '(' in do-while loop.");
    yyerror("Unexpected end of file.");
  }
  | TOK_DO line_number_info statement TOK_LEX_EOF
  {
    free_node ($2);
    $$=0;
    yyerror("Missing 'while' in do-while loop.");
    yyerror("Unexpected end of file.");
  }
  ;

expected_semicolon: ';'
  | TOK_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  ;

for: TOK_FOR save_block_level save_locals line_number_info
  '(' optional_void_expr expected_semicolon for_expr expected_semicolon optional_void_expr end_cond
  statement
  {
    $$=mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $6),
	      mknode(F_FOR,$8,mknode(':',$12,$10)));
    COPY_LINE_NUMBER_INFO($$, $4);
    free_node($4);
    $$ = pop_local_variables($3, $$);
    Pike_compiler->compiler_frame->last_block_level = $2;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;

while: TOK_WHILE save_block_level save_locals line_number_info
  '(' safe_init_expr end_cond statement
  {
    $$=mknode(F_FOR,$6,mknode(':',$8,NULL));
    COPY_LINE_NUMBER_INFO($$, $4);
    free_node($4);
    $$ = pop_local_variables($3, $$);
    Pike_compiler->compiler_frame->last_block_level = $2;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;

for_expr: /* EMPTY */ { $$=mkintnode(1); }
  | safe_init_expr
  ;

switch: TOK_SWITCH save_block_level save_locals line_number_info
  '(' safe_init_expr end_cond statement
  {
    $$=mknode(F_SWITCH,$6,$8);
    COPY_LINE_NUMBER_INFO($$, $4);
    free_node($4);
    $$ = pop_local_variables($3, $$);
    Pike_compiler->compiler_frame->last_block_level = $2;
  }
  ;

case: TOK_CASE safe_init_expr expected_colon
  {
    $$=mknode(F_CASE,$2,0);
  }
  | TOK_CASE safe_init_expr expected_dot_dot optional_init_expr expected_colon
  {
     $$=mknode(F_CASE_RANGE,$2,$4);
  }
  | TOK_CASE expected_dot_dot safe_init_expr expected_colon
  {
     $$=mknode(F_CASE_RANGE,0,$3);
  }
  ;

expected_colon: ':'
  | ';'
  {
    yyerror("Missing ':'.");
  }
  | '}'
  {
    yyerror("Missing ':'.");
  }
  | TOK_LEX_EOF
  {
    yyerror("Missing ':'.");
    yyerror("Unexpected end of file.");
  }
  ;

optional_continue: { $$ = 0; }
  | TOK_CONTINUE { $$ = 1; }
  | TOK_BREAK { $$ = 0; }
  ;

return: optional_continue TOK_RETURN expected_semicolon
  {
    if(!match_types(Pike_compiler->compiler_frame->current_return_type,
		    void_type_string))
    {
      yytype_error("Must return a value for a non-void function.",
		   Pike_compiler->compiler_frame->current_return_type,
		   void_type_string, 0);
    }
    $$ = mknode(F_RETURN, mkintnode(0), mkintnode($1));
  }
  | optional_continue TOK_RETURN safe_init_expr expected_semicolon
  {
    if (!$3 ||
	!($3->tree_info & (OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT|OPT_APPLY)) ||
	check_tailrecursion()) {
      $$ = mknode(F_RETURN, $3, mkintnode($1));
    } else {
      /* The expression might be doing something that depends on
       * us holding eg a Thread.MutexKey.
       */
      $$ = mknode(F_VOLATILE_RETURN, $3, mkintnode($1));
    }
  }
  ;


splice_expr: assignment_expr
      | '@' assignment_expr { $$=mknode(F_PUSH_ARRAY,$2,0); };

optional_comma: { $$=0; } | ',' { $$=1; };

expr_list: { $$=0; }
  | expr_list2 optional_comma
  ;


expr_list2: splice_expr
  | expr_list2 ',' splice_expr { $$=mknode(F_ARG_LIST,$1,$3); }
  ;

m_expr_list: { $$=0; }
  | m_expr_list2 optional_comma
  ;

m_expr_list2: assoc_pair
  | m_expr_list2 ',' assoc_pair
  {
    if ($3) {
      $$=mknode(F_ARG_LIST,$1,$3);
    } else {
      /* Error in assoc_pair */
      $$=$1;
    }
  }
  | m_expr_list2 ',' error
  ;

assoc_pair:  assignment_expr expected_colon assignment_expr
  {
    $$=mknode(F_ARG_LIST,$1,$3);
  }
  | assignment_expr expected_colon error { free_node($1); $$=0; }
  ;

literal_expr: string
  | TOK_NUMBER
  | TOK_FLOAT { $$=mkfloatnode((FLOAT_TYPE)$1); }
  | open_paren_with_line_info '{' expr_list close_brace_or_missing ')'
    {
      /* FIXME: May eat lots of stack; cf Standards.FIPS10_4.divisions */
      $$=mkefuncallnode("aggregate",$3);
      COPY_LINE_NUMBER_INFO($$, $1);
      free_node ($1);
    }
  | open_paren_with_line_info
    open_bracket_with_line_info	/* Only to avoid shift/reduce conflicts. */
    m_expr_list close_bracket_or_missing ')'
    {
      /* FIXME: May eat lots of stack; cf Standards.FIPS10_4.divisions */
      $$=mkefuncallnode("aggregate_mapping",$3);
      COPY_LINE_NUMBER_INFO($$, $1);
      free_node ($1);
      free_node ($2);
    }
  | TOK_MULTISET_START line_number_info expr_list TOK_MULTISET_END
    {
      /* FIXME: May eat lots of stack; cf Standards.FIPS10_4.divisions */
      $$=mkefuncallnode("aggregate_multiset",$3);
      COPY_LINE_NUMBER_INFO($$, $2);
      free_node ($2);
    }
  | TOK_MULTISET_START line_number_info expr_list ')'
    {
      yyerror("Missing '>'.");
      $$=mkefuncallnode("aggregate_multiset",$3);
      COPY_LINE_NUMBER_INFO($$, $2);
      free_node ($2);
    }
  | TOK_MULTISET_START line_number_info error TOK_MULTISET_END { $$=$2; yyerrok; }
  | TOK_MULTISET_START line_number_info error ')' {
    yyerror("Missing '>'.");
    $$=$2; yyerrok;
  }
  | TOK_MULTISET_START line_number_info error TOK_LEX_EOF
  {
    $$=$2; yyerror("Missing '>)'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_MULTISET_START line_number_info error ';' { $$=$2; yyerror("Missing '>)'."); }
  | TOK_MULTISET_START line_number_info error '}' { $$=$2; yyerror("Missing '>)'."); }
  ;

unqualified_id_expr: low_id_expr
  | unqualified_id_expr '.' TOK_IDENTIFIER
  {
    $$ = index_node($1, $3->u.sval.u.string);
    free_node($1);
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);
    free_node($3);
  }
  | unqualified_id_expr '.' bad_identifier {}
  ;

qualified_ident:
  TOK_PREDEF TOK_COLON_COLON TOK_IDENTIFIER
  {
    struct compilation *c = THIS_COMPILATION;
    node *tmp2;

    if(Pike_compiler->last_identifier)
      free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);

    tmp2 = mkconstantsvaluenode(&c->default_module);
    set_node_name(tmp2, predef_scope_string);

    $$ = index_node(tmp2, $3->u.sval.u.string);
    if(!$$->name)
      add_ref( $$->name=$3->u.sval.u.string );
    free_node(tmp2);
    free_node($3);
  }
  | TOK_PREDEF TOK_COLON_COLON bad_identifier
  {
    $$=0;
  }
  | TOK_VERSION TOK_COLON_COLON TOK_IDENTIFIER
  {
    $$ = find_versioned_identifier($3->u.sval.u.string,
                                   $1->u.integer.a, $1->u.integer.b);
    free_node($1);
    free_node($3);
  }
  | TOK_VERSION TOK_COLON_COLON bad_identifier
  {
    free_node($1);
    $$=0;
  }
  | inherit_specifier TOK_IDENTIFIER
  {
    int id;

    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $2->u.sval.u.string);

    $$ = find_inherited_identifier(inherit_state, inherit_depth, $1,
                                   Pike_compiler->last_identifier);
    if (!$$) {
      if ((Pike_compiler->flags & COMPILATION_FORCE_RESOLVE) ||
          (Pike_compiler->compiler_pass == COMPILER_PASS_LAST)) {
        if (($1 >= 0) && inherit_state->new_program &&
            inherit_state->new_program->inherits[$1].name) {
          my_yyerror("Undefined identifier %pS::%pS.",
                     inherit_state->new_program->inherits[$1].name,
                     Pike_compiler->last_identifier);
        } else {
          my_yyerror("Undefined identifier %pS.",
                     Pike_compiler->last_identifier);
        }
        $$=0;
      }
      else
        $$=mknode(F_UNDEFINED,0,0);
    }

    free_node($2);
  }
  | inherit_specifier bad_identifier { $$=0; }
  | inherit_specifier error { $$=0; }
  | TOK_COLON_COLON TOK_IDENTIFIER
  {
    if(Pike_compiler->last_identifier) {
      free_string(Pike_compiler->last_identifier);
    }
    copy_shared_string(Pike_compiler->last_identifier, $2->u.sval.u.string);

    $$ = find_inherited_identifier(Pike_compiler, 0, INHERIT_ALL,
                                   Pike_compiler->last_identifier);
    if(!$$)
    {
      if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
        my_yyerror("Undefined identifier ::%pS.", $2->u.sval.u.string);
      }
      $$=mkintnode(0);
    }
    free_node($2);
  }
  | TOK_COLON_COLON bad_identifier
  {
    $$=0;
  }
  ;

qualified_id_expr: qualified_ident
  | qualified_id_expr '.' TOK_IDENTIFIER
  {
    $$ = index_node($1, $3->u.sval.u.string);
    free_node($1);
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);
    free_node($3);
  }
  | qualified_id_expr '.' bad_identifier {}
  ;

id_expr: unqualified_id_expr
  | qualified_id_expr
  ;

primary_expr: literal_expr
  | catch
  | gauge
  | typeof
  | sscanf
  | static_assertion { $$ = mknewintnode(0); }
  | lambda
  | implicit_modifiers anon_class { $$ = $2; }
  | implicit_modifiers enum { $$ = $2; }
  | apply
  | primary_expr '.' line_number_info TOK_IDENTIFIER
  {
    $$ = index_node($1, $4->u.sval.u.string);
    COPY_LINE_NUMBER_INFO($$, $3);
    free_node ($1);
    free_node ($3);
    free_node ($4);
  }
  | postfix_expr open_bracket_with_line_info '*' ']'
  {
    $$=mknode(F_AUTO_MAP_MARKER, $1, 0);
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node ($2);
  }
  | postfix_expr open_bracket_with_line_info assignment_expr ']'
  {
    $$=mknode(F_INDEX,$1,$3);
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node ($2);
  }
  | postfix_expr open_bracket_with_line_info
    range_bound expected_dot_dot range_bound ']'
  {
    $$=mknode(F_RANGE,$1,mknode(':',$3,$5));
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node ($2);
  }
  | postfix_expr TOK_SAFE_START_INDEX line_number_info assignment_expr ']'
  {
    /* A[?X] to ((tmp=A) && tmp[X]) */
    if( $1 && ($1->token == F_LOCAL) )
    {
      $$=mknode(F_LAND, copy_node($1), mknode(F_INDEX,  $1, $4));
    }
    else
    {
      fix_type_field( $1 );
      if( $1 && $1->type )
      {
        int temporary;
        $1->type->refs++;

        temporary = add_local_name(empty_pike_string, $1->type, 0);
        Pike_compiler->compiler_frame->local_names[temporary].flags |= LOCAL_VAR_IS_USED;
        $$=mknode(F_LAND,
                  mknode(F_ASSIGN, mklocalnode(temporary,0), $1),
                  mknode(F_INDEX,  mklocalnode(temporary,0), $4));
        $$ = pop_local_variables(temporary, $$);
      }
      else
      {
        $$=mknode(F_INDEX, $1,$4);
        yyerror("Indexing unexpected value.");
      }
    }
    COPY_LINE_NUMBER_INFO($$, $3);
    free_node ($3);
  }
  | postfix_expr TOK_SAFE_START_INDEX  line_number_info
    range_bound expected_dot_dot range_bound ']'
  {
    /* A[?X..Y] to ((tmp=A) && tmp[X..Y]) */
    node *range = mknode(':',$4,$6);
    if( $1 && ($1->token == F_LOCAL ) )
    {
      $$ = mknode( F_LAND, copy_node($1), mknode(F_RANGE, $1, range) );
    }
    else
    {
      fix_type_field( $1 );
      if( $1 && $1->type )
      {
        int temporary;
        $1->type->refs++;

        temporary = add_local_name(empty_pike_string, $1->type, 0);
        Pike_compiler->compiler_frame->local_names[temporary].flags |= LOCAL_VAR_IS_USED;
        $$=mknode(F_LAND,
                  mknode(F_ASSIGN, mklocalnode(temporary,0), $1),
                  mknode(F_RANGE,  mklocalnode(temporary,0), range) );
        $$ = pop_local_variables(temporary, $$);
      }
      else
      {
        $$ = mknode( F_LAND, $1, mknode(F_RANGE,$1,range) );
        yyerror("Indexing unexpected value.");
      }
    }
    COPY_LINE_NUMBER_INFO($$, $3);
    free_node ($3);
  }
  | postfix_expr open_bracket_with_line_info error ']'
  {
    $$=$1;
    free_node ($2);
    yyerrok;
  }
  | postfix_expr open_bracket_with_line_info error TOK_LEX_EOF
  {
    $$=$1; yyerror("Missing ']'.");
    yyerror("Unexpected end of file.");
    free_node ($2);
  }
  | postfix_expr open_bracket_with_line_info error ';'
    {$$=$1; yyerror("Missing ']'."); free_node ($2);}
  | postfix_expr open_bracket_with_line_info error '}'
    {$$=$1; yyerror("Missing ']'."); free_node ($2);}
  | postfix_expr open_bracket_with_line_info error ')'
    {$$=$1; yyerror("Missing ']'."); free_node ($2);}
  | open_paren_with_line_info comma_expr ')'
    {
      $$=$2;
      if ($$) {
        COPY_LINE_NUMBER_INFO($$, $1);
      }
      free_node ($1);
    }
  | open_paren_with_line_info error ')' { $$=$1; yyerrok; }
  | open_paren_with_line_info error TOK_LEX_EOF
  {
    $$=$1; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | open_paren_with_line_info error ';' { $$=$1; yyerror("Missing ')'."); }
  | open_paren_with_line_info error '}' { $$=$1; yyerror("Missing ')'."); }
  | postfix_expr TOK_ARROW line_number_info magic_identifier
  {
    $$=mknode(F_ARROW,$1,$4);
    COPY_LINE_NUMBER_INFO($$, $3);
    free_node ($3);
  }
  | postfix_expr TOK_SAFE_INDEX line_number_info TOK_IDENTIFIER
  {
    /* A->?B to ((tmp=A) && tmp->B) */
    int temporary;
    if( $1 && ($1->token == F_LOCAL) )
    {
      $$=mknode(F_LAND, copy_node($1), mknode(F_ARROW, $1, $4));
    }
    else
    {
      fix_type_field( $1 );
      if( $1 && $1->type )
      {
        add_ref($1->type);
        temporary = add_local_name(empty_pike_string, $1->type, 0);
        Pike_compiler->compiler_frame->local_names[temporary].flags |=
          LOCAL_VAR_IS_USED;
        $$=mknode(F_LAND,
                  mknode(F_ASSIGN, mklocalnode(temporary,0), $1),
                  mknode(F_ARROW,  mklocalnode(temporary,0), $4));
        $$ = pop_local_variables(temporary, $$);
      }
      else
      {
        $$=mknode(F_ARROW, $1,$4);
        yyerror("Indexing unexpected value.");
      }
    }
    COPY_LINE_NUMBER_INFO($$, $3);
    free_node ($3);
  }
  | postfix_expr TOK_ARROW line_number_info error {$$=$1; free_node ($3);}
  ;

postfix_expr: id_expr | primary_expr
  | bad_expr_ident
  {
    $$ = mknewintnode(0);
  }
  | postfix_expr TOK_INC { $$ = mknode(F_POST_INC, $1, mkintnode(1)); }
  | postfix_expr TOK_DEC { $$ = mknode(F_POST_DEC, $1, mkintnode(1)); }
  ;

pow_expr: postfix_expr
  | postfix_expr TOK_POW pow_expr { $$=mkopernode("`**",$1,$3); }
  | postfix_expr TOK_POW error
  ;

unary_expr: pow_expr
  | TOK_INC unary_expr { $$ = mknode(F_INC, $2, mkintnode(1)); }
  | TOK_DEC unary_expr { $$ = mknode(F_DEC, $2, mkintnode(1)); }
  | TOK_NOT cast_expr { $$ = mkopernode("`!", $2, 0); }
  | '+' cast_expr { $$ = $2; }
  | '~' cast_expr
  {
    if ($2 && ($2->token == F_CONSTANT) && (TYPEOF($2->u.sval) == T_INT)) {
      $$ = mkintnode(~($2->u.sval.u.integer));
      free_node($2);
    } else {
      $$ = mkopernode("`~", $2, 0);
    }
  }
  | '-' cast_expr
  {
    if ($2 && ($2->token == F_CONSTANT) && (TYPEOF($2->u.sval) == T_INT) &&
        !INT_TYPE_NEG_OVERFLOW($2->u.sval.u.integer)) {
      $$ = mkintnode(-($2->u.sval.u.integer));
      free_node($2);
    } else {
      $$=mkopernode("`-", $2, 0);
    }
  }
  ;

cast_expr: unary_expr
  | cast cast_expr
  {
    $$ = mkcastnode($1->u.sval.u.type, $2);
    free_node($1);
  }
  | soft_cast cast_expr
  {
    $$ = mksoftcastnode($1->u.sval.u.type, $2);
    free_node($1);
  }
  ;

mul_expr: cast_expr
  | mul_expr '*' cast_expr { $$=mkopernode("`*",$1,$3); }
  | mul_expr '%' cast_expr { $$=mkopernode("`%",$1,$3); }
  | mul_expr '/' cast_expr { $$=mkopernode("`/",$1,$3); }
  | mul_expr '*' error
  | mul_expr '%' error
  | mul_expr '/' error
  ;

add_expr: mul_expr
  | add_expr '+' mul_expr { $$ = mkopernode("`+", $1, $3); }
  | add_expr '-' mul_expr { $$ = mkopernode("`-", $1, $3); }
  | add_expr '+' error
  | add_expr '-' error
  ;

shift_expr: add_expr
  | shift_expr TOK_LSH add_expr { $$ = mkopernode("`<<", $1, $3); }
  | shift_expr TOK_RSH add_expr { $$ = mkopernode("`>>", $1, $3); }
  | shift_expr TOK_LSH error
  | shift_expr TOK_RSH error
  ;

rel_expr: shift_expr
  | rel_expr '>' shift_expr    { $$ = mkopernode("`>", $1, $3); }
  | rel_expr TOK_GE shift_expr { $$ = mkopernode("`>=", $1, $3); }
  | rel_expr '<' shift_expr    { $$ = mkopernode("`<", $1, $3); }
  | rel_expr TOK_LE shift_expr { $$ = mkopernode("`<=", $1, $3); }
  | rel_expr '>' error
  | rel_expr TOK_GE error
  | rel_expr '<' error
  | rel_expr TOK_LE error
  ;

eq_expr: rel_expr
  | eq_expr TOK_EQ rel_expr { $$ = mkopernode("`==", $1, $3); }
  | eq_expr TOK_NE rel_expr { $$ = mkopernode("`!=", $1, $3); }
  | eq_expr TOK_EQ error
  | eq_expr TOK_NE error
  ;

and_expr: eq_expr
  | and_expr '&' eq_expr { $$ = mkopernode("`&",$1,$3); }
  | and_expr '&' error
  ;

xor_expr: and_expr
  | xor_expr '^' and_expr { $$ = mkopernode("`^", $1, $3); }
  | xor_expr '^' error
  ;

or_expr: xor_expr
  | or_expr '|' xor_expr { $$ = mkopernode("`|", $1, $3); }
  | or_expr '|' error
  ;

land_expr: or_expr
  | land_expr TOK_LAND or_expr { $$ = mknode(F_LAND, $1, $3); }
  | land_expr TOK_LAND error
  ;

lor_expr: land_expr
  | lor_expr TOK_LOR land_expr { $$ = mknode(F_LOR,$1,$3); }
  | lor_expr TOK_LOR error
  ;

cond_expr: lor_expr
  | lor_expr '?' cond_expr ':' assignment_expr
  {
    $$ = mknode('?', $1, mknode(':', $3, $5));
  }
  ;

assignment_expr: cond_expr
  | lor_expr assign assignment_expr  { $$=mknode($2,$1,$3); }
  | lor_expr assign error { $$=$1; reset_type_stack(); yyerrok; }
  | open_bracket_with_line_info low_lvalue_list ']' low_assign assignment_expr
  {
    if (!(THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
      mark_lvalues_as_used($2);
    }
    $$=mknode($4,mknode(F_ARRAY_LVALUE,$2,0),$5);
    COPY_LINE_NUMBER_INFO($$, $1);
    free_node ($1);
  }
  | open_bracket_with_line_info low_lvalue_list ']' error
    {
      $$=$2; free_node ($1); reset_type_stack(); yyerrok;
    }
/*  | error { $$=0; reset_type_stack(); } */
  ;

low_assign: '=' { $$=F_ASSIGN; } ;

assign: low_assign
  | TOK_AND_EQ { $$=F_AND_EQ; }
  | TOK_OR_EQ  { $$=F_OR_EQ; }
  | TOK_XOR_EQ { $$=F_XOR_EQ; }
  | TOK_LSH_EQ { $$=F_LSH_EQ; }
  | TOK_RSH_EQ { $$=F_RSH_EQ; }
  | TOK_ADD_EQ { $$=F_ADD_EQ; }
  | TOK_SUB_EQ { $$=F_SUB_EQ; }
  | TOK_MULT_EQ{ $$=F_MULT_EQ; }
  | TOK_POW_EQ { $$=F_POW_EQ; }
  | TOK_MOD_EQ { $$=F_MOD_EQ; }
  | TOK_DIV_EQ { $$=F_DIV_EQ; }
  | TOK_ATOMIC_GET_SET { $$=F_ATOMIC_GET_SET; }
  ;

comma_expr: assignment_expr
  | comma_expr ',' assignment_expr
  {
    $$ = mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $1), $3);
  }
  ;

init_expr: comma_expr
  | simple_type2 local_name_list { $$=$2; }
  ;

safe_init_expr: init_expr
  | error { $$=0; }
  ;

optional_init_expr: /* empty */ { $$=0; }
  | safe_init_expr
  ;

void_expr: init_expr { $$ = mkcastnode(void_type_string, $1); } ;

optional_void_expr: /* empty */ { $$=0; }
  | void_expr
  | error { $$=0; }
  ;

/* FIXMEs
 * It would be nice if 'return' would exit from
 * the surrounding function rather than from the
 * implicit lambda. (I think) So beware that the
 * behaviour of 'return' might change some day.
 * -Hubbe
 *
 * It would also be nice if it was possible to send
 * arguments to the implicit function, but it would
 * require using ugly implicit variables or extending
 * the syntax, and if you extend the syntax you might
 * as well use lambda() instead.
 * -Hubbe
 *
 * We might want to allow having more than block after
 * a function ( ie. func(args) {} {} {} {} )
 * -Hubbe
 */

optional_block: /* EMPTY */ { $$=0; }
  | '{' line_number_info
  /* FIXME: Use implicit_identifier to make __func__ point to the lambda? */
  start_lambda
  {
    /* block code */
    $<number>1=Pike_compiler->num_used_modules;
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;

    /* Declare the argument variable.
     *
     * NB: The code in the next block knows that this variable
     *     will be variable  #0.
     */
    push_type(T_MIXED);
    push_unlimited_array_type(T_ARRAY);
    add_local_name(args_string, compiler_pop_type(), 0);
  }
  statements end_block
  {
    struct pike_type *type;
    int f/*, e */;
    struct pike_string *name;
    struct compilation *c = THIS_COMPILATION;
    struct pike_string *save_file = c->lex.current_file;
    int save_line = c->lex.current_line;

    /* Don't warn about the argument if unused. */
    Pike_compiler->compiler_frame->local_names[0].flags |= LOCAL_VAR_IS_USED;

    c->lex.current_file = $2->current_file;
    c->lex.current_line = $2->line_number;

    /* block code */
    unuse_modules(Pike_compiler->num_used_modules - $<number>1);
    $5 = pop_local_variables($<number>4, $5);

    debug_malloc_touch($5);
    $5=mknode(F_COMMA_EXPR,$5,mknode(F_RETURN,mkintnode(0),0));

    if (Pike_compiler->compiler_pass != COMPILER_PASS_FIRST) {
      /* Doing this in pass 1 might induce too strict checks on types
       * in cases where we got placeholders. */
      type=find_return_type($5);
      if (type) {
	push_finished_type(type);
	free_type(type);
      } else {
	yywarning("Failed to determine return type for implicit lambda.");
	push_type(T_ZERO);
      }
    } else {
      /* Tentative return type. */
      push_type(T_MIXED);
    }

    /* Any extra args are available in __ARGS__, but allow
     * the user to ignore them implicitly if not used.
     */
    push_type(T_MIXED);
    push_type(T_MANY);

    type=compiler_pop_type();

    name = get_new_name(NULL);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: IMPLICIT LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, name->str,
	    (long)Pike_compiler->new_program->id,
	    Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    f=dooptcode(name,
		$5,
		type,
		ID_PROTECTED | ID_PRIVATE | ID_INLINE | ID_USED);

    if(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPED) {
      $$ = mktrampolinenode(f,Pike_compiler->compiler_frame->previous);
    } else {
      $$ = mkidentifiernode(f);
    }

    c->lex.current_line = save_line;
    c->lex.current_file = save_file;
    free_node ($2);
    free_string(name);
    free_type(type);
#ifdef PIKE_DEBUG
    if (Pike_compiler->compiler_frame != $3) {
      Pike_fatal("Lost track of compiler_frame!\n"
		 "  Got: %p (Expected: %p) Previous: %p\n",
		 Pike_compiler->compiler_frame, $3,
		 Pike_compiler->compiler_frame->previous);
    }
#endif
    pop_compiler_frame();
  }
  ;

apply:
  postfix_expr open_paren_with_line_info expr_list ')' optional_block
  {
    $$ = mkapplynode($1, mknode(F_ARG_LIST, $3, $5));
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node ($2);
  }
  | postfix_expr safe_apply_with_line_info expr_list ')' optional_block
  {
    /* A(?B...) to ((tmp=A) && tmp(B...)) */
    int temporary;
    if( $1 && ($1->token == F_LOCAL) )
    {
      $$=mknode(F_LAND, copy_node($1), mkapplynode($1, mknode(F_ARG_LIST, $3, $5)));
    }
    else
    {
      fix_type_field( $1 );
      if( $1 && $1->type )
      {
        add_ref($1->type);
        temporary = add_local_name(empty_pike_string, $1->type, 0);
        Pike_compiler->compiler_frame->local_names[temporary].flags |=
	  LOCAL_VAR_IS_USED;
        $$=mknode(F_LAND,
                  mknode(F_ASSIGN, mklocalnode(temporary,0), $1),
		  mkapplynode(mklocalnode(temporary,0), mknode(F_ARG_LIST, $3, $5)));
	$$ = pop_local_variables(temporary, $$);
      }
      else
      {
	$$ = mkapplynode($1, mknode(F_ARG_LIST, $3, $5));
        yyerror("Indexing unexpected value.");
      }
    }
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node ($2);
  }
  | postfix_expr open_paren_or_safe_apply_with_line_info error ')' optional_block
  {
    $$=mkapplynode($1, $5);
    free_node ($2);
    yyerrok;
  }
  | postfix_expr open_paren_or_safe_apply_with_line_info error TOK_LEX_EOF
  {
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
    $$=mkapplynode($1, NULL);
    free_node ($2);
  }
  | postfix_expr open_paren_or_safe_apply_with_line_info error ';'
  {
    yyerror("Missing ')'.");
    $$=mkapplynode($1, NULL);
    free_node ($2);
  }
  | postfix_expr open_paren_or_safe_apply_with_line_info error '}'
  {
    yyerror("Missing ')'.");
    $$=mkapplynode($1, NULL);
    free_node ($2);
  }
  ;

implicit_modifiers:
  {
    free_node(Pike_compiler->current_annotations);
    Pike_compiler->current_annotations = NULL;
    $$ = Pike_compiler->current_modifiers = ID_PROTECTED|ID_INLINE|ID_PRIVATE |
      (THIS_COMPILATION->lex.pragmas & ID_MODIFIER_MASK);
  }
  ;

string_or_identifier: TOK_IDENTIFIER
  | string
  ;

/* Note that the result of this rule is passed both with
 * the node value (inherit number) and with two global
 * variables (inherit_state and inherit_depth).
 *
 * Note also that inherit number -1 indicates any inherit.
 */
inherit_specifier: string_or_identifier TOK_COLON_COLON
  {
    struct compilation *c = THIS_COMPILATION;
    struct program_state *state = Pike_compiler;
    int depth;
    int e = INHERIT_ALL;

    inherit_state = NULL;
    inherit_depth = 0;

    /* NB: The heuristics here are a bit strange
     *     (all to make it as backward compatible as possible).
     *
     *     The priority order is as follows:
     *
     *     1: Direct inherits in the current class.
     *
     *     2: The name of the current class.
     *
     *     3: 1 & 2 recursively for surrounding parent classes.
     *
     *     4: Indirect inherits in the current class.
     *
     *     5: 4 recursively for surrounding parent classes.
     *
     *     6: this & this_program.
     *
     * Note that a deep inherit in the current class trumphs
     * a not so deep inherit in a parent class (but not a
     * direct inherit in a parent class). To select the deep
     * inherit in the parent class in this case, prefix it
     * with the name of the parent class.
     */
    for (depth = 0;; depth++, state = state->previous) {
      int inh = find_inherit(state->new_program, $1->u.sval.u.string);
      if (inh &&
	  (!inherit_state ||
	   (state->new_program->inherits[inh].inherit_level == 1))) {
	/* Found, and we've either not found anything earlier,
	 * or this is a direct inherit (and the previous
	 * wasn't since we didn't break out of the loop).
	 */
	e = inh;
	inherit_state = state;
	inherit_depth = depth;
	if (state->new_program->inherits[inh].inherit_level == 1) {
	  /* Name of direct inherit ==> Done. */
	  break;
	}
      }
      /* The top-level class does not have a name, so break here. */
      if (depth == c->compilation_depth) break;
      if (ID_FROM_INT (state->previous->new_program,
		       state->parent_identifier)->name ==
	  $1->u.sval.u.string) {
	/* Name of surrounding class ==> Done. */
	e = INHERIT_SELF;
	inherit_state = state;
	inherit_depth = depth;
	break;
      }
    }
    if (e < 0) {
      inherit_state = Pike_compiler;
      inherit_depth = 0;
      e = INHERIT_SELF;

      if (($1->u.sval.u.string != this_program_string) &&
	  ($1->u.sval.u.string != this_string)) {
        my_yyerror("No inherit or surrounding class %pS.",
                   $1->u.sval.u.string);
      }
    }
    free_node($1);
    $$ = e;
  }
  | TOK_LOCAL_ID TOK_COLON_COLON
  {
    inherit_state = Pike_compiler;
    inherit_depth = 0;
    $$ = INHERIT_LOCAL;
  }
  | TOK_GLOBAL TOK_COLON_COLON
  {
    struct compilation *c = THIS_COMPILATION;
    inherit_state = Pike_compiler;
    for (inherit_depth = 0; inherit_depth < c->compilation_depth;
	 inherit_depth++, inherit_state = inherit_state->previous) {}
    $$ = INHERIT_GLOBAL;
  }
  | inherit_specifier TOK_LOCAL_ID TOK_COLON_COLON
  {
    if ($1 > 0) {
      yywarning("local:: references to inherited symbols is a noop.");
      $$ = $1;
    } else {
      $$ = INHERIT_LOCAL;
    }
  }
  | inherit_specifier TOK_IDENTIFIER TOK_COLON_COLON
  {
    int e = 0;
    if ($1 < 0) {
      $1 = INHERIT_SELF;
    }
#if 0
    /* FIXME: The inherit modifiers aren't kept. */
    if (!(inherit_state->new_program->inherits[$1].flags & ID_PRIVATE)) {
#endif /* 0 */
      e = find_inherit(inherit_state->new_program->inherits[$1].prog,
		       $2->u.sval.u.string);
#if 0
    }
#endif /* 0 */
    if (!e) {
      if (inherit_state->new_program->inherits[$1].name) {
        my_yyerror("No such inherit %pS::%pS.",
		   inherit_state->new_program->inherits[$1].name,
		   $2->u.sval.u.string);
      } else {
        my_yyerror("No such inherit %pS.", $2->u.sval.u.string);
      }
      $$ = INHERIT_ALL;
    } else {
      /* We know stuff about the inherit structure... */
      $$ = e + $1;
    }
    free_node($2);
  }
  | inherit_specifier bad_inherit TOK_COLON_COLON { $$ = INHERIT_ALL; }
  ;

low_id_expr: TOK_IDENTIFIER
  {
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $1->u.sval.u.string);

    if(($$=lexical_islocal(Pike_compiler->last_identifier)))
    {
      /* done, nothing to do here */
    }else if(!($$=find_module_identifier(Pike_compiler->last_identifier,1)) &&
	     !($$ = program_magic_identifier (Pike_compiler, 0, -1,
					      Pike_compiler->last_identifier, 0))) {
      if((Pike_compiler->flags & COMPILATION_FORCE_RESOLVE) ||
	 (Pike_compiler->compiler_pass == COMPILER_PASS_LAST)) {
        my_yyerror("Undefined identifier %pS.",
		   Pike_compiler->last_identifier);
	/* FIXME: Add this identifier as a constant in the current program to
	 *        avoid multiple reporting of the same identifier.
	 * NOTE: This should then only be done in the second pass.
	 */
	$$=0;
      }else{
	$$=mknode(F_UNDEFINED,0,0);
      }
    }

    set_node_name($$, $1->name);
    free_node($1);
  }
  | '.' TOK_IDENTIFIER
  {
    push_constant_text("");
    if (call_handle_import()) {
      node *tmp=mkconstantsvaluenode(Pike_sp-1);
      set_node_name(tmp, empty_pike_string);
      pop_stack();

      $$ = index_node(tmp, $2->u.sval.u.string);
      free_node(tmp);
    }
    else
      $$=mknewintnode(0);
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $2->u.sval.u.string);
    free_node($2);
  }
  | TOK_GLOBAL '.' TOK_IDENTIFIER
  {
    $$ = resolve_identifier ($3->u.sval.u.string);
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);
    free_node ($3);
  }
  | TOK_RESERVED
  {
    ref_push_string($1->u.sval.u.string);
    low_yyreport(REPORT_ERROR, NULL, 0, parser_system_string,
		 1, "Unknown reserved symbol %s.");
    free_node($1);
    $$ = 0;
  }
  ;

range_bound:
  /* empty */
  {$$ = mknode (F_RANGE_OPEN, NULL, NULL);}
  | init_expr
  {$$ = mknode (F_RANGE_FROM_BEG, $1, NULL);}
  | '<' init_expr
  {$$ = mknode (F_RANGE_FROM_END, $2, NULL);}
  | TOK_LEX_EOF
  {
    yyerror("Unexpected end of file.");
    $$ = mknode (F_RANGE_OPEN, NULL, NULL);
  }
  | '<' TOK_LEX_EOF
  {
    yyerror("Unexpected end of file.");
    $$ = mknode (F_RANGE_OPEN, NULL, NULL);
  }
  ;

gauge: TOK_GAUGE catch_arg
  {
    $$=mkopernode("`/",
		  mkopernode("`-",
			     mkopernode("`-",
					mkefuncallnode("gethrvtime",
						       mkintnode(1)),
					mknode(F_COMMA_EXPR,
					       mknode(F_POP_VALUE, $2, NULL),
					       mkefuncallnode("gethrvtime",
							      mkintnode(1)))),
			     NULL),
		  mkfloatnode((FLOAT_TYPE)1e9));
  };

typeof: TOK_TYPEOF '(' assignment_expr ')'
  {
    $$ = mknode(F_TYPEOF, $3, 0);
  }
  | TOK_TYPEOF '(' error ')' { $$=0; yyerrok; }
  | TOK_TYPEOF '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | TOK_TYPEOF '(' error TOK_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_TYPEOF '(' error ';' { $$=0; yyerror("Missing ')'."); }
  ;

catch_arg: '(' init_expr ')'  { $$=$2; }
  | '(' error ')' { $$=0; yyerrok; }
  | '(' error TOK_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | '(' error ';' { $$=0; yyerror("Missing ')'."); }
  | block
  | error { $$=0; yyerror("Bad expression for catch."); }
  ;

catch: TOK_CATCH
     {
       Pike_compiler->catch_level++;
     }
     catch_arg
     {
       $$=mknode(F_CATCH,$3,NULL);
       Pike_compiler->catch_level--;
     }
     ;

sscanf: TOK_SSCANF '(' assignment_expr ',' assignment_expr lvalue_list ')'
  {
    if ($6 && !(THIS_COMPILATION->lex.pragmas & ID_STRICT_TYPES)) {
      mark_lvalues_as_used($6);
    }
    if (TEST_COMPAT(8, 0)) {
      $$ = mknode(F_SSCANF_80, mknode(F_ARG_LIST, $3, $5), $6);
    } else {
      $$ = mknode(F_SSCANF, mknode(F_ARG_LIST, $3, $5), $6);
    }
  }
  | TOK_SSCANF '(' assignment_expr ',' assignment_expr error ')'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerrok;
  }
  | TOK_SSCANF '(' assignment_expr ',' assignment_expr error TOK_LEX_EOF
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_SSCANF '(' assignment_expr ',' assignment_expr error '}'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' assignment_expr ',' assignment_expr error ';'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' assignment_expr error ')'
  {
    $$=0;
    free_node($3);
    yyerrok;
  }
  | TOK_SSCANF '(' assignment_expr error TOK_LEX_EOF
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_SSCANF '(' assignment_expr error '}'
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' assignment_expr error ';'
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' error ')' { $$=0; yyerrok; }
  | TOK_SSCANF '(' error TOK_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_SSCANF '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | TOK_SSCANF '(' error ';' { $$=0; yyerror("Missing ')'."); }
  ;

lvalue: assignment_expr
  | open_bracket_with_line_info low_lvalue_list ']'
  {
    /* NB: The optional default value is handled by assignment_expr above. */
    $$ = mknode(F_ARRAY_LVALUE, $2, 0);
    COPY_LINE_NUMBER_INFO($$, $1);
    free_node($1);
  }
  | type2 TOK_IDENTIFIER optional_default_value
  {
    int id = add_local_name($2->u.sval.u.string,compiler_pop_type(),0);
    /* Note: Variable intentionally not marked as used. */
    if (id >= 0) {
      if ($3) {
        $$ = mknode(F_ASSIGN, mklocalnode(id, 0), $3);
      } else {
        $$ = mknode(F_CLEAR_LOCAL, mklocalnode(id, 0), NULL);
      }
    } else
      $$ = 0;
    free_node($2);
  }
  /* FIXME: Add production for type2 ==> constant type svalue here? */
  ;

low_lvalue_list: lvalue lvalue_list
  {
    $$=mknode(F_LVALUE_LIST,$1,$2);
  }
  ;

lvalue_list: /* empty */ { $$ = 0; }
  | ',' lvalue lvalue_list
  {
    $$ = mknode(F_LVALUE_LIST,$2,$3);
  }
  ;

string_segment: TOK_STRING
  | TOK_FUNCTION_NAME
  {
    struct compiler_frame *f = Pike_compiler->compiler_frame;
    if (!f || (f->current_function_number < 0)) {
      $$ = mkstrnode(lfun_strings[LFUN___INIT]);
    } else {
      struct identifier *id =
	ID_FROM_INT(Pike_compiler->new_program, f->current_function_number);
      if (!id->name->size_shift) {
	int len;
	if ((len = strlen(id->name->str)) == id->name->len) {
	  /* Most common case. */
	  $$ = mkstrnode(id->name);
	} else {
	  struct pike_string *str =
	    make_shared_binary_string(id->name->str, len);
	  $$ = mkstrnode(str);
	  free_string(str);
	}
      } else {
	struct pike_string *str;
	struct array *split;
	MAKE_CONST_STRING(str, "\0");
	split = explode(id->name, str);
	$$ = mkstrnode(split->item->u.string);
	free_array(split);
      }
    }
  }
  ;

string: string_segment
  | string string_segment
  {
    struct pike_string *a,*b;
    copy_shared_string(a,$1->u.sval.u.string);
    copy_shared_string(b,$2->u.sval.u.string);
    free_node($1);
    free_node($2);
    a=add_and_free_shared_strings(a,b);
    $$=mkstrnode(a);
    free_string(a);
  }
  ;

string_constant: string
  | string_constant '+' string
  {
    struct pike_string *a,*b;
    copy_shared_string(a,$1->u.sval.u.string);
    copy_shared_string(b,$3->u.sval.u.string);
    free_node($1);
    free_node($3);
    a=add_and_free_shared_strings(a,b);
    $$=mkstrnode(a);
    free_string(a);
  }
  ;

/* Same as string_constant above, but without TOK_FUNCTION_NAME. */
real_string_constant: TOK_STRING
  | real_string_constant TOK_STRING
  {
    struct pike_string *a,*b;
    copy_shared_string(a,$1->u.sval.u.string);
    copy_shared_string(b,$2->u.sval.u.string);
    free_node($1);
    free_node($2);
    a=add_and_free_shared_strings(a,b);
    $$=mkstrnode(a);
    free_string(a);
  }
  | real_string_constant '+' TOK_STRING
  {
    struct pike_string *a,*b;
    copy_shared_string(a,$1->u.sval.u.string);
    copy_shared_string(b,$3->u.sval.u.string);
    free_node($1);
    free_node($3);
    a=add_and_free_shared_strings(a,b);
    $$=mkstrnode(a);
    free_string(a);
  }
  ;


/*
 * Some error-handling
 */

/* FIXME: Should probably set Pike_compiler->last_identifier. */
bad_identifier: bad_inherit
  | TOK_LOCAL_ID
  { yyerror_reserved("local"); }
  ;

bad_inherit: bad_expr_ident
  | TOK_ARRAY_ID
  { yyerror_reserved("array"); }
  | TOK_ATTRIBUTE_ID
  { yyerror_reserved("__attribute__"); }
  | TOK_BREAK
  { yyerror_reserved("break"); }
  | TOK_CASE
  { yyerror_reserved("case"); }
  | TOK_CATCH
  { yyerror_reserved("catch"); }
  | TOK_CLASS
  { yyerror_reserved("class"); }
  | TOK_CONTINUE
  { yyerror_reserved("continue"); }
  | TOK_DEFAULT
  { yyerror_reserved("default"); }
  | TOK_DEPRECATED_ID
  { yyerror_reserved("__deprecated__"); }
  | TOK_DO
  { yyerror_reserved("do"); }
  | TOK_ENUM
  { yyerror_reserved("enum"); }
  | TOK_FLOAT_ID
  { yyerror_reserved("float");}
  | TOK_FOR
  { yyerror_reserved("for"); }
  | TOK_FOREACH
  { yyerror_reserved("foreach"); }
  | TOK_FUNCTION_ID
  { yyerror_reserved("function");}
  | TOK_FUNCTION_NAME
  { yyerror_reserved("__FUNCTION__");}
  | TOK_GAUGE
  { yyerror_reserved("gauge"); }
  | TOK_IF
  { yyerror_reserved("if"); }
  | TOK_IMPORT
  { yyerror_reserved("import"); }
  | TOK_INT_ID
  { yyerror_reserved("int"); }
  | TOK_LAMBDA
  { yyerror_reserved("lambda"); }
  | TOK_MAPPING_ID
  { yyerror_reserved("mapping"); }
  | TOK_MIXED_ID
  { yyerror_reserved("mixed"); }
  | TOK_MULTISET_ID
  { yyerror_reserved("multiset"); }
  | TOK_OBJECT_ID
  { yyerror_reserved("object"); }
  | TOK_PROGRAM_ID
  { yyerror_reserved("program"); }
  | TOK_RETURN
  { yyerror_reserved("return"); }
  | TOK_SSCANF
  { yyerror_reserved("sscanf"); }
  | TOK_STRING_ID
  { yyerror_reserved("string"); }
  | TOK_SWITCH
  { yyerror_reserved("switch"); }
  | TOK_TYPEDEF
  { yyerror_reserved("typedef"); }
  | TOK_TYPEOF
  { yyerror_reserved("typeof"); }
  | TOK_UNKNOWN
  { yyerror_reserved("__unknown__"); }
  | TOK_VOID_ID
  { yyerror_reserved("void"); }
  | TOK_RESERVED
  {
    ref_push_string($1->u.sval.u.string);
    low_yyreport(REPORT_ERROR, NULL, 0, parser_system_string,
		 1, "Unknown reserved symbol %s.");
    free_node($1);
  }
  ;

bad_expr_ident:
    TOK_INLINE
  { yyerror_reserved("inline"); }
  | TOK_PREDEF
  { yyerror_reserved("predef"); }
  | TOK_PRIVATE
  { yyerror_reserved("private"); }
  | TOK_PROTECTED
  { yyerror_reserved("protected"); }
  | TOK_PUBLIC
  { yyerror_reserved("public"); }
  | TOK_OPTIONAL
  { yyerror_reserved("optional"); }
  | TOK_VARIANT
  { yyerror_reserved("variant"); }
  | TOK_WEAK
  { yyerror_reserved("__weak__"); }
  | TOK_STATIC
  { yyerror_reserved("static"); }
  | TOK_EXTERN
  { yyerror_reserved("extern"); }
  | TOK_FINAL_ID
  { yyerror_reserved("final");}
  | TOK_ELSE
  { yyerror("else without if."); }
  | TOK_INHERIT
  { yyerror_reserved("inherit"); }
  ;

/*
 * Kludge for bison 1.50.
 *
 * Bison 1.50 doesn't support having multiple action blocks
 * in a sequence where a block refers to the value of its
 * immediate predecessor.
 */

/* empty: ; */ /* line_number_info is now used in these cases. */

%%

static void yyerror_reserved(const char *keyword)
{
  my_yyerror("%s is a reserved word.", keyword);
}

struct pike_string *get_new_name(struct pike_string *prefix)
{
  char buf[40];
  /* Generate a name for a global symbol... */
  sprintf(buf,"__lambda_%ld_%ld_line_%d",
	  (long)Pike_compiler->new_program->id,
	  (long)(Pike_compiler->local_class_counter++ & 0xffffffff), /* OSF/1 cc bug. */
	  (int) THIS_COMPILATION->lex.current_line);
  if (prefix) {
    struct string_builder sb;
    init_string_builder_alloc(&sb, prefix->len + strlen(buf) + 1,
			      prefix->size_shift);
    string_builder_shared_strcat(&sb, prefix);
    string_builder_putchar(&sb, 0);
    string_builder_strcat(&sb, buf);
    return finish_string_builder(&sb);
  }
  return make_shared_string(buf);
}


static int low_islocal(struct compiler_frame *f,
		       struct pike_string *str)
{
  int e;
  for(e=f->current_number_of_locals-1;e>=0;e--)
    if(f->local_names[e].name==str) {
      f->local_names[e].flags |= LOCAL_VAR_IS_USED;
      return e;
    }
  return -1;
}

int low_bind_local(struct compiler_frame *frame, node *n)
{
  int local_no;
  if (!frame) {
    frame = Pike_compiler->compiler_frame;
  }
  if (!n || (n->token != F_LOCAL_INDIRECT)) {
    if (n && (n->token == F_LOCAL) && (n->u.integer.b <= 0)) {
      return n->u.integer.a;
    }
    return -1;
  }

  local_no = n->u.integer.a;

  n->token = F_LOCAL;
  do {
    if ((n->u.integer.a = frame->next_local_offset++) >= MAX_LOCAL) {
      yyerror("Too many local variables.");
      n->u.integer.a = -1;
      return -1;
    }
  } while (frame->local_variables[n->u.integer.a]);
  frame->local_variables[n->u.integer.a] = local_no + 1;

  if (frame->next_local_offset > frame->max_number_of_locals) {
    frame->max_number_of_locals = frame->next_local_offset;
  }

  if (frame->generator_local != -1) {
    /* For generators all locals need to be scoped.
     *
     * NB: We do not need to mess with the frame flags,
     *     as that has already been done when
     *     generator_local was set.
     */
    n->u.integer.b = -1;
    frame->local_names[local_no].flags |= LOCAL_VAR_USED_IN_SCOPE;
    frame->local_shared[n->u.integer.a>>4] |= 1 << (n->u.integer.a & 0xf);
    if (frame->min_number_of_locals <= n->u.integer.a) {
      frame->min_number_of_locals = n->u.integer.a + 1;
    }
  }

  return n->u.integer.a;
}

int bind_local(struct compiler_frame *frame, int local_no)
{
  node *n;
  if (!frame) {
    frame = Pike_compiler->compiler_frame;
  }
  n = frame->local_names[local_no].def;
#ifdef PIKE_DEBUG
  if (!n) Pike_fatal("No def for local #%d!\n", local_no);
#endif
  return low_bind_local(frame, n);
}

void release_local(struct compiler_frame *frame, int var)
{
  if (!frame) {
    frame = Pike_compiler->compiler_frame;
  }
  frame->local_variables[var] = 0;
  if (frame->next_local_offset > var) {
    frame->next_local_offset = var;
  }
}

/* Add a local variable to the current function in frame.
 * NOTE: Steals a reference to each of type, def and init, but not to str.
 *
 * NB: type MUST be NULL if def is not NULL.
 */
int low_add_local_name(struct compiler_frame *frame,
		       struct pike_string *str,
		       struct pike_type *type,
		       node *def,
		       node *init)
{
  int var;

  if (!frame) {
    frame = Pike_compiler->compiler_frame;
  }

  if (str->len) {
    int tmp=low_islocal(frame,str);
    if(tmp>=0 && tmp >= frame->last_block_level)
    {
      struct local_name *l = frame->local_names + tmp;
      if (l->def) {
        my_yyerror("Duplicate local identifier %pS, "
                   "previous declaration on line %ld.",
                   str, (long)l->def->line_number);
      } else {
        my_yyerror("Duplicate local identifier %pS.", str);
      }
    }

    if(type == void_type_string)
    {
      my_yyerror("Local variable %pS is void.", str);
    }
  }

  debug_malloc_touch(def);
  debug_malloc_touch(init);
  debug_malloc_touch(type);
  debug_malloc_touch(str);
  /* NOTE: The number of locals can be 0..255 (not 256), due to
   *       the use of READ_INCR_BYTE() in apply_low.h.
   */
  do {
    var = frame->current_number_of_locals;

    if (var >= MAX_LOCAL-1)
    {
      my_yyerror("Too many local variables: no space for local variable %pS.",
                 str);
      free_type(type);
      if (def) free_node(def);
      if (init) free_node(init);
      return -1;
    }

    frame->current_number_of_locals++;
  } while (frame->local_names[var].def);

  if (var >= frame->max_number_of_locals) {
    frame->max_number_of_locals = var + 1;
  }

  if (type && pike_types_le(type, void_type_string, 0, 0)) {
    if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
      yywarning("Declaring local variable %pS with type void "
                "(converted to type zero).", str);
    }
  }

  frame->local_names[var].name = str;
  reference_shared_string(str);
  if (def) {
    frame->local_names[var].def = def;
#ifdef PIKE_DEBUG
    if (type) {
      Pike_fatal("Type specified for local alias!\n");
    }
#endif
  } else {
    frame->local_names[var].def = def = internal_mklocalnode(frame, var);
#ifdef PIKE_DEBUG
    if (frame->local_names[var].def->type) {
      Pike_fatal("Internal local already has a type!\n");
    }
#endif
    def->type = type;
  }
  if (type && (type->type == PIKE_T_AUTO)) {
    frame->local_names[var].def->node_info |= OPT_TYPE_NOT_FIXED;
  }
  frame->local_names[var].init = init;

  if (frame->generator_local != -1) {
    frame->local_names[var].flags = LOCAL_VAR_IS_USED | LOCAL_VAR_USED_IN_SCOPE;
  } else if (pike_types_le(void_type_string, type, 0, 0)) {
    /* Don't warn about unused voidable variables. */
    frame->local_names[var].flags = LOCAL_VAR_IS_USED;
  } else {
    frame->local_names[var].flags = 0;
  }


  return var;
}


/* argument must be a shared string */
/* Note that this function eats a reference to 'type' */
/* If init is nonzero, it also eats a ref to init */
int add_local_name(struct pike_string *str,
		   struct pike_type *type,
		   node *init)
{
  return low_add_local_name(NULL, str, type, NULL, init);
}

/* Note that this function eats a reference each to 'def' and 'init'. */
static void redefine_local(struct compiler_frame *frame, int var,
                           node *def, node *init)
{
  if (!frame) {
    frame = Pike_compiler->compiler_frame;
  }
#ifdef PIKE_DEBUG
  if ((var < 0) || (var >= frame->current_number_of_locals)) {
    Pike_fatal("No such local: $%d.\n", var);
  }
#endif
  if (def) {
    free_node(frame->local_names[var].def);
    frame->local_names[var].def = def;
  }
  if (init) {
    if (frame->local_names[var].init) {
      free_node(frame->local_names[var].init);
    }
    frame->local_names[var].init = init;
  }
}

/* Mark local variable as used. */
static void mark_lvalue_as_used(node *n)
{
  while (n) {
    switch(n->token) {
    case F_CLEAR_LOCAL:
      n = CAR(n);
      break;
    case F_LOCAL:
      if (n->u.integer.b <= 0) {
        int ee =
          Pike_compiler->compiler_frame->local_variables[n->u.integer.a] - 1;
        struct local_name *var =
          Pike_compiler->compiler_frame->local_names + ee;
        var->flags |= LOCAL_VAR_IS_USED;
      }
      return;
    case F_LOCAL_INDIRECT:
      Pike_compiler->compiler_frame->local_names[n->u.integer.a].flags |=
        LOCAL_VAR_IS_USED;
      return;
    case F_ARRAY_LVALUE:
      mark_lvalues_as_used(CAR(n));
      return;
    default:
      return;
    }
  }
}

/* Mark local variables declared in a multi-assign or sscanf expression
 * as used. */
static void mark_lvalues_as_used(node *n)
{
  while (n && (n->token == F_LVALUE_LIST)) {
    mark_lvalue_as_used(CAR(n));
    n = CDR(n);
  }
}

#if 0
/* Note that this function eats a reference to each of
 * 'type' and 'initializer', but not to 'name'.
 * Note also that 'initializer' may be NULL.
 */
static node *add_protected_variable(struct pike_string *name,
				    struct pike_type *type,
				    int depth,
				    node *initializer)
{
  struct compiler_frame *f = Pike_compiler->compiler_frame;
  int i;
  int id;
  node *n = NULL;

  if (initializer) {
    /* FIXME: We need to pop levels off local and external variables here. */
  }

  for(i = depth; f && i; i--) {
    f->lexical_scope |= SCOPE_SCOPED;
    f = f->previous;
  }
  if (!f) {
    int parent_depth = i;
    struct program_state *p = Pike_compiler;
    struct pike_string *tmp_name;
    while (i--) {
      if (!p->previous) {
	my_yyerror("Too many levels of protected (%d, max:%d).",
		   depth, depth - (i+1));
	parent_depth -= i+1;
	break;
      }
      p->new_program->flags |= PROGRAM_USES_PARENT;
      p = p->previous;
    }

    tmp_name = get_new_name(name);
    id = define_parent_variable(p, tmp_name, type,
				ID_PROTECTED|ID_PRIVATE|ID_INLINE);
    free_string(tmp_name);
    if (id >= 0) {
      if (def) {
	p->init_node =
	  mknode(F_COMMA_EXPR, Pike_compiler->init_node,
		 mkcastnode(void_type_string,
			    mknode(F_ASSIGN,
				   mkidentifiernode(id), initializer)));
	initializer = NULL;
      }
      n = mkexternalnode(id, parent_depth);
    }
  } else if (depth) {
    f->lexical_scope|=SCOPE_SCOPE_USED;
    tmp_name = get_new_name(name);
    id = low_add_local_name(f, tmp_name, type, NULL, NULL);
    free_string(tmp_name);
    if(f->min_number_of_locals < id+1)
      f->min_number_of_locals = id+1;
    if (initializer) {
      /* FIXME! */
      yyerror("Initializers not yet supported for protected variables with function scope.");
    }
    n = mklocalnode(id, depth);
  }
  id = low_add_local_name(NULL, name, type, n, NULL);
  if (id >= 0) {
    if (initializer) {
      return mknode(F_ASSIGN, mklocalnode(id,0), initializer);
    }
    return mklocalnode(id, 0);
  }
  if (initializer) {
    free_node(initializer);
  }
  return NULL;
}
#endif /* 0 */

int islocal(struct pike_string *str)
{
  return low_islocal(Pike_compiler->compiler_frame, str);
}

/* argument must be a shared string */
static node *lexical_islocal(struct pike_string *str)
{
  int e,depth=0;
  struct compiler_frame *f=Pike_compiler->compiler_frame;

  while(1)
  {
    for(e=f->current_number_of_locals-1;e>=0;e--)
    {
      if(f->local_names[e].name==str)
      {
        node *n;
	f->local_names[e].flags |= LOCAL_VAR_IS_USED;

        n = mklocalnode(e, depth);
        set_node_name(n, str);

        return n;
      }
    }
    if(!(f->lexical_scope & SCOPE_LOCAL)) return 0;
    depth++;
    f=f->previous;
  }
}

static node *safe_inc_enum(node *n)
{
  JMP_BUF recovery;
  node *ret;
  STACK_LEVEL_START(0);

  if (SETJMP(recovery)) {
    handle_compile_exception ("Bad implicit enum value (failed to add 1).");
    push_int(0);
  } else {
    if (n->token != F_CONSTANT) Pike_fatal("Bad node to safe_inc_enum().\n");
    push_svalue(&n->u.sval);
    push_int(1);
    f_add(2);
  }
  UNSETJMP(recovery);
  STACK_LEVEL_DONE(1);
  free_node(n);
  ret = mkconstantsvaluenode(Pike_sp-1);
  pop_stack();
  return ret;
}

static node *find_versioned_identifier(struct pike_string *identifier,
				       int major, int minor)
{
  struct compilation *c = THIS_COMPILATION;
  int old_major = Pike_compiler->compat_major;
  int old_minor = Pike_compiler->compat_minor;
  struct svalue *efun = NULL;
  node *res;

  change_compiler_compatibility(major, minor);

  if(Pike_compiler->last_identifier)
    free_string(Pike_compiler->last_identifier);
  copy_shared_string(Pike_compiler->last_identifier, identifier);

  /* Check predef:: first, and then the modules. */

  res = NULL;
  if (TYPEOF(c->default_module) == T_MAPPING) {
    if ((efun = low_mapping_string_lookup(c->default_module.u.mapping,
					  identifier)))
      res = mkconstantsvaluenode(efun);
  }
  else if (TYPEOF(c->default_module) != T_INT) {
    JMP_BUF tmp;
    if (SETJMP (tmp)) {
      handle_compile_exception ("Couldn't index %d.%d "
                                "default module with %pq.",
				major, minor, identifier);
    } else {
      push_svalue(&c->default_module);
      ref_push_string(identifier);
      f_index (2);
      if (!IS_UNDEFINED(Pike_sp - 1))
	res = mkconstantsvaluenode(Pike_sp - 1);
      pop_stack();
    }
    UNSETJMP(tmp);
  }

  if (!res && !(res = resolve_identifier(identifier))) {
    if((Pike_compiler->flags & COMPILATION_FORCE_RESOLVE) ||
       (Pike_compiler->compiler_pass == COMPILER_PASS_LAST)) {
      my_yyerror("Undefined identifier %d.%d::%pS.",
		 major, minor, identifier);
    }else{
      res = mknode(F_UNDEFINED, 0, 0);
    }
  }

  if (res) {
    push_int(major);
    push_text(".");
    push_int(minor);
    push_text("::");
    ref_push_string(identifier);
    f_add(5);
    set_node_name(res, Pike_sp[-1].u.string);
    pop_stack();
  }

  change_compiler_compatibility(old_major, old_minor);

  return res;
}

static node *set_default_value(int e)
{
  node *init = Pike_compiler->compiler_frame->local_names[e].init;
  struct pike_type *type = Pike_compiler->compiler_frame->local_names[e].def->type;
  struct pike_string *name = Pike_compiler->compiler_frame->local_names[e].name;

  if (!init) return NULL;

  add_ref(init);

  if (Pike_compiler->compiler_frame->local_names[e].def->token != F_LOCAL) {
    /* Unconditional initializer.
     *
     * NB: No need to check for F_LOCAL_INDIRECT, as this code is
     *     currently only run for function arguments, ad they are
     *     always bound.
     *
     * $e = init;
     */
    return mknode(F_POP_VALUE,
                  mknode(F_ASSIGN,
                         mklocalnode(e, 0),
                         init), NULL);
  }

  if (type->flags & PT_FLAG_VOIDABLE) {
    if (Pike_compiler->compiler_pass == COMPILER_PASS_LAST) {
      yytype_report(REPORT_WARNING,
                    NULL, 0, NULL,
                    NULL, 0, type,
                    0, "Extraneous void in type for variable with default %pS.",
                    name);
    }
  }

  if (type->flags & PT_FLAG_NULLABLE) {
    /* if (undefinedp($e)) { $e = init; } */
    return mknode(F_POP_VALUE,
		  mknode(F_LAND,
			 mkefuncallnode("undefinedp",
					mklocalnode(e, 0)),
			 mknode(F_ASSIGN,
				mklocalnode(e, 0),
				init)), NULL);
  }

  /* if (!$e) { $e = init; } */
  return mknode(F_POP_VALUE,
		mknode(F_LOR,
		       mklocalnode(e, 0),
		       mknode(F_ASSIGN,
			      mklocalnode(e, 0),
			      init)), NULL);
}

static int call_handle_import(void)
{
  if (safe_apply_low2(Pike_fp->current_object,
                      PC_HANDLE_IMPORT_FUN_NUM
                      + Pike_fp->context->identifier_level, 1, NULL)) {
    if ((1 << TYPEOF(Pike_sp[-1])) &
	(BIT_MAPPING|BIT_OBJECT|BIT_PROGRAM|BIT_ZERO)) {
      if (SAFE_IS_ZERO(Pike_sp - 1)) {
	pop_stack();
	push_int(0);
      }
      if (TYPEOF(Pike_sp[-1]) != T_INT) return 1;

      pop_stack();
      my_yyerror("Couldn't find module to import.");
      return 0;
    }
    my_yyerror("Invalid return value from handle_import: %pO", Pike_sp-1);
    pop_stack();
    return 0;
  }
  handle_compile_exception ("Error finding module to import");
  pop_stack();

  return 0;
}

/* Set compiler_frame->current_type from the type stack. */
static void update_current_type(void)
{
  if (TEST_COMPAT(8,0)) {
    struct pike_type *t = peek_type_stack();
    if (!t || (t->type != PIKE_T_AUTO)) {
      /* Implicit zero */
      push_type(PIKE_T_ZERO);
      push_type(T_OR);
    }
  }
  if(Pike_compiler->compiler_frame->current_type)
    free_type(Pike_compiler->compiler_frame->current_type);
  Pike_compiler->compiler_frame->current_type = compiler_pop_type();
}
