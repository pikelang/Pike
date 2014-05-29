/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MEMORY_H
#define MEMORY_H

#include "global.h"
#include "stralloc.h"

#ifdef USE_VALGRIND

#define HAVE_VALGRIND_MACROS
/* Assume that any of the following header files have the macros we
 * need. Haven't checked if it's true or not. */

#ifdef HAVE_MEMCHECK_H
#include <memcheck.h>
#elif defined(HAVE_VALGRIND_MEMCHECK_H)
#include <valgrind/memcheck.h>
#elif defined(HAVE_VALGRIND_H)
#include <valgrind.h>
#else
#undef HAVE_VALGRIND_MACROS
#endif

#endif	/* USE_VALGRIND */

#ifdef HAVE_VALGRIND_MACROS

#ifndef VALGRIND_MAKE_MEM_NOACCESS
#define VALGRIND_MAKE_MEM_NOACCESS VALGRIND_MAKE_NOACCESS
#define VALGRIND_MAKE_MEM_UNDEFINED VALGRIND_MAKE_WRITABLE
#define VALGRIND_MAKE_MEM_DEFINED VALGRIND_MAKE_READABLE
#endif

/* No Access */
#define PIKE_MEM_NA(lvalue) do {					\
    PIKE_MEM_NA_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_NA_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_MEM_NOACCESS(addr, bytes));		\
  } while (0)

/* Write Only -- Will become RW when having been written to */
#define PIKE_MEM_WO(lvalue) do {					\
    PIKE_MEM_WO_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_WO_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_MEM_UNDEFINED(addr, bytes));		\
  } while (0)

/* Read/Write */
#define PIKE_MEM_RW(lvalue) do {					\
    PIKE_MEM_RW_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_RW_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_MEM_DEFINED(addr, bytes));		\
  } while (0)

/* Read Only -- Not currently supported by valgrind */
#define PIKE_MEM_RO(lvalue) do {					\
    PIKE_MEM_RO_RANGE(&(lvalue), sizeof (lvalue));			\
  } while (0)
#define PIKE_MEM_RO_RANGE(addr, bytes) do {				\
    VALGRIND_DISCARD(VALGRIND_MAKE_MEM_DEFINED(addr, bytes));		\
  } while (0)

/* Return true if a memchecker is in use. */
#define PIKE_MEM_CHECKER() RUNNING_ON_VALGRIND

/* Return true if a memchecker reports the memory to not be
 * addressable (might also print debug messages etc). */
#define PIKE_MEM_NOT_ADDR(lvalue)					\
  PIKE_MEM_NOT_ADDR_RANGE(&(lvalue), sizeof (lvalue))
#define PIKE_MEM_NOT_ADDR_RANGE(addr, bytes)				\
  VALGRIND_CHECK_MEM_IS_ADDRESSABLE(addr, bytes)

/* Return true if a memchecker reports the memory to not be defined
 * (might also print debug messages etc). */
#define PIKE_MEM_NOT_DEF(lvalue)					\
  PIKE_MEM_NOT_DEF_RANGE(&(lvalue), sizeof (lvalue))
#define PIKE_MEM_NOT_DEF_RANGE(addr, bytes)				\
  VALGRIND_CHECK_MEM_IS_DEFINED(addr, bytes)

#ifdef VALGRIND_CREATE_MEMPOOL
# define PIKE_MEMPOOL_CREATE(a)         VALGRIND_CREATE_MEMPOOL(a, 0, 0)
# define PIKE_MEMPOOL_ALLOC(a, p, l)    VALGRIND_MEMPOOL_ALLOC(a, p, l)
# define PIKE_MEMPOOL_FREE(a, p, l)     VALGRIND_MEMPOOL_FREE(a, p)
# define PIKE_MEMPOOL_DESTROY(a)        VALGRIND_DESTROY_MEMPOOL(a)
#else
/* somewhat functional alternatives to mempool macros */
# define PIKE_MEMPOOL_CREATE(a)
# define PIKE_MEMPOOL_ALLOC(a, p, l)    PIKE_MEM_WO_RANGE(p, l)
# define PIKE_MEMPOOL_FREE(a, p, l)     PIKE_MEM_NA_RANGE(p, l)
# define PIKE_MEMPOOL_DESTROY(a)
#endif

#else  /* !HAVE_VALGRIND_MACROS */

#define PIKE_MEM_NA(lvalue)		do {} while (0)
#define PIKE_MEM_NA_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_WO(lvalue)		do {} while (0)
#define PIKE_MEM_WO_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_RW(lvalue)		do {} while (0)
#define PIKE_MEM_RW_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_RO(lvalue)		do {} while (0)
#define PIKE_MEM_RO_RANGE(addr, bytes)	do {} while (0)
#define PIKE_MEM_CHECKER()		0
#define PIKE_MEM_NOT_ADDR(lvalue)	0
#define PIKE_MEM_NOT_ADDR_RANGE(addr, bytes) 0
#define PIKE_MEM_NOT_DEF(lvalue)	0
#define PIKE_MEM_NOT_DEF_RANGE(addr, bytes) 0
#define PIKE_MEMPOOL_CREATE(a)
#define PIKE_MEMPOOL_ALLOC(a, p, l)
#define PIKE_MEMPOOL_FREE(a, p, l)
#define PIKE_MEMPOOL_DESTROY(a)

#endif	/* !HAVE_VALGRIND_MACROS */


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

/*
 * The purpose of this function is to avoid dead store elimination in cases when
 * sensitive data has to be cleared from memory.
 */
static INLINE void __attribute__((unused)) * guaranteed_memset(void * p, int c, size_t n) {
    volatile char * _p = (char *)p;
    while (n--) *_p++ = c;
    return (void *)p;
}

static INLINE unsigned INT64 __attribute__((unused)) get_unaligned64(const void * ptr) {
    unsigned INT64 v;
    memcpy(&v, ptr, 8);
    return v;
}

static INLINE void __attribute__((unused)) set_unaligned64(void * ptr, unsigned INT64 v) {
    memcpy(ptr, &v, 8);
}

static INLINE unsigned INT64 __attribute__((unused)) get_unaligned32(const void * ptr) {
    unsigned INT32 v;
    memcpy(&v, ptr, 4);
    return v;
}

static INLINE void __attribute__((unused)) set_unaligned32(void * ptr, unsigned INT32 v) {
    memcpy(ptr, &v, 4);
}

static INLINE unsigned INT16 __attribute__((unused)) get_unaligned16(const void * ptr) {
    unsigned INT16 v;
    memcpy(&v, ptr, 2);
    return v;
}

static INLINE void __attribute__((unused)) set_unaligned16(void * ptr, unsigned INT16 v) {
    memcpy(ptr, &v, 2);
}

#include "pike_search.h"

#include "block_alloc_h.h"
extern int page_size;

/* Note to self: Prototypes must be updated manually /Hubbe */
PMOD_EXPORT ptrdiff_t pcharp_memcmp(PCHARP a, PCHARP b, int sz) ATTRIBUTE((pure));
PMOD_EXPORT long pcharp_strlen(PCHARP a)  ATTRIBUTE((pure));

#define MEMCHR0 MEMCHR
p_wchar1 *MEMCHR1(p_wchar1 *p, p_wchar2 c, ptrdiff_t e)  ATTRIBUTE((pure));
p_wchar2 *MEMCHR2(p_wchar2 *p, p_wchar2 c, ptrdiff_t e)  ATTRIBUTE((pure));

/* PMOD_EXPORT void swap(char *a, char *b, size_t size); */
PMOD_EXPORT void reverse(char *memory, size_t nitems, size_t size);
PMOD_EXPORT void reorder(char *memory, INT32 nitems, INT32 size,INT32 *order);

#if (defined(__i386__) || defined(__amd64__)) && defined(__GNUC__)
extern PMOD_EXPORT
#ifdef __i386__
__attribute__((fastcall)) 
#endif
size_t (*low_hashmem)(const void *, size_t, size_t, size_t);
#else
PMOD_EXPORT size_t low_hashmem(const void *, size_t len, size_t mlen, size_t key) ATTRIBUTE((pure));
#endif
PMOD_EXPORT size_t hashmem(const void *, size_t len, size_t mlen) ATTRIBUTE((pure));
/*
PMOD_EXPORT void memfill(char *to,
	     INT32 tolen,
	     char *from,
	     INT32 fromlen,
	     INT32 offset);
*/
#define MALLOC_FUNCTION  ATTRIBUTE((malloc)) ATTRIBUTE((warn_unused_result))

PMOD_EXPORT void *debug_xalloc(size_t size) MALLOC_FUNCTION;
PMOD_EXPORT void *debug_xmalloc(size_t s) MALLOC_FUNCTION;
PMOD_EXPORT void debug_xfree(void *mem);
PMOD_EXPORT void *debug_xrealloc(void *m, size_t s) MALLOC_FUNCTION;
PMOD_EXPORT void *debug_xcalloc(size_t n, size_t s) MALLOC_FUNCTION;
PMOD_EXPORT void *aligned_alloc(size_t size, size_t alignment) MALLOC_FUNCTION;

#define PIKE_ALIGNTO(x, a)	(((x) + (a)-1) & ~((a)-1))

PMOD_EXPORT void *mexec_alloc(size_t sz) MALLOC_FUNCTION;
PMOD_EXPORT void *mexec_realloc(void *ptr, size_t sz) MALLOC_FUNCTION;
PMOD_EXPORT void mexec_free(void *ptr);
void init_pike_memory (void);
void exit_pike_memory (void);

#ifdef DEBUG_MALLOC
PMOD_EXPORT void * system_malloc(size_t) MALLOC_FUNCTION;
PMOD_EXPORT void system_free(void *);
#endif

#undef BLOCK_ALLOC

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#define DO_IF_ELSE_UNALIGNED_MEMORY_ACCESS(IF, ELSE)	IF
#else /* !HANDLES_UNALIGNED_MEMORY_ACCESS */
#define DO_IF_ELSE_UNALIGNED_MEMORY_ACCESS(IF, ELSE)	ELSE
#endif /* HANDLES_UNALIGNED_MEMORY_ACCESS */

/* Determine if we should use our own heap manager for executable
 * memory. */
#if defined(MEXEC_USES_MMAP) || defined (_WIN32)
#define USE_MY_MEXEC_ALLOC
#endif

#endif
