#include "config.h"
#ifndef __NT__
#include <global.h>
#include <threads.h>
#include <stralloc.h>
#include <signal.h>
#include <fdlib.h>

#ifdef _REENTRANT
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "accept_and_parse.h"
#include "timeout.h"
#include "util.h"

#ifdef HAVE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#else /* !HAVE_POLL_H */
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#else /* !HAVE_SYS_POLL_H */
#undef HAVE_POLL
#endif /* HAVE_SYS_POLL_H */
#endif /* HAVE_POLL_H */
#endif /* HAVE_POLL */

MUTEX_T aap_timeout_mutex;

struct timeout {
  int raised;
  struct timeout *next, *prev;
  THREAD_T thr;
  int when;
} *first_timeout, *last_timeout;

struct timeout *next_free_timeout;



/*
 * Debug code, since I for some reason always manage
 * to mess up my double linked lists (this time I had forgot a mt_lock()..
 */
#ifdef DEBUG
extern int d_flag;
/*
 * If I call the 'fatal' in pike, I run out of stack space. This is
 * not a good idea, since you cannot debug this program with gdb
 * if this happends.  Thus this macro... 
 */
#define FATAL(X) do{fprintf(stderr,"%s:%d: %s",__FILE__,__LINE__,X);abort();}while(0)

static void debug_tchain(void)
{
  if(first_timeout)
  {
    struct timeout *t = first_timeout, *p=NULL;
    if(!last_timeout)
      FATAL("First timeout, but no last timeout.\n");
    if(last_timeout->next)
      FATAL("Impossible timeout list: lt->next != NULL\n");
    
    while( t )
    {
      p = t;
      t = t->next;
      if(p == t)
	FATAL("Impossible timeout list: t->next==t\n");
      if(t->prev != p)
	FATAL("Impossible timeout list: t->prev!=p\n");
    }
    if(p != last_timeout)
      FATAL("End of to list != lt\n");
  }
}
#endif


#define CHUNK 1000

static struct timeout *new_timeout(THREAD_T thr, int secs)
{
  struct timeout *t;
  if(next_free_timeout)
  {
    t = next_free_timeout;
    next_free_timeout = t->next;
  } else {
    int i;
    char *foo = malloc(sizeof(struct timeout) * CHUNK);
    for(i = 1; i<CHUNK; i++)
    {
      struct timeout *c = (struct timeout *)(foo+(i*sizeof(struct timeout)));
      c->next = next_free_timeout;
      next_free_timeout = c;
    }
    t = (struct timeout *)foo;
  }
  t->next = NULL;
  t->prev = NULL;
  t->thr = thr;
  t->raised = 0;
  t->when = aap_get_time() + secs;
  return t;
}

static void free_timeout( struct timeout *t )
{
  t->next = next_free_timeout;
  next_free_timeout = t;
}

int  *aap_add_timeout_thr(THREAD_T thr, int secs)
{
  struct timeout *to;
  mt_lock( &aap_timeout_mutex );
  to = new_timeout( thr, secs );
  if(last_timeout)
  {
    last_timeout->next = to;
    to->prev = last_timeout;
    last_timeout = to;
  } else
    last_timeout = first_timeout = to;
#ifdef DEBUG
  if(d_flag>2) debug_tchain();
#endif
  mt_unlock( &aap_timeout_mutex );
  return &to->raised;
}


#ifndef OFFSETOF
#define OFFSETOF(str_type, field) ((long)& (((struct str_type *)0)->field))
#endif

void aap_remove_timeout_thr(int *to)
{
  mt_lock( &aap_timeout_mutex );
  {
    /* This offset is most likely 0.
     * But who knows what odd things compilers
     * can come up with when you are not looking...
     */
    struct timeout *t=(struct timeout *)(((char *)to)
					 -OFFSETOF(timeout,raised));
    if(t)
    {
      if(t->next) 
	t->next->prev = t->prev;
      else
	last_timeout = t->prev;
      if(t->prev) 
	t->prev->next = t->next;
      else
	first_timeout = t->next;
      free_timeout( t );
    }
  }
#ifdef DEBUG
  if(d_flag>2) debug_tchain();
#endif
  mt_unlock( &aap_timeout_mutex );
}

static volatile int aap_time_to_die = 0;

static void *handle_timeouts(void *ignored)
{
  while(!aap_time_to_die)
  {
    if(first_timeout)
    {
      mt_lock( &aap_timeout_mutex );
      {
	struct timeout *t = first_timeout;
	if(t)
	{
	  if(t->when < aap_get_time())
	  {
	    t->raised++;
	    th_kill( t->thr, SIGCHLD );
	  }
	  if(last_timeout != first_timeout)
	  {
	    first_timeout = t->next;
            first_timeout->prev = NULL;
	    t->next=NULL;
	    last_timeout->next = t;
            t->prev = last_timeout;
	    last_timeout = t;
	  }
	}
      }
      mt_unlock( &aap_timeout_mutex );
    }
#ifdef DEBUG
    if(d_flag>2) debug_tchain();
#endif
#ifdef HAVE_POLL
    poll(0,0,300);
#else
    sleep(1);
#endif
  }
  /*
   * Now we're dead...
   */
#ifdef AAP_DEBUG
  fprintf(stderr, "AAP: handle_timeout() is now dead.\n");
#endif /* AAP_DEBUG */
  return(NULL);
}

static THREAD_T aap_timeout_thread;

void aap_init_timeouts(void)
{
#ifdef AAP_DEBUG
  fprintf(stderr, "AAP: aap_init_timeouts.\n");
#endif /* AAP_DEBUG */
  mt_init(&aap_timeout_mutex);
  th_create_small(&aap_timeout_thread, handle_timeouts, 0);
#ifdef AAP_DEBUG
  fprintf(stderr, "AAP: handle_timeouts started.\n");
#endif /* AAP_DEBUG */
}

void aap_exit_timeouts(void)
{
  void *res;

#ifdef AAP_DEBUG
  fprintf(stderr, "AAP: aap_exit_timeouts.\n");
#endif /* AAP_DEBUG */
  aap_time_to_die = 1;
  if (thread_id) {
    THREADS_ALLOW();
    th_join(aap_timeout_thread, &res);
    THREADS_DISALLOW();
  } else {
    th_join(aap_timeout_thread, &res);
  }
#ifdef AAP_DEBUG
  fprintf(stderr, "AAP: aap_exit_timeouts done.\n");
#endif /* AAP_DEBUG */
}

#endif
#endif
