/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef DOCODE_H
#define DOCODE_H

#define DO_LVALUE 1
#define DO_NOT_COPY 2
#define DO_POP 4
#define DO_INDIRECT 8
#define DO_LVALUE_IF_POSSIBLE 16
#define DO_NOT_COPY_TOPLEVEL 32

#define WANT_LVALUE (DO_LVALUE | DO_INDIRECT)

#define emit0(X)     insert_opcode0((X),lex.current_line, lex.current_file)
#define emit1(X,Y)   insert_opcode1((X),(Y),lex.current_line, lex.current_file)
#define emit2(X,Y,Z) insert_opcode2((X),(Y),(Z),lex.current_line, lex.current_file)

/* Prototypes begin here */
void upd_int(int offset, INT32 tmp);
INT32 read_int(int offset);
void push_address(void);
void push_explicit(INT32 address);
INT32 pop_address(void);
int alloc_label(void);
int do_jump(int token,INT32 lbl);
void do_pop(int x);
int do_docode(node *n, INT16 flags);
void do_cond_jump(node *n, int label, int iftrue, int flags);
INT32 do_code_block(node *n);
int docode(node *n);
/* Prototypes end here */

#endif
