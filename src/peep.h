/*
 * $Id: peep.h,v 1.7 2000/12/01 01:13:46 hubbe Exp $
 */
#ifndef PEEP_H
#define PEEP_H

#include "dynamic_buffer.h"
extern dynamic_buffer instrbuf;

struct p_instr_s
{
  short opcode;
  short line;
  struct pike_string *file;
  INT32 arg;
  INT32 arg2;
};

typedef struct p_instr_s p_instr;

/* Prototypes begin here */
void init_bytecode(void);
void exit_bytecode(void);
ptrdiff_t insert_opcode2(unsigned int f,
			 INT32 b,
			 INT32 c,
			 INT32 current_line,
			 struct pike_string *current_file);
ptrdiff_t insert_opcode1(unsigned int f,
			 INT32 b,
			 INT32 current_line,
			 struct pike_string *current_file);
ptrdiff_t insert_opcode0(int f,int current_line, struct pike_string *current_file);
void update_arg(int instr,INT32 arg);
void ins_f_byte(unsigned int b);
void assemble(void);
ptrdiff_t insopt2(int f, INT32 a, INT32 b, int cl, struct pike_string *cf);
ptrdiff_t insopt1(int f, INT32 a, int cl, struct pike_string *cf);
ptrdiff_t insopt0(int f, int cl, struct pike_string *cf);
/* Prototypes end here */

#endif
