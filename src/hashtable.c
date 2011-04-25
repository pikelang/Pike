/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "hashtable.h"
#include "stralloc.h"
#include "stuff.h"
#include "pike_error.h"

RCSID("$Id$");

static size_t gobble(struct pike_string *s)
{
  size_t i;
  i=my_hash_string(s);
  i+=i >> 3;
  i+=i >> 7;
  i+=i >> 12;
  return i;
}

/*
 * Search hash for a specific string.
 */
struct hash_entry *hash_lookup(struct hash_table *h, struct pike_string *s)
{
  struct hash_entry *e, **prev, **base;

  if(!h) return 0;
  base = prev = h->htable + (gobble(s) & h->mask);
  for( ;(e = *prev); prev= &e->next)
  {
    if(s == e->s)
    {
      /* Teleport entry to beginning of line */
      *prev = e->next;
      e->next = *base;
      *base = e;
      /* Entry arrives in a puff of smoke. */

      return e;
    }
  }
  return 0;
}

/*
 * We want to keep the order of the hash chain, as it has been carefully
 * built up to be fast.
 */
static void rehash_list_backwards(struct hash_table *h,
				  struct hash_entry *n)
{
  struct hash_entry **base;
  if(!n) return;
  rehash_list_backwards(h,n->next);
  base=h->htable + (gobble(n->s) & h->mask);
  n->next = *base;
  *base=n;
}

/*
 * create a new, empty hashable
*/
struct hash_table *create_hash_table(void)
{
  struct hash_table *new;
  new=(struct hash_table *)calloc(1,sizeof(struct hash_table)+
				  (NEW_HASHTABLE_SIZE-1)*sizeof(struct hash_entry *));
  new->entries=0;
  new->mask=NEW_HASHTABLE_SIZE-1;
  return new;
}

/*
 * rehash - ugh
 */
struct hash_table *hash_rehash(struct hash_table *h,int size)
{
  struct hash_table *new;
  int e;

#ifdef PIKE_DEBUG
  if( 1 << my_log2(size) != size)
    Pike_fatal("Size is not a power of two!\n");
#endif

  new=(struct hash_table *)calloc(1,sizeof(struct hash_table)+
				  (size-1)*sizeof(struct hash_entry *));
  new->mask = size - 1;
  new->entries = h->entries;

  for(e=0; e<=h->mask; e++)
    rehash_list_backwards(new,h->htable[e]);

  free((char *)h);
  return new;
}

/*
 * insert a newly created hash entry on it's rightful place in the
 * hashtable
 */
struct hash_table *hash_insert(struct hash_table *h, struct hash_entry *s)
{
  struct hash_entry **base;

  if(!h) h=create_hash_table();

  if(h->entries > AVERAGE_HASH_LENGTH * 2 * h->mask)
    h=hash_rehash(h,(h->mask+1) * 2);

  base = h->htable + (gobble(s->s) & h->mask);
  s->next = *base;
  *base = s;
  h->entries++;

  return h;
}

/*
 * unlink an entry from an hash table
 */
struct hash_table *hash_unlink(struct hash_table *h, struct hash_entry *s)
{
  struct hash_entry **prev,*e;

  prev = h->htable + (gobble(s->s) & h->mask);
  for(;( e=*prev ); prev=&e->next)
  {
    if(e==s)
    {
      *prev = e->next;
      h->entries--;
      if(h->mask > NEW_HASHTABLE_SIZE  &&
	 h->entries < AVERAGE_HASH_LENGTH / 2 * h->mask)
	h=hash_rehash(h,(h->mask+1) / 2);
      return h;
    }
  }
#ifdef PIKE_DEBUG
  Pike_fatal("hash_entry not in hashtable\n");
#endif
  return h;
}

void map_hashtable(struct hash_table *h, void (*fun)(struct hash_entry *))
{
  INT32 e;
  struct hash_entry *i, *n;
  for(e=0;e<=h->mask;e++)
  {
    for(i=h->htable[e];i;i=n)
    {
      n=i->next;
      fun(i);
    }
  }
}

void free_hashtable(struct hash_table *h,
		    void (*free_entry)(struct hash_entry *))
{
  INT32 e;
  struct hash_entry *i, *n;
  for(e=0;e<=h->mask;e++)
  {
    for(i=h->htable[e];i;i=n)
    {
      n=i->next;
      free_string(i->s);
      if(free_entry)
	free_entry(i);
      else
	free((char *)i);
    }
  }
  free((char *)h);
}
