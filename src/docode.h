/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef DOCODE_H
#define DOCODE_H

extern int store_linenumbers;
extern int comp_stackp;
extern INT32 comp_stack[COMPILER_STACK_SIZE];

/* Prototypes begin here */
void ins_byte(unsigned char b,int area);
void ins_signed_byte(char b,int area);
void ins_short(INT16 l,int area);
void ins_long(long l,int area);
void ins_f_byte(unsigned int b);
void push_address();
void push_explicit(int address);
int pop_address();
struct jump;
int docode(node *n);
void do_code_block(node *n);
/* Prototypes end here */

#endif
