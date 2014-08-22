/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#undef INIT_BLOCK
#undef EXIT_BLOCK
#undef BLOCK_ALLOC
#undef LOW_PTR_HASH_ALLOC
#undef PTR_HASH_ALLOC_FIXED
#undef PTR_HASH_ALLOC
#undef BLOCK_ALLOC_HSIZE_SHIFT
#undef BLOCK_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FIXED_FILL_PAGES

#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)
#define BLOCK_ALLOC_HSIZE_SHIFT 2
#ifndef PIKE_HASH_T
#define PIKE_HASH_T	size_t
#endif /* !PIKE_HASH_T */

#define BA_STATIC
#define BA_INLINE


#define PTR_HASH_ALLOC_FILL_PAGES(DATA, PAGES)				\
  PTR_HASH_ALLOC(DATA,							\
                 ((PIKE_MALLOC_PAGE_SIZE * (PAGES))			\
                  - PIKE_MALLOC_OVERHEAD - BLOCK_HEADER_SIZE) /		\
                 sizeof (struct DATA))

#define PTR_HASH_ALLOC_FIXED_FILL_PAGES(DATA, PAGES)			\
  PTR_HASH_ALLOC_FIXED(DATA,						\
                       ((PIKE_MALLOC_PAGE_SIZE * (PAGES))		\
                        - PIKE_MALLOC_OVERHEAD - BLOCK_HEADER_SIZE) /	\
                       sizeof (struct DATA))

/* Size of the members in the block struct below that don't contain
 * the payload data (i.e. that aren't x). This can be used in BSIZE to
 * make the block fit within a page. */
#ifndef BLOCK_HEADER_SIZE
#define BLOCK_HEADER_SIZE (3 * sizeof (void *) + sizeof (INT32) \
			   DO_IF_DMALLOC( + sizeof(INT32)))
#endif

#ifndef PTR_HASH_HASHFUN_DEFINED
#define PTR_HASH_HASHFUN_DEFINED

static INLINE PIKE_HASH_T ptr_hashfun(void * ptr) {
  PIKE_HASH_T q = (size_t)((char*)ptr - (char*)0);
  q ^= (q >> 20) ^ (q >> 12);
  return q ^ (q >> 7) ^ (q >> 4);
}

static INLINE size_t ptr_hash_find_hashsize(size_t size) {
    if (size & (size-1)) {
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
        size |= size >> 32;
        size++;
    }
    return size;
}
#endif

#include "block_allocator.h"

#define LOW_PTR_HASH_ALLOC(DATA,BSIZE)					     \
									     \
static struct block_allocator PIKE_CONCAT(DATA,_allocator) =                 \
    BA_INIT(sizeof(struct DATA), BSIZE);                                     \
                                                                             \
struct DATA *PIKE_CONCAT(alloc_,DATA)(void) {	                             \
  struct DATA * ptr = (struct DATA *)ba_alloc(&PIKE_CONCAT(DATA,_allocator));\
  INIT_BLOCK(ptr);                                                           \
  return ptr;                                                                \
}									     \
                                                                             \
void PIKE_CONCAT(really_free_,DATA)(struct DATA *d) {		             \
  EXIT_BLOCK(d);							     \
  ba_free(&PIKE_CONCAT(DATA,_allocator), d);                                 \
}                                                                            \
                                                                             \
void PIKE_CONCAT3(count_memory_in_,DATA,s)(size_t *num_, size_t *size_)	{    \
  ba_count_all(&PIKE_CONCAT(DATA,_allocator), num_, size_);                  \
}                                                                            \
									     \
struct DATA **PIKE_CONCAT(DATA,_hash_table)=0;				     \
size_t PIKE_CONCAT(DATA,_hash_table_size)=0;				     \
static size_t PIKE_CONCAT(num_,DATA)=0;					     \
									     \
static struct DATA *						     \
 PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(void *ptr,		     \
					       PIKE_HASH_T hval)	     \
{									     \
  struct DATA *p;							     \
  p=PIKE_CONCAT(DATA,_hash_table)[hval];                                     \
  if(!p || p->PTR_HASH_ALLOC_DATA == ptr)				     \
  {                                                                          \
    return p;                                                                \
  }                                                                          \
  while((p=p->BLOCK_ALLOC_NEXT))					     \
  {									     \
    if(p->PTR_HASH_ALLOC_DATA==ptr)					     \
    {									     \
      return p;								     \
    }									     \
  }									     \
  return 0;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(find_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;                                                            \
  PIKE_HASH_T hval = ptr_hashfun(ptr);			                     \
  if(!PIKE_CONCAT(DATA,_hash_table)) {                                       \
    return 0;                                                                \
  }                                                                          \
  hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;		     \
  p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval);		     \
  return p;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(make_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;							     \
  PIKE_HASH_T hval = ptr_hashfun(ptr);			                     \
  hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;		     \
  p=PIKE_CONCAT3(make_,DATA,_unlocked)(ptr,hval);                            \
  return p;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(get_,DATA)(void *ptr)			 	     \
{									     \
  struct DATA *p;							     \
  PIKE_HASH_T hval = ptr_hashfun(ptr);			                     \
  hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;		     \
  if(!(p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval)))          \
    p=PIKE_CONCAT3(make_,DATA,_unlocked)(ptr, hval);		             \
  return p;                                                                  \
}									     \
									     \
int PIKE_CONCAT3(check_,DATA,_semaphore)(void *ptr)			     \
{									     \
  PIKE_HASH_T hval = ptr_hashfun(ptr);			                     \
  hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;		     \
  if(PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval))		     \
  {                                                                          \
    return 0;								     \
  }                                                                          \
									     \
  PIKE_CONCAT3(make_,DATA,_unlocked)(ptr, hval);			     \
  return 1;								     \
}									     \
									     \
void PIKE_CONCAT(move_,DATA)(struct DATA *block, void *new_ptr)		     \
{									     \
  struct DATA **pp, *p;							     \
  PIKE_HASH_T hval = ptr_hashfun(block->PTR_HASH_ALLOC_DATA);		     \
  hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;		     \
  pp=PIKE_CONCAT(DATA,_hash_table) + hval;                                   \
  while((p = *pp))							     \
  {									     \
    if(p == block)							     \
    {									     \
      *pp = p->BLOCK_ALLOC_NEXT;					     \
      break;								     \
    }									     \
    pp = &p->BLOCK_ALLOC_NEXT;						     \
  }									     \
  if (!p) Pike_fatal("The block to move wasn't found.\n");		     \
  block->PTR_HASH_ALLOC_DATA = new_ptr;					     \
  hval = ptr_hashfun(new_ptr) &				                     \
    ((PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1);		     \
  block->BLOCK_ALLOC_NEXT = PIKE_CONCAT(DATA,_hash_table)[hval];	     \
  PIKE_CONCAT(DATA,_hash_table)[hval] = block;				     \
}									     \
									     \
int PIKE_CONCAT(remove_,DATA)(void *ptr)				     \
{									     \
  struct DATA **pp, *p;							     \
  PIKE_HASH_T hval = ptr_hashfun(ptr);			                     \
  if(!PIKE_CONCAT(DATA,_hash_table))                                         \
  {                                                                          \
    return 0;				                                     \
  }                                                                          \
  hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;		     \
  pp=PIKE_CONCAT(DATA,_hash_table) + hval;                                   \
  while((p = *pp))							     \
  {									     \
    if(p->PTR_HASH_ALLOC_DATA==ptr)					     \
    {									     \
      *pp = p->BLOCK_ALLOC_NEXT;					     \
      PIKE_CONCAT(num_,DATA)--;						     \
      PIKE_CONCAT(really_free_,DATA)(p);				     \
      return 1;								     \
    }									     \
    pp = &p->BLOCK_ALLOC_NEXT;						     \
  }									     \
  return 0;								     \
}									     \
									     \
static void PIKE_CONCAT3(low_init_,DATA,_hash)(size_t size)			     \
{									     \
  PIKE_CONCAT(DATA,_hash_table_size)=					     \
     ptr_hash_find_hashsize(size);                                           \
									     \
  PIKE_CONCAT(DATA,_hash_table)=(struct DATA **)			     \
    malloc(sizeof(struct DATA *)*PIKE_CONCAT(DATA,_hash_table_size));	     \
  if(!PIKE_CONCAT(DATA,_hash_table))					     \
  {									     \
    fprintf(stderr,"Fatal: out of memory.\n");				     \
    exit(17);								     \
  }									     \
  MEMSET(PIKE_CONCAT(DATA,_hash_table),0,				     \
	 sizeof(struct DATA *)*PIKE_CONCAT(DATA,_hash_table_size));	     \
}									     \
									     \
									     \
static void __attribute((unused)) PIKE_CONCAT3(init_,DATA,_hash)(void)       \
{									     \
  PIKE_CONCAT3(low_init_,DATA,_hash)(BSIZE);		                     \
}									     \
									     \
static void PIKE_CONCAT3(exit_,DATA,_hash)(void)				     \
{									     \
  ba_free_all(& PIKE_CONCAT(DATA,_allocator));                               \
  free(PIKE_CONCAT(DATA,_hash_table));					     \
  PIKE_CONCAT(DATA,_hash_table)=0;					     \
  PIKE_CONCAT(num_,DATA)=0;						     \
}

#define PTR_HASH_ALLOC_FIXED(DATA,BSIZE)				     \
static struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, PIKE_HASH_T hval);\
LOW_PTR_HASH_ALLOC(DATA,BSIZE)                                               \
									     \
static struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, PIKE_HASH_T hval) \
{									     \
  struct DATA *p;							     \
									     \
  DO_IF_DEBUG( if(!PIKE_CONCAT(DATA,_hash_table))			     \
    Pike_fatal("Hash table error!\n"); )				     \
  PIKE_CONCAT(num_,DATA)++;						     \
									     \
  p=PIKE_CONCAT(alloc_,DATA)();                                              \
  p->PTR_HASH_ALLOC_DATA=ptr;						     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}


#define PTR_HASH_ALLOC(DATA,BSIZE)					     \
static struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr,		     \
						PIKE_HASH_T hval);	     \
LOW_PTR_HASH_ALLOC(DATA,BSIZE)                                               \
                                                                             \
static void PIKE_CONCAT(DATA,_rehash)(void)				     \
{									     \
  /* Time to re-hash */							     \
  struct DATA **old_hash;	                                 	     \
  struct DATA *p;							     \
  PIKE_HASH_T hval;							     \
  size_t e;								     \
                                                                             \
  old_hash= PIKE_CONCAT(DATA,_hash_table);		                     \
  e=PIKE_CONCAT(DATA,_hash_table_size);			                     \
									     \
  PIKE_CONCAT(DATA,_hash_table_size) *= 2;				     \
  if((PIKE_CONCAT(DATA,_hash_table)=(struct DATA **)			     \
      malloc(PIKE_CONCAT(DATA,_hash_table_size)*			     \
	     sizeof(struct DATA *))))					     \
  {									     \
    MEMSET(PIKE_CONCAT(DATA,_hash_table),0,				     \
	   sizeof(struct DATA *)*PIKE_CONCAT(DATA,_hash_table_size));	     \
    while(e-- > 0)							     \
    {									     \
      while((p=old_hash[e]))						     \
      {									     \
	old_hash[e]=p->BLOCK_ALLOC_NEXT;				     \
	hval = ptr_hashfun(p->PTR_HASH_ALLOC_DATA);		             \
	hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;	     \
	p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];	     \
	PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
      }									     \
    }									     \
    free((char *)old_hash);						     \
  }else{								     \
    PIKE_CONCAT(DATA,_hash_table)=old_hash;				     \
    PIKE_CONCAT(DATA,_hash_table_size)=e;				     \
  }									     \
}									     \
									     \
static struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr,		     \
						PIKE_HASH_T hval)	     \
{									     \
  struct DATA *p;							     \
									     \
  DO_IF_DEBUG( if(!PIKE_CONCAT(DATA,_hash_table))			     \
    Pike_fatal("Hash table error!\n"); )				     \
  PIKE_CONCAT(num_,DATA)++;						     \
									     \
  if(( PIKE_CONCAT(num_,DATA)>>BLOCK_ALLOC_HSIZE_SHIFT ) >=		     \
     PIKE_CONCAT(DATA,_hash_table_size))				     \
  {									     \
    PIKE_CONCAT(DATA,_rehash)();					     \
    hval = ptr_hashfun(ptr);				                     \
    hval &= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size) - 1;	     \
  }									     \
									     \
  p=PIKE_CONCAT(alloc_,DATA)();                                              \
  p->PTR_HASH_ALLOC_DATA=ptr;						     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}
