/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: cache.c,v 1.22 2004/03/10 16:31:41 nilsson Exp $
*/

#include "config.h"
#include <global.h>
#include <threads.h>
#include <stralloc.h>

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

#include "pike_netlib.h"
#include "accept_and_parse.h"
#include "cache.h"
#include "util.h"
#include "backend.h"


struct cache *first_cache;

static struct pike_string *free_queue[1024];
static int numtofree;
static MUTEX_T tofree_mutex;

static MUTEX_T cache_entry_lock;
int next_free_ce, num_cache_entries;
struct cache_entry *free_cache_entries[1024];

static void low_free_cache_entry( struct cache_entry *arg )
{
  num_cache_entries--;

  aap_enqueue_string_to_free( arg->data );
  aap_free( arg->url ); /* host is in the same malloced area */

  mt_lock( &cache_entry_lock );
  if( next_free_ce < 1024 )
    free_cache_entries[next_free_ce++] = arg;
  else
    aap_free(arg);
  mt_unlock( &cache_entry_lock );
/*   fprintf(stderr, " %d+%d args\n", num_cache_entries, next_free_ce ); */
}

struct cache_entry *new_cache_entry( )
{
  struct cache_entry *res;
  mt_lock( &cache_entry_lock );
  num_cache_entries++;
  if( next_free_ce )
    res = free_cache_entries[--next_free_ce];
  else
    res = aap_malloc( sizeof( struct cache_entry ) );
  mt_unlock( &cache_entry_lock );
/*   fprintf(stderr, " %d+%d centries\n", num_cache_entries, next_free_ce ); */
  return res;
}

static void really_free_from_queue(void) 
/* Must have tofree lock and interpreter lock */
{
  int i;
  for( i=0; i<numtofree; i++ )
    free_string( free_queue[i] );
  numtofree=0;
}

static int ensure_interpreter_lock(void)
{
  struct thread_state *thi;
  int free=0;
  if( (thi = thread_state_for_id( th_self() )) )
  {
    if( thi->swapped ) /* We are swapped out.. */
    {
      low_mt_lock_interpreter(); /* Can run even if threads_disabled. */
      return 1;
    }
    return 0; /* we are swapped in */
  }

  /* we are not a pike thread */
  if( num_threads == 1 )
    free=num_threads++;
  wake_up_backend();
  low_mt_lock_interpreter();	/* Can run even if threads_disabled. */
  if( free )
    num_threads--;
  return 1;
}

static void free_from_queue(void)
{
  /* We have the interpreter lock here, this is a backend callback */
  mt_lock( &tofree_mutex );
  really_free_from_queue();
  mt_unlock( &tofree_mutex );
}

void aap_enqueue_string_to_free( struct pike_string *s )
{
  mt_lock( &tofree_mutex );
  if( numtofree > 1020 )
  {
    /* This should not happend all that often.
     * Almost never, actually.
     *
     * This only happends if 1020 different cache entries
     * have to be freed in one backend callback loop.
     *
     */
    int free_interpreter_lock = ensure_interpreter_lock();
    really_free_from_queue();
    if( free_interpreter_lock )
      mt_unlock_interpreter();
  }
  free_queue[ numtofree++ ] = s;
  mt_unlock( &tofree_mutex );
}

static size_t cache_hash(char *s, ptrdiff_t len)
{
  size_t res = len * 9471111;
  while(len--) { res=res<<1 ^ ((res&(~0x7ffffff))>>31); res ^= s[len]; }
  return (res % CACHE_HTABLE_SIZE)/2;
} /*                              ^^ OBS! */

static void really_free_cache_entry(struct cache  *c, struct cache_entry *e,
				    struct cache_entry *prev, size_t b)
{
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>2)
  {
    if(b!=(cache_hash(e->url, e->url_len) +
	   cache_hash(e->host, e->host_len)))
      Pike_fatal("Cache entry did not hash to the same spot\n");
    if(!mt_trylock( & c->mutex ))
      Pike_fatal("Cache free_entry running unlocked\n");
    if(prev && prev->next != e)
      Pike_fatal("prev->next != e\n");
  }
#endif
  if(!prev)
    c->htable[ b ] = e->next;
  else
    prev->next = e->next;

  c->size -= e->data->len;
  c->entries--;
  low_free_cache_entry( e );
}




void aap_free_cache_entry(struct cache *c, struct cache_entry *e,
			  struct cache_entry *prev, size_t b)
{
#ifdef DEBUG
  if(e->refs<=0)
    Pike_fatal("Freeing free cache entry\n");
#endif
  if(!--e->refs) 
    really_free_cache_entry(c,e,prev,b);
}

void simple_aap_free_cache_entry(struct cache *c, struct cache_entry *e)
{
  mt_lock( &c->mutex );
  if(!--e->refs)
  {
    struct cache_entry *t, *p=0;
    size_t hv = cache_hash(e->url, e->url_len)+cache_hash(e->host,e->host_len);
    t = c->htable[ hv ];
    while(t)
    {
      if( t == e ) 
      {
	really_free_cache_entry(c,t,p,hv);
	break;
      }
      p=t;
      t=t->next;
    }
  }  
  mt_unlock( &c->mutex );
}


void aap_cache_insert(struct cache_entry *ce, struct cache *c)
{
  struct cache_entry *head, *p;
  char *t;
  size_t hv;
#ifdef DEBUG
  extern int d_flag;
  if((d_flag > 2) && !mt_trylock( & c->mutex ))
    Pike_fatal("Cache insert running unlocked\n");
#endif
  c->size += ce->data->len;
  if((head = aap_cache_lookup(ce->url, ce->url_len, 
                              ce->host, ce->host_len, c, 1, 
                              &p, &hv)))
  {
    c->size -= head->data->len;
    aap_enqueue_string_to_free(head->data);
    head->data = ce->data;
    head->stale_at = ce->stale_at;
    aap_free_cache_entry( c, head, p, hv );
    aap_free(ce);
  } else {
    c->entries++;
    t = aap_malloc( ce->url_len + ce->host_len );
    MEMCPY(t,ce->url,ce->url_len);   ce->url = t;   t+=ce->url_len;
    MEMCPY(t,ce->host,ce->host_len); ce->host = t;
    ce->next = c->htable[hv];
    ce->refs = 1;
    c->htable[hv] = ce;
  }
}

struct cache_entry *aap_cache_lookup(char *s, ptrdiff_t len,
				     char *ho, ptrdiff_t hlen, 
				     struct cache *c, int nolock,
				     struct cache_entry **p, size_t *hv)
{
  size_t h = cache_hash(s, len) + cache_hash(ho,hlen);
  struct cache_entry *e, *prev=NULL;

  if( hv ) *hv = h;
  if(!nolock) 
    mt_lock(&c->mutex);
#ifdef DEBUG
  else 
  {
    extern int d_flag;
    if((d_flag>2) && !mt_trylock( & c->mutex ))
      Pike_fatal("Cache lookup running unlocked\n");
  }
#endif
  if( p ) *p = 0;
  e = c->htable[h];
  while(e)
  {
    if(e->url_len == len && e->host_len == hlen 
       && !memcmp(e->url,s,len) 
       && !memcmp(e->host,ho,hlen))
    {
      int t = aap_get_time();
      if(e->stale_at < t)
      {
        aap_free_cache_entry( c, e, prev, h );
	if(!nolock) mt_unlock(&c->mutex);
	return 0;
      }
      c->hits++;
      /* cache hit. Lets add it to the top of the list */
      if(c->htable[h] != e)
      {
	if(prev) prev->next = e->next;
	e->next = c->htable[h];
	c->htable[h] = e;
      }
      if(!nolock) mt_unlock(&c->mutex);
      e->refs++;
      return e;
    }
    prev = e;
    if( p ) *p = prev;
    e = e->next;
  }
  c->misses++;
  if(!nolock) mt_unlock(&c->mutex);
  return 0;
}

void aap_clean_cache(void)
{
  struct cache *c = first_cache;
  if(numtofree) free_from_queue();
}

void aap_init_cache(void)
{
  mt_init(&tofree_mutex);
  mt_init(&cache_entry_lock);
}
#endif
