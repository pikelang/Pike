#include "cyclic.h"

#define CYCLIC_HASH_SIZE 4711

CYCLIC *cyclic_hash[CYCLIC_HASH_SIZE];

void unlink_cyclic(CYCLIC *c)
{
  unsigned int h;
  CYCLIC **p;
  h=(int)c->id;
  h*=33;
  h|=(int)c->a;
  h*=33;
  h|=(int)c->b;
  h*=33;
  h|=(int)c->th;
  h*=33;
  h%=CYCLIC_HASH_SIZE;

  for(p=cyclic_hash+h;*p;p=&(p[0]->next))
  {
    if(c == *p)
    {
      *p=c->next;
      UNSET_ONERROR(c->onerr);
      return;
    }
  }
  fatal("Unlink cyclic on lost cyclic struct.\n");
}

void *begin_cyclic(CYCLIC *c,
		   void *id,
		   void *th,
		   void *a,
		   void *b)
{
  unsigned int h;
  CYCLIC *p;

  h=(int)id;
  h*=33;
  h|=(int)a;
  h*=33;
  h|=(int)b;
  h*=33;
  h|=(int)th;
  h*=33;
  h%=CYCLIC_HASH_SIZE;

  for(p=cyclic_hash[h];p;p=p->next)
    if(a == p->a && b==p->b && id==p->id)
      return p->ret;

  c->ret=(void *)1;
  c->a=a;
  c->b=b;
  c->id=id;
  c->th=th;
  c->next=cyclic_hash[h];
  cyclic_hash[h]=c;
  SET_ONERROR(c->onerr, unlink_cyclic, &c);
  return 0;
}
