/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: opcodes.h,v 1.33 2004/05/11 15:25:26 grubba Exp $
*/

#ifndef OPCODES_H
#define OPCODES_H

/* Opcodes */

#define OPCODE0(X,Y,F) X,
#define OPCODE1(X,Y,F) X,
#define OPCODE2(X,Y,F) X,
#define OPCODE0_TAIL(X,Y,F) X,
#define OPCODE1_TAIL(X,Y,F) X,
#define OPCODE2_TAIL(X,Y,F) X,
#define OPCODE0_JUMP(X,Y,F) X,
#define OPCODE1_JUMP(X,Y,F) X,
#define OPCODE2_JUMP(X,Y,F) X,
#define OPCODE0_TAILJUMP(X,Y,F) X,
#define OPCODE1_TAILJUMP(X,Y,F) X,
#define OPCODE2_TAILJUMP(X,Y,F) X,
#define OPCODE0_RETURN(X,Y,F) X,
#define OPCODE1_RETURN(X,Y,F) X,
#define OPCODE2_RETURN(X,Y,F) X,
#define OPCODE0_TAILRETURN(X,Y,F) X,
#define OPCODE1_TAILRETURN(X,Y,F) X,
#define OPCODE2_TAILRETURN(X,Y,F) X,
#define OPCODE0_BRANCH(X,Y,F) X,
#define OPCODE1_BRANCH(X,Y,F) X,
#define OPCODE2_BRANCH(X,Y,F) X,
#define OPCODE0_TAILBRANCH(X,Y,F) X,
#define OPCODE1_TAILBRANCH(X,Y,F) X,
#define OPCODE2_TAILBRANCH(X,Y,F) X,
#define OPCODE0_ALIAS(X,Y,F,A) X,
#define OPCODE1_ALIAS(X,Y,F,A) X,
#define OPCODE2_ALIAS(X,Y,F,A) X,

enum Pike_opcodes
{
  /*
   * These values are used by the stack machine, and can not be directly
   * called from Pike.
   */
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

  /*
   * These are the predefined functions that can be accessed from Pike.
   */

#include "interpret_protos.h"

  /* Used to mark an entry point from eval_instruction(). */
  F_ENTRY,

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
  F_CASE_RANGE,
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
  F_NORMAL_STMT_LABEL,
  F_CUSTOM_STMT_LABEL,
  F_DATA,
  F_START_FUNCTION,
  F_BYTE,
  F_NOTREACHED,
  F_AUTO_MAP_MARKER,
  F_AUTO_MAP,

  /* Alias for F_RETURN, but cannot be optimized into a tail recursion call */
  F_VOLATILE_RETURN,

  F_MAX_INSTR,

  /* These are only used for dumping. */
  F_FILENAME,
  F_LINE,
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
#undef OPCODE0_RETURN
#undef OPCODE1_RETURN
#undef OPCODE2_RETURN
#undef OPCODE0_TAILRETURN
#undef OPCODE1_TAILRETURN
#undef OPCODE2_TAILRETURN
#undef OPCODE0_BRANCH
#undef OPCODE1_BRANCH
#undef OPCODE2_BRANCH
#undef OPCODE0_TAILBRANCH
#undef OPCODE1_TAILBRANCH
#undef OPCODE2_TAILBRANCH
#undef OPCODE0_ALIAS
#undef OPCODE1_ALIAS
#undef OPCODE2_ALIAS


/* Prototypes begin here */
void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind);
void o_index(void);
void o_cast_to_int(void);
void o_cast_to_string(void);
void o_cast(struct pike_type *type, INT32 run_time_type);
PMOD_EXPORT void f_cast(void);
void o_sscanf(INT32 args);
PMOD_EXPORT void f_sscanf(INT32 args);
/* Prototypes end here */

#endif
