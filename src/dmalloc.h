#ifdef DEBUG_MALLOC

extern int verbose_debug_malloc;
extern int verbose_debug_exit;
extern void *debug_malloc(size_t, const char *, int);
extern char *debug_xalloc(long);
extern void *debug_calloc(size_t, size_t, const char *, int);
extern void *debug_realloc(void *, size_t, const char *, int);
extern void debug_free(void *, const char *, int);
extern char *debug_strdup(const char *, const char *, int);
void *debug_malloc_update_location(void *,const char *, int);
#define malloc(x) debug_malloc((x), __FILE__, __LINE__)
#define calloc(x, y) debug_calloc((x), (y), __FILE__, __LINE__)
#define realloc(x, y) debug_realloc((x), (y), __FILE__, __LINE__)
#define free(x) debug_free((x), __FILE__, __LINE__)
#define strdup(x) debug_strdup((x), __FILE__, __LINE__)
#define DO_IF_DMALLOC(X) X
#define debug_malloc_touch(X) debug_malloc_update_location((X),__FILE__,__LINE__)
#define debug_malloc_pass(X) debug_malloc_update_location((X),__FILE__,__LINE__)
#define xalloc(X) ((char *)debug_malloc_touch(debug_xalloc(X)))
#else
#define xalloc debug_xalloc
#define dbm_main main
#define DO_IF_DMALLOC(X)
#define debug_malloc_update_location(X,Y,Z) (X)
#define debug_malloc_touch(X)
#define debug_malloc_pass(X) (X)
#endif
