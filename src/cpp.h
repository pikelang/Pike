/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: cpp.h,v 1.7 2002/10/11 01:39:30 nilsson Exp $
*/

#ifndef CPP_H
#define CPP_H

#ifndef STRUCT_HASH_ENTRY_DECLARED
struct hash_entry;
#define STRUCT_HASH_ENTRY_DECLARED
#endif

/* Prototypes begin here */
struct define_part;
struct define_argument;
struct define;
struct cpp;
void cpp_error(struct cpp *this,char *err);
void PUSH_STRING(char *str,
		 INT32 len,
		 dynamic_buffer *buf);
void free_one_define(struct hash_entry *h);
void f_cpp(INT32 args);
void init_cpp(void);
void add_predefine(char *s);
void exit_cpp(void);
/* Prototypes end here */

#endif
