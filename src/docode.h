/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: docode.h,v 1.5 1998/03/28 15:38:02 grubba Exp $
 */
#ifndef DOCODE_H
#define DOCODE_H

/*
 * The compiler stack is used when compiling to keep track of data.
 * This value need too be large enough for the programs you compile.
 */
#define COMPILER_STACK_SIZE	8000


#define DO_LVALUE 1
#define DO_NOT_COPY 2
#define DO_POP 4

extern int store_linenumbers;
extern int comp_stackp;
extern INT32 comp_stack[COMPILER_STACK_SIZE];

#define emit(X,Y) insert_opcode((X),(Y),lex.current_line, lex.current_file)
#define emit2(X) insert_opcode2((X),lex.current_line, lex.current_file)

/* Prototypes begin here */
void upd_int(int offset, INT32 tmp);
INT32 read_int(int offset);
void push_address(void);
void push_explicit(INT32 address);
INT32 pop_address(void);
int alloc_label(void);
int do_jump(int token,INT32 lbl);
void do_pop(int x);
int do_docode(node *n,INT16 flags);
void do_cond_jump(node *n, int label, int iftrue, int flags);
void do_code_block(node *n);
int docode(node *n);
/* Prototypes end here */

#endif
