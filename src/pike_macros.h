/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MACROS_H
#define MACROS_H

#include "global.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "pike_memory.h"

#define PTR_TO_INT(PTR) ((size_t) ((char *) (PTR)))

#if __GNUC__ >= 4
#  define OFFSETOF(T,X) __builtin_offsetof(struct T,X)
#  define ALIGNOF(X) __alignof__(X)
#else
#  define OFFSETOF(T, X) PTR_TO_INT(& (((struct T *)NULL)->X))
#  define ALIGNOF(X) OFFSETOF({ char ignored_; X fooo_;}, fooo_)
#endif

#define BASEOF(ptr, str_type, field) \
   ((struct str_type *)((char*)ptr - OFFSETOF(str_type, field)))

#define NELEM(a) (sizeof (a) / sizeof ((a)[0]))
#define ALLOC_STRUCT(X) ( (struct X *)xalloc(sizeof(struct X)) )

#define MINIMUM(X,Y) ((X)<(Y)?(X):(Y))
#define MAXIMUM(X,Y) ((X)>(Y)?(X):(Y))

#define DO_ALIGN(X,Y) (((size_t)(X)+((Y)-1)) & ~((Y)-1))
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

/* Useful to get a literal comma in an argument to a macro. */
#define COMMA ,

/* Necessary to pass an empty argument to a macro for some preprocessors. */
#define NOTHING
#endif
