#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_SEM_H
#include <sys/sem.h>
#endif

#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "add_efun.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_efuns.h"

void f_lock(INT32 args)
{
#ifdef HAVE_SEMOP
  int res;
  struct sembuf opts;
  if(!args) error("You have to supply the lock-id.\n");
  opts.sem_num=0;
  opts.sem_op = -1;
  res = semop(sp[-1].u.integer, &opts, SEM_UNDO);
  if(res==-1) push_int(-errno);
  else push_int(0);
#endif
}

void f_unlock(INT32 args)
{
#ifdef HAVE_SEMOP
  int res;
  struct sembuf opts;
  if(!args) error("You have to supply the lock-id.\n");
  opts.sem_num=0;
  opts.sem_op = 1;
  res = semop(sp[-1].u.integer, &opts, SEM_UNDO);
  if(res==-1) push_int(-errno);
  else push_int(0);
#endif
}

void f_newlock(INT32 args)
{
#ifdef HAVE_SEMGET
  pop_n_elems(args);
  push_int(semget(IPC_PRIVATE, 1, IPC_CREAT));
#endif
}

void f_freelock(INT32 args)
{
#ifdef HAVE_SEMCTL
  if(!args) error("You have to supply the lock-id.\n");
  return !(semctl(args[-1].u.integer, 0, IPC_RMID))
#endif
}

