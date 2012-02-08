#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H
#include <stdint.h>
#ifdef BA_MEMTRACE
 #include <stdio.h>
#endif

#ifndef PIKE_XCONCAT
#define PIKE_CONCAT(X, Y)	X##Y
#define PIKE_XCONCAT(X, Y)	PIKE_CONCAT(X, Y)
#endif

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

#ifndef PIKE_MEM_RW_RANGE
# define PIKE_MEM_RW_RANGE(x, y)
#endif
#ifndef PIKE_MEM_RW
# define PIKE_MEM_RW(x)
#endif
#ifndef PIKE_MEM_WO
# define PIKE_MEM_WO(x)
#endif


#if defined(PIKE_CORE) || defined(DYNAMIC_MODULE)
# include "global.h"
# include "pike_error.h"
#else
# ifdef PMOD_EXPORT
#  undef PMOD_EXPORT
# endif
# define PMOD_EXPORT
# include <stdio.h>
# include <unistd.h>

static inline void _Pike_error(int line, char * file, char * msg) {
    fprintf(stderr, "%s:%d\t%s\n", file, line, msg);
    _exit(1);
}
# define Pike_error(x...)	do { sprintf(errbuf, x); _Pike_error(__LINE__, __FILE__, errbuf); } while(0)
# ifndef INLINE
#  define INLINE __inline__
# endif
# ifndef ATTRIBUTE
#  define ATTRIBUTE(x)	__attribute__(x)
# endif
#endif

#if defined(BA_DEBUG) || (!defined(PIKE_CORE) && !defined(DYNAMIC_MODULE) \
			  && !defined(STATIC_MODULE))
#endif
extern char errbuf[];

#ifdef BA_DEBUG
#define BA_ERROR(x...)	do { fprintf(stderr, "ERROR at %s:%d\n", __FILE__, __LINE__); ba_print_htable(a); ba_show_pages(a); Pike_error(x); } while(0)
#else
#define BA_ERROR(x...)	do { Pike_error(x); } while(0)
#endif

typedef struct ba_block_header * ba_block_header;
typedef struct ba_page * ba_page;

typedef uint32_t ba_page_t;
typedef uint32_t ba_block_t;

#ifdef BA_STATS
struct block_alloc_stats {
    size_t st_max;
    size_t st_used;
    size_t st_max_pages;
    size_t st_max_allocated;
    char * st_name;
    size_t free_fast1, free_fast2, free_full, free_empty, max, full, empty;
    size_t find_linear, find_hash;
    struct mallinfo st_mallinfo;
};
# define BA_INIT_STATS(block_size, blocks, name) {\
    0/*st_max*/,\
    0/*st_used*/,\
    0/*st_max_pages*/,\
    0/*st_max_allocated*/,\
    name/*st_name*/,\
    0,0,0,0,0,0,0,\
    0,0}
#else
# define BA_INIT_STATS(block_size, blocks, name)
#endif

struct block_allocator {
    ba_block_header first_blk;
    size_t offset;
    ba_page last_free;
    ba_page first;	/* 24 */
    ba_block_t blocks;
    uint32_t magnitude;
    ba_page * pages;
    ba_page last;       /* 48 */
    ba_page empty;
    ba_page_t empty_pages, max_empty_pages;
    ba_page_t allocated;
    uint32_t block_size;
    ba_page_t num_pages;
    char * blueprint;   /* 88 */
#ifdef BA_STATS
    struct block_alloc_stats stats;
#endif
};
#define BA_INIT(block_size, blocks, name) {\
    NULL/*first_blk*/,\
    0/*offset*/,\
    NULL/*last_free*/,\
    NULL/*first*/,\
    blocks/*blocks*/,\
    0/*magnitude*/,\
    NULL/*pages*/,\
    NULL/*last*/,\
    NULL/*empty*/,\
    0/*empty_pages*/,\
    BA_MAX_EMPTY/*max_empty_pages*/,\
    0/*allocated*/,\
    block_size/*block_size*/,\
    0/*num_pages*/,\
    NULL/*blueprint*/,\
    BA_INIT_STATS(block_size, blocks, name)\
}

struct ba_page {
    struct ba_block_header * first;
    ba_page next, prev;
    ba_page hchain;
    ba_block_t used;
};

struct ba_block_header {
    struct ba_block_header * next;
#ifdef BA_DEBUG
    uint32_t magic;
#endif
};

#define BA_ONE	((ba_block_header)1)

PMOD_EXPORT void ba_low_free(struct block_allocator * a,
				    ba_page p, ba_block_header ptr);
PMOD_EXPORT void ba_show_pages(const struct block_allocator * a);
PMOD_EXPORT void ba_init(struct block_allocator * a, uint32_t block_size,
			 ba_page_t blocks);
PMOD_EXPORT void ba_low_alloc(struct block_allocator * a);
PMOD_EXPORT INLINE void ba_find_page(struct block_allocator * a,
			      const void * ptr);
PMOD_EXPORT void ba_remove_page(struct block_allocator * a, ba_page p);
#ifdef BA_DEBUG
PMOD_EXPORT void ba_print_htable(const struct block_allocator * a);
PMOD_EXPORT void ba_check_allocator(struct block_allocator * a, char*, char*, int);
#endif
#ifdef BA_STATS
PMOD_EXPORT void ba_print_stats(struct block_allocator * a);
#endif
PMOD_EXPORT void ba_free_all(struct block_allocator * a);
PMOD_EXPORT void ba_count_all(struct block_allocator * a, size_t *num, size_t *size);
PMOD_EXPORT void ba_destroy(struct block_allocator * a);

#define BA_PAGE(a, n)   ((a)->pages[(n) - 1])
#define BA_BLOCKN(a, p, n) ((ba_block_header)(((char*)(p+1)) + (n)*((a)->block_size)))
#define BA_CHECK_PTR(a, p, ptr)	((size_t)((char*)ptr - (char*)(p)) <= (a)->offset)
#define BA_LASTBLOCK(a, p) ((ba_block_header)((char*)p + (a)->offset))

#ifdef BA_STATS
# define INC(X) do { (a->stats.X++); } while (0)
# define IF_STATS(x)	do { x; } while(0)
#else
# define INC(X) do { } while (0)
# define IF_STATS(x)	do { } while(0)
#endif

#ifdef BA_MEMTRACE
static int _do_mtrace;
static char _ba_buf[256];
static inline void emit(int n) {
    if (!_do_mtrace) return;
    fflush(NULL);
    write(3, _ba_buf, n);
    fflush(NULL);
}

static ATTRIBUTE((constructor)) void _________() {
    _do_mtrace = !!getenv("MALLOC_TRACE");
}

# define PRINT(fmt, args...)     emit(snprintf(_ba_buf, sizeof(_ba_buf), fmt, args))
#endif

ATTRIBUTE((always_inline,malloc))
static INLINE void * ba_alloc(struct block_allocator * a) {
    ba_block_header ptr;
#ifdef BA_STATS
    struct block_alloc_stats *s = &a->stats;
#endif

    if (unlikely(!a->first_blk)) {
	ba_low_alloc(a);
    }
    ptr = a->first_blk;
#ifndef BA_CHAIN_PAGE
    if (ptr->next == BA_ONE) {
	a->first_blk = (ba_block_header)(((char*)ptr) + a->block_size);
	a->first_blk->next = (ba_block_header)(size_t)!(a->first_blk == BA_LASTBLOCK(a, a->first));
    } else
#endif
    a->first_blk = ptr->next;
#ifdef BA_DEBUG
    ((ba_block_header)ptr)->magic = BA_MARK_ALLOC;
#endif
#ifdef BA_MEMTRACE
    PRINT("%% %p 0x%x\n", ptr, a->block_size);
#endif
#ifdef BA_STATS
    if (++s->st_used > s->st_max) {
      s->st_max = s->st_used;
      s->st_max_allocated = a->allocated;
      s->st_max_pages = a->num_pages;
      s->st_mallinfo = mallinfo();
    }
#endif
    
    return ptr;
}


ATTRIBUTE((always_inline))
static INLINE void ba_free(struct block_allocator * a, void * ptr) {
    ba_page p = NULL;

#ifdef BA_MEMTRACE
    PRINT("%% %p\n", ptr);
#endif

#ifdef BA_DEBUG
    if (a->empty_pages == a->num_pages) {
	BA_ERROR("we did it!\n");
    }
    if (((ba_block_header)ptr)->magic == BA_MARK_FREE) {
	BA_ERROR("double freed somethign\n");
    }
    //memset(ptr, 0x75, a->block_size);
    ((ba_block_header)ptr)->magic = BA_MARK_FREE;
#endif

#ifdef BA_STATS
    a->stats.st_used--;
#endif

    if (BA_CHECK_PTR(a, a->first, ptr)) {
      ((ba_block_header)ptr)->next = a->first_blk;
      a->first_blk = (ba_block_header)ptr;
      INC(free_fast1);
      return;
    } else if (BA_CHECK_PTR(a, a->last_free, ptr)) {
      p = a->last_free;
      ((ba_block_header)ptr)->next = p->first;
      p->first = (ba_block_header)ptr;
      if ((--p->used) && (((ba_block_header)ptr)->next)) return;
      INC(free_fast2);
    } else {
      ba_find_page(a, ptr);
      p = a->last_free;
      ((ba_block_header)ptr)->next = p->first;
      p->first = (ba_block_header)ptr;
      if ((--p->used) && (((ba_block_header)ptr)->next)) return;
    }

#ifdef BA_DEBUG
    if (a->num_pages == 1) {
	BA_ERROR("absolutely not planned!\n");
    }
#endif
    
    ba_low_free(a, p, (ba_block_header)ptr);
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
    PRE_INIT_BLOCK((X));						\
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
#define BA_INLINE INLINE
#else
#define BA_UL(X) X
#define BA_STATIC
#define BA_INLINE
#endif

/* Size of the members in the block struct below that don't contain
 * the payload data (i.e. that aren't x). This can be used in BSIZE to
 * make the block fit within a page. */
#ifndef BLOCK_HEADER_SIZE
#undef BLOCK_HEADER_SIZE
#endif
/* we assume here that malloc has 8 bytes of overhead */
#define BLOCK_HEADER_SIZE (sizeof(struct ba_page) + sizeof(void*))


#define BLOCK_ALLOC_FILL_PAGES(DATA, PAGES)				\
  BLOCK_ALLOC(DATA,							\
      ((PIKE_MALLOC_PAGE_SIZE * (PAGES)) - BLOCK_HEADER_SIZE) /	\
      sizeof (struct DATA))

#define PTR_HASH_ALLOC_FILL_PAGES(DATA, PAGES)				\
  PTR_HASH_ALLOC(DATA,							\
     ((PIKE_MALLOC_PAGE_SIZE * (PAGES)) - BLOCK_HEADER_SIZE) /	\
     sizeof (struct DATA))

#define PTR_HASH_ALLOC_FIXED_FILL_PAGES(DATA, PAGES)			\
  PTR_HASH_ALLOC_FIXED(DATA,						\
       ((PIKE_MALLOC_PAGE_SIZE * (PAGES)) - BLOCK_HEADER_SIZE) /	\
       sizeof (struct DATA))

#define MS(x)	#x

#define WALK_NONFREE_BLOCKS(DATA, BLOCK, FCOND, CODE)	do {		\
    struct block_allocator * a = &PIKE_CONCAT(DATA, _allocator);	\
    ba_page_t n;							\
    ba_block_t used;							\
    for (n = 1; n <= a->num_pages; n++) {				\
	ba_block_t i;							\
	ba_page p = BA_PAGE(a, n);					\
	used = (p == a->first) ? a->blocks : p->used;			\
	for (i = 0; used && i < a->blocks; i++) {			\
	    BLOCK = ((struct DATA*)(p+1)) + i;				\
	    if (FCOND) {						\
		do CODE while(0);					\
		--used;							\
	    }								\
	}								\
    }									\
} while(0)
#define BLOCK_ALLOC(DATA,BSIZE)						\
static struct block_allocator PIKE_CONCAT(DATA, _allocator) =		\
	BA_INIT(sizeof(struct DATA), (BSIZE), #DATA);			\
static struct DATA PIKE_CONCAT(DATA,_blueprint);			\
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
ATTRIBUTE((destructor))							\
static void PIKE_CONCAT(print_stats_,DATA)(void) {			\
  IF_STATS(ba_print_stats(&PIKE_CONCAT(DATA, _allocator)));		\
}									\
static void PIKE_CONCAT3(free_all_,DATA,_blocks_unlocked)(void)		\
{									\
  IF_STATS(ba_print_stats(&PIKE_CONCAT(DATA, _allocator)));		\
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
    DO_PRE_INIT_BLOCK(((struct DATA *)(PIKE_CONCAT(DATA,_allocator).blueprint = (char*)&PIKE_CONCAT(DATA, _blueprint))));	\
    /*fprintf(stderr, #DATA"_allocator: %p\n", &PIKE_CONCAT(DATA, _allocator));*/\
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
