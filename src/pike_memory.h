/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: pike_memory.h,v 1.14 2000/07/28 17:16:55 hubbe Exp $
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "global.h"
#include "stralloc.h"

#define MEMSEARCH_LINKS 512

struct link
{
  struct link *next;
  INT32 key, offset;
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
  SIZE_T needlelen;
  unsigned INT32 hsize, max;
  struct link links[MEMSEARCH_LINKS];
  struct link *set[MEMSEARCH_LINKS];
};

struct generic_mem_searcher
{
  char needle_shift;
  char haystack_shift;
  union data_u
  {
    struct mem_searcher eightbit;
    struct other_search_s
    {
      enum methods method;
      void *needle;
      SIZE_T needlelen;
      int first_char;
    } other;
  } data;
};

#include "block_alloc_h.h"
#define MEMCHR0 MEMCHR

/* Note to self: Prototypes must be updated manually /Hubbe */
PMOD_EXPORT int pcharp_memcmp(PCHARP a, PCHARP b, int sz);
PMOD_EXPORT long pcharp_strlen(PCHARP a);
PMOD_EXPORT INLINE p_wchar1 *MEMCHR1(p_wchar1 *p,p_wchar1 c,INT32 e);
PMOD_EXPORT INLINE p_wchar2 *MEMCHR2(p_wchar2 *p,p_wchar2 c,INT32 e);
PMOD_EXPORT void swap(char *a, char *b, INT32 size);
PMOD_EXPORT void reverse(char *memory, INT32 nitems, INT32 size);
PMOD_EXPORT void reorder(char *memory, INT32 nitems, INT32 size,INT32 *order);
PMOD_EXPORT unsigned INT32 hashmem(const unsigned char *a,INT32 len,INT32 mlen);
PMOD_EXPORT unsigned INT32 hashstr(const unsigned char *str,INT32 maxn);
PMOD_EXPORT unsigned INT32 simple_hashmem(const unsigned char *str,INT32 len, INT32 maxn);
PMOD_EXPORT void init_memsearch(struct mem_searcher *s,
		    char *needle,
		    SIZE_T needlelen,
		    SIZE_T max_haystacklen);
PMOD_EXPORT char *memory_search(struct mem_searcher *s,
		    char *haystack,
		    SIZE_T haystacklen);
PMOD_EXPORT void init_generic_memsearcher(struct generic_mem_searcher *s,
			      void *needle,
			      SIZE_T needlelen,
			      char needle_shift,
			      SIZE_T estimated_haystack,
			      char haystack_shift);
PMOD_EXPORT void *generic_memory_search(struct generic_mem_searcher *s,
			    void *haystack,
			    SIZE_T haystacklen,
			    char haystack_shift);
PMOD_EXPORT char *my_memmem(char *needle,
		SIZE_T needlelen,
		char *haystack,
		SIZE_T haystacklen);
PMOD_EXPORT void memfill(char *to,
	     INT32 tolen,
	     char *from,
	     INT32 fromlen,
	     INT32 offset);
PMOD_EXPORT char *debug_xalloc(long size);

#undef BLOCK_ALLOC

#endif
