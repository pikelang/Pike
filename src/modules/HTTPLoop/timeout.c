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

/* This must be included last! */
#include "module_magic.h"

MUTEX_T aap_timeout_mutex;

struct timeout 
{
  int raised;
  int when;
  struct timeout *next;
  THREAD_T thr;
} *first_timeout;


#define FATAL(X) do{fprintf(stderr,"%s:%d: %s",__FILE__,__LINE__,X);abort();}while(0)


#define CHUNK 100
int num_timeouts;

static struct timeout *new_timeout(THREAD_T thr, int secs) /* running locked */
{
  struct timeout *t = aap_malloc(sizeof(struct timeout));
  num_timeouts++;
  t->thr = thr;
  t->raised = 0;   
  t->next = NULL;
  t->when = aap_get_time()+secs;

  if( !first_timeout )
  {
    first_timeout = t;
  }
  else
  {
    struct timeout *p = first_timeout;
    while( p->next )
      p = p->next;
    p->next = t;
  }
  return t;
}

static void free_timeout( struct timeout *t ) /* running locked */
{
  num_timeouts--;
  aap_free( t );
}

int *aap_add_timeout_thr(THREAD_T thr, int secs)
{
  struct timeout *to;
  mt_lock( &aap_timeout_mutex );
  to = new_timeout( thr, secs );
  mt_unlock( &aap_timeout_mutex );
  return &to->raised;
}

#ifdef PIKE_DEBUG
void debug_print_timeout_queue( struct timeout *target )
{
  struct timeout *p = first_timeout;
  int found=0, actual_size=0;
  fprintf( stderr, "\n\nTimeout list, searching for <%p>, num=%d:\n",
           (void *)target, num_timeouts);
  while( p )
  {
    actual_size++;
    if( p == target )
      fprintf( stderr, "> %p < [%d]\n", (void *)target, ++found );
    else
      fprintf( stderr, "  %p  \n", (void *)target );
    p = p->next;
  }
  if( actual_size != num_timeouts )
  {
    fprintf( stderr, "There should be %d timeouts, only %d found\n",
             num_timeouts, actual_size );
  }
  if( !found )
    FATAL( "Timeout not found in chain\n");
  if( found > 1 )
    FATAL( "Timeout found more than once in chain\n");
}
#endif /* PIKE_DEBUG */

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
/*       debug_print_timeout_queue( t ); */
      if( t == first_timeout )
      {
        first_timeout = t->next;
      } else {
        struct timeout *p = first_timeout;
        while( p && p != t && p->next != t )
          p = p->next;
        if( p && (p->next == t) )
          p->next = t->next;
      }
      free_timeout( t );
    }
  }
  mt_unlock( &aap_timeout_mutex );
}

static volatile int aap_time_to_die = 0;

static void *handle_timeouts(void *ignored)
{
  while(!aap_time_to_die)
  {
    struct timeout *t;
    mt_lock( &aap_timeout_mutex );
    t = first_timeout;
    while(t)
    {
      if(t->when < aap_get_time())
      {
        t->raised++;
        th_kill( t->thr, SIGCHLD );
      }
      t = t->next;
    }
    mt_unlock( &aap_timeout_mutex );
#ifdef HAVE_POLL
    {
      /*  MacOS X is stupid, and requires a non-NULL pollfd pointer. */
      struct pollfd sentinel;
      poll(&sentinel,0,1000);
    }
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
  if (Pike_interpreter.thread_id) {
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
