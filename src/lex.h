/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef LEX_H
#define LEX_H

#include <stdio.h>

struct keyword
{
  char *word;
  int token;
  int hasarg;
};

struct instr
{
#ifdef DEBUG
  long runs;
  long compiles;
#endif
  int hasarg;
  char *name;
};

#ifdef DEBUG
#define ADD_COMPILED(X) instrs[(X)-F_OFFSET].compiles++
#define ADD_RUNNED(X) instrs[(X)-F_OFFSET].runs++
#else
#define ADD_COMPILED(X)
#define ADD_RUNNED(X)
#endif

#ifndef STRUCT_HASH_ENTRY_DECLARED
#define STRUCT_HASH_ENTRY_DECLARED
struct hash_entry;
#endif

#ifndef STRUCT_INPUTSTATE_DECLARED
#define STRUCT_INPUTSTATE_DECLARED
struct inputstate;
#endif

#ifndef STRUCT_HASH_TABLE_DECLARED
#define STRUCT_HASH_TABLE_DECLARED
struct hash_table;
#endif

extern struct instr instrs[];
extern struct hash_table *defines;
extern struct pike_string *current_file;
extern INT32 current_line;
extern INT32 old_line;
extern INT32 total_lines;
extern INT32 nexpands;
extern int pragma_all_inline;          /* inline all possible inlines */
extern struct inputstate *istate;

extern struct pike_predef_s * pike_predefs;

/* Prototypes begin here */
struct pike_predef_s;
void exit_lex();
struct reserved;
void init_lex();
void free_reswords();
char *low_get_f_name(int n,struct program *p);
char *get_f_name(int n);
char *get_token_name(int n);
struct inputstate;
struct define;
void free_one_define(struct hash_entry *h);
void insert_current_line();
void insert_current_file_as_string();
void insert_current_time_as_string();
void insert_current_date_as_string();
void start_new_file(int fd,struct pike_string *filename);
void start_new_string(char *s,INT32 len,struct pike_string *name);
void end_new_file();
void add_predefine(char *s);
/* Prototypes end here */

#endif
