/*
 * $Id$
 */

@TOP@

@BOTTOM@

#include "svalue.h"

/* NOTE: las.h must be included before the define of lex. */
#include "las.h"

#include "pike_error.h"

/* Struct holding the current compilation context. */
struct compiler_context
{
  struct object *lexer;    /* Object implementing Tokenizer.pike_tokenizer. */
  struct object *handler;  /* Compilation handler object. */
  INT32 lex_value;         /* Reference number for lexer->value(). */
  INT32 lex_next;          /* Reference number for lexer->`+(). */
  INT32 handle_report_msg; /* Reference number for handler->report_msg(). */

  struct svalue result;    /* The resulting parse tree. */

  /* Memory handling stuff. */
  struct svalue *free_sval_list;
  struct sval_block *sval_block_list;
};

#define SVAL_BLOCK_SIZE		1020

struct sval_block
{
  struct sval_block *next;
  INT32 used_cnt;
  struct svalue svals[SVAL_BLOCK_SIZE];
};

/*! @module Tokenizer
 */

/*! @decl constant NOTICE=0
 *!   Informational message.
 */
#define MSG_NOTICE 0
/*! @decl constant WARNING=1
 *!   Warning message.
 */
#define MSG_WARNING 1
/*! @decl constant ERROR=2
 *!   Recoverable error message.
 */
#define MSG_ERROR 2
/*! @decl constant FATAL=3
 *!   Unrecoverable error message.
 */
#define MSG_FATAL 3
/*! @decl typedef int(0..3) SeverityLevel
 *!   Level of severity in reported messages.
 *! @seealso
 *!   @[NOTICE], @[WARNING], @[ERROR], @[FATAL]
 */

/*! @endmodule
 */

/* Prototypes */

/* misc.c */
struct svalue *new_sval(struct compiler_context *context, struct svalue *osval);
void free_sval(struct compiler_context *context, struct svalue *sval);
void free_context(struct compiler_context *context);

void tokenizer_yyerror(struct compiler_context *context, char *msg);

/* Pike.yacc */
int tokenizer_yyparse(void *context);
void init_tokenizer_yyparse(void);
