/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: opcodes.h,v 1.39 2004/03/12 21:56:52 mast Exp $
*/

#ifndef OPCODES_H
#define OPCODES_H

#include "pikecode.h"

/* To provide the cast and sscanf declarations for compatibility. */
#include "operators.h"
#include "sscanf.h"

struct keyword
{
  char *word;
  int token;
  int flags;
#ifdef PIKE_USE_MACHINE_CODE
  void *address;
#endif
};

/*
 * Instruction flags
 *
 * Flags used to classify the instructions.
 *
 * Note that branches that take arguments use an immediately
 * following F_POINTER instruction to hold the destination
 * address.
 */
#define I_HASARG	1	/* Instruction has a parameter. */
#define I_POINTER	2	/* arg is a label number. */
#define I_JUMP		4	/* Instruction performs a jump. */
#define I__DATA		8	/* Instruction is raw data (data, byte)*/
#define I_HASARG2	16	/* Instruction has a second parameter. */
#define I_HASPOINTER	32	/* Instruction is followed by a F_POINTER. */
#define I_BRANCH	128	/* Opcode either jumps to the address
				 * given by I_POINTER/I_HASPOINTER or
				 * continues. */
/* The following are useful for the code generator.
 * Note that they apply to the change of state as seen
 * by the immediately following instruction.
 */
#define I_UPDATE_SP	256	/* Opcode modifies Pike_sp */
#define I_UPDATE_FP	512	/* Opcode modifies Pike_fp */
#define I_UPDATE_M_SP	1024	/* Opcode modifies Pike_mark_sp */

/* Convenience variants */
#define I_TWO_ARGS	(I_HASARG | I_HASARG2)
#define I_DATA		(I_HASARG | I__DATA)
#define I_ISPOINTER	(I_HASARG | I_POINTER)	/* Only F_POINTER */
#define I_ISJUMP	(I_JUMP)
#define I_ISJUMPARG	(I_HASARG | I_JUMP)
#define I_ISJUMPARGS	(I_TWO_ARGS | I_JUMP)
#define I_ISPTRJUMP	(I_HASARG | I_POINTER | I_JUMP)
#define I_ISPTRJUMPARG	(I_HASARG | I_HASPOINTER | I_JUMP)
#define I_ISPTRJUMPARGS	(I_TWO_ARGS | I_HASPOINTER | I_JUMP)
#define I_ISBRANCH	(I_HASARG | I_POINTER | I_JUMP | I_BRANCH)
#define I_ISBRANCHARG	(I_HASARG | I_HASPOINTER | I_JUMP | I_BRANCH)
#define I_ISBRANCHARGS	(I_TWO_ARGS | I_HASPOINTER | I_JUMP | I_BRANCH)
#define I_IS_MASK	(I_TWO_ARGS | I_POINTER | I_HASPOINTER | I_JUMP)

#define I_UPDATE_ALL	(I_UPDATE_SP | I_UPDATE_FP | I_UPDATE_M_SP)

/* Valid masked flags:
 *
 * 0			Generic instruction without immediate arguments.
 * I_HAS_ARG		Generic instruction with one argument.
 * I_TWO_ARGS		Generic instruction with two arguments.
 * I_DATA		Raw data (F_BYTE or F_DATA).
 * I_ISPOINTER		Raw jump address (F_POINTER).
 * I_ISJUMP		Jump instruction without immediate arguments.
 * I_ISJUMPARG		Jump instruction with one argument.
 * I_ISJUMPARGS		Jump instruction with two arguments.
 * I_ISPTRJUMP		Jump instruction with pointer.
 * I_ISPTRJUMPARG	Jump instruction with pointer and one argument.
 * I_ISPTRJUMPARGS	Jump instruction with pointer and two arguments.
 */

#ifdef PIKE_DEBUG
#define INSTR_PROFILING
#endif


struct instr
{
#ifdef PIKE_DEBUG
  long compiles;
#endif
  int flags;
  char *name;
#ifdef PIKE_USE_MACHINE_CODE
  void *address;
#endif
};

#ifdef PIKE_DEBUG
#define ADD_COMPILED(X) instrs[(X)-F_OFFSET].compiles++
#ifdef INSTR_PROFILING
extern void add_runned(PIKE_INSTR_T);
#define ADD_RUNNED(X) add_runned(X)
#else
#define ADD_RUNNED(X)
#endif
#else
#define ADD_COMPILED(X)
#define ADD_RUNNED(X)
#endif

#ifndef STRUCT_HASH_ENTRY_DECLARED
#define STRUCT_HASH_ENTRY_DECLARED
struct hash_entry;
#endif

#ifndef STRUCT_HASH_TABLE_DECLARED
#define STRUCT_HASH_TABLE_DECLARED
struct hash_table;
#endif

extern struct instr instrs[];
#ifdef PIKE_USE_MACHINE_CODE
extern size_t instrs_checksum;
#endif /* PIKE_USE_MACHINE_CODE */

/* Opcode enum */

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
#define OPCODE0_PTRJUMP(X,Y,F) X,
#define OPCODE1_PTRJUMP(X,Y,F) X,
#define OPCODE2_PTRJUMP(X,Y,F) X,
#define OPCODE0_TAILPTRJUMP(X,Y,F) X,
#define OPCODE1_TAILPTRJUMP(X,Y,F) X,
#define OPCODE2_TAILPTRJUMP(X,Y,F) X,
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
#undef OPCODE0_PTRJUMP
#undef OPCODE1_PTRJUMP
#undef OPCODE2_PTRJUMP
#undef OPCODE0_TAILPTRJUMP
#undef OPCODE1_TAILPTRJUMP
#undef OPCODE2_TAILPTRJUMP
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
#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP
#undef OPCODE0_ALIAS
#undef OPCODE1_ALIAS
#undef OPCODE2_ALIAS

char *low_get_f_name(int n,struct program *p);
char *get_f_name(int n);
#ifdef HAVE_COMPUTED_GOTO
char *get_opcode_name(PIKE_INSTR_T n);
#else /* !HAVE_COMPUTED_GOTO */
#define get_opcode_name(n) get_f_name(n + F_OFFSET)
#endif /* HAVE_COMPUTED_GOTO */
char *get_token_name(int n);
void init_opcodes(void);
void exit_opcodes(void);

#endif	/* !OPCODES_H */
