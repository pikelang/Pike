/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef OPCODES_H
#define OPCODES_H

#include "pikecode.h"

/* To provide the cast and sscanf declarations for compatibility. */
#include "operators.h"
#include "sscanf.h"

struct keyword
{
  const char *word;
  const int token;
  const unsigned int flags;
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

#define I_RETURN	2048	/* Opcode may return to the previous frame. */

/* Argument type information for disassembly. */
#define I_ARG_T_INT	0	/* Argument is integer. */
#define I_ARG_T_STRING	0x1000	/* Argument is string index. */
#define I_ARG_T_LOCAL	0x2000	/* Argument is local number. */
#define I_ARG_T_GLOBAL	0x3000	/* Argument is global number. */
#define I_ARG_T_RTYPE	0x4000	/* Argument is runtime type. */
#define I_ARG_T_CONST	0x5000	/* Argument is constant index. */
#define I_ARG_T_MASK	0x7000	/* Mask for I_ARG_T_* above. */

#define I_ARG2_T_SHIFT	3	/* Bits to shift left for arg2 info. */
#define I_ARG2_T_INT	0	/* Argument is integer. */
#define I_ARG2_T_STRING	0x08000	/* Argument is string index. */
#define I_ARG2_T_LOCAL	0x10000	/* Argument is local number. */
#define I_ARG2_T_GLOBAL	0x18000	/* Argument is global number. */
#define I_ARG2_T_RTYPE	0x20000	/* Argument is runtime type. */
#define I_ARG2_T_CONST	0x28000	/* Argument is constant index. */
#define I_ARG2_T_MASK	0x38000	/* Mask for I_ARG_T_* above. */

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
  const char *name;
  int flags;
#ifdef PIKE_USE_MACHINE_CODE
  void *address;
#endif
};

#ifdef PIKE_DEBUG
extern unsigned long pike_instrs_compiles[];
#define ADD_COMPILED(X) pike_instrs_compiles[(X)-F_OFFSET]++
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

extern const struct instr instrs[];
#ifdef PIKE_USE_MACHINE_CODE
extern size_t instrs_checksum;
#endif /* PIKE_USE_MACHINE_CODE */

#include "enum_Pike_opcodes.h"

#ifdef PIKE_DEBUG
const char *low_get_f_name(int n,struct program *p);
const char *get_f_name(int n);
#define get_opcode_name(n) get_f_name(n + F_OFFSET)
#endif
const char *get_token_name(int n);
void init_opcodes(void);
void exit_opcodes(void);

#endif	/* !OPCODES_H */
