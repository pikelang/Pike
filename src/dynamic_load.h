/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef DYNAMIC_LOAD_H
#define DYNAMIC_LOAD_H

/* Prototypes begin here */
struct module_list;
void f_load_module(INT32 args);
void init_dynamic_load(void);
void exit_dynamic_load(void);
void free_dynamic_load(void);
/* Prototypes end here */

#endif
