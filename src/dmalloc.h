#ifdef DEBUG_MALLOC

extern int verbose_debug_malloc;
extern int verbose_debug_exit;
#define main dbm_main
extern void *debug_malloc(size_t, const char *, int);
extern char *debug_xalloc(long, const char *, int);
extern void *debug_calloc(size_t, size_t, const char *, int);
extern void *debug_realloc(void *, size_t, const char *, int);
extern void debug_free(void *, const char *, int);
extern char *debug_strdup(const char *, const char *, int);
void *debug_malloc_update_location(void *,const char *, int);
#define malloc(x) debug_malloc((x), __FILE__, __LINE__)
#define xalloc(x) debug_xalloc((x), __FILE__, __LINE__)
#define calloc(x, y) debug_calloc((x), (y), __FILE__, __LINE__)
#define realloc(x, y) debug_realloc((x), (y), __FILE__, __LINE__)
#define free(x) debug_free((x), __FILE__, __LINE__)
#define strdup(x) debug_strdup((x), __FILE__, __LINE__)
#define DO_IF_DMALLOC(X) X
#else
#define DO_IF_DMALLOC(X)
extern char *xalloc(long);
#define debug_malloc_update_location(X,Y,Z) (X)
#endif
