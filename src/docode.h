/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef DOCODE_H
#define DOCODE_H

#define DO_LVALUE 1
#define DO_NOT_COPY 2
#define DO_POP 4

extern int store_linenumbers;
extern int comp_stackp;
extern INT32 comp_stack[COMPILER_STACK_SIZE];

/* Prototypes begin here */
void ins_byte(unsigned char b,int area);
void ins_signed_byte(char b,int area);
void ins_short(INT16 l,int area);
void ins_long(INT32 l,int area);
void ins_f_byte(unsigned int b);
void push_address();
void push_explicit(INT32 address);
INT32 pop_address();
struct jump;
struct jump_list;
int do_docode(node *n,INT16 flags);
int docode(node *n);
void do_code_block(node *n);
/* Prototypes end here */

#endif
