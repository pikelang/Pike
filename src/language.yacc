/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
%pure_parser

/*
 * These values are used by the stack machine, and can not be directly
 * called from Pike.
 */
%token F_PREFIX_256 F_PREFIX_512 F_PREFIX_768 F_PREFIX_1024
%token F_PREFIX_CHARX256 F_PREFIX_WORDX256 F_PREFIX_24BITX256
%token F_POP_VALUE F_POP_N_ELEMS F_MARK F_MARK2
%token F_CALL_LFUN F_CALL_LFUN_AND_POP
%token F_APPLY F_APPLY_AND_POP F_MARK_APPLY F_MARK_APPLY_POP

%token F_BRANCH F_BRANCH_WHEN_ZERO F_BRANCH_WHEN_NON_ZERO
%token F_BRANCH_WHEN_LT F_BRANCH_WHEN_GT
%token F_BRANCH_WHEN_LE F_BRANCH_WHEN_GE
%token F_BRANCH_WHEN_EQ F_BRANCH_WHEN_NE
%token F_INC_LOOP F_DEC_LOOP
%token F_INC_NEQ_LOOP F_DEC_NEQ_LOOP

%token F_INDEX F_ARROW F_INDIRECT F_STRING_INDEX F_LOCAL_INDEX
%token F_POS_INT_INDEX F_NEG_INT_INDEX
%token F_LTOSVAL F_LTOSVAL2
%token F_PUSH_ARRAY 
%token F_RANGE F_COPY_VALUE

/*
 * Basic value pushing
 */
%token F_LFUN F_GLOBAL F_LOCAL F_2_LOCALS F_MARK_AND_LOCAL
%token F_GLOBAL_LVALUE F_LOCAL_LVALUE
%token F_CLEAR_LOCAL F_CLEAR_2_LOCAL F_CLEAR_STRING_SUBTYPE
%token F_CONSTANT F_FLOAT F_STRING F_ARROW_STRING
%token F_NUMBER F_NEG_NUMBER F_CONST_1 F_CONST0 F_CONST1 F_BIGNUM
/*
 * These are the predefined functions that can be accessed from Pike.
 */

%token F_INC F_DEC F_POST_INC F_POST_DEC F_INC_AND_POP F_DEC_AND_POP
%token F_INC_LOCAL F_INC_LOCAL_AND_POP F_POST_INC_LOCAL
%token F_DEC_LOCAL F_DEC_LOCAL_AND_POP F_POST_DEC_LOCAL
%token F_RETURN F_DUMB_RETURN F_RETURN_0 F_RETURN_1 F_THROW_ZERO

%token F_ASSIGN F_ASSIGN_AND_POP
%token F_ASSIGN_LOCAL F_ASSIGN_LOCAL_AND_POP
%token F_ASSIGN_GLOBAL F_ASSIGN_GLOBAL_AND_POP
%token F_ADD F_SUBTRACT F_ADD_INT F_ADD_NEG_INT
%token F_MULTIPLY F_DIVIDE F_MOD

%token F_LT F_GT F_EQ F_GE F_LE F_NE
%token F_NEGATE F_NOT F_COMPL
%token F_AND F_OR F_XOR
%token F_LSH F_RSH
%token F_LAND F_LOR
%token F_EQ_OR F_EQ_AND

%token F_SWITCH F_SSCANF F_CATCH
%token F_CAST
%token F_FOREACH

%token F_SIZEOF F_SIZEOF_LOCAL
%token F_ADD_TO_AND_POP

/*
 * These are token values that needn't have an associated code for the
 * compiled file
 */

%token F_MAX_OPCODE
%token F_ADD_EQ
%token F_AND_EQ
%token F_ARG_LIST
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
%token F_PREDEF
%token F_EFUN_CALL
%token F_ELSE
%token F_FLOAT_ID
%token F_FOR
%token F_FUNCTION_ID
%token F_GAUGE
%token F_IDENTIFIER
%token F_IF
%token F_IMPORT
%token F_INHERIT
%token F_INLINE
%token F_INT_ID
%token F_LAMBDA
%token F_MULTISET_ID
%token F_MULTISET_END
%token F_MULTISET_START
%token F_LOCAL
%token F_LSH_EQ
%token F_LVALUE_LIST
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
%token F_PUBLIC
%token F_RSH_EQ
%token F_STATIC
%token F_STATUS
%token F_STRING_ID
%token F_SUBSCRIPT
%token F_SUB_EQ
%token F_TYPEOF
%token F_VAL_LVAL
%token F_VARARGS 
%token F_VOID_ID
%token F_WHILE
%token F_XOR_EQ
%token F_NOP

%token F_ALIGN
%token F_POINTER
%token F_LABEL
%token F_BYTE

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
RCSID("$Id: language.yacc,v 1.50 1997/10/27 09:59:21 hubbe Exp $");
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include "interpret.h"
#include "array.h"
#include "object.h"
#include "stralloc.h"
#include "las.h"
#include "interpret.h"
#include "lex.h"
#include "program.h"
#include "pike_types.h"
#include "constants.h"
#include "pike_macros.h"
#include "error.h"
#include "docode.h"
#include "machine.h"

#define YYMAXDEPTH	600

#ifdef DEBUG
#define YYDEBUG 1
#endif

void free_all_local_names(void);
void add_local_name(struct pike_string *,struct pike_string *);

/*
 * The names and types of arguments and auto variables.
 */
struct locals *local_variables = 0;

static int varargs;
static INT32  current_modifiers;
static struct pike_string *last_identifier=0;

void fix_comp_stack(int sp)
{
  if(comp_stackp>sp)
  {
    yyerror("Compiler stack fixed.");
    comp_stackp=sp;
  }else if(comp_stackp<sp){
    fatal("Compiler stack frame underflow.");
  }
}

/*
 * Kludge for Bison not using prototypes.
 */
#ifndef __GNUC__
#ifndef __cplusplus
static void __yy_memcpy(char *to, char *from, int count);
#endif /* !__cplusplus */
#endif /* !__GNUC__ */

%}

%union
{
  int number;
  FLOAT_TYPE fnum;
  unsigned int address;		/* Address of an instruction */
  struct pike_string *string;
  char *str;
  unsigned short type;
  struct node_s *n;
  struct efun *efun;
}

%{
int yylex(YYSTYPE *yylval);
%}

%type <fnum> F_FLOAT

%type <string> F_IDENTIFIER
%type <string> F_STRING
%type <string> cast
%type <string> simple_type
%type <string> low_string
%type <string> optional_identifier
%type <string> optional_rename_inherit
%type <string> string_constant

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
%type <number> F_NUMBER
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
%type <number> F_VARARGS
%type <number> F_VOID_ID
%type <number> F_WHILE
%type <number> arguments
%type <number> arguments2
%type <number> func_args
%type <number> assign
%type <number> modifier
%type <number> modifier_list
%type <number> modifiers
%type <number> optional_dot_dot_dot
%type <number> optional_stars

/* The following symbos return type information */

%type <n> assoc_pair
%type <n> block
%type <n> failsafe_block
%type <n> block_or_semi
%type <n> break
%type <n> case
%type <n> catch
%type <n> catch_arg
%type <n> class
%type <n> comma_expr
%type <n> comma_expr2
%type <n> comma_expr_or_maxint
%type <n> comma_expr_or_zero
%type <n> cond
%type <n> continue
%type <n> default
%type <n> do
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
%type <n> low_idents
%type <n> lvalue
%type <n> lvalue_list
%type <n> m_expr_list
%type <n> m_expr_list2
%type <n> new_local_name
%type <n> optional_comma_expr
%type <n> optional_else_part
%type <n> return
%type <n> sscanf
%type <n> statement
%type <n> statements
%type <n> string
%type <n> switch
%type <n> typeof
%type <n> unused
%type <n> unused2
%type <n> while
%%

all: program;

program: program def optional_semi_colon
  |  /* empty */
  ;

optional_semi_colon: /* empty */
  | ';' 
  ;

string_constant: low_string
  | string_constant '+' low_string
  {
    $$=add_shared_strings($1,$3);
    free_string($1);
    free_string($3);
  }
  ;

optional_rename_inherit: ':' F_IDENTIFIER { $$=$2; }
  | { $$=0; }
  ;

program_ref: string_constant
  {
    reference_shared_string($1);
    push_string($1);
    push_string($1);
    reference_shared_string(current_file);
    push_string(current_file);
    SAFE_APPLY_MASTER("handle_inherit", 2);

    if(sp[-1].type != T_PROGRAM)
      my_yyerror("Couldn't cast string \"%s\" to program",$1->str);
  }
  | idents
  {
    if(last_identifier)
    {
      push_string(last_identifier);
      last_identifier->refs++;
    }else{
      push_text("");
    }
    
    resolv_constant($1);
    if(sp[-1].type == T_OBJECT)
    {
      struct program *p=sp[-1].u.object->prog;
      if(!p)
      {
	pop_stack();
	push_int(0);
      }else{
	p->refs++;
	pop_stack();
	push_program(p);
      }
    }
    if(sp[-1].type != T_PROGRAM)
    {
      yyerror("Illegal program identifier");
      pop_stack();
      push_int(0);
    }
    free_node($1);
  }
  ;
          
inheritance: modifiers F_INHERIT program_ref optional_rename_inherit ';'
  {
    struct pike_string *s;
    if(sp[-1].type == T_PROGRAM)
    {
      s=sp[-2].u.string;
      if($4) s=$4;
      do_inherit(sp[-1].u.program,$1,s);
      if($4) free_string($4);
    }
    pop_n_elems(2);
  }
  ;

import: modifiers F_IMPORT idents ';'
  {
    resolv_constant($3);
    free_node($3);
    use_module(sp-1);
    pop_stack();
  }
  ;

constant_name: F_IDENTIFIER '=' expr0
  {
    int tmp;
    node *n;
    /* This can be made more lenient in the future */
    n=mknode(F_ARG_LIST,$3,0); /* Make sure it is optimized */
    if(!is_const(n))
    {
      struct svalue tmp;
      yyerror("Constant definition is not constant.");
      tmp.type=T_INT;
      tmp.u.integer=0;
      add_constant($1,&tmp, current_modifiers);
    } else {
      tmp=eval_low(n);
      free_node(n);
      if(tmp < 1)
      {
	yyerror("Error in constant definition.");
      }else{
	pop_n_elems(tmp-1);
	add_constant($1,sp-1,current_modifiers);
	pop_stack();
      }
      free_string($1);
    }
  }
  ;

constant_list: constant_name
  | constant_list ',' constant_name
  ;

constant: modifiers F_CONSTANT constant_list ';' {}
  ;

block_or_semi: block
  {
    $$ = mknode(F_ARG_LIST,$1,mknode(F_RETURN,mkintnode(0),0));
  }
  | ';' { $$ = NULL;}
  ;


type_or_error: simple_type
  {
    if(local_variables->current_type)
      free_string(local_variables->current_type); 
    local_variables->current_type=$1;
  }
  | /* empty */
  {
    yyerror("Missing type.");
    copy_shared_string(local_variables->current_type,
		       mixed_type_string);
  }
  

def: modifiers type_or_error optional_stars F_IDENTIFIER '(' arguments ')'
  {
    int e;
    /* construct the function type */
    push_finished_type(local_variables->current_type);
    while(--$3>=0) push_type(T_ARRAY);
    
    if(local_variables->current_return_type)
      free_string(local_variables->current_return_type);
    local_variables->current_return_type=pop_type();
    
    push_finished_type(local_variables->current_return_type);
    
    e=$6-1;
    if(varargs)
    {
      push_finished_type(local_variables->variable[e].type);
      e--;
      varargs=0;
      pop_type_stack();
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--)
    {
      push_finished_type(local_variables->variable[e].type);
      if($1 & ID_VARARGS)
      {
	push_type(T_VOID);
	push_type(T_OR);
      }
    }
    push_type(T_FUNCTION);
    
    $<string>$=pop_type();
    define_function($4,
		    $<string>$,
		    $1,
		    IDENTIFIER_PIKE_FUNCTION,
		    0);
  }
  block_or_semi
  {
    int e;
    if($9)
    {
      for(e=0; e<$6; e++)
      {
	if(!local_variables->variable[e].name ||
	   !local_variables->variable[e].name->len)
	{
	  my_yyerror("Missing name for argument %d",e);
	}
      }

      dooptcode($4, $9, $<string>8, $1);
#ifdef DEBUG
      if(recoveries && sp-evaluator_stack < recoveries->sp)
	fatal("Stack error (underflow)\n");
#endif
    }
    if(local_variables->current_return_type)
    {
      free_string(local_variables->current_return_type);
      local_variables->current_return_type=0;
    }
    free_all_local_names();
    free_string($4);
    free_string($<string>8);
  }
  | modifiers type_or_error name_list ';' {}
  | inheritance {}
  | import {}
  | constant {}
  | class { free_node($1); }
  | error ';' { yyerrok; }
  {
    reset_type_stack();
/*    if(num_parse_error>5) YYACCEPT; */
  }
  ;


optional_dot_dot_dot: F_DOT_DOT_DOT { $$=1; }
  | /* empty */ { $$=0; }
  ;

optional_identifier: F_IDENTIFIER
  | /* empty */ { $$=0; }
  ;


new_arg_name: type optional_dot_dot_dot optional_identifier
  {
    if(varargs) yyerror("Can't define more arguments after ...");

    if($2)
    {
      push_type(T_ARRAY);
      varargs=1;
    }
    if(!$3) $3=make_shared_string("");
    if(islocal($3) >= 0)
      my_yyerror("Variable '%s' appears twice in argument list.",
		 $3->str);
    
    add_local_name($3, pop_type());
  }
  ;

func_args: '(' arguments ')' { $$=$2; }
         | '(' error ')' { $$=0; yyerrok; }
         | error { $$=0; yyerrok; }
         ;

arguments: /* empty */ optional_comma { $$=0; }
  | arguments2 optional_comma { $$=$1; }
  ;

arguments2: new_arg_name { $$ = 1; }
  | arguments2 ',' new_arg_name { $$ = $1 + 1; }
  ;

modifier: F_NO_MASK    { $$ = ID_NOMASK; }
  | F_STATIC     { $$ = ID_STATIC; }
  | F_PRIVATE    { $$ = ID_PRIVATE; }
  | F_PUBLIC     { $$ = ID_PUBLIC; }
  | F_VARARGS    { $$ = ID_VARARGS; }
  | F_PROTECTED  { $$ = ID_PROTECTED; }
  | F_INLINE     { $$ = ID_INLINE | ID_NOMASK; }
  ;

modifiers: modifier_list { $$=current_modifiers=$1; } ;

modifier_list: /* empty */ { $$ = 0; }
  | modifier modifier_list { $$ = $1 | $2; }
  ;

optional_stars: optional_stars '*' { $$=$1 + 1; }
  | /* empty */ { $$=0; }
  ;

cast: '(' type ')' { $$=pop_type(); }
    ;
  
type: type '*' { push_type(T_ARRAY); }
  | type2
  ;

simple_type: type2 { $$=pop_type(); }
           ;

type2: type2 '|' type3 { push_type(T_OR); }
  | type3
  ;

type3: F_INT_ID      { push_type(T_INT); }
  | F_FLOAT_ID    { push_type(T_FLOAT); }
  | F_STRING_ID   { push_type(T_STRING); }
  | F_PROGRAM_ID  { push_type(T_PROGRAM); }
  | F_VOID_ID     { push_type(T_VOID); }
  | F_MIXED_ID    { push_type(T_MIXED); }
  | F_OBJECT_ID   opt_object_type { push_type(T_OBJECT); }
  | F_MAPPING_ID opt_mapping_type { push_type(T_MAPPING); }
  | F_ARRAY_ID opt_array_type { push_type(T_ARRAY); }
  | F_MULTISET_ID opt_array_type { push_type(T_MULTISET); }
  | F_FUNCTION_ID opt_function_type { push_type(T_FUNCTION); }
  ;

opt_object_type:  /* Empty */ { push_type_int(0); }
  | '(' program_ref ')'
  {
    if(sp[-1].type == T_PROGRAM)
    {
      push_type_int(sp[-1].u.program->id);
    }else{
      yyerror("Not a valid program specifier");
      push_type_int(0);
    }
    pop_n_elems(2);
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
      push_type(T_MANY);
      type_stack_reverse();
    }else{
      type_stack_reverse();
      push_type(T_MANY);
      push_type(T_VOID);
    }
    type_stack_mark();
  }
  type ')'
  {
    type_stack_reverse();
    type_stack_reverse();
  }
  | /* empty */
  {
   push_type(T_MIXED);
   push_type(T_MIXED);
   push_type(T_MANY);
  }
  ;

function_type_list: /* Empty */ optional_comma
  | function_type_list2 optional_comma
  ;

function_type_list2: type 
  | function_type_list2 ','
  {
    type_stack_reverse();
    type_stack_mark();
  }
  type
  ;

opt_array_type: '(' type ')'
  |  { push_type(T_MIXED); }
  ;

opt_mapping_type: '('
  { 
    type_stack_mark();
    type_stack_mark();
  }
  type ':'
  { 
    type_stack_reverse();
    type_stack_mark();
  }
  type
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
    push_finished_type(local_variables->current_type);
    while($1--) push_type(T_ARRAY);
    type=pop_type();
    define_variable($2, type, current_modifiers);
    free_string(type);
    free_string($2);
  }
  | optional_stars F_IDENTIFIER '='
  {
    struct pike_string *type;
    push_finished_type(local_variables->current_type);
    while($1--) push_type(T_ARRAY);
    type=pop_type();
    $<number>$=define_variable($2, type, current_modifiers);
    free_string(type);
  }
  expr0
  {
    init_node=mknode(F_ARG_LIST,init_node,
		     mkcastnode(void_type_string,
				mknode(F_ASSIGN,$5,
				       mkidentifiernode($<number>4))));
    free_string($2);
  }
  ;


new_local_name: optional_stars F_IDENTIFIER
  {
    push_finished_type(local_variables->current_type);
    while($1--) push_type(T_ARRAY);
    add_local_name($2, pop_type());
    $$=mknode(F_ASSIGN,mkintnode(0), mklocalnode(islocal($2)));
  }
  | optional_stars F_IDENTIFIER '=' expr0
  {
    push_finished_type(local_variables->current_type);
    while($1--) push_type(T_ARRAY);
    add_local_name($2, pop_type());
    $$=mknode(F_ASSIGN,$4,mklocalnode(islocal($2)));
  }
  ;


block:'{'
  {
    $<number>$=local_variables->current_number_of_locals;
  } 
  statements '}'
  {
    while(local_variables->current_number_of_locals > $<number>2)
    {
      int e;
      e=--(local_variables->current_number_of_locals);
      free_string(local_variables->variable[e].name);
      free_string(local_variables->variable[e].type);
    }
    $$=$3;
  }
  ;

failsafe_block: block
              | error { $$=0; }
              ;
  

local_name_list: new_local_name
  | local_name_list ',' new_local_name { $$=mknode(F_ARG_LIST,$1,$3); }
  ;

statements: { $$=0; }
  | statements statement
  {
    $$=mknode(F_ARG_LIST,$1,mkcastnode(void_type_string,$2));
  }
  ;

statement: unused2 ';' { $$=$1; }
  | cond
  | while
  | do
  | for
  | switch
  | case
  | default
  | return ';'
  | block {}
  | foreach
  | break ';'
  | continue ';'
  | error ';' { reset_type_stack(); $$=0; yyerrok; }
  | ';' { $$=0; } 
  ;


break: F_BREAK { $$=mknode(F_BREAK,0,0); } ;
default: F_DEFAULT ':'  { $$=mknode(F_DEFAULT,0,0); } ;
continue: F_CONTINUE { $$=mknode(F_CONTINUE,0,0); } ;

lambda: F_LAMBDA
  {
    push_locals();
    $<number>$=comp_stackp;
    
    if(local_variables->current_return_type)
      free_string(local_variables->current_return_type);
    copy_shared_string(local_variables->current_return_type,any_type_string);
  }
  func_args failsafe_block
  {
    struct pike_string *type;
    char buf[40];
    int f,e;
    struct pike_string *name;
    
    setup_fake_program();
    fix_comp_stack($<number>2);
    
    push_type(T_MIXED);
    
    e=$3-1;
    if(varargs)
    {
      push_finished_type(local_variables->variable[e].type);
      e--;
      varargs=0;
      pop_type_stack();
    }else{
      push_type(T_VOID);
    }
    push_type(T_MANY);
    for(; e>=0; e--)
      push_finished_type(local_variables->variable[e].type);
    
    push_type(T_FUNCTION);
    
    type=pop_type();
    
    sprintf(buf,"__lambda_%ld",
	    (long)fake_program.num_identifier_references);
    name=make_shared_string(buf);

    f=dooptcode(name,
	      mknode(F_ARG_LIST,$4,mknode(F_RETURN,mkintnode(0),0)),
	      type,
	      0);
#ifdef DEBUG
    if(recoveries && sp-evaluator_stack < recoveries->sp)
      fatal("Stack error (underflow)\n");
#endif
    free_string(name);
    free_string(type);
    comp_stackp=$<number>2;
    pop_locals();
    $$=mkidentifiernode(f);
  }
  ;

failsafe_program: '{' program '}'
                | error { yyerrok; }
                ;

class: modifiers F_CLASS optional_identifier
  {
    start_new_program();
    /* write(2, "start\n", 6); */
  }
  failsafe_program
  {
    struct svalue s;
    /* write(2, "end\n", 4); */
    s.u.program=end_program();
    if(!s.u.program)
    {
      yyerror("Class definition failed.");
      s.type=T_INT;
      s.subtype=0;
    } else {
      s.type=T_PROGRAM;
      s.subtype=0;
    }
    if($3)
    { 
      add_constant($3, &s, $1);
      free_string($3);
    }
    $$=mksvaluenode(&s);
    free_svalue(&s);
  }
  ;

cond: F_IF '(' comma_expr ')' statement optional_else_part
  {
    $$=mknode('?',$3,mknode(':',$5,$6));
    $$->line_number=$1;
    $$=mkcastnode(void_type_string,$$);
    $$->line_number=$1;
  }
  ;

optional_else_part: { $$=0; }
  | F_ELSE statement { $$=$2; }
  ;      

foreach: F_FOREACH '(' expr0 ',' lvalue ')' statement
  {
    $$=mknode(F_FOREACH,mknode(F_VAL_LVAL,$3,$5),$7);
    $$->line_number=$1;
  }
  ;

do: F_DO statement F_WHILE '(' comma_expr ')' ';'
  {
    $$=mknode(F_DO,$2,$5);
    $$->line_number=$1;
  }
  ;

for: F_FOR '(' unused  ';' for_expr ';' unused ')' statement
  {
    int i=current_line;
    current_line=$1;
    $$=mknode(F_ARG_LIST,mkcastnode(void_type_string,$3),mknode(F_FOR,$5,mknode(':',$9,$7)));
    current_line=i;
  }
  ;


while:  F_WHILE '(' comma_expr ')' statement
  {
    int i=current_line;
    current_line=$1;
    $$=mknode(F_FOR,$3,mknode(':',$5,NULL));
    current_line=i;
  }
  ;

for_expr: /* EMPTY */ { $$=mkintnode(1); }
  | comma_expr
  ;

switch:	F_SWITCH '(' comma_expr ')' statement
  {
    $$=mknode(F_SWITCH,$3,$5);
    $$->line_number=$1;
  }
  ;

case: F_CASE comma_expr ':'
  {
    $$=mknode(F_CASE,$2,0);
  }
  | F_CASE comma_expr F_DOT_DOT optional_comma_expr ':'
  {
    $$=mknode(F_CASE,$4?$2:0,$4?$4:$2);
  }
  ;

return: F_RETURN
  {
    if(!match_types(local_variables->current_return_type,
		    void_type_string))
    {
      yyerror("Must return a value for a non-void function.");
    }
    $$=mknode(F_RETURN,mkintnode(0),0);
  }
  | F_RETURN comma_expr
  {
    $$=mknode(F_RETURN,$2,0);
  }
  ;
	
unused: { $$=0; }
  | unused2
  ;

unused2: comma_expr { $$=mkcastnode(void_type_string,$1);  } ;

optional_comma_expr: { $$=0; }
  | comma_expr
  ;

comma_expr: comma_expr2
  | type2
  {
    if(local_variables->current_type)
      free_string(local_variables->current_type);
    local_variables->current_type=pop_type();
  } local_name_list { $$=$3; }
  ;
          

comma_expr2: expr0
  | comma_expr2 ',' expr0
  {
    $$ = mknode(F_ARG_LIST,mkcastnode(void_type_string,$1),$3); 
  }
  ;

expr00: expr0
      | '@' expr0 { $$=mknode(F_PUSH_ARRAY,$2,0); };

expr0: expr01
  | expr4 '=' expr0  { $$=mknode(F_ASSIGN,$3,$1); }
  | expr4 assign expr0  { $$=mknode($2,$1,$3); }
  | error assign expr01 { $$=0; reset_type_stack(); yyerrok; }
  ;

expr01: expr1 { $$ = $1; }
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

optional_comma: | ',' ;

expr_list: { $$=0; }
  | expr_list2 optional_comma { $$=$1; }
  ;
         

expr_list2: expr00
  | expr_list2 ',' expr00 { $$=mknode(F_ARG_LIST,$1,$3); }
  ;

m_expr_list: { $$=0; }
  | m_expr_list2 optional_comma { $$=$1; }
  ;

m_expr_list2: assoc_pair
  | m_expr_list2 ',' assoc_pair { $$=mknode(F_ARG_LIST,$1,$3); }
  ;

assoc_pair:  expr0 ':' expr1 { $$=mknode(F_ARG_LIST,$1,$3); } ;

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
  ;

expr2: expr3
  | cast expr2
  {
    $$=mkcastnode($1,$2);
    free_string($1);
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
  | F_NUMBER { $$=mkintnode($1); }
  | F_FLOAT { $$=mkfloatnode($1); }
  | catch
  | gauge
  | typeof
  | sscanf
  | lambda
  | class
  | idents
  | expr4 '(' expr_list ')' { $$=mkapplynode($1,$3); }
  | expr4 '[' expr0 ']' { $$=mknode(F_INDEX,$1,$3); }
  | expr4 '['  comma_expr_or_zero F_DOT_DOT comma_expr_or_maxint ']'
  {
    $$=mknode(F_RANGE,$1,mknode(F_ARG_LIST,$3,$5));
  }
  | '(' comma_expr2 ')' { $$=$2; }
  | '(' '{' expr_list '}' ')'
    { $$=mkefuncallnode("aggregate",$3); }
  | '(' '[' m_expr_list ']' ')'
    { $$=mkefuncallnode("aggregate_mapping",$3); };
  | F_MULTISET_START expr_list F_MULTISET_END
    { $$=mkefuncallnode("aggregate_multiset",$2); }
  | expr4 F_ARROW F_IDENTIFIER
  {
    $$=mknode(F_ARROW,$1,mkstrnode($3));
    free_string($3);
  }
  ;

idents: low_idents
  | idents '.' F_IDENTIFIER
  {
    $$=index_node($1, $3);
    free_node($1);
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $3);
    free_string($3);
  }
  ;

low_idents: F_IDENTIFIER
  {
    int i;
    struct efun *f;
    if(last_identifier) free_string(last_identifier);
    copy_shared_string(last_identifier, $1);
    if((i=islocal($1))>=0)
    {
      $$=mklocalnode(i);
    }else if((i=isidentifier($1))>=0){
      $$=mkidentifiernode(i);
    }else if(find_module_identifier($1)){
      $$=mkconstantsvaluenode(sp-1);
      pop_stack();
    }else{
      $$=0;
      if(!num_parse_error)
      {
	if( get_master() )
	{
	  reference_shared_string($1);
	  push_string($1);
	  reference_shared_string(current_file);
	  push_string(current_file);
	  SAFE_APPLY_MASTER("resolv", 2);
	  
	  if(throw_value.type == T_STRING)
	  {
	    my_yyerror("%s",throw_value.u.string->str);
	  }
	  else if(IS_ZERO(sp-1) && sp[-1].subtype==1)
	  {
	    my_yyerror("'%s' undefined.", $1->str);
	  }else{
	    $$=mkconstantsvaluenode(sp-1);
	  }
	  pop_stack();
	}else{
	  my_yyerror("'%s' undefined.", $1->str);
	}
      }
    }
    free_string($1);
  }
  | F_PREDEF F_COLON_COLON F_IDENTIFIER
  {
    struct svalue tmp;
    node *tmp2;
    tmp.type=T_MAPPING;
#ifdef __CHECKER__
    tmp.subtype=0;
#endif /* __CHECKER__ */
    tmp.u.mapping=get_builtin_constants();
    tmp2=mkconstantsvaluenode(&tmp);
    $$=index_node(tmp2, $3);
    free_node(tmp2);
    free_string($3);
  }
  | F_IDENTIFIER F_COLON_COLON F_IDENTIFIER
  {
    int f;
    struct reference *idp;

    setup_fake_program();
    f=reference_inherited_identifier($1,$3);
    idp=fake_program.identifier_references+f;
    if (f<0 || ID_FROM_PTR(&fake_program,idp)->func.offset == -1)
    {
      my_yyerror("Undefined identifier %s::%s", $1->str,$3->str);
      $$=mkintnode(0);
    } else {
      $$=mkidentifiernode(f);
    }

    free_string($1);
    free_string($3);
  }
  | F_COLON_COLON F_IDENTIFIER
  {
    int e,i;

    $$=0;
    setup_fake_program();
    for(e=1;e<(int)fake_program.num_inherits;e++)
    {
      if(fake_program.inherits[e].inherit_level!=1) continue;
      i=low_reference_inherited_identifier(e,$2);
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
      $$=mkintnode(0);
    }else{
      if($$->token==F_ARG_LIST) $$=mkefuncallnode("aggregate",$$);
    }
    free_string($2);
  }
  ;

comma_expr_or_zero: /* empty */ { $$=mkintnode(0); }
  | comma_expr
  ;

comma_expr_or_maxint: /* empty */ { $$=mkintnode(0x7fffffff); }
  | comma_expr
  ;

gauge: F_GAUGE catch_arg
  {
#ifdef HAVE_GETHRVTIME
    $$=mkopernode("`-",
		  mkopernode("`/", 
			     mkopernode("`-", mkefuncallnode("gethrvtime",0),
					mknode(F_ARG_LIST,$2,
					       mkefuncallnode("gethrvtime",0))),
			     mkintnode(1000)), 0);
#else
  $$=mkopernode("`-",
		  mkopernode("`-",
			     mknode(F_INDEX,mkefuncallnode("rusage",0),
				    mkintnode(GAUGE_RUSAGE_INDEX)),
			     mknode(F_ARG_LIST,$2,
				    mknode(F_INDEX,mkefuncallnode("rusage",0),
					   mkintnode(GAUGE_RUSAGE_INDEX)))),0);
#endif
  };

typeof: F_TYPEOF '(' expr0 ')'
  {
    node *tmp;
    tmp=mknode(F_ARG_LIST,$3,0);
    if($3 && $3->type)
    {
       $$=mkstrnode(describe_type($3->type));
    }else{
       $$=mkstrnode(describe_type(mixed_type_string));
    }
    free_node(tmp);
  } ;
 
catch_arg: '(' comma_expr ')'  { $$=$2; }
  | block
  ; 

catch: F_CATCH catch_arg { $$=mknode(F_CATCH,$2,NULL); } ;

sscanf: F_SSCANF '(' expr0 ',' expr0 lvalue_list ')'
  {
    $$=mknode(F_SSCANF,mknode(F_ARG_LIST,$3,$5),$6);
  }
  ;

lvalue: expr4
  | type F_IDENTIFIER
  {
    add_local_name($2,pop_type());
    $$=mklocalnode(islocal($2));
  }

lvalue_list: /* empty */ { $$ = 0; }
  | ',' lvalue lvalue_list { $$ = mknode(F_LVALUE_LIST,$2,$3); }
  ;

low_string: F_STRING 
  | low_string F_STRING
  {
    $$=add_shared_strings($1,$2);
    free_string($1);
    free_string($2);
  }
  ;

string: low_string { $$=mkstrnode($1); free_string($1); } ;


%%

void yyerror(char *str)
{
  extern int num_parse_error;
  extern int cumulative_parse_error;

#ifdef DEBUG
  if(recoveries && sp-evaluator_stack < recoveries->sp)
    fatal("Stack error (underflow)\n");
#endif

  if (num_parse_error > 5) return;
  num_parse_error++;
  cumulative_parse_error++;

  if ( get_master() )
  {
    sp->type = T_STRING;
    copy_shared_string(sp->u.string, current_file);
    sp++;
    sp->type = T_INT;
    sp->u.integer = current_line;
    sp++;
    sp->type = T_STRING;
    sp->u.string = make_shared_string(str);
    sp++;
    SAFE_APPLY_MASTER("compile_error",3);
    pop_stack();
#ifdef DEBUG
    if(recoveries && sp-evaluator_stack < recoveries->sp)
      fatal("Stack error (underflow)\n");
#endif
  }else{
    (void)fprintf(stderr, "%s:%ld: %s\n",
		  current_file->str,
		  (long)current_line,
		  str);
    fflush(stderr);
  }
}

/* argument must be a shared string (no need to free it) */
void add_local_name(struct pike_string *str,
		    struct pike_string *type)
{
  if (local_variables->current_number_of_locals == MAX_LOCAL)
  {
    yyerror("Too many local variables");
  }else {
    local_variables->variable[local_variables->current_number_of_locals].type = type;
    local_variables->variable[local_variables->current_number_of_locals].name = str;
    local_variables->current_number_of_locals++;
    if(local_variables->current_number_of_locals > 
       local_variables->max_number_of_locals)
    {
      local_variables->max_number_of_locals=
	local_variables->current_number_of_locals;
    }
  }
}

/* argument must be a shared string */
int islocal(struct pike_string *str)
{
  int e;
  for(e=local_variables->current_number_of_locals-1;e>=0;e--)
    if(local_variables->variable[e].name==str)
      return e;
  return -1;
}

void free_all_local_names(void)
{
  int e;

  for (e=0; e<local_variables->current_number_of_locals; e++)
  {
    if(local_variables->variable[e].name)
    {
      free_string(local_variables->variable[e].name);
      free_string(local_variables->variable[e].type);
    }
    local_variables->variable[e].name=0;
    local_variables->variable[e].type=0;
  }
  local_variables->current_number_of_locals = 0;
  local_variables->max_number_of_locals = 0;
}

