/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: pike_memory.h,v 1.5 1998/10/09 17:56:32 hubbe Exp $
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "global.h"

#define MEMSEARCH_LINKS 512

typedef unsigned char p_wchar0;
typedef unsigned INT16 p_wchar1;
typedef unsigned INT32 p_wchar2;

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

#define BLOCK_ALLOC(X,Y)
#define MEMCHR0 MEMCHR

/* Prototypes begin here */
char *strdup(const char *str);
INLINE p_wchar1 *MEMCHR1(p_wchar1 *p,p_wchar1 c,INT32 e);
INLINE p_wchar2 *MEMCHR2(p_wchar2 *p,p_wchar2 c,INT32 e);
void swap(char *a, char *b, INT32 size);
void reverse(char *memory, INT32 nitems, INT32 size);
void reorder(char *memory, INT32 nitems, INT32 size,INT32 *order);
unsigned INT32 hashmem(const unsigned char *a,INT32 len,INT32 mlen);
unsigned INT32 hashstr(const unsigned char *str,INT32 maxn);
void init_memsearch(struct mem_searcher *s,
		    char *needle,
		    SIZE_T needlelen,
		    SIZE_T max_haystacklen);
char *memory_search(struct mem_searcher *s,
		    char *haystack,
		    SIZE_T haystacklen);
void init_generic_memsearcher(struct generic_mem_searcher *s,
			      void *needle,
			      SIZE_T needlelen,
			      char needle_shift,
			      SIZE_T estimated_haystack,
			      char haystack_shift);
void *generic_memory_search(struct generic_mem_searcher *s,
			    void *haystack,
			    SIZE_T haystacklen,
			    char haystack_shift);
char *my_memmem(char *needle,
		SIZE_T needlelen,
		char *haystack,
		SIZE_T haystacklen);
void memfill(char *to,
	     INT32 tolen,
	     char *from,
	     INT32 fromlen,
	     INT32 offset);
char *debug_xalloc(long size);
struct fileloc;
BLOCK_ALLOC(fileloc, 4090)
struct memloc;
BLOCK_ALLOC(memloc, 16382)
struct memhdr;
char *do_pad(char *mem, long size);
void check_pad(struct memhdr *mh, int freeok);
void low_add_marks_to_memhdr(struct memhdr *to,
			     struct memhdr *from);
void add_marks_to_memhdr(struct memhdr *to, void *ptr);
BLOCK_ALLOC(memhdr,16382)




static struct memhdr *find_memhdr(void *p);
void *debug_malloc(size_t s, const char *fn, int line);
void *debug_calloc(size_t a, size_t b, const char *fn, int line);
void *debug_realloc(void *p, size_t s, const char *fn, int line);
void debug_free(void *p, const char *fn, int line);
char *debug_strdup(const char *s, const char *fn, int line);
void dump_memhdr_locations(struct memhdr *from,
			   struct memhdr *notfrom);
void cleanup_memhdrs();
void reset_debug_malloc(void);
/* Prototypes end here */

#undef BLOCK_ALLOC

#endif
