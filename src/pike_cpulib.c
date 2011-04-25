/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "pike_cpulib.h"
#include "svalue.h"

#ifdef PIKE_NEED_MEMLOCK


#define PIKE_MEM_HASH 17903
PIKE_MUTEX_T pike_memory_locks[PIKE_MEM_HASH];

void init_pike_cpulib(void)
{
  int e;
  for(e=0;e<PIKE_MEM_HASH;e++)
    mt_init_recursive(pike_memory_locks+e);
}

#endif
