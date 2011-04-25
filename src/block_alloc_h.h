/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#undef BLOCK_ALLOC
#undef PTR_HASH_ALLOC
#undef PTR_HASH_ALLOC_FIXED
#undef BLOCK_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FIXED_FILL_PAGES

#define BLOCK_ALLOC(DATA,SIZE)						\
struct DATA *PIKE_CONCAT(alloc_,DATA)(void);				\
void PIKE_CONCAT3(really_free_,DATA,_unlocked)(struct DATA *d);		\
void PIKE_CONCAT(really_free_,DATA)(struct DATA *d);			\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void);			\
void PIKE_CONCAT3(count_memory_in_,DATA,s)(INT32 *num, INT32 *size);	\
void PIKE_CONCAT3(init_,DATA,_blocks)(void);				\


#define PTR_HASH_ALLOC(DATA,BSIZE)				\
BLOCK_ALLOC(DATA,BSIZE)						\
extern struct DATA **PIKE_CONCAT(DATA,_hash_table);		\
extern size_t PIKE_CONCAT(DATA,_hash_table_size);		\
struct DATA *PIKE_CONCAT(find_,DATA)(void *ptr);		\
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, size_t hval); \
struct DATA *PIKE_CONCAT(make_,DATA)(void *ptr);		\
struct DATA *PIKE_CONCAT(get_,DATA)(void *ptr);			\
int PIKE_CONCAT3(check_,DATA,_semafore)(void *ptr);		\
void PIKE_CONCAT(move_,DATA)(struct DATA *block, void *new_ptr); \
int PIKE_CONCAT(remove_,DATA)(void *ptr);			\
void PIKE_CONCAT3(low_init_,DATA,_hash)(size_t);		\
void PIKE_CONCAT3(init_,DATA,_hash)(void);			\
void PIKE_CONCAT3(exit_,DATA,_hash)(void);			\

#define PTR_HASH_ALLOC_FIXED(DATA,BSIZE)			\
PTR_HASH_ALLOC(DATA,BSIZE);

#define BLOCK_ALLOC_FILL_PAGES(DATA,PAGES) BLOCK_ALLOC(DATA, n/a)
#define PTR_HASH_ALLOC_FILL_PAGES(DATA,PAGES) PTR_HASH_ALLOC(DATA, n/a)
#define PTR_HASH_ALLOC_FIXED_FILL_PAGES(DATA,PAGES) PTR_HASH_ALLOC_FIXED(DATA, n/a)

#define PTR_HASH_LOOP(DATA,HVAL,PTR)					\
  for ((HVAL) = PIKE_CONCAT(DATA,_hash_table_size); (HVAL)-- > 0;)	\
    for ((PTR) = PIKE_CONCAT(DATA,_hash_table)[HVAL];			\
	 (PTR); (PTR) = (PTR)->BLOCK_ALLOC_NEXT)

/* The name of a member in the BLOCK_ALLOC struct big enough to
 * contain a void * (used for the free list). */
#define BLOCK_ALLOC_NEXT next

/* The name of a void * member in the PTR_HASH_ALLOC struct containing
 * the key. */
#define PTR_HASH_ALLOC_DATA data
