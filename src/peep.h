/*
 * $Id: peep.h,v 1.5 2000/04/20 02:41:45 hubbe Exp $
 */
#ifndef PEEP_H
#define PEEP_H

#include "dynamic_buffer.h"
extern dynamic_buffer instrbuf;

/* Prototypes begin here */
struct p_instr_s;
void init_bytecode(void);
void exit_bytecode(void);
int insert_opcode2(unsigned int f,
		  INT32 b,
		  INT32 c,
		  INT32 current_line,
		  struct pike_string *current_file);
int insert_opcode1(unsigned int f,
		  INT32 b,
		  INT32 current_line,
		  struct pike_string *current_file);
int insert_opcode0(int f,int current_line, struct pike_string *current_file);
void update_arg(int instr,INT32 arg);
void ins_f_byte(unsigned int b);
void assemble(void);
int insopt2(int f, INT32 a, INT32 b, int cl, struct pike_string *cf);
int insopt1(int f, INT32 a, int cl, struct pike_string *cf);
int insopt0(int f, int cl, struct pike_string *cf);
/* Prototypes end here */

#endif
