#ifndef CYCLIC_H
#define CYCLIC_H

#include "error.h"
#include "threads.h"

typedef struct CYCLIC
{
  ONERROR onerr;
  void *th;
  void *id,*a,*b;
  void *ret;
  struct CYCLIC *next;
} CYCLIC;


#define DECLARE_CYCLIC() \
  static char cyclic_identifier__; \
  CYCLIC cyclic_struct__

#define BEGIN_CYCLIC(A,B) \
   begin_cyclic(&cyclic_struct__, &cyclic_identifier__, th_self(), (void *)(A), (void *)(B))

#define SET_CYCLIC_RET(RET) \
   cyclic_struct__.ret=(void *)(RET)

#define END_CYCLIC()  unlink_cyclic(&cyclic_struct__)

/* Prototypes begin here */
void unlink_cyclic(CYCLIC *c);
void *begin_cyclic(CYCLIC *c,
		   void *id,
		   void *thread,
		   void *a,
		   void *b);
/* Prototypes end here */

#endif /* CYCLIC_H */

