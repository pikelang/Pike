/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: opcodes.h,v 1.5 1998/04/16 01:14:17 hubbe Exp $
 */
#ifndef OPCODES_H
#define OPCODES_H

/* Prototypes begin here */
void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind);
void o_index(void);
void o_cast(struct pike_string *type, INT32 run_time_type);
void f_cast(void);
void o_sscanf(INT32 args);
void f_sscanf(INT32 args);
/* Prototypes end here */

#endif
