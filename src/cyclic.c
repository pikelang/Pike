/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: cyclic.c,v 1.10 2003/01/06 17:20:59 grubba Exp $
*/

#include "global.h"
#include "cyclic.h"

RCSID("$Id: cyclic.c,v 1.10 2003/01/06 17:20:59 grubba Exp $");

#define CYCLIC_HASH_SIZE 4711

CYCLIC *cyclic_hash[CYCLIC_HASH_SIZE];

static void low_unlink_cyclic(CYCLIC *c)
{
  size_t h;
  CYCLIC **p;
  h=((char *)c->id)-(char *)0;
  h*=33;
  h|=((char *)c->a)-(char *)0;
  h*=33;
  h|=((char *)c->b)-(char *)0;
  h*=33;
  h|=((char *)c->th)-(char *)0;
  h*=33;
  h%=CYCLIC_HASH_SIZE;

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
  Pike_fatal("Unlink cyclic on lost cyclic struct.\n");
}

void unlink_cyclic(CYCLIC *c)
{
  UNSET_ONERROR(c->onerr);
  low_unlink_cyclic(c);
}

void *begin_cyclic(CYCLIC *c,
		   char *id,
		   void *th,
		   void *a,
		   void *b)
{
  size_t h;
  void *ret=0;
  CYCLIC *p;

  h=((char *)id)-(char *)0;
  h*=33;
  h|=((char *)a)-(char *)0;
  h*=33;
  h|=((char *)b)-(char *)0;
  h*=33;
  h|=((char *)th)-(char *)0;
  h*=33;
  h%=CYCLIC_HASH_SIZE;

  for(p=cyclic_hash[h];p;p=p->next)
  {
    if(a == p->a && b==p->b && id==p->id)
    {
#ifdef CYCLIC_DEBUG
      fprintf (stderr, "%s: BEGIN_CYCLIC a=%p b=%p: found cycle\n", id, a, b);
#endif
      ret=p->ret;
      break;
    }
  }

  c->ret=(void *)(ptrdiff_t)1;
  c->a=a;
  c->b=b;
  c->id=id;
  c->th=th;
  c->next=cyclic_hash[h];
  cyclic_hash[h]=c;
  SET_ONERROR(c->onerr, low_unlink_cyclic, c);
#ifdef CYCLIC_DEBUG
  if (!ret) fprintf (stderr, "%s: BEGIN_CYCLIC a=%p b=%p: no cycle\n", id, a, b);
#endif
  return ret;
}
