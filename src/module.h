/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MODULE_H
#define MODULE_H

#include "types.h"

typedef void (*fun)(void);

struct module
{
  char *name;
  fun init_efuns;  /* this one _might_ be called before the master is compiled */
  fun init_programs; /* this one is called after the master is compiled */
  fun exit;
  INT32 refs;
};

#define UGLY_WORKAROUND []
extern struct module *current_module;

/* Prototypes begin here */
void init_modules_efuns();
void init_modules_programs();
void exit_modules();
/* Prototypes end here */


#endif
