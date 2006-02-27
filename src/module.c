/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: module.c,v 1.39 2006/02/27 12:33:55 mast Exp $
*/

#include "global.h"
#include "module.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "main.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "mapping.h"
#include "program_id.h"
#include "lex.h"
#include "pike_security.h"
#include "cpp.h"
#include "backend.h"
#include "threads.h"
#include "operators.h"
#include "signal_handler.h"
#include "dynamic_load.h"
#include "gc.h"
#include "multiset.h"
#include "pike_types.h"
#include "constants.h"
#include "bignum.h"
#include "module_support.h"

#include "modules/modlist_headers.h"
#ifndef PRE_PIKE
#include "post_modules/modlist_headers.h"
#endif

/* Define this to trace the initialization and cleanup of static modules. */
/* #define TRACE_MODULE */

#if defined(TRACE_MAIN) || defined(TRACE_MODULE)
#define TRACE(X)	fprintf X
#else /* !TRACE_MAIN */
#define TRACE(X)
#endif /* TRACE_MAIN */

static void init_builtin_modules(void)
{
  void init_iterators(void);
#ifdef WITH_FACETS
  void init_facetgroup(void);
#endif

  init_cpp();
  init_backend();
  init_iterators();
  init_pike_searching();
  init_error();
  init_pike_security();
  th_init();
  init_operators();

  init_builtin();

  init_builtin_efuns();
  init_signals();
  init_dynamic_load();
#ifdef WITH_FACETS
  init_facetgroup();
#endif
}

static void exit_builtin_modules(void)
{
#ifdef DO_PIKE_CLEANUP
  void exit_iterators(void);
#ifdef WITH_FACETS
  void exit_facetgroup(void);
#endif

  /* Clear various global references. */

#ifdef AUTO_BIGNUM
  exit_auto_bignum();
#endif
  exit_pike_searching();
  exit_object();
  exit_signals();
  exit_builtin();
  exit_cpp();
  cleanup_interpret();
  exit_builtin_constants();
  cleanup_module_support();
  exit_operators();
  exit_iterators();
#ifdef WITH_FACETS
  exit_facetgroup();
#endif
  cleanup_program();
  cleanup_compiler();
  cleanup_error();
  exit_backend();
  cleanup_gc();
  cleanup_pike_types();

  /* This zaps Pike_interpreter.thread_state among other things, so
   * THREADS_ALLOW/DISALLOW are NOPs beyond this point. */
  th_cleanup();

#ifdef SHARED_NODES
  free(node_hash.table);
#endif /* SHARED_NODES */

  exit_pike_security();
  free_svalue(& throw_value);
  throw_value.type=T_INT;

  do_gc(NULL, 1);

  if (exit_with_cleanup)
  {
    int leak_found = 0;

#ifdef _REENTRANT
    if(count_pike_threads()>1)
    {
      fprintf(stderr,"Byte counting aborted, because all threads have not exited properly.\n");
      exit_with_cleanup = 0;
      return;
    }
#endif

#ifdef DEBUG_MALLOC
    search_all_memheaders_for_references();
#endif

    /* The use of markers below only works after a gc run where it
     * hasn't freed anything. Since we've destructed all objects in
     * exit_main, nothing should be left after the run above, so only
     * one more run is necessary. */
    gc_keep_markers = 1;
    do_gc (NULL, 1);

#define STATIC_ARRAYS {&empty_array, &weak_empty_array}

#define REPORT_LINKED_LIST_LEAKS(TYPE, START, STATICS, T_TYPE, NAME) do { \
      struct TYPE *x;							\
      for (x = START; x; x = x->next) {					\
	struct marker *m = find_marker (x);				\
	if (!m) {							\
	  DO_IF_DEBUG (							\
	    fprintf (stderr, "Didn't find gc marker as expected for:\n"); \
	    describe_something (x, T_TYPE, 2, 2, 0, NULL);		\
	  );								\
	}								\
	else {								\
	  int is_static = 0;						\
	  static const struct TYPE *statics[] = STATICS;		\
	  ptrdiff_t i; /* Use signed type to avoid warnings from gcc. */ \
	  for (i = 0; i < (ptrdiff_t) NELEM (statics); i++)		\
	    if (x == statics[i])					\
	      is_static = 1;						\
	  if (x->refs != m->refs + is_static) {				\
	    if (!leak_found) {						\
	      fputs ("Leak(s) found at exit:\n", stderr);		\
	      leak_found = 1;						\
	    }								\
	    fprintf (stderr, NAME " got %d unaccounted references: ",	\
		     x->refs - (m->refs + is_static));			\
	    print_short_svalue (stderr, (union anything *) &x, T_TYPE);	\
	    fputc ('\n', stderr);					\
	    DO_IF_DMALLOC (debug_malloc_dump_references (x, 0, 1, 0));	\
	  }								\
	}								\
      }									\
    } while (0)

    REPORT_LINKED_LIST_LEAKS (array, first_array, STATIC_ARRAYS, T_ARRAY, "Array");
    REPORT_LINKED_LIST_LEAKS (multiset, first_multiset, {}, T_MULTISET, "Multiset");
    REPORT_LINKED_LIST_LEAKS (mapping, first_mapping, {}, T_MAPPING, "Mapping");
    REPORT_LINKED_LIST_LEAKS (program, first_program, {}, T_PROGRAM, "Program");
    REPORT_LINKED_LIST_LEAKS (object, first_object, {}, T_OBJECT, "Object");

#undef REPORT_LINKED_LIST_LEAKS

    /* Just remove the extra external refs reported above and do
     * another gc so that we don't report the blocks again in the low
     * level dmalloc reports. */

#if 1
    /* It can be a good idea to disable this to leave the blocks
     * around to be reported by an external memchecker like valgrind.
     * Ideally we should only free the svalues inside these things but
     * leave the blocks themselves. */

#define ZAP_LINKED_LIST_LEAKS(TYPE, START, STATICS) do {		\
      struct TYPE *x, *next;						\
      for (x = START; x; x = next) {					\
	struct marker *m = find_marker (x);				\
	next = x->next;							\
	if (m) {							\
	  int is_static = 0;						\
	  static const struct TYPE *statics[] = STATICS;		\
	  ptrdiff_t i; /* Use signed type to avoid warnings from gcc. */ \
	  INT32 refs;							\
	  for (i = 0; i < (ptrdiff_t) NELEM (statics); i++)		\
	    if (x == statics[i])					\
	      is_static = 1;						\
	  refs = x->refs;						\
	  while (refs > m->refs + is_static) {				\
	    DO_IF_DEBUG (m->flags |= GC_CLEANUP_FREED);			\
	    PIKE_CONCAT(free_, TYPE) (x);				\
	    refs--;							\
	  }								\
	}								\
      }									\
    } while (0)

    ZAP_LINKED_LIST_LEAKS (array, first_array, STATIC_ARRAYS);
    ZAP_LINKED_LIST_LEAKS (multiset, first_multiset, {});
    ZAP_LINKED_LIST_LEAKS (mapping, first_mapping, {});
    ZAP_LINKED_LIST_LEAKS (program, first_program, {});
    ZAP_LINKED_LIST_LEAKS (object, first_object, {});

#undef ZAP_LINKED_LIST_LEAKS

#ifdef PIKE_DEBUG
    /* If we stumble on the real refs whose refcounts we've zapped
     * above we should try to handle it gracefully. */
    gc_external_refs_zapped = 1;
#endif

#endif

    do_gc (NULL, 1);

    gc_keep_markers = 0;
    exit_gc();

#ifdef DEBUG_MALLOC
    {
      INT32 num, size;
      count_memory_in_pike_types(&num, &size);
      if (num)
	fprintf(stderr, "Types left: %d (%d bytes)\n", num, size);
      describe_all_types();
    }
#endif
  }

  destruct_objects_to_destruct_cb();

  /* Now there are no arrays/objects/programs/anything left. */

  really_clean_up_interpret();

  cleanup_callbacks();
  free_all_callable_blocks();
  exit_destroy_called_mark_hash();

  cleanup_pike_type_table();
  cleanup_shared_string_table();

  free_dynamic_load();
  first_mapping=0;
  free_all_mapping_blocks();
  first_object=0;
  free_all_object_blocks();
  first_program=0;
  free_all_program_blocks();
  exit_multiset();
#endif
}

typedef void (*modfun)(void);

struct static_module
{
  char *name;
  modfun init;
  modfun exit;
};

static const struct static_module module_list[] = {
  { "Builtin", init_builtin_modules, exit_builtin_modules }
#include "modules/modlist.h"
#ifndef PRE_PIKE
#include "post_modules/modlist.h"
#endif
};

void init_modules(void)
{
  struct program *p = NULL;
  volatile unsigned int e;
  struct lex save_lex;

  save_lex = lex;
  lex.current_line=1;
  lex.current_file=make_shared_string("-");

  start_new_program();
  Pike_compiler->new_program->id=PROG___BUILTIN_ID;

  for(e=0;e<NELEM(module_list);e++)
  {
    JMP_BUF recovery;
    if (!p) {
      start_new_program();
      p = Pike_compiler->new_program;
    }
    if(SETJMP(recovery)) {
      /* FIXME: We could loop here until we find p. */
      free_program(end_program());
      p = NULL;
      call_handle_error();
    } else {
      TRACE((stderr, "Initializing static module #%d: \"%s\"...\n",
	     e, module_list[e].name));
      module_list[e].init();
      if (
#if 0
	  Pike_compiler->new_program->num_identifier_references
#else /* !0 */
	  1
#endif /* 0 */
	  ) {
	debug_end_class(module_list[e].name,strlen(module_list[e].name),0);
	p = NULL;
      } else {
	/* No identifier references -- Disabled module. */
      }
    }
    UNSETJMP(recovery);
  }
  if (p) free_program(end_program());
  push_text("_static_modules");
  push_object(low_clone(p=end_program()));
  f_add_constant(2);
  free_program(p);
  free_string(lex.current_file);
  lex = save_lex;
}

void exit_modules(void)
{
  JMP_BUF recovery;
  volatile int e;

#ifdef DO_PIKE_CLEANUP
  size_t count;

  if (exit_with_cleanup) {
    /* Destruct all remaining objects while we have a proper execution
     * environment. The downside is that the leak report below will
     * always report destructed objects. We use the gc in a special mode
     * for this to get a reasonably sane destruct order. */
    gc_destruct_everything = 1;
    count = do_gc (NULL, 1);
    while (count) {
      size_t new_count = do_gc (NULL, 1);
      if (new_count >= count) {
	fprintf (stderr, "Some destroy function is creating new objects "
		 "during final cleanup - can't exit cleanly.\n");
	break;
      }
      count = new_count;
    }
    gc_destruct_everything = 0;
    exit_cleanup_in_progress = 1; /* Warn about object creation from now on. */
  }

  /* Unload dynamic modules before static ones. */
  exit_dynamic_load();
#endif

  for(e=NELEM(module_list)-1;e>=0;e--)
  {
    if(SETJMP(recovery))
      call_handle_error();
    else {
      TRACE((stderr, "Exiting static module #%d: \"%s\"...\n",
	     e, module_list[e].name));
      module_list[e].exit();
    }
    UNSETJMP(recovery);
  }
}
