#undef BLOCK_ALLOC
#undef PTR_HASH_ALLOC

#define BLOCK_ALLOC(DATA,SIZE)						\
struct DATA *PIKE_CONCAT(alloc_,DATA)(void);				\
void PIKE_CONCAT(really_free_,DATA)(struct DATA *d);			\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void);			\
void PIKE_CONCAT3(count_memory_in_,DATA,s)(INT32 *num, INT32 *size);	\


#define PTR_HASH_ALLOC(DATA,BSIZE)				\
BLOCK_ALLOC(DATA,BSIZE)						\
extern struct DATA **PIKE_CONCAT(DATA,_hash_table);		\
extern size_t PIKE_CONCAT(DATA,_hash_table_size);		\
struct DATA *PIKE_CONCAT(find_,DATA)(void *ptr);		\
struct DATA *PIKE_CONCAT(get_,DATA)(void *ptr);			\
int PIKE_CONCAT3(check_,DATA,_semafore)(void *ptr);		\
int PIKE_CONCAT(remove_,DATA)(void *ptr);			\
void PIKE_CONCAT3(init_,DATA,_hash)(void);			\
void PIKE_CONCAT3(exit_,DATA,_hash)(void);			\

#define PTR_HASH_LOOP(DATA,HVAL,PTR)					\
  for ((HVAL) = PIKE_CONCAT(DATA,_hash_table_size); (HVAL)-- > 0;)	\
    for ((PTR) = PIKE_CONCAT(DATA,_hash_table)[HVAL];			\
	 (PTR); (PTR) = (PTR)->BLOCK_ALLOC_NEXT)

#define BLOCK_ALLOC_NEXT next
