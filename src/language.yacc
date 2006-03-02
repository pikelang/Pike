/* -*- C -*- */
/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
%pure_parser
/*
 * These values are used by the stack machine, and can not be directly
 * called from Pike.
 */
%token F_OFFSET
%token F_PREFIX_256 F_PREFIX_512 F_PREFIX_768 F_PREFIX_1024
%token F_PREFIX_CHARX256 F_PREFIX_WORDX256 F_PREFIX_24BITX256
%token F_POP_VALUE F_POP_N_ELEMS F_MARK F_MARK2 F_MARK_X F_POP_MARK
%token F_LDA F_CALL_LFUN F_CALL_LFUN_AND_POP F_CALL_LFUN_AND_RETURN
%token F_APPLY F_APPLY_AND_POP F_MARK_APPLY F_MARK_APPLY_POP
%token F_APPLY_AND_RETURN F_MARK_AND_STRING
%token F_APPLY_ASSIGN_LOCAL F_APPLY_ASSIGN_LOCAL_AND_POP

%token F_BRANCH F_BRANCH_WHEN_ZERO F_BRANCH_WHEN_NON_ZERO
%token F_BRANCH_AND_POP_WHEN_ZERO F_BRANCH_AND_POP_WHEN_NON_ZERO
%token F_BRANCH_WHEN_LT F_BRANCH_WHEN_GT
%token F_BRANCH_WHEN_LE F_BRANCH_WHEN_GE
%token F_BRANCH_WHEN_EQ F_BRANCH_WHEN_NE
%token F_BRANCH_IF_LOCAL F_BRANCH_IF_NOT_LOCAL
%token F_BRANCH_IF_NOT_LOCAL_ARROW
%token F_INC_LOOP F_DEC_LOOP
%token F_INC_NEQ_LOOP F_DEC_NEQ_LOOP

%token F_LEXICAL_LOCAL F_LEXICAL_LOCAL_LVALUE

%token F_INDEX F_ARROW F_INDIRECT F_STRING_INDEX F_LOCAL_INDEX
%token F_LOCAL_LOCAL_INDEX F_LOCAL_ARROW F_GLOBAL_LOCAL_INDEX
%token F_POS_INT_INDEX F_NEG_INT_INDEX F_MAGIC_INDEX F_MAGIC_SET_INDEX
%token F_LTOSVAL F_LTOSVAL2
%token F_PUSH_ARRAY 
%token F_RANGE F_COPY_VALUE

/*
 * Basic value pushing
 */
%token F_LFUN F_TRAMPOLINE
%token F_GLOBAL F_GLOBAL_LVALUE
%token F_LOCAL F_2_LOCALS F_LOCAL_LVALUE F_MARK_AND_LOCAL
%token F_EXTERNAL F_EXTERNAL_LVALUE
%token F_CLEAR_LOCAL F_CLEAR_2_LOCAL F_CLEAR_4_LOCAL
%token F_CLEAR_STRING_SUBTYPE
%token F_CONSTANT F_FLOAT F_STRING F_ARROW_STRING
%token F_NUMBER F_NEG_NUMBER F_CONST_1 F_CONST0 F_CONST1 F_BIGNUM
%token F_UNDEFINED
/*
 * These are the predefined functions that can be accessed from Pike.
 */

%token F_INC F_DEC F_POST_INC F_POST_DEC F_INC_AND_POP F_DEC_AND_POP
%token F_INC_LOCAL F_INC_LOCAL_AND_POP F_POST_INC_LOCAL
%token F_DEC_LOCAL F_DEC_LOCAL_AND_POP F_POST_DEC_LOCAL
%token F_RETURN F_DUMB_RETURN F_RETURN_0 F_RETURN_1 F_RETURN_LOCAL
%token F_RETURN_IF_TRUE F_THROW_ZERO

%token F_ASSIGN F_ASSIGN_AND_POP
%token F_ASSIGN_LOCAL F_ASSIGN_LOCAL_AND_POP
%token F_ASSIGN_GLOBAL F_ASSIGN_GLOBAL_AND_POP
%token F_LOCAL_2_LOCAL F_LOCAL_2_GLOBAL F_GLOBAL_2_LOCAL
%token F_ADD F_SUBTRACT F_ADD_INT F_ADD_NEG_INT
%token F_ADD_TO_AND_POP
%token F_MULTIPLY F_DIVIDE F_MOD

%token F_LT F_GT F_EQ F_GE F_LE F_NE
%token F_NEGATE F_NOT F_COMPL
%token F_AND F_OR F_XOR
%token F_LSH F_RSH
%token F_LAND F_LOR
%token F_EQ_OR F_EQ_AND

%token F_SWITCH F_SSCANF F_CATCH
%token F_CAST
%token F_SOFT_CAST
%token F_FOREACH

%token F_SIZEOF F_SIZEOF_LOCAL
%token F_CALL_FUNCTION F_CALL_FUNCTION_AND_RETURN

/*
 * These are token values that needn't have an associated code for the
 * compiled file
 */
%token F_MAX_OPCODE

/* This is the end of file marker used by the lexer
 * to enable nicer EOF in error handling.
 */
%token F_LEX_EOF

%token F_ADD_EQ
%token F_AND_EQ
%token F_ARG_LIST
%token F_COMMA_EXPR
%token F_ARRAY_ID
%token F_BREAK
%token F_CASE
%token F_CLASS
%token F_COLON_COLON
%token F_COMMA
%token F_CONTINUE 
%token F_DEFAULT
%token F_DIV_EQ
%token F_DO
%token F_DOT_DOT
%token F_DOT_DOT_DOT
%token F_REDEF
%token F_EFUN_CALL
%token F_ELSE
%token F_EXTERN
%token F_FLOAT_ID
%token F_FOR
%token F_FUNCTION_ID
%token F_GAUGE
%token F_IDENTIFIER
%token F_IF
%token F_IMPORT
%token F_INHERIT
%token F_INLINE
%token F_LOCAL_ID
%token F_FINAL_ID
%token F_INT_ID
%token F_LAMBDA
%token F_MULTISET_ID
%token F_MULTISET_END
%token F_MULTISET_START
%token F_LOCAL
%token F_LSH_EQ
%token F_LVALUE_LIST
%token F_ARRAY_LVALUE
%token F_MAPPING_ID
%token F_MIXED_ID
%token F_MOD_EQ
%token F_MULT_EQ
%token F_NO_MASK
%token F_OBJECT_ID
%token F_OR_EQ
%token F_PRIVATE
%token F_PROGRAM_ID
%token F_PROTECTED
%token F_PREDEF
%token F_PUBLIC
%token F_RSH_EQ
%token F_STATIC
%token F_STATUS
%token F_STRING_ID
%token F_SUBSCRIPT
%token F_SUB_EQ
%token F_TYPEOF
%token F_VAL_LVAL
%token F_VOID_ID
%token F_WHILE
%token F_XOR_EQ
%token F_NOP
%token F_UNDEFINED
%token F_OPTIONAL

%token F_ALIGN
%token F_POINTER
%token F_LABEL
%token F_BYTE
%token F_DATA

%token F_MAX_INSTR

%right '='
%right '?'
%left F_LOR
%left F_LAND
%left '|'
%left '^'
%left '&'
%left F_EQ F_NE
%left '>' F_GE '<' F_LE  /* nonassoc? */
%left F_LSH F_RSH
%left '+' '-'
%left '*' '%' '/'
%right F_NOT '~'
%nonassoc F_INC F_DEC

%{
/* This is the grammar definition of Pike. */

#include "global.h"
RCSID("$Id: language.yacc,v 1.187 2006/03/02 10:37:58 grubba Exp $");
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
#include "error.h"
#include "docode.h"
#include "machine.h"
#include "main.h"

#define YYMAXDEPTH	1000

#ifdef PIKE_DEBUG
#ifndef YYDEBUG
/* May also be defined by machine.h */
#define YYDEBUG 1
#endif /* YYDEBUG */
#endif


int add_local_name(struct pike_string *,struct pike_string *,node *);
int low_add_local_name(struct compiler_frame *,
		       struct pike_string *,struct pike_string *,node *);
static node *lexical_islocal(struct pike_string *);

static int varargs;
INT32  current_modifiers;
static struct pike_string *last_identifier=0;


/*
 * Kludge for Bison not using prototypes.
 */
#ifndef __GNUC__
#ifndef __cplusplus
static void __yy_memcpy(char *to, char *from, unsigned int count);
#endif /* !__cplusplus */
#endif /* !__GNUC__ */

%}

%union
{
  int number;
  FLOAT_TYPE fnum;
  struct node_s *n;
  char *str;
}

%{
/* Needs to be included after YYSTYPE is defined. */
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

%type <fnum> F_FLOAT

%type <number> F_ARRAY_ID
%type <number> F_BREAK
%type <number> F_CASE
%type <number> F_CATCH
%type <number> F_CONTINUE
%type <number> F_DEFAULT
%type <number> F_DO
%type <number> F_ELSE
%type <number> F_FLOAT_ID
%type <number> F_FOR
%type <number> F_FOREACH
%type <number> F_FUNCTION_ID
%type <number> F_GAUGE
%type <number> F_IF
%type <number> F_INHERIT
%type <number> F_INLINE
%type <number> F_INT_ID
%type <number> F_LAMBDA
%type <number> F_LOCAL
%type <number> F_MAPPING_ID
%type <number> F_MIXED_ID
%type <number> F_MULTISET_ID
%type <number> F_NO_MASK
%type <number> F_OBJECT_ID
%type <number> F_PREDEF
%type <number> F_PRIVATE
%type <number> F_PROGRAM_ID
%type <number> F_PROTECTED
%type <number> F_PUBLIC
%type <number> F_RETURN
%type <number> F_SSCANF
%type <number> F_STATIC
%type <number> F_STRING_ID
%type <number> F_SWITCH
%type <number> F_VOID_ID
%type <number> F_WHILE
%type <number> arguments
%type <number> arguments2
%type <number> func_args
%type <number> assign
%type <number> modifier
%type <number> modifier_list
%type <number> modifiers
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
%type <n> F_STRING
%type <n> F_NUMBER
%type <n> optional_rename_inherit
%type <n> optional_identifier
%type <n> F_IDENTIFIER
%type <n> assoc_pair
%type <n> block
%type <n> failsafe_block
%type <n> close_paren_or_missing
%type <n> block_or_semi
%type <n> break
%type <n> case
%type <n> catch
%type <n> catch_arg
%type <n> class
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
%type <n> optional_else_part
%type <n> return
%type <n> sscanf
%type <n> statement
%type <n> statements
%type <n> switch
%type <n> typeof
%type <n> unused
%type <n> unused2
%type <n> while
%type <n> optional_comma_expr
%type <n> low_program_ref
%type <n> local_function
%type <n> local_function2
%type <n> magic_identifier
%%

all: program { YYACCEPT; }
  | program F_LEX_EOF { YYACCEPT; }
/*  | error F_LEX_EOF { YYABORT; } */
  ;

program: program def optional_semi_colons
/*  | error { yyerrok; } */
  |  /* empty */
  ;

optional_semi_colons: /* empty */
  | optional_semi_colons ';'
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

optional_rename_inherit: ':' F_IDENTIFIER { $$=$2; }
  | ':' bad_identifier { $$=0; }
  | ':' error { $$=0; }
  | { $$=0; }
  ;

/* NOTE: This rule pushes a string "name" on the stack in addition
 * to resolving the program reference.
 */
low_program_ref: string_constant
  {
    ref_push_string($1->u.sval.u.string);
    ref_push_string($1->u.sval.u.string);
    ref_push_string(lex.current_file);
    
    if (error_handler && error_handler->prog) {
      ref_push_object(error_handler);
      SAFE_APPLY_MASTER("handle_inherit", 3);
    } else {
      SAFE_APPLY_MASTER("handle_inherit", 2);
    }

    if(sp[-1].type != T_PROGRAM)
      my_yyerror("Couldn't cast string \"%s\" to program",
		 $1->u.sval.u.string->str);
    free_node($1);
    $$=mksvaluenode(sp-1);
    if($$->name) free_string($$->name);
    add_ref( $$->name=sp[-2].u.string );
    pop_stack();
  }
  | idents
  {
    if(last_identifier)
    {
      ref_push_string(last_identifier);
    }else{
      push_constant_text("");
    }
    $$=$1;
  }
  ;

/* NOTE: Pushes the resolved program on the stack. */
program_ref: low_program_ref
  {
    resolv_program($1);
    free_node($1);
  }
  ;
      
inheritance: modifiers F_INHERIT low_program_ref optional_rename_inherit ';'
  {
    if (($1 & ID_EXTERN) && (compiler_pass == 1)) {
      yywarning("Extern declared inherit.");
    }
    if(!(new_program->flags & PROGRAM_PASS_1_DONE))
    {
      struct pike_string *s=sp[-1].u.string;
      if($4) s=$4->u.sval.u.string;
      compiler_do_inherit($3,$1,s);
    }
    if($4) free_node($4);
    pop_n_elems(1);
    free_node($3);
  }
  | modifiers F_INHERIT low_program_ref error ';'
  {
    free_node($3); yyerrok;
  }
  | modifiers F_INHERIT low_program_ref error F_LEX_EOF
  {
    free_node($3);
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | modifiers F_INHERIT low_program_ref error '}'
  {
    free_node($3); yyerror("Missing ';'.");
  }
  | modifiers F_INHERIT error ';' { yyerrok; }
  | modifiers F_INHERIT error F_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | modifiers F_INHERIT error '}' { yyerror("Missing ';'."); }
  ;

import: F_IMPORT idents ';'
  {
    resolv_constant($2);
    free_node($2);
    use_module(sp-1);
    pop_stack();
  }
  | F_IMPORT string ';'
  {
    ref_push_string($2->u.sval.u.string);
    free_node($2);
    ref_push_string(lex.current_file);
    if (error_handler && error_handler->prog) {
      ref_push_object(error_handler);
      SAFE_APPLY_MASTER("handle_import", 3);
    } else {
      SAFE_APPLY_MASTER("handle_import", 2);
    }
    use_module(sp-1);
    pop_stack();
  }
  | F_IMPORT error ';' { yyerrok; }
  | F_IMPORT error F_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | F_IMPORT error '}' { yyerror("Missing ';'."); }
  ;

constant_name: F_IDENTIFIER '=' safe_expr0
  {
    int tmp;
    /* This can be made more lenient in the future */

    /* Ugly hack to make sure that $3 is optimized */
    tmp=compiler_pass;
    $3=mknode(F_COMMA_EXPR,$3,0);
    compiler_pass=tmp;

    if ((current_modifiers & ID_EXTERN) && (compiler_pass == 1)) {
      yywarning("Extern declared constant.");
    }

    if(!is_const($3))
    {
      if(compiler_pass==2)
	yyerror("Constant definition is not constant.");
      else
	add_constant($1->u.sval.u.string, 0, current_modifiers & ~ID_EXTERN);
    } else {
      if(!num_parse_error)
      {
	tmp=eval_low($3);
	if(tmp < 1)
	{
	  yyerror("Error in constant definition.");
	}else{
	  pop_n_elems(tmp-1);
	  add_constant($1->u.sval.u.string, sp-1,
		       current_modifiers & ~ID_EXTERN);
	  pop_stack();
	}
      }
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

constant: modifiers F_CONSTANT constant_list ';' {}
  | modifiers F_CONSTANT error ';' { yyerrok; }
  | modifiers F_CONSTANT error F_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  | modifiers F_CONSTANT error '}' { yyerror("Missing ';'."); }
  ;

block_or_semi: block
  {
    $$ = check_node_hash(mknode(F_COMMA_EXPR,$1,mknode(F_RETURN,mkintnode(0),0)));
  }
  | ';' { $$ = NULL; }
  | F_LEX_EOF { yyerror("Expected ';'."); $$ = NULL; }
  | error { $$ = NULL; }
  ;


type_or_error: simple_type
  {
#ifdef PIKE_DEBUG
    check_type_string(check_node_hash($1)->u.sval.u.string);
#endif /* PIKE_DEBUG */
    if(compiler_frame->current_type)
      free_string(compiler_frame->current_type); 
    copy_shared_string(compiler_frame->current_type,$1->u.sval.u.string);
    free_node($1);
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

close_bracket_or_missing: ']'
  | /* empty */
  {
    yyerror("Missing ']'.");
  }
  ;

push_compiler_frame0: /* empty */
  {
    push_compiler_frame(0);

    if(!compiler_frame->previous ||
       !compiler_frame->previous->current_type)
    {
      yyerror("Internal compiler fault.");
      copy_shared_string(compiler_frame->current_type,
			 mixed_type_string);
    }else{
      copy_shared_string(compiler_frame->current_type,
			 compiler_frame->previous->current_type);
    }
  }
  ;

def: modifiers type_or_error optional_stars F_IDENTIFIER push_compiler_frame0
  '(' arguments close_paren_or_missing
  {
    int e;
    /* construct the function type */
    push_finished_type(compiler_frame->current_type);
    if ($3 && (compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while(--$3>=0) push_type(T_ARRAY);
    
    if(compiler_frame->current_return_type)
      free_string(compiler_frame->current_return_type);
    compiler_frame->current_return_type=compiler_pop_type();
    
    push_finished_type(compiler_frame->current_return_type);
    
    e=$7-1;
    if(varargs)
    {
      push_finished_type(compiler_frame->variable[e].type);
      e--;
      varargs=0;
      pop_type_stack();
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--)
    {
      push_finished_type(compiler_frame->variable[e].type);
    }
    push_type(T_FUNCTION);

    {
      struct pike_string *s=compiler_pop_type();
      $<n>$=mkstrnode(s);
      free_string(s);
    }

    if(compiler_pass==1)
    {
      $<number>5=define_function(check_node_hash($4)->u.sval.u.string,
				 check_node_hash($<n>$)->u.sval.u.string,
				 $1 & (~ID_EXTERN),
				 IDENTIFIER_PIKE_FUNCTION,
				 0);
    }
  }
  block_or_semi
  {
    int e;
    if($10)
    {
      int f;
      node *check_args = NULL;
      int save_line = lex.current_line;
#ifdef PIKE_DEBUG
      struct pike_string *save_file = lex.current_file;
      lex.current_file = $8->current_file;
#endif /* PIKE_DEBUG */
      lex.current_line = $8->line_number;

      if (($1 & ID_EXTERN) && (compiler_pass == 1)) {
	yywarning("Extern declared function definition.");
      }

      for(e=0; e<$7; e++)
      {
	if(!compiler_frame->variable[e].name ||
	   !compiler_frame->variable[e].name->len)
	{
	  my_yyerror("Missing name for argument %d.",e);
	} else {
	  /* FIXME: Should probably use some other flag. */
	  if ((runtime_options & RUNTIME_CHECK_TYPES) &&
	      (compiler_pass == 2) &&
	      (compiler_frame->variable[e].type != mixed_type_string)) {
	    node *local_node;

	    /* fprintf(stderr, "Creating soft cast node for local #%d\n", e);*/

	    local_node = mklocalnode(e, 0);

	    /* The following is needed to go around the optimization in
	     * mksoftcastnode().
	     */
	    free_string(local_node->type);
	    copy_shared_string(local_node->type, mixed_type_string);

	    check_args =
	      mknode(F_COMMA_EXPR, check_args,
		     mksoftcastnode(compiler_frame->variable[e].type,
				    local_node));
	  }
	}
      }

      if (check_args) {
	/* Prepend the arg checking code. */
	$10 = mknode(F_COMMA_EXPR, mknode(F_POP_VALUE, check_args, NULL), $10);
      }

      lex.current_line = save_line;
#ifdef PIKE_DEBUG
      lex.current_file = save_file;
#endif /* PIKE_DEBUG */

      f=dooptcode(check_node_hash($4)->u.sval.u.string, check_node_hash($10),
		  check_node_hash($<n>9)->u.sval.u.string, $1);
#ifdef PIKE_DEBUG
      if(recoveries && sp-evaluator_stack < recoveries->sp)
	fatal("Stack error (underflow)\n");

      if(compiler_pass == 1 && f!=$<number>5)
	fatal("define_function screwed up! %d != %d\n",f,$<number>5);
#endif
    }
    pop_compiler_frame();
    free_node($4);
    free_node($8);
    free_node($<n>9);
  }
  | modifiers type_or_error optional_stars F_IDENTIFIER push_compiler_frame0
    error
  {
    pop_compiler_frame();
    free_node($4);
  }
  | modifiers type_or_error optional_stars bad_identifier
  {
    free_string(compiler_pop_type());
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
  | error F_LEX_EOF
  {
    reset_type_stack();
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file");
  }
  | error ';'
  {
    reset_type_stack();
    yyerrok;
/*     if(num_parse_error>5) YYACCEPT; */
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
   '}'
    {
      lex.pragmas=$<number>3;
    }
  ;

optional_dot_dot_dot: F_DOT_DOT_DOT { $$=1; }
  | /* empty */ { $$=0; }
  ;

optional_identifier: F_IDENTIFIER
  | bad_identifier { $$=0; }
  | /* empty */ { $$=0; }
  ;

new_arg_name: type7 optional_dot_dot_dot optional_identifier
  {
    if(varargs) yyerror("Can't define more arguments after ...");

    if($2)
    {
      push_type(T_ARRAY);
      varargs=1;
    }

    if(!$3)
    {
      struct pike_string *s;
      MAKE_CONSTANT_SHARED_STRING(s,"");
      $3=mkstrnode(s);
      free_string(s);
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

modifier: F_NO_MASK    { $$ = ID_NOMASK; }
  | F_FINAL_ID   { $$ = ID_NOMASK; }
  | F_STATIC     { $$ = ID_STATIC; }
  | F_EXTERN     { $$ = ID_EXTERN; }
  | F_OPTIONAL   { $$ = ID_OPTIONAL; }
  | F_PRIVATE    { $$ = ID_PRIVATE | ID_STATIC; }
  | F_LOCAL_ID   { $$ = ID_INLINE; }
  | F_PUBLIC     { $$ = ID_PUBLIC; }
  | F_PROTECTED  { $$ = ID_PROTECTED; }
  | F_INLINE     { $$ = ID_INLINE; }
  ;

magic_identifiers1:
    F_NO_MASK    { $$ = "nomask"; }
  | F_FINAL_ID   { $$ = "final"; }
  | F_STATIC     { $$ = "static"; }
  | F_EXTERN	 { $$ = "extern"; }
  | F_PRIVATE    { $$ = "private"; }
  | F_LOCAL_ID   { $$ = "local"; }
  | F_PUBLIC     { $$ = "public"; }
  | F_PROTECTED  { $$ = "protected"; }
  | F_INLINE     { $$ = "inline"; }
  | F_OPTIONAL   { $$ = "optional"; }
  ;

magic_identifiers2:
    F_VOID_ID       { $$ = "void"; }
  | F_MIXED_ID      { $$ = "mixed"; }
  | F_ARRAY_ID      { $$ = "array"; }
  | F_MAPPING_ID    { $$ = "mapping"; }
  | F_MULTISET_ID   { $$ = "multiset"; }
  | F_OBJECT_ID     { $$ = "object"; }
  | F_FUNCTION_ID   { $$ = "function"; }
  | F_PROGRAM_ID    { $$ = "program"; }
  | F_STRING_ID     { $$ = "string"; }
  | F_FLOAT_ID      { $$ = "float"; }
  | F_INT_ID        { $$ = "int"; }
  ;

magic_identifiers3:
    F_IF         { $$ = "if"; }
  | F_DO         { $$ = "do"; }
  | F_FOR        { $$ = "for"; }
  | F_WHILE      { $$ = "while"; }
  | F_ELSE       { $$ = "else"; }
  | F_FOREACH    { $$ = "foreach"; }
  | F_CATCH      { $$ = "catch"; }
  | F_GAUGE      { $$ = "gauge"; }
  | F_CLASS      { $$ = "class"; }
  | F_BREAK      { $$ = "break"; }
  | F_CASE       { $$ = "case"; }
  | F_CONSTANT   { $$ = "constant"; }
  | F_CONTINUE   { $$ = "continue"; }
  | F_DEFAULT    { $$ = "default"; }
  | F_IMPORT     { $$ = "import"; }
  | F_INHERIT    { $$ = "inherit"; }
  | F_LAMBDA     { $$ = "lambda"; }
  | F_PREDEF     { $$ = "predef"; }
  | F_RETURN     { $$ = "return"; }
  | F_SSCANF     { $$ = "sscanf"; }
  | F_SWITCH     { $$ = "switch"; }
  | F_TYPEOF     { $$ = "typeof"; }
  ;

magic_identifiers: magic_identifiers1 | magic_identifiers2 | magic_identifiers3 ;

magic_identifier: F_IDENTIFIER 
  | magic_identifiers
  {
    struct pike_string *tmp=make_shared_string($1);
    $$=mkstrnode(tmp);
    free_string(tmp);
  }
  ;

modifiers: modifier_list
 {
   $$=current_modifiers=$1 | (lex.pragmas & ID_MODIFIER_MASK);
 }
 ;

modifier_list: /* empty */ { $$ = 0; }
  | modifier modifier_list { $$ = $1 | $2; }
  ;

optional_stars: optional_stars '*' { $$=$1 + 1; }
  | /* empty */ { $$=0; }
  ;

cast: '(' type ')'
    {
      struct pike_string *s=compiler_pop_type();
      $$=mkstrnode(s);
      free_string(s);
    }
    ;

soft_cast: '[' type ']'
    {
      struct pike_string *s=compiler_pop_type();
      $$=mkstrnode(s);
      free_string(s);
    }
    ;

type6: type | identifier_type ;
  
type: type '*'
  {
    if (compiler_pass == 2) {
       yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    push_type(T_ARRAY);
  }
  | type2
  ;

type7: type7 '*'
  {
    if (compiler_pass == 2) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    push_type(T_ARRAY);
  }
  | type4
  ;

simple_type: type4
  {
    struct pike_string *s=compiler_pop_type();
    $$=mkstrnode(s);
#ifdef PIKE_DEBUG
    if ($$->u.sval.u.string != s) {
      fatal("mkstrnode(%p:\"%s\") created node with %p:\"%s\"\n",
	    s, s->str, $$->u.sval.u.string, $$->u.sval.u.string->str);
    }
#endif /* PIKE_DEBUG */
    free_string(s);
  }
  ;

simple_type2: type2
  {
    struct pike_string *s=compiler_pop_type();
    $$=mkstrnode(s);
#ifdef PIKE_DEBUG
    if ($$->u.sval.u.string != s) {
      fatal("mkstrnode(%p:\"%s\") created node with %p:\"%s\"\n",
	    s, s->str, $$->u.sval.u.string, $$->u.sval.u.string->str);
    }
#endif /* PIKE_DEBUG */
    free_string(s);
  }
  ;

simple_identifier_type: identifier_type
  {
    struct pike_string *s=compiler_pop_type();
    $$=mkstrnode(s);
#ifdef PIKE_DEBUG
    if ($$->u.sval.u.string != s) {
      fatal("mkstrnode(%p:\"%s\") created node with %p:\"%s\"\n",
	    s, s->str, $$->u.sval.u.string, $$->u.sval.u.string->str);
    }
#endif /* PIKE_DEBUG */
    free_string(s);
  }
  ;

identifier_type: idents
  { 
    resolv_constant($1);

    if (sp[-1].type == T_TYPE) {
      /* "typedef" */
      push_finished_type(sp[-1].u.string);
    } else {
      /* object type */
      struct program *p = NULL;

      if (sp[-1].type == T_OBJECT) {
	if(!sp[-1].u.object->prog)
	{
	  pop_stack();
	  push_int(0);
	  yyerror("Destructed object used as program identifier.");
	}else{
	  extern void f_object_program(INT32);
	  f_object_program(1);
	}
      }

      switch(sp[-1].type) {
      case T_FUNCTION:
	if((p = program_from_function(sp-1)))
	  break;
      
      default:
	if (compiler_pass!=1)
	  yyerror("Illegal program identifier.");
	pop_stack();
	push_int(0);
	break;
	
      case T_PROGRAM:
	p = sp[-1].u.program;
	break;
      }

      if(p) {
	push_type_int(p->id);
      }else{
	push_type_int(0);
      }
      push_type(0);
      push_type(T_OBJECT);
    }
    pop_stack();
    free_node($1);
  }
  ;

type4: type4 '|' type8 { push_type(T_OR); }
  | type8
  ;

type2: type2 '|' type3 { push_type(T_OR); }
  | type3 
  ;

type3: F_INT_ID  opt_int_range    { push_type(T_INT); }
  | F_FLOAT_ID    { push_type(T_FLOAT); }
  | F_PROGRAM_ID  { push_type(T_PROGRAM); }
  | F_VOID_ID     { push_type(T_VOID); }
  | F_MIXED_ID    { push_type(T_MIXED); }
  | F_STRING_ID { push_type(T_STRING); }
  | F_OBJECT_ID   opt_object_type { push_type(T_OBJECT); }
  | F_MAPPING_ID opt_mapping_type { push_type(T_MAPPING); }
  | F_ARRAY_ID opt_array_type { push_type(T_ARRAY); }
  | F_MULTISET_ID opt_array_type { push_type(T_MULTISET); }
  | F_FUNCTION_ID opt_function_type { push_type(T_FUNCTION); }
  ;

type8: type3 | identifier_type ;

number_or_maxint: /* Empty */
  {
    $$ = mkintnode(MAX_INT32);
  }
  | F_NUMBER
  | '-' F_NUMBER
  {
#ifdef PIKE_DEBUG
    if (($2->token != F_CONSTANT) || ($2->u.sval.type != T_INT)) {
      fatal("Unexpected number in negative int-range.\n");
    }
#endif /* PIKE_DEBUG */
    $$ = mkintnode(-($2->u.sval.u.integer));
    free_node($2);
  }
  ;

number_or_minint: /* Empty */
  {
    $$ = mkintnode(MIN_INT32);
  }
  | F_NUMBER
  | '-' F_NUMBER
  {
#ifdef PIKE_DEBUG
    if (($2->token != F_CONSTANT) || ($2->u.sval.type != T_INT)) {
      fatal("Unexpected number in negative int-range.\n");
    }
#endif /* PIKE_DEBUG */
    $$ = mkintnode(-($2->u.sval.u.integer));
    free_node($2);
  }
  ;

opt_int_range: /* Empty */
  {
    push_type_int(MAX_INT32);
    push_type_int(MIN_INT32);
  }
  | '(' number_or_minint F_DOT_DOT number_or_maxint ')'
  {
    /* FIXME: Check that $4 is >= $2. */
    if($4->token == F_CONSTANT && $4->u.sval.type == T_INT)
    {
      push_type_int($4->u.sval.u.integer);
    }else{
      push_type_int(MAX_INT32);
    }

    if($2->token == F_CONSTANT && $2->u.sval.type == T_INT)
    {
      push_type_int($2->u.sval.u.integer);
    }else{
      push_type_int(MIN_INT32);
    }

    free_node($2);
    free_node($4);
  }
  ;

opt_object_type:  /* Empty */ { push_type_int(0); push_type(0); }
  | '(' program_ref ')'
  {
    /* NOTE: On entry, there are two items on the stack:
     *   sp-2:	Name of the program reference (string).
     *   sp-1:	The resolved program (program|function|zero).
     */
    struct program *p=program_from_svalue(sp-1);
    if(p)
    {
      push_type_int(p->id);
    }else{
      if (compiler_pass!=1) {
	if ((sp[-2].type == T_STRING) && (sp[-2].u.string->len > 0) &&
	    (sp[-2].u.string->len < 256)) {
	  my_yyerror("Not a valid program specifier: '%s'",
		     sp[-2].u.string->str);
	} else {
	  yyerror("Not a valid program specifier.");
	}
      }
      push_type_int(0);
    }
    pop_n_elems(2);
    push_type(0);
  }
  ;

opt_function_type: '('
  {
    type_stack_mark();
    type_stack_mark();
  }
  function_type_list optional_dot_dot_dot ':'
  {
    if ($4)
    {
      if ($3) {
	push_type(T_MANY);
	type_stack_reverse();
      } else {
	/* function_type_list ends with a comma, or is empty.
	 * FIXME: Should this be a syntax error or not?
	 */
	if (compiler_pass == 1) {
	  yyerror("Missing type before ... .");
	}
	type_stack_reverse();
	push_type(T_MANY);
	push_type(T_MIXED);
      }
    }else{
      type_stack_reverse();
      push_type(T_MANY);
      push_type(T_VOID);
    }
    type_stack_mark();
  }
  type7 ')'
  {
    type_stack_reverse();
    type_stack_reverse();
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
    type_stack_reverse();
    type_stack_mark();
  }
  type7
  ;

opt_array_type: '(' type7 ')'
  |  { push_type(T_MIXED); }
  ;

opt_mapping_type: '('
  { 
    type_stack_mark();
    type_stack_mark();
  }
  type7 ':'
  { 
    type_stack_reverse();
    type_stack_mark();
  }
  type7
  { 
    type_stack_reverse();
    type_stack_reverse();
  }
  ')'
  | /* empty */ 
  {
    push_type(T_MIXED);
    push_type(T_MIXED);
  }
  ;



name_list: new_name
  | name_list ',' new_name
  ;

new_name: optional_stars F_IDENTIFIER
  {
    struct pike_string *type;
    push_finished_type(compiler_frame->current_type);
    if ($1 && (compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    type=compiler_pop_type();
    define_variable($2->u.sval.u.string, type, current_modifiers);
    free_string(type);
    free_node($2);
  }
  | optional_stars bad_identifier {}
  | optional_stars F_IDENTIFIER '='
  {
    struct pike_string *type;
    push_finished_type(compiler_frame->current_type);
    if ($1 && (compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    type=compiler_pop_type();
    if ((current_modifiers & ID_EXTERN) && (compiler_pass == 1)) {
      yywarning("Extern declared variable has initializer.");
    }
    $<number>$=define_variable($2->u.sval.u.string, type,
			       current_modifiers & (~ID_EXTERN));
    free_string(type);
  }
  expr0
  {
    init_node=mknode(F_COMMA_EXPR,init_node,
		     mkcastnode(void_type_string,
				mknode(F_ASSIGN,$5,
				       mkidentifiernode($<number>4))));
    free_node($2);
  }
  | optional_stars F_IDENTIFIER '=' error
  {
    free_node($2);
  }
  | optional_stars F_IDENTIFIER '=' F_LEX_EOF
  {
    yyerror("Unexpected end of file in variable definition.");
    free_node($2);
  }
  | optional_stars bad_identifier '=' expr0
  {
    free_node($4);
  }
  ;


new_local_name: optional_stars F_IDENTIFIER
  {    
    push_finished_type($<n>0->u.sval.u.string);
    if ($1 && (compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    add_local_name($2->u.sval.u.string, compiler_pop_type(),0);
    $$=mknode(F_ASSIGN,mkintnode(0),mklocalnode(islocal($2->u.sval.u.string),0));
    free_node($2);
  }
  | optional_stars bad_identifier { $$=0; }
  | optional_stars F_IDENTIFIER '=' expr0 
  {
    push_finished_type($<n>0->u.sval.u.string);
    if ($1 && (compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    add_local_name($2->u.sval.u.string, compiler_pop_type(),0);
    $$=mknode(F_ASSIGN,$4,mklocalnode(islocal($2->u.sval.u.string),0));
    free_node($2);
  }
  | optional_stars bad_identifier '=' expr0
  {
    free_node($4);
    $$=0;
  }
  | optional_stars F_IDENTIFIER '=' error
  {
    free_node($2);
    /* No yyerok here since we aren't done yet. */
    $$=0;
  }
  | optional_stars F_IDENTIFIER '=' F_LEX_EOF
  {
    yyerror("Unexpected end of file in local variable definition.");
    free_node($2);
    /* No yyerok here since we aren't done yet. */
    $$=0;
  }
  ;

new_local_name2: F_IDENTIFIER
  {
    add_ref($<n>0->u.sval.u.string);
    add_local_name($1->u.sval.u.string, $<n>0->u.sval.u.string, 0);
    $$=mknode(F_ASSIGN,mkintnode(0),mklocalnode(islocal($1->u.sval.u.string),0));
    free_node($1);
  }
  | bad_identifier { $$=0; }
  | F_IDENTIFIER '=' safe_expr0
  {
    add_ref($<n>0->u.sval.u.string);
    add_local_name($1->u.sval.u.string, $<n>0->u.sval.u.string, 0);
    $$=mknode(F_ASSIGN,$3, mklocalnode(islocal($1->u.sval.u.string),0));
    free_node($1);
  }
  | bad_identifier '=' safe_expr0 { $$=$3; }
  ;

block:'{'
  {
    $<number>1=num_used_modules;
    $<number>$=compiler_frame->current_number_of_locals;
  } 
  statements end_block
  {
    unuse_modules(num_used_modules - $<number>1);
    pop_local_variables($<number>2);
    $$=$3;
  }
  ;

end_block: '}'
  | F_LEX_EOF
  {
    yyerror("Missing '}'.");
    yyerror("Unexpected end of file.");
  }
  ;

failsafe_block: block
              | error { $$=0; }
              | F_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
              ;
  

local_name_list: new_local_name
  | local_name_list ',' { $<n>$=$<n>0; } new_local_name { $$=mknode(F_COMMA_EXPR,mkcastnode(void_type_string,$1),$4); }
  ;

local_name_list2: new_local_name2
  | local_name_list2 ',' { $<n>$=$<n>0; } new_local_name { $$=mknode(F_COMMA_EXPR,mkcastnode(void_type_string,$1),$4); }
  ;

statements: { $$=0; }
  | statements statement
  {
    $$=mknode(F_COMMA_EXPR,$1,mkcastnode(void_type_string,$2));
  }
  ;

statement: unused2 ';'
  | import { $$=0; }
  | cond
  | while
  | do
  | for
  | switch
  | case
  | default
  | return expected_semicolon
  | block
  | foreach
  | break expected_semicolon
  | continue expected_semicolon
  | error ';' { reset_type_stack(); $$=0; yyerrok; }
  | error F_LEX_EOF
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


break: F_BREAK { $$=mknode(F_BREAK,0,0); } ;
default: F_DEFAULT ':'  { $$=mknode(F_DEFAULT,0,0); }
  | F_DEFAULT
  {
    $$=mknode(F_DEFAULT,0,0); yyerror("Expected ':' after default.");
  }
  ;

continue: F_CONTINUE { $$=mknode(F_CONTINUE,0,0); } ;

push_compiler_frame1: /* empty */
  {
    push_compiler_frame(1);
  }
  ;

lambda: F_LAMBDA push_compiler_frame1
  {
    debug_malloc_touch(compiler_frame->current_return_type);
    if(compiler_frame->current_return_type)
      free_string(compiler_frame->current_return_type);
    copy_shared_string(compiler_frame->current_return_type,any_type_string);
  }
  func_args
  {
    $<number>$ = varargs;
    varargs = 0;
  }
  failsafe_block
  {
    struct pike_string *type;
    char buf[40];
    int f,e;
    struct pike_string *name;
    
    debug_malloc_touch($6);
    $6=mknode(F_COMMA_EXPR,$6,mknode(F_RETURN,mkintnode(0),0));
    type=find_return_type($6);

    if(type) {
      push_finished_type(type);
      free_string(type);
    } else
      push_type(T_MIXED);
    
    e=$4-1;
    if($<number>5)
    {
      push_finished_type(compiler_frame->variable[e].type);
      e--;
      pop_type_stack();
    }else{
      push_type(T_VOID);
    }
    varargs=0;
    push_type(T_MANY);
    for(; e>=0; e--)
      push_finished_type(compiler_frame->variable[e].type);
    
    push_type(T_FUNCTION);
    
    type=compiler_pop_type();

    sprintf(buf,"__lambda_%ld_%ld",
	    (long)new_program->id,
	    (long)(local_class_counter++ & 0xffffffff)); /* OSF/1 cc bug. */
    name=make_shared_string(buf);

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    compiler_pass, buf, (long)new_program->id, local_class_counter-1);
#endif /* LAMBDA_DEBUG */
    
    f=dooptcode(name,
		$6,
		type,
		ID_STATIC | ID_PRIVATE | ID_INLINE);

    if(compiler_frame->lexical_scope == 2) {
      $$ = mktrampolinenode(f);
    } else {
      $$ = mkidentifiernode(f);
    }
    free_string(name);
    free_string(type);
    pop_compiler_frame();
  }
  | F_LAMBDA push_compiler_frame1 error
  {
    pop_compiler_frame();
    $$ = mkintnode(0);
  }
  ;

local_function: F_IDENTIFIER push_compiler_frame1 func_args 
  {
    char buf[40];
    struct pike_string *name,*type;
    int id,e;
    node *n;
    struct identifier *i=0;

    debug_malloc_touch(compiler_frame->current_return_type);
    if(compiler_frame->current_return_type)
      free_string(compiler_frame->current_return_type);
    copy_shared_string(compiler_frame->current_return_type,
		       $<n>0->u.sval.u.string);


    /***/
    push_finished_type(compiler_frame->current_return_type);
    
    e=$3-1;
    if(varargs)
    {
      push_finished_type(compiler_frame->variable[e].type);
      e--;
      varargs=0;
      pop_type_stack();
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--)
      push_finished_type(compiler_frame->variable[e].type);
    
    push_type(T_FUNCTION);
    
    type=compiler_pop_type();
    /***/

    sprintf(buf,"__lambda_%ld_%ld",
	    (long)new_program->id,
	    (long)(local_class_counter++ & 0xffffffff)); /* OSF/1 cc bug. */

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    compiler_pass, buf, (long)new_program->id, local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    name=make_shared_string(buf);

    id=define_function(name,
		       type,
		       0,
		       IDENTIFIER_PIKE_FUNCTION,
		       0);
    n=0;
#if 0
    if(compiler_pass > 1 &&
       (i=ID_FROM_INT(new_program, id)))
      if(!(i->identifier_flags & IDENTIFIER_SCOPED))
	n = mkidentifiernode(id);
#endif

    low_add_local_name(compiler_frame->previous,
		       $1->u.sval.u.string, type, n);

    $<number>$=id;
    free_string(name);
  }
  failsafe_block
  {
    int localid;
    struct identifier *i=ID_FROM_INT(new_program, $<number>4);

    $5=mknode(F_COMMA_EXPR,$5,mknode(F_RETURN,mkintnode(0),0));

    debug_malloc_touch($5);
    dooptcode(i->name,
	      $5,
	      i->type,
	      ID_STATIC | ID_PRIVATE | ID_INLINE);

    pop_compiler_frame();
    free_node($1);

    /* WARNING: If the local function adds more variables we are screwed */
    /* WARNING2: if add_local_name stops adding local variables at the end,
     *           this has to be fixed.
     */

    localid=compiler_frame->current_number_of_locals-1;
    if(compiler_frame->variable[localid].def)
    {
      $$=copy_node(compiler_frame->variable[localid].def);
    }else{
      $$ = mknode(F_ASSIGN, mktrampolinenode($<number>3),
		mklocalnode(localid,0));
    }
  }
  | F_IDENTIFIER push_compiler_frame1 error
  {
    pop_compiler_frame();
    $$=mkintnode(0);
  }
  ;

local_function2: optional_stars F_IDENTIFIER push_compiler_frame1 func_args 
  {
    char buf[40];
    struct pike_string *name,*type;
    int id,e;
    node *n;
    struct identifier *i=0;

    /***/
    debug_malloc_touch(compiler_frame->current_return_type);
    
    push_finished_type($<n>0->u.sval.u.string);
    if ($1 && (compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);

    if(compiler_frame->current_return_type)
      free_string(compiler_frame->current_return_type);
    compiler_frame->current_return_type=compiler_pop_type();

    /***/
    push_finished_type(compiler_frame->current_return_type);
    
    e=$4-1;
    if(varargs)
    {
      push_finished_type(compiler_frame->variable[e].type);
      e--;
      varargs=0;
      pop_type_stack();
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--)
      push_finished_type(compiler_frame->variable[e].type);
    
    push_type(T_FUNCTION);
    
    type=compiler_pop_type();
    /***/


    sprintf(buf,"__lambda_%ld_%ld",
	    (long)new_program->id,
	    (long)(local_class_counter++ & 0xffffffff)); /* OSF/1 cc bug. */

#ifdef LAMBDA_DEBUG
    fprintf(stderr, "%d: LAMBDA: %s 0x%08lx 0x%08lx\n",
	    compiler_pass, buf, (long)new_program->id, local_class_counter-1);
#endif /* LAMBDA_DEBUG */

    name=make_shared_string(buf);


    id=define_function(name,
		       type,
		       0,
		       IDENTIFIER_PIKE_FUNCTION,
		       0);
    n=0;
#if 0
    if(compiler_pass > 1 &&
       (i=ID_FROM_INT(new_program, id)))
      if(!(i->identifier_flags & IDENTIFIER_SCOPED))
	n = mkidentifiernode(id);
#endif

    low_add_local_name(compiler_frame->previous,
		       $2->u.sval.u.string, type, n);
    $<number>$=id;
    free_string(name);
  }
  failsafe_block
  {
    int localid;
    struct identifier *i=ID_FROM_INT(new_program, $<number>5);

    debug_malloc_touch($6);
    $6=mknode(F_COMMA_EXPR,$6,mknode(F_RETURN,mkintnode(0),0));


    debug_malloc_touch($6);
    dooptcode(i->name,
	      $6,
	      i->type,
	      ID_STATIC | ID_PRIVATE | ID_INLINE);

    pop_compiler_frame();
    free_node($2);

    /* WARNING: If the local function adds more variables we are screwed */
    /* WARNING2: if add_local_name stops adding local variables at the end,
     *           this has to be fixed.
     */

    localid=compiler_frame->current_number_of_locals-1;
    if(compiler_frame->variable[localid].def)
    {
      $$=copy_node(compiler_frame->variable[localid].def);
    }else{
      $$ = mknode(F_ASSIGN, mktrampolinenode($<number>5),
		mklocalnode(localid,0));
    }
  }
  | optional_stars F_IDENTIFIER push_compiler_frame1 error
  {
    pop_compiler_frame();
    free_node($2);
    $$=mkintnode(0);
  }
  ;


failsafe_program: '{' program end_block
                | error { yyerrok; }
                | F_LEX_EOF
                {
		  yyerror("End of file where program definition expected.");
		}
                ;

class: modifiers F_CLASS optional_identifier
  {
    extern int num_parse_error;
    int num_errors=num_parse_error;
    if(!$3)
    {
      struct pike_string *s;
      char buffer[42];
      sprintf(buffer,"__class_%ld_%ld",(long)new_program->id,
	      local_class_counter++);
      s=make_shared_string(buffer);
      $3=mkstrnode(s);
      free_string(s);
      $1|=ID_STATIC | ID_PRIVATE | ID_INLINE;
    }
    /* fprintf(stderr, "LANGUAGE.YACC: CLASS start\n"); */
    if(compiler_pass==1)
    {
      if ($1 & ID_EXTERN) {
	yywarning("Extern declared class definition.");
      }
      low_start_new_program(0, $3->u.sval.u.string,
			    $1,
			    &$<number>$);
      if(lex.current_file)
      {
	store_linenumber(lex.current_line, lex.current_file);
	debug_malloc_name(new_program, lex.current_file->str,
			  lex.current_line);
      }
    }else{
      int i;
      struct program *p;
      struct identifier *id;
      int tmp=compiler_pass;
      i=isidentifier($3->u.sval.u.string);
      if(i<0)
      {
	low_start_new_program(new_program,0,
			      $1,
			      &$<number>$);
	yyerror("Pass 2: program not defined!");
      }else{
	id=ID_FROM_INT(new_program, i);
	if(IDENTIFIER_IS_CONSTANT(id->identifier_flags))
	{
	  struct svalue *s;
	  s=&PROG_FROM_INT(new_program,i)->constants[id->func.offset].sval;
	  if(s->type==T_PROGRAM)
	  {
	    low_start_new_program(s->u.program,
				  $3->u.sval.u.string,
				  $1,
				  &$<number>$);
	  }else{
	    yyerror("Pass 2: constant redefined!");
	    low_start_new_program(new_program, 0,
				  $1,
				  &$<number>$);
	  }
	}else{
	  yyerror("Pass 2: class constant no longer constant!");
	  low_start_new_program(new_program, 0,
				$1,
				&$<number>$);
	}
      }
      compiler_pass=tmp;
    }
    num_parse_error=num_errors; /* Kluge to prevent gazillion error messages */
  }
  failsafe_program
  {
    struct program *p;
    if(compiler_pass == 1)
      p=end_first_pass(0);
    else
      p=end_first_pass(1);

    /* fprintf(stderr, "LANGUAGE.YACC: CLASS end\n"); */

    if(!p) {
      yyerror("Class definition failed.");
    }else{
      free_program(p);
    }

    $$=mkidentifiernode($<number>4);

    free_node($3);
    check_tree($$,0);
  }
  ;

cond: F_IF
  {
    $<number>$=compiler_frame->current_number_of_locals;
  }
  '(' safe_comma_expr end_cond statement optional_else_part
  {
    $$=mknode('?',$4,mknode(':',$6,$7));
    $$->line_number=$1;
    $$=mkcastnode(void_type_string,$$);
    $$->line_number=$1;
    pop_local_variables($<number>2);
  }
  ;

end_cond: ')'
  | '}' { yyerror("Missing ')'."); }
  | F_LEX_EOF
  {
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  ;

optional_else_part: { $$=0; }
  | F_ELSE statement { $$=$2; }
  ;      

safe_lvalue: lvalue
  | error { $$=0; }
  ;

safe_expr0: expr0
  | F_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
  | error { $$=0; }
  ;

foreach: F_FOREACH
  {
    $<number>$=compiler_frame->current_number_of_locals;
  }
  '(' expr0 ',' safe_lvalue end_cond statement
  {
    if ($6) {
      $$=mknode(F_FOREACH, mknode(F_VAL_LVAL,$4,$6),$8);
      $$->line_number=$1;
    } else {
      /* Error in lvalue */
      free_node($4);
      $$=$8;
    }
    pop_local_variables($<number>2);
  }
  ;

do: F_DO statement F_WHILE '(' safe_comma_expr end_cond expected_semicolon
  {
    $$=mknode(F_DO,$2,$5);
    $$->line_number=$1;
  }
  | F_DO statement F_WHILE F_LEX_EOF
  {
    $$=0;
    yyerror("Missing '(' in do-while loop.");
    yyerror("Unexpected end of file.");
  }
  | F_DO statement F_LEX_EOF
  {
    $$=0;
    yyerror("Missing 'while' in do-while loop.");
    yyerror("Unexpected end of file.");
  }
  ;

expected_semicolon: ';'
  | F_LEX_EOF
  {
    yyerror("Missing ';'.");
    yyerror("Unexpected end of file.");
  }
  ;

for: F_FOR
  {
    $<number>$=compiler_frame->current_number_of_locals;
  }
  '(' unused expected_semicolon for_expr expected_semicolon unused end_cond
  statement
  {
    int i=lex.current_line;
    lex.current_line=$1;
    $$=mknode(F_COMMA_EXPR, mkcastnode(void_type_string,$4),
	      mknode(F_FOR,$6,mknode(':',$10,$8)));
    lex.current_line=i;
    pop_local_variables($<number>2);
  }
  ;


while:  F_WHILE
  {
    $<number>$=compiler_frame->current_number_of_locals;
  }
  '(' safe_comma_expr end_cond statement
  {
    int i=lex.current_line;
    lex.current_line=$1;
    $$=mknode(F_FOR,$4,mknode(':',$6,NULL));
    lex.current_line=i;
    pop_local_variables($<number>2);
  }
  ;

for_expr: /* EMPTY */ { $$=mkintnode(1); }
  | safe_comma_expr
  ;

switch:	F_SWITCH
  {
    $<number>$=compiler_frame->current_number_of_locals;
  }
  '(' safe_comma_expr end_cond statement
  {
    $$=mknode(F_SWITCH,$4,$6);
    $$->line_number=$1;
    pop_local_variables($<number>2);
  }
  ;

case: F_CASE safe_comma_expr expected_colon
  {
    $$=mknode(F_CASE,$2,0);
  }
  | F_CASE safe_comma_expr F_DOT_DOT optional_comma_expr expected_colon
  {
     $$=mknode(F_CASE,$4?$2:0,$4?$4:$2);
  }
  ;

expected_colon: ':'
  | ';'
  {
    yyerror("Missing ':'.");
  }
  | '{'
  {
    yyerror("Missing ':'.");
  }
  | '}'
  {
    yyerror("Missing ':'.");
  }
  | F_LEX_EOF
  {
    yyerror("Missing ':'.");
    yyerror("Unexpected end of file.");
  }
  ;

return: F_RETURN
  {
    if(!match_types(compiler_frame->current_return_type,
		    void_type_string))
    {
      yyerror("Must return a value for a non-void function.");
    }
    $$=mknode(F_RETURN,mkintnode(0),0);
  }
  | F_RETURN safe_comma_expr
  {
    $$=mknode(F_RETURN,$2,0);
  }
  ;
	
unused: { $$=0; }
  | safe_comma_expr { $$=mkcastnode(void_type_string,$1);  }
  ;

unused2: comma_expr { $$=mkcastnode(void_type_string,$1);  } ;

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
    $$ = mknode(F_COMMA_EXPR,mkcastnode(void_type_string,$1),$3); 
  }
  ;

expr00: expr0
      | '@' expr0 { $$=mknode(F_PUSH_ARRAY,$2,0); };

expr0: expr01
  | expr4 '=' expr0  { $$=mknode(F_ASSIGN,$3,$1); }
  | expr4 '=' error { $$=$1; reset_type_stack(); yyerrok; }
  | bad_expr_ident '=' expr0 { $$=$3; }
  | '[' low_lvalue_list ']' '=' expr0  { $$=mknode(F_ASSIGN,$5,mknode(F_ARRAY_LVALUE,$2,0)); }
  | expr4 assign expr0  { $$=mknode($2,$1,$3); }
  | expr4 assign error { $$=$1; reset_type_stack(); yyerrok; }
  | bad_expr_ident assign expr0 { $$=$3; }
  | '[' low_lvalue_list ']' assign expr0  { $$=mknode($4,mknode(F_ARRAY_LVALUE,$2,0),$5); }
  | '[' low_lvalue_list ']' error { $$=$2; reset_type_stack(); yyerrok; }
/*  | error { $$=0; reset_type_stack(); } */
  ;

expr01: expr1
  | expr1 '?' expr01 ':' expr01 { $$=mknode('?',$1,mknode(':',$3,$5)); }
  ;

assign: F_AND_EQ { $$=F_AND_EQ; }
  | F_OR_EQ  { $$=F_OR_EQ; }
  | F_XOR_EQ { $$=F_XOR_EQ; }
  | F_LSH_EQ { $$=F_LSH_EQ; }
  | F_RSH_EQ { $$=F_RSH_EQ; }
  | F_ADD_EQ { $$=F_ADD_EQ; }
  | F_SUB_EQ { $$=F_SUB_EQ; }
  | F_MULT_EQ{ $$=F_MULT_EQ; }
  | F_MOD_EQ { $$=F_MOD_EQ; }
  | F_DIV_EQ { $$=F_DIV_EQ; }
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

assoc_pair:  expr0 expected_colon expr1 { $$=mknode(F_ARG_LIST,$1,$3); }
  | expr0 expected_colon error { free_node($1); $$=0; }
  ;

expr1: expr2
  | expr1 F_LOR expr1  { $$=mknode(F_LOR,$1,$3); }
  | expr1 F_LAND expr1 { $$=mknode(F_LAND,$1,$3); }
  | expr1 '|' expr1    { $$=mkopernode("`|",$1,$3); }
  | expr1 '^' expr1    { $$=mkopernode("`^",$1,$3); }
  | expr1 '&' expr1    { $$=mkopernode("`&",$1,$3); }
  | expr1 F_EQ expr1   { $$=mkopernode("`==",$1,$3); }
  | expr1 F_NE expr1   { $$=mkopernode("`!=",$1,$3); }
  | expr1 '>' expr1    { $$=mkopernode("`>",$1,$3); }
  | expr1 F_GE expr1   { $$=mkopernode("`>=",$1,$3); }
  | expr1 '<' expr1    { $$=mkopernode("`<",$1,$3); }
  | expr1 F_LE expr1   { $$=mkopernode("`<=",$1,$3); }
  | expr1 F_LSH expr1  { $$=mkopernode("`<<",$1,$3); }
  | expr1 F_RSH expr1  { $$=mkopernode("`>>",$1,$3); }
  | expr1 '+' expr1    { $$=mkopernode("`+",$1,$3); }
  | expr1 '-' expr1    { $$=mkopernode("`-",$1,$3); }
  | expr1 '*' expr1    { $$=mkopernode("`*",$1,$3); }
  | expr1 '%' expr1    { $$=mkopernode("`%",$1,$3); }
  | expr1 '/' expr1    { $$=mkopernode("`/",$1,$3); }
  | expr1 F_LOR error 
  | expr1 F_LAND error
  | expr1 '|' error   
  | expr1 '^' error   
  | expr1 '&' error   
  | expr1 F_EQ error  
  | expr1 F_NE error  
  | expr1 '>' error   
  | expr1 F_GE error  
  | expr1 '<' error   
  | expr1 F_LE error  
  | expr1 F_LSH error 
  | expr1 F_RSH error 
  | expr1 '+' error   
  | expr1 '-' error   
  | expr1 '*' error   
  | expr1 '%' error   
  | expr1 '/' error
  ;

expr2: expr3
  | cast expr2
  {
    $$=mkcastnode($1->u.sval.u.string,$2);
    free_node($1);
  }
  | soft_cast expr2
  {
    $$=mksoftcastnode($1->u.sval.u.string,$2);
    free_node($1);
  }
  | F_INC expr4       { $$=mknode(F_INC,$2,0); }
  | F_DEC expr4       { $$=mknode(F_DEC,$2,0); }
  | F_NOT expr2        { $$=mkopernode("`!",$2,0); }
  | '~' expr2          { $$=mkopernode("`~",$2,0); }
  | '-' expr2          { $$=mkopernode("`-",$2,0); }
  ;

expr3: expr4
  | expr4 F_INC       { $$=mknode(F_POST_INC,$1,0); }
  | expr4 F_DEC       { $$=mknode(F_POST_DEC,$1,0); }
  ;

expr4: string
  | F_NUMBER 
  | F_FLOAT { $$=mkfloatnode((FLOAT_TYPE)$1); }
  | catch
  | gauge
  | typeof
  | sscanf
  | lambda
  | class
  | idents
  | expr4 '(' expr_list ')' { $$=mkapplynode($1,$3); }
  | expr4 '(' error ')' { $$=mkapplynode($1, NULL); yyerrok; }
  | expr4 '(' error F_LEX_EOF
  {
    yyerror("Missing ')'."); $$=mkapplynode($1, NULL);
    yyerror("Unexpected end of file.");
  }
  | expr4 '(' error ';' { yyerror("Missing ')'."); $$=mkapplynode($1, NULL); }
  | expr4 '(' error '}' { yyerror("Missing ')'."); $$=mkapplynode($1, NULL); }
  | expr4 '[' expr0 ']' { $$=mknode(F_INDEX,$1,$3); }
  | expr4 '['  comma_expr_or_zero F_DOT_DOT comma_expr_or_maxint ']'
  {
    $$=mknode(F_RANGE,$1,mknode(F_ARG_LIST,$3,$5));
  }
  | expr4 '[' error ']' { $$=$1; yyerrok; }
  | expr4 '[' error F_LEX_EOF
  {
    $$=$1; yyerror("Missing ']'.");
    yyerror("Unexpected end of file.");
  }
  | expr4 '[' error ';' { $$=$1; yyerror("Missing ']'."); }
  | expr4 '[' error '}' { $$=$1; yyerror("Missing ']'."); }
  | expr4 '[' error ')' { $$=$1; yyerror("Missing ']'."); }
  | '(' comma_expr2 ')' { $$=$2; }
  | '(' '{' expr_list close_brace_or_missing ')'
    { $$=mkefuncallnode("aggregate",$3); }
  | '(' '[' m_expr_list close_bracket_or_missing ')'
    { $$=mkefuncallnode("aggregate_mapping",$3); }
  | F_MULTISET_START expr_list F_MULTISET_END
    { $$=mkefuncallnode("aggregate_multiset",$2); }
  | F_MULTISET_START expr_list ')'
    {
      yyerror("Missing '>'.");
      $$=mkefuncallnode("aggregate_multiset",$2);
    }
  | '(' error ')' { $$=0; yyerrok; }
  | '(' error F_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | '(' error ';' { $$=0; yyerror("Missing ')'."); }
  | '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | F_MULTISET_START error F_MULTISET_END { $$=0; yyerrok; }
  | F_MULTISET_START error ')' {
    yyerror("Missing '>'.");
    $$=0; yyerrok;
  }
  | F_MULTISET_START error F_LEX_EOF
  {
    $$=0; yyerror("Missing '>)'.");
    yyerror("Unexpected end of file.");
  }
  | F_MULTISET_START error ';' { $$=0; yyerror("Missing '>)'."); }
  | F_MULTISET_START error '}' { $$=0; yyerror("Missing '>)'."); }
  | expr4 F_ARROW magic_identifier
  {
    $$=mknode(F_ARROW,$1,$3);
  }
  | expr4 F_ARROW error {}
  ;

idents: low_idents
  | idents '.' F_IDENTIFIER
  {
    $$=index_node($1, last_identifier?last_identifier->str:NULL,
		  $3->u.sval.u.string);
    free_node($1);
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $3->u.sval.u.string);
    free_node($3);
  }
  | '.' F_IDENTIFIER
  {
    node *tmp;
    push_text(".");
    ref_push_string(lex.current_file);
    if (error_handler && error_handler->prog) {
      ref_push_object(error_handler);
      SAFE_APPLY_MASTER("handle_import", 3);
    } else {
      SAFE_APPLY_MASTER("handle_import", 2);
    }
    tmp=mkconstantsvaluenode(sp-1);
    pop_stack();
    $$=index_node(tmp, ".", $2->u.sval.u.string);
    free_node(tmp);
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $2->u.sval.u.string);
    free_node($2);
  }
  | idents '.' bad_identifier {}
  | idents '.' error {}
  ;

low_idents: F_IDENTIFIER
  {
    int i;
    struct efun *f;
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $1->u.sval.u.string);

    if(($$=lexical_islocal(last_identifier)))
    {
      /* done, nothing to do here */
    }else if((i=isidentifier(last_identifier))>=0){
      $$=mkidentifiernode(i);
    }else if(!($$=find_module_identifier(last_identifier,1))){
      if(!num_parse_error)
      {
	if(compiler_pass==2)
	{
	  my_yyerror("'%s' undefined.", last_identifier->str);
	  $$=0;
	}else{
	  $$=mknode(F_UNDEFINED,0,0);
	}
      }else{
	$$=mkintnode(0);
      }
    }
    free_node($1);
  }
  | F_PREDEF F_COLON_COLON F_IDENTIFIER
  {
    struct svalue tmp;
    node *tmp2;
    tmp.type=T_MAPPING;
#ifdef __CHECKER__
    tmp.subtype=0;
#endif /* __CHECKER__ */
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $3->u.sval.u.string);
    tmp.u.mapping=get_builtin_constants();
    tmp2=mkconstantsvaluenode(&tmp);
    $$=index_node(tmp2, "predef", $3->u.sval.u.string);
    if(!$$->name)
      add_ref( $$->name=$3->u.sval.u.string );
    free_node(tmp2);
    free_node($3);
  }
  | F_PREDEF F_COLON_COLON bad_identifier
  {
    $$=0;
  }
  | F_IDENTIFIER F_COLON_COLON F_IDENTIFIER
  {
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $3->u.sval.u.string);

    $$=reference_inherited_identifier($1->u.sval.u.string,
				     $3->u.sval.u.string);

    if (!$$)
    {
      my_yyerror("Undefined identifier %s::%s", 
		 $1->u.sval.u.string->str,
		 $3->u.sval.u.string->str);
      $$=0;
    }

    free_node($1);
    free_node($3);
  }
  | F_IDENTIFIER F_COLON_COLON bad_identifier
  | F_IDENTIFIER F_COLON_COLON error
  | F_COLON_COLON F_IDENTIFIER
  {
    int e,i;

    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $2->u.sval.u.string);

    $$=0;
    for(e=1;e<(int)new_program->num_inherits;e++)
    {
      if(new_program->inherits[e].inherit_level!=1) continue;
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
	if(ISCONSTSTR($2->u.sval.u.string,"`->") ||
	   ISCONSTSTR($2->u.sval.u.string,"`[]") )
	{
	  $$=mknode(F_MAGIC_INDEX,mknewintnode(0),mknewintnode(0));
	}
	else if(ISCONSTSTR($2->u.sval.u.string,"`->=") ||
		ISCONSTSTR($2->u.sval.u.string,"`[]=") )
	{
	  $$=mknode(F_MAGIC_SET_INDEX,mknewintnode(0),mknewintnode(0));
	}
	else
	{
	  $$=mkintnode(0);
	}
    }else{
      if($$->token==F_ARG_LIST) $$=mkefuncallnode("aggregate",$$);
    }
    free_node($2);
  }
  | F_COLON_COLON bad_identifier
  {
    $$=0;
  }
  ;

comma_expr_or_zero: /* empty */ { $$=mkintnode(0); }
  | comma_expr
  | F_LEX_EOF { yyerror("Unexpected end of file."); $$=0; }
  ;

comma_expr_or_maxint: /* empty */ { $$=mkintnode(0x7fffffff); }
  | comma_expr
  | F_LEX_EOF { yyerror("Unexpected end of file."); $$=mkintnode(0x7fffffff); }
  ;

gauge: F_GAUGE catch_arg
  {
#ifdef HAVE_GETHRVTIME
    $$=mkefuncallnode("abs",
		  mkopernode("`/", 
			     mkopernode("`-", mkefuncallnode("gethrvtime",0),
					mknode(F_COMMA_EXPR,
					       mknode(F_POP_VALUE, $2, NULL),
					       mkefuncallnode("gethrvtime",0))),
			     mkfloatnode((FLOAT_TYPE)1000000.0)));
#else
  $$=mkefuncallnode("abs",
	mkopernode("`/", 
		mkopernode("`-",
			 mknode(F_INDEX,mkefuncallnode("rusage",0),
				mkintnode(GAUGE_RUSAGE_INDEX)),
			   mknode(F_COMMA_EXPR, mknode(F_POP_VALUE, $2, NULL),
				mknode(F_INDEX,mkefuncallnode("rusage",0),
				       mkintnode(GAUGE_RUSAGE_INDEX)))),
		mkfloatnode((FLOAT_TYPE)1000.0)));
#endif
  };

typeof: F_TYPEOF '(' expr0 ')'
  {
    struct pike_string *s;
    node *tmp;

    /* FIXME: Why build the node at all? */

    tmp=mknode(F_COMMA_EXPR,$3,0);

    s=describe_type( tmp && CAR(tmp) && CAR(tmp)->type ? CAR(tmp)->type : mixed_type_string);
    $$=mkstrnode(s);
    free_string(s);
    free_node(tmp);
  } 
  | F_TYPEOF '(' error ')' { $$=0; yyerrok; }
  | F_TYPEOF '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | F_TYPEOF '(' error F_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | F_TYPEOF '(' error ';' { $$=0; yyerror("Missing ')'."); }
  ;
 
catch_arg: '(' comma_expr ')'  { $$=$2; }
  | '(' error ')' { $$=0; yyerrok; }
  | '(' error F_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | '(' error ';' { $$=0; yyerror("Missing ')'."); }
  | block
  | error { $$=0; yyerror("Bad expression for catch."); }
  ; 

catch: F_CATCH
     {
       catch_level++;
     }
     catch_arg
     {
       $$=mknode(F_CATCH,$3,NULL);
       catch_level--;
     }
     ;

sscanf: F_SSCANF '(' expr0 ',' expr0 lvalue_list ')'
  {
    $$=mknode(F_SSCANF,mknode(F_ARG_LIST,$3,$5),$6);
  }
  | F_SSCANF '(' expr0 ',' expr0 error ')'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerrok;
  }
  | F_SSCANF '(' expr0 ',' expr0 error F_LEX_EOF
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | F_SSCANF '(' expr0 ',' expr0 error '}'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
  }
  | F_SSCANF '(' expr0 ',' expr0 error ';'
  {
    $$=0;
    free_node($3);
    free_node($5);
    yyerror("Missing ')'.");
  }
  | F_SSCANF '(' expr0 error ')'
  {
    $$=0;
    free_node($3);
    yyerrok;
  }
  | F_SSCANF '(' expr0 error F_LEX_EOF
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | F_SSCANF '(' expr0 error '}'
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
  }
  | F_SSCANF '(' expr0 error ';'
  {
    $$=0;
    free_node($3);
    yyerror("Missing ')'.");
  }
  | F_SSCANF '(' error ')' { $$=0; yyerrok; }
  | F_SSCANF '(' error F_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | F_SSCANF '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | F_SSCANF '(' error ';' { $$=0; yyerror("Missing ')'."); }
  ;

lvalue: expr4
  | '[' low_lvalue_list ']' { $$=mknode(F_ARRAY_LVALUE, $2,0); }
  | type6 F_IDENTIFIER
  {
    add_local_name($2->u.sval.u.string,compiler_pop_type(),0);
    $$=mklocalnode(islocal($2->u.sval.u.string),-1);
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

string: F_STRING 
  | string F_STRING
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

/* FIXME: Should probably set last_identifier. */
bad_identifier: bad_expr_ident
  | F_CLASS
  { yyerror("class is a reserved word."); }
  | F_ARRAY_ID
  { yyerror("array is a reserved word."); }
  | F_FLOAT_ID
  { yyerror("float is a reserved word.");}
  | F_FUNCTION_ID
  { yyerror("function is a reserved word.");}
  | F_INT_ID
  { yyerror("int is a reserved word."); }
  | F_MAPPING_ID
  { yyerror("mapping is a reserved word."); }
  | F_MIXED_ID
  { yyerror("mixed is a reserved word."); }
  | F_MULTISET_ID
  { yyerror("multiset is a reserved word."); }
  | F_OBJECT_ID
  { yyerror("object is a reserved word."); }
  | F_PROGRAM_ID
  { yyerror("program is a reserved word."); }
  | F_STRING_ID
  { yyerror("string is a reserved word."); }
  | F_VOID_ID
  { yyerror("void is a reserved word."); }
  ;

bad_expr_ident:
    F_INLINE
  { yyerror("inline is a reserved word."); }
  | F_LOCAL_ID
  { yyerror("local is a reserved word."); }
  | F_NO_MASK
  { yyerror("nomask is a reserved word."); }
  | F_PREDEF
  { yyerror("predef is a reserved word."); }
  | F_PRIVATE
  { yyerror("private is a reserved word."); }
  | F_PROTECTED
  { yyerror("protected is a reserved word."); }
  | F_PUBLIC
  { yyerror("public is a reserved word."); }
  | F_OPTIONAL
  { yyerror("optional is a reserved word."); }
  | F_STATIC
  { yyerror("static is a reserved word."); }
  | F_EXTERN
  { yyerror("extern is a reserved word."); }
  | F_FINAL_ID
  { yyerror("final is a reserved word.");}
  | F_DO
  { yyerror("do is a reserved word."); }
  | F_ELSE
  { yyerror("else without if."); }
  | F_RETURN
  { yyerror("return is a reserved word."); }
  | F_CONSTANT
  { yyerror("constant is a reserved word."); }
  | F_IMPORT
  { yyerror("import is a reserved word."); }
  | F_INHERIT
  { yyerror("inherit is a reserved word."); }
  | F_CATCH
  { yyerror("catch is a reserved word."); }
  | F_GAUGE
  { yyerror("gauge is a reserved word."); }
  | F_LAMBDA
  { yyerror("lambda is a reserved word."); }
  | F_SSCANF
  { yyerror("sscanf is a reserved word."); }
  | F_SWITCH
  { yyerror("switch is a reserved word."); }
  | F_TYPEOF
  { yyerror("typeof is a reserved word."); }
  | F_BREAK
  { yyerror("break is a reserved word."); }
  | F_CASE
  { yyerror("case is a reserved word."); }
  | F_CONTINUE
  { yyerror("continue is a reserved word."); }
  | F_DEFAULT
  { yyerror("default is a reserved word."); }
  | F_FOR
  { yyerror("for is a reserved word."); }
  | F_FOREACH
  { yyerror("foreach is a reserved word."); }
  | F_IF
  { yyerror("if is a reserved word."); }
  ;



%%

void yyerror(char *str)
{
  extern int num_parse_error;
  extern int cumulative_parse_error;

#ifdef PIKE_DEBUG
  if(recoveries && sp-evaluator_stack < recoveries->sp)
    fatal("Stack error (underflow)\n");
#endif

  if (num_parse_error > 10) return;
  num_parse_error++;
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
    if (error_handler && error_handler->prog) {
      safe_apply(error_handler, "compile_error", 3);
    } else {
      SAFE_APPLY_MASTER("compile_error", 3);
    }
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
}


int low_add_local_name(struct compiler_frame *frame,
			struct pike_string *str,
			struct pike_string *type,
			node *def)
{
  debug_malloc_touch(def);
  debug_malloc_touch(type);
  debug_malloc_touch(str);
  reference_shared_string(str);
  if (frame->current_number_of_locals == MAX_LOCAL)
  {
    yyerror("Too many local variables.");
    return 0;
  }else {
#ifdef PIKE_DEBUG
    check_type_string(type);
#endif /* PIKE_DEBUG */
    if (pike_types_le(type, void_type_string)) {
      if (compiler_pass != 1) {
	yywarning("Declaring local variable with type void "
		  "(converted to type zero).");
      }
      free_string(type);
      copy_shared_string(type, zero_type_string);
    }
    frame->variable[frame->current_number_of_locals].type = type;
    frame->variable[frame->current_number_of_locals].name = str;
    frame->variable[frame->current_number_of_locals].def = def;
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
		   struct pike_string *type,
		   node *def)
{
  return low_add_local_name(compiler_frame,
			    str,
			    type,
			    def);
}

/* argument must be a shared string */
int islocal(struct pike_string *str)
{
  int e;
  for(e=compiler_frame->current_number_of_locals-1;e>=0;e--)
    if(compiler_frame->variable[e].name==str)
      return e;
  return -1;
}

/* argument must be a shared string */
static node *lexical_islocal(struct pike_string *str)
{
  int e,depth=0;
  struct compiler_frame *f=compiler_frame;
  
  while(1)
  {
    for(e=f->current_number_of_locals-1;e>=0;e--)
    {
      if(f->variable[e].name==str)
      {
	struct compiler_frame *q=compiler_frame;
	if(f->variable[e].def)
	  return copy_node(f->variable[e].def);
	while(q!=f) 
	{
	  q->lexical_scope=2;
	  q=q->previous;
	}
	if (depth) {
	  if(q->min_number_of_locals < e+1)
	    q->min_number_of_locals = e+1;
	}

	return mklocalnode(e,depth);
      }
    }
    if(!f->lexical_scope) return 0;
    depth++;
    f=f->previous;
  }
}

void cleanup_compiler(void)
{
  if(last_identifier)
  {
    free_string(last_identifier);
    last_identifier=0;
  }
}
