/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

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
  hubbe_search
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

/* Prototypes begin here */
char *xalloc(SIZE_T size);
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
char *my_memmem(char *needle,
		SIZE_T needlelen,
		char *haystack,
		SIZE_T haystacklen);
void memfill(char *to,
	     INT32 tolen,
	     char *from,
	     INT32 fromlen,
	     INT32 offset);
/* Prototypes end here */

#endif
