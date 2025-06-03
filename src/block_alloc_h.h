/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#undef BLOCK_ALLOC
#undef PTR_HASH_ALLOC
#undef PTR_HASH_ALLOC_FIXED
#undef BLOCK_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FIXED_FILL_PAGES

#ifndef PIKE_HASH_T
#define PIKE_HASH_T	size_t
#endif /* !PIKE_HASH_T */

#ifdef BLOCK_ALLOC_USED
#error "block_alloc.h must be included after all uses of block_alloc_h.h"
#endif /* BLOCK_ALLOC_USED */

#define BLOCK_ALLOC(DATA,SIZE)						\
struct DATA *PIKE_CONCAT(alloc_,DATA)(void);				\
PMOD_EXPORT void PIKE_CONCAT(really_free_,DATA)(struct DATA *d);			\
void PIKE_CONCAT3(count_memory_in_,DATA,s)(size_t *num, size_t *size);


#define PTR_HASH_ALLOC(DATA,BSIZE)				\
BLOCK_ALLOC(DATA,BSIZE);					\
extern struct DATA **PIKE_CONCAT(DATA,_hash_table);		\
extern size_t PIKE_CONCAT(DATA,_hash_table_size);		\
struct DATA *PIKE_CONCAT(find_,DATA)(const void *ptr);		\
struct DATA *PIKE_CONCAT(make_,DATA)(const void *ptr);		\
struct DATA *PIKE_CONCAT(get_,DATA)(const void *ptr);		\
int PIKE_CONCAT3(check_,DATA,_semaphore)(const void *ptr);	\
void PIKE_CONCAT(move_,DATA)(struct DATA *block, void *new_ptr); \
int PIKE_CONCAT(remove_,DATA)(void *ptr);

#define PTR_HASH_ALLOC_FIXED(DATA,BSIZE)			\
PTR_HASH_ALLOC(DATA,BSIZE)

#define BLOCK_ALLOC_FILL_PAGES(DATA,PAGES) BLOCK_ALLOC(DATA, n/a)
#define PTR_HASH_ALLOC_FILL_PAGES(DATA,PAGES) PTR_HASH_ALLOC(DATA, n/a)
#define PTR_HASH_ALLOC_FIXED_FILL_PAGES(DATA,PAGES) PTR_HASH_ALLOC_FIXED(DATA, n/a)

#define PTR_HASH_LOOP(DATA,HVAL,PTR)					\
  for ((HVAL) = (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size); (HVAL)-- > 0;)	\
    for ((PTR) = PIKE_CONCAT(DATA,_hash_table)[HVAL];			\
	 (PTR); (PTR) = (PTR)->BLOCK_ALLOC_NEXT)

/* The name of a member in the BLOCK_ALLOC struct big enough to
 * contain a void * (used for the free list). */
#define BLOCK_ALLOC_NEXT next

/* The name of a void * member in the PTR_HASH_ALLOC struct containing
 * the key. */
#define PTR_HASH_ALLOC_DATA data
