/*
 * $Id: peep.h,v 1.4 1998/03/28 15:06:27 grubba Exp $
 */
#ifndef PEEP_H
#define PEEP_H

#include "dynamic_buffer.h"
extern dynamic_buffer instrbuf;

/* Prototypes begin here */
struct p_instr_s;
void init_bytecode(void);
void exit_bytecode(void);
int insert_opcode(unsigned int f,
		  INT32 b,
		  INT32 current_line,
		  struct pike_string *current_file);
int insert_opcode2(int f,int current_line, struct pike_string *current_file);
void update_arg(int instr,INT32 arg);
void ins_f_byte(unsigned int b);
void assemble(void);
/* Prototypes end here */

#endif
