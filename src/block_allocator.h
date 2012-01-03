#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H
#include <stdint.h>

//#define BA_DEBUG

#ifdef HAS___BUILTIN_EXPECT
#define likely(x)	(__builtin_expect((x), 1))
#define unlikely(x)	(__builtin_expect((x), 0))
#else
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif
#define BA_MARK_FREE	0xF4337575
#define BA_MARK_ALLOC	0x4110C375
#define BA_ALLOC_INITIAL	8
#define BA_HASH_THLD		8
#define BA_MAX_EMPTY		8
#ifdef BA_HASH_THLD
#define IF_HASH(x)	do { if(a->allocated <= BA_HASH_THLD) break; x; }\
			while(0)
#else
#define IF_HASH(x)	x
#endif

#ifdef BA_DEBUG
#define IF_DEBUG(x)	x
#else
#define IF_DEBUG(x)
#endif

#if defined(PIKE_CORE) || defined(DYNAMIC_MODULE)
#include "global.h"
#include "pike_error.h"
#else
#ifdef PMOD_EXPORT
# undef PMOD_EXPORT
#endif
#define PMOD_EXPORT
#include <stdio.h>
#include <unistd.h>

static inline void _Pike_error(int line, char * file, char * msg) {
    fprintf(stderr, "%s:%d\t%s\n", file, line, msg);
    _exit(1);
}
#define Pike_error(x)	do { _Pike_error(__LINE__, __FILE__, errbuf); } while(0)
#endif

#if defined(BA_DEBUG) || (!defined(PIKE_CORE) && !defined(DYNAMIC_MODULE) \
			  && !defined(STATIC_MODULE))
#endif
extern char errbuf[];

#ifdef BA_DEBUG
#define BA_ERROR(x...)	do { fprintf(stderr, "ERROR at %s:%d\n", __FILE__, __LINE__); ba_print_htable(a); ba_show_pages(a); snprintf(errbuf, 8191, x); Pike_error(errbuf); } while(0)
#else
#define BA_ERROR(x...)	do { snprintf(errbuf, 8191, x); Pike_error(errbuf); } while(0)
#endif



typedef struct ba_page * ba_page;

typedef uint32_t ba_page_t;
typedef uint32_t ba_block_t;

struct block_allocator {
    uint32_t block_size;
    uint32_t magnitude;
    ba_block_t blocks;
    ba_page_t num_pages;
    ba_page_t empty_pages, max_empty_pages;
    ba_page_t allocated;
    ba_page last_free;
    ba_page first;
    ba_page pages;
    ba_page_t * htable;
};
#define BA_INIT(block_size, blocks) { block_size, 0, blocks, 0, 0,\
				      BA_MAX_EMPTY, 0,\
				      NULL, NULL, NULL, NULL }


struct ba_page {
    struct ba_block_header * data;
    struct ba_block_header * first;
    ba_page_t next, prev;
    ba_page_t hchain;
    ba_block_t blocks_used;
};

struct ba_block_header {
    struct ba_block_header * next;
#ifdef BA_DEBUG
    uint32_t magic;
#endif
};

typedef struct ba_block_header * ba_block_header;

PMOD_EXPORT void ba_show_pages(struct block_allocator * a);
PMOD_EXPORT void ba_init(struct block_allocator * a, uint32_t block_size,
			 ba_page_t blocks);
PMOD_EXPORT void * ba_low_alloc(struct block_allocator * a);
PMOD_EXPORT void ba_low_free(struct block_allocator * a, void * ptr, ba_page p);
PMOD_EXPORT void ba_remove_page(struct block_allocator * a, ba_page_t n);
#ifdef BA_DEBUG
PMOD_EXPORT void ba_print_htable(const struct block_allocator * a);
PMOD_EXPORT void ba_check_allocator(struct block_allocator * a, char*, char*, int);
#endif
PMOD_EXPORT void ba_free_all(struct block_allocator * a);
PMOD_EXPORT void ba_count_all(struct block_allocator * a, size_t *num, size_t *size);
PMOD_EXPORT void ba_destroy(struct block_allocator * a);

#define BA_PAGE(a, n)   ((a)->pages + (n) - 1)
#define BA_BLOCKN(a, p, n) ((ba_block_header)(((char*)(p)->data) + (n)*((a)->block_size)))
#define BA_CHECK_PTR(a, p, ptr)	((char*)ptr >= (char*)p->data && (char*)BA_LASTBLOCK(a,p) >= (char*)ptr)
#define BA_LASTBLOCK(a, p) BA_BLOCKN(a, p, (a)->blocks - 1)

__attribute__((always_inline,malloc))
static inline void * ba_alloc(struct block_allocator * a) {
    if (a->first) {
	ba_block_header ptr;
	ba_page p = a->first;

#ifdef BA_DEBUG
	if (p->prev) {
	    BA_ERROR("a->first has previous: %d\n", p->prev);
	}
#endif

#ifdef BA_DEBUG
	if (p->blocks_used == a->blocks)
	    BA_ERROR("baaad!\n");
#endif
	if (!p->blocks_used++) a->empty_pages--;
#ifdef BA_DEBUG
	if (p->first < p->data || p->first > BA_LASTBLOCK(a, p)) {
	    BA_ERROR("bad pointer %p (should be inside [%p..%p]\n", p->first, p->data, BA_LASTBLOCK(a, p));
	}
#endif
	ptr = p->first;

#ifdef BA_DEBUG
	if (!p->data) {
	    BA_ERROR("got null pointer from uninitialized page %p of block %p (num_pages %d).\n", a->first, p->first, a->num_pages);
	}
#endif
	//fprintf(stderr, "alloced pointer %p (%u/%u used %u)\n",
		//ptr, p->first-1, a->blocks, p->blocks_used);

	if (unlikely(p->blocks_used == a->blocks)) {
	    if (p->next) {
		a->first = BA_PAGE(a, p->next);
		a->first->prev = 0;
#ifdef BA_DEBUG
		p->next = 0;
#endif
	    } else a->first = NULL;
	}

	p->first = ptr->next;

#ifdef BA_DEBUG
	if (!ptr) BA_ERROR("PTR is NULL\n");
	if (ptr->magic != BA_MARK_FREE) {
	    BA_ERROR("found block with bad magic: %X\n", ptr->magic);
	    BA_ERROR("bad block!\n");
	}
	ptr->magic = BA_MARK_ALLOC;
#endif
	//fprintf(stderr, "first is %u\n", p->first);
	//memset(ptr, 0, a->block_size);

	return (void*)ptr;
    } else return ba_low_alloc(a);
}

__attribute__((always_inline))
static inline void ba_free(struct block_allocator * a, void * ptr) {
    ba_page p = a->last_free;
    
    if (!p || !BA_CHECK_PTR(a, p, ptr)) {
	p = a->first;
	if (!p || !BA_CHECK_PTR(a, p, ptr)) {
#ifdef BA_HASH_THLD
	    p = NULL;
	    if (a->allocated <= BA_HASH_THLD) {
		ba_page_t t;
		for (t = 1; t <= a->num_pages; t++) {
		    if (BA_CHECK_PTR(a, BA_PAGE(a, t), ptr)) {
			a->last_free = p = BA_PAGE(a, t);
			goto FOUND;
		    }
		}
# ifdef BA_DEBUG
# else
		BA_ERROR("Unknown pointer \n");
#endif
	    } else {
#endif
		goto LOW_FREE;
#ifdef BA_HASH_THLD
	    }
#endif
	}

	a->last_free = p;
    }

#ifdef BA_DEBUG
    if ((p < a->pages || p > BA_PAGE(a, a->num_pages))) {
	BA_ERROR("found bad page %p (last_free: %p, first: %p).\n", p, a->last_free, a->first);
    }
#endif
FOUND:
    if ((p->blocks_used == 1 && ++a->empty_pages > a->max_empty_pages) || p->blocks_used == a->blocks) goto LOW_FREE;

    p->blocks_used --;
#ifdef BA_DEBUG
    if (((ba_block_header)ptr)->magic == BA_MARK_FREE) {
	BA_ERROR("double freed somethign\n");
    }
    //memset(ptr, 0x75, a->block_size);
    ((ba_block_header)ptr)->magic = BA_MARK_FREE;
#endif
    ((ba_block_header)ptr)->next = p->first;
    //fprintf(stderr, "setting first to %u (%p vs %p) n: %u vs %u\n", t+1, BA_BLOCKN(a, p, t+1), ptr, BA_NBLOCK(a, p, BA_BLOCKN(a, p, t+1)), BA_NBLOCK(a, p, ptr));
    p->first = ptr;
    return;
LOW_FREE:
    ba_low_free(a, ptr, p);
}

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
#warning RUNNING UNLOCKED
#include "threads.h"

/* Block Alloc UnLocked */
#define BA_UL(X) PIKE_CONCAT(X,_unlocked)
#define BA_STATIC static
#define BA_INLINE __attribute__((always_inline)) inline __inline__
#define ____
#else
#define BA_UL(X) X
#define BA_STATIC
#define BA_INLINE
#endif

/* Size of the members in the block struct below that don't contain
 * the payload data (i.e. that aren't x). This can be used in BSIZE to
 * make the block fit within a page. */
#ifndef BLOCK_HEADER_SIZE
#define BLOCK_HEADER_SIZE 0
#endif


#define BLOCK_ALLOC_FILL_PAGES(DATA, PAGES)				\
  BLOCK_ALLOC(DATA,							\
              ((PIKE_MALLOC_PAGE_SIZE * (PAGES))) /			\
              sizeof (struct DATA))

#define PTR_HASH_ALLOC_FILL_PAGES(DATA, PAGES)				\
  PTR_HASH_ALLOC(DATA,							\
                 ((PIKE_MALLOC_PAGE_SIZE * (PAGES))) /			\
                 sizeof (struct DATA))

#define PTR_HASH_ALLOC_FIXED_FILL_PAGES(DATA, PAGES)			\
  PTR_HASH_ALLOC_FIXED(DATA,						\
                       ((PIKE_MALLOC_PAGE_SIZE * (PAGES))) /		\
                       sizeof (struct DATA))

#define MS(x)	#x

#define WALK_NONFREE_BLOCKS(DATA, BLOCK, FCOND, CODE)	do {		\
    ba_page_t n;							\
    ba_block_t used;							\
    for (n = 1; n <= PIKE_CONCAT(DATA, _allocator).num_pages; n++) {	\
	ba_block_t i;							\
	ba_page p = BA_PAGE(&PIKE_CONCAT(DATA, _allocator), n);		\
	used = p->blocks_used;						\
	for (i = 0; used && i < PIKE_CONCAT(DATA, _allocator).blocks; i++) {\
	    if (FCOND) {						\
		do CODE while(0);					\
		--used;							\
	    }								\
	}								\
    }									\
} while(0)
#define BLOCK_ALLOC(DATA,BSIZE)						\
static struct block_allocator PIKE_CONCAT(DATA, _allocator) =		\
	BA_INIT(sizeof(struct DATA), (BSIZE));	\
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
  ba_page_t num_pages = PIKE_CONCAT(DATA, _allocator).num_pages;	\
  tmp = (struct DATA *)ba_alloc(&PIKE_CONCAT(DATA, _allocator));	\
  if (unlikely(num_pages > PIKE_CONCAT(DATA, _allocator).num_pages)) {	\
      ba_block_t n;							\
      ba_page p = BA_PAGE(&PIKE_CONCAT(DATA, _allocator), num_pages+1);	\
      for (n = 1; n < PIKE_CONCAT(DATA, _allocator).blocks; n++) {	\
	  struct DATA *tmp2 = (struct DATA *)BA_BLOCKN(&PIKE_CONCAT(DATA, _allocator), p, n);\
	  DO_PRE_INIT_BLOCK(tmp2);					\
      }									\
  }									\
  IF_DEBUG(ba_check_allocator(&PIKE_CONCAT(DATA, _allocator), "alloc_"#DATA, __FILE__, __LINE__);)			\
  PIKE_MEM_RW(*tmp);							\
  PIKE_MEM_WO(*tmp);							\
  INIT_BLOCK(tmp);							\
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
  EXIT_BLOCK(d);							\
  DO_PRE_INIT_BLOCK(d);							\
  ba_free(&PIKE_CONCAT(DATA, _allocator), (void*)d);			\
  IF_DEBUG(ba_check_allocator(&PIKE_CONCAT(DATA, _allocator),		\
	    "really_free_"#DATA, __FILE__, __LINE__);)			\
  PIKE_MEM_NA(*d);							\
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
  ba_destroy(&PIKE_CONCAT(DATA, _allocator));				\
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
PMOD_EXPORT void PIKE_CONCAT(show_pages_,DATA)() {\
    fprintf(stderr, "blocks of "#DATA"\n");\
    ba_show_pages(&PIKE_CONCAT(DATA,_allocator));			\
}									\
									\
void PIKE_CONCAT3(init_,DATA,_blocks)(void)				\
{                                                                       \
    /*fprintf(stderr, #DATA"_allocator: %p\n", &PIKE_CONCAT(DATA, _allocator)); */\
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
