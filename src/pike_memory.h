/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_memory.h,v 1.49 2004/03/24 20:05:34 grubba Exp $
*/

#ifndef MEMORY_H
#define MEMORY_H

#include "global.h"
#include "stralloc.h"

#ifdef USE_VALGRIND

#ifdef HAVE_MEMCHECK_H
#include <memcheck.h>
#elif defined(HAVE_VALGRIND_MEMCHECK_H)
#include <valgrind/memcheck.h>
#elif defined(HAVE_VALGRIND_H)
#include <valgrind.h>
#endif

/* No Access */
#define PIKE_MEM_NA(lvalue) do {					\
    PIKE_MEM_NA_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_NA_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_NOACCESS(addr, bytes));		\
  } while (0)

/* Write Only -- Will become RW when having been written to */
#define PIKE_MEM_WO(lvalue) do {					\
    PIKE_MEM_WO_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_WO_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_WRITABLE(addr, bytes));		\
  } while (0)

/* Read/Write */
#define PIKE_MEM_RW(lvalue) do {					\
    PIKE_MEM_RW_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_RW_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_READABLE(addr, bytes));		\
  } while (0)

/* Read Only -- Not currently supported by valgrind */
#define PIKE_MEM_RO(lvalue) do {					\
    PIKE_MEM_RO_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_RO_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_READABLE(addr, bytes));		\
  } while (0)

/* Return true if a memchecker is in use. */
#define PIKE_MEM_CHECKER() RUNNING_ON_VALGRIND

#else

#define PIKE_MEM_NA(lvalue)		do {} while (0)
#define PIKE_MEM_NA_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_WO(lvalue)		do {} while (0)
#define PIKE_MEM_WO_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_RW(lvalue)		do {} while (0)
#define PIKE_MEM_RW_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_RO(lvalue)		do {} while (0)
#define PIKE_MEM_RO_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_CHECKER()		0

#endif /* USE_VALGRIND */


#define MEMSEARCH_LINKS 512

struct link
{
  struct link *next;
  INT32 key;
  ptrdiff_t offset;
};

enum methods {
  no_search,
  use_memchr,
  memchr_and_memcmp,
  hubbe_search,
  boyer_moore
};

struct mem_searcher
{
  enum methods method;
  char *needle;
  size_t needlelen;
  size_t hsize, max;
  struct link links[MEMSEARCH_LINKS];
  struct link *set[MEMSEARCH_LINKS];
};


#include "pike_search.h"

#include "block_alloc_h.h"
#define MEMCHR0 MEMCHR

/* Note to self: Prototypes must be updated manually /Hubbe */
PMOD_EXPORT ptrdiff_t pcharp_memcmp(PCHARP a, PCHARP b, int sz);
PMOD_EXPORT long pcharp_strlen(PCHARP a);
PMOD_EXPORT p_wchar1 *MEMCHR1(p_wchar1 *p, p_wchar2 c, ptrdiff_t e);
PMOD_EXPORT p_wchar2 *MEMCHR2(p_wchar2 *p, p_wchar2 c, ptrdiff_t e);
PMOD_EXPORT void swap(char *a, char *b, size_t size);
PMOD_EXPORT void reverse(char *memory, size_t nitems, size_t size);
PMOD_EXPORT void reorder(char *memory, INT32 nitems, INT32 size,INT32 *order);
PMOD_EXPORT size_t hashmem(const unsigned char *a, size_t len, size_t mlen);
PMOD_EXPORT size_t hashstr(const unsigned char *str, ptrdiff_t maxn);
PMOD_EXPORT size_t simple_hashmem(const unsigned char *str, ptrdiff_t len, ptrdiff_t maxn);
PMOD_EXPORT size_t simple_hashmem1(const p_wchar1 *str, ptrdiff_t len, ptrdiff_t maxn);
PMOD_EXPORT size_t simple_hashmem2(const p_wchar2 *str, ptrdiff_t len, ptrdiff_t maxn);
PMOD_EXPORT void memfill(char *to,
	     INT32 tolen,
	     char *from,
	     INT32 fromlen,
	     INT32 offset);
PMOD_EXPORT void *debug_xalloc(size_t size);
PMOD_EXPORT void *debug_xmalloc(size_t s);
PMOD_EXPORT void debug_xfree(void *mem);
PMOD_EXPORT void *debug_xrealloc(void *m, size_t s);
PMOD_EXPORT void *debug_xcalloc(size_t n, size_t s);

#undef BLOCK_ALLOC

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#define DO_IF_ELSE_UNALIGNED_MEMORY_ACCESS(IF, ELSE)	IF
#else /* !HANDLES_UNALIGNED_MEMORY_ACCESS */
#define DO_IF_ELSE_UNALIGNED_MEMORY_ACCESS(IF, ELSE)	ELSE
#endif /* HANDLES_UNALIGNED_MEMORY_ACCESS */

#if SIZEOF_CHAR_P == 4
#define DIVIDE_BY_2_CHAR_P(X)	(X >>= 3)
#else /* sizeof(char *) != 4 */
#if SIZEOF_CHAR_P == 8
#define DIVIDE_BY_2_CHAR_P(X)	(X >>= 4)
#else /* sizeof(char *) != 8 */
#define DIVIDE_BY_2_CHAR_P(X)	(X /= 2*sizeof(size_t))
#endif /* sizeof(char *) == 8 */
#endif /* sizeof(char *) == 4 */

/* NB: RET should be an lvalue of type size_t. */
#define DO_HASHMEM(RET, A, LEN, MLEN)			\
  do {							\
    const unsigned char *a = A;				\
    size_t len = LEN;					\
    size_t mlen = MLEN;					\
    size_t ret;						\
  							\
    ret = 9248339*len;					\
    if(len<mlen)					\
      mlen=len;						\
    else						\
    {							\
      switch(len-mlen)					\
      {							\
  	default: ret^=(ret<<6) + a[len-7];		\
  	case 7:						\
  	case 6: ret^=(ret<<7) + a[len-5];		\
  	case 5:						\
  	case 4: ret^=(ret<<4) + a[len-4];		\
  	case 3: ret^=(ret<<3) + a[len-3];		\
  	case 2: ret^=(ret<<3) + a[len-2];		\
  	case 1: ret^=(ret<<3) + a[len-1];		\
      }							\
    }							\
    a += mlen & 7;					\
    switch(mlen&7)					\
    {							\
      case 7: ret^=a[-7];				\
      case 6: ret^=(ret<<4)+a[-6];			\
      case 5: ret^=(ret<<7)+a[-5];			\
      case 4: ret^=(ret<<6)+a[-4];			\
      case 3: ret^=(ret<<3)+a[-3];			\
      case 2: ret^=(ret<<7)+a[-2];			\
      case 1: ret^=(ret<<5)+a[-1];			\
    }							\
  							\
    DO_IF_ELSE_UNALIGNED_MEMORY_ACCESS(			\
      {							\
  	size_t *b;					\
  	b=(size_t *)a;					\
    							\
  	for(DIVIDE_BY_2_CHAR_P(mlen);mlen--;)		\
  	{						\
  	  ret^=(ret<<7)+*(b++);				\
  	  ret^=(ret>>6)+*(b++);				\
  	}						\
      }							\
    ,							\
      for(mlen >>= 3; mlen--;)				\
      {							\
  	register size_t t1;				\
  	register size_t t2;				\
  	t1= a[0];					\
  	t2= a[1];					\
  	t1=(t1<<5) + a[2];				\
  	t2=(t2<<4) + a[3];				\
  	t1=(t1<<7) + a[4];				\
  	t2=(t2<<5) + a[5];				\
  	t1=(t1<<3) + a[6];				\
  	t2=(t2<<4) + a[7];				\
  	a += 8;						\
  	ret^=(ret<<7) + (ret>>6) + t1 + (t2<<6);	\
      }							\
    )							\
  							\
    RET = ret;						\
  } while(0)

#endif
