/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PEEP_H
#define PEEP_H

#include "buffer.h"
extern struct byte_buffer instrbuf;

struct p_instr_s
{
  short opcode;
  INT_TYPE line;
  struct pike_string *file;
  INT32 arg;
  INT32 arg2;
};

typedef struct p_instr_s p_instr;

/* Prototypes begin here */
void init_bytecode(void);
void exit_bytecode(void);
ptrdiff_t insert_opcode2(unsigned int f,
			 INT32 a,
			 INT32 b,
			 INT_TYPE current_line,
			 struct pike_string *current_file);
ptrdiff_t insert_opcode1(unsigned int f,
			 INT32 a,
			 INT_TYPE current_line,
			 struct pike_string *current_file);
ptrdiff_t insert_opcode0(int f, INT_TYPE current_line,
			 struct pike_string *current_file);
void update_arg(int instr,INT32 arg);
INT32 assemble(struct string_builder *disassembly, int store_linenumbers);
/* Prototypes end here */

#endif
