/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef CYCLIC_H
#define CYCLIC_H

#include "pike_error.h"
#include "threads.h"

/* #define CYCLIC_DEBUG */

typedef struct CYCLIC
{
  ONERROR onerr;
  void *th;
  char *id;
  void *a,*b;
  void *ret;
  struct CYCLIC *next;
} CYCLIC;


#ifdef CYCLIC_DEBUG

#define DECLARE_CYCLIC()						\
  static char cyclic_identifier__[] = __FILE__ ":" DEFINETOSTR(__LINE__); \
  CYCLIC cyclic_struct__
#define BEGIN_CYCLIC(A,B) \
   begin_cyclic(&cyclic_struct__, cyclic_identifier__, \
                THREAD_T_TO_PTR(th_self()), (void *)(A), (void *)(B))

#else  /* CYCLIC_DEBUG */

#define DECLARE_CYCLIC()	   \
  static char cyclic_identifier__; \
  CYCLIC cyclic_struct__
#define BEGIN_CYCLIC(A,B) \
   begin_cyclic(&cyclic_struct__, &cyclic_identifier__, \
                THREAD_T_TO_PTR(th_self()), (void *)(A), (void *)(B))

#endif	/* !CYCLIC_DEBUG */

#define SET_CYCLIC_RET(RET) \
   cyclic_struct__.ret=(void *)(RET)

#define END_CYCLIC()  unlink_cyclic(&cyclic_struct__)

/* Prototypes begin here */
void unlink_cyclic(CYCLIC *c);
void *begin_cyclic(CYCLIC *c,
		   char *id,
		   void *thread,
		   void *a,
		   void *b);
/* Prototypes end here */

#endif /* CYCLIC_H */
