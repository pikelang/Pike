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

/* Use the Generalized LR-parser */
%glr_parser

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
%token TOK_META
%type <sval> TOK_META

%token TOK_DO
%type <sval> TOK_DO
%token TOK_ELSE
%type <sval> TOK_ELSE
%token TOK_FOR
%type <sval> TOK_FOR
%token TOK_FOREACH
%type <sval> TOK_FOREACH
%token TOK_IF
%type <sval> TOK_IF
%token TOK_SWITCH
%type <sval> TOK_SWITCH
%token TOK_WHILE
%type <sval> TOK_WHILE

/* Generic identifier */
/* NOTE: TOK_IDENTIFIER *MUST* be the last of the tokens! */
%token TOK_IDENTIFIER
%type <sval> TOK_IDENTIFIER

/* Assocciativity */

%left TOK_COMMA
%nonassoc TOK_DOT_DOT TOK_DOT_DOT_DOT
%right TOK_ASSIGN TOK_AND_EQ TOK_OR_EQ TOK_XOR_EQ TOK_LSH_EQ TOK_RSH_EQ TOK_ADD_EQ TOK_SUB_EQ TOK_MULT_EQ TOK_MOD_EQ TOK_DIV_EQ
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
%left TOK_ARROW
%left TOK_DOT TOK_COLON_COLON

/* Node types. */
%type <sval> all
%type <sval> statement_list
%type <sval> statement
%type <sval> meta_statement
%type <sval> block_or_semi
%type <sval> opt_colon
%type <sval> modifier_list
%type <sval> modifier
%type <sval> opt_arg_list
%type <sval> opt_var_list
%type <sval> var_list
%type <sval> single_var_decl
%type <sval> expr
%type <sval> no_par_expr_list
%type <sval> optional_comma_expr
%type <sval> comma_expr
%type <sval> par_expr
%type <sval> no_par_expr
%type <sval> var_decl_expr
%type <sval> var_name_list
%type <sval> var_spec
%type <sval> assign_op
%type <sval> bin_op
%type <sval> un_op
%type <sval> post_op
%type <sval> value
%type <sval> idents
%type <sval> string_constant

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

    /* Keywords: */
    "class", "enum", "typedef", "namespace",
    "constant",

    /* Keywords:Special forms */
    "do",
    "else",
    "for",
    "foreach",
    "if",
    "switch",
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

/* Main rule */
decls: /* empty */
  { $$ = NULL; }
  | decls decl
  { $$ = DECL_LIST($1, $2); }
  ;

decl: semicolon
  { $$ = NULL; }
  | modifier_list raw_decl
  { $$ = MODIFIER_BLOCK($1, $3); }
  ;

raw_decl: lbrace decls rbrace
  { $$ = DECL_BLOCK($2); }
  | TOK_CLASS identifier lbrace decls rbrace
  { $$ = META_DECL($1, $2, $4); }
  | TOK_CONSTANT initializer_list semicolon
  { $$ = META_DECL($1, NULL, $2); }
  | TOK_NAMESPACE program_spec opt_rename semicolon
  { $$ = META_DECL($1, $3, $2); }
  | TOK_ENUM opt_name lbrace enum_list rbrace
  { $$ = META_DECL($1, $2, $4); }
  | TOK_TYPEDEF type identifier semicolon
  { $$ = META_DECL($1, $3, $2); }
  | type identifier lparen arguments rparen semicolon
  { $$ = PROTOTYPE($2, $1, $4); }
  | type identifier lparen arguments rparen
    lbrace statements rbrace
  { $$ = FUN_DECL($2, $1, $4, $7); }
  | type variable_list semicolon
  { $$ = VAR_DECL($1, $2); }
  ;

/* Helpers */
semicolon: TOK_SEMICOLON { free_sval(context, $1); } ;
colon: TOK_COLON { free_sval(context, $1); } ;

lbrace: TOK_LBRACE { free_sval(context, $1); } ;
rbrace: TOK_RBRACE { free_sval(context, $1); } ;

lparen: TOK_LPAREN { free_sval(context, $1); } ;
rparen: TOK_RPAREN { free_sval(context, $1); } ;

lbracket: TOK_LBRACKET { free_sval(context, $1); } ;
rbracket: TOK_RBRACKET { free_sval(context, $1); } ;

lt: TOK_LT { free_sval(context, $1); } ;
gt: TOK_GT { free_sval(context, $1); } ;

lmultiset: lparen lt;
rmultiset: gt rparen;

larray: lparen lbrace;
rarray: rbrace rparen;

lmapping: lparen lbracket;
rmapping: rbracket rparen;

rename: colon identifier { $$ = $2; };

/* Optionals */
opt_rename: /* empty */
  { $$ = NULL; }
  | rename
  ;


statement_list: /* empty */ { $$ = NULL; }
  | statement_list statement { $$ = LIST($1, $2) }
  ;

statement: modifier_list block_or_semi { $$ = STATEMENT($1, $2); }
  | TOK_IF TOK_LPAREN comma_expr TOK_RPAREN statement TOK_ELSE statement
  | TOK_IF TOK_LPAREN comma_expr TOK_RPAREN statement
  | TOK_DO statement TOK_WHILE TOK_LPAREN comma_expr TOK_RPAREN TOK_SEMICOLON
  | TOK_WHILE statement
  | TOK_FOR TOK_LPAREN optional_comma_expr TOK_SEMICOLON optional_comma_expr
    TOK_SEMICOLON optional_comma_expr TOK_RPAREN statement
  | TOK_FOREACH TOK_LPAREN comma_expr TOK_SEMICOLON optional_comma_expr
    TOK_SEMICOLON optional_comma_expr TOK_RPAREN statement
  | TOK_FOREACH TOK_LPAREN expr TOK_COMMA expr TOK_RPAREN statement
  | TOK_SWITCH TOK_LPAREN comma_expr TOK_RPAREN statement
  ;

meta_statement: modifier_list TOK_META idents opt_arg_list block_or_semi
  | modifier_list TOK_META expr opt_colon TOK_IDENTIFIER TOK_SEMICOLON
  ;

block_or_semi:
    TOK_LBRACE statement_list TOK_RBRACE { $$ = $2; }
  | TOK_SEMICOLON { $$ = NULL; }
  ;

opt_colon: /* empty */
  { $$ = NULL; }
  | TOK_COLON
  ;

modifier_list: /* empty */
  { $$ = NULL; }
  | modifier_list modifier
  ;

modifier: TOK_MODIFIER
  | TOK_MODIFIER TOK_LPAREN comma_expr TOK_RPAREN
  ;

opt_arg_list: /* empty */
  { $$ = NULL; }
  | TOK_LPAREN opt_var_list TOK_RPAREN
  ;

opt_var_list: /* empty */
  { $$= NULL; }
  | var_list
  ;

var_list: single_var_decl
  | var_list TOK_COMMA single_var_decl
  ;

single_var_decl: modifier_list no_par_expr var_spec ;

/*
 * Meta-meta programs:
 *
 * Tagged:
 *   Global scope:
 *     constant
 *     class
 *     enum
 *     typedef
 *     inherit
 *     import
 *
 *   Function scope:
 *     break
 *     continue
 *     return
 *     (goto)
 *
 * Untagged:
 *   Global scope:
 *     function definition
 *     variable definition
 *     modifier block
 *   Function scope:
 *     expression
 */

/*
 * Meta programs:
 *
 * Function like:
 *   sscanf
 *   catch
 *   gauge
 *   typeof
 *
 * Modifier like:
 *   local
 *   static
 *   private
 *   public
 *   inline
 *   global
 *   (shared)
 * 
 * Special:
 *   auto map
 */

/*
 * Not handled yet:
 *
 * Labels:
 *   foo:
 *   case ... :
 *   default:
 *
 * Symbol lookup renaming:
 *   : Foo
 */

optional_comma_expr: /* empty */
  { $$ = NULL; }
  | comma_expr
  ;

comma_expr: expr
  | comma_expr TOK_COMMA expr
  ;

expr: par_expr
  | no_par_expr
  | var_decl_expr
  ;

par_expr: TOK_LPAREN comma_expr TOK_RPAREN
  | TOK_LBRACKET comma_expr TOK_RBRACKET
  ;

var_decl_expr: no_par_expr var_name_list ;

var_name_list: var_spec
  | var_name_list TOK_COMMA var_spec
  ;

var_spec: TOK_IDENTIFIER
  | TOK_IDENTIFIER TOK_ASSIGN expr
  ;

no_par_expr: value
  | TOK_SPLICE expr
  { $$ = SPLICE($2); }
  | expr assign_op expr
  { $$ = ASSIGN($1, $2, $3); }
  | expr TOK_COND expr TOK_COLON expr
  | expr bin_op expr
  { $$ = BIN_OP($1, $2, $3); }
  | un_op expr
  { $$ = UN_OP($1, $2); }
  | expr post_op
  { $$ = POST_OP($1, $2); }
  | idents
  ;

no_par_expr_list: /* empty */ { $$ = NULL; }
  | no_par_expr_list no_par_expr { $$ = LIST($1, $2); }
  ;

assign_op: TOK_ASSIGN
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

bin_op: TOK_COMMA
  | TOK_DOT_DOT
  | TOK_DOT_DOT_DOT
  | TOK_LOR
  | TOK_LAND
  | TOK_OR
  | TOK_XOR
  | TOK_AND
  | TOK_EQ
  | TOK_NE
  | TOK_GT
  | TOK_GE
  | TOK_LT
  | TOK_LE
  | TOK_LSH
  | TOK_RSH
  | TOK_PLUS
  | TOK_MINUS
  | TOK_STAR
  | TOK_MOD
  | TOK_DIV
  | TOK_ARROW
  | TOK_DOT
  | TOK_COLON_COLON
  ;

un_op: TOK_INC
  | TOK_DEC
  | TOK_NOT
  | TOK_INVERT
  | TOK_MINUS
  | TOK_STAR
  | TOK_PLUS
  | TOK_DOT
  ;

post_op: TOK_INC
  | TOK_DEC
  ;

value: string_constant
  | TOK_NUMBER
  | TOK_FLOAT
  | TOK_LPAREN comma_expr TOK_RPAREN
  | TOK_LBRACKET comma_expr TOK_RBRACKET
  | TOK_LPAREN TOK_LT comma_expr TOK_MULTISET_END
  ;
/* Note that ([]) and ({}) are handled indirectly. */

/*
 * Identifier handling
 */

idents: TOK_IDENTIFIER ;

/*
 * Literals
 */

string_constant: TOK_STRING
  | string_constant TOK_PLUS TOK_STRING
  { $$ = STRING_CONCAT($1, $3); free_sval(context, $2); }
  | string_constant TOK_STRING
  { $$ = STRING_CONCAT($1, $2); }
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

