/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: block_alloc.h,v 1.53 2002/11/24 20:31:23 mast Exp $
*/

#undef PRE_INIT_BLOCK
#undef INIT_BLOCK
#undef EXIT_BLOCK
#undef BLOCK_ALLOC
#undef LOW_PTR_HASH_ALLOC
#undef PTR_HASH_ALLOC_FIXED
#undef PTR_HASH_ALLOC
#undef COUNT_BLOCK
#undef COUNT_OTHER
#undef BLOCK_ALLOC_HSIZE_SHIFT
#undef MAX_EMPTY_BLOCKS

#define PRE_INIT_BLOCK(X)
#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)
#define COUNT_BLOCK(X)
#define COUNT_OTHER()
#define BLOCK_ALLOC_HSIZE_SHIFT 2
#define MAX_EMPTY_BLOCKS 4

#ifdef PIKE_RUN_UNLOCKED
#include "threads.h"

/* Block Alloc UnLocked */
#define BA_UL(X) PIKE_CONCAT(X,_unlocked)
#define BA_STATIC static
#define BA_INLINE inline
#else
#define BA_UL(X) X
#define BA_STATIC
#define BA_INLINE
#endif

/* Size of the members in the block struct below that doesn't contain
 * the payload data (i.e. that aren't x). This can be used in
 * BSIZE to make the block fit within a page. */
#define BLOCK_HEADER_SIZE (3 * sizeof (void *) + sizeof (INT32))

#define BLOCK_ALLOC(DATA,BSIZE)						\
									\
struct PIKE_CONCAT(DATA,_block)						\
{									\
  struct PIKE_CONCAT(DATA,_block) *next;				\
  struct PIKE_CONCAT(DATA,_block) *prev;				\
  struct DATA *PIKE_CONCAT3(free_,DATA,s);				\
  INT32 used;								\
  struct DATA x[BSIZE];							\
};									\
									\
static struct PIKE_CONCAT(DATA,_block) *PIKE_CONCAT(DATA,_blocks)=0;	\
static struct PIKE_CONCAT(DATA,_block) *PIKE_CONCAT(DATA,_free_blocks)=(void*)-1;  \
static INT32 PIKE_CONCAT3(num_empty_,DATA,_blocks)=0;			\
DO_IF_RUN_UNLOCKED(static PIKE_MUTEX_T PIKE_CONCAT(DATA,_mutex);)       \
									\
static void PIKE_CONCAT(alloc_more_,DATA)(void)				\
{                                                                       \
  struct PIKE_CONCAT(DATA,_block) *n;					\
  size_t e;								\
  n=(struct PIKE_CONCAT(DATA,_block) *)					\
     malloc(sizeof(struct PIKE_CONCAT(DATA,_block)));			\
  if(!n)								\
  {									\
    fprintf(stderr,"Fatal: out of memory.\n");				\
    exit(17);								\
  }									\
  if((n->next=PIKE_CONCAT(DATA,_blocks)))				\
    n->next->prev=n;							\
  n->prev=NULL;								\
  n->used=0;								\
  PIKE_CONCAT(DATA,_blocks)=n;						\
  PIKE_CONCAT(DATA,_free_blocks)=n;					\
									\
  n->x[0].BLOCK_ALLOC_NEXT=NULL;					\
  PRE_INIT_BLOCK( n->x );						\
  for(e=1;e<(BSIZE);e++)						\
  {									\
    n->x[e].BLOCK_ALLOC_NEXT=(void *)&n->x[e-1];			\
    PRE_INIT_BLOCK( (n->x+e) );						\
  }									\
  n->PIKE_CONCAT3(free_,DATA,s)=&n->x[(BSIZE)-1];			\
  /* Mark the new blocks as unavailable for now... */			\
  PIKE_MEM_NA(n->x);							\
}									\
									\
BA_STATIC BA_INLINE struct DATA *BA_UL(PIKE_CONCAT(alloc_,DATA))(void)	\
{									\
  struct DATA *tmp;							\
  struct PIKE_CONCAT(DATA,_block) *blk;					\
									\
  if(!(blk = PIKE_CONCAT(DATA,_free_blocks))) {				\
    PIKE_CONCAT(alloc_more_,DATA)();					\
    blk = PIKE_CONCAT(DATA,_blocks);					\
    blk->used++;							\
  }									\
  DO_IF_DEBUG(								\
    else if (PIKE_CONCAT(DATA,_free_blocks) == (void *)-1)		\
      Pike_fatal("Block alloc " #DATA " not initialized.\n");		\
  )									\
  else if(!blk->used++)							\
    --PIKE_CONCAT3(num_empty_,DATA,_blocks);				\
									\
  tmp = blk->PIKE_CONCAT3(free_,DATA,s);				\
  /* Mark the new block as available. */				\
  PIKE_MEM_RW(*tmp);							\
  if(!(blk->PIKE_CONCAT3(free_,DATA,s) = (void *)tmp->BLOCK_ALLOC_NEXT))    \
    PIKE_CONCAT(DATA,_free_blocks) = blk->prev;				\
  DO_IF_DMALLOC(                                                        \
    dmalloc_unregister(tmp, 1);                                         \
    dmalloc_register(tmp,sizeof(struct DATA), DMALLOC_LOCATION());      \
  )                                                                     \
  /* Mark the new block as available but uninitialized. */		\
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
static void PIKE_CONCAT(check_free_,DATA)(struct DATA *d)               \
{                                                                       \
  struct PIKE_CONCAT(DATA,_block) *tmp;					\
  for(tmp=PIKE_CONCAT(DATA,_blocks);tmp;tmp=tmp->next)                  \
  {                                                                     \
    if( (char *)d < (char *)tmp) continue;                              \
    if( (char *)d >= (char *)(tmp->x+(BSIZE))) continue;		\
    return;                                                             \
  }                                                                     \
  Pike_fatal("really_free_%s called on non-block_alloc region (%p).\n",      \
         #DATA, d);                                                     \
}                                                                       \
)									\
									\
DO_IF_RUN_UNLOCKED(                                                     \
void PIKE_CONCAT3(really_free_,DATA,_unlocked)(struct DATA *d)		\
{									\
  struct PIKE_CONCAT(DATA,_block) *blk = PIKE_CONCAT(DATA,_free_blocks);     \
  EXIT_BLOCK(d);							\
  if(blk == NULL || (char *)d < (char *)blk ||				\
     (char *)d >= (char *)(blk->x+(BSIZE))) {				\
    blk = PIKE_CONCAT(DATA,_blocks);					\
    if((char *)d < (char *)blk ||					\
       (char *)d >= (char *)(blk->x+(BSIZE))) {				\
      do {								\
        blk = blk->next;						\
      } while((char *)d < (char *)blk ||				\
	      (char *)d >= (char *)(blk->x+(BSIZE)));			\
      if(blk == PIKE_CONCAT(DATA,_free_blocks))				\
        PIKE_CONCAT(DATA,_free_blocks) = blk->prev;			\
      blk->prev->next = blk->next;					\
      if(blk->next)							\
        blk->next->prev = blk->prev;					\
      blk->prev = NULL;							\
      blk->next = PIKE_CONCAT(DATA,_blocks);				\
      blk->next->prev = blk;						\
      PIKE_CONCAT(DATA,_blocks) = blk;					\
    }									\
    if(PIKE_CONCAT(DATA,_free_blocks) == NULL)				\
      PIKE_CONCAT(DATA,_free_blocks) = blk;				\
  }									\
  DO_IF_DMALLOC( PIKE_CONCAT(check_free_,DATA)(d);                      \
                 dmalloc_mark_as_free(d, 1);  )				\
  d->BLOCK_ALLOC_NEXT = (void *)blk->PIKE_CONCAT3(free_,DATA,s);	\
  PRE_INIT_BLOCK(d);							\
  /* Mark block as unavailable. */					\
  PIKE_MEM_NA(*d);							\
  blk->PIKE_CONCAT3(free_,DATA,s)=d;					\
  if(!--blk->used &&							\
     ++PIKE_CONCAT3(num_empty_,DATA,_blocks) > MAX_EMPTY_BLOCKS) {	\
    if(blk == PIKE_CONCAT(DATA,_free_blocks)) {				\
      if((blk->prev->next = blk->next))					\
        blk->next->prev = blk->prev;					\
      PIKE_CONCAT(DATA,_free_blocks) = blk->prev;			\
    } else {								\
      PIKE_CONCAT(DATA,_blocks) = blk->next;				\
      blk->next->prev = NULL;						\
    }									\
    /* Mark meta-block as available, since libc will mess with it. */	\
    PIKE_MEM_RW(*blk);							\
    free(blk);								\
    --PIKE_CONCAT3(num_empty_,DATA,_blocks);				\
  }									\
})									\
									\
void PIKE_CONCAT(really_free_,DATA)(struct DATA *d)			\
{									\
  struct PIKE_CONCAT(DATA,_block) *blk;					\
  EXIT_BLOCK(d);							\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));               \
  blk = PIKE_CONCAT(DATA,_free_blocks);					\
  if(blk == NULL || (char *)d < (char *)blk ||				\
     (char *)d >= (char *)(blk->x+(BSIZE))) {				\
    blk = PIKE_CONCAT(DATA,_blocks);					\
    if((char *)d < (char *)blk ||					\
       (char *)d >= (char *)(blk->x+(BSIZE))) {				\
      do {								\
        blk = blk->next;						\
      } while((char *)d < (char *)blk ||				\
	      (char *)d >= (char *)(blk->x+(BSIZE)));			\
      if(blk == PIKE_CONCAT(DATA,_free_blocks))				\
        PIKE_CONCAT(DATA,_free_blocks) = blk->prev;			\
      blk->prev->next = blk->next;					\
      if(blk->next)							\
        blk->next->prev = blk->prev;					\
      blk->prev = NULL;							\
      blk->next = PIKE_CONCAT(DATA,_blocks);				\
      blk->next->prev = blk;						\
      PIKE_CONCAT(DATA,_blocks) = blk;					\
    }									\
    if(PIKE_CONCAT(DATA,_free_blocks) == NULL)				\
      PIKE_CONCAT(DATA,_free_blocks) = blk;				\
  }									\
  DO_IF_DMALLOC( PIKE_CONCAT(check_free_,DATA)(d);                      \
                 dmalloc_mark_as_free(d, 1);  )				\
  d->BLOCK_ALLOC_NEXT = (void *)blk->PIKE_CONCAT3(free_,DATA,s);	\
  PRE_INIT_BLOCK(d);							\
  /* Mark block as unavailable. */					\
  PIKE_MEM_NA(*d);							\
  blk->PIKE_CONCAT3(free_,DATA,s)=d;					\
  if(!--blk->used &&							\
     ++PIKE_CONCAT3(num_empty_,DATA,_blocks) > MAX_EMPTY_BLOCKS) {	\
    if(blk == PIKE_CONCAT(DATA,_free_blocks)) {				\
      if((blk->prev->next = blk->next))					\
        blk->next->prev = blk->prev;					\
      PIKE_CONCAT(DATA,_free_blocks) = blk->prev;			\
    } else {								\
      PIKE_CONCAT(DATA,_blocks) = blk->next;				\
      blk->next->prev = NULL;						\
    }									\
    /* Mark meta-block as available, since libc will mess with it. */	\
    PIKE_MEM_RW(*blk);							\
    free(blk);								\
    --PIKE_CONCAT3(num_empty_,DATA,_blocks);				\
  }									\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
}									\
									\
static void PIKE_CONCAT3(free_all_,DATA,_blocks_unlocked)(void)		\
{									\
  struct PIKE_CONCAT(DATA,_block) *tmp;					\
  DO_IF_DMALLOC(                                                        \
   for(tmp=PIKE_CONCAT(DATA,_blocks);tmp;tmp=tmp->next)                 \
   {                                                                    \
     size_t tmp2;							\
     extern void dmalloc_check_block_free(void *p, char *loc);          \
     for(tmp2=0;tmp2<(BSIZE);tmp2++)					\
     {                                                                  \
       dmalloc_check_block_free(tmp->x+tmp2, DMALLOC_LOCATION());       \
       dmalloc_unregister(tmp->x+tmp2, 1);                              \
     }                                                                  \
   }                                                                    \
  )                                                                     \
  while((tmp=PIKE_CONCAT(DATA,_blocks)))				\
  {									\
    PIKE_CONCAT(DATA,_blocks)=tmp->next;				\
    /* Mark meta-block as available, since libc will mess with it. */	\
    PIKE_MEM_RW(tmp->x);						\
    free((char *)tmp);							\
  }									\
  PIKE_CONCAT(DATA,_blocks)=0;						\
  PIKE_CONCAT(DATA,_free_blocks)=0;					\
  PIKE_CONCAT3(num_empty_,DATA,_blocks)=0;				\
}									\
									\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void)				\
{									\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));               \
  PIKE_CONCAT3(free_all_,DATA,_blocks_unlocked)();  			\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
}                                                                       \
									\
void PIKE_CONCAT3(count_memory_in_,DATA,s)(INT32 *num_, INT32 *size_)	\
{									\
  INT32 num=0, size=0;							\
  struct PIKE_CONCAT(DATA,_block) *tmp;					\
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));               \
  for(tmp=PIKE_CONCAT(DATA,_blocks);tmp;tmp=tmp->next)			\
  {									\
    size+=sizeof(struct PIKE_CONCAT(DATA,_block));			\
    num+=tmp->used;							\
    COUNT_BLOCK(tmp);                                                   \
  }									\
  COUNT_OTHER();                                                        \
  *num_=num;								\
  *size_=size;								\
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));             \
}                                                                       \
									\
									\
void PIKE_CONCAT3(init_,DATA,_blocks)(void)				\
{                                                                       \
/*  DO_IF_RUN_UNLOCKED(mt_init_recursive(&PIKE_CONCAT(DATA,_mutex)));*/ \
  DO_IF_RUN_UNLOCKED(mt_init(&PIKE_CONCAT(DATA,_mutex)));               \
  PIKE_CONCAT(DATA,_free_blocks)=0;					\
}                                                                       \



#define LOW_PTR_HASH_ALLOC(DATA,BSIZE)					     \
									     \
BLOCK_ALLOC(DATA,BSIZE)							     \
									     \
struct DATA **PIKE_CONCAT(DATA,_hash_table)=0;				     \
size_t PIKE_CONCAT(DATA,_hash_table_size)=0;				     \
size_t PIKE_CONCAT(DATA,_hash_table_magnitude)=0;			     \
static size_t PIKE_CONCAT(num_,DATA)=0;				     \
									     \
static inline struct DATA *						     \
 PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(void *ptr, size_t hval)    \
{									     \
  struct DATA *p,**pp;							     \
  p=PIKE_CONCAT(DATA,_hash_table)[hval];                                     \
  if(!p || p->data == ptr)                                                   \
  {                                                                          \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return p;                                                                \
  }                                                                          \
  while((p=*(pp=&p->BLOCK_ALLOC_NEXT)))                                      \
  {									     \
    if(p->data==ptr)							     \
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
  size_t hval = (size_t)ptr;						     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  if(!PIKE_CONCAT(DATA,_hash_table_size)) {                                  \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 0;                                                                \
  }                                                                          \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval);		     \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;								     \
}									     \
									     \
									     \
									     \
struct DATA *PIKE_CONCAT(make_,DATA)(void *ptr, size_t hval)		     \
{									     \
  struct DATA *p;							     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  p=PIKE_CONCAT3(make_,DATA,_unlocked)(ptr,hval);                            \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(get_,DATA)(void *ptr)			 	     \
{									     \
  struct DATA *p;							     \
  size_t hval=(size_t)ptr;					     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  if(!(p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval)))          \
    p=PIKE_CONCAT3(make_,DATA,_unlocked)(ptr, hval);		             \
  DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                  \
  return p;                                                                  \
}									     \
									     \
int PIKE_CONCAT3(check_,DATA,_semafore)(void *ptr)			     \
{									     \
  struct DATA *p;							     \
  size_t hval=(size_t)ptr;					     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  if((p=PIKE_CONCAT3(really_low_find_,DATA,_unlocked)(ptr, hval)))	     \
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
int PIKE_CONCAT(remove_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;							     \
  size_t hval=(size_t)ptr;					     \
  DO_IF_RUN_UNLOCKED(mt_lock(&PIKE_CONCAT(DATA,_mutex)));                    \
  if(!PIKE_CONCAT(DATA,_hash_table))                                         \
  {                                                                          \
    DO_IF_RUN_UNLOCKED(mt_unlock(&PIKE_CONCAT(DATA,_mutex)));                \
    return 0;				                                     \
  }                                                                          \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
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
void PIKE_CONCAT3(low_init_,DATA,_hash)(size_t size)	     	     	\
{									     \
  extern INT32 hashprimes[32];						     \
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
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, size_t hval);     \
LOW_PTR_HASH_ALLOC(DATA,BSIZE)                                               \
									     \
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, size_t hval)   \
{									     \
  struct DATA *p;							     \
									     \
  DO_IF_DEBUG( if(!PIKE_CONCAT(DATA,_hash_table))			     \
    Pike_fatal("Hash table error!\n"); )					     \
  PIKE_CONCAT(num_,DATA)++;						     \
									     \
  p=BA_UL(PIKE_CONCAT(alloc_,DATA))();					     \
  p->data=ptr;								     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}									     \


#define PTR_HASH_ALLOC(DATA,BSIZE)					     \
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, size_t hval);     \
LOW_PTR_HASH_ALLOC(DATA,BSIZE)                                               \
                                                                             \
static void PIKE_CONCAT(DATA,_rehash)()					     \
{									     \
  /* Time to re-hash */							     \
  extern INT32 hashprimes[32];						     \
  struct DATA **old_hash;	                                 	     \
  struct DATA *p;							     \
  size_t hval;							     \
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
	hval=(size_t)(p->data);					     \
	hval%=PIKE_CONCAT(DATA,_hash_table_size);			     \
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
struct DATA *PIKE_CONCAT3(make_,DATA,_unlocked)(void *ptr, size_t hval)   \
{									     \
  struct DATA *p;							     \
									     \
  DO_IF_DEBUG( if(!PIKE_CONCAT(DATA,_hash_table))			     \
    Pike_fatal("Hash table error!\n"); )					     \
  PIKE_CONCAT(num_,DATA)++;						     \
									     \
  if(( PIKE_CONCAT(num_,DATA)>>BLOCK_ALLOC_HSIZE_SHIFT ) >=		     \
     PIKE_CONCAT(DATA,_hash_table_size))				     \
  {									     \
    PIKE_CONCAT(DATA,_rehash)();					     \
    hval=(size_t)ptr;						     \
    hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  }									     \
									     \
  p=BA_UL(PIKE_CONCAT(alloc_,DATA))();					     \
  p->data=ptr;								     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}
