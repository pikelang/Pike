/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef DMALLOC_H
#define DMALLOC_H

PMOD_EXPORT extern void *debug_xalloc(size_t);
PMOD_EXPORT extern void debug_xfree(void *);
PMOD_EXPORT extern void *debug_xmalloc(size_t);
PMOD_EXPORT extern void *debug_xcalloc(size_t,size_t);
PMOD_EXPORT extern void *debug_xrealloc(void *,size_t);
PMOD_EXPORT char *debug_xstrdup(const char *src);

#if defined (HAVE_EXECINFO_H) && defined (HAVE_BACKTRACE)
/* GNU libc provides some tools to inspect the stack. */
#include <execinfo.h>
typedef void *c_stack_frame;

#define DUMP_C_STACK_TRACE() do {					\
    c_stack_frame bt[100];						\
    int n = backtrace (bt, 100);					\
    backtrace_symbols_fd (bt, n, 2);					\
  } while (0)

#else
#undef DMALLOC_C_STACK_TRACE
#define DUMP_C_STACK_TRACE() do {} while (0)
#endif /* defined (HAVE_EXECINFO_H) && defined (HAVE_BACKTRACE) */

#define DMALLOC_NAMED_LOCATION(NAME)	\
    (("NS" __FILE__ ":" DEFINETOSTR(__LINE__) NAME )+1)

#define DMALLOC_LOCATION() DMALLOC_NAMED_LOCATION("")

/* Location types in loc[0]:
 * 'S': static (DMALLOC_NAMED_LOCATION)
 * 'D': dynamic (dynamic_location)
 * 'B': dynamic with backtrace (debug_malloc_update_location_bt)
 * 'T': memory map template
 * 'M': memory map
 */
typedef const char *LOCATION;
#define LOCATION_TYPE(X) ((X)[0])
#define LOCATION_NAME(X) ((X)+1)
#define LOCATION_IS_DYNAMIC(X)						\
  (LOCATION_TYPE (X)=='D' || LOCATION_TYPE (X) == 'B')

#ifdef DMALLOC_TRACE
#define DMALLOC_TRACELOGSIZE 131072

extern char *dmalloc_tracelog[DMALLOC_TRACELOGSIZE];
extern size_t dmalloc_tracelogptr;

#define DMALLOC_TRACE_LOG(X)  (dmalloc_tracelog[ dmalloc_tracelogptr = (dmalloc_tracelogptr +1 )%DMALLOC_TRACELOGSIZE ] = (X))

#endif /* DMALLOC_TRACE */

#ifdef PIKE_DEBUG
PMOD_EXPORT extern int gc_external_refs_zapped;
PMOD_EXPORT void gc_check_zapped (void *a, TYPE_T type, const char *file, INT_TYPE line);
#endif /* PIKE_DEBUG */

#ifdef DO_PIKE_CLEANUP
extern int exit_with_cleanup;
extern int exit_cleanup_in_progress;
#define DO_IF_PIKE_CLEANUP(X) X
#else
#define DO_IF_PIKE_CLEANUP(X)
#endif /* DO_PIKE_CLEANUP */

typedef void describe_block_fn (void *);

#ifdef DEBUG_MALLOC
struct memhdr;
struct block_allocator;

void dump_memhdr_locations(struct memhdr *from,
			   struct memhdr *notfrom,
			   int indent);
struct memhdr *alloc_memhdr(void);
void really_free_memhdr(struct memhdr *mh);
void add_marks_to_memhdr(struct memhdr *to,void *ptr);

extern int verbose_debug_malloc;
PMOD_EXPORT void dmalloc_trace(void *);
PMOD_EXPORT void dmalloc_register(void *, int, LOCATION);
PMOD_EXPORT int dmalloc_unregister(void *, int);
PMOD_EXPORT void dmalloc_unregister_all(struct block_allocator *);
PMOD_EXPORT void *debug_malloc(size_t, LOCATION);
PMOD_EXPORT void *debug_calloc(size_t, size_t, LOCATION);
PMOD_EXPORT void *debug_realloc(void *, size_t, LOCATION);
PMOD_EXPORT void debug_free(void *, LOCATION, int);
PMOD_EXPORT char *debug_strdup(const char *, LOCATION);
void reset_debug_malloc(void);
PMOD_EXPORT int dmalloc_check_allocated (void *p, int must_be_freed);
PMOD_EXPORT void dmalloc_free(void *p);
PMOD_EXPORT int debug_malloc_touch_fd(int, LOCATION);
PMOD_EXPORT int debug_malloc_register_fd(int, LOCATION);
PMOD_EXPORT void debug_malloc_accept_leak_fd(int);
PMOD_EXPORT int debug_malloc_close_fd(int, LOCATION);
PMOD_EXPORT int dmalloc_mark_as_free(void*,int);

PMOD_EXPORT void *debug_malloc_update_location(void *, LOCATION);
PMOD_EXPORT void *debug_malloc_update_location_ptr(void *, ptrdiff_t, LOCATION);
PMOD_EXPORT void *debug_malloc_update_location_bt (void *p, const char *file,
						   INT_TYPE line, const char *name);
void search_all_memheaders_for_references(void);

/* Beware! names of named memory regions are never ever freed!! /Hubbe */
PMOD_EXPORT void *debug_malloc_name(void *p, const char *fn, INT_TYPE line);
PMOD_EXPORT int debug_malloc_copy_names(void *p, void *p2);
const char *dmalloc_find_name(void *p);

/* glibc 2.1 defines this as a macro. */
#ifdef strdup
#undef strdup
#endif

#define malloc(x) debug_malloc((x), DMALLOC_NAMED_LOCATION(" malloc"))
#define calloc(x, y) debug_calloc((x), (y), DMALLOC_NAMED_LOCATION(" calloc"))
#define realloc(x, y) debug_realloc((x), (y), DMALLOC_NAMED_LOCATION(" realloc"))
#define free(x) debug_free((x), DMALLOC_NAMED_LOCATION(" free"),0)
#define dmfree(x) debug_free((x),DMALLOC_NAMED_LOCATION(" free"),1)
#define strdup(x) debug_strdup((x), DMALLOC_NAMED_LOCATION(" strdup"))
#define DO_IF_DMALLOC(X) X
#define DO_IF_NOT_DMALLOC(X)
#define debug_malloc_touch(X) debug_malloc_update_location((void *)(X),DMALLOC_LOCATION())
#define debug_malloc_touch_named(X,NAME) debug_malloc_update_location((void *)(X),DMALLOC_NAMED_LOCATION(" " NAME))
#define debug_malloc_pass(X) debug_malloc_update_location((void *)(X),DMALLOC_LOCATION())
#define debug_malloc_pass_named(X,NAME) debug_malloc_update_location((void *)(X),DMALLOC_NAMED_LOCATION(" " NAME))
#define dmalloc_touch_struct_ptr(TYPE,X,MEMBER) ((TYPE)debug_malloc_update_location_ptr((void *)(X), ((ptrdiff_t)& (((TYPE)0)->MEMBER)), DMALLOC_LOCATION()))

/* These also save the call stack if support for that exists. Beware
 * that this can take a lot of extra memory - for temporary use only. */
#define debug_malloc_touch_bt(X) debug_malloc_update_location_bt ((void *)(X), __FILE__, __LINE__, NULL)
#define debug_malloc_touch_named_bt(X,NAME) debug_malloc_update_location_bt ((void *)(X), __FILE__, __LINE__, (NAME))

#define xalloc(X) ((void *)debug_malloc_update_location((void *)debug_xalloc(X), DMALLOC_NAMED_LOCATION(" xalloc")))
#define xcalloc(N, S) ((void *)debug_malloc_update_location((void *)debug_xcalloc(N, S), DMALLOC_NAMED_LOCATION(" xcalloc")))
#define xmalloc(X) ((void *)debug_malloc_update_location((void *)debug_xmalloc(X), DMALLOC_NAMED_LOCATION(" xmalloc")))
#define xrealloc(N, S) ((void *)debug_malloc_update_location((void *)debug_xrealloc(N, S), DMALLOC_NAMED_LOCATION(" xrealloc")))
#define xstrdup(X) ((void *)debug_malloc_update_location((void *)debug_xstrdup(X), DMALLOC_NAMED_LOCATION(" xstrdup")))
#define xfree(X) debug_xfree(debug_malloc_update_location((X), DMALLOC_NAMED_LOCATION(" free")))
PMOD_EXPORT void debug_malloc_dump_references(void *x, int indent, int depth, int flags);
#define dmalloc_touch(TYPE,X) ((TYPE) debug_malloc_pass (X))
#define dmalloc_touch_named(TYPE,X,NAME) ((TYPE) debug_malloc_pass_named (X, NAME))
void debug_malloc_dump_fd(int fd);
#define dmalloc_touch_svalue(X) do {		\
    const struct svalue *_tmp = (X);		\
    if (REFCOUNTED_TYPE(TYPEOF(*_tmp))) {	\
      debug_malloc_touch(_tmp->u.refs);		\
    }						\
  } while(0)
#define dmalloc_touch_svalue_named(X,NAME) do {		\
    const struct svalue *_tmp = (X);			\
    if (REFCOUNTED_TYPE(TYPEOF(*_tmp))) {		\
      debug_malloc_touch_named(_tmp->u.refs,NAME);	\
    }							\
  } while(0)

#define DMALLOC_LINE_ARGS ,char * dmalloc_location
#define DMALLOC_POS ,DMALLOC_LOCATION()
#define DMALLOC_PROXY_ARGS ,dmalloc_location
PMOD_EXPORT void dmalloc_accept_leak(void *);
#define dmalloc_touch_fd(X) debug_malloc_touch_fd((X),DMALLOC_LOCATION())
#define dmalloc_register_fd(X) debug_malloc_register_fd((X),DMALLOC_LOCATION())
#define dmalloc_accept_leak_fd(X) debug_malloc_accept_leak_fd(X)
#define dmalloc_close_fd(X) debug_malloc_close_fd((X),DMALLOC_LOCATION())

/* Beware, these do not exist without DMALLOC */
struct memory_map;
void dmalloc_set_mmap(void *ptr, struct memory_map *m);
void dmalloc_set_mmap_template(void *ptr, struct memory_map *m);
void dmalloc_set_mmap_from_template(void *p, void *p2);
void dmalloc_describe_location(void *p, int offset, int indent);
struct memory_map *dmalloc_alloc_mmap(char *name, INT_TYPE line);
void dmalloc_add_mmap_entry(struct memory_map *m,
			    char *name,
			    int offset,
			    int size,
			    int count,
			    struct memory_map *recur,
			    int recur_offset);
int dmalloc_is_invalid_memory_block(void *block);


#else /* DEBUG_MALLOC */

#ifdef USE_DL_MALLOC
PMOD_EXPORT void* dlmalloc(size_t);
PMOD_EXPORT void  dlfree(void*);
PMOD_EXPORT void* dlcalloc(size_t, size_t);
PMOD_EXPORT void* dlrealloc(void*, size_t);
PMOD_EXPORT void* dlmemalign(size_t, size_t);
PMOD_EXPORT void* dlvalloc(size_t);
PMOD_EXPORT void* dlpvalloc(size_t);
PMOD_EXPORT struct mallinfo dlmallinfo(void);
#define malloc(x) dlmalloc(x)
#define free	  dlfree
#define calloc    dlcalloc
#define realloc	  dlrealloc
#define memalign  dlmemalign
#define valloc	  dlvalloc
#define pvalloc	  dlpvalloc
/* No define for mallinfo since there's a struct with the same name. */
#ifdef strdup
#undef strdup
#endif
#define strdup    debug_xstrdup

#endif /* USE_DL_MALLOC */

#define dmalloc_touch_fd(X) (X)
#define dmalloc_register_fd(X) (X)
#define dmalloc_accept_leak_fd(X)
#define dmalloc_close_fd(X) (X)
#define dmfree(X) free((X))
#define dmalloc_accept_leak(X) (void)(X)
#define DMALLOC_LINE_ARGS
#define DMALLOC_POS
#define DMALLOC_PROXY_ARGS
#define debug_malloc_dump_references(X,x,y,z)
#define debug_malloc_dump_fd(fd)
#define xalloc debug_xalloc
#define xcalloc debug_xcalloc
#define xrealloc debug_xrealloc
#define xstrdup debug_xstrdup

#if defined(DYNAMIC_MODULE) && defined(__NT__) && !defined(USE_DLL)
#define xmalloc debug_xmalloc
#define xfree debug_xfree
#else
#define xmalloc malloc
#define xfree free
#endif /* defined(DYNAMIC_MODULE) && defined(__NT__) && !defined(USE_DLL) */

#define dbm_main main
#define DO_IF_DMALLOC(X)
#define DO_IF_NOT_DMALLOC(X) X
#define dmalloc_trace(X)
#define dmalloc_register(X,Y,Z)
#define dmalloc_unregister(X,Y) 1
#define dmalloc_unregister_all(X)
#define debug_free(X,Y,Z) free((X))
#define debug_malloc_name(P,FN,LINE)
#define debug_malloc_copy_names(p,p2) 0
#define search_all_memheaders_for_references()
#define dmalloc_find_name(X) "unknown (no dmalloc)"
#define dmalloc_touch_struct_ptr(TYPE,X,MEMBER) (X)

#ifdef DMALLOC_TRACE
#define debug_malloc_update_location(X,Y) (DMALLOC_TRACE_LOG(DMALLOC_LOCATION()),(X))
#define dmalloc_touch_svalue(X) DMALLOC_TRACE_LOG(DMALLOC_LOCATION())
#define dmalloc_touch_svalue_named(X,NAME) DMALLOC_TRACE_LOG(DMALLOC_NAMED_LOCATION(" " NAME))
#define debug_malloc_touch(X) DMALLOC_TRACE_LOG(DMALLOC_LOCATION())
#define debug_malloc_touch_named(X,NAME) DMALLOC_TRACE_LOG(DMALLOC_NAMED_LOCATION(" " NAME))
#define debug_malloc_pass(X) (DMALLOC_TRACE_LOG(DMALLOC_LOCATION()),(X))
#define debug_malloc_pass_named(X,NAME) (DMALLOC_TRACE_LOG(DMALLOC_NAMED_LOCATION(" " NAME)), (X))
#define debug_malloc_touch_bt(X) DMALLOC_TRACE_LOG(DMALLOC_LOCATION())
#define debug_malloc_touch_named_bt(X,NAME) DMALLOC_TRACE_LOG(DMALLOC_NAMED_LOCATION(" " NAME))
#define dmalloc_touch(TYPE,X) ((TYPE)debug_malloc_pass(X))
#define dmalloc_touch_named(TYPE,X,NAME) ((TYPE)debug_malloc_pass_named(X, NAME))
#else /* DMALLOC_TRACE */
#define debug_malloc_update_location(X,Y) (X)
#define dmalloc_touch_svalue(X)
#define dmalloc_touch_svalue_named(X,NAME)
#define debug_malloc_touch(X)
#define debug_malloc_touch_named(X,NAME)
#define debug_malloc_pass(X) (X)
#define debug_malloc_pass_named(X,NAME) (X)
#define debug_malloc_touch_bt(X)
#define debug_malloc_touch_named_bt(X,NAME)
#define dmalloc_touch(TYPE,X) ((TYPE)(X))
#define dmalloc_touch_named(TYPE,X,NAME) ((TYPE)(X))
#endif /* !MALLOC_TRACE */

#endif /* !DEBUG_MALLOC */

#endif	/* !DMALLOC_H */
