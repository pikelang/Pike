/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "cyclic.h"

#define CYCLIC_HASH_SIZE 0x1000

static CYCLIC *cyclic_hash[CYCLIC_HASH_SIZE];

static size_t cyclic_hash_func(CYCLIC *c)
{
  size_t h;

  h = PTR_TO_INT(c->id);
  h *= 33;
  h ^= PTR_TO_INT(c->a);
  h *= 33;
  h ^= PTR_TO_INT(c->b);
  h *= 33;
  h ^= PTR_TO_INT(c->th);

#if SIZEOF_CHAR_P > 4
  h ^= h>>8;
#endif
  /* Fold h. This is to retain as many bits of h as possible.
   *
   * NB: The "magic" constant below has a 1 bit every 10 bits
   *     starting at the least significant, and is == 1 when
   *     shifted right 20 bits. Note also that 32 - 20 == 12
   *     and 1<<12 == 0x1000 == CYCLIC_HASH_SIZE.
   *
   *     The multiplication has the effect of accumulating
   *     the segments of 10 bits of h in the most significant
   *     segment, which is then shifted down.
   */
  h *= 0x100401;
  h >>= 20;

  return h & (CYCLIC_HASH_SIZE-1);
}

static void low_unlink_cyclic(CYCLIC *c)
{
  size_t h;
  CYCLIC **p;

  h = cyclic_hash_func(c);

  for(p=cyclic_hash+h;*p;p=&(p[0]->next))
  {
    if(c == *p)
    {
      *p=c->next;
#ifdef CYCLIC_DEBUG
      fprintf (stderr, "%s: END_CYCLIC a=%p b=%p: no cycle\n", c->id, c->a, c->b);
#endif
      return;
    }
  }
#ifdef PIKE_DEBUG
  Pike_fatal("Unlink cyclic on lost cyclic struct (%s).\n", c->id);
#else
  Pike_fatal("Unlink cyclic on lost cyclic struct.\n");
#endif
}

PMOD_EXPORT void unlink_cyclic(CYCLIC *c)
{
  UNSET_ONERROR(c->onerr);
  low_unlink_cyclic(c);
}

PMOD_EXPORT void *begin_cyclic(CYCLIC *c,
			       char *id,
			       void *th,
			       void *a,
			       void *b)
{
  size_t h;
  void *ret = 0;
  CYCLIC *p;

  c->ret = (void *)(ptrdiff_t)1;
  c->a = a;
  c->b = b;
  c->id = id;
  c->th = th;

  h = cyclic_hash_func(c);

  for(p=cyclic_hash[h];p;p=p->next)
  {
    if(a == p->a && b==p->b && id==p->id && th==p->th)
    {
#ifdef CYCLIC_DEBUG
      fprintf (stderr, "%s: BEGIN_CYCLIC a=%p b=%p: found cycle\n", id, a, b);
#endif
      c->ret = ret = p->ret;
      break;
    }
  }

  c->next = cyclic_hash[h];
  cyclic_hash[h] = c;
  SET_ONERROR(c->onerr, low_unlink_cyclic, c);
#ifdef CYCLIC_DEBUG
  if (!ret) fprintf (stderr, "%s: BEGIN_CYCLIC a=%p b=%p: no cycle\n", id, a, b);
#endif
  return ret;
}
