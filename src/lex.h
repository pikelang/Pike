/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef LEX_H
#define LEX_H

#include <stdio.h>

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
#define I_JUMP		4	/* Instruction performs a jump to its pointer. */
#define I__DATA		8	/* Instruction is raw data (data, byte)*/
#define I_HASARG2	16	/* Instruction has a second parameter. */
#define I_HASPOINTER	32	/* Instruction is followed by a F_POINTER. */
#define I_PC_AT_NEXT	64	/* Opcode needs PC to be updated. */
#define I_BRANCH	128	/* Opcode is a clean branch instruction. */

#define I_TWO_ARGS	(I_HASARG | I_HASARG2)
#define I_DATA		(I_HASARG | I__DATA)
#define I_ISPOINTER	(I_HASARG | I_POINTER)	/* Only F_POINTER */
#define I_ISJUMP	(I_HASARG | I_POINTER | I_JUMP)
#define I_ISJUMPARG	(I_HASARG | I_HASPOINTER | I_JUMP)
#define I_ISJUMPARGS	(I_TWO_ARGS | I_HASPOINTER | I_JUMP)
#define I_ISBRANCH	(I_HASARG | I_POINTER | I_JUMP | I_BRANCH)
#define I_ISBRANCHARG	(I_HASARG | I_HASPOINTER | I_JUMP | I_BRANCH)
#define I_ISBRANCHARGS	(I_TWO_ARGS | I_HASPOINTER | I_JUMP | I_BRANCH)
#define I_IS_MASK	(I_TWO_ARGS | I_POINTER | I_HASPOINTER | I_JUMP)

/* Valid masked flags:
 *
 * 0		Generic instruction without immediate arguments.
 * I_HAS_ARG	Generic instruction with one argument.
 * I_TWO_ARGS	Generic instruction with two arguments.
 * I_DATA	Raw data (F_BYTE or F_DATA).
 * I_ISPOINTER	Raw jump address (F_POINTER).
 * I_ISJUMP	Branch instruction or F_CATCH.
 * I_ISJUMPARG	Branch instruction with one argument.
 * I_ISJUMPARGS	Branch instruction with two arguments.
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
extern void add_runned(PIKE_OPCODE_T);
#ifdef HAVE_COMPUTED_GOTO
#define ADD_RUNNED(X)	add_runned(X)
#else /* !HAVE_COMPUTED_GOTO */
#define ADD_RUNNED(X) add_runned((X)-F_OFFSET)
#endif /* HAVE_COMPUTED_GOTO */
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

#define NEW_LEX

struct lex
{
  char *pos;
  char *end;
  INT32 current_line;
  INT32 pragmas;
  struct pike_string *current_file;
  int (*current_lexer)(YYSTYPE *);
};

extern struct lex lex;
extern struct instr instrs[];
extern unsigned INT32 instrs_checksum;

/* Prototypes begin here */
void exit_lex(void);
struct reserved;
void init_lex(void);
char *low_get_f_name(int n,struct program *p);
char *get_f_name(int n);
#ifdef HAVE_COMPUTED_GOTO
char *get_opcode_name(PIKE_OPCODE_T n);
#else /* !HAVE_COMPUTED_GOTO */
#define get_opcode_name(n)	get_f_name(n)
#endif /* HAVE_COMPUTED_GOTO */
char *get_token_name(int n);

int yylex0(YYSTYPE *);
int yylex1(YYSTYPE *);
int yylex2(YYSTYPE *);

/* Prototypes end here */

#endif
