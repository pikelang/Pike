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
  free_string(e->data);
  free(e->host);
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
  if(!--e->refs) really_free_cache_entry(c,e,prev,b);
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
			  &p, &hv)) && !head->dead)
  {
    c->size -= head->data->len;
    if(head->data != ce->data)
    {
      free_string(head->data);
      head->data = ce->data;
    } else
      free_string(head->data);
    head->stale_at = ce->stale_at;
    head->dead = 0;
    aap_free_cache_entry( c, head, p, hv );
    free(ce);
  } else {
    c->entries++;
    t = malloc(ce->url_len);  MEMCPY(t,ce->url,ce->url_len);   ce->url = t;
    t = malloc(ce->host_len+1); MEMCPY(t,ce->host,ce->host_len); ce->host = t;
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

  *hv = h;
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
  *p = 0;
  e = c->htable[h];
  while(e)
  {
    if(!e->dead && e->url_len == len && e->host_len == hlen 
       && !memcmp(e->url,s,len) && !memcmp(e->host,ho,hlen))
    {
      int t = aap_get_time();
      if(e->stale_at < t)
      {
	c->unclean = 1;
	/* We cannot free the pike-string from here. 
	 * Let the clean-up code handle that instead. 
	 */
	e->dead = 1;
	c->stale++;
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
    *p = prev = e;
    e = e->next;
  }
  c->misses++;
  if(!nolock) mt_unlock(&c->mutex);
  return 0;
}


void aap_clean_cache(struct cache *c, int nolock)
{
  int i;
  struct cache_entry *e, *p, *prev = 0;
  if(!nolock) mt_lock(&c->mutex);
  for(i=0; i<CACHE_HTABLE_SIZE; i++)
  {
    e = c->htable[i];
    prev=0;
    while(e)
    {
      p = e->next;
      if(e->dead) 
	aap_free_cache_entry(c, e, prev, i);
      else 
	prev = e;
      e = p;
    }
  }
  if(!nolock) mt_unlock(&c->mutex);
}
#endif
