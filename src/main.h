/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MAIN_H
#define MAIN_H

#include "callback.h"

extern int d_flag, t_flag, a_flag, l_flag, c_flag;

/* Prototypes begin here */
struct callback *add_post_master_callback(callback_func call,
					  void *arg,
					  callback_func free_func);
void main(int argc, char **argv, char **env);
void init_main_efuns();
void init_main_programs();
void exit_main();
/* Prototypes end here */

#endif
