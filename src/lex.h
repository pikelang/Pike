/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef LEX_H
#define LEX_H

#include <stdio.h>

struct keyword
{
  char *word;
  int token;
};

struct instr
{
  char *name;
};

struct hash_entry;
struct inputstate;
struct hash_table;

extern struct hash_table *defines;
extern struct lpc_string *current_file;
extern INT32 current_line;
extern INT32 old_line;
extern INT32 total_lines;
extern INT32 nexpands;
extern int pragma_all_inline;          /* inline all possible inlines */
extern struct inputstate *istate;

extern struct lpc_predef_s * lpc_predefs;

/* Prototypes begin here */
struct lpc_predef_s;
void exit_lex();
struct reserved;
void init_lex();
void free_reswords();
char *low_get_f_name(int n,struct program *p);
char *get_f_name(int n);
struct inputstate;
struct define;
void free_one_define(struct hash_entry *h);
void start_new_file(int fd,struct lpc_string *filename);
void start_new_string(char *s,INT32 len,struct lpc_string *name);
void end_new_file();
void add_predefine(char *s);
/* Prototypes end here */

#endif
