#define PRE_INIT_BLOCK(X)
#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)
#define BLOCK_ALLOC(DATA,BSIZE)									\
												\
struct PIKE_CONCAT(DATA,_block)									\
{												\
  struct PIKE_CONCAT(DATA,_block) *next;							\
  struct DATA x[BSIZE];										\
};												\
												\
static struct PIKE_CONCAT(DATA,_block) *PIKE_CONCAT(DATA,_blocks)=0;				\
static struct DATA *PIKE_CONCAT3(free_,DATA,s)=0;						\
												\
struct DATA *PIKE_CONCAT(alloc_,DATA)(void)							\
{												\
  struct DATA *tmp;										\
  if(!PIKE_CONCAT3(free_,DATA,s))								\
  {												\
    struct PIKE_CONCAT(DATA,_block) *n;								\
    int e;											\
    n=(struct PIKE_CONCAT(DATA,_block) *)malloc(sizeof(struct PIKE_CONCAT(DATA,_block)));	\
    if(!n)											\
    {												\
      fprintf(stderr,"Fatal: out of memory.\n");						\
      exit(17);											\
    }												\
    n->next=PIKE_CONCAT(DATA,_blocks);								\
    PIKE_CONCAT(DATA,_blocks)=n;								\
												\
    for(e=0;e<BSIZE;e++)									\
    {												\
      n->x[e].next=PIKE_CONCAT3(free_,DATA,s);							\
      PRE_INIT_BLOCK( (n->x+e) );								\
      PIKE_CONCAT3(free_,DATA,s)=n->x+e;							\
    }												\
  }												\
												\
  tmp=PIKE_CONCAT3(free_,DATA,s);								\
  PIKE_CONCAT3(free_,DATA,s)=tmp->next;								\
  INIT_BLOCK(tmp);										\
  return tmp;											\
}												\
												\
inline void PIKE_CONCAT(free_,DATA)(struct DATA *d)						\
{												\
  EXIT_BLOCK(d);										\
  d->next=PIKE_CONCAT3(free_,DATA,s);								\
  PRE_INIT_BLOCK(d);										\
  PIKE_CONCAT3(free_,DATA,s)=d;									\
}												\
												\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void)							\
{												\
  struct PIKE_CONCAT(DATA,_block) *tmp;								\
  while((tmp=PIKE_CONCAT(DATA,_blocks)))							\
  {												\
    PIKE_CONCAT(DATA,_blocks)=tmp->next;							\
    free((char *)tmp);										\
  }												\
  PIKE_CONCAT(DATA,_blocks)=0;									\
  PIKE_CONCAT3(free_,DATA,s)=0;									\
}												\

