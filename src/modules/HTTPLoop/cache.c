#include "config.h"
#include <global.h>
#include <threads.h>
#include <stralloc.h>
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
#include "cache.h"
#include "util.h"

struct cache *first_cache;

static struct pike_string *free_queue[1024];
static int numtofree;
static MUTEX_T tofree_mutex;

static void really_free_from_queue() 
/* Must have tofree lock and interpreter lock */
{
  int i;
  for( i=0; i<numtofree; i++ )
    free_string( free_queue[i] );
  numtofree=0;
}

static int ensure_interpreter_lock( )
{
  struct thread_state *thi;
  int free=0;
  if( thi = thread_state_for_id( th_self() ) )
  {
    if( thi->swapped ) /* We are swapped out.. */
    {
      mt_lock( &interpreter_lock );
      return 1;
    }
    return 0; /* we are swapped in */
  }

  /* we are not a pike thread */
  if( num_threads == 1 )
    free=num_threads++;
  wake_up_backend();
  mt_lock( &interpreter_lock );
  if( free ) 
    num_threads--;
  return 1;
}

static void free_from_queue()
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
    int free_interpreter_lock = ensure_interpteter_lock();
    really_free_from_queue();
    if( free_interpreter_lock )
      mt_unlock( &interpreter_lock );
  }
  free_queue[ numtofree++ ] = s;
  mt_unlock( &tofree_mutex );
}

static int cache_hash(char *s, int len)
{
  unsigned int res = len * 9471111;
  while(len--) { res=res<<1 ^ ((res&(~0x7ffffff))>>31); res ^= s[len]; }
  return (res % CACHE_HTABLE_SIZE)/2;
} /*                              ^^ OBS! */

static void really_free_cache_entry(struct cache  *c, struct cache_entry *e,
				    struct cache_entry *prev, int b)
{
#ifdef DEBUG
  extern int d_flag;
  if(d_flag>2)
  {
    if(b!=(cache_hash(e->url, e->url_len) +
	   cache_hash(e->host, e->host_len)))
      fatal("Cache entry did not hash to the same spot\n");
    if(!mt_trylock( & c->mutex ))
      fatal("Cache free_entry running unlocked\n");
    if(prev && prev->next != e)
      fatal("prev->next != e\n");
  }
#endif
  if(!prev)
    c->htable[ b ] = e->next;
  else
    prev->next = e->next;

  c->size -= e->data->len;
  c->entries--;
    /* Not really a good idea here.. 
       We might be running without the interpreter lock. */
/*   free_string(e->data);  */
  aap_enqueue_string_to_free( e->data );
/*   free(e->host);  Same as URL*/
  free(e->url);
  free(e);
}




void aap_free_cache_entry(struct cache *c, struct cache_entry *e,
			  struct cache_entry *prev, int b)
{
#ifdef DEBUG
  if(e->refs<=0)
    fatal("Freeing free cache entry\n");
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
    int hv = cache_hash(e->url, e->url_len)+cache_hash(e->host,e->host_len);
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
  int hv;
#ifdef DEBUG
  extern int d_flag;
  if((d_flag > 2) && !mt_trylock( & c->mutex ))
    fatal("Cache insert running unlocked\n");
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
    free(ce);
  } else {
    c->entries++;
    t = malloc( ce->url_len + ce->host_len );
    MEMCPY(t,ce->url,ce->url_len);   ce->url = t;   t+=ce->url_len;
    MEMCPY(t,ce->host,ce->host_len); ce->host = t;
    ce->next = c->htable[hv];
    ce->refs = 1;
    c->htable[hv] = ce;
  }
}

struct cache_entry *aap_cache_lookup(char *s, int len,char *ho, int hlen, 
				     struct cache *c, int nolock,
				     struct cache_entry **p, int *hv)
{
  int h = cache_hash(s, len) + cache_hash(ho,hlen);
  struct cache_entry *e, *prev=NULL;

  if( hv ) *hv = h;
  if(!nolock) 
    mt_lock(&c->mutex);
#ifdef DEBUG
  else 
  {
    extern int d_flag;
    if((d_flag>2) && !mt_trylock( & c->mutex ))
      fatal("Cache lookup running unlocked\n");
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

void aap_clean_cache()
{
  struct cache *c = first_cache;
  if(numtofree) free_from_queue();
/*   while( c ) */
/*   { */
/*     mt_lock( &c->mutex ); */
/*     while( c->size > c->max_size ) */
/*     { */
/*       int i, mlen=0; */
/*       struct cache_entry *e; */
/*       int cv = 0; */
/*       for( i=0; i<CACHE_HTABLE_SIZE; i++ ) */
/*         if( c->htable[i] && */
/*             c->htable[i]->data->len  > mlen ) */
/*         { */
/*           mlen = c->htable[i]->data->len; */
/*           e = c->htable[i]; */
/*           cv = i; */
/*         } */
/*       aap_free_cache_entry( c, e, 0, cv ); */
/*     } */
/*     mt_unlock( &c->mutex ); */
/*     c = c->next; */
/*   } */
/*   if(numtofree) free_from_queue(); */
}
#endif
