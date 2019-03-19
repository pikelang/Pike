/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef CPP_H
#define CPP_H

#ifndef STRUCT_HASH_ENTRY_DECLARED
struct hash_entry;
#define STRUCT_HASH_ENTRY_DECLARED
#endif

/* Prototypes begin here */
void f_cpp(INT32 args);
void init_cpp(void);
void add_predefine(const char *s);
void exit_cpp(void);
/* Prototypes end here */
#endif
