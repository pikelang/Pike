#include "global.h"
#include "pike_cpulib.h"
#include "svalue.h"

#ifdef PIKE_NEED_MEMLOCK


#define PIKE_MEM_HASH 17903
PIKE_MUTEX_T pike_memory_locks[PIKE_MEM_HASH];

void init_cpulib(void)
{
  int e;
  for(e=0;e<PIKE_MEM_HASH;e++) mt_init(pike_memory_locks+e);
}

#endif




