/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: pike_macros.h,v 1.14 2000/04/08 01:09:33 hubbe Exp $
 */
#ifndef MACROS_H
#define MACROS_H

#include <global.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "pike_memory.h"

#define OFFSETOF(str_type, field) ((long)& (((struct str_type *)0)->field))
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

#define ALIGNOF(X) ((int)&(((struct { char ignored_ ; X fooo_; } *)0)->fooo_))

#define DO_ALIGN(X,Y) (((long)(X)+((Y)-1)) & -(Y))
#define CONSTANT_STRLEN(X) (sizeof(X) - sizeof(""))

#define SET_NEXT_AND_FREE(p,free_program) do{	\
  next=p->next;					\
  while(p->refs == 1 && (next=p->next))		\
  {						\
    add_ref(next);				\
    free_program(p);				\
    p=next;					\
  }						\
  free_program(p);				\
}while(0)
  
#endif
