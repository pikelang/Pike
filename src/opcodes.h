/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef OPCODES_H
#define OPCODES_H

/* Prototypes begin here */
void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind);
void f_index();
void f_cast();
void f_sscanf(INT32 args);
/* Prototypes end here */

#endif
