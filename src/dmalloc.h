/*
 * $Id: dmalloc.h,v 1.11 1999/05/02 08:11:38 hubbe Exp $
 */

extern char *debug_xalloc(long);
#ifdef DEBUG_MALLOC
struct memhdr;

void dump_memhdr_locations(struct memhdr *from,
			   struct memhdr *notfrom);
struct memhdr *alloc_memhdr(void);
void really_free_memhdr(struct memhdr *mh);
void add_marks_to_memhdr(struct memhdr *to,void *ptr);
void low_add_marks_to_memhdr(struct memhdr *to,
			     struct memhdr *from);

extern int verbose_debug_malloc;
extern int verbose_debug_exit;
extern void dmalloc_register(void *, int,char *, int);
extern int dmalloc_unregister(void *, int);
extern void *debug_malloc(size_t, const char *, int);
extern void *debug_calloc(size_t, size_t, const char *, int);
extern void *debug_realloc(void *, size_t, const char *, int);
extern void debug_free(void *, const char *, int,int);
extern char *debug_strdup(const char *, const char *, int);
extern void reset_debug_malloc(void);
extern void dmalloc_free(void *p);
void *debug_malloc_update_location(void *,const char *, int);
#define malloc(x) debug_malloc((x), __FILE__, __LINE__)
#define calloc(x, y) debug_calloc((x), (y), __FILE__, __LINE__)
#define realloc(x, y) debug_realloc((x), (y), __FILE__, __LINE__)
#define free(x) debug_free((x), __FILE__, __LINE__,0)
#define dmfree(x) debug_free((x),__FILE__,__LINE__,1)
#define strdup(x) debug_strdup((x), __FILE__, __LINE__)
#define DO_IF_DMALLOC(X) X
#define debug_malloc_touch(X) debug_malloc_update_location((X),__FILE__,__LINE__)
#define debug_malloc_pass(X) debug_malloc_update_location((X),__FILE__,__LINE__)
#define xalloc(X) ((char *)debug_malloc_touch(debug_xalloc(X)))
void debug_malloc_dump_references(void *x);
#define dmalloc_touch(TYPE,X) ((TYPE)debug_malloc_update_location((X),__FILE__,__LINE__))

#define DMALLOC_LINE_ARGS ,char * dmalloc_file, int dmalloc_line
#define DMALLOC_POS ,__FILE__,__LINE__
#define DMALLOC_PROXY_ARGS ,dmalloc_file,dmalloc_line
void *dmalloc_accept_leak(void *);
#else
#define dmfree(X) free((X))
#define dmalloc_accept_leak(X) (X)
#define DMALLOC_LINE_ARGS 
#define DMALLOC_POS 
#define DMALLOC_PROXY_ARGS
#define debug_malloc_dump_references(X)
#define xalloc debug_xalloc
#define dbm_main main
#define DO_IF_DMALLOC(X)
#define debug_malloc_update_location(X,Y,Z) (X)
#define debug_malloc_touch(X)
#define debug_malloc_pass(X) (X)
#define dmalloc_touch(TYPE,X) (X)
#define dmalloc_register(X,Y,Z)
#define dmalloc_unregister(X,Y)
#define debug_free(X,Y,Z,Q) free((X))
#endif
