/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: lex.h,v 1.14 2000/04/18 17:23:35 hubbe Exp $
 */
#ifndef LEX_H
#define LEX_H

#include <stdio.h>

struct keyword
{
  char *word;
  int token;
  int flags;
};

#define I_HASARG 1
#define I_POINTER 2
#define I_JUMP 4
#define I_ISPOINTER 3
#define I_ISJUMP 7
#define I_DATA 9
#define I_ACCUMULATOR 16
#define I_TWO_ARGS 17

#ifdef PIKE_DEBUG
#define INSTR_PROFILING
#endif

#ifdef INSTR_PROFILING
extern int last_instruction;
#endif

struct instr
{
#ifdef PIKE_DEBUG
#ifdef INSTR_PROFILING
  long reruns[256];
#endif
  long runs;
  long compiles;
#endif
  int flags;
  char *name;
};

#ifdef PIKE_DEBUG
#define ADD_COMPILED(X) instrs[(X)-F_OFFSET].compiles++
#ifdef INSTR_PROFILING
#define ADD_RUNNED(X) do { int _x=(X)-F_OFFSET; instrs[last_instruction].reruns[_x]++; instrs[last_instruction=_x].runs++; } while(0)
#else
#define ADD_RUNNED(X) instrs[(X)-F_OFFSET].runs++
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

/* Prototypes begin here */
void exit_lex(void);
struct reserved;
void init_lex(void);
char *low_get_f_name(int n,struct program *p);
char *get_f_name(int n);
char *get_token_name(int n);

int yylex0(YYSTYPE *);
int yylex1(YYSTYPE *);
int yylex2(YYSTYPE *);

/* Prototypes end here */

#endif
