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
%token F_ADD_256 F_ADD_512 F_ADD_768 F_ADD_1024 F_ADD_256X
%token F_PREFIX_256 F_PREFIX_512 F_PREFIX_768 F_PREFIX_1024
%token F_PREFIX_CHARX256 F_PREFIX_WORDX256 F_PREFIX_24BITX256
%token F_POP_VALUE F_POP_N_ELEMS F_MARK F_MARK2
%token F_CALL_LFUN F_CALL_LFUN_AND_POP

%token F_BRANCH F_BRANCH_WHEN_ZERO F_BRANCH_WHEN_NON_ZERO
%token F_BRANCH_WHEN_LT F_BRANCH_WHEN_GT
%token F_BRANCH_WHEN_LE F_BRANCH_WHEN_GE
%token F_BRANCH_WHEN_EQ F_BRANCH_WHEN_NE
%token F_INC_LOOP F_DEC_LOOP
%token F_INC_NEQ_LOOP F_DEC_NEQ_LOOP

%token F_INDEX F_INDIRECT F_STRING_INDEX F_LOCAL_INDEX
%token F_POS_INT_INDEX F_NEG_INT_INDEX
%token F_LTOSVAL F_LTOSVAL2
%token F_PUSH_ARRAY 
%token F_RANGE F_COPY_VALUE

/*
 * Basic value pushing
 */
%token F_LFUN F_GLOBAL F_LOCAL
%token F_GLOBAL_LVALUE F_LOCAL_LVALUE
%token F_CLEAR_LOCAL
%token F_CONSTANT F_FLOAT F_STRING
%token F_NUMBER F_NEG_NUMBER F_CONST_1 F_CONST0 F_CONST1 F_BIGNUM
/*
 * These are the predefined functions that can be accessed from Pike.
 */

%token F_INC F_DEC F_POST_INC F_POST_DEC F_INC_AND_POP F_DEC_AND_POP
%token F_INC_LOCAL F_INC_LOCAL_AND_POP F_POST_INC_LOCAL
%token F_DEC_LOCAL F_DEC_LOCAL_AND_POP F_POST_DEC_LOCAL
%token F_RETURN F_DUMB_RETURN F_RETURN_0 F_THROW_ZERO

%token F_ASSIGN F_ASSIGN_AND_POP
%token F_ASSIGN_LOCAL F_ASSIGN_LOCAL_AND_POP
%token F_ASSIGN_GLOBAL F_ASSIGN_GLOBAL_AND_POP
%token F_ADD F_SUBTRACT
%token F_MULTIPLY F_DIVIDE F_MOD

%token F_LT F_GT F_EQ F_GE F_LE F_NE
%token F_NEGATE F_NOT F_COMPL
%token F_AND F_OR F_XOR
%token F_LSH F_RSH
%token F_LAND F_LOR

%token F_SWITCH F_SSCANF F_CATCH
%token F_CAST
%token F_FOREACH

%token F_SIZEOF F_SIZEOF_LOCAL

/*
 * These are token values that needn't have an associated code for the
 * compiled file
 */

%token F_MAX_OPCODE
%token F_ADD_EQ
%token F_AND_EQ
%token F_APPLY
%token F_ARG_LIST
%token F_ARRAY_ID
%token F_ARROW
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
%token F_MULT
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

%token F_ALIGN
%token F_POINTER
%token F_LABEL

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
%left '*' '%' '/' F_MULT
%right F_NOT '~'
%nonassoc F_INC F_DEC


%{
/* This is the grammar definition of Pike. */

#include "global.h"
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
#include "macros.h"
#include "error.h"
#include "docode.h"

#define YYMAXDEPTH	600

static void push_locals();
static void pop_locals();
void free_all_local_names();
void add_local_name(struct pike_string *,struct pike_string *);

/*
 * The names and types of arguments and auto variables.
 */
struct locals *local_variables;

static int varargs;
static INT32  current_modifiers;

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

%type <fnum> F_FLOAT
%type <number> modifiers modifier optional_dot_dot_dot
%type <number> assign F_NUMBER F_LOCAL arguments arguments2
%type <number> optional_stars modifier_list
%type <string> F_IDENTIFIER F_STRING string_constant low_string
%type <string> optional_identifier cast simple_type
%type <string> optional_rename_inherit

%type <number> F_ARRAY_ID F_BREAK F_CASE F_CATCH F_CONTINUE F_DEFAULT F_DO
%type <number> F_PREDEF F_ELSE F_FLOAT_ID F_FOR F_FOREACH F_FUNCTION_ID F_GAUGE
%type <number> F_IF F_INHERIT F_INLINE F_INT_ID F_LAMBDA F_MULTISET_ID F_MAPPING_ID
%type <number> F_MIXED_ID F_NO_MASK F_OBJECT_ID F_PRIVATE F_PROGRAM_ID
%type <number> F_PROTECTED F_PUBLIC F_RETURN F_SSCANF F_STATIC
%type <number> F_STRING_ID F_SWITCH F_VARARGS F_VOID_ID F_WHILE

/* The following symbos return type information */

%type <n> string expr01 expr00 comma_expr comma_expr_or_zero
%type <n> expr2 expr1 expr3 expr0 expr4 catch lvalue_list
%type <n> lambda for_expr block  assoc_pair new_local_name
%type <n> expr_list2 m_expr_list m_expr_list2 statement gauge sscanf
%type <n> for do cond optional_else_part while statements
%type <n> local_name_list class catch_arg comma_expr_or_maxint
%type <n> unused2 foreach unused switch case return expr_list default
%type <n> continue break block_or_semi typeof
%%

all: program;

program: program def optional_semi_colon
       |  /* empty */ ;

optional_semi_colon: /* empty */
                   | ';' { yyerror("Extra ';'. Ignored."); };

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
          
inheritance: modifiers F_INHERIT string_constant optional_rename_inherit ';'
	{
	  simple_do_inherit($3,$1,$4);
	};

block_or_semi: block { $$ = mknode(F_ARG_LIST,$1,mknode(F_RETURN,mkintnode(0),0)); }
             | ';' { $$ = NULL;}
             ;


type_or_error: simple_type
             {
                if(local_variables->current_type) free_string(local_variables->current_type); 
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
     while($3--) push_type(T_ARRAY);

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
       union idptr tmp;
       int args, vargs;
       for(e=0; e<$6; e++)
       {
	 if(!local_variables->variable[e].name ||
	    !local_variables->variable[e].name->len)
	 {
	   my_yyerror("Missing name for argument %d",e);
	 }
       }

       tmp.offset=PC;
       args=count_arguments($<string>8);
       if(args < 0) 
       {
	 args=~args;
	 vargs=IDENTIFIER_VARARGS;
       }else{
	 vargs=0;
       }
       ins_byte(local_variables->max_number_of_locals, A_PROGRAM);
       ins_byte(args, A_PROGRAM);
       dooptcode($4, $9, $6);

       define_function($4,
		       $<string>8,
		       $1,
		       IDENTIFIER_PIKE_FUNCTION | vargs,
		       &tmp);
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
   | error 
   {
     reset_type_stack();
     if(num_parse_error>5) YYACCEPT;
   } ;


optional_dot_dot_dot: F_DOT_DOT_DOT { $$=1; }
                    | /* empty */ { $$=0; }
                    ;

optional_identifier: F_IDENTIFIER
                   | /* empty */ { $$=make_shared_string(""); }
                   ;


new_arg_name: type optional_dot_dot_dot optional_identifier
            {
              if(varargs) yyerror("Can't define more arguments after ...");

	      if($2)
	      {
		push_type(T_ARRAY);
		varargs=1;
	      }
	      if(islocal($3) >= 0)
		my_yyerror("Variable '%s' appear twice in argument list.",
			   $3->str);

	      add_local_name($3, pop_type());
            };

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

modifiers: modifier_list { $$=current_modifiers=$1; }

modifier_list: /* empty */ { $$ = 0; }
	 | modifier modifier_list
         {
            $$ = $1 | $2;
         }
         ;

optional_stars: optional_stars '*' { $$=$1 + 1; }
              | /* empty */ { $$=0; }
              ;

cast: '(' type ')' { $$=pop_type(); } ;

type: type '*' { push_type(T_ARRAY); }
    | type2
    ;

simple_type: type2 { $$=pop_type(); }

type2: type2 '|' type3 { push_type(T_OR); }
     | type3
     ;

type3: F_INT_ID      { push_type(T_INT); }
     | F_FLOAT_ID    { push_type(T_FLOAT); }
     | F_STRING_ID   { push_type(T_STRING); }
     | F_OBJECT_ID   { push_type(T_OBJECT); }
     | F_PROGRAM_ID  { push_type(T_PROGRAM); }
     | F_VOID_ID     { push_type(T_VOID); }
     | F_MIXED_ID    { push_type(T_MIXED); }
     | F_MAPPING_ID opt_mapping_type { push_type(T_MAPPING); }
     | F_ARRAY_ID opt_array_type { push_type(T_ARRAY); }
     | F_MULTISET_ID opt_array_type { push_type(T_MULTISET); }
     | F_FUNCTION_ID opt_function_type { push_type(T_FUNCTION); }
     ;

opt_function_type: '('
                 {
                   type_stack_mark();
                   type_stack_mark();
		 }
 		 function_type_list
                 optional_dot_dot_dot
                 ':'
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
                 | {
                   push_type(T_MIXED);
                   push_type(T_MIXED);
                   push_type(T_MANY);
                 };

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
                | {
                    push_type(T_MIXED);
                    push_type(T_MIXED);
		  }
                ;



name_list: new_name
	 | name_list ',' new_name;

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
		mkcastnode(void_type_string, mknode(F_ASSIGN,$5,mkidentifiernode($<number>4))));
          free_string($2);
	} ;


new_local_name: optional_stars F_IDENTIFIER
                {
		  push_finished_type(local_variables->current_type);
		  while($1--) push_type(T_ARRAY);
                  add_local_name($2, pop_type());
                  $$=mkcastnode(void_type_string,
				mknode(F_ASSIGN,mkintnode(0),
				       mklocalnode(islocal($2))));
                }
              | optional_stars F_IDENTIFIER '=' expr0
                {
		  push_finished_type(local_variables->current_type);
		  while($1--) push_type(T_ARRAY);
                  add_local_name($2, pop_type());
                  $$=mkcastnode(void_type_string,
				mknode(F_ASSIGN,$4,
				       mklocalnode(islocal($2))));
                  
                };


block:'{'
     {
       $<number>$=local_variables->current_number_of_locals;
     } 
     statements
     '}'
     {
       while(local_variables->current_number_of_locals > $<number>2)
       {
	 int e;
	 e=--(local_variables->current_number_of_locals);
	 free_string(local_variables->variable[e].name);
	 free_string(local_variables->variable[e].type);
       }
       $$=$3;
     } ;

local_name_list: new_local_name
               | local_name_list ',' new_local_name
	       {
		 $$=mknode(F_ARG_LIST,$1,$3);
	       }
               ;

statements: { $$=0; }
          | statements statement
            {
              $$=mknode(F_ARG_LIST,$1,mkcastnode(void_type_string,$2));
            }
          ;

statement: unused2 ';' { $$=$1; }
         | simple_type
         {
	   if(local_variables->current_type) free_string(local_variables->current_type);
	   local_variables->current_type=$1;
	 } local_name_list ';' { $$=$3; }
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
         | error ';' { $$=0; }
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
       '(' arguments ')' block
        {
          struct pike_string *type;
	  char buf[40];
	  int f,e,args,vargs;
	  union idptr func;
	  struct pike_string *name;

	  setup_fake_program();
	  fix_comp_stack($<number>2);

	  push_type(T_MIXED);

	  e=$4-1;
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
	  func.offset=PC;

	  args=count_arguments(type);
	  if(args < 0) 
	  {
	    args=~args;
	    vargs=IDENTIFIER_VARARGS;
	  }else{
	    vargs=0;
	  }
	  ins_byte(local_variables->max_number_of_locals, A_PROGRAM);
	  ins_byte(args, A_PROGRAM);
	  
	  sprintf(buf,"__lambda_%ld",
		  (long)fake_program.num_identifier_references);
	  name=make_shared_string(buf);
          dooptcode(name,mknode(F_ARG_LIST,$6,mknode(F_RETURN,mkintnode(0),0)),$4);
	  f=define_function(name,
			    type,
			    0,
			    IDENTIFIER_PIKE_FUNCTION | vargs,
			    &func);
	  free_string(name);
	  free_string(type);
	  pop_locals();
          $$=mkidentifiernode(f);
	} ;

class: F_CLASS '{'
       {
         start_new_program();
       }
       program
       '}'
       {
         struct svalue s;
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
	 $$=mksvaluenode(&s);
         free_svalue(&s);
       } ;

cond: F_IF '(' comma_expr ')' 
      statement
      optional_else_part
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

foreach: F_FOREACH '(' expr0 ',' expr4 ')'
         statement
	 {
           $$=mknode(F_FOREACH,mknode(F_VAL_LVAL,$3,$5),$7);
	   $$->line_number=$1;
	 } ;

do: F_DO statement F_WHILE '(' comma_expr ')' ';'
    {
      $$=mknode(F_DO,$2,$5);
      $$->line_number=$1;
    } ;


for: F_FOR '(' unused  ';' for_expr ';' unused ')'
     statement
     {
       int i=current_line;
       current_line=$1;
       $$=mknode(F_ARG_LIST,mkcastnode(void_type_string,$3),mknode(F_FOR,$5,mknode(':',$9,$7)));
       current_line=i;
     } ;


while:  F_WHILE '(' comma_expr ')'
        statement
	{
	  int i=current_line;
	  current_line=$1;
	  $$=mknode(F_FOR,$3,mknode(':',$5,NULL));
	  current_line=i;
        } ;

for_expr: /* EMPTY */ { $$=mkintnode(1); }
        | comma_expr;

switch:	F_SWITCH '(' comma_expr ')'
        statement
        {
          $$=mknode(F_SWITCH,$3,$5);
	  $$->line_number=$1;
        } ;

case: F_CASE comma_expr ':'
    {
      $$=mknode(F_CASE,$2,0);
    }
    | F_CASE comma_expr F_DOT_DOT comma_expr ':'
    {
      $$=mknode(F_CASE,$2,$4);
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
	};
	
unused: { $$=0; }
      | unused2
      ;

unused2: comma_expr
       {
         $$=mkcastnode(void_type_string,$1);
       }
       ;

comma_expr: expr0
          | unused2 ',' expr0
          {
	    $$ = mknode(F_ARG_LIST,mkcastnode(void_type_string,$1),$3); 
	  }
          ;

expr00: expr0
      | '@' expr0
      {
	$$=mknode(F_PUSH_ARRAY,$2,0);
      };

expr0: expr01
     | expr4 '=' expr0
       {
         $$=mknode(F_ASSIGN,$3,$1);
       }
     | expr4 assign expr0
       {
	 $$=mknode($2,$1,$3);
       }
     | error assign expr01
       {
          $$=0;
       };

expr01: expr1 { $$ = $1; }
     | expr1 '?' expr01 ':' expr01
	{
          $$=mknode('?',$1,mknode(':',$3,$5));
	};

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

assoc_pair:  expr0 ':' expr1
             {
               $$=mknode(F_ARG_LIST,$1,$3);
             } ;

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
     | expr1 F_MULT expr1    { $$=mkopernode("`*",$1,$3); }
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
     | F_IDENTIFIER
     {
       int i;
       struct efun *f;
       if((i=islocal($1))>=0)
       {
         $$=mklocalnode(i);
       }else if((i=isidentifier($1))>=0){
         $$=mkidentifiernode(i);
       }else if((f=lookup_efun($1))){
	 $$=mkconstantsvaluenode(&f->function);
       }else{
	 my_yyerror("'%s' undefined.",$1->str);
         $$=0;
       }
       free_string($1);
     }
     | F_PREDEF F_COLON_COLON F_IDENTIFIER
     {
       struct efun *f;
       f=lookup_efun($3);
       if(!f)
       {
	 my_yyerror("Unknown efun: %s.",$3->str);
	 $$=mkintnode(0);
       }else{
	 $$=mksvaluenode(&f->function);
       }
       free_string($3);
     }
     | expr4 '(' expr_list ')' { $$=mkapplynode($1,$3); }
     | expr4 '[' expr0 ']' { $$=mknode(F_INDEX,$1,$3); }
     | expr4 '['  comma_expr_or_zero F_DOT_DOT comma_expr_or_maxint ']'
     {
       $$=mknode(F_RANGE,$1,mknode(F_ARG_LIST,$3,$5));
     }
     | '(' comma_expr ')' { $$=$2; }
     | '(' '{' expr_list '}' ')'
       { $$=mkefuncallnode("aggregate",$3); }
     | '(' '[' m_expr_list ']' ')'
       { $$=mkefuncallnode("aggregate_mapping",$3); };
     | F_MULTISET_START expr_list F_MULTISET_END
       { $$=mkefuncallnode("aggregate_multiset",$2); }
     | expr4 F_ARROW F_IDENTIFIER
     {
       $$=mknode(F_INDEX,$1,mkstrnode($3));
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
    $$=mkopernode("`-",
		  mkopernode("`-",
			     mknode(F_INDEX,mkefuncallnode("rusage",0),
				    mkintnode(GAUGE_RUSAGE_INDEX)),
			     mknode(F_ARG_LIST,$2,
				    mknode(F_INDEX,mkefuncallnode("rusage",0),
					   mkintnode(GAUGE_RUSAGE_INDEX)))),0);
  } ;

typeof: F_TYPEOF '(' expr0 ')'
      {
	node *tmp;
	tmp=mknode(F_ARG_LIST,$3,0);
        $$=mkstrnode(describe_type($3->type));
        free_node(tmp);
      } ;

catch_arg: '(' comma_expr ')'  { $$=$2; }
         | block
         ; 

catch: F_CATCH catch_arg
     {
       $$=mknode(F_CATCH,$2,NULL);
     } ;

sscanf: F_SSCANF '(' expr0 ',' expr0 lvalue_list ')'
      {
        $$=mknode(F_SSCANF,mknode(F_ARG_LIST,$3,$5),$6);
      }


lvalue_list: /* empty */ { $$ = 0; }
	   | ',' expr4 lvalue_list
           {
             $$ = mknode(F_LVALUE_LIST,$2,$3);
           } ;

low_string: F_STRING 
          | low_string F_STRING
          {
            $$=add_shared_strings($1,$2);
            free_string($1);
            free_string($2);
          }
          ;

string: low_string
      {
	$$=mkstrnode($1);
	free_string($1);
      } ;


%%

void yyerror(char *str)
{
  extern int num_parse_error;

  if (num_parse_error > 5) return;
  num_parse_error++;

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

void free_all_local_names()
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

static void push_locals()
{
  struct locals *l;
  l=ALLOC_STRUCT(locals);
  l->current_type=0;
  l->current_return_type=0;
  l->next=local_variables;
  l->current_number_of_locals=0;
  l->max_number_of_locals=0;
  local_variables=l;
}

static void pop_locals()
{
  struct locals *l;
  free_all_local_names();
  l=local_variables->next;
  if(local_variables->current_type)
    free_string(local_variables->current_type);
  if(local_variables->current_return_type)
    free_string(local_variables->current_return_type);
  free((char *)local_variables);

  local_variables=l;
  /* insert check if ( local->next == parent locals ) here */
}
