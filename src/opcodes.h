/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: opcodes.h,v 1.8 2000/12/01 03:19:01 hubbe Exp $
 */
#ifndef OPCODES_H
#define OPCODES_H

/* Opcodes */

/*
 * These values are used by the stack machine, and can not be directly
 * called from Pike.
 */
#define OPCODE0(X,Y) X,
#define OPCODE1(X,Y) X,
#define OPCODE2(X,Y) X,
#define OPCODE0_TAIL(X,Y) X,
#define OPCODE1_TAIL(X,Y) X,
#define OPCODE2_TAIL(X,Y) X,
#define OPCODE0_JUMP(X,Y) X,
#define OPCODE1_JUMP(X,Y) X,
#define OPCODE2_JUMP(X,Y) X,
#define OPCODE0_TAILJUMP(X,Y) X,
#define OPCODE1_TAILJUMP(X,Y) X,
#define OPCODE2_TAILJUMP(X,Y) X,

enum Pike_opcodes
{
  F_OFFSET = 257,
  F_PREFIX_256,
  F_PREFIX_512,
  F_PREFIX_768,
  F_PREFIX_1024,
  F_PREFIX_CHARX256,
  F_PREFIX_WORDX256,
  F_PREFIX_24BITX256,
  F_PREFIX2_256,
  F_PREFIX2_512,
  F_PREFIX2_768,
  F_PREFIX2_1024,
  F_PREFIX2_CHARX256,
  F_PREFIX2_WORDX256,
  F_PREFIX2_24BITX256,
  F_APPLY,
  F_APPLY_AND_POP,
  F_APPLY_AND_RETURN,

  F_BRANCH_AND_POP_WHEN_ZERO,
  F_BRANCH_AND_POP_WHEN_NON_ZERO,
  F_BRANCH_WHEN_LT,
  F_BRANCH_WHEN_GT,
  F_BRANCH_WHEN_LE,
  F_BRANCH_WHEN_GE,
  F_BRANCH_WHEN_EQ,
  F_BRANCH_WHEN_NE,
  F_BRANCH_IF_NOT_LOCAL,
  F_INC_LOOP,
  F_DEC_LOOP,
  F_INC_NEQ_LOOP,
  F_DEC_NEQ_LOOP,

  F_INDEX,
  F_POS_INT_INDEX,
  F_NEG_INT_INDEX,

/*
 * These are the predefined functions that can be accessed from Pike.
 */
  F_RETURN,
  F_DUMB_RETURN,
  F_RETURN_0,
  F_RETURN_1,
  F_RETURN_LOCAL,
  F_RETURN_IF_TRUE,

  F_LT,
  F_GT,
  F_EQ,
  F_GE,
  F_LE,
  F_NE,
  F_LAND,
  F_LOR,
  F_EQ_OR,
  F_EQ_AND,

  F_CATCH,
  F_FOREACH,

  F_CALL_FUNCTION,
  F_CALL_FUNCTION_AND_RETURN,

#include "interpret_protos.h"
/*
 * These are token values that needn't have an associated code for the
 * compiled file
 */
  F_MAX_OPCODE,

  F_ADD_EQ,
  F_AND_EQ,
  F_ARG_LIST,
  F_COMMA_EXPR,
  F_BREAK,
  F_CASE,
  F_CONTINUE,
  F_DEFAULT,
  F_DIV_EQ,
  F_DO,
  F_EFUN_CALL,
  F_FOR,
  F_IDENTIFIER,
  F_LSH_EQ,
  F_LVALUE_LIST,
  F_MOD_EQ,
  F_MULT_EQ,
  F_OR_EQ,
  F_RSH_EQ,
  F_SUB_EQ,
  F_VAL_LVAL,
  F_XOR_EQ,
  F_NOP,

  F_ALIGN,
  F_POINTER,
  F_LABEL,
  F_DATA,
  F_START_FUNCTION,
  F_BYTE,
  F_NOTREACHED,
  F_MAX_INSTR
};

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE0_TAIL
#undef OPCODE1_TAIL
#undef OPCODE2_TAIL
#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP


/* Prototypes begin here */
void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind);
void o_index(void);
void o_cast(struct pike_string *type, INT32 run_time_type);
void f_cast(void);
void o_sscanf(INT32 args);
void f_sscanf(INT32 args);
/* Prototypes end here */

#endif
