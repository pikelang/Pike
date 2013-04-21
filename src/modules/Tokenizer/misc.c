/*
 * $Id$
 *
 * Misc memory handling stuff.
 *
 * Henrik Grubbström 2002-07-25
 */

#include "global.h"
#include "config.h"

#include "object.h"
#include "interpret.h"

struct svalue *new_sval(struct compiler_context *context, struct svalue *osval)
{
  struct svalue *sval = context->free_sval_list;

  if (!sval) {
    struct sval_block *sb = context->sval_block_list;

    if (!sb || (sb->used_cnt >= SVAL_BLOCK_SIZE)) {
      sb = xalloc(sizeof(struct sval_block));
      sb->used_cnt = 0;
      sb->next = context->sval_block_list;
      context->sval_block_list = sb;
    }
    sval = &sb->svals[sb->used_cnt++];
  } else {
    context->free_sval_list = sval->u.lval;	/* Next free sval */
  }
  assign_svalue_no_free(sval, osval);
  return sval;
}

void free_sval(struct compiler_context *context, struct svalue *sval)
{
  free_svalue(sval);
  sval->type = PIKE_T_INT;
  sval->u.lval = context->free_sval_list;
  context->free_sval_list = sval;
}

/* FIXME: This code isn't safe if free_svalue() or free_object()
 *        can throw errors!
 */
void free_context(struct compiler_context *context)
{
  struct sval_block *sb, *next_sb;

  for (sb = context->sval_block_list; sb; sb = next_sb) {
    int i;

    for (i=0; i < sb->used_cnt; i++) {
      if (sb->svals[i].type != PIKE_T_INT) {
	free_svalue(&sb->svals[i]);
      }
    }
    next_sb = sb->next;
    free(sb);
  }
  context->free_sval_list = NULL;
  context->sval_block_list = NULL;
  if (context->lexer) {
    free_object(context->lexer);
    context->lexer = NULL;
  }
  if (context->handler) {
    free_object(context->handler);
    context->handler = NULL;
  }
  free_svalue(&context->result);
  context->result.type = PIKE_T_INT;
}

/* Error handling */

void tokenizer_yyerror(struct compiler_context *context, char *msg)
{
  push_int(MSG_ERROR);
  push_constant_text("parse");
  push_text(msg);
  apply_low(context->handler, context->handle_report_msg, 3);
  pop_stack();
}
