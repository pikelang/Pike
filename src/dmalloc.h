/*
 * $Id: dmalloc.h,v 1.19 2000/03/07 21:23:41 hubbe Exp $
 */

extern char *debug_xalloc(long);
#ifdef DEBUG_MALLOC
struct memhdr;

void dump_memhdr_locations(struct memhdr *from,
			   struct memhdr *notfrom);
struct memhdr *alloc_memhdr(void);
void really_free_memhdr(struct memhdr *mh);
void add_marks_to_memhdr(struct memhdr *to,void *ptr);

extern int verbose_debug_malloc;
extern int verbose_debug_exit;
extern void dmalloc_register(void *, int,const char *, int);
extern int dmalloc_unregister(void *, int);
extern void *debug_malloc(size_t, const char *, int);
extern void *debug_calloc(size_t, size_t, const char *, int);
extern void *debug_realloc(void *, size_t, const char *, int);
extern void debug_free(void *, const char *, int,int);
extern char *debug_strdup(const char *, const char *, int);
extern void reset_debug_malloc(void);
extern void dmalloc_free(void *p);
extern int debug_malloc_touch_fd(int, const char *, int);
extern int debug_malloc_register_fd(int, const char *, int);
extern int debug_malloc_close_fd(int, const char *, int);

void *debug_malloc_update_location(void *,const char *, int);
void search_all_memheaders_for_references(void);

/* Beware! names of named memory regions are never ever freed!! /Hubbe */
void *debug_malloc_name(void *p,const char *fn, int line);
void debug_malloc_copy_names(void *p, void *p2);

/* glibc 2.1 defines this as a macro. */
#ifdef strdup
#undef strdup
#endif

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
#define dmalloc_touch_svalue(X) do { struct svalue *_tmp = (X); if ((X)->type <= MAX_REF_TYPE) { debug_malloc_touch(_tmp->u.refs); } } while(0)

#define DMALLOC_LINE_ARGS ,char * dmalloc_file, int dmalloc_line
#define DMALLOC_POS ,__FILE__,__LINE__
#define DMALLOC_PROXY_ARGS ,dmalloc_file,dmalloc_line
void dmalloc_accept_leak(void *);
#define dmalloc_touch_fd(X) debug_malloc_touch_fd((X),__FILE__,__LINE__)
#define dmalloc_register_fd(X) debug_malloc_register_fd((X),__FILE__,__LINE__)
#define dmalloc_close_fd(X) debug_malloc_close_fd((X),__FILE__,__LINE__)
#else
#define dmalloc_touch_fd(X) (X)
#define dmalloc_register_fd(X) (X)
#define dmalloc_close_fd(X) (X)
#define dmfree(X) free((X))
#define dmalloc_accept_leak(X) (void)(X)
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
#define dmalloc_touch_svalue(X)
#define dmalloc_register(X,Y,Z,W)
#define dmalloc_unregister(X,Y)
#define debug_free(X,Y,Z,Q) free((X))
#define debug_malloc_name(P,FN,LINE)
#define debug_malloc_copy_names(p,p2)
#define search_all_memheaders_for_references()
#endif
