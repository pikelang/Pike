/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: language.yacc,v 1.344 2006/04/02 16:42:23 grubba Exp $
*/

%pure_parser

%token TOK_ARROW

/*
 * Basic value pushing
 */
%token TOK_CONSTANT TOK_FLOAT TOK_STRING
%token TOK_NUMBER

/*
 * These are the predefined functions that can be accessed from Pike.
 */

%token TOK_INC TOK_DEC
%token TOK_RETURN

%token TOK_EQ TOK_GE TOK_LE TOK_NE
%token TOK_NOT
%token TOK_LSH TOK_RSH
%token TOK_LAND TOK_LOR

%token TOK_SWITCH TOK_SSCANF TOK_CATCH
%token TOK_FOREACH

/* This is the end of file marker used by the lexer
 * to enable nicer EOF in error handling.
 */
%token TOK_LEX_EOF

%token TOK_ADD_EQ
%token TOK_AND_EQ
%token TOK_ARRAY_ID
%token TOK_BREAK
%token TOK_CASE
%token TOK_CLASS
%token TOK_COLON_COLON
%token TOK_CONTINUE 
%token TOK_DEFAULT
%token TOK_DIV_EQ
%token TOK_DO
%token TOK_DOT_DOT
%token TOK_DOT_DOT_DOT
%token TOK_ELSE
%token TOK_ENUM
%token TOK_EXTERN
%token TOK_FLOAT_ID
%token TOK_FOR
%token TOK_FUNCTION_ID
%token TOK_GAUGE
%token TOK_GLOBAL
%token TOK_IDENTIFIER
%token TOK_IF
%token TOK_IMPORT
%token TOK_INHERIT
%token TOK_INLINE
%token TOK_LOCAL_ID
%token TOK_FINAL_ID
%token TOK_INT_ID
%token TOK_LAMBDA
%token TOK_MULTISET_ID
%token TOK_MULTISET_END
%token TOK_MULTISET_START
%token TOK_LSH_EQ
%token TOK_MAPPING_ID
%token TOK_MIXED_ID
%token TOK_MOD_EQ
%token TOK_MULT_EQ
%token TOK_NO_MASK
%token TOK_OBJECT_ID
%token TOK_OR_EQ
%token TOK_PRIVATE
%token TOK_PROGRAM_ID
%token TOK_PROTECTED
%token TOK_PREDEF
%token TOK_PUBLIC
%token TOK_RSH_EQ
%token TOK_STATIC
%token TOK_STRING_ID
%token TOK_SUB_EQ
%token TOK_TYPEDEF
%token TOK_TYPEOF
%token TOK_VARIANT
%token TOK_VOID_ID
%token TOK_WHILE
%token TOK_XOR_EQ
%token TOK_OPTIONAL


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
%nonassoc TOK_INC TOK_DEC

%{
/* This is the grammar definition of Pike. */

#include "global.h"
RCSID("$Id: language.yacc,v 1.344 2006/04/02 16:42:23 grubba Exp $");
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include "interpret.h"
#include "array.h"
#include "object.h"
#include "stralloc.h"
#include "las.h"
#include "interpret.h"
#include "program.h"
#include "pike_types.h"
#include "constants.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "docode.h"
#include "machine.h"
#include "main.h"
#include "opcodes.h"
#include "operators.h"
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

int add_local_name(struct pike_string *, struct pike_type *, node *);
int low_add_local_name(struct compiler_frame *,
		       struct pike_string *, struct pike_type *, node *);
static node *lexical_islocal(struct pike_string *);
static void safe_inc_enum(void);
static int call_handle_import(struct pike_string *s);

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
/* Needs to be included after YYSTYPE is defined. */
#define INCLUDED_FROM_LANGUAGE_YACC
#include "lex.h"
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
%type <number> TOK_NO_MASK
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
%type <number> TOK_SWITCH
%type <number> TOK_VOID_ID
%type <number> TOK_WHILE
%type <number> arguments
%type <number> arguments2
%type <number> func_args
%type <number> optional_create_arguments
%type <number> create_arguments
%type <number> create_arguments2
%type <number> create_arg
%type <number> assign
%type <number> modifier
%type <number> modifier_list
%type <number> modifiers
%type <number> inherit_specifier
%type <number> function_type_list
%type <number> function_type_list2
%type <number> optional_dot_dot_dot
%type <number> optional_comma
%type <number> optional_stars

%type <str> magic_identifiers
%type <str> magic_identifiers1
%type <str> magic_identifiers2
%type <str> magic_identifiers3

/* The following symbols return type information */

%type <n> number_or_minint
%type <n> number_or_maxint
%type <n> cast
%type <n> soft_cast
%type <n> simple_type
%type <n> simple_type2
%type <n> simple_identifier_type
%type <n> string_constant
%type <n> string
%type <n> TOK_STRING
%type <n> TOK_NUMBER
%type <n> optional_rename_inherit
%type <n> optional_identifier
%type <n> TOK_IDENTIFIER
%type <n> assoc_pair
%type <n> line_number_info
%type <n> block
%type <n> optional_block
%type <n> failsafe_block
%type <n> open_paren_with_line_info
%type <n> close_paren_or_missing
%type <n> open_bracket_with_line_info
%type <n> block_or_semi
%type <n> break
%type <n> case
%type <n> catch
%type <n> catch_arg
%type <n> class
%type <n> enum
%type <n> safe_comma_expr
%type <n> comma_expr
%type <n> comma_expr2
%type <n> comma_expr_or_maxint
%type <n> comma_expr_or_zero
%type <n> cond
%type <n> continue
%type <n> default
%type <n> do
%type <n> safe_expr0
%type <n> expr00
%type <n> expr01
%type <n> expr1
%type <n> expr2
%type <n> expr3 expr0
%type <n> expr4
%type <n> expr_list
%type <n> expr_list2
%type <n> for
%type <n> for_expr
%type <n> foreach
%type <n> gauge
%type <n> idents
%type <n> idents2
%type <n> labeled_statement
%type <n> lambda
%type <n> local_name_list
%type <n> local_name_list2
%type <n> low_idents
%type <n> safe_lvalue
%type <n> lvalue
%type <n> lvalue_list
%type <n> low_lvalue_list
%type <n> m_expr_list
%type <n> m_expr_list2
%type <n> new_local_name
%type <n> new_local_name2
%type <n> normal_label_statement
%type <n> optional_else_part
%type <n> optional_label
%type <n> return
%type <n> sscanf
%type <n> statement
%type <n> statements
%type <n> statement_with_semicolon
%type <n> switch
%type <n> typeof
%type <n> unused
%type <n> unused2
%type <n> while
%type <n> optional_comma_expr
%type <n> low_program_ref
%type <n> inherit_ref
%type <n> local_function
%type <n> local_function2
%type <n> magic_identifier
%type <n> simple_identifier
%type <n> foreach_lvalues
%type <n> foreach_optional_lvalue
%%

all: program { YYACCEPT; }
  | program TOK_LEX_EOF { YYACCEPT; }
/*  | error TOK_LEX_EOF { YYABORT; } */
  ;

program: program def
  | program ';'
  |  /* empty */
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

optional_rename_inherit: ':' TOK_IDENTIFIER { $$=$2; }
  | ':' bad_identifier { $$=0; }
  | ':' error { $$=0; }
  | { $$=0; }
  ;

/* NOTE: This rule pushes a string "name" on the stack in addition
 * to resolving the program reference.
 */
low_program_ref: string_constant
  {
    STACK_LEVEL_START(0);

    ref_push_string($1->u.sval.u.string);
    if (call_handle_inherit($1->u.sval.u.string)) {
      $$=mksvaluenode(Pike_sp-1);
      pop_stack();
    }
    else
      $$=mknewintnode(0);
    if($$->name) free_string($$->name);
    add_ref( $$->name=Pike_sp[-1].u.string );
    free_node($1);

    STACK_LEVEL_DONE(1);
  }
  | idents
  {
    STACK_LEVEL_START(0);

    if(Pike_compiler->last_identifier)
    {
      ref_push_string(Pike_compiler->last_identifier);
    }else{
      push_constant_text("");
    }
    $$=$1;

    STACK_LEVEL_DONE(1);
  }
  ;

/* NOTE: Pushes the resolved program on the stack. */
program_ref: low_program_ref
  {
    STACK_LEVEL_START(0);

    resolv_program($1);
    free_node($1);

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
    if (($1 & ID_EXTERN) && (Pike_compiler->compiler_pass == 1)) {
      yywarning("Extern declared inherit.");
    }
    if($3 && !(Pike_compiler->new_program->flags & PROGRAM_PASS_1_DONE))
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
  | modifiers TOK_INHERIT error ';' { yyerrok; }
  | modifiers TOK_INHERIT error TOK_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | modifiers TOK_INHERIT error '}' { yyerror("Missing ';'."); }
  ;

import: TOK_IMPORT idents ';'
  {
    resolv_constant($2);
    free_node($2);
    use_module(Pike_sp-1);
    pop_stack();
  }
  | TOK_IMPORT string ';'
  {
    if (call_handle_import($2->u.sval.u.string)) {
      use_module(Pike_sp-1);
      pop_stack();
    }
    free_node($2);
  }
  | TOK_IMPORT error ';' { yyerrok; }
  | TOK_IMPORT error TOK_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_IMPORT error '}' { yyerror("Missing ';'."); }
  ;

constant_name: TOK_IDENTIFIER '=' safe_expr0
  {
    /* This can be made more lenient in the future */

    /* Ugly hack to make sure that $3 is optimized */
    {
      int tmp=Pike_compiler->compiler_pass;
      $3=mknode(F_COMMA_EXPR,$3,0);
      Pike_compiler->compiler_pass=tmp;
    }

    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == 1)) {
      yywarning("Extern declared constant.");
    }

    if(!is_const($3))
    {
      if(Pike_compiler->compiler_pass==2)
	yyerror("Constant definition is not constant.");
      else
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
	  pop_n_elems(DO_NOT_WARN((INT32)(tmp - 1)));
	}
      } else {
	push_undefined();
      }
      add_constant($1->u.sval.u.string, Pike_sp-1,
		   Pike_compiler->current_modifiers & ~ID_EXTERN);
      pop_stack();
    }
    if($3) free_node($3);
    free_node($1);
  }
  | bad_identifier '=' safe_expr0 { if ($3) free_node($3); }
  | error '=' safe_expr0 { if ($3) free_node($3); }
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
    $$ = check_node_hash(mknode(F_COMMA_EXPR,$1,mknode(F_RETURN,mkintnode(0),0)));
    if ($1) COPY_LINE_NUMBER_INFO ($$, $1);
  }
  | ';' { $$ = NULL; }
  | TOK_LEX_EOF { yyerror("Expected ';'."); $$ = NULL; }
  | error { $$ = NULL; }
  ;


type_or_error: simple_type
  {
#ifdef PIKE_DEBUG
    check_type_string(check_node_hash($1)->u.sval.u.type);
#endif /* PIKE_DEBUG */
    if(Pike_compiler->compiler_frame->current_type)
      free_type(Pike_compiler->compiler_frame->current_type); 
    copy_pike_type(Pike_compiler->compiler_frame->current_type,
		   $1->u.sval.u.type);
    free_node($1);
  }
  ;

open_paren_with_line_info: '('
  {
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
  ;

close_paren_or_missing: ')'
  {
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
  | /* empty */
  {
    yyerror("Missing ')'.");
    /* Used to hold line-number info */
    $$ = mkintnode(0);
  }
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

push_compiler_frame0: /* empty */
  {
    push_compiler_frame(SCOPE_LOCAL);

    if(!Pike_compiler->compiler_frame->previous ||
       !Pike_compiler->compiler_frame->previous->current_type)
    {
      yyerror("Internal compiler fault.");
      copy_pike_type(Pike_compiler->compiler_frame->current_type,
		mixed_type_string);
    }else{
      copy_pike_type(Pike_compiler->compiler_frame->current_type,
		Pike_compiler->compiler_frame->previous->current_type);
    }
  }
  ;

def: modifiers type_or_error optional_stars TOK_IDENTIFIER push_compiler_frame0
  '(' arguments close_paren_or_missing
  {
    int e;
    /* construct the function type */
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    if ($3 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while(--$3>=0) push_type(T_ARRAY);
    
    if(Pike_compiler->compiler_frame->current_return_type)
      free_type(Pike_compiler->compiler_frame->current_return_type);
    Pike_compiler->compiler_frame->current_return_type = compiler_pop_type();
    
    push_finished_type(Pike_compiler->compiler_frame->current_return_type);
    
    e=$7-1;
    if(Pike_compiler->varargs)
    {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      e--;
      pop_type_stack(T_ARRAY);
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--)
    {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      push_type(T_FUNCTION);
    }

    {
      struct pike_type *s=compiler_pop_type();
      int i = isidentifier($4->u.sval.u.string);

      if (Pike_compiler->compiler_pass == 1) {
	if ($1 & ID_VARIANT) {
	  /* FIXME: Lookup the type of any existing variant */
	  /* Or the types. */
	  fprintf(stderr, "Pass %d: Identifier %s:\n",
		  Pike_compiler->compiler_pass, $4->u.sval.u.string->str);

	  if (i >= 0) {
	    struct identifier *id = ID_FROM_INT(Pike_compiler->new_program, i);
	    if (id) {
	      struct pike_type *new_type;
	      fprintf(stderr, "Defined, type:\n");
#ifdef PIKE_DEBUG
	      simple_describe_type(id->type);
#endif

	      new_type = or_pike_types(s, id->type, 1);
	      free_type(s);
	      s = new_type;

	      fprintf(stderr, "Resulting type:\n");
#ifdef PIKE_DEBUG
	      simple_describe_type(s);
#endif
	    } else {
	      my_yyerror("Lost identifier %s (%d).",
			 $4->u.sval.u.string->str, i);
	    }
	  } else {
	    fprintf(stderr, "Not defined.\n");
	  }
	  fprintf(stderr, "New type:\n");
#ifdef PIKE_DEBUG
	  simple_describe_type(s);
#endif
	}
      } else {
	/* FIXME: Second pass reuses the type from the end of
	 * the first pass if this is a variant function.
	 */
	if (i >= 0) {
	  if (Pike_compiler->new_program->identifier_references[i].id_flags &
	      ID_VARIANT) {
	    struct identifier *id = ID_FROM_INT(Pike_compiler->new_program, i);
	    fprintf(stderr, "Pass %d: Identifier %s:\n",
		    Pike_compiler->compiler_pass, $4->u.sval.u.string->str);

	    free_type(s);
	    copy_pike_type(s, id->type);

	    fprintf(stderr, "Resulting type:\n");
#ifdef PIKE_DEBUG
	    simple_describe_type(s);
#endif
	  }
	} else {
	  my_yyerror("Identifier %s lost after first pass.",
		     $4->u.sval.u.string->str);
	}
      }

      $<n>$ = mktypenode(s);
      free_type(s);
    }


/*    if(Pike_compiler->compiler_pass==1) */
    {
      /* FIXME:
       * set current_function_number for local functions as well
       */
      Pike_compiler->compiler_frame->current_function_number=
	define_function(check_node_hash($4)->u.sval.u.string,
			check_node_hash($<n>$)->u.sval.u.type,
			$1 & (~ID_EXTERN),
			IDENTIFIER_PIKE_FUNCTION |
			(Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
			0,
			OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);

      Pike_compiler->varargs=0;

      if ($1 & ID_VARIANT) {
	fprintf(stderr, "Function number: %d\n",
		Pike_compiler->compiler_frame->current_function_number);
      }
    }
  }
  block_or_semi
  {
    int e;
    if($10)
    {
      int f;
      node *check_args = NULL;
      struct pike_string *save_file = lex.current_file;
      int save_line = lex.current_line;
      int num_required_args = 0;
      struct identifier *i;
      lex.current_file = $4->current_file;
      lex.current_line = $4->line_number;

      if (($1 & ID_EXTERN) && (Pike_compiler->compiler_pass == 1)) {
	yywarning("Extern declared function definition.");
      }

      for(e=0; e<$7; e++)
      {
	if(!Pike_compiler->compiler_frame->variable[e].name ||
	   !Pike_compiler->compiler_frame->variable[e].name->len)
	{
	  my_yyerror("Missing name for argument %d.",e);
	} else {
	  if (Pike_compiler->compiler_pass == 2) {
	    if ($1 & ID_VARIANT) {
	      struct pike_type *arg_type =
		Pike_compiler->compiler_frame->variable[e].type;

	      /* FIXME: Generate code that checks the arguments. */
	      /* If there is a bad argument, call the fallback, and return. */
	      if (! pike_types_le(void_type_string, arg_type)) {
		/* Argument my not be void.
		 * ie it's required.
		 */
		num_required_args++;
	      }
	    } else {
	      /* FIXME: Should probably use some other flag. */
	      if ((runtime_options & RUNTIME_CHECK_TYPES) &&
		  (Pike_compiler->compiler_frame->variable[e].type !=
		   mixed_type_string)) {
		node *local_node;

		/* fprintf(stderr, "Creating soft cast node for local #%d\n", e);*/

		local_node = mklocalnode(e, 0);

		/* The following is needed to go around the optimization in
		 * mksoftcastnode().
		 */
		free_type(local_node->type);
		copy_pike_type(local_node->type, mixed_type_string);

		check_args =
		  mknode(F_COMMA_EXPR, check_args,
			 mksoftcastnode(Pike_compiler->compiler_frame->variable[e].type,
					local_node));
	      }
	    }
	  }
	}
      }

      if ($1 & ID_VARIANT) {
	struct pike_string *bad_arg_str;
	MAKE_CONST_STRING(bad_arg_str,
			  "Bad number of arguments!\n");

	fprintf(stderr, "Required args: %d\n", num_required_args);

	check_args =
	  mknode('?',
		 mkopernode("`<",
			    mkefuncallnode("query_num_arg", NULL),
			    mkintnode(num_required_args)),
		 mknode(':',
			mkefuncallnode("throw",
				       mkefuncallnode("aggregate",
						      mkstrnode(bad_arg_str))),
			NULL));
      }

      {
	int l = $10->line_number;
	struct pike_string *f = $10->current_file;
	if (check_args) {
	  /* Prepend the arg checking code. */
	  $10 = mknode(F_COMMA_EXPR, mknode(F_POP_VALUE, check_args, NULL), $10);
	}
	lex.current_line = l;
	lex.current_file = f;
      }

      f=dooptcode(check_node_hash($4)->u.sval.u.string,
		  check_node_hash($10),
		  check_node_hash($<n>9)->u.sval.u.type,
		  $1);

      i = ID_FROM_INT(Pike_compiler->new_program, f);
      i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

      if ($1 & ID_VARIANT) {
	fprintf(stderr, "Function number: %d\n", f);
      }

#ifdef PIKE_DEBUG
      if(Pike_interpreter.recoveries &&
	 ((Pike_sp - Pike_interpreter.evaluator_stack) <
	  Pike_interpreter.recoveries->stack_pointer))
	Pike_fatal("Stack error (underflow)\n");

      if((Pike_compiler->compiler_pass == 1) &&
	 (f != Pike_compiler->compiler_frame->current_function_number)) {
	fprintf(stderr, "define_function()/do_opt_code() failed for symbol %s\n",
		$4->u.sval.u.string->str);
	dump_program_desc(Pike_compiler->new_program);
	Pike_fatal("define_function screwed up! %d != %d\n",
	      f, Pike_compiler->compiler_frame->current_function_number);
      }
#endif

      lex.current_line = save_line;
      lex.current_file = save_file;
    }
    pop_compiler_frame();
    free_node($4);
    free_node($8);
    free_node($<n>9);
  }
  | modifiers type_or_error optional_stars TOK_IDENTIFIER push_compiler_frame0
    error
  {
    pop_compiler_frame();
    free_node($4);
  }
  | modifiers type_or_error optional_stars bad_identifier
  {
    free_type(compiler_pop_type());
  }
    '(' arguments ')' block_or_semi
  {
    if ($9) free_node($9);
  }
  | modifiers type_or_error name_list ';' {}
  | inheritance {}
  | import {}
  | constant {}
  | class { free_node($1); }
  | enum { free_node($1); }
  | typedef {}
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
      $<number>$=lex.pragmas;
      lex.pragmas|=$1;
    }
      program
   close_brace_or_eof
    {
      lex.pragmas=$<number>3;
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

new_arg_name: type7 optional_dot_dot_dot optional_identifier
  {
    if(Pike_compiler->varargs) yyerror("Can't define more arguments after ...");

    if($2)
    {
      push_type(T_ARRAY);
      Pike_compiler->varargs=1;
    }

    if(!$3)
    {
      $3=mkstrnode(empty_pike_string);
    }

    if($3->u.sval.u.string->len &&
       islocal($3->u.sval.u.string) >= 0)
      my_yyerror("Variable '%s' appears twice in argument list.",
		 $3->u.sval.u.string->str);
    
    add_local_name($3->u.sval.u.string, compiler_pop_type(),0);
    free_node($3);
  }
  ;

func_args: '(' arguments close_paren_or_missing
  {
    free_node($3);
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
    TOK_NO_MASK    { $$ = ID_NOMASK; }
  | TOK_FINAL_ID   { $$ = ID_NOMASK; }
  | TOK_STATIC     { $$ = ID_STATIC; }
  | TOK_EXTERN     { $$ = ID_EXTERN; }
  | TOK_OPTIONAL   { $$ = ID_OPTIONAL; }
  | TOK_PRIVATE    { $$ = ID_PRIVATE | ID_STATIC; }
  | TOK_LOCAL_ID   { $$ = ID_INLINE; }
  | TOK_PUBLIC     { $$ = ID_PUBLIC; }
  | TOK_PROTECTED  { $$ = ID_PROTECTED; }
  | TOK_INLINE     { $$ = ID_INLINE; }
  | TOK_VARIANT    { $$ = ID_VARIANT; }
  ;

magic_identifiers1:
    TOK_NO_MASK    { $$ = "nomask"; }
  | TOK_FINAL_ID   { $$ = "final"; }
  | TOK_STATIC     { $$ = "static"; }
  | TOK_EXTERN	   { $$ = "extern"; }
  | TOK_PRIVATE    { $$ = "private"; }
  | TOK_LOCAL_ID   { $$ = "local"; }
  | TOK_PUBLIC     { $$ = "public"; }
  | TOK_PROTECTED  { $$ = "protected"; }
  | TOK_INLINE     { $$ = "inline"; }
  | TOK_OPTIONAL   { $$ = "optional"; }
  | TOK_VARIANT    { $$ = "variant"; }
  ;

magic_identifiers2:
    TOK_VOID_ID       { $$ = "void"; }
  | TOK_MIXED_ID      { $$ = "mixed"; }
  | TOK_ARRAY_ID      { $$ = "array"; }
  | TOK_MAPPING_ID    { $$ = "mapping"; }
  | TOK_MULTISET_ID   { $$ = "multiset"; }
  | TOK_OBJECT_ID     { $$ = "object"; }
  | TOK_FUNCTION_ID   { $$ = "function"; }
  | TOK_PROGRAM_ID    { $$ = "program"; }
  | TOK_STRING_ID     { $$ = "string"; }
  | TOK_FLOAT_ID      { $$ = "float"; }
  | TOK_INT_ID        { $$ = "int"; }
  | TOK_ENUM	      { $$ = "enum"; }
  | TOK_TYPEDEF       { $$ = "typedef"; }
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

magic_identifier: TOK_IDENTIFIER 
  | magic_identifiers
  {
    struct pike_string *tmp=make_shared_string($1);
    $$=mkstrnode(tmp);
    free_string(tmp);
  }
  ;

modifiers: modifier_list
 {
   $$=Pike_compiler->current_modifiers=$1 | (lex.pragmas & ID_MODIFIER_MASK);
 }
 ;

modifier_list: /* empty */ { $$ = 0; }
  | modifier modifier_list { $$ = $1 | $2; }
  ;

optional_stars: optional_stars '*' { $$=$1 + 1; }
  | /* empty */ { $$=0; }
  ;

cast: open_paren_with_line_info type ')'
    {
      struct pike_type *s = compiler_pop_type();
      $$ = mktypenode(s);
      free_type(s);
      COPY_LINE_NUMBER_INFO ($$, $1);
      free_node ($1);
    }
    ;

soft_cast: open_bracket_with_line_info type ']'
    {
      struct pike_type *s = compiler_pop_type();
      $$ = mktypenode(s);
      free_type(s);
      COPY_LINE_NUMBER_INFO ($$, $1);
      free_node ($1);
    }
    ;

full_type: type4
  | full_type '*'
  {
    if (Pike_compiler->compiler_pass == 2) {
       yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    push_type(T_ARRAY);
  }
  ;

type6: type | identifier_type ;
  
type: type '*'
  {
    if (Pike_compiler->compiler_pass == 2) {
       yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    push_type(T_ARRAY);
  }
  | type2
  ;

type7: type7 '*'
  {
    if (Pike_compiler->compiler_pass == 2) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    push_type(T_ARRAY);
  }
  | type4
  ;

simple_type: type4
  {
    struct pike_type *s = compiler_pop_type();
    $$ = mktypenode(s);
#ifdef PIKE_DEBUG
    if ($$->u.sval.u.type != s) {
      Pike_fatal("mktypenode(%p) created node with %p\n", s, $$->u.sval.u.type);
    }
#endif /* PIKE_DEBUG */
    free_type(s);
  }
  ;

simple_type2: type2
  {
    struct pike_type *s = compiler_pop_type();
    $$ = mktypenode(s);
#ifdef PIKE_DEBUG
    if ($$->u.sval.u.type != s) {
      Pike_fatal("mktypenode(%p) created node with %p\n", s, $$->u.sval.u.type);
    }
#endif /* PIKE_DEBUG */
    free_type(s);
  }
  ;

simple_identifier_type: identifier_type
  {
    struct pike_type *s = compiler_pop_type();
    $$ = mktypenode(s);
#ifdef PIKE_DEBUG
    if ($$->u.sval.u.type != s) {
      Pike_fatal("mktypenode(%p) created node with %p\n", s, $$->u.sval.u.type);
    }
#endif /* PIKE_DEBUG */
    free_type(s);
  }
  ;

type4: type4 '|' type8 { push_type(T_OR); }
  | type8
  ;

type2: type2 '|' type8 { push_type(T_OR); }
  | basic_type 
  ;

type8: basic_type | identifier_type ;

basic_type:
    TOK_FLOAT_ID                      { push_type(T_FLOAT); }
  | TOK_VOID_ID                       { push_type(T_VOID); }
  | TOK_MIXED_ID                      { push_type(T_MIXED); }
  | TOK_STRING_ID                     { push_type(T_STRING); }
  | TOK_INT_ID      opt_int_range     {}
  | TOK_MAPPING_ID  opt_mapping_type  {}
  | TOK_FUNCTION_ID opt_function_type {}
  | TOK_OBJECT_ID   opt_object_type   {}
  | TOK_PROGRAM_ID  opt_object_type   { push_type(T_PROGRAM); }
  | TOK_ARRAY_ID    opt_array_type    { push_type(T_ARRAY); }
  | TOK_MULTISET_ID opt_array_type    { push_type(T_MULTISET); }
  ;

identifier_type: idents
  { 
    resolv_constant($1);

    if (Pike_sp[-1].type == T_TYPE) {
      /* "typedef" */
      push_finished_type(Pike_sp[-1].u.type);
    } else {
      /* object type */
      struct program *p = NULL;

      if (Pike_sp[-1].type == T_OBJECT) {
	if(!Pike_sp[-1].u.object->prog)
	{
	  pop_stack();
	  push_int(0);
	  yyerror("Destructed object used as program identifier.");
	}else{
	  int f=FIND_LFUN(Pike_sp[-1].u.object->prog,LFUN_CALL);
	  if(f!=-1)
	  {
	    Pike_sp[-1].subtype=f;
	    Pike_sp[-1].type=T_FUNCTION;
	  }else{
	    extern void f_object_program(INT32);
	    if (Pike_compiler->compiler_pass == 2 && !TEST_COMPAT (7, 4)) {
	      yywarning("Using object as program identifier.");
	    }
	    f_object_program(1);
	  }
	}
      }

      switch(Pike_sp[-1].type) {
	case T_FUNCTION:
        if((p = program_from_function(Pike_sp-1)))
	  push_object_type(0, p?(p->id):0);
	else
	{
	  struct pike_type *a,*b;
	  a=get_type_of_svalue(Pike_sp-1);
	  b=check_call(function_type_string,a,0);
	  push_finished_type(b);
	  free_type(a);
	  free_type(b);
	}
	break;

      
      default:
	if (Pike_compiler->compiler_pass!=1)
	  my_yyerror("Illegal program identifier (type: %s).",
		     get_name_of_type(Pike_sp[-1].type));
	pop_stack();
	push_int(0);
	push_object_type(0, 0);
	break;
	
      case T_PROGRAM:
	p = Pike_sp[-1].u.program;
	push_object_type(0, p?(p->id):0);
	break;
      }
    }
    /* Attempt to name the type. */
    if (Pike_compiler->last_identifier) {
      push_type_name(Pike_compiler->last_identifier);
    }
    pop_stack();
    free_node($1);
  }
  ;

number_or_maxint: /* Empty */
  {
    $$ = mkintnode(MAX_INT_TYPE);
  }
  | TOK_NUMBER
  | '-' TOK_NUMBER
  {
#ifdef PIKE_DEBUG
    if (($2->token != F_CONSTANT) || ($2->u.sval.type != T_INT)) {
      Pike_fatal("Unexpected number in negative int-range.\n");
    }
#endif /* PIKE_DEBUG */
    $$ = mkintnode(-($2->u.sval.u.integer));
    free_node($2);
  }
  ;

number_or_minint: /* Empty */
  {
    $$ = mkintnode(MIN_INT_TYPE);
  }
  | TOK_NUMBER
  | '-' TOK_NUMBER
  {
#ifdef PIKE_DEBUG
    if (($2->token != F_CONSTANT) || ($2->u.sval.type != T_INT)) {
      Pike_fatal("Unexpected number in negative int-range.\n");
    }
#endif /* PIKE_DEBUG */
    $$ = mkintnode(-($2->u.sval.u.integer));
    free_node($2);
  }
  ;

expected_dot_dot: TOK_DOT_DOT
  | TOK_DOT_DOT_DOT
  {
    yyerror("Elipsis ('...') where range indicator ('..') expected.");
  }
  ;

opt_int_range: /* Empty */
  {
    push_int_type(MIN_INT_TYPE, MAX_INT_TYPE);
  }
  | '(' number_or_minint expected_dot_dot number_or_maxint ')'
  {
    INT_TYPE min = MIN_INT_TYPE;
    INT_TYPE max = MAX_INT_TYPE;

    /* FIXME: Check that $4 is >= $2. */
    if($4->token == F_CONSTANT) {
      if ($4->u.sval.type == T_INT) {
	max = $4->u.sval.u.integer;
#ifdef AUTO_BIGNUM
      } else if (is_bignum_object_in_svalue(&$4->u.sval)) {
	push_int(0);
	if (is_lt(&$4->u.sval, Pike_sp-1)) {
	  max = MIN_INT_TYPE;
	}
	pop_stack();
#endif /* AUTO_BIGNUM */
      }
    }

    if($2->token == F_CONSTANT) {
      if ($2->u.sval.type == T_INT) {
	min = $2->u.sval.u.integer;
#ifdef AUTO_BIGNUM
      } else if (is_bignum_object_in_svalue(&$2->u.sval)) {
	push_int(0);
	if (is_lt(Pike_sp-1, &$2->u.sval)) {
	  min = MAX_INT_TYPE;
	}
	pop_stack();
#endif /* AUTO_BIGNUM */
      }
    }

    push_int_type(min, max);

    free_node($2);
    free_node($4);
  }
  ;

opt_object_type:  /* Empty */ { push_object_type(0, 0); }
  | {
#ifdef PIKE_DEBUG
      $<ptr>$ = Pike_sp;
#endif /* PIKE_DEBUG */
    }
    '(' program_ref ')'
  {
    /* NOTE: On entry, there are two items on the stack:
     *   Pike_sp-2:	Name of the program reference (string).
     *   Pike_sp-1:	The resolved program (program|function|zero).
     */
    struct program *p=program_from_svalue(Pike_sp-1);

#ifdef PIKE_DEBUG
    if ($<ptr>1 != (Pike_sp - 2)) {
      Pike_fatal("Unexpected stack depth: %p != %p\n",
		 $<n>1, Pike_sp-2);
    }
#endif /* PIKE_DEBUG */

    if(!p) {
      if (Pike_compiler->compiler_pass!=1) {
	if ((Pike_sp[-2].type == T_STRING) && (Pike_sp[-2].u.string->len > 0) &&
	    (Pike_sp[-2].u.string->len < 256)) {
	  my_yyerror("Not a valid program specifier: '%s'",
		     Pike_sp[-2].u.string->str);
	} else {
	  yyerror("Not a valid program specifier.");
	}
      }
    }
    push_object_type(0, p?(p->id):0);
    /* Attempt to name the type. */
    if (Pike_sp[-2].type == T_STRING) {
      push_type_name(Pike_sp[-2].u.string);
    }
    pop_n_elems(2);
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
	if (Pike_compiler->compiler_pass == 1) {
	  yyerror("Missing type before ... .");
	}
	push_type(T_MIXED);
      }
    }else{
      push_type(T_VOID);
    }
  }
  type7 ')'
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

   push_type(T_ZERO);
   push_type(T_VOID);
   push_type(T_OR);

   push_type(T_MANY);
  }
  ;

function_type_list: /* Empty */ optional_comma { $$=0; }
  | function_type_list2 optional_comma { $$=!$2; }
  ;

function_type_list2: type7 { $$=1; }
  | function_type_list2 ','
  {
  }
  type7
  ;

opt_array_type: '(' type7 ')'
  |  { push_type(T_MIXED); }
  ;

opt_mapping_type: '('
  { 
  }
  type7 ':'
  { 
  }
  type7
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

new_name: optional_stars TOK_IDENTIFIER
  {
    struct pike_type *type;
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    type=compiler_pop_type();
    define_variable($2->u.sval.u.string, type,
		    Pike_compiler->current_modifiers);
    free_type(type);
    free_node($2);
  }
  | optional_stars bad_identifier {}
  | optional_stars TOK_IDENTIFIER '='
  {
    struct pike_type *type;
    push_finished_type(Pike_compiler->compiler_frame->current_type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    type=compiler_pop_type();
    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == 1)) {
      yywarning("Extern declared variable has initializer.");
    }
    $<number>$=define_variable($2->u.sval.u.string, type,
			       Pike_compiler->current_modifiers & (~ID_EXTERN));
    free_type(type);
  }
  expr0
  {
    Pike_compiler->init_node=mknode(F_COMMA_EXPR,Pike_compiler->init_node,
		     mkcastnode(void_type_string,
				mknode(F_ASSIGN,$5,
				       mkidentifiernode($<number>4))));
    free_node($2);
  }
  | optional_stars TOK_IDENTIFIER '=' error
  {
    free_node($2);
  }
  | optional_stars TOK_IDENTIFIER '=' TOK_LEX_EOF
  {
    yyerror("Unexpected end of file in variable definition.");
    free_node($2);
  }
  | optional_stars bad_identifier '=' expr0
  {
    free_node($4);
  }
  ;


new_local_name: optional_stars TOK_IDENTIFIER
  {
    int id;
    push_finished_type($<n>0->u.sval.u.type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    id = add_local_name($2->u.sval.u.string, compiler_pop_type(),0);
    if (id >= 0)
      $$=mknode(F_ASSIGN,mkintnode(0),mklocalnode(id,0));
    else
      $$ = 0;
    free_node($2);
  }
  | optional_stars bad_identifier { $$=0; }
  | optional_stars TOK_IDENTIFIER '=' expr0 
  {
    int id;
    push_finished_type($<n>0->u.sval.u.type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    id = add_local_name($2->u.sval.u.string, compiler_pop_type(),0);
    if (id >= 0)
      $$=mknode(F_ASSIGN,$4,mklocalnode(id,0));
    else
      $$ = 0;
    free_node($2);
  }
  | optional_stars bad_identifier '=' expr0
  {
    free_node($4);
    $$=0;
  }
  | optional_stars TOK_IDENTIFIER '=' error
  {
    free_node($2);
    /* No yyerok here since we aren't done yet. */
    $$=0;
  }
  | optional_stars TOK_IDENTIFIER '=' TOK_LEX_EOF
  {
    yyerror("Unexpected end of file in local variable definition.");
    free_node($2);
    /* No yyerok here since we aren't done yet. */
    $$=0;
  }
  ;

new_local_name2: TOK_IDENTIFIER
  {
    int id;
    add_ref($<n>0->u.sval.u.type);
    id = add_local_name($1->u.sval.u.string, $<n>0->u.sval.u.type, 0);
    if (id >= 0)
      $$=mknode(F_ASSIGN,mkintnode(0),mklocalnode(id,0));
    else
      $$ = 0;
    free_node($1);
  }
  | bad_identifier { $$=0; }
  | TOK_IDENTIFIER '=' safe_expr0
  {
    int id;
    add_ref($<n>0->u.sval.u.type);
    id = add_local_name($1->u.sval.u.string, $<n>0->u.sval.u.type, 0);
    if (id >= 0)
      $$=mknode(F_ASSIGN,$3, mklocalnode(id,0));
    else
      $$ = 0;
    free_node($1);
  }
  | bad_identifier '=' safe_expr0 { $$=$3; }
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
  statements end_block
  {
    unuse_modules(Pike_compiler->num_used_modules - $<number>1);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
    if ($5) COPY_LINE_NUMBER_INFO ($5, $3);
    free_node ($3);
    $$=$5;
  }
  ;

end_block: '}'
  | TOK_LEX_EOF
  {
    yyerror("Missing '}'.");
    yyerror("Unexpected end of file.");
  }
  ;

failsafe_block: block
              | error { $$=0; }
              | TOK_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
              ;


local_name_list: new_local_name
  | local_name_list ',' { $<n>$=$<n>0; } new_local_name
    { $$ = mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $1), $4); }
  ;

local_name_list2: new_local_name2
  | local_name_list2 ',' { $<n>$=$<n>0; } new_local_name
    { $$ = mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $1), $4); }
  ;


local_constant_name: TOK_IDENTIFIER '=' safe_expr0
  {
    struct pike_type *type;

    /* Ugly hack to make sure that $3 is optimized */
    {
      int tmp=Pike_compiler->compiler_pass;
      $3=mknode(F_COMMA_EXPR,$3,0);
      optimize_node($3);
      Pike_compiler->compiler_pass=tmp;
      type=$3->u.node.a->type;
    }

    if(!is_const($3))
    {
      if(Pike_compiler->compiler_pass==2)
	yyerror("Constant definition is not constant.");
    }else{
      ptrdiff_t tmp=eval_low($3,1);
      if(tmp < 1)
      {
	yyerror("Error in constant definition.");
      }else{
	pop_n_elems(DO_NOT_WARN((INT32)(tmp - 1)));
	if($3) free_node($3);
	$3=mksvaluenode(Pike_sp-1);
	type=$3->type;
	pop_stack();
      }
    }
    if(!type) type = mixed_type_string;
    add_ref(type);
    low_add_local_name(Pike_compiler->compiler_frame, /*->previous,*/
		       $1->u.sval.u.string,
		       type, $3);
    free_node($1);
  }
  | bad_identifier '=' safe_expr0 { if ($3) free_node($3); }
  | error '=' safe_expr0 { if ($3) free_node($3); }
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

statement_with_semicolon: unused2 optional_block
  {
    if($2)
    {
      $$=recursive_add_call_arg($1,$2);
    }else{
      $$=$1;
    }
  }
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
    COPY_LINE_NUMBER_INFO ($$, $1);
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

push_compiler_frame1: /* empty */
  {
    push_compiler_frame(SCOPE_LOCAL);
  }
  ;

lambda: TOK_LAMBDA line_number_info push_compiler_frame1
  {
    debug_malloc_touch(Pike_compiler->compiler_frame->current_return_type);
    if(Pike_compiler->compiler_frame->current_return_type)
      free_type(Pike_compiler->compiler_frame->current_return_type);
    copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
		   any_type_string);
  }
  func_args
  {
    $<number>$ = Pike_compiler->varargs;
    Pike_compiler->varargs = 0;
  }
  failsafe_block
  {
    struct pike_type *type;
    char buf[80];
    int f,e;
    struct pike_string *name;
    struct pike_string *save_file = lex.current_file;
    int save_line = lex.current_line;
    lex.current_file = $2->current_file;
    lex.current_line = $2->line_number;

    debug_malloc_touch($7);
    $7=mknode(F_COMMA_EXPR,$7,mknode(F_RETURN,mkintnode(0),0));
    if (Pike_compiler->compiler_pass == 2)
      /* Doing this in pass 1 might induce too strict checks on types
       * in cases where we got placeholders. */
      type=find_return_type($7);
    else
      type = NULL;

    if(type) {
      push_finished_type(type);
      free_type(type);
    } else
      push_type(T_MIXED);
    
    e=$5-1;
    if($<number>6)
    {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      e--;
      pop_type_stack(T_ARRAY);
    }else{
      push_type(T_VOID);
    }
    Pike_compiler->varargs=0;
    push_type(T_MANY);
    for(; e>=0; e--) {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      push_type(T_FUNCTION);
    }
    
    type=compiler_pop_type();

    sprintf(buf,"__lambda_%ld_%ld_line_%d",
	    (long)Pike_compiler->new_program->id,
	    (long)(Pike_compiler->local_class_counter++ & 0xffffffff), /* OSF/1 cc bug. */
	    (int) lex.current_line);
    name=make_shared_string(buf);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, buf, (long)Pike_compiler->new_program->id, Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */
    if(Pike_compiler->compiler_pass == 2)
      Pike_compiler->compiler_frame->current_function_number=isidentifier(name);

    f=dooptcode(name,
		$7,
		type,
		ID_STATIC | ID_PRIVATE | ID_INLINE);

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
    free_string(name);
    free_type(type);
    lex.current_line = save_line;
    lex.current_file = save_file;
    free_node ($2);
    pop_compiler_frame();
  }
  | TOK_LAMBDA line_number_info push_compiler_frame1 error
  {
    pop_compiler_frame();
    $$ = mkintnode(0);
    COPY_LINE_NUMBER_INFO($$, $2);
    free_node($2);
  }
  ;

local_function: TOK_IDENTIFIER push_compiler_frame1 func_args 
  {
    char buf[40];
    struct pike_string *name;
    struct pike_type *type;
    int id,e;
    node *n;
    struct identifier *i=0;

    debug_malloc_touch(Pike_compiler->compiler_frame->current_return_type);
    if(Pike_compiler->compiler_frame->current_return_type)
      free_type(Pike_compiler->compiler_frame->current_return_type);
    copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
		   $<n>0->u.sval.u.type);


    /***/
    push_finished_type(Pike_compiler->compiler_frame->current_return_type);
    
    e=$3-1;
    if(Pike_compiler->varargs)
    {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      e--;
      pop_type_stack(T_ARRAY);
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--) {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
    push_type(T_FUNCTION);
    }
    
    type=compiler_pop_type();
    /***/

    sprintf(buf,"__lambda_%ld_%ld_line_%d",
	    (long)Pike_compiler->new_program->id,
	    (long)(Pike_compiler->local_class_counter++ & 0xffffffff), /* OSF/1 cc bug. */
	    (int) $1->line_number);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, buf, (long)Pike_compiler->new_program->id, Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    name=make_shared_string(buf);

    if(Pike_compiler->compiler_pass > 1)
    {
      id=isidentifier(name);
    }else{
      id=define_function(name,
			 type,
			 ID_INLINE,
			 IDENTIFIER_PIKE_FUNCTION |
			 (Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
			 0,
			 OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
    }
    Pike_compiler->varargs=0;
    Pike_compiler->compiler_frame->current_function_number=id;

    n=0;
    if(Pike_compiler->compiler_pass > 1 &&
       (i=ID_FROM_INT(Pike_compiler->new_program, id)))
    {
      if(i->identifier_flags & IDENTIFIER_SCOPED)
	n = mktrampolinenode(id, Pike_compiler->compiler_frame->previous);
      else
	n = mkidentifiernode(id);
    }

    low_add_local_name(Pike_compiler->compiler_frame->previous,
		       $1->u.sval.u.string, type, n);

    $<number>$=id;
    free_string(name);
  }
  failsafe_block
  {
    int localid;
    struct identifier *i=ID_FROM_INT(Pike_compiler->new_program, $<number>4);
    struct pike_string *save_file = lex.current_file;
    int save_line = lex.current_line;
    lex.current_file = $1->current_file;
    lex.current_line = $1->line_number;

    $5=mknode(F_COMMA_EXPR,$5,mknode(F_RETURN,mkintnode(0),0));

    debug_malloc_touch($5);
    dooptcode(i->name,
	      $5,
	      i->type,
	      ID_STATIC | ID_PRIVATE | ID_INLINE);

    i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

    lex.current_line = save_line;
    lex.current_file = save_file;
    pop_compiler_frame();
    free_node($1);

    /* WARNING: If the local function adds more variables we are screwed */
    /* WARNING2: if add_local_name stops adding local variables at the end,
     *           this has to be fixed.
     */

    localid=Pike_compiler->compiler_frame->current_number_of_locals-1;
    if(Pike_compiler->compiler_frame->variable[localid].def)
    {
      $$=copy_node(Pike_compiler->compiler_frame->variable[localid].def);
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
  | TOK_IDENTIFIER push_compiler_frame1 error
  {
    pop_compiler_frame();
    $$=mkintnode(0);
  }
  ;

local_function2: optional_stars TOK_IDENTIFIER push_compiler_frame1 func_args 
  {
    char buf[40];
    struct pike_string *name;
    struct pike_type *type;
    int id,e;
    node *n;
    struct identifier *i=0;

    /***/
    debug_malloc_touch(Pike_compiler->compiler_frame->current_return_type);
    
    push_finished_type($<n>0->u.sval.u.type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);

    if(Pike_compiler->compiler_frame->current_return_type)
      free_type(Pike_compiler->compiler_frame->current_return_type);
    Pike_compiler->compiler_frame->current_return_type=compiler_pop_type();

    /***/
    push_finished_type(Pike_compiler->compiler_frame->current_return_type);
    
    e=$4-1;
    if(Pike_compiler->varargs)
    {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      e--;
      pop_type_stack(T_ARRAY);
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--) {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      push_type(T_FUNCTION);
    }
    
    type=compiler_pop_type();
    /***/


    sprintf(buf,"__lambda_%ld_%ld_line_%d",
	    (long)Pike_compiler->new_program->id,
	    (long)(Pike_compiler->local_class_counter++ & 0xffffffff), /* OSF/1 cc bug. */
	    (int) $2->line_number);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, buf, (long)Pike_compiler->new_program->id, Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    name=make_shared_string(buf);

    if(Pike_compiler->compiler_pass > 1)
    {
      id=isidentifier(name);
    }else{
      id=define_function(name,
			 type,
			 ID_INLINE,
			 IDENTIFIER_PIKE_FUNCTION|
			 (Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
			 0,
			 OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
    }
    Pike_compiler->varargs=0;
    Pike_compiler->compiler_frame->current_function_number=id;

    n=0;
    if(Pike_compiler->compiler_pass > 1 &&
       (i=ID_FROM_INT(Pike_compiler->new_program, id)))
    {
      if(i->identifier_flags & IDENTIFIER_SCOPED)
	n = mktrampolinenode(id, Pike_compiler->compiler_frame->previous);
      else
	n = mkidentifiernode(id);
    }

    low_add_local_name(Pike_compiler->compiler_frame->previous,
		       $2->u.sval.u.string, type, n);
    $<number>$=id;
    free_string(name);
  }
  failsafe_block
  {
    int localid;
    struct identifier *i=ID_FROM_INT(Pike_compiler->new_program, $<number>5);
    struct pike_string *save_file = lex.current_file;
    int save_line = lex.current_line;
    lex.current_file = $2->current_file;
    lex.current_line = $2->line_number;

    debug_malloc_touch($6);
    $6=mknode(F_COMMA_EXPR,$6,mknode(F_RETURN,mkintnode(0),0));


    debug_malloc_touch($6);
    dooptcode(i->name,
	      $6,
	      i->type,
	      ID_STATIC | ID_PRIVATE | ID_INLINE);

    i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

    lex.current_line = save_line;
    lex.current_file = save_file;
    pop_compiler_frame();
    free_node($2);

    /* WARNING: If the local function adds more variables we are screwed */
    /* WARNING2: if add_local_name stops adding local variables at the end,
     *           this has to be fixed.
     */

    localid=Pike_compiler->compiler_frame->current_number_of_locals-1;
    if(Pike_compiler->compiler_frame->variable[localid].def)
    {
      $$=copy_node(Pike_compiler->compiler_frame->variable[localid].def);
    }else{
      if(Pike_compiler->compiler_frame->lexical_scope & 
	 (SCOPE_SCOPE_USED | SCOPE_SCOPED))
      {
        $$ = mktrampolinenode($<number>5,Pike_compiler->compiler_frame);
      }else{
        $$ = mkidentifiernode($<number>5);
      }
    }
  }
  | optional_stars TOK_IDENTIFIER push_compiler_frame1 error
  {
    pop_compiler_frame();
    free_node($2);
    $$=mkintnode(0);
  }
  ;

create_arg: modifiers type_or_error optional_stars optional_dot_dot_dot TOK_IDENTIFIER
  {
    struct pike_type *type;

    if (Pike_compiler->varargs) {
      yyerror("Can't define more variables after ...");
    }

    push_finished_type(Pike_compiler->compiler_frame->current_type);
    if ($3 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($3--) push_type(T_ARRAY);
    if ($4) {
      push_type(T_ARRAY);
      Pike_compiler->varargs = 1;
    }
    type=compiler_pop_type();

    if(islocal($5->u.sval.u.string) >= 0)
      my_yyerror("Variable '%s' appears twice in create argument list.",
		 $5->u.sval.u.string->str);

    /* Add the identifier both globally and locally. */
    define_variable($5->u.sval.u.string, type,
		    Pike_compiler->current_modifiers);
    add_local_name($5->u.sval.u.string, type, 0);

    /* free_type(type); */
    free_node($5);
    $$=0;
  }
  | modifiers type_or_error optional_stars bad_identifier { $$=0; }
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

push_compiler_frame01: /* empty */
  {
    push_compiler_frame(SCOPE_LOCAL);
  }
  ;

optional_create_arguments: /* empty */ { $$ = 0; }
  | '(' push_compiler_frame01 create_arguments close_paren_or_missing
  {
    int e;
    node *create_code = NULL;
    struct pike_type *type = NULL;
    struct pike_string *create_string = NULL;
    int f;

    MAKE_CONST_STRING(create_string, "create");

    /* First: Deduce the type for the create() function. */
    push_type(T_VOID); /* Return type. */
    e = $3-1;
    if (Pike_compiler->varargs) {
      /* Varargs */
      push_finished_type(Pike_compiler->compiler_frame->variable[e--].type);
      pop_type_stack(T_ARRAY); /* Pop one level of array. */
    } else {
      /* Not varargs. */
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e >= 0; e--) {
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
      push_type(T_FUNCTION);
    }

    type = compiler_pop_type();

    /* Second: Declare the function. */

    Pike_compiler->compiler_frame->current_function_number=
      define_function(create_string, type,
		      ID_INLINE | ID_STATIC,
		      IDENTIFIER_PIKE_FUNCTION |
		      (Pike_compiler->varargs?IDENTIFIER_VARARGS:0),
		      0,
		      OPT_SIDE_EFFECT);

    Pike_compiler->varargs = 0;

    /* Third: Generate the initialization code.
     *
     * global_arg = [type]local_arg;
     * [,..]
     */

    for(e=0; e<$3; e++)
    {
      if(!Pike_compiler->compiler_frame->variable[e].name ||
	 !Pike_compiler->compiler_frame->variable[e].name->len)
      {
	my_yyerror("Missing name for argument %d.",e);
      } else {
	node *local_node = mklocalnode(e, 0);

	/* FIXME: Should probably use some other flag. */
	if ((runtime_options & RUNTIME_CHECK_TYPES) &&
	    (Pike_compiler->compiler_pass == 2) &&
	    (Pike_compiler->compiler_frame->variable[e].type !=
	     mixed_type_string)) {
	  /* fprintf(stderr, "Creating soft cast node for local #%d\n", e);*/

	  /* The following is needed to go around the optimization in
	   * mksoftcastnode().
	   */
	  free_type(local_node->type);
	  copy_pike_type(local_node->type, mixed_type_string);
	  
	  local_node = mksoftcastnode(Pike_compiler->compiler_frame->
				      variable[e].type, local_node);
	}
	create_code =
	  mknode(F_COMMA_EXPR, create_code,
		 mknode(F_ASSIGN, local_node,
			mkidentifiernode(isidentifier(Pike_compiler->
						      compiler_frame->
						      variable[e].name))));
      }
    }

    /* Fourth: Add a return 0; at the end. */

    create_code = mknode(F_COMMA_EXPR,
			 mknode(F_POP_VALUE, create_code, NULL),
			 mknode(F_RETURN, mkintnode(0), NULL));

    /* Fifth: Define the function. */

    f=dooptcode(create_string, check_node_hash(create_code),
		type, ID_STATIC);

#ifdef PIKE_DEBUG
    if(Pike_interpreter.recoveries &&
       Pike_sp-Pike_interpreter.evaluator_stack < Pike_interpreter.recoveries->stack_pointer)
      Pike_fatal("Stack error (underflow)\n");

    if(Pike_compiler->compiler_pass == 1 &&
       f!=Pike_compiler->compiler_frame->current_function_number)
      Pike_fatal("define_function screwed up! %d != %d\n",
	    f, Pike_compiler->compiler_frame->current_function_number);
#endif

    /* Done. */

    pop_compiler_frame();
    free_node($4);
    free_type(type);
  }
  ;

failsafe_program: '{' program end_block
                | error { yyerrok; }
                | TOK_LEX_EOF
                {
		  yyerror("End of file where program definition expected.");
		}
                ;

class: modifiers TOK_CLASS line_number_info optional_identifier
  {
    if(!$4)
    {
      struct pike_string *s;
      char buffer[42];
      sprintf(buffer,"__class_%ld_%ld_line_%d",
	      (long)Pike_compiler->new_program->id,
	      (long)Pike_compiler->local_class_counter++,
	      (int) $3->line_number);
      s=make_shared_string(buffer);
      $4=mkstrnode(s);
      free_string(s);
      $1|=ID_STATIC | ID_PRIVATE | ID_INLINE;
    }
    /* fprintf(stderr, "LANGUAGE.YACC: CLASS start\n"); */
    if(Pike_compiler->compiler_pass==1)
    {
      if ($1 & ID_EXTERN) {
	yywarning("Extern declared class definition.");
      }
      low_start_new_program(0, 1, $4->u.sval.u.string,
			    $1,
			    &$<number>$);

      /* fprintf(stderr, "Pass 1: Program %s has id %d\n",
	 $4->u.sval.u.string->str, Pike_compiler->new_program->id); */

      store_linenumber($3->line_number, $3->current_file);
      debug_malloc_name(Pike_compiler->new_program,
			$3->current_file->str,
			$3->line_number);
    }else{
      int i;
      struct identifier *id;
      int tmp=Pike_compiler->compiler_pass;
      i=isidentifier($4->u.sval.u.string);
      if(i<0)
      {
	/* Seriously broken... */
	yyerror("Pass 2: program not defined!");
	low_start_new_program(0, 2, 0,
			      $1,
			      &$<number>$);
      }else{
	id=ID_FROM_INT(Pike_compiler->new_program, i);
	if(IDENTIFIER_IS_CONSTANT(id->identifier_flags))
	{
	  struct svalue *s;
	  if ((id->func.offset >= 0) &&
	      ((s = &PROG_FROM_INT(Pike_compiler->new_program,i)->
		constants[id->func.offset].sval)->type == T_PROGRAM))
	  {
	    low_start_new_program(s->u.program, 2,
				  $4->u.sval.u.string,
				  $1,
				  &$<number>$);

	    /* fprintf(stderr, "Pass 2: Program %s has id %d\n",
	       $4->u.sval.u.string->str, Pike_compiler->new_program->id); */

	  }else{
	    yyerror("Pass 2: constant redefined!");
	    low_start_new_program(0, 2, 0,
				  $1,
				  &$<number>$);
	  }
	}else{
	  yyerror("Pass 2: class constant no longer constant!");
	  low_start_new_program(0, 2, 0,
				$1,
				&$<number>$);
	}
      }
      Pike_compiler->compiler_pass=tmp;
    }
  }
  optional_create_arguments failsafe_program
  {
    struct program *p;
    if(Pike_compiler->compiler_pass == 1)
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

    $$=mkidentifiernode($<number>5);

    free_node ($3);
    free_node($4);
    check_tree($$,0);
  }
  ;

simple_identifier: TOK_IDENTIFIER
  | bad_identifier { $$ = 0; }
  ;

enum_value: /* EMPTY */
  {
    safe_inc_enum();
  }
  | '=' safe_expr0
  {
    pop_stack();

    /* This can be made more lenient in the future */

    /* Ugly hack to make sure that $2 is optimized */
    {
      int tmp=Pike_compiler->compiler_pass;
      $2=mknode(F_COMMA_EXPR,$2,0);
      Pike_compiler->compiler_pass=tmp;
    }

    if(!is_const($2))
    {
      if(Pike_compiler->compiler_pass==2)
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
	  pop_n_elems(DO_NOT_WARN((INT32)(tmp - 1)));
	}
      } else {
	push_int(0);
      }
    }
    if($2) free_node($2);
  }
  ;

enum_def: /* EMPTY */
  | simple_identifier enum_value
  {
    if ($1) {
      add_constant($1->u.sval.u.string, Pike_sp-1,
		   (Pike_compiler->current_modifiers & ~ID_EXTERN) | ID_INLINE);
    }
    free_node($1);
    /* Update the type. */
    {
      struct pike_type *current = pop_unfinished_type();
      struct pike_type *new = get_type_of_svalue(Pike_sp-1);
      struct pike_type *res = or_pike_types(new, current, 1);
      free_type(current);
      free_type(new);
      type_stack_mark();
      push_finished_type(res);
      free_type(res);
    }
  }
  ;

enum_list: enum_def
  | enum_list ',' enum_def
  ;

enum: modifiers TOK_ENUM
  {
    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == 1)) {
      yywarning("Extern declared enum.");
    }

    push_int(-1);	/* Last enum-value. */
    type_stack_mark();
    push_type(T_ZERO);	/* Joined type so far. */
  }
  optional_identifier '{' enum_list end_block
  {
    struct pike_type *t = pop_unfinished_type();
    pop_stack();
    if ($4) {
      ref_push_type_value(t);
      add_constant($4->u.sval.u.string, Pike_sp-1,
		   (Pike_compiler->current_modifiers & ~ID_EXTERN) | ID_INLINE);
      pop_stack();
      free_node($4);
    }
    $$ = mktypenode(t);
    free_type(t);
  }
  ;

typedef: modifiers TOK_TYPEDEF full_type simple_identifier ';'
  {
    struct pike_type *t = compiler_pop_type();

    if ((Pike_compiler->current_modifiers & ID_EXTERN) &&
	(Pike_compiler->compiler_pass == 1)) {
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

cond: TOK_IF
  {
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  line_number_info
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' safe_comma_expr end_cond statement optional_else_part
  {
    $$ = mknode('?', $6,
		mknode(':',
		       mkcastnode(void_type_string, $8),
		       mkcastnode(void_type_string, $9)));
    $$ = mkcastnode(void_type_string, $$);
    COPY_LINE_NUMBER_INFO ($$, $3);
    free_node ($3);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
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
  | error { $$=0; }
  ;

safe_expr0: expr0
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

foreach: TOK_FOREACH
  {
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  line_number_info
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' expr0 foreach_lvalues end_cond statement
  {
    if ($7) {
      $$=mknode(F_FOREACH,
		mknode(F_VAL_LVAL,$6,$7),
		$9);
    } else {
      /* Error in lvalue */
      free_node($6);
      $$=$9;
    }
    COPY_LINE_NUMBER_INFO ($$, $3);
    free_node ($3);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;

do: TOK_DO line_number_info statement
  TOK_WHILE '(' safe_comma_expr end_cond expected_semicolon
  {
    $$=mknode(F_DO,$3,$6);
    COPY_LINE_NUMBER_INFO ($$, $2);
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

for: TOK_FOR
  {
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  line_number_info
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' unused expected_semicolon for_expr expected_semicolon unused end_cond
  statement
  {
    $$=mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $6),
	      mknode(F_FOR,$8,mknode(':',$12,$10)));
    COPY_LINE_NUMBER_INFO ($$, $3);
    free_node ($3);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;


while:  TOK_WHILE
  {
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  line_number_info
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' safe_comma_expr end_cond statement
  {
    $$=mknode(F_FOR,$6,mknode(':',$8,NULL));
    COPY_LINE_NUMBER_INFO ($$, $3);
    free_node ($3);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;

for_expr: /* EMPTY */ { $$=mkintnode(1); }
  | safe_comma_expr
  ;

switch:	TOK_SWITCH
  {
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  line_number_info
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' safe_comma_expr end_cond statement
  {
    $$=mknode(F_SWITCH,$6,$8);
    COPY_LINE_NUMBER_INFO ($$, $3);
    free_node ($3);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>4;
  }
  ;

case: TOK_CASE safe_comma_expr expected_colon
  {
    $$=mknode(F_CASE,$2,0);
  }
  | TOK_CASE safe_comma_expr expected_dot_dot optional_comma_expr expected_colon
  {
     $$=mknode(F_CASE_RANGE,$2,$4);
  }
  | TOK_CASE expected_dot_dot safe_comma_expr expected_colon
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

return: TOK_RETURN expected_semicolon
  {
    if(!TEST_COMPAT(0,6) &&
       !match_types(Pike_compiler->compiler_frame->current_return_type,
		    void_type_string))
    {
      yytype_error("Must return a value for a non-void function.",
		   Pike_compiler->compiler_frame->current_return_type,
		   void_type_string, 0);
    }
    $$=mknode(F_RETURN,mkintnode(0),0);
  }
  | TOK_RETURN safe_comma_expr expected_semicolon
  {
    $$=mknode(F_RETURN,$2,0);
  }
  ;
	
unused: { $$=0; }
  | safe_comma_expr { $$=mkcastnode(void_type_string, $1);  }
  ;

unused2: comma_expr { $$=mkcastnode(void_type_string, $1);  } ;

optional_comma_expr: { $$=0; }
  | safe_comma_expr
  ;

safe_comma_expr: comma_expr
  | error { $$=0; }
  ;

comma_expr: comma_expr2
  | simple_type2 local_name_list { $$=$2; free_node($1); }
  | simple_identifier_type local_name_list2 { $$=$2; free_node($1); }
  | simple_identifier_type local_function { $$=$2; free_node($1); }
  | simple_type2 local_function2 { $$=$2; free_node($1); }
  ;
          

comma_expr2: expr0
  | comma_expr2 ',' expr0
  {
    $$ = mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $1), $3); 
  }
  ;

expr00: expr0
      | '@' expr0 { $$=mknode(F_PUSH_ARRAY,$2,0); };

expr0: expr01
  | expr4 '=' expr0  { $$=mknode(F_ASSIGN,$3,$1); }
  | expr4 '=' error { $$=$1; reset_type_stack(); yyerrok; }
  | bad_expr_ident '=' expr0 { $$=$3; }
  | open_bracket_with_line_info low_lvalue_list ']' '=' expr0
  {
    $$=mknode(F_ASSIGN,$5,mknode(F_ARRAY_LVALUE,$2,0));
    COPY_LINE_NUMBER_INFO ($$, $1);
    free_node ($1);
  }
  | expr4 assign expr0  { $$=mknode($2,$1,$3); }
  | expr4 assign error { $$=$1; reset_type_stack(); yyerrok; }
  | bad_expr_ident assign expr0 { $$=$3; }
  | open_bracket_with_line_info low_lvalue_list ']' assign expr0
  {
    $$=mknode($4,mknode(F_ARRAY_LVALUE,$2,0),$5);
    COPY_LINE_NUMBER_INFO ($$, $1);
    free_node ($1);
  }
  | open_bracket_with_line_info low_lvalue_list ']' error
    { $$=$2; free_node ($1); reset_type_stack(); yyerrok; }
/*  | error { $$=0; reset_type_stack(); } */
  ;

expr01: expr1
  | expr1 '?' expr01 ':' expr01 { $$=mknode('?',$1,mknode(':',$3,$5)); }
  ;

assign: TOK_AND_EQ { $$=F_AND_EQ; }
  | TOK_OR_EQ  { $$=F_OR_EQ; }
  | TOK_XOR_EQ { $$=F_XOR_EQ; }
  | TOK_LSH_EQ { $$=F_LSH_EQ; }
  | TOK_RSH_EQ { $$=F_RSH_EQ; }
  | TOK_ADD_EQ { $$=F_ADD_EQ; }
  | TOK_SUB_EQ { $$=F_SUB_EQ; }
  | TOK_MULT_EQ{ $$=F_MULT_EQ; }
  | TOK_MOD_EQ { $$=F_MOD_EQ; }
  | TOK_DIV_EQ { $$=F_DIV_EQ; }
  ;

optional_comma: { $$=0; } | ',' { $$=1; };

expr_list: { $$=0; }
  | expr_list2 optional_comma
  ;
         

expr_list2: expr00
  | expr_list2 ',' expr00 { $$=mknode(F_ARG_LIST,$1,$3); }
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

assoc_pair:  expr0 expected_colon expr0 
  {
    $$=mknode(F_ARG_LIST,$1,$3);
  }
  | expr0 expected_colon error { free_node($1); $$=0; }
  ;

expr1: expr2
  | expr1 TOK_LOR expr1  { $$=mknode(F_LOR,$1,$3); }
  | expr1 TOK_LAND expr1 { $$=mknode(F_LAND,$1,$3); }
  | expr1 '|' expr1    { $$=mkopernode("`|",$1,$3); }
  | expr1 '^' expr1    { $$=mkopernode("`^",$1,$3); }
  | expr1 '&' expr1    { $$=mkopernode("`&",$1,$3); }
  | expr1 TOK_EQ expr1   { $$=mkopernode("`==",$1,$3); }
  | expr1 TOK_NE expr1   { $$=mkopernode("`!=",$1,$3); }
  | expr1 '>' expr1    { $$=mkopernode("`>",$1,$3); }
  | expr1 TOK_GE expr1   { $$=mkopernode("`>=",$1,$3); }
  | expr1 '<' expr1    { $$=mkopernode("`<",$1,$3); }
  | expr1 TOK_LE expr1   { $$=mkopernode("`<=",$1,$3); }
  | expr1 TOK_LSH expr1  { $$=mkopernode("`<<",$1,$3); }
  | expr1 TOK_RSH expr1  { $$=mkopernode("`>>",$1,$3); }
  | expr1 '+' expr1    { $$=mkopernode("`+",$1,$3); }
  | expr1 '-' expr1    { $$=mkopernode("`-",$1,$3); }
  | expr1 '*' expr1    { $$=mkopernode("`*",$1,$3); }
  | expr1 '%' expr1    { $$=mkopernode("`%",$1,$3); }
  | expr1 '/' expr1    { $$=mkopernode("`/",$1,$3); }
  | expr1 TOK_LOR error 
  | expr1 TOK_LAND error
  | expr1 '|' error   
  | expr1 '^' error   
  | expr1 '&' error   
  | expr1 TOK_EQ error  
  | expr1 TOK_NE error  
  | expr1 '>' error   
  | expr1 TOK_GE error  
  | expr1 '<' error   
  | expr1 TOK_LE error  
  | expr1 TOK_LSH error 
  | expr1 TOK_RSH error 
  | expr1 '+' error   
  | expr1 '-' error   
  | expr1 '*' error   
  | expr1 '%' error   
  | expr1 '/' error
  ;

expr2: expr3
  | cast expr2
  {
    $$ = mkcastnode($1->u.sval.u.type, $2);
    free_node($1);
  }
  | soft_cast expr2
  {
    $$ = mksoftcastnode($1->u.sval.u.type, $2);
    free_node($1);
  }
  | TOK_INC expr4       { $$=mknode(F_INC,$2,0); }
  | TOK_DEC expr4       { $$=mknode(F_DEC,$2,0); }
  | TOK_NOT expr2        { $$=mkopernode("`!",$2,0); }
  | '~' expr2          { $$=mkopernode("`~",$2,0); }
  | '-' expr2          { $$=mkopernode("`-",$2,0); }
  ;

expr3: expr4
  | expr4 TOK_INC       { $$=mknode(F_POST_INC,$1,0); }
  | expr4 TOK_DEC       { $$=mknode(F_POST_DEC,$1,0); }
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

optional_block: ';' /* EMPTY */ { $$=0; }
  | '{' line_number_info push_compiler_frame0
  {
    debug_malloc_touch(Pike_compiler->compiler_frame->current_return_type);
    if(Pike_compiler->compiler_frame->current_return_type)
      free_type(Pike_compiler->compiler_frame->current_return_type);
    copy_pike_type(Pike_compiler->compiler_frame->current_return_type,
		   any_type_string);

    /* block code */
    $<number>1=Pike_compiler->num_used_modules;
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  statements end_block expected_semicolon
  {
    struct pike_type *type;
    char buf[40];
    int f/*, e */;
    struct pike_string *name;
    struct pike_string *save_file = lex.current_file;
    int save_line = lex.current_line;
    lex.current_file = $2->current_file;
    lex.current_line = $2->line_number;

    /* block code */
    unuse_modules(Pike_compiler->num_used_modules - $<number>1);
    pop_local_variables($<number>4);

    debug_malloc_touch($5);
    $5=mknode(F_COMMA_EXPR,$5,mknode(F_RETURN,mkintnode(0),0));
    if (Pike_compiler->compiler_pass == 2)
      /* Doing this in pass 1 might induce too strict checks on types
       * in cases where we got placeholders. */
      type=find_return_type($5);
    else
      type = NULL;

    if(type) {
      push_finished_type(type);
      free_type(type);
    } else
      push_type(T_MIXED);
    
    push_type(T_VOID);
    push_type(T_MANY);
/*
    e=$5-1;
    for(; e>=0; e--)
      push_finished_type(Pike_compiler->compiler_frame->variable[e].type);
*/
    
    type=compiler_pop_type();

    sprintf(buf,"__lambda_%ld_%ld_line_%d",
	    (long)Pike_compiler->new_program->id,
	    (long)(Pike_compiler->local_class_counter++ & 0xffffffff), /* OSF/1 cc bug. */
	    (int) lex.current_line);
    name=make_shared_string(buf);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: IMPLICIT LAMBDA: %s 0x%08lx 0x%08lx\n",
	    Pike_compiler->compiler_pass, buf, (long)Pike_compiler->new_program->id, Pike_compiler->local_class_counter-1);
#endif /* LAMBDA_DEBUG */
    
    f=dooptcode(name,
		$5,
		type,
		ID_STATIC | ID_PRIVATE | ID_INLINE);

    if(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPED) {
      $$ = mktrampolinenode(f,Pike_compiler->compiler_frame->previous);
    } else {
      $$ = mkidentifiernode(f);
    }

    lex.current_line = save_line;
    lex.current_file = save_file;
    free_node ($2);
    free_string(name);
    free_type(type);
    pop_compiler_frame();
  }
  ;

expr4: string
  | TOK_NUMBER 
  | TOK_FLOAT { $$=mkfloatnode((FLOAT_TYPE)$1); }
  | catch
  | gauge
  | typeof
  | sscanf
  | lambda
  | class
  | enum
  | idents2
  | expr4 open_paren_with_line_info expr_list  ')'
    {
      $$=mkapplynode($1,$3);
      COPY_LINE_NUMBER_INFO ($$, $2);
      free_node ($2);
    }
  | expr4 open_paren_with_line_info error ')'
  {
    $$=mkapplynode($1, NULL);
    free_node ($2);
    yyerrok;
  }
  | expr4 open_paren_with_line_info error TOK_LEX_EOF
  {
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
    $$=mkapplynode($1, NULL);
    free_node ($2);
  }
  | expr4 open_paren_with_line_info error ';'
  {
    yyerror("Missing ')'.");
    $$=mkapplynode($1, NULL);
    free_node ($2);
  }
  | expr4 open_paren_with_line_info error '}'
  {
    yyerror("Missing ')'.");
    $$=mkapplynode($1, NULL);
    free_node ($2);
  }
  | expr4 open_bracket_with_line_info '*' ']'
  {
    $$=mknode(F_AUTO_MAP_MARKER, $1, 0);
    COPY_LINE_NUMBER_INFO ($$, $2);
    free_node ($2);
  }
  | expr4 open_bracket_with_line_info expr0 ']'
  {
    $$=mknode(F_INDEX,$1,$3);
    COPY_LINE_NUMBER_INFO ($$, $2);
    free_node ($2);
  }
  | expr4 open_bracket_with_line_info
    comma_expr_or_zero expected_dot_dot comma_expr_or_maxint ']'
  {
    $$=mknode(F_RANGE,$1,mknode(F_ARG_LIST,$3,$5));
    COPY_LINE_NUMBER_INFO ($$, $2);
    free_node ($2);
  }
  | expr4 open_bracket_with_line_info error ']'
  {
    $$=$1;
    free_node ($2);
    yyerrok;
  }
  | expr4 open_bracket_with_line_info error TOK_LEX_EOF
  {
    $$=$1; yyerror("Missing ']'.");
    yyerror("Unexpected end of file.");
    free_node ($2);
  }
  | expr4 open_bracket_with_line_info error ';'
    {$$=$1; yyerror("Missing ']'."); free_node ($2);}
  | expr4 open_bracket_with_line_info error '}'
    {$$=$1; yyerror("Missing ']'."); free_node ($2);}
  | expr4 open_bracket_with_line_info error ')'
    {$$=$1; yyerror("Missing ']'."); free_node ($2);}
  | open_paren_with_line_info comma_expr2 ')'
  {
    $$=$2;
    if ($$)
      COPY_LINE_NUMBER_INFO ($$, $1);
    free_node ($1);
  }
  | open_paren_with_line_info '{' expr_list close_brace_or_missing ')'
    {
      $$=mkefuncallnode("aggregate",$3);
      COPY_LINE_NUMBER_INFO ($$, $1);
      free_node ($1);
    }
  | open_paren_with_line_info
    open_bracket_with_line_info	/* Only to avoid shift/reduce conflicts. */
    m_expr_list close_bracket_or_missing ')'
    {
      $$=mkefuncallnode("aggregate_mapping",$3);
      COPY_LINE_NUMBER_INFO ($$, $1);
      free_node ($1);
      free_node ($2);
    }
  | TOK_MULTISET_START line_number_info expr_list TOK_MULTISET_END
    {
      $$=mkefuncallnode("aggregate_multiset",$3);
      COPY_LINE_NUMBER_INFO ($$, $2);
      free_node ($2);
    }
  | TOK_MULTISET_START line_number_info expr_list ')'
    {
      yyerror("Missing '>'.");
      $$=mkefuncallnode("aggregate_multiset",$3);
      COPY_LINE_NUMBER_INFO ($$, $2);
      free_node ($2);
    }
  | open_paren_with_line_info error ')' { $$=$1; yyerrok; }
  | open_paren_with_line_info error TOK_LEX_EOF
  {
    $$=$1; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | open_paren_with_line_info error ';' { $$=$1; yyerror("Missing ')'."); }
  | open_paren_with_line_info error '}' { $$=$1; yyerror("Missing ')'."); }
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
  | expr4 TOK_ARROW line_number_info magic_identifier
  {
    $$=mknode(F_ARROW,$1,$4);
    COPY_LINE_NUMBER_INFO ($$, $3);
    free_node ($3);
  }
  | expr4 TOK_ARROW line_number_info error {$$=$1; free_node ($3);}
  ;

idents2: idents
  | TOK_LOCAL_ID TOK_COLON_COLON TOK_IDENTIFIER
  {
    int i;

    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);

    if (((i = find_shared_string_identifier(Pike_compiler->last_identifier,
					    Pike_compiler->new_program)) >= 0) ||
	((i = really_low_find_shared_string_identifier(Pike_compiler->last_identifier,
						       Pike_compiler->new_program,
						       SEE_STATIC|
						       SEE_PRIVATE)) >= 0)) {
      struct reference *ref = Pike_compiler->new_program->identifier_references + i;
      if (!TEST_COMPAT (7, 2) &&
	  IDENTIFIER_IS_VARIABLE (
	    ID_FROM_PTR (Pike_compiler->new_program, ref)->identifier_flags)) {
	/* Allowing local:: on variables would lead to pathological
	 * behavior: If a non-local variable in a class is referenced
	 * both with and without local::, both references would
	 * address the same variable in all cases except where an
	 * inheriting program overrides it (c.f. [bug 1252]).
	 *
	 * Furthermore, that's not how it works currently; if this
	 * error is removed then local:: will do nothing on variables
	 * except forcing a lookup in the closest surrounding class
	 * scope. */
	yyerror ("Cannot make local references to variables.");
	$$ = 0;
      }
      else {
	if (!(ref->id_flags & ID_HIDDEN)) {
	  /* We need to generate a new reference. */
	  int d;
	  struct reference funp = *ref;
	  funp.id_flags = (funp.id_flags & ~ID_INHERITED) | ID_INLINE|ID_HIDDEN;
	  i = -1;
	  for(d = 0; d < (int)Pike_compiler->new_program->num_identifier_references; d++) {
	    struct reference *refp;
	    refp = Pike_compiler->new_program->identifier_references + d;

	    if(!MEMCMP((char *)refp,(char *)&funp,sizeof funp)) {
	      i = d;
	      break;
	    }
	  }
	  if (i < 0) {
	    add_to_identifier_references(funp);
	    i = Pike_compiler->new_program->num_identifier_references - 1;
	  }
	}
	$$ = mkidentifiernode(i);
      }
    } else {
      if (Pike_compiler->compiler_pass == 2) {
	my_yyerror("'%s' not defined in local scope.",
		   Pike_compiler->last_identifier->str);
	$$ = 0;
      } else {
	$$ = mknode(F_UNDEFINED, 0, 0);
      }
    }

    free_node($3);
  }
  | TOK_LOCAL_ID TOK_COLON_COLON bad_identifier
  {
    $$=0;
  }
  ;

idents: low_idents
  | idents '.' TOK_IDENTIFIER
  {
    $$=index_node($1, Pike_compiler->last_identifier?Pike_compiler->last_identifier->str:NULL,
		  $3->u.sval.u.string);
    free_node($1);
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);
    free_node($3);
  }
  | '.' TOK_IDENTIFIER
  {
    struct pike_string *dot;
    MAKE_CONST_STRING(dot, ".");
    if (call_handle_import(dot)) {
      node *tmp=mkconstantsvaluenode(Pike_sp-1);
      pop_stack();
      $$=index_node(tmp, ".", $2->u.sval.u.string);
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
  | idents '.' bad_identifier {}
  | idents '.' error {}
  ;

inherit_specifier: TOK_IDENTIFIER TOK_COLON_COLON
  {
    int e = -1;

    inherit_state = Pike_compiler;

    for (inherit_depth = 0;; inherit_depth++, inherit_state = inherit_state->previous) {
      int inh = find_inherit(inherit_state->new_program, $1->u.sval.u.string);
      if (inh) {
	e = inh;
	break;
      }
      if (inherit_depth == compilation_depth) break;
      if (!TEST_COMPAT (7, 2) &&
	  ID_FROM_INT (inherit_state->previous->new_program,
		       inherit_state->previous->parent_identifier)->name ==
	  $1->u.sval.u.string) {
	e = 0;
	break;
      }
    }
    if (e == -1) {
      if (TEST_COMPAT (7, 2))
	my_yyerror("No such inherit %s.", $1->u.sval.u.string->str);
      else {
	if ($1->u.sval.u.string == this_program_string) {
	  inherit_state = Pike_compiler;
	  inherit_depth = 0;
	  e = 0;
	}
	else
	  my_yyerror("No inherit or surrounding class %s.", $1->u.sval.u.string->str);
      }
    }
    free_node($1);
    $$ = e;
  }
  | TOK_GLOBAL TOK_COLON_COLON
  {
    inherit_state = Pike_compiler;
    for (inherit_depth = 0; inherit_depth < compilation_depth;
	 inherit_depth++, inherit_state = inherit_state->previous) {}
    $$ = 0;
  }
  | inherit_specifier TOK_IDENTIFIER TOK_COLON_COLON
  {
    if ($1 >= 0) {
      int e = 0;
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
	  my_yyerror("No such inherit %s::%s.",
		     inherit_state->new_program->inherits[$1].name->str,
		     $2->u.sval.u.string->str);
	} else {
	  my_yyerror("No such inherit %s.", $2->u.sval.u.string->str);
	}
	$$ = -1;
      } else {
	/* We know stuff about the inherit structure... */
	$$ = e + $1;
      }
    }
    free_node($2);
  }
  | inherit_specifier bad_identifier TOK_COLON_COLON { $$ = -1; }
  ;

low_idents: TOK_IDENTIFIER
  {
    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $1->u.sval.u.string);

    if(($$=lexical_islocal(Pike_compiler->last_identifier)))
    {
      /* done, nothing to do here */
    }else if(!($$=find_module_identifier(Pike_compiler->last_identifier,1)) &&
	     !($$ = program_magic_identifier (Pike_compiler, 0, 0,
					      Pike_compiler->last_identifier, 0))) {
      if((Pike_compiler->flags & COMPILATION_FORCE_RESOLVE) ||
	 (Pike_compiler->compiler_pass==2)) {
	my_yyerror("Undefined identifier \"%s\".",
		   Pike_compiler->last_identifier->str);
	/* FIXME: Add this identifier as a constant in the current program to
	 *        avoid multiple reporting of the same identifier.
	 * NOTE: This should then only be done in the second pass.
	 */	
	$$=0;
      }else{
	$$=mknode(F_UNDEFINED,0,0);
      }
    }
    free_node($1);
  }
  | TOK_PREDEF TOK_COLON_COLON TOK_IDENTIFIER
  {
    node *tmp2;
    extern dynamic_buffer used_modules;

    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $3->u.sval.u.string);

    tmp2=mkconstantsvaluenode((struct svalue *) used_modules.s.str );
    $$=index_node(tmp2, "predef", $3->u.sval.u.string);
    if(!$$->name)
      add_ref( $$->name=$3->u.sval.u.string );
    free_node(tmp2);
    free_node($3);
  }
  | TOK_PREDEF TOK_COLON_COLON bad_identifier
  {
    $$=0;
  }
  | inherit_specifier TOK_IDENTIFIER
  {
    if ($1 >= 0) {
      int id;

      if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
      copy_shared_string(Pike_compiler->last_identifier, $2->u.sval.u.string);

      if ($1 > 0)
	id = low_reference_inherited_identifier(inherit_state,
						$1,
						Pike_compiler->last_identifier,
						SEE_STATIC);
      else
	id = really_low_find_shared_string_identifier(Pike_compiler->last_identifier,
						      inherit_state->new_program,
						      SEE_STATIC|SEE_PRIVATE);

      if (id != -1) {
	if (inherit_depth > 0) {
	  $$ = mkexternalnode(inherit_state->new_program, id);
	} else {
	  $$ = mkidentifiernode(id);
	}
      } else if (($$ = program_magic_identifier (inherit_state, inherit_depth, $1,
						 Pike_compiler->last_identifier, 1))) {
	/* All done. */
      }
      else {
	if ((Pike_compiler->flags & COMPILATION_FORCE_RESOLVE) ||
	    (Pike_compiler->compiler_pass == 2)) {
	  if (inherit_state->new_program->inherits[$1].name) {
	    my_yyerror("Undefined identifier \"%s::%s\".",
		       inherit_state->new_program->inherits[$1].name->str,
		       Pike_compiler->last_identifier->str);
	  } else {
	    my_yyerror("Undefined identifier \"%s\".",
		       Pike_compiler->last_identifier->str);
	  }
	  $$=0;
	}
	else
	  $$=mknode(F_UNDEFINED,0,0);
      }
    } else {
      $$=0;
    }

    free_node($2);
  }
  | inherit_specifier bad_identifier { $$=0; }
  | inherit_specifier error { $$=0; }
  | TOK_COLON_COLON TOK_IDENTIFIER
  {
    int e,i;

    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $2->u.sval.u.string);

    $$=0;
    for(e=1;e<(int)Pike_compiler->new_program->num_inherits;e++)
    {
      if(Pike_compiler->new_program->inherits[e].inherit_level!=1) continue;
      i=low_reference_inherited_identifier(0,e,$2->u.sval.u.string,SEE_STATIC);
      if(i==-1) continue;
      if($$)
      {
	$$=mknode(F_ARG_LIST,$$,mkidentifiernode(i));
      }else{
	$$=mkidentifiernode(i);
      }
    }
    if(!$$)
    {
      if (!($$ = program_magic_identifier (Pike_compiler, 0, -1,
					   $2->u.sval.u.string, 1)))
      {
	if (Pike_compiler->compiler_pass == 2) {
	  if (TEST_COMPAT(7,2)) {
	    yywarning("Undefined identifier \"::%s.\"",
		      $2->u.sval.u.string->str);
	  } else {
	    my_yyerror("Undefined identifier \"::%s\".",
		       $2->u.sval.u.string->str);
	  }
	}
	$$=mkintnode(0);
      }
    }else{
      if($$->token==F_ARG_LIST) $$=mkefuncallnode("aggregate",$$);
    }
    free_node($2);
  }
  | TOK_COLON_COLON bad_identifier
  {
    $$=0;
  }
  ;

comma_expr_or_zero: /* empty */ { $$=mkintnode(0); }
  | comma_expr
  | TOK_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
  ;

comma_expr_or_maxint: /* empty */ { $$=mkintnode(MAX_INT_TYPE); }
  | comma_expr
  | TOK_LEX_EOF { yyerror("Unexpected end of file."); $$=mkintnode(MAX_INT_TYPE); }
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

typeof: TOK_TYPEOF '(' expr0 ')'
  {
    struct pike_type *t;
    node *tmp;

    /* FIXME: Why build the node at all? */
    /* Because the optimizer cannot optimize the root node of the
     * tree properly -Hubbe
     */
    tmp=mknode(F_COMMA_EXPR, $3, 0);
    optimize_node(tmp);

    t=(tmp && CAR(tmp) && CAR(tmp)->type ? CAR(tmp)->type : mixed_type_string);
    if(TEST_COMPAT(7,0))
    {
      struct pike_string *s=describe_type(t);
      $$ = mkstrnode(s);
      free_string(s);
    }else{
      $$ = mktypenode(t);
    }
    free_node(tmp);
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
 
catch_arg: '(' comma_expr ')'  { $$=$2; }
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

sscanf: TOK_SSCANF '(' expr0 ',' expr0 lvalue_list ')'
  {
    $$=mknode(F_SSCANF,mknode(F_ARG_LIST,$3,$5),$6);
  }
  | TOK_SSCANF '(' expr0 ',' expr0 error ')'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerrok;
  }
  | TOK_SSCANF '(' expr0 ',' expr0 error TOK_LEX_EOF
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_SSCANF '(' expr0 ',' expr0 error '}'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' expr0 ',' expr0 error ';'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' expr0 error ')'
  {
    $$=0;
    free_node($3);
    yyerrok;
  }
  | TOK_SSCANF '(' expr0 error TOK_LEX_EOF
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_SSCANF '(' expr0 error '}'
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
  }
  | TOK_SSCANF '(' expr0 error ';'
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

lvalue: expr4
  | open_bracket_with_line_info low_lvalue_list ']'
    { $$=mknode(F_ARRAY_LVALUE, $2,0); COPY_LINE_NUMBER_INFO ($$, $1); free_node ($1); }
  | type6 TOK_IDENTIFIER
  {
    int id = add_local_name($2->u.sval.u.string,compiler_pop_type(),0);
    if (id >= 0)
      $$=mklocalnode(id,-1);
    else
      $$ = 0;
    free_node($2);
  }
  | bad_expr_ident
  { $$=mknewintnode(0); }
  ;
low_lvalue_list: lvalue lvalue_list { $$=mknode(F_LVALUE_LIST,$1,$2); }
  ;

lvalue_list: /* empty */ { $$ = 0; }
  | ',' lvalue lvalue_list { $$ = mknode(F_LVALUE_LIST,$2,$3); }
  ;

string: TOK_STRING 
  | string TOK_STRING
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

/*
 * Some error-handling
 */

/* FIXME: Should probably set Pike_compiler->last_identifier. */
bad_identifier: bad_expr_ident
  | TOK_ARRAY_ID
  { yyerror("array is a reserved word."); }
  | TOK_CLASS
  { yyerror("class is a reserved word."); }
  | TOK_ENUM
  { yyerror("enum is a reserved word."); }
  | TOK_FLOAT_ID
  { yyerror("float is a reserved word.");}
  | TOK_FUNCTION_ID
  { yyerror("function is a reserved word.");}
  | TOK_INT_ID
  { yyerror("int is a reserved word."); }
  | TOK_MAPPING_ID
  { yyerror("mapping is a reserved word."); }
  | TOK_MIXED_ID
  { yyerror("mixed is a reserved word."); }
  | TOK_MULTISET_ID
  { yyerror("multiset is a reserved word."); }
  | TOK_OBJECT_ID
  { yyerror("object is a reserved word."); }
  | TOK_PROGRAM_ID
  { yyerror("program is a reserved word."); }
  | TOK_STRING_ID
  { yyerror("string is a reserved word."); }
  | TOK_TYPEDEF
  { yyerror("typedef is a reserved word."); }
  | TOK_VOID_ID
  { yyerror("void is a reserved word."); }
  ;

bad_expr_ident:
    TOK_INLINE
  { yyerror("inline is a reserved word."); }
  | TOK_LOCAL_ID
  { yyerror("local is a reserved word."); }
  | TOK_NO_MASK
  { yyerror("nomask is a reserved word."); }
  | TOK_PREDEF
  { yyerror("predef is a reserved word."); }
  | TOK_PRIVATE
  { yyerror("private is a reserved word."); }
  | TOK_PROTECTED
  { yyerror("protected is a reserved word."); }
  | TOK_PUBLIC
  { yyerror("public is a reserved word."); }
  | TOK_OPTIONAL
  { yyerror("optional is a reserved word."); }
  | TOK_VARIANT
  { yyerror("variant is a reserved word."); }
  | TOK_STATIC
  { yyerror("static is a reserved word."); }
  | TOK_EXTERN
  { yyerror("extern is a reserved word."); }
  | TOK_FINAL_ID
  { yyerror("final is a reserved word.");}
  | TOK_DO
  { yyerror("do is a reserved word."); }
  | TOK_ELSE
  { yyerror("else without if."); }
  | TOK_RETURN
  { yyerror("return is a reserved word."); }
  | TOK_IMPORT
  { yyerror("import is a reserved word."); }
  | TOK_INHERIT
  { yyerror("inherit is a reserved word."); }
  | TOK_CATCH
  { yyerror("catch is a reserved word."); }
  | TOK_GAUGE
  { yyerror("gauge is a reserved word."); }
  | TOK_LAMBDA
  { yyerror("lambda is a reserved word."); }
  | TOK_SSCANF
  { yyerror("sscanf is a reserved word."); }
  | TOK_SWITCH
  { yyerror("switch is a reserved word."); }
  | TOK_TYPEOF
  { yyerror("typeof is a reserved word."); }
  | TOK_BREAK
  { yyerror("break is a reserved word."); }
  | TOK_CASE
  { yyerror("case is a reserved word."); }
  | TOK_CONTINUE
  { yyerror("continue is a reserved word."); }
  | TOK_DEFAULT
  { yyerror("default is a reserved word."); }
  | TOK_FOR
  { yyerror("for is a reserved word."); }
  | TOK_FOREACH
  { yyerror("foreach is a reserved word."); }
  | TOK_IF
  { yyerror("if is a reserved word."); }
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

void yyerror(char *str)
{
  extern int cumulative_parse_error;

  STACK_LEVEL_START(0);

#ifdef PIKE_DEBUG
  if(Pike_interpreter.recoveries && Pike_sp-Pike_interpreter.evaluator_stack < Pike_interpreter.recoveries->stack_pointer)
    Pike_fatal("Stack error (underflow)\n");
#endif

  if (Pike_compiler->num_parse_error > 20) return;
  Pike_compiler->num_parse_error++;
  cumulative_parse_error++;

  if ((error_handler && error_handler->prog) || get_master())
  {
    if (lex.current_file) {
      ref_push_string(lex.current_file);
    } else {
      /* yyerror() can be called from define_function(), which
       * can be called by the C module initialization code.
       */
      push_constant_text("");
    }
    push_int(lex.current_line);
    push_text(str);
    low_safe_apply_handler("compile_error", error_handler, compat_handler, 3);
    pop_stack();
  }else{
    if (lex.current_file) {
      (void)fprintf(stderr, "%s:%ld: %s\n",
		    lex.current_file->str,
		    (long)lex.current_line,
		    str);
    } else {
      (void)fprintf(stderr, "NULL:%ld: %s\n",
		    (long)lex.current_line,
		    str);
    }
    fflush(stderr);
  }

  STACK_LEVEL_DONE(0);
}

static int low_islocal(struct compiler_frame *f,
		       struct pike_string *str)
{
  int e;
  for(e=f->current_number_of_locals-1;e>=0;e--)
    if(f->variable[e].name==str)
      return e;
  return -1;
}



int low_add_local_name(struct compiler_frame *frame,
		       struct pike_string *str,
		       struct pike_type *type,
		       node *def)
{

  if (str->len && !TEST_COMPAT(7,0)) {
    int tmp=low_islocal(frame,str);
    if(tmp>=0 && tmp >= frame->last_block_level)
    {
      if(str->size_shift)
	my_yyerror("Duplicate local variable, "
		   "previous declaration on line %d\n",
		   frame->variable[tmp].line);
      else
	my_yyerror("Duplicate local variable '%s', "
		   "previous declaration on line %d\n",
		   STR0(str), frame->variable[tmp].line);
    }

    if(type == void_type_string)
    {
      if(str->size_shift)
	my_yyerror("Local variables cannot be of type of 'void'.\n");
      else
	my_yyerror("Local variable '%s' is void.\n",STR0(str));
    }
  }

  debug_malloc_touch(def);
  debug_malloc_touch(type);
  debug_malloc_touch(str);
  if (frame->current_number_of_locals == MAX_LOCAL-1)
  {
    yyerror("Too many local variables.");
    free_type(type);
    if (def) free_node(def);
    return -1;
  }else {
    reference_shared_string(str);
#ifdef PIKE_DEBUG
    check_type_string(type);
#endif /* PIKE_DEBUG */
    if (pike_types_le(type, void_type_string)) {
      if (Pike_compiler->compiler_pass != 1) {
	yywarning("Declaring local variable with type void "
		  "(converted to type zero).");
      }
      free_type(type);
      copy_pike_type(type, zero_type_string);
    }
    frame->variable[frame->current_number_of_locals].type = type;
    frame->variable[frame->current_number_of_locals].name = str;
    frame->variable[frame->current_number_of_locals].def = def;

    frame->variable[frame->current_number_of_locals].line=lex.current_line;
    frame->variable[frame->current_number_of_locals].file=lex.current_file;
    add_ref(lex.current_file);

    frame->current_number_of_locals++;
    if(frame->current_number_of_locals > 
       frame->max_number_of_locals)
    {
      frame->max_number_of_locals=
	frame->current_number_of_locals;
    }

    return frame->current_number_of_locals-1;
  }
}


/* argument must be a shared string */
/* Note that this function eats a reference to 'type' */
/* If def is nonzero, it also eats a ref to def */
int add_local_name(struct pike_string *str,
		   struct pike_type *type,
		   node *def)
{
  return low_add_local_name(Pike_compiler->compiler_frame,
			    str,
			    type,
			    def);
}



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
      if(f->variable[e].name==str)
      {
	struct compiler_frame *q=Pike_compiler->compiler_frame;

	while(q!=f) 
	{
	  q->lexical_scope|=SCOPE_SCOPED;
	  q=q->previous;
	}

	if(depth) {
	  q->lexical_scope|=SCOPE_SCOPE_USED;

	  if(q->min_number_of_locals < e+1)
	    q->min_number_of_locals = e+1;
	}

	if(f->variable[e].def) {
	  /*fprintf(stderr, "Found prior definition of \"%s\"\n", str->str); */
	  return copy_node(f->variable[e].def);
	}

	return mklocalnode(e,depth);
      }
    }
    if(!(f->lexical_scope & SCOPE_LOCAL)) return 0;
    depth++;
    f=f->previous;
  }
}

static void safe_inc_enum(void)
{
  JMP_BUF recovery;
  STACK_LEVEL_START(1);

  if (SETJMP_SP(recovery, 1)) {
    handle_compile_exception ("Bad implicit enum value (failed to add 1).");
    push_int(0);
  } else {
    push_int(1);
    f_add(2);
  }
  UNSETJMP(recovery);
  STACK_LEVEL_DONE(1);
}


static int call_handle_import(struct pike_string *s)
{
  int args;

  ref_push_string(s);
  ref_push_string(lex.current_file);
  if (error_handler && error_handler->prog) {
    ref_push_object(error_handler);
    args = 3;
  }
  else args = 2;

  if (safe_apply_handler("handle_import", error_handler, compat_handler,
			 args, BIT_MAPPING|BIT_OBJECT|BIT_PROGRAM|BIT_ZERO))
    if (Pike_sp[-1].type != T_INT)
      return 1;
    else {
      pop_stack();
      if (!s->size_shift)
	my_yyerror("Couldn't find module to import: %s",s->str);
      else
	yyerror("Couldn't find module to import");
    }
  else
    handle_compile_exception ("Error finding module to import");

  return 0;
}

void cleanup_compiler(void)
{
}
