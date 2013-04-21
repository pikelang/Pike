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

all: program ;

program: program def
  { $$ = DEF_LIST($1, $2); }
  | program TOK_SEMICOLON
  { free_sval(context, $2); $$ = $1; }
  |  /* empty */
  { $$ = NULL; }
  ;

def: symbol_lookup_directive
  | constant
  | class
  | enum
  | typedef
  | func_def
  | var_def
  | modifiers program_block
  { $$ = MODIFIER_BLOCK($1, $2); }
  ;

/*
 * Symbol lookup directive
 */

symbol_lookup_directive:
  modifiers TOK_NAMESPACE program_ref optional_rename TOK_SEMICOLON
  { $$ = SYMBOL_LOOKUP($1, $2, $3, $4); free_sval(context, $5); }
  ;

program_ref: string_constant
  | idents
  ;

optional_rename: TOK_COLON TOK_IDENTIFIER
  { $$ = $2; free_sval(context, $1); }
  | /* empty */
  { $$ = NULL; }
  ;

/*
 * Constant directive
 */

constant: modifiers TOK_CONSTANT constant_list TOK_SEMICOLON
  { $$ = CONSTANT_DEF($1, $3); free_sval(context, $2); free_sval(context, $4); }
  ;

constant_list: constant_name
  { $$ = CONSTANT_LIST(NULL, $1); }
  | constant_list TOK_COMMA constant_name
  { $$ = CONSTANT_LIST($1, $3); free_sval(context, $2); }
  ;

constant_name: simple_identifier initialization
  { $$ = IDENTIFIER_INIT($1, $2); }
  ;

/*
 * Class directive
 */

class: modifiers TOK_CLASS optional_identifier
  optional_create_arguments program_block
  {
    /* FIXME: Might want to use $2 for linenumber info. */
    $$ = CLASS_DEF($1, $3, $4, $5); free_sval(context, $2);
  }
  ;

optional_create_arguments: /* empty */
  { $$ = NULL; }
  | TOK_LPAREN create_arguments close_paren_or_missing
  { $$ = $2; free_sval(context, $1); }
  ;

create_arguments: /* empty */ optional_comma
  { $$ = NULL; }
  | create_arguments2 optional_comma
  { $$ = $1; }
  ;

create_arguments2: create_arg
  { $$ = ARG_DEF_LIST(NULL, $1); }
  | create_arguments2 TOK_COMMA create_arg
  { $$ = ARG_DEF_LIST($1, $3); free_sval(context, $2); }
  | create_arguments2 TOK_COLON
  { yyerror("Expected ',', got ':'."); }
    create_arg
  { $$ = ARG_DEF_LIST($1, $4); free_sval(context, $2); }
  ;

create_arg:
    modifiers type4 optional_stars optional_dot_dot_dot simple_identifier
  { $$ = ARG_DEF($1, $2, $3, $4, $5); }
  ;

program_block: TOK_LBRACE program TOK_RBRACE
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  ;

/*
 * Enum directive
 */

enum: modifiers TOK_ENUM
  optional_identifier TOK_LBRACE enum_list optional_comma TOK_RBRACE
  {
    /* FIXME: Might want to use $2 for linenumber info. */
    $$ = ENUM_DEF($1, $3, $5);
    free_sval(context, $2); free_sval(context, $4); free_sval(context, $7);
  }
  ;

enum_list: enum_def
  { $$ = ENUM_LIST(NULL, $1); }
  | enum_list TOK_COMMA enum_def
  { $$ = ENUM_LIST($1, $3); free_sval(context, $2); }
  ;

enum_def: simple_identifier optional_initialization
  { $$ = IDENTIFIER_INIT($1, $2); }
  ;

/*
 * Typedef directive
 */

typedef: modifiers TOK_TYPEDEF full_type simple_identifier TOK_SEMICOLON
  {
    /* FIXME: Might want to use $2 for line no info. */
    $$ = TYPE_DEF($1, $3, $4);
    free_sval(context, $2); free_sval(context, $5);
  }
  ;

/*
 * Function definition
 */

func_def: modifiers type4 optional_stars TOK_IDENTIFIER func_args block_or_semi
  { $$ = FUNC_DEF($1, $2, $3, $4, $5, $6); }
  ;

func_args: TOK_LPAREN arguments close_paren_or_missing
  { $$ = $2; free_sval(context, $1); }
  ;

arguments:
    optional_base_argument_list
    new_many_arg_name optional_comma
  { $$ = ARG_DEF_LIST($1, $2); }
  | base_argument_list
  | optional_base_argument_list
  | /* empty */ TOK_COMMA
  { $$ = NULL; free_sval(context, $1); }
  ;

optional_base_argument_list: /* empty */
  { $$ = NULL; }
  | base_argument_list TOK_COMMA
  { $$ = $1; free_sval(context, $2); }
  | base_argument_list TOK_COLON
  {
    $$ = $1; free_sval(context, $2);
    yyerror("Expected ',', got ':'.");
  }
  ;

base_argument_list: optional_base_argument_list new_arg_name
  { $$ = ARG_DEF_LIST($1, $2); }
  ;

new_arg_name: full_type optional_identifier
  { $$ = ARG_DEF(NULL, $1, NULL, NULL, $2); }
  ;

new_many_arg_name: full_type many_indicator optional_identifier
  { $$ = ARG_DEF(NULL, $1, NULL, $2, $3); }
  ;

many_indicator: TOK_DOT_DOT_DOT
  | TOK_DOT_DOT
  {
    $$ = $1;
    yyerror("Expected '...', got '..'.");
  }
  ;

block_or_semi: block
  | TOK_SEMICOLON
  { $$ = NULL; free_sval(context, $1); }
  ;

/*
 * Variable definition
 */

var_def: modifiers type4 name_list TOK_SEMICOLON
  { $$ = VAR_DEF($1, $2, $3); free_sval(context, $4); }
  ;

name_list: optional_stars new_name
  { $$ = VAR_LIST(NULL, $1, $2); }
  | name_list TOK_COMMA optional_stars new_name
  { $$ = VAR_LIST($1, $3, $4); free_sval(context, $2); }
  ;

name_list2: new_name
  { $$ = VAR_LIST(NULL, NULL, $1); }
  | name_list2 TOK_COMMA optional_stars new_name
  { $$ = VAR_LIST($1, $3, $4); free_sval(context, $2); }
  ;

new_name: simple_identifier optional_initialization
  { $$ = IDENTIFIER_INIT($1, $2); }
  ;

optional_initialization: /* empty */
  { $$ = NULL; }
  | initialization
  ;

initialization: TOK_ASSIGN expr0
  { $$ = $2; free_sval(context, $1); }
  ;

/*
 * Modifier handling
 */

modifiers: modifier_list
  ;

modifier_list: /* empty */
  { $$ = NULL; }
  | modifier_list TOK_MODIFIER
  { $$ = MODIFIER_LIST($1, $2); }
  ;

/*
 * Statement handling
 */

block: TOK_LBRACE statements TOK_RBRACE
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  ;

statements: /* empty */
  { $$ = NULL; }
  | statements statement
  { $$ = STATEMENT_LIST($1, $2); }
  ;

statement: normal_label_statement
  | cond
  | while
  | do
  | for
  | foreach
  | switch
  | label statement
  { $$ = STATEMENT_LABEL($1, $2); }
  ;

normal_label_statement: statement_with_semicolon
  | return
  | break TOK_SEMICOLON
  { $$ = $1; free_sval(context, $2); }
  | continue TOK_SEMICOLON
  { $$ = $1; free_sval(context, $2); }
  | local_constant
  | block
  | TOK_SEMICOLON
  { $$ = NULL; free_sval(context, $1); }
  ;

statement_with_semicolon: comma_expr optional_block
  { $$ = EXPR_STATEMENT($1, $2); }
  ;

label: TOK_IDENTIFIER TOK_COLON
  { $$ = LABEL($1); free_sval(context, $2); }
  | case
  | default
  ;

/*
 * If statement
 *
 * Note: The rule below has the dangling-else ambiguity.
 *       The ambiguity hasn't been removed, since removing
 *       it causes a significant number of duplicate rules.
 */

cond: TOK_IF
  TOK_LPAREN comma_expr end_cond statement optional_else_part
  {
    $$ = IF_STMT($3, $5, $6);
    free_sval(context, $1);
    free_sval(context, $2);
  }
  ;

end_cond: TOK_RPAREN
  { free_sval(context, $1); }
  | TOK_RBRACE
  {
    yyerror("Expected ')', got '}'.");
    free_sval(context, $1);
  }
  ;

optional_else_part: /* empty */
  { $$ = NULL; }
  | TOK_ELSE statement
  { $$ = $2; free_sval(context, $1); }
  ;      

/*
 * Loop statements
 */

while:  TOK_WHILE
  TOK_LPAREN comma_expr end_cond statement
  {
    $$ = WHILE_STMT($3, $5);
    free_sval(context, $1);
    free_sval(context, $2);
  }
  ;

do: TOK_DO statement TOK_WHILE TOK_LPAREN comma_expr end_cond TOK_SEMICOLON
  {
    $$ = DO_STMT($2, $5);
    free_sval(context, $1);
    free_sval(context, $3);
    free_sval(context, $4);
    free_sval(context, $7);
  }
  ;

for: TOK_FOR
  TOK_LPAREN optional_comma_expr TOK_SEMICOLON optional_comma_expr TOK_SEMICOLON optional_comma_expr
  end_cond statement
  {
    $$ = FOR_STMT($3, $5, $7, $9);
    free_sval(context, $1);
    free_sval(context, $2);
    free_sval(context, $4);
    free_sval(context, $6);
  }
  ;

foreach: TOK_FOREACH
  TOK_LPAREN expr0 foreach_lvalues end_cond statement
  {
    $$ = FOREACH_STMT($3, $4, $6);
    free_sval(context, $1);
    free_sval(context, $2);
  }
  ;

foreach_lvalues:  TOK_COMMA lvalue
  {
    $$ = LVALUE_PAIR(NULL, $2);
    free_sval(context, $1);
  }
  | TOK_SEMICOLON optional_lvalue TOK_SEMICOLON optional_lvalue
  {
    $$ = LVALUE_PAIR($2, $4);
    free_sval(context, $1);
    free_sval(context, $3);
  }
  ;

optional_lvalue: /* empty */
  { $$ = NULL; }
  | lvalue
  ;

lvalue: lvalue_expr
  | TOK_LBRACKET low_lvalue_list TOK_RBRACKET
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  | idents TOK_IDENTIFIER
  { $$ = VAR_DEF($1, $2); }
  | bad_expr_ident
  { $$ = NULL; }
  ;

low_lvalue_list: lvalue
  { $$ = LVALUE_LIST(NULL, $1); }
  | low_lvalue_list TOK_COMMA lvalue
  { $$ = LVALUE_LIST($1, $3); free_sval(context, $2); }
  ;

lvalue_list: /* empty */
  { $$ = NULL; }
  | TOK_COMMA low_lvalue_list
  { $$ = $2; free_sval(context, $1); }
  ;

/*
 * Switch statement
 */

switch:	TOK_SWITCH
  TOK_LPAREN comma_expr end_cond statement
  { $$ = SWITCH_STMT($3, $5); free_sval(context, $1); free_sval(context, $2); }
  ;

case: TOK_CASE comma_expr expected_colon
  { $$ = CASE_LABEL($2); free_sval(context, $1); }
  | TOK_CASE comma_expr expected_dot_dot optional_comma_expr expected_colon
  { $$ = CASE_RANGE($2, $4); free_sval(context, $1); }
  | TOK_CASE expected_dot_dot comma_expr expected_colon
  { $$ = CASE_RANGE(NULL, $3); free_sval(context, $1); }
  ;

default: TOK_DEFAULT TOK_COLON
  { $$ = $1; free_sval(context, $2); }
  | TOK_DEFAULT
  { yyerror("Missing ':' after default."); $$ = $1; }
  ;

/*
 * Control flow statements
 */

return: TOK_RETURN TOK_SEMICOLON
  { $$ = RETURN_STMT($1, NULL); free_sval(context, $2); }
  | TOK_RETURN comma_expr TOK_SEMICOLON
  { $$ = RETURN_STMT($1, $2); free_sval(context, $3); }
  ;
	
break: TOK_BREAK optional_identifier
  { $$ = BREAK_STMT($1, $2); }
  ;

continue: TOK_CONTINUE optional_identifier
  { $$ = CONTINUE_STMT($1, $2); }
  ;

/*
 * Local constant handling
 */

local_constant: TOK_CONSTANT constant_list TOK_SEMICOLON
  { $$ = CONSTANT_DEF(NULL, $2); free_sval(context, $1); free_sval(context, $3); }
  ;

/*
 * Expression handling
 */

comma_expr: comma_expr2
  | local_def
  ;

comma_expr2: expr0
  | comma_expr2 TOK_COMMA expr0
  { $$ = COMMA_EXPR($1, $3); free_sval(context, $2); }
  ;

splice_expr: expr0
  | TOK_SPLICE expr0
  { $$ = SPLICE_EXPR($1, $2); }
  ;

expr0: expr01
  | lvalue_expr assign expr0
  { $$ = ASSIGN_EXPR($1, $2, $3); }
  | bad_expr_ident assign expr0
  { $$ = ASSIGN_EXPR(NULL, $2, $3); }
  | TOK_LBRACKET low_lvalue_list TOK_RBRACKET assign expr0
  { $$ = ASSIGN_EXPR($2, $4, $5); free_sval(context, $1); free_sval(context, $3); }
  ;

assign: TOK_ASSIGN
  | TOK_AND_EQ
  | TOK_OR_EQ
  | TOK_XOR_EQ
  | TOK_LSH_EQ
  | TOK_RSH_EQ
  | TOK_ADD_EQ
  | TOK_SUB_EQ
  | TOK_MULT_EQ
  | TOK_MOD_EQ
  | TOK_DIV_EQ
  ;

expr01: expr1
  | expr1 TOK_COND expr01 TOK_COLON expr01
  { $$ = COND_EXPR($1, $3, $5); free_sval(context, $2); free_sval(context, $4); }
  ;

expr1: expr2
  | expr1 TOK_LOR expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_LAND expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_OR expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_XOR expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_AND expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_EQ expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_NE expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_GT expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_GE expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_LT expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_LE expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_LSH expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_RSH expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_PLUS expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_MINUS expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_STAR expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_MOD expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  | expr1 TOK_DIV expr1
  { $$ = BIN_EXPR($1, $2, $3); }
  ;

expr2: expr3
  | TOK_INC lvalue_expr
  { $$ = UN_EXPR($1, $2); }
  | TOK_DEC lvalue_expr
  { $$ = UN_EXPR($1, $2); }
  | TOK_NOT expr2
  { $$ = UN_EXPR($1, $2); }
  | TOK_INVERT expr2
  { $$ = UN_EXPR($1, $2); }
  | TOK_MINUS expr2
  { $$ = UN_EXPR($1, $2); }
  | TOK_STAR expr2
  { $$ = UN_EXPR($1, $2); }
  ;

expr3: expr4
  | lvalue_expr TOK_INC
  { $$ = POST_EXPR($1, $2); }
  | lvalue_expr TOK_DEC
  { $$ = POST_EXPR($1, $2); }
  ;

/* Note: TOK_MULTISET_END is needed to avoid a shift/reduce conflict
 *       with expr01. An LR(2) parser is needed otherwise.
 */
expr4: string
  | TOK_NUMBER 
  | TOK_FLOAT
  | class
  | enum
  | catch
  | gauge
  | typeof
  | sscanf
  | lambda
  | expr4 TOK_LPAREN expr_list TOK_RPAREN
  { $$ = FUN_CALL($1, $3); free_sval(context, $2); free_sval(context, $4); }
  | TOK_LPAREN comma_expr2 TOK_RPAREN
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  | TOK_LPAREN comma_expr2 TOK_RBRACE TOK_RPAREN
  {
    yyerror("Expected ')', got '})'.");
    $$ = $2; free_sval(context, $1); free_sval(context, $3); free_sval(context, $4);
  }
  | TOK_LPAREN comma_expr2 TOK_RBRACKET TOK_RPAREN
  {
    yyerror("Expected ')', got '])'.");
    $$ = $2; free_sval(context, $1); free_sval(context, $3); free_sval(context, $4);
  }
  | TOK_LPAREN TOK_LBRACE expr_list close_brace_or_missing TOK_RPAREN
  { $$ = MK_ARRAY($3); free_sval(context, $1); free_sval(context, $2); free_sval(context, $5); }
  | TOK_LPAREN TOK_LBRACKET m_expr_list close_bracket_or_missing TOK_RPAREN
  { $$ = MK_MAPPING($3); free_sval(context, $1); free_sval(context, $2); free_sval(context, $5); }
  | TOK_LPAREN TOK_LT expr_list TOK_MULTISET_END
  { $$ = MK_MULTISET($3); free_sval(context, $1); free_sval(context, $2); free_sval(context, $4); }
  | TOK_LPAREN TOK_LT expr_list TOK_RPAREN
  {
    yyerror("Expected '>)', got ')'.");
    $$ = MK_MULTISET($3); free_sval(context, $1); free_sval(context, $2); free_sval(context, $4);
  }
  | basic_type
  | lvalue_expr
  ;

lvalue_expr: idents
  | expr4 TOK_LBRACKET expr0 TOK_RBRACKET
  { $$ = INDEX_EXPR($1, $3); free_sval(context, $2); free_sval(context, $4); }
  | expr4 TOK_LBRACKET TOK_STAR TOK_RBRACKET
  { $$ = INDEX_EXPR($1, $3); free_sval(context, $2); free_sval(context, $4); }
  | expr4 TOK_LBRACKET  comma_expr_or_zero expected_dot_dot comma_expr_or_maxint TOK_RBRACKET
  { $$ = RANGE_EXPR($1, $3, $5); free_sval(context, $2); free_sval(context, $6); }
  | expr4 TOK_ARROW magic_identifier
  { $$ = ARROW_EXPR($1, $3); free_sval(context, $2); }
  ;

expr_list: /* empty */
  { $$ = NULL; }
  | expr_list2 optional_comma
  ;

expr_list2: splice_expr
  | expr_list2 TOK_COMMA splice_expr
  { $$ = EXPR_LIST($1, $3); free_sval(context, $2); }
  ;

m_expr_list: /* empty */
  { $$ = NULL; }
  | m_expr_list2 optional_comma
  ;

m_expr_list2: assoc_pair
  { $$ = EXPR_LIST(NULL, $1); }
  | m_expr_list2 TOK_COMMA assoc_pair
  { $$ = EXPR_LIST($1, $3); free_sval(context, $2); }
  ;

assoc_pair:  expr0 expected_colon expr0
  { $$ = ASSOC_PAIR($1, $3); }
  ;

/*
 * Special forms
 */

catch: TOK_CATCH catch_arg
  { $$ = CATCH_EXPR($1, $2); }
  ;

gauge: TOK_GAUGE catch_arg
  { $$ = CATCH_EXPR($1, $2); }
  ;

typeof: TOK_TYPEOF TOK_LPAREN expr0 TOK_RPAREN
  { $$ = TYPEOF_EXPR($1, $3); free_sval(context, $2); free_sval(context, $4); }
  ;
 
sscanf: TOK_SSCANF TOK_LPAREN expr0 TOK_COMMA expr0 lvalue_list TOK_RPAREN
  {
    $$ = SSCANF_EXPR($1, $3, $5, $6);
    free_sval(context, $2); free_sval(context, $4); free_sval(context, $7);
  }
  ;

catch_arg: TOK_LPAREN comma_expr TOK_RPAREN
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  | block
  ; 

lambda: TOK_LAMBDA func_args block
  { $$ = LAMBDA_EXPR($1, $2, $3); }
  ;

/*
 * Local definitions
 */

local_def: expr0 name_list2
  {
    $$ = VARDEF($1, $2);
  }
  | expr0 TOK_IDENTIFIER func_args block
  {
    $$ = FUNDEF($1, $2, $3, $4);
  }
  ;

/*
 * Type handling
 */

full_type: type4
  | full_type TOK_STAR
  {
    $$ = ARRAY_TYPE($1);
  }
  ;

type4: type4 TOK_OR type8
  {
    $$ = OR_TYPE($1, $2);
  }
  | type8
  ;

type8: basic_type | idents ;


/*
 * Basic type handling
 */

basic_type:
    TOK_FLOAT_ID
  | TOK_VOID_ID
  | TOK_MIXED_ID
  | TOK_STRING_ID
  | TOK_INT_ID      opt_int_range
  { $$ = INT_TYPE_EXPR($1, $2); }
  | TOK_MAPPING_ID  opt_mapping_type
  { $$ = MAPPING_TYPE_EXPR($1, $2); }
  | TOK_FUNCTION_ID opt_function_type
  { $$ = FUNCTION_TYPE_EXPR($1, $2); }
  | TOK_OBJECT_ID   opt_object_type
  { $$ = OBJECT_TYPE_EXPR($1, $2); }
  | TOK_PROGRAM_ID  opt_object_type
  { $$ = PROGRAM_TYPE_EXPR($1, $2); }
  | TOK_ARRAY_ID    opt_array_type
  { $$ = ARRAY_TYPE_EXPR($1, $2); }
  | TOK_MULTISET_ID opt_array_type
  { $$ = MULTISET_TYPE_EXPR($1, $2); }
  ;

/* The following rules conflict with function calls.
 * Can be worked around by having the scanner return
 * a different token for TOK_LPAREN in these cases.
 */

opt_int_range: /* empty */
  { $$ = NULL; }
  | TOK_TYPE_SPECIFIER number_or_minint expected_dot_dot number_or_maxint TOK_RPAREN
  { $$ = RANGE_PAIR($2, $4); free_sval(context, $1); free_sval(context, $5); }
  ;

opt_mapping_type: TOK_TYPE_SPECIFIER
  full_type expected_colon
  full_type
  TOK_RPAREN
  { $$ = ASSOC_PAIR($2, $4); free_sval(context, $1); free_sval(context, $5); }
  | /* empty */ 
  { $$ = NULL; }
  ;

opt_function_type: TOK_TYPE_SPECIFIER
  function_type_list optional_dot_dot_dot expected_colon
  full_type TOK_RPAREN
  { $$ = FUNC_TYPE_SPEC($2, $3, $5); free_sval(context, $1); free_sval(context, $6); }
  | /* empty */
  { $$ = NULL; }
  ;

function_type_list: /* empty */ optional_comma
  { $$ = NULL; }
  | function_type_list2 optional_comma
  ;

function_type_list2: full_type
  { $$ = FUNC_TYPE_LIST(NULL, $1); }
  | function_type_list2 TOK_COMMA full_type
  { $$ = FUNC_TYPE_LIST($1, $3); free_sval(context, $2); }
  ;

opt_object_type:  /* empty */
  { $$ = NULL; }
  | TOK_TYPE_SPECIFIER program_ref TOK_RPAREN
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  ;

opt_array_type: TOK_TYPE_SPECIFIER full_type TOK_RPAREN
  { $$ = $2; free_sval(context, $1); free_sval(context, $3); }
  | /* empty */
  { $$ = NULL; }
  ;

/*
 * Identifier handling
 */

idents:
    inherit_specifier TOK_COLON_COLON TOK_IDENTIFIER
  { $$ = INHERIT_REF($1, $3); free_sval(context, $2); }
  | TOK_DOT TOK_IDENTIFIER
  { $$ = MODULE_REF(NULL, $2); free_sval(context, $1); }
  | TOK_IDENTIFIER
  | idents TOK_DOT TOK_IDENTIFIER
  { $$ = MODULE_REF($1, $3); free_sval(context, $2); }
  ;

inherit_specifier:
    TOK_MODIFIER
  { $$ = INHERIT_SPECIFIER($1); }
  | TOK_GLOBAL
  { $$ = INHERIT_SPECIFIER($1); }
  | TOK_PREDEF
  { $$ = INHERIT_SPECIFIER($1); }
  | inherit_specifier_path
  ;

inherit_specifier_path: /* empty */
  { $$ = NULL; }
  | TOK_IDENTIFIER
  | inherit_specifier_path TOK_COLON_COLON TOK_IDENTIFIER
  { $$ = INHERIT_PATH($1, $3); free_sval(context, $2); }
  ;

/*
 * Raw identifier
 */

simple_identifier: TOK_IDENTIFIER
  | bad_identifier
  ;

/*
 * Literals
 */

string_constant: string
  | string_constant TOK_PLUS string
  { $$ = STRING_CONCAT($1, $3); free_sval(context, $2); }
  ;

/*
 *
 */

close_paren_or_missing: TOK_RPAREN
  { free_sval(context, $1); }
  | /* empty */
  { yyerror("Missing '('."); }
  ;

close_brace_or_missing: TOK_RBRACE
  { free_sval(context, $1); }
  | /* empty */
  { yyerror("Missing '}'."); }
  ;

close_bracket_or_missing: TOK_RBRACKET
  { free_sval(context, $1); }
  | /* empty */
  { yyerror("Missing ']'."); }
  ;

optional_identifier: simple_identifier
  | /* empty */
  { $$ = NULL; }
  ;

optional_dot_dot_dot: many_indicator
  | /* empty */
  { $$ = NULL; }
  ;

optional_stars: optional_stars TOK_STAR
  | /* empty */
  { $$ = NULL; }
  ;

magic_identifiers1: TOK_MODIFIER | TOK_GLOBAL ;

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
  | TOK_CLASS
  | TOK_BREAK
  | TOK_CASE
  | TOK_CONSTANT
  | TOK_CONTINUE
  | TOK_DEFAULT
  | TOK_RETURN
  | TOK_SWITCH
  | TOK_NAMESPACE
  | TOK_PREDEF
  | TOK_CATCH		/* */
  | TOK_GAUGE		/* */
  | TOK_LAMBDA		/* */
  | TOK_SSCANF		/* */
  | TOK_TYPEOF		/* */
  ;

magic_identifiers: magic_identifiers1 | magic_identifiers2 | magic_identifiers3 ;

magic_identifier: TOK_IDENTIFIER 
  | magic_identifiers
  ;

number_or_maxint: /* empty */
  {
    $$ = INT_CONSTANT(0x7fffffff);
  }
  | TOK_NUMBER
  | TOK_MINUS TOK_NUMBER
  {
    free_sval(context, $1);
    $$ = NEGATE($2);
  }
  ;

number_or_minint: /* empty */
  {
    $$ = INT_CONSTANT(-0x80000000);
  }
  | TOK_NUMBER
  | TOK_MINUS TOK_NUMBER
  ;

expected_dot_dot: TOK_DOT_DOT
  { free_sval(context, $1); }
  | TOK_DOT_DOT_DOT
  { yyerror("Expected '..', got '...'."); free_sval(context, $1); }
  ;

expected_colon: TOK_COLON
  { free_sval(context, $1); }
  | TOK_SEMICOLON
  { yyerror("Expected ':', got ';'."); free_sval(context, $1); }
  | TOK_RBRACE
  { yyerror("Expected ':', got '}'."); free_sval(context, $1); }
  ;

optional_comma_expr: /* empty */
  { $$ = NULL; }
  | comma_expr
  ;

optional_comma: /* empty */
  | TOK_COMMA
  { free_sval(context, $1); }
  ;

optional_block: TOK_SEMICOLON /* empty */
  | TOK_LBRACE statements TOK_RBRACE
  {
    free_sval(context, $1);
    free_sval(context, $3);
    $$ = $2;
  }
  ;

comma_expr_or_zero: /* empty */
  {
    $$ = INT_CONSTANT(0);
  }
  | comma_expr
  ;

comma_expr_or_maxint: /* empty */
  {
    $$ = INT_CONSTANT(0x7fffffff);
  }
  | comma_expr
  ;

string: TOK_STRING 
  | string TOK_STRING
  {
    $$ = STR_CONCAT($1, $2);
  }
  ;

bad_identifier: bad_expr_ident
  | TOK_ARRAY_ID
  | TOK_CLASS
  | TOK_ENUM
  | TOK_FLOAT_ID
  | TOK_FUNCTION_ID
  | TOK_INT_ID
  | TOK_MAPPING_ID
  | TOK_MIXED_ID
  | TOK_MULTISET_ID
  | TOK_OBJECT_ID
  | TOK_PROGRAM_ID
  | TOK_STRING_ID
  | TOK_TYPEDEF
  | TOK_VOID_ID
  ;

bad_expr_ident: TOK_PREDEF
  | TOK_GLOBAL
  | TOK_DO
  | TOK_ELSE
  | TOK_RETURN
  | TOK_NAMESPACE
  | TOK_CATCH
  | TOK_GAUGE
  | TOK_LAMBDA
  | TOK_SSCANF
  | TOK_SWITCH
  | TOK_TYPEOF
  | TOK_BREAK
  | TOK_CASE
  | TOK_CONTINUE
  | TOK_DEFAULT
  | TOK_FOR
  | TOK_FOREACH
  | TOK_IF
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

