#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H
#include <stdint.h>

typedef struct ba_page * ba_page;

struct block_allocator {
    uint32_t block_size;
    uint32_t blocks;
    uint32_t num_pages;
    uint16_t first, last; 
    uint32_t magnitude;
#ifdef BA_SEGREGATE
    ba_page pages;
#else
    void * pages;
#endif
    uint16_t * htable;
    uint16_t allocated;
#ifdef BA_SEGREGATE
    uint16_t last_free;
#else
    uint16_t last_free, ba_page_size;
#endif
};

#ifdef BA_SEGREGATE
#define BA_INIT(block_size, blocks) { block_size, blocks, 0, NULL, NULL, \
				      0, NULL, NULL, 0, 0 }
#else
#define BA_INIT(block_size, blocks) { block_size, blocks, 0, NULL, NULL, \
				      0, NULL, NULL, 0, 0, 0 }
#endif

void ba_init(struct block_allocator * a, uint32_t block_size, uint32_t blocks);
void * ba_alloc(struct block_allocator * a);
void ba_free(struct block_allocator * a, void * ptr);
void ba_print_htable(struct block_allocator * a);
void ba_free_all(struct block_allocator * a);
void ba_count_all(struct block_allocator * a, size_t *num, size_t *size);
void ba_destroy(struct block_allocator * a);

#endif /* BLOCK_ALLOCATOR_H */

#undef PRE_INIT_BLOCK
#undef DO_PRE_INIT_BLOCK
#undef INIT_BLOCK
#undef EXIT_BLOCK
#undef BLOCK_ALLOC
#undef LOW_PTR_HASH_ALLOC
#undef PTR_HASH_ALLOC_FIXED
#undef PTR_HASH_ALLOC
#undef COUNT_BLOCK
#undef COUNT_OTHER
#undef DMALLOC_DESCRIBE_BLOCK
#undef BLOCK_ALLOC_HSIZE_SHIFT
#undef MAX_EMPTY_BLOCKS
#undef BLOCK_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FILL_PAGES
#undef PTR_HASH_ALLOC_FIXED_FILL_PAGES

/* Define this to keep freed blocks around in a backlog, which can
 * help locating leftover pointers to other blocks. It can also hide
 * bugs since the blocks remain intact a while after they are freed.
 * Valgrind will immediately detect attempts to use blocks on the
 * backlog list, though. Only available with dmalloc debug. */
/* #define DMALLOC_BLOCK_BACKLOG */

/* Note: The block_alloc mutex is held while PRE_INIT_BLOCK runs. */
#define PRE_INIT_BLOCK(X)
#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)
#define COUNT_BLOCK(X)
#define COUNT_OTHER()
#define DMALLOC_DESCRIBE_BLOCK(X)
#define BLOCK_ALLOC_HSIZE_SHIFT 2
#define MAX_EMPTY_BLOCKS 4	/* Must be >= 1. */

#if defined (DMALLOC_BLOCK_BACKLOG) && defined (DEBUG_MALLOC)
#define DO_IF_BLOCK_BACKLOG(X) X
#define DO_IF_NOT_BLOCK_BACKLOG(X)
//#define BLOCK_ALLOC_USED real_used
#else
#define DO_IF_BLOCK_BACKLOG(X)
#define DO_IF_NOT_BLOCK_BACKLOG(X) X
//#define BLOCK_ALLOC_USED used
#endif

/* Invalidate the block as far as possible if running with dmalloc.
 */
#define DO_PRE_INIT_BLOCK(X)	do {				\
    DO_IF_DMALLOC(MEMSET((X), 0x55, sizeof(*(X))));		\
    PRE_INIT_BLOCK(X);						\
  } while (0)

#ifndef PIKE_HASH_T
#define PIKE_HASH_T	size_t
#endif /* !PIKE_HASH_T */

#ifdef PIKE_RUN_UNLOCKED
#include "threads.h"

/* Block Alloc UnLocked */
#define BA_UL(X) PIKE_CONCAT(X,_unlocked)
#define BA_STATIC static
#define BA_INLINE INLINE
#else
#define BA_UL(X) X
#define BA_STATIC
#define BA_INLINE
#endif

#define BLOCK_ALLOC_FILL_PAGES(DATA, PAGES)				\
  BLOCK_ALLOC(DATA,							\
              ((PIKE_MALLOC_PAGE_SIZE * (PAGES))			\
               - PIKE_MALLOC_OVERHEAD - BLOCK_HEADER_SIZE) /		\
              sizeof (struct DATA))

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

#define BLOCK_ALLOC(DATA,BSIZE)						\
static struct block_allocator PIKE_CONCAT(DATA, _allocator) =			\
	BA_INIT(sizeof(struct DATA), (BSIZE)/sizeof(struct DATA));	\
									\
void PIKE_CONCAT3(new_,DATA,_context)(void)				\
{									\
}									\
									\
static void PIKE_CONCAT(alloc_more_,DATA)(void)				\
{                                                                       \
}									\
									\
BA_STATIC BA_INLINE struct DATA *BA_UL(PIKE_CONCAT(alloc_,DATA))(void)	\
{									\
  struct DATA *tmp;							\
  tmp = (struct DATA *)ba_alloc(&PIKE_CONCAT(DATA, _allocator));	\
  return tmp;								\
}									\
									\
DO_IF_RUN_UNLOCKED(                                                     \
struct DATA *PIKE_CONCAT(alloc_,DATA)(void)			        \
{									\
  struct DATA *ret;  							\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));		\
  ret=PIKE_CONCAT3(alloc_,DATA,_unlocked)();  				\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
  return ret;								\
})									\
									\
DO_IF_DMALLOC(                                                          \
static void PIKE_CONCAT3(dmalloc_,DATA,_not_freed) (struct DATA *d,	\
						    const char *msg)	\
{									\
  /* Separate function to allow gdb breakpoints. */			\
  fprintf (stderr, "struct " TOSTR(DATA)				\
	   " at %p is still in use %s\n", d, msg);			\
}									\
									\
static void PIKE_CONCAT(dmalloc_late_free_,DATA) (struct DATA *d)	\
{									\
  /* Separate function to allow gdb breakpoints. */			\
  fprintf (stderr, "struct " TOSTR(DATA) " at %p freed now (too late)\n", d); \
  dmalloc_mark_as_free (d, 1);						\
  dmalloc_unregister (d, 1);						\
  PIKE_MEM_NA (*d);							\
})									\
									\
BA_STATIC BA_INLINE							\
void BA_UL(PIKE_CONCAT(really_free_,DATA))(struct DATA *d)		\
{									\
  ba_free(&PIKE_CONCAT(DATA, _allocator), (void*)d);			\
}									\
									\
DO_IF_RUN_UNLOCKED(                                                     \
		   void PIKE_CONCAT(really_free_,DATA)(struct DATA *d)	\
{									\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));		\
  BA_UL(PIKE_CONCAT(really_free_,DATA))(d);				\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
})									\
									\
static void PIKE_CONCAT3(free_all_,DATA,_blocks_unlocked)(void)		\
{									\
  ba_free_all(&PIKE_CONCAT(DATA, _allocator));				\
}									\
									\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void)				\
{									\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));               \
  PIKE_CONCAT3(free_all_,DATA,_blocks_unlocked)();  			\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
}                                                                       \
									\
void PIKE_CONCAT3(count_memory_in_,DATA,s)(size_t *num_, size_t *size_)	\
{									\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));               \
  ba_count_all(&PIKE_CONCAT(DATA, _allocator), num_, size_);		\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
}                                                                       \
									\
									\
void PIKE_CONCAT3(init_,DATA,_blocks)(void)				\
{                                                                       \
}



#define LOW_PTR_HASH_ALLOC(DATA,BSIZE)					     \
									     \
BLOCK_ALLOC(DATA,BSIZE)							     \
									     \
struct DATA **PIKE_CONCAT(DATA,_hash_table)=0;				     \
size_t PIKE_CONCAT(DATA,_hash_table_size)=0;				     \
size_t PIKE_CONCAT(DATA,_hash_table_magnitude)=0;			     \
static size_t PIKE_CONCAT(num_,DATA)=0;					     \
									     \
static INLINE struct DATA *						     \
 PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(void *ptr,		     \
					       PIKE_HASH_T hval)	     \
{									     \
  struct DATA *p,**pp;							     \
  p=PIKE_CONCAT(DATA,_hash_table)[hval];                                     \
  if(!p || p->PTR_HASH_ALLOC_DATA == ptr)				     \
  {                                                                          \
    return p;                                                                \
  }                                                                          \
  while((p=*(pp=&p->BLOCK_ALLOC_NEXT)))                                      \
  {									     \
    if(p->PTR_HASH_ALLOC_DATA==ptr)					     \
    {									     \
      *pp=p->BLOCK_ALLOC_NEXT;						     \
      p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
      PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
      return p;								     \
    }									     \
  }									     \
  return 0;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(find_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;                                                            \
  PIKE_HASH_T hval = (PIKE_HASH_T)PTR_TO_INT(ptr);			     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  if(!PIKE_CONCAT(DATA,_hash_table)) {                                       \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 0;                                                                \
  }                                                                          \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval);		     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;								     \
}									     \
									     \
static INLINE struct DATA *						     \
 PIKE_CONCAT3(just_find_,DATA,_unlocked)(void *ptr,			     \
					 PIKE_HASH_T hval)		     \
{									     \
  struct DATA *p;							     \
  p=PIKE_CONCAT(DATA,_hash_table)[hval];                                     \
  if(!p || p->PTR_HASH_ALLOC_DATA == ptr)				     \
  {                                                                          \
    return p;                                                                \
  }                                                                          \
  while((p=p->BLOCK_ALLOC_NEXT)) 	                                     \
  {									     \
    if(p->PTR_HASH_ALLOC_DATA==ptr) return p;				     \
  }									     \
  return 0;								     \
}									     \
									     \
static struct DATA *PIKE_CONCAT(just_find_,DATA)(void *ptr)		     \
{									     \
  struct DATA *p;                                                            \
  PIKE_HASH_T hval = (PIKE_HASH_T)PTR_TO_INT(ptr);			     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  if(!PIKE_CONCAT(DATA,_hash_table)) {                                       \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 0;                                                                \
  }                                                                          \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  p=PIKE_CONCAT3(just_find_,DATA,_unlocked)(ptr, hval);			     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;								     \
}									     \
									     \
									     \
									     \
struct DATA *PIKE_CONCAT(make_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;							     \
  PIKE_HASH_T hval = (PIKE_HASH_T)PTR_TO_INT(ptr);			     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  p=PIKE_CONCAT3(make_,DATA,_unlocked)(ptr,hval);                            \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(get_,DATA)(void *ptr)			 	     \
{									     \
  struct DATA *p;							     \
  PIKE_HASH_T hval = (PIKE_HASH_T)PTR_TO_INT(ptr);			     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  if(!(p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval)))          \
    p=PIKE_CONCAT3(make_,DATA,_unlocked)(ptr, hval);		             \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;                                                                  \
}									     \
									     \
int PIKE_CONCAT3(check_,DATA,_semaphore)(void *ptr)			     \
{									     \
  PIKE_HASH_T hval = (PIKE_HASH_T)PTR_TO_INT(ptr);			     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  if(PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval))		     \
  {                                                                          \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 0;								     \
  }                                                                          \
									     \
  PIKE_CONCAT3(make_,DATA,_unlocked)(ptr, hval);			     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return 1;								     \
}									     \
									     \
void PIKE_CONCAT(move_,DATA)(struct DATA *block, void *new_ptr)		     \
{									     \
  PIKE_HASH_T hval =							     \
    (PIKE_HASH_T)PTR_TO_INT(block->PTR_HASH_ALLOC_DATA);		     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));		     \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  if (!PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(			     \
	block->PTR_HASH_ALLOC_DATA, hval))				     \
    Pike_fatal("The block to move wasn't found.\n");			     \
  DO_IF_DEBUG(								     \
    if (PIKE_CONCAT(DATA,_hash_table)[hval] != block)			     \
      Pike_fatal("Expected the block to be at the top of the hash chain.\n"); \
  );									     \
  PIKE_CONCAT(DATA,_hash_table)[hval] = block->BLOCK_ALLOC_NEXT;	     \
  block->PTR_HASH_ALLOC_DATA = new_ptr;					     \
  hval = (PIKE_HASH_T)PTR_TO_INT(new_ptr) %				     \
    (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);			     \
  block->BLOCK_ALLOC_NEXT = PIKE_CONCAT(DATA,_hash_table)[hval];	     \
  PIKE_CONCAT(DATA,_hash_table)[hval] = block;				     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));		     \
}									     \
									     \
int PIKE_CONCAT(remove_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;							     \
  PIKE_HASH_T hval = (PIKE_HASH_T)PTR_TO_INT(ptr);			     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  if(!PIKE_CONCAT(DATA,_hash_table))                                         \
  {                                                                          \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 0;				                                     \
  }                                                                          \
  hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  if((p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval)))	     \
  {									     \
    PIKE_CONCAT(num_,DATA)--;						     \
    DO_IF_DEBUG( if(PIKE_CONCAT(DATA,_hash_table)[hval]!=p)                  \
                    Pike_fatal("GAOssdf\n"); );                       	     \
    PIKE_CONCAT(DATA,_hash_table)[hval]=p->BLOCK_ALLOC_NEXT;		     \
    BA_UL(PIKE_CONCAT(really_free_,DATA))(p);				     \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 1;								     \
  }									     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return 0;								     \
}									     \
									     \
void PIKE_CONCAT3(low_init_,DATA,_hash)(size_t size)			     \
{									     \
  extern const INT32 hashprimes[32];					     \
  extern int my_log2(size_t x);					             \
  PIKE_CONCAT3(init_,DATA,_blocks)();                                        \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  PIKE_CONCAT(DATA,_hash_table_size)=					     \
     hashprimes[PIKE_CONCAT(DATA,_hash_table_magnitude)=my_log2(size)];	     \
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
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
}									     \
									     \
									     \
void PIKE_CONCAT3(init_,DATA,_hash)(void)				     \
{									     \
  PIKE_CONCAT3(low_init_,DATA,_hash)(BSIZE);		                     \
}									     \
									     \
void PIKE_CONCAT3(exit_,DATA,_hash)(void)				     \
{									     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  PIKE_CONCAT3(free_all_,DATA,_blocks_unlocked)();			     \
  free(PIKE_CONCAT(DATA,_hash_table));					     \
  PIKE_CONCAT(DATA,_hash_table)=0;					     \
  PIKE_CONCAT(num_,DATA)=0;						     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
}

#define PTR_HASH_ALLOC_FIXED(DATA,BSIZE)				     \
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, PIKE_HASH_T hval);\
LOW_PTR_HASH_ALLOC(DATA,BSIZE)                                               \
									     \
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, PIKE_HASH_T hval) \
{									     \
  struct DATA *p;							     \
									     \
  DO_IF_DEBUG( if(!PIKE_CONCAT(DATA,_hash_table))			     \
    Pike_fatal("Hash table error!\n"); )				     \
  PIKE_CONCAT(num_,DATA)++;						     \
									     \
  p=BA_UL(PIKE_CONCAT(alloc_,DATA))();					     \
  p->PTR_HASH_ALLOC_DATA=ptr;						     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}


#define PTR_HASH_ALLOC(DATA,BSIZE)					     \
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr,		     \
						PIKE_HASH_T hval);	     \
LOW_PTR_HASH_ALLOC(DATA,BSIZE)                                               \
                                                                             \
static void PIKE_CONCAT(DATA,_rehash)(void)				     \
{									     \
  /* Time to re-hash */							     \
  extern const INT32 hashprimes[32];					     \
  struct DATA **old_hash;	                                 	     \
  struct DATA *p;							     \
  PIKE_HASH_T hval;							     \
  size_t e;								     \
                                                                             \
  old_hash= PIKE_CONCAT(DATA,_hash_table);		                     \
  e=PIKE_CONCAT(DATA,_hash_table_size);			                     \
									     \
  PIKE_CONCAT(DATA,_hash_table_magnitude)++;				     \
  PIKE_CONCAT(DATA,_hash_table_size)=					     \
    hashprimes[PIKE_CONCAT(DATA,_hash_table_magnitude)];		     \
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
	hval = (PIKE_HASH_T)PTR_TO_INT(p->PTR_HASH_ALLOC_DATA);		     \
	hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);	     \
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
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr,		     \
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
    hval = (PIKE_HASH_T)PTR_TO_INT(ptr);				     \
    hval %= (PIKE_HASH_T)PIKE_CONCAT(DATA,_hash_table_size);		     \
  }									     \
									     \
  p=BA_UL(PIKE_CONCAT(alloc_,DATA))();					     \
  p->PTR_HASH_ALLOC_DATA=ptr;						     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}
