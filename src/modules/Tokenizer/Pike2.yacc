/* -*- C -*- */

/* Pike programming language grammar definition
 *
 * $Id$
 *
 * Henrik Grubbström 2002-05-17
 */

/* Grammar changes compared with Pike 7.3:
 *
 * o import directives can now take modifiers (ignored).
 * o import directives can now rename modules.
 * o The function many type declaration is now checked
 *   by the grammar, and not by the semantic rules.
 *
 */

/*
 * Declaration section
 */

/* Avoid global variables. */
%pure_parser

/* Include some files */
%{
#include "global.h"

#include "config.h"

#include "interpret.h"
#include "mapping.h"
%}

/* Parse tree node type. */
%union{
  struct svalue *sval;
}

/* NOTE: TOK_SEMICOLON *MUST* be the first of the tokens. */

/* Syntactic elements */
%token TOK_SEMICOLON
%type <sval> TOK_SEMICOLON
%token TOK_COMMA
%type <sval> TOK_COMMA
%token TOK_DOT
%type <sval> TOK_DOT
%token TOK_LPAREN
%type <sval> TOK_LPAREN
%token TOK_RPAREN
%type <sval> TOK_RPAREN
%token TOK_LBRACKET
%type <sval> TOK_LBRACKET
%token TOK_RBRACKET
%type <sval> TOK_RBRACKET
%token TOK_LBRACE
%type <sval> TOK_LBRACE
%token TOK_RBRACE
%type <sval> TOK_RBRACE

/* Needed to resolv LR(n), n>1 problems */
%token TOK_MULTISET_END		/* '>' ')' */
%type <sval> TOK_MULTISET_END		/* '>' ')' */
%token TOK_TYPE_SPECIFIER	/* '(' after literal type */
%type <sval> TOK_TYPE_SPECIFIER	/* '(' after literal type */

/* Literals */
%token TOK_FLOAT
%type <sval> TOK_FLOAT
%token TOK_STRING
%type <sval> TOK_STRING
%token TOK_NUMBER
%type <sval> TOK_NUMBER

/* Operators:Mutable */
%token TOK_ASSIGN
%type <sval> TOK_ASSIGN
%token TOK_INC
%type <sval> TOK_INC
%token TOK_DEC
%type <sval> TOK_DEC
%token TOK_ADD_EQ
%type <sval> TOK_ADD_EQ
%token TOK_AND_EQ
%type <sval> TOK_AND_EQ
%token TOK_DIV_EQ
%type <sval> TOK_DIV_EQ
%token TOK_LSH_EQ
%type <sval> TOK_LSH_EQ
%token TOK_MOD_EQ
%type <sval> TOK_MOD_EQ
%token TOK_MULT_EQ
%type <sval> TOK_MULT_EQ
%token TOK_OR_EQ
%type <sval> TOK_OR_EQ
%token TOK_RSH_EQ
%type <sval> TOK_RSH_EQ
%token TOK_SUB_EQ
%type <sval> TOK_SUB_EQ
%token TOK_XOR_EQ
%type <sval> TOK_XOR_EQ

/* Operators:Boolean */
%token TOK_LT
%type <sval> TOK_LT
%token TOK_GT
%type <sval> TOK_GT
%token TOK_EQ
%type <sval> TOK_EQ
%token TOK_GE
%type <sval> TOK_GE
%token TOK_LE
%type <sval> TOK_LE
%token TOK_NE
%type <sval> TOK_NE
%token TOK_NOT
%type <sval> TOK_NOT
%token TOK_LAND
%type <sval> TOK_LAND
%token TOK_LOR
%type <sval> TOK_LOR

/* Operators:Other */
%token TOK_COND
%type <sval> TOK_COND
%token TOK_COLON
%type <sval> TOK_COLON
%token TOK_OR
%type <sval> TOK_OR
%token TOK_AND
%type <sval> TOK_AND
%token TOK_XOR
%type <sval> TOK_XOR
%token TOK_PLUS
%type <sval> TOK_PLUS
%token TOK_MINUS
%type <sval> TOK_MINUS
%token TOK_STAR
%type <sval> TOK_STAR
%token TOK_DIV
%type <sval> TOK_DIV
%token TOK_MOD
%type <sval> TOK_MOD
%token TOK_INVERT
%type <sval> TOK_INVERT
%token TOK_LSH
%type <sval> TOK_LSH
%token TOK_RSH
%type <sval> TOK_RSH
%token TOK_ARROW
%type <sval> TOK_ARROW
%token TOK_SPLICE
%type <sval> TOK_SPLICE
%token TOK_COLON_COLON
%type <sval> TOK_COLON_COLON
%token TOK_DOT_DOT
%type <sval> TOK_DOT_DOT
%token TOK_DOT_DOT_DOT
%type <sval> TOK_DOT_DOT_DOT

/* Keywords */

/* Keywords:Modifiers */
%token TOK_MODIFIER
%type <sval> TOK_MODIFIER

/* Keywords:Type names */
%token TOK_ARRAY_ID
%type <sval> TOK_ARRAY_ID
%token TOK_FLOAT_ID
%type <sval> TOK_FLOAT_ID
%token TOK_FUNCTION_ID
%type <sval> TOK_FUNCTION_ID
%token TOK_INT_ID
%type <sval> TOK_INT_ID
%token TOK_MAPPING_ID
%type <sval> TOK_MAPPING_ID
%token TOK_MIXED_ID
%type <sval> TOK_MIXED_ID
%token TOK_MULTISET_ID
%type <sval> TOK_MULTISET_ID
%token TOK_OBJECT_ID
%type <sval> TOK_OBJECT_ID
%token TOK_PROGRAM_ID
%type <sval> TOK_PROGRAM_ID
%token TOK_STRING_ID
%type <sval> TOK_STRING_ID
%token TOK_VOID_ID
%type <sval> TOK_VOID_ID

%token TOK_CLASS
%type <sval> TOK_CLASS
%token TOK_CONSTANT
%type <sval> TOK_CONSTANT
%token TOK_ENUM
%type <sval> TOK_ENUM
%token TOK_NAMESPACE
%type <sval> TOK_NAMESPACE
%token TOK_TYPEDEF
%type <sval> TOK_TYPEDEF

%token TOK_GLOBAL
%type <sval> TOK_GLOBAL
%token TOK_PREDEF
%type <sval> TOK_PREDEF

/* Keywords:Special forms */
%token TOK_BREAK
%type <sval> TOK_BREAK
%token TOK_CASE
%type <sval> TOK_CASE
%token TOK_CATCH
%type <sval> TOK_CATCH
%token TOK_CONTINUE 
%type <sval> TOK_CONTINUE 
%token TOK_DEFAULT
%type <sval> TOK_DEFAULT
%token TOK_DO
%type <sval> TOK_DO
%token TOK_ELSE
%type <sval> TOK_ELSE
%token TOK_FOR
%type <sval> TOK_FOR
%token TOK_FOREACH
%type <sval> TOK_FOREACH
%token TOK_GAUGE
%type <sval> TOK_GAUGE
%token TOK_IF
%type <sval> TOK_IF
%token TOK_LAMBDA
%type <sval> TOK_LAMBDA
%token TOK_RETURN
%type <sval> TOK_RETURN
%token TOK_SSCANF
%type <sval> TOK_SSCANF
%token TOK_SWITCH
%type <sval> TOK_SWITCH
%token TOK_TYPEOF
%type <sval> TOK_TYPEOF
%token TOK_WHILE
%type <sval> TOK_WHILE

/* Generic identifier */
/* NOTE: TOK_IDENTIFIER *MUST* be the last of the tokens! */
%token TOK_IDENTIFIER
%type <sval> TOK_IDENTIFIER

/* Assocciativity */

%right TOK_ASSIGN
%right TOK_COND
%left TOK_LOR
%left TOK_LAND
%left TOK_OR
%left TOK_XOR
%left TOK_AND
%left TOK_EQ TOK_NE
%left TOK_GT TOK_GE TOK_LT TOK_LE  /* nonassoc? */
%left TOK_LSH TOK_RSH
%left TOK_PLUS TOK_MINUS
%left TOK_STAR TOK_MOD TOK_DIV
%right TOK_NOT TOK_INVERT
%nonassoc TOK_INC TOK_DEC

/* Node types. */
%type <sval> all
%type <sval> program
%type <sval> def
%type <sval> symbol_lookup_directive
%type <sval> program_ref
%type <sval> optional_rename
%type <sval> constant
%type <sval> constant_list
%type <sval> constant_name
%type <sval> class
%type <sval> optional_create_arguments
%type <sval> create_arguments
%type <sval> create_arguments2
%type <sval> create_arg
%type <sval> program_block
%type <sval> enum
%type <sval> enum_list
%type <sval> enum_def
%type <sval> typedef
%type <sval> func_def
%type <sval> func_args
%type <sval> arguments
%type <sval> optional_base_argument_list
%type <sval> base_argument_list
%type <sval> new_arg_name
%type <sval> new_many_arg_name
%type <sval> many_indicator
%type <sval> block_or_semi
%type <sval> var_def
%type <sval> name_list
%type <sval> name_list2
%type <sval> new_name
%type <sval> optional_initialization
%type <sval> initialization
%type <sval> modifiers
%type <sval> modifier_list
%type <sval> block
%type <sval> statements
%type <sval> statement
%type <sval> normal_label_statement
%type <sval> statement_with_semicolon
%type <sval> label
%type <sval> cond
/* %type <sval> end_cond */
%type <sval> optional_else_part
%type <sval> while
%type <sval> do
%type <sval> for
%type <sval> foreach
%type <sval> foreach_lvalues
%type <sval> optional_lvalue
%type <sval> lvalue
%type <sval> low_lvalue_list
%type <sval> lvalue_list
%type <sval> switch
%type <sval> case
%type <sval> default
%type <sval> return
%type <sval> break
%type <sval> continue
%type <sval> local_constant
%type <sval> comma_expr
%type <sval> comma_expr2
%type <sval> splice_expr
%type <sval> expr0
%type <sval> assign
%type <sval> expr01
%type <sval> expr1
%type <sval> expr2
%type <sval> expr3
%type <sval> expr4
%type <sval> lvalue_expr
%type <sval> expr_list
%type <sval> expr_list2
%type <sval> m_expr_list
%type <sval> m_expr_list2
%type <sval> assoc_pair
%type <sval> catch
%type <sval> gauge
%type <sval> typeof
%type <sval> sscanf
%type <sval> catch_arg
%type <sval> lambda
%type <sval> local_def
%type <sval> full_type
%type <sval> type4
%type <sval> type8
%type <sval> basic_type
%type <sval> opt_int_range
%type <sval> opt_mapping_type
%type <sval> opt_function_type
%type <sval> function_type_list
%type <sval> function_type_list2
%type <sval> opt_object_type
%type <sval> opt_array_type
%type <sval> idents
%type <sval> inherit_specifier
%type <sval> inherit_specifier_path
%type <sval> simple_identifier
%type <sval> string_constant
/* %type <sval> close_paren_or_missing */
/* %type <sval> close_brace_or_missing */
/* %type <sval> close_bracket_or_missing */
%type <sval> optional_identifier
%type <sval> optional_dot_dot_dot
%type <sval> optional_stars
%type <sval> magic_identifiers1
%type <sval> magic_identifiers2
%type <sval> magic_identifiers3
%type <sval> magic_identifiers
%type <sval> magic_identifier
%type <sval> number_or_maxint
%type <sval> number_or_minint
/* %type <sval> expected_dot_dot */
/* %type <sval> expected_colon */
%type <sval> optional_comma_expr
/* %type <sval> optional_comma */
%type <sval> optional_block
%type <sval> comma_expr_or_zero
%type <sval> comma_expr_or_maxint
%type <sval> string
%type <sval> bad_identifier
%type <sval> bad_expr_ident

%{
  /*
   * Magic Bison stuff, and renaming of yyparse()/yylex().
   */

#define yyparse		tokenizer_yyparse
#define YYPARSE_PARAM	context

#define yylex		tokenizer_yylex
#define YYLEX_PARAM	context

%}

%{
  /*
   * Lexer
   */

  static struct mapping *token_to_int = NULL;

  /* List of symbolic names returned by the tokenizer function.
   * NOTE: Must be in the same order as the %token directives above!
   */
  static const char *token_names[] = {
    /* Syntactic elements */
    ";",
    ",",
    ".",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",

    /* Needed to resolv LR(n), n>1 problems */
    ">)",
    "type_specifier",	/* '(' after literal type */

    /* Literals */
    "float_constant",
    "string_constant",
    "int_constant",

    /* Operators:Mutable */
    "=",
    "++",
    "--",
    "+=",
    "&=",
    "/=",
    "<<=",
    "%=",
    "*=",
    "|=",
    ">>=",
    "-=",
    "^=",

    /* Operators:Boolean */
    "<",
    ">",
    "==",
    ">=",
    "<=",
    "!=",
    "!",
    "&&",
    "||",

    /* Operators:Other */
    "?",
    ":",
    "|",
    "&",
    "^",
    "+",
    "-",
    "*",
    "/",
    "%",
    "~",
    "<<",
    ">>",
    "->",
    "@",
    "::",
    "..",
    "...",

    /* Keywords */

    /* Keywords:Modifiers */
    "modifier",

    /* Keywords:Type names */
    "array",
    "float",
    "function",
    "int",
    "mapping",
    "mixed",
    "multiset",
    "object",
    "program",
    "string",
    "void",

    "class",
    "constant",
    "enum",
    "namespace",
    "typedef",

    "global",
    "predef",

    /* Keywords:Special forms */
    "break",
    "case",
    "catch",
    "continue", 
    "default",
    "do",
    "else",
    "for",
    "foreach",
    "gauge",
    "if",
    "lambda",
    "return",
    "sscanf",
    "switch",
    "typeof",
    "while",

    /* Generic identifier */
    "identifier",
  };

  static int yylex(YYSTYPE *lval, struct compiler_context *context)
  {
    ref_push_mapping(token_to_int);
    apply_low(context->lexer, context->lex_value, 0);
    lval->sval = new_sval(context, Pike_sp-1);
    if (Pike_sp[-1].type == PIKE_T_INT) {
      Pike_sp--;
      return Pike_sp[0].u.integer;
    }
    push_constant_text("token");
    f_index(2);

    fprintf(stderr, "YYLEX: ");
    print_svalue(stderr, Pike_sp-1);
    fprintf(stderr, "\n");

    f_index(2);

#ifdef PIKE_DEBUG
    if (Pike_sp[-1].type != PIKE_T_INT) {
      fprintf(stderr, "Unexepected entry in token_to_int mapping:\n");
      print_svalue(stderr, Pike_sp-1);
      fprintf(stderr, "\n");
      fatal("Unexepected entry in token_to_int mapping.\n");
    }
    /* FIXME: The following is temporary debug code. */
    if ((!Pike_sp[-1].u.integer) && (Pike_sp[-1].subtype)) {
      fprintf(stderr, "WARNING: Token ");
      print_svalue(stderr, lval->sval);
      fprintf(stderr, " not found in token_to_int mapping.\n");
    }
#endif /* PIKE_DEBUG */

    /* Advance to the next token. */
    push_int(1);
    apply_low(context->lexer, context->lex_next, 1);
    pop_stack();

    fprintf(stderr, "YYLEX: %d\n", Pike_sp[-1].u.integer);
    
    Pike_sp--;
    return Pike_sp[0].u.integer;
  }

%}

%%

/*
 * Grammar section
 */

all: program { YYACCEPT; }
  ;

program: /* empty */
  { $$ = NULL; }
  | program def
  { $$ = LIST($1, $2); }
  | program TOK_SEMICOLON
  { $$ = $1; free_sval(context, $2); }
  ;

string_constant: string
  | string_constant TOK_PLUS string
  { $$ = BIN_OP($2, $1, $3); }
  ;

optional_rename_inherit: /* empty */ { $$ = NULL; }
  | TOK_COLON TOK_IDENTIFIER { $$ = $2; free_sval(context, $1); }
  | TOK_COLON bad_identifier { $$=NULL; free_sval(context, $1); }
  | TOK_COLON error { $$=NULL; free_sval(context, $1); }
  ;

force_resolve: /* empty */
  ;

low_program_ref: string_constant
  | idents
  ;

program_ref: low_program_ref
  ;
      
inheritance: modifiers TOK_INHERIT 
  low_program_ref optional_rename_inherit TOK_SEMICOLON
  {
    $$ = META($2, $1, RENAME($3, $4)); free_sval(context, $5);
  }
  | modifiers TOK_INHERIT low_program_ref error TOK_SEMICOLON
  {
    $$ = META($2, $1, RENAME($3, NULL)); free_sval(context, $5);
  }
  | modifiers TOK_INHERIT low_program_ref error TOK_RBRACE
  {
    yyerror("Missing ';'.");

    $$ = META($2, $1, RENAME($3, NULL)); free_sval(context, $5);    
  }
  | modifiers TOK_INHERIT error ';'
  {
    free_sval(context, $1);
    free_sval(context, $2);
    yyerrok;
  }
  | modifiers TOK_INHERIT error TOK_RBRACE
  {
    free_sval(context, $1);
    free_sval(context, $2);
    yyerror("Missing ';'.");
  }
  ;

import: TOK_IMPORT idents TOK_SEMICOLON
  {
    $$ = META($1, NULL, RENAME($2, NULL)); free_sval(context, $3);
  }
  | TOK_IMPORT string TOK_SEMICOLON
  {
    $$ = META($1, NULL, RENAME($2, NULL)); free_sval(context, $3);
  }
  | TOK_IMPORT error TOK_SEMICOLON
  {
    $$ = NULL;
    free_sval(context, $1);
    free_sval(context, $3);
    yyerrok;
  }
  | TOK_IMPORT error '}'
  {
    yyerror("Missing ';'.");
    $$ = NULL;
    free_sval(context, $1);
    free_sval(context, $3);
  }
  ;

constant_name: TOK_IDENTIFIER TOK_ASSIGN safe_expr0
  {
    $$ = INITIALIZER($1, $3);
    free_sval(context, $2);
  }
  | bad_identifier TOK_ASSIGN safe_expr0
  {
    free_sval(context, $2);
    free_sval(context, $3);
  }
  | error TOK_ASSIGN safe_expr0
  {
    free_sval(context, $2);
    free_sval(context, $3);
  }
  ;

constant_list: constant_name
  | constant_list ',' constant_name
  { $$ = LIST($1, $3); free_sval(context, $2); }
  ;

constant: modifiers TOK_CONSTANT constant_list TOK_SEMICOLON
  {
    $$ = META($2, $1, $3);
    free_sval(context, $4);
  }
  | modifiers TOK_CONSTANT error TOK_SEMICOLON
  {
    free_sval(context, $1);
    free_sval(context, $2);
    free_sval(context, $4);
    yyerrok;
    $$ = NULL;
  }
  | modifiers TOK_CONSTANT error TOK_RBRACE
  {
    yyerror("Missing ';'.");
    free_sval(context, $1);
    free_sval(context, $2);
    free_sval(context, $4);
    $$ = NULL;
  }
  ;

block_or_semi: block
  | TOK_SEMICOLON
  {
    $$ = NULL;
    free_sval(context, $1);
  }
  | error
  {
    $$ = NULL;
  }
  ;


type_or_error: simple_type
  ;

close_paren_or_missing: ')'
  | /* empty */
  {
    yyerror("Missing ')'.");
  }
  ;

close_brace_or_missing: '}'
  | /* empty */
  {
    yyerror("Missing '}'.");
  }
  ;

close_brace_or_eof: '}'
  ;

close_bracket_or_missing: ']'
  | /* empty */
  {
    yyerror("Missing ']'.");
  }
  ;

push_compiler_frame0: /* empty */
  ;

def: modifiers type_or_error optional_stars TOK_IDENTIFIER
     TOK_LPAREN arguments close_paren_or_missing block_or_semi
  {
    $$ = FUNC_DEF($1, $2, $3, $4, $6, $8);
    free_sval(context, $5);
  }
  | modifiers type_or_error optional_stars TOK_IDENTIFIER error
  {
    $$ = NULL;
  }
  | modifiers type_or_error optional_stars bad_identifier
    TOK_LPAREN arguments close_paren_or_missing block_or_semi
  {
    $$ = NULL;
  }
  | modifiers type_or_error name_list TOK_SEMICOLON
  {
    $$ = VAR_DEF($1, $2, $3); free_sval(context, $4);
  }
  | inheritance
  | import
  | constant
  | class
  | enum
  | typedef
  | error TOK_SEMICOLON
  {
    yyerrok;
    $$ = NULL;
    free_sval(context, $2);
  }
  | error TOK_RBRACE
  {
    yyerror("Missing ';'.");
    $$ = NULL;
    free_sval(context, $2);
  }
  | modifiers TOK_LBRACE program close_brace_or_eof
  {
    $$ = META(NULL, $1, $3);
    free_sval(context, $2);
  }
  ;

optional_dot_dot_dot: /* empty */ { $$ = NULL; }
  | TOK_DOT_DOT_DOT
  | TOK_DOT_DOT
  {
    yyerror("Range indicator ('..') where elipsis ('...') expected.");
    $$ = $1;
  }
  ;

optional_identifier: /* empty */ { $$=NULL; }
  | TOK_IDENTIFIER
  | bad_identifier
  ;

new_arg_name: type7 optional_dot_dot_dot optional_identifier
  {
    $$ = ARG_NAME($1, $2, $3);
  }
  ;

func_args: TOK_LPAREN arguments close_paren_or_missing
  {
    $$=$2;
    free_node($1);
  }
  ;

arguments: /* empty */ optional_comma { $$ = NULL; }
  | arguments2 optional_comma
  ;

arguments2: new_arg_name
  | arguments2 TOK_COMMA new_arg_name
  {
    $$ = LIST($1, $3);
    free_sval($2);
  }
  | arguments2 TOK_COLON new_arg_name
  {
    yyerror("Unexpected ':' in argument list.");
    $$ = LIST($1, $3);
    free_sval($2);
  }
  ;

modifier:
    TOK_NO_MASK
  | TOK_FINAL_ID
  | TOK_STATIC
  | TOK_EXTERN
  | TOK_OPTIONAL
  | TOK_PRIVATE
  | TOK_LOCAL_ID
  | TOK_PUBLIC
  | TOK_PROTECTED
  | TOK_INLINE
  | TOK_VARIANT
  ;

magic_identifiers1:
    TOK_NO_MASK
  | TOK_FINAL_ID
  | TOK_STATIC
  | TOK_EXTERN
  | TOK_PRIVATE
  | TOK_LOCAL_ID
  | TOK_PUBLIC
  | TOK_PROTECTED
  | TOK_INLINE
  | TOK_OPTIONAL
  | TOK_VARIANT
  ;

magic_identifiers2:
    TOK_VOID_ID
  | TOK_MIXED_ID
  | TOK_ARRAY_ID
  | TOK_MAPPING_ID
  | TOK_MULTISET_ID
  | TOK_OBJECT_ID
  | TOK_FUNCTION_ID
  | TOK_PROGRAM_ID
  | TOK_STRING_ID
  | TOK_FLOAT_ID
  | TOK_INT_ID
  | TOK_ENUM
  | TOK_TYPEDEF
  ;

magic_identifiers3:
    TOK_IF
  | TOK_DO
  | TOK_FOR
  | TOK_WHILE
  | TOK_ELSE
  | TOK_FOREACH
  | TOK_CATCH
  | TOK_GAUGE
  | TOK_CLASS
  | TOK_BREAK
  | TOK_CASE
  | TOK_CONSTANT
  | TOK_CONTINUE
  | TOK_DEFAULT
  | TOK_IMPORT
  | TOK_INHERIT
  | TOK_LAMBDA
  | TOK_PREDEF
  | TOK_RETURN
  | TOK_SSCANF
  | TOK_SWITCH
  | TOK_TYPEOF
  | TOK_GLOBAL
  ;

magic_identifiers: magic_identifiers1 | magic_identifiers2 | magic_identifiers3 ;

magic_identifier: TOK_IDENTIFIER 
  | magic_identifiers
  ;

modifiers: modifier_list
 ;

modifier_list: /* empty */ { $$ = NULL; }
  | modifier modifier_list { $$ = LIST($1, $2); }
  ;

optional_stars: /* empty */ { $$ = NULL; }
  | optional_stars '*' { $$ = LIST($1, $2); }
  ;

cast: TOK_LPAREN type TOK_RPAREN
    {
      $$ = CAST($2); free_sval($1); free_sval($3);
    }
    ;

soft_cast: TOK_LBRACKET type TOK_RBRACKET
    {
      $$ = SOFT_CAST($2); free_sval($1); free_sval($3);
    }
    ;

full_type: type4
  | full_type TOK_STAR
  {
    $$ = ARRAY_TYPE($1);
    free_sval(context, $2);
  }
  ;

type6: type | identifier_type ;
  
type: type '*'
  {
    $$ = ARRAY_TYPE($1);
    free_sval(context, $2);
  }
  | type2
  ;

type7: type7 '*'
  {
    $$ = ARRAY_TYPE($1);
    free_sval(context, $2);
  }
  | type4
  ;

simple_type: type4
  ;

simple_type2: type2
  ;

simple_identifier_type: identifier_type
  ;

type4: type4 TOK_OR type8
  {
    $$ = BINOP($2, $1, $3);
  }
  | type8
  ;

type2: type2 TOK_OR type8
  {
    $$ = BINOP($2, $1, $3);
  }
  | basic_type 
  ;

type8: basic_type | identifier_type ;

basic_type:
    TOK_FLOAT_ID
  | TOK_VOID_ID
  | TOK_MIXED_ID
  | TOK_STRING_ID
  | TOK_INT_ID      opt_int_range
  {
    $$ = TYPE_OPTION($1, $2);
  }
  | TOK_MAPPING_ID  opt_mapping_type
  {
    $$ = TYPE_OPTION($1, $2);
  }
  | TOK_FUNCTION_ID opt_function_type
  {
    $$ = TYPE_OPTION($1, $2);
  }
  | TOK_OBJECT_ID   opt_object_type
  {
    $$ = TYPE_OPTION($1, $2);
  }
  | TOK_PROGRAM_ID  opt_object_type
  {
    $$ = TYPE_OPTION($1, $2);
  }
  | TOK_ARRAY_ID    opt_array_type
  {
    $$ = TYPE_OPTION($1, $2);
  }
  | TOK_MULTISET_ID opt_array_type
  {
    $$ = TYPE_OPTION($1, $2);
  }
  ;

identifier_type: idents
  ;

number_or_maxint: /* Empty */
  {
    $$ = NULL;
  }
  | TOK_NUMBER
  | TOK_MINUS TOK_NUMBER
  { $$ = UNOP($1, $2); }
  ;

number_or_minint: /* Empty */
  {
    $$ = NULL;
  }
  | TOK_NUMBER
  | '-' TOK_NUMBER
  { $$ = UNOP($1, $2); }
  ;

expected_dot_dot: TOK_DOT_DOT
  | TOK_DOT_DOT_DOT
  {
    yyerror("Elipsis ('...') where range indicator ('..') expected.");
  }
  ;

opt_int_range: /* Empty */
  {
    $$ = NULL;
  }
  | TOK_LPAREN number_or_minint expected_dot_dot number_or_maxint TOK_RPAREN
  {
    $$ = LIST($2, $4);
    free_sval(context, $1);
    free_sval(context, $3);
    free_sval(context, $5);
  }
  ;

opt_object_type:  /* Empty */ { $$ = NULL; }
  | TOK_LPAREN program_ref TOK_RPAREN
  {
    $$ = $2;
    free_sval(context, $1);
    free_sval(context, $3);
  }
  ;

opt_function_type: /* empty */
  { $$ = NULL; }
  | TOK_LPAREN function_type_list optional_dot_dot_dot TOK_COLON
    type7 TOK_RPAREN
  {
    $$ = FUNC_TYPE($2, $3, $5);
    free_sval(context, $1);
    free_sval(context, $4);
    free_sval(context, $6);
  }
  ;

function_type_list: /* Empty */ optional_comma { $$ = NULL; }
  | function_type_list2 optional_comma { $$ = $1; }
  ;

function_type_list2: type7
  | function_type_list2 TOK_COMMA type7
  {
    $$ = LIST($1, $3);
    free_sval(context, $2);
  }
  ;

opt_array_type: /* empty */ { $$ = NULL; }
  | TOK_LPAREN type7 TOK_RPAREN
  {
    $$ = $2;
    free_sval(context, $1);
    free_sval(context, $3);
  }
  ;

opt_mapping_type: /* empty */ 
  {
    push_type(T_MIXED);
    push_type(T_MIXED);
    push_type(T_MAPPING);
  }
  | TOK_LPAREN type7 TOK_COLON type7 TOK_RPAREN
  { 
    $$ = LIST($2, $4);
    free_sval(context, $1);
    free_sval(context, $3);
    free_sval(context, $5);
  }
  ;

name_list: new_name
  | name_list ',' new_name
  {
    $$ = LIST($1, $3);
    free_sval(context, $2);
  }
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
    push_finished_type($<n>0->u.sval.u.type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
      yywarning("The *-syntax in types is obsolete. Use array instead.");
    }
    while($1--) push_type(T_ARRAY);
    add_local_name($2->u.sval.u.string, compiler_pop_type(),0);
    $$=mknode(F_ASSIGN,mkintnode(0),mklocalnode(islocal($2->u.sval.u.string),0));
    free_node($2);
  }
  | optional_stars bad_identifier { $$=0; }
  | optional_stars TOK_IDENTIFIER '=' expr0 
  {
    push_finished_type($<n>0->u.sval.u.type);
    if ($1 && (Pike_compiler->compiler_pass == 2)) {
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
    add_ref($<n>0->u.sval.u.type);
    add_local_name($1->u.sval.u.string, $<n>0->u.sval.u.type, 0);
    $$=mknode(F_ASSIGN,mkintnode(0),mklocalnode(islocal($1->u.sval.u.string),0));
    free_node($1);
  }
  | bad_identifier { $$=0; }
  | TOK_IDENTIFIER '=' safe_expr0
  {
    add_ref($<n>0->u.sval.u.type);
    add_local_name($1->u.sval.u.string, $<n>0->u.sval.u.type, 0);
    $$=mknode(F_ASSIGN,$3, mklocalnode(islocal($1->u.sval.u.string),0));
    free_node($1);
  }
  | bad_identifier '=' safe_expr0 { $$=$3; }
  ;

block:'{'
  {
    $<number>1=Pike_compiler->num_used_modules;
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  } 
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;

    if($<number>$ == -1) /* if 'first block' */
      Pike_compiler->compiler_frame->last_block_level=0; /* all variables */
    else
      Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  {
    $<number>$=lex.current_line;
  }
  statements end_block
  {
    unuse_modules(Pike_compiler->num_used_modules - $<number>1);
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>3;
    if ($5) $5->line_number = $<number>4;
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
      ptrdiff_t tmp=eval_low($3);
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
    $<number>$ = lex.current_line;
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
    $$->line_number = $<number>2;
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
    $<number>$ = lex.current_line;
  }
  ;

lambda: TOK_LAMBDA push_compiler_frame1
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
    int save_line = lex.current_line;
    lex.current_line = $<number>1;

    debug_malloc_touch($6);
    $6=mknode(F_COMMA_EXPR,$6,mknode(F_RETURN,mkintnode(0),0));
    type=find_return_type($6);

    if(type) {
      push_finished_type(type);
      free_type(type);
    } else
      push_type(T_MIXED);
    
    e=$4-1;
    if($<number>5)
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
		$6,
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
    pop_compiler_frame();
  }
  | TOK_LAMBDA push_compiler_frame1 error
  {
    pop_compiler_frame();
    $$ = mkintnode(0);
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
      Pike_compiler->varargs=0;
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
			 IDENTIFIER_PIKE_FUNCTION,
			 0,
			 OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
    }
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
    int save_line = lex.current_line;
#ifdef PIKE_DEBUG
    struct pike_string *save_file = lex.current_file;
    lex.current_file = $1->current_file;
#endif
    lex.current_line = $1->line_number;

    $5=mknode(F_COMMA_EXPR,$5,mknode(F_RETURN,mkintnode(0),0));

    debug_malloc_touch($5);
    dooptcode(i->name,
	      $5,
	      i->type,
	      ID_STATIC | ID_PRIVATE | ID_INLINE);

    i->opt_flags = Pike_compiler->compiler_frame->opt_flags;

    lex.current_line = save_line;
#ifdef PIKE_DEBUG
    lex.current_file = save_file;
#endif
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
	$$ = mktrampolinenode($<number>3,Pike_compiler->compiler_frame);
      }else{
	$$ = mkidentifiernode($<number>3);
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
      Pike_compiler->varargs=0;
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
			 IDENTIFIER_PIKE_FUNCTION,
			 0,
			 OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
    }
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
    int save_line = lex.current_line;
#ifdef PIKE_DEBUG
    struct pike_string *save_file = lex.current_file;
    lex.current_file = $2->current_file;
#endif
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
#ifdef PIKE_DEBUG
    lex.current_file = save_file;
#endif
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

    MAKE_CONSTANT_SHARED_STRING(create_string, "create");

    /* First: Deduce the type for the create() function. */
    push_type(T_VOID); /* Return type. */
    e = $3-1;
    if (Pike_compiler->varargs) {
      /* Varargs */
      push_finished_type(Pike_compiler->compiler_frame->variable[e--].type);
      pop_type_stack(T_ARRAY); /* Pop one level of array. */
      Pike_compiler->varargs = 0;
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
		      ID_INLINE | ID_STATIC, IDENTIFIER_PIKE_FUNCTION, 0,
		      OPT_SIDE_EFFECT);

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
    free_string(create_string);
  }
  ;

failsafe_program: '{' program end_block
                | error { yyerrok; }
                | TOK_LEX_EOF
                {
		  yyerror("End of file where program definition expected.");
		}
                ;

class: modifiers TOK_CLASS optional_identifier
  {
    extern int num_parse_error;
    int num_errors=Pike_compiler->num_parse_error;
    if(!$3)
    {
      struct pike_string *s;
      char buffer[42];
      sprintf(buffer,"__class_%ld_%ld_line_%d",
	      (long)Pike_compiler->new_program->id,
	      (long)Pike_compiler->local_class_counter++,
	      (int) $<number>2);
      s=make_shared_string(buffer);
      $3=mkstrnode(s);
      free_string(s);
      $1|=ID_STATIC | ID_PRIVATE | ID_INLINE;
    }
    /* fprintf(stderr, "LANGUAGE.YACC: CLASS start\n"); */
    if(Pike_compiler->compiler_pass==1)
    {
      if ($1 & ID_EXTERN) {
	yywarning("Extern declared class definition.");
      }
      low_start_new_program(0, $3->u.sval.u.string,
			    $1,
			    &$<number>$);

      /* fprintf(stderr, "Pass 1: Program %s has id %d\n",
	 $3->u.sval.u.string->str, Pike_compiler->new_program->id); */

      if(lex.current_file)
      {
	store_linenumber($<number>2, lex.current_file);
	debug_malloc_name(Pike_compiler->new_program, lex.current_file->str,
			  $<number>2);
      }
    }else{
      int i;
      struct identifier *id;
      int tmp=Pike_compiler->compiler_pass;
      i=isidentifier($3->u.sval.u.string);
      if(i<0)
      {
	low_start_new_program(Pike_compiler->new_program,0,
			      $1,
			      &$<number>$);
	yyerror("Pass 2: program not defined!");
      }else{
	id=ID_FROM_INT(Pike_compiler->new_program, i);
	if(IDENTIFIER_IS_CONSTANT(id->identifier_flags))
	{
	  struct svalue *s;
	  s=&PROG_FROM_INT(Pike_compiler->new_program,i)->constants[id->func.offset].sval;
	  if(s->type==T_PROGRAM)
	  {
	    low_start_new_program(s->u.program,
				  $3->u.sval.u.string,
				  $1,
				  &$<number>$);

	    /* fprintf(stderr, "Pass 2: Program %s has id %d\n",
	       $3->u.sval.u.string->str, Pike_compiler->new_program->id); */

	  }else{
	    yyerror("Pass 2: constant redefined!");
	    low_start_new_program(Pike_compiler->new_program, 0,
				  $1,
				  &$<number>$);
	  }
	}else{
	  yyerror("Pass 2: class constant no longer constant!");
	  low_start_new_program(Pike_compiler->new_program, 0,
				$1,
				&$<number>$);
	}
      }
      Pike_compiler->compiler_pass=tmp;
    }
    Pike_compiler->num_parse_error=num_errors; /* Kluge to prevent gazillion error messages */
  }
  optional_create_arguments failsafe_program
  {
    struct program *p;
    if(Pike_compiler->compiler_pass == 1)
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
	ptrdiff_t tmp=eval_low($2);
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
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' safe_comma_expr end_cond statement optional_else_part
  {
    int i=lex.current_line;
    lex.current_line=$1;
    $$ = mknode('?', $5,
		mknode(':',
		       mkcastnode(void_type_string, $7),
		       mkcastnode(void_type_string, $8)));
    $$ = mkcastnode(void_type_string, $$);
    lex.current_line = i;
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>3;
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
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' expr0 foreach_lvalues end_cond statement
  {
    if ($6) {
      $$=mknode(F_FOREACH,
		mknode(F_VAL_LVAL,$5,$6),
		$8);
      $$->line_number=$1;
    } else {
      /* Error in lvalue */
      free_node($5);
      $$=$8;
    }
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>3;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;

do: TOK_DO statement TOK_WHILE '(' safe_comma_expr end_cond expected_semicolon
  {
    $$=mknode(F_DO,$2,$5);
    $$->line_number=$1;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  | TOK_DO statement TOK_WHILE TOK_LEX_EOF
  {
    $$=0;
    yyerror("Missing '(' in do-while loop.");
    yyerror("Unexpected end of file.");
  }
  | TOK_DO statement TOK_LEX_EOF
  {
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
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' unused expected_semicolon for_expr expected_semicolon unused end_cond
  statement
  {
    int i=lex.current_line;
    lex.current_line=$1;
    $$=mknode(F_COMMA_EXPR, mkcastnode(void_type_string, $5),
	      mknode(F_FOR,$7,mknode(':',$11,$9)));
    lex.current_line=i;
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>3;
    Pike_compiler->compiler_frame->opt_flags |= OPT_CUSTOM_LABELS;
  }
  ;


while:  TOK_WHILE
  {
    $<number>$=Pike_compiler->compiler_frame->current_number_of_locals;
  }
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' safe_comma_expr end_cond statement
  {
    int i=lex.current_line;
    lex.current_line=$1;
    $$=mknode(F_FOR,$5,mknode(':',$7,NULL));
    lex.current_line=i;
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>3;
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
  {
    /* Trick to store more than one number on compiler stack - Hubbe */
    $<number>$=Pike_compiler->compiler_frame->last_block_level;
    Pike_compiler->compiler_frame->last_block_level=$<number>2;
  }
  '(' safe_comma_expr end_cond statement
  {
    $$=mknode(F_SWITCH,$5,$7);
    $$->line_number=$1;
    pop_local_variables($<number>2);
    Pike_compiler->compiler_frame->last_block_level=$<number>3;
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
      yyerror("Must return a value for a non-void function.");
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

assoc_pair:  expr0 expected_colon expr0 { $$=mknode(F_ARG_LIST,$1,$3); }
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
  | '{' push_compiler_frame0
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
    int save_line = lex.current_line;
    lex.current_line = $<number>2;

    /* block code */
    unuse_modules(Pike_compiler->num_used_modules - $<number>1);
    pop_local_variables($<number>3);

    debug_malloc_touch($4);
    $4=mknode(F_COMMA_EXPR,$4,mknode(F_RETURN,mkintnode(0),0));

    type=find_return_type($4);

    if(type) {
      push_finished_type(type);
      free_type(type);
    } else
      push_type(T_MIXED);
    
    push_type(T_VOID);
    push_type(T_MANY);
/*
    e=$4-1;
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
		$4,
		type,
		ID_STATIC | ID_PRIVATE | ID_INLINE);

    if(Pike_compiler->compiler_frame->lexical_scope & SCOPE_SCOPED) {
      $$ = mktrampolinenode(f,Pike_compiler->compiler_frame->previous);
    } else {
      $$ = mkidentifiernode(f);
    }

    lex.current_line = save_line;
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
  | expr4 '(' expr_list  ')'
    {
      $$=mkapplynode($1,$3);
    }
  | expr4 '(' error ')' { $$=mkapplynode($1, NULL); yyerrok; }
  | expr4 '(' error TOK_LEX_EOF
  {
    yyerror("Missing ')'."); $$=mkapplynode($1, NULL);
    yyerror("Unexpected end of file.");
  }
  | expr4 '(' error ';' { yyerror("Missing ')'."); $$=mkapplynode($1, NULL); }
  | expr4 '(' error '}' { yyerror("Missing ')'."); $$=mkapplynode($1, NULL); }
  | expr4 '[' '*' ']'   { $$=mknode(F_AUTO_MAP_MARKER, $1, 0); }
  | expr4 '[' expr0 ']' { $$=mknode(F_INDEX,$1,$3); }
  | expr4 '['  comma_expr_or_zero expected_dot_dot comma_expr_or_maxint ']'
  {
    $$=mknode(F_RANGE,$1,mknode(F_ARG_LIST,$3,$5));
  }
  | expr4 '[' error ']' { $$=$1; yyerrok; }
  | expr4 '[' error TOK_LEX_EOF
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
  | TOK_MULTISET_START expr_list TOK_MULTISET_END
    { $$=mkefuncallnode("aggregate_multiset",$2); }
  | TOK_MULTISET_START expr_list ')'
    {
      yyerror("Missing '>'.");
      $$=mkefuncallnode("aggregate_multiset",$2);
    }
  | '(' error ')' { $$=0; yyerrok; }
  | '(' error TOK_LEX_EOF
  {
    $$=0; yyerror("Missing ')'.");
    yyerror("Unexpected end of file.");
  }
  | '(' error ';' { $$=0; yyerror("Missing ')'."); }
  | '(' error '}' { $$=0; yyerror("Missing ')'."); }
  | TOK_MULTISET_START error TOK_MULTISET_END { $$=0; yyerrok; }
  | TOK_MULTISET_START error ')' {
    yyerror("Missing '>'.");
    $$=0; yyerrok;
  }
  | TOK_MULTISET_START error TOK_LEX_EOF
  {
    $$=0; yyerror("Missing '>)'.");
    yyerror("Unexpected end of file.");
  }
  | TOK_MULTISET_START error ';' { $$=0; yyerror("Missing '>)'."); }
  | TOK_MULTISET_START error '}' { $$=0; yyerror("Missing '>)'."); }
  | expr4 TOK_ARROW magic_identifier
  {
    $$=mknode(F_ARROW,$1,$3);
  }
  | expr4 TOK_ARROW error {}
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
      if (!(Pike_compiler->new_program->identifier_references[i].id_flags & ID_HIDDEN)) {
	/* We need to generate a new reference. */
	int d;
	struct reference funp = Pike_compiler->new_program->identifier_references[i];
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
    } else {
      if (!Pike_compiler->num_parse_error) {
	if (Pike_compiler->compiler_pass == 2) {
	  my_yyerror("'%s' not defined in local scope.", Pike_compiler->last_identifier->str);
	  $$ = 0;
	} else {
	  $$ = mknode(F_UNDEFINED, 0, 0);
	}
      } else {
	$$ = mkintnode(0);
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
    MAKE_CONSTANT_SHARED_STRING(dot, ".");
    if (call_handle_import(dot)) {
      node *tmp=mkconstantsvaluenode(Pike_sp-1);
      pop_stack();
      $$=index_node(tmp, ".", $2->u.sval.u.string);
      free_node(tmp);
    }
    else
      $$=mknewintnode(0);
    free_string(dot);
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
    int i;

    if(Pike_compiler->last_identifier) free_string(Pike_compiler->last_identifier);
    copy_shared_string(Pike_compiler->last_identifier, $1->u.sval.u.string);

    if(($$=lexical_islocal(Pike_compiler->last_identifier)))
    {
      /* done, nothing to do here */
    }else if(!($$=find_module_identifier(Pike_compiler->last_identifier,1)) &&
	     !($$ = program_magic_identifier (Pike_compiler, 0, 0,
					      Pike_compiler->last_identifier, 0))) {
      if(!Pike_compiler->num_parse_error)
      {
	if(Pike_compiler->compiler_pass==2)
	{
	  my_yyerror("'%s' undefined.", Pike_compiler->last_identifier->str);
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
	if (Pike_compiler->compiler_pass == 2) {
	  if (inherit_state->new_program->inherits[$1].name) {
	    my_yyerror("Undefined identifier %s::%s.",
		       inherit_state->new_program->inherits[$1].name->str,
		       Pike_compiler->last_identifier->str);
	  } else {
	    my_yyerror("Undefined identifier %s.", Pike_compiler->last_identifier->str);
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
	/* FIXME: Shouldn't this be an error? /mast */
	$$=mkintnode(0);
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

comma_expr_or_maxint: /* empty */ { $$=mkintnode(0x7fffffff); }
  | comma_expr
  | TOK_LEX_EOF { yyerror("Unexpected end of file."); $$=mkintnode(0x7fffffff); }
  ;

gauge: TOK_GAUGE catch_arg
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
  | '[' low_lvalue_list ']' { $$=mknode(F_ARRAY_LVALUE, $2,0); }
  | type6 TOK_IDENTIFIER
  {
    add_local_name($2->u.sval.u.string,compiler_pop_type(),0);
    $$=mklocalnode(islocal($2->u.sval.u.string),0);
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

%%

/*
 * Misc section
 */

void init_tokenizer_yyparse(void)
{
  int i;

#ifdef PIKE_DEBUG
  if (token_to_int) {
    fatal("init_tokenizer_yyparse() called multiple times!\n");
  }
#endif /* PIKE_DEBUG */

  for (i=0; i < sizeof(token_names)/sizeof(token_names[0]); i++) {
    push_text(token_names[i]);
    push_int(TOK_SEMICOLON + i);
  }
#ifdef PIKE_DEBUG
  if (TOK_SEMICOLON + i != TOK_IDENTIFIER + 1) {
    fatal("Token count mismatch: %d != %d\n",
	  TOK_IDENTIFIER + 1 - TOK_SEMICOLON, i);
  }
#endif /* PIKE_DEBUG */
  f_aggregate_mapping(i*2);
#ifdef PIKE_DEBUG
  if (Pike_sp[-1].type != PIKE_T_MAPPING) {
    fatal("Bad return value from f_aggregate_mapping()\n");
  }
#endif /* PIKE_DEBUG */
  Pike_sp--;
  token_to_int = Pike_sp[0].u.mapping;
}

