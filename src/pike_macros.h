/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_macros.h,v 1.39 2005/03/15 09:58:35 grubba Exp $
*/

#ifndef MACROS_H
#define MACROS_H

#include <global.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "pike_memory.h"

#define PTR_TO_INT(PTR) ((size_t) ((char *) (PTR) - (char *) NULL))

#define OFFSETOF(str_type, field) \
  PTR_TO_INT(& (((struct str_type *)NULL)->field))
#define BASEOF(ptr, str_type, field)  \
  ((struct str_type *)((char*)ptr - OFFSETOF(str_type, field)))
#ifdef __cplusplus
extern "C++" {
    template<typename T> static inline int low_alignof_(T *ignored)
    {
	struct { char x; T y;} *bar = NULL;
	return PTR_TO_INT(&bar->y);
    }
};
#define ALIGNOF(X) low_alignof_((X*)NULL)
#else
#define ALIGNOF(X) OFFSETOF({ char ignored_; X fooo_;}, fooo_)
#endif
/* #define ALIGNOF(X) PTR_TO_INT(&(((struct { char ignored_ ; X fooo_; } *)NULL)->fooo_)) */

#define NELEM(a) (sizeof (a) / sizeof ((a)[0]))
#define ALLOC_STRUCT(X) ( (struct X *)xalloc(sizeof(struct X)) )

#define MINIMUM(X,Y) ((X)<(Y)?(X):(Y))
#define MAXIMUM(X,Y) ((X)>(Y)?(X):(Y))


PMOD_EXPORT extern const char Pike_is8bitalnum_vector[];
#define is8bitalnum(X)	(Pike_is8bitalnum_vector[((unsigned)(X))&0xff] == '1')
  
#define isidchar(X) is8bitalnum(X)

#ifndef HAVE_ISGRAPH
#define isgraph(X)	(ispunct(X) || isupper(X) || islower(X) || isdigit(X))
#endif /* !HAVE_ISGRAPH */



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
/* how can SET_NEXT_AND_FREE leave *next* pointing to a deallocated block?
 * -Hubbe
 */
/* Afaik it doesn't, but it leaves no value that is usable for the
 * purposes this macro was made for. /mast */
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
  debug_malloc_touch(o);                        \
  o->next=first_object;				\
  o->prev=0;					\
  if(first_object) first_object->prev=o;	\
  first_object=o;				\
}while(0)

#define DOUBLEUNLINK(first_object,o) do{	\
  debug_malloc_touch(o);                        \
  if(o->prev) {					\
    o->prev->next=o->next;			\
  }else {					\
    DO_IF_DEBUG(				\
      if(first_object != o) {			\
        describe(o);                            \
        Pike_fatal("Linked in wrong list!\n");	\
      }                                         \
    )						\
    first_object=o->next;			\
  }						\
						\
  if(o->next) o->next->prev=o->prev;		\
}while(0)


#define PIKE_XCONCAT(X,Y)	PIKE_CONCAT(X,Y)
#define PIKE_XCONCAT3(X,Y,Z)	PIKE_CONCAT(X,Y,Z)
#define PIKE_XCONCAT4(X,Y,Z,Q)	PIKE_CONCAT(X,Y,Z,Q)

/* Useful to get a literal comma in an argument to a macro. */
#define COMMA ,

/* Needed for fsort_template.h */
PMOD_EXPORT int my_log2(size_t x);

#endif
