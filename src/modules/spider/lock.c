#include "spider.h"
#ifdef HAVE_SYS_SEM_H

#include <sys/types.h>

#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#include <sys/sem.h>

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

#include <errno.h>
#endif

#ifdef HAVE_SYS_SEM_H
static struct sembuf op_lock[2] = {
  0, 0, 0,        /* Wait for sem#0 to become zero. */
  0, 1, SEM_UNDO  /* .. and increment by one */
};

static struct sembuf op_unlock[1] = {
  0, -1, IPC_NOWAIT | SEM_UNDO  /* Decrement sem#0 by one */
};
#endif

#ifdef HAVE_SYS_SEM_H
void f_lock(INT32 args)
{
#ifdef HAVE_SEMOP
  int res;
  if(!args) error("You have to supply the lock-id.\n");
  res = semop(sp[-1].u.integer, op_lock, 2);
  pop_n_elems(args);
  if(res==-1) push_int(-errno);
  else push_int(0);
#endif
}
#endif

#ifdef HAVE_SYS_SEM_H
void f_unlock(INT32 args)
{
#ifdef HAVE_SEMOP
  int res;
  if(!args) error("You have to supply the lock-id.\n");
  res = semop(sp[-1].u.integer, op_unlock, 1);
  pop_n_elems(args);
  if(res==-1) push_int(-errno);
  else push_int(0);
#endif
}
#endif

#ifdef HAVE_SYS_SEM_H
#include "list.h"
static struct list *marks;

static void mark_as_used(int id, int p)
{
  if(!marks)
  {
    if(p)
    {
      push_int(id);
      f_aggregate_list(1);
    } else {
      f_aggregate_list(0);
    }
    marks = sp[-1].u.list;
    marks->refs++;
    pop_stack();
  } else {
    push_int(id);
    if(p)
      list_insert(marks, sp-1);
    else
      list_delete(marks, sp-1);
    pop_stack();
  }
}
#endif

#ifdef HAVE_SYS_SEM_H
void f_newlock(INT32 args)
{
#ifdef HAVE_SEMGET
  int res;
  if(!args) error("You have to supply the lock-key.\n");
  res = semget(sp[-1].u.integer, 1, IPC_CREAT | 0666);
  pop_n_elems(args);
  if(res != -1)
    mark_as_used(res, 1);
  push_int(res);
#endif
}
#endif


#ifdef HAVE_SYS_SEM_H
void f_freelock(INT32 args)
{
#ifdef HAVE_SEMCTL
  int res;
  if(!args) error("You have to supply the lock-id.\n");
  res = semctl(sp[-1].u.integer, 0, IPC_RMID, 0);
  if(!res)
    mark_as_used(sp[-1].u.integer, 0);
  pop_n_elems(args);
  push_int(!res);
#endif
}
#endif


#ifdef HAVE_SYS_SEM_H
#include <stdio.h>
void free_all_locks()
{
  int i;
  if(marks)
  {
    fprintf(stderr, "Free all locks..");
    for(i=0; i<marks->ind->size; i++)
    {
      push_int(marks->ind->item[i].u.integer);
      fprintf(stderr, "%d..", sp[-1].u.integer);
      f_freelock(1);
      pop_stack();
    }
    free_list(marks);
    fprintf(stderr, "\n");
  }
}
#endif
