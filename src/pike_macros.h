/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: pike_macros.h,v 1.23 2000/12/16 05:45:44 marcus Exp $
 */
#ifndef MACROS_H
#define MACROS_H

#include <global.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "pike_memory.h"

#define OFFSETOF(str_type, field) ((size_t)& (((struct str_type *)0)->field))
#define BASEOF(ptr, str_type, field)  \
((struct str_type *)((char*)ptr - (char*)& (((struct str_type *)0)->field)))

#define NELEM(a) (sizeof (a) / sizeof ((a)[0]))
#define ALLOC_STRUCT(X) ( (struct X *)xalloc(sizeof(struct X)) )

#define MINIMUM(X,Y) ((X)<(Y)?(X):(Y))
#define MAXIMUM(X,Y) ((X)>(Y)?(X):(Y))


#define is8bitalnum(X)	("0000000000000000" \
			 "0000000000000000" \
			 "0000000000000000" \
			 "1111111111000000" \
			 "0111111111111111" \
			 "1111111111100001" \
			 "0111111111111111" \
			 "1111111111100000" \
			 "0000000000000000" \
			 "0000000000000000" \
			 "1011110101100010" \
			 "1011011001101110" \
			 "1111111111111111" \
			 "1111111011111111" \
			 "1111111111111111" \
			 "1111111011111111"[((unsigned)(X))&0xff] == '1')
  
#define isidchar(X) is8bitalnum(X)

#ifndef HAVE_ISGRAPH
#define isgraph(X)	(ispunct(X) || isupper(X) || islower(X) || isdigit(X))
#endif /* !HAVE_ISGRAPH */

/*
 * #define ALIGNOF(X) __alignof__(X)
 * #define ALIGNOF(X) (sizeof(X)>ALIGN_BOUND?ALIGN_BOUND:( 1<<my_log2(sizeof(X))))
 */

#define ALIGNOF(X) ((size_t)&(((struct { char ignored_ ; X fooo_; } *)0)->fooo_))

#define DO_ALIGN(X,Y) (((size_t)(X)+((Y)-1)) & -(Y))
#define CONSTANT_STRLEN(X) (sizeof(X) - sizeof(""))

#define SET_NEXT_AND_FREE(p,free_item) do{	\
  next=p->next;					\
  while(p->refs == 1 && (next=p->next))		\
  {						\
    add_ref(next);				\
    free_item(p);				\
    p=next;					\
  }						\
  free_item(p);					\
}while(0)

/* This variant never leaves p pointing at a deallocated block, as the
 * one above can do. I.e. it frees a ref to the item p points at, and
 * sets p to the same or next item with references, or sets it to
 * zero. */
#define FREE_AND_GET_REFERENCED(p, item_type, free_item) do {		\
  item_type *next;							\
  while (1) {								\
    if (p->refs > 1) {							\
      free_item(p);							\
      break;								\
    }									\
    if (!(next = p->next)) {						\
      free_item(p);							\
      p = 0;								\
      break;								\
    }									\
    add_ref(next);							\
    free_item(p);							\
    p = next;								\
  }									\
} while (0)

#define DOUBLELINK(first_object, o) do {	\
  o->next=first_object;				\
  o->prev=0;					\
  if(first_object) first_object->prev=o;	\
  first_object=o;				\
}while(0)

#define DOUBLEUNLINK(first_object,o) do{	\
  if(o->prev) {					\
    o->prev->next=o->next;			\
  }else {					\
    DO_IF_DEBUG(				\
      if(first_object != o)			\
        fatal("Linked in wrong list!\n");	\
    )						\
    first_object=o->next;			\
  }						\
						\
  if(o->next) o->next->prev=o->prev;		\
}while(0)


#define PIKE_XCONCAT(X,Y)	PIKE_CONCAT(X,Y)
#define PIKE_XCONCAT3(X,Y,Z)	PIKE_CONCAT(X,Y,Z)
#define PIKE_XCONCAT4(X,Y,Z,Q)	PIKE_CONCAT(X,Y,Z,Q)


/* Needed for fsort_template.h */
PMOD_EXPORT int my_log2(size_t x);

#endif
