#undef BLOCK_ALLOC
#define BLOCK_ALLOC(DATA,SIZE)					\
struct DATA *PIKE_CONCAT(alloc_,DATA)(void);			\
void PIKE_CONCAT(really_free_,DATA)(struct DATA *d);	\
void PIKE_CONCAT3(free_all_,DATA,_blocks)(void);
