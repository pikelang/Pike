/*
 * $Id: dynamic_load.h,v 1.4 2000/09/28 15:37:24 grubba Exp $
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
