#include "spider.h"

#if defined(HAVE_PTHREAD_MUTEX_UNLOCK) || defined(HAVE_MUTEX_UNLOCK)

#include <sys/types.h>

#ifdef HAVE_THREAD_H
# include <thread.h>
#else
# ifdef HAVE_PTHREAD_H
#  include <pthread.h>
# endif
#endif

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
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

#include <fcntl.h>
#include <sys/stat.h>

#include <errno.h>
#include "sharedmem.h"

#ifdef HAVE_THREAD_H
typedef mutex_t mylock_t;
#else
typedef pthread_mutex_t mylock_t;
#endif

#define LOCK_HASH_TABLE_SIZE ((1<<10)-1) /* MUST BE ((1<<n)-1), 1<n<30 */
 
struct _lock {
  mylock_t lock; /* MUST BE FIRST IN THE STRUCT */
  int id;
  struct _lock *next;
  struct _lock *next_hash;  
  struct _lock *prev_hash;
  struct _lock *prev;
} *first_lock=NULL, *last_lock=NULL;

struct _lock *lock_hash_table[LOCK_HASH_TABLE_SIZE+1];

mylock_t *findlock( int id )
{
  struct _lock *current = lock_hash_table[ id & LOCK_HASH_TABLE_SIZE ];
  while(current)
  {
    if(current->id == id)
      return &current->lock;
    current = current->next_hash;
  }
  return NULL;
}

mylock_t *newlock( int id )
{
  struct _lock *lock;
  struct _lock *tmp=NULL;
  lock = (struct _lock *)shared_malloc( sizeof( struct _lock ) );
  lock->id = id;
  if(!first_lock)
  {
    first_lock = last_lock = lock;
    lock->prev = lock->next = NULL;
  } else {
    lock->prev = last_lock;
    lock->next = NULL;
    last_lock->next = lock;
    last_lock = lock;
  }
  if( tmp = lock_hash_table[ id & LOCK_HASH_TABLE_SIZE] )
    tmp->prev_hash = lock;

  lock_hash_table[ id & LOCK_HASH_TABLE_SIZE] = lock;
  lock->next_hash = tmp;
  lock->prev_hash = NULL;

#ifdef HAVE_MUTEX_UNLOCK
  mutex_init( &lock->lock, USYNC_PROCESS, 0 );
#else
  {
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init( &attributes );
    pthread_mutexattr_setpshared( &attributes,  PTHREAD_PROCESS_SHARED );
    pthread_mutex_init( &lock->lock, &attributes );
    pthread_mutexattr_destroy( &attributes );
  }
#endif
  return &lock->lock;
}

void lock(mylock_t *lock)
{
#ifdef HAVE_MUTEX_UNLOCK
  mutex_lock( lock );
#else
  pthread_mutex_lock( lock );
#endif
}

void unlock(mylock_t *lock)
{
#ifdef HAVE_MUTEX_UNLOCK
  mutex_unlock( lock );
#else
  pthread_mutex_unlock( lock );
#endif
}

void freelock( mylock_t *lock )
{
  struct _lock *rlock;
  rlock = (struct _lock *)lock;
  if(rlock->next)
    rlock->next->prev = rlock->prev;
  if(rlock->prev)
    rlock->prev->next = rlock->next;
  if(rlock == first_lock)
    first_lock = rlock->next;
  if(rlock == last_lock)
    last_lock = rlock->prev;

  if(rlock->next_hash)
    rlock->next_hash->prev_hash= rlock->prev_hash;
  if(rlock->prev_hash)
    rlock->prev_hash->next_hash= rlock->next_hash;
  if(lock_hash_table[ rlock->id & LOCK_HASH_TABLE_SIZE] == rlock)
    lock_hash_table[ rlock->id & LOCK_HASH_TABLE_SIZE] = rlock->next_hash;

#ifdef HAVE_MUTEX_UNLOCK
  mutex_destroy( lock );
#else
  pthread_mutex_destroy( lock );
#endif
  shared_free( (char *)lock );
}

void f_lock(INT32 args)
{
  mylock_t *lck;
  if(!args)  error("You have to supply the lock-id.\n");
  lck = findlock( sp[-1].u.integer );
  pop_n_elems(args);
  if(!lck)  error("Unknown lock-id.\n");
  lock( lck );
  push_int( 1 );
}

void f_unlock(INT32 args)
{
  mylock_t *lock;
  if(!args) error("You have to supply the lock-id.\n");
  lock = findlock( sp[-1].u.integer );
  pop_n_elems(args);
  if(!lock)  error("Unknown lock-id.\n");
  unlock( lock );
  push_int( 1 );
}

void f_newlock(INT32 args)
{
  int res;
  int id;
  if(args) 
  {
    id = sp[-1].u.integer;
    pop_n_elems(args);
  } else
    id = 0;
  while(findlock( id )) 
    id+=rand(); /* This should limit the number of times we have to try. */

  newlock( id );
  push_int( id );
}


void f_freelock(INT32 args)
{
  int res;
  mylock_t *lock;
  if(!args) error("You have to supply the lock-id.\n");

  lock = findlock( sp[-args].u.integer );
  pop_n_elems(args);
  if(!lock) error("Unknown lock-id.\n");
  freelock( lock );
  push_int(1);
}
#endif

