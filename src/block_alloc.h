/* $Id: block_alloc.h,v 1.32 2003/03/17 18:53:22 grubba Exp $ */
#undef PRE_INIT_BLOCK
#undef INIT_BLOCK
#undef EXIT_BLOCK
#undef BLOCK_ALLOC
#undef PTR_HASH_ALLOC
#undef COUNT_BLOCK
#undef COUNT_OTHER

#define PRE_INIT_BLOCK(X)
#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)
#define COUNT_BLOCK(X)
#define COUNT_OTHER()

#define BLOCK_ALLOC(DATA,BSIZE)						\
									\
struct PIKE_CONCAT(DATA,_block)						\
{									\
  struct PIKE_CONCAT(DATA,_block) *next;				\
  struct DATA x[BSIZE];							\
};									\
									\
static struct PIKE_CONCAT(DATA,_block) *PIKE_CONCAT(DATA,_blocks)=0;	\
static struct DATA *PIKE_CONCAT3(free_,DATA,s)=0;			\
									\
struct DATA *PIKE_CONCAT(alloc_,DATA)(void)				\
{									\
  struct DATA *tmp;							\
  if(!PIKE_CONCAT3(free_,DATA,s))					\
  {									\
    struct PIKE_CONCAT(DATA,_block) *n;					\
    int e;								\
    n=(struct PIKE_CONCAT(DATA,_block) *)				\
       malloc(sizeof(struct PIKE_CONCAT(DATA,_block)));			\
    if(!n)								\
    {									\
      fprintf(stderr,"Fatal: out of memory.\n");			\
      exit(17);								\
    }									\
    n->next=PIKE_CONCAT(DATA,_blocks);					\
    PIKE_CONCAT(DATA,_blocks)=n;					\
									\
    for(e=0;e<BSIZE;e++)						\
    {									\
      n->x[e].BLOCK_ALLOC_NEXT=(void *)PIKE_CONCAT3(free_,DATA,s);	\
      PRE_INIT_BLOCK( (n->x+e) );					\
      PIKE_CONCAT3(free_,DATA,s)=n->x+e;				\
    }									\
  }									\
									\
  tmp=PIKE_CONCAT3(free_,DATA,s);					\
  PIKE_CONCAT3(free_,DATA,s)=(struct DATA *)tmp->BLOCK_ALLOC_NEXT;	\
  DO_IF_DMALLOC( dmalloc_register(tmp,sizeof(struct DATA), DMALLOC_LOCATION());  )\
  INIT_BLOCK(tmp);							\
  return tmp;								\
}									\
									\
void PIKE_CONCAT(really_free_,DATA)(struct DATA *d)			\
{									\
  EXIT_BLOCK(d);							\
  DO_IF_DMALLOC( dmalloc_unregister(d, 1);  )				\
  d->BLOCK_ALLOC_NEXT = (void *)PIKE_CONCAT3(free_,DATA,s);		\
  PRE_INIT_BLOCK(d);							\
  PIKE_CONCAT3(free_,DATA,s)=d;						\
}									\
									\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void)				\
{									\
  struct PIKE_CONCAT(DATA,_block) *tmp;					\
  DO_IF_DMALLOC(                                                        \
   for(tmp=PIKE_CONCAT(DATA,_blocks);tmp;tmp=tmp->next)                 \
   {                                                                    \
     int tmp2;                                                          \
     extern void dmalloc_check_block_free(void *p, char *loc);          \
     for(tmp2=0;tmp2<BSIZE;tmp2++)                                      \
       dmalloc_check_block_free(tmp->x+tmp2, DMALLOC_LOCATION());       \
   }                                                                    \
  )                                                                     \
  while((tmp=PIKE_CONCAT(DATA,_blocks)))				\
  {									\
    PIKE_CONCAT(DATA,_blocks)=tmp->next;				\
    free((char *)tmp);							\
  }									\
  PIKE_CONCAT(DATA,_blocks)=0;						\
  PIKE_CONCAT3(free_,DATA,s)=0;						\
}									\
									\
									\
void PIKE_CONCAT3(count_memory_in_,DATA,s)(INT32 *num_, INT32 *size_)	\
{									\
  INT32 num=0, size=0;							\
  struct PIKE_CONCAT(DATA,_block) *tmp;					\
  struct DATA *tmp2;							\
  for(tmp=PIKE_CONCAT(DATA,_blocks);tmp;tmp=tmp->next)			\
  {									\
    num+=BSIZE;								\
    size+=sizeof(struct PIKE_CONCAT(DATA,_block));			\
    COUNT_BLOCK(tmp);                                                   \
  }									\
  for(tmp2=PIKE_CONCAT3(free_,DATA,s);tmp2;				\
          tmp2 = (struct DATA *)tmp2->BLOCK_ALLOC_NEXT) num--;		\
  COUNT_OTHER();                                                        \
  *num_=num;								\
  *size_=size;								\
}




#define PTR_HASH_ALLOC(DATA,BSIZE)					     \
									     \
BLOCK_ALLOC(DATA,BSIZE)							     \
									     \
struct DATA **PIKE_CONCAT(DATA,_hash_table)=0;				     \
size_t PIKE_CONCAT(DATA,_hash_table_size)=0;				     \
static size_t PIKE_CONCAT(num_,DATA)=0;				     \
									     \
inline struct DATA *							     \
 PIKE_CONCAT(really_low_find_,DATA)(void *ptr, size_t hval)		     \
{									     \
  struct DATA *p,**pp;							     \
  p=PIKE_CONCAT(DATA,_hash_table)[hval];                                     \
  if(!p) return 0;                                                           \
  if(p->data == ptr) return p;                                               \
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
									     \
struct DATA *PIKE_CONCAT(find_,DATA)(void *ptr)				     \
{									     \
  size_t hval = (size_t)ptr;						     \
  if(!PIKE_CONCAT(DATA,_hash_table_size)) return 0;                          \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  return PIKE_CONCAT(really_low_find_,DATA)(ptr, hval);			     \
}									     \
									     \
									     \
static void PIKE_CONCAT(DATA,_rehash)()					     \
{									     \
  /* Time to re-hash */							     \
  struct DATA **old_hash= PIKE_CONCAT(DATA,_hash_table);		     \
  struct DATA *p;							     \
  size_t hval;							     \
  size_t e=PIKE_CONCAT(DATA,_hash_table_size);			     \
									     \
  PIKE_CONCAT(DATA,_hash_table_size)*=2;				     \
  PIKE_CONCAT(DATA,_hash_table_size)++;					     \
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
static inline struct DATA *						     \
 PIKE_CONCAT(really_low_just_find_,DATA)(void *ptr, size_t hval)	     \
{									     \
  struct DATA *p,**pp;							     \
  p=PIKE_CONCAT(DATA,_hash_table)[hval];                                     \
  if(!p || p->data == ptr)						     \
  {                                                                          \
    return p;                                                                \
  }                                                                          \
  while((p=p->BLOCK_ALLOC_NEXT)) 	                                     \
  {									     \
    if(p->data==ptr) return p;						     \
  }									     \
  return 0;								     \
}									     \
									     \
static struct DATA *PIKE_CONCAT(just_find_,DATA)(void *ptr)		     \
{									     \
  struct DATA *p;                                                            \
  size_t hval = (size_t)ptr;						     \
  if(!PIKE_CONCAT(DATA,_hash_table_size)) {                                  \
    return 0;                                                                \
  }                                                                          \
  hval %= PIKE_CONCAT(DATA,_hash_table_size);				     \
  p=PIKE_CONCAT(really_low_just_find_,DATA)(ptr, hval);			     \
  return p;								     \
}									     \
									     \
									     \
struct DATA *PIKE_CONCAT(make_,DATA)(void *ptr, size_t hval)		     \
{									     \
  struct DATA *p;							     \
									     \
  DO_IF_DEBUG( if(!PIKE_CONCAT(DATA,_hash_table))			     \
    fatal("Hash table error!\n"); )					     \
  PIKE_CONCAT(num_,DATA)++;						     \
									     \
  if(( PIKE_CONCAT(num_,DATA)>>2 ) >=					     \
     PIKE_CONCAT(DATA,_hash_table_size))				     \
  {									     \
    PIKE_CONCAT(DATA,_rehash)();					     \
    hval=(size_t)ptr;						     \
    hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  }									     \
									     \
  p=PIKE_CONCAT(alloc_,DATA)();						     \
  p->data=ptr;								     \
  p->BLOCK_ALLOC_NEXT=PIKE_CONCAT(DATA,_hash_table)[hval];		     \
  PIKE_CONCAT(DATA,_hash_table)[hval]=p;				     \
  return p;								     \
}									     \
									     \
struct DATA *PIKE_CONCAT(get_,DATA)(void *ptr)			 	     \
{									     \
  struct DATA *p;							     \
  size_t hval=(size_t)ptr;					     \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  if((p=PIKE_CONCAT(really_low_find_,DATA)(ptr, hval)))			     \
    return p;								     \
									     \
  return PIKE_CONCAT(make_,DATA)(ptr, hval);				     \
}									     \
									     \
int PIKE_CONCAT3(check_,DATA,_semafore)(void *ptr)			     \
{									     \
  struct DATA *p;							     \
  size_t hval=(size_t)ptr;					     \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  if((p=PIKE_CONCAT(really_low_find_,DATA)(ptr, hval)))			     \
    return 0;								     \
									     \
  PIKE_CONCAT(make_,DATA)(ptr, hval);					     \
  return 1;								     \
}									     \
									     \
int PIKE_CONCAT(remove_,DATA)(void *ptr)				     \
{									     \
  struct DATA *p;							     \
  size_t hval=(size_t)ptr;					     \
  if(!PIKE_CONCAT(DATA,_hash_table)) return 0;				     \
  hval%=PIKE_CONCAT(DATA,_hash_table_size);				     \
  if((p=PIKE_CONCAT(really_low_find_,DATA)(ptr, hval)))			     \
  {									     \
    PIKE_CONCAT(num_,DATA)--;						     \
    if(PIKE_CONCAT(DATA,_hash_table)[hval]!=p) fatal("GAOssdf\n");	     \
    PIKE_CONCAT(DATA,_hash_table)[hval]=p->BLOCK_ALLOC_NEXT;		     \
    PIKE_CONCAT(really_free_,DATA)(p);					     \
    return 1;								     \
  }									     \
  return 0;								     \
}									     \
									     \
void PIKE_CONCAT3(init_,DATA,_hash)(void)				     \
{									     \
  extern INT32 hashprimes[32];						     \
  extern int my_log2(size_t x);					     \
  PIKE_CONCAT(DATA,_hash_table_size)=hashprimes[my_log2(BSIZE)];	     \
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
void PIKE_CONCAT3(exit_,DATA,_hash)(void)				     \
{									     \
  PIKE_CONCAT3(free_all_,DATA,_blocks)();				     \
  free(PIKE_CONCAT(DATA,_hash_table));					     \
  PIKE_CONCAT(DATA,_hash_table)=0;					     \
  PIKE_CONCAT(num_,DATA)=0;						     \
}
