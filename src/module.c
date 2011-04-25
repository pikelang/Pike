/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
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

#ifdef PIKE_EXTRA_DEBUG
#define TRACE_MAIN
#define TRACE_MODULE
#endif

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

#ifdef DEBUG_MALLOC
  /* Make some statically allocated structs known to dmalloc. These
   * will still show up in the DO_PIKE_CLEANUP leak report if they
   * leak, in spite of dmalloc_accept_leak. */
  dmalloc_register (&empty_array, sizeof (empty_array),
		    DMALLOC_LOCATION());
  dmalloc_accept_leak (&empty_array);
  dmalloc_register (&weak_empty_array, sizeof (weak_empty_array),
		    DMALLOC_LOCATION());
  dmalloc_accept_leak (&weak_empty_array);
#endif

  TRACE((stderr, "Init cpp...\n"));

  init_cpp();

  TRACE((stderr, "Init backend...\n"));

  init_backend();

  TRACE((stderr, "Init iterators...\n"));

  init_iterators();

  TRACE((stderr, "Init searching...\n"));

  init_pike_searching();

  TRACE((stderr, "Init error handling...\n"));

  init_error();

  TRACE((stderr, "Init security system...\n"));

  init_pike_security();

  TRACE((stderr, "Init threads...\n"));

  th_init();

  TRACE((stderr, "Init operators...\n"));

  init_operators();


  TRACE((stderr, "Init builtin...\n"));

  init_builtin();


  TRACE((stderr, "Init efuns...\n"));

  init_builtin_efuns();

  TRACE((stderr, "Init signal handling...\n"));

  init_signals();

  TRACE((stderr, "Init dynamic loading...\n"));

  init_dynamic_load();
#ifdef WITH_FACETS

  TRACE((stderr, "Init facets...\n"));

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
  exit_builtin_efuns();
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

#ifdef PIKE_THREADS
  /* This zaps Pike_interpreter.thread_state among other things, so
   * THREADS_ALLOW/DISALLOW are NOPs beyond this point. */
  th_cleanup();
#endif
  free_all_pike_list_node_blocks();

  exit_pike_security();
  free_svalue(& throw_value);
  mark_free_svalue (&throw_value);

  do_gc(NULL, 1);

  if (exit_with_cleanup)
  {
    int leak_found = 0;

#ifdef PIKE_THREADS
    if(count_pike_threads())
    {
      fprintf(stderr,"Cleanup-on-exit aborted "
	      "because %d thread(s) still are running.\n",
	      count_pike_threads());
      exit_with_cleanup = 0;
      return;
    }
#endif

#ifdef DEBUG_MALLOC
    search_all_memheaders_for_references();
#endif

    /* The use of markers below only works after a gc run where it
     * hasn't freed anything. Since we've destructed all live objects
     * in exit_modules, nothing should be left after the run above, so
     * only one more run is necessary. */
    gc_keep_markers = 1;
    do_gc (NULL, 1);

#define STATIC_ARRAYS &empty_array, &weak_empty_array,

#define REPORT_LINKED_LIST_LEAKS(TYPE, START, STATICS, T_TYPE, NAME,	\
				 PRINT_EXTRA) do {			\
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
	  /*static const*/ struct TYPE *statics[] = {STATICS NULL};	\
	  ptrdiff_t i; /* Use signed type to avoid warnings from gcc. */ \
	  for (i = 0; i < (ptrdiff_t) (NELEM (statics) - 1); i++)	\
	    if (x == statics[i])					\
	      is_static = 1;						\
	  /* Note: m->xrefs is always zero here since the mark pass	\
	   * isn't run in gc_destruct_everything mode. */		\
	  if (x->refs != m->refs + is_static) {				\
	    if (!leak_found) {						\
	      fputs ("Leak(s) found at exit:\n", stderr);		\
	      leak_found = 1;						\
	    }								\
	    fprintf (stderr, NAME " at %p got %d unaccounted refs "	\
		     "(and %d accounted): ",				\
		     x, x->refs - (m->refs + is_static),		\
		     m->refs + is_static);				\
	    safe_print_short_svalue (stderr, (union anything *) &x, T_TYPE); \
	    {PRINT_EXTRA;}						\
	    fputc ('\n', stderr);					\
	    /* describe (x); */						\
	    DO_IF_DMALLOC (						\
	      debug_malloc_dump_references (x, 2, 1, 0);		\
	      fputc ('\n', stderr);					\
	    );								\
	    DO_IF_DEBUG (m->flags |= GC_CLEANUP_LEAKED);		\
	  }								\
	}								\
      }									\
    } while (0)

    REPORT_LINKED_LIST_LEAKS (array, first_array, STATIC_ARRAYS, T_ARRAY, "Array", {});
    REPORT_LINKED_LIST_LEAKS (multiset, first_multiset, NOTHING, T_MULTISET, "Multiset", {});
    REPORT_LINKED_LIST_LEAKS (mapping, first_mapping, NOTHING, T_MAPPING, "Mapping", {});
    REPORT_LINKED_LIST_LEAKS (
      program, first_program, NOTHING, T_PROGRAM, "Program",
      {
	/* This kind of info is rarely useful - the output from
	 * print_short_svalue is usually enough to identify the program, and
	 * the dmalloc ref dump shows where it has been used.
	DO_IF_DEBUG(
	  struct program *p = (struct program *)x;
	  fputc ('\n', stderr);
	  if (p->parent) {
	    fprintf(stderr, "    Parent is: %p\n",
		    p->parent);
	    dump_program_tables(p->parent, 6);
	  }
	  fprintf(stderr, "  Symbol tables:\n");
	  dump_program_tables(p, 4);
	);
	*/
      }
    );
    REPORT_LINKED_LIST_LEAKS (
      object, first_object, NOTHING, T_OBJECT, "Object", {
	if (!x->prog) {
	  fputs (" (destructed)", stderr);
	  DO_IF_DEBUG ({
	      struct program *p = id_to_program (x->program_id);
	      if (p) {
		fputs ("\n  Program for destructed object: ", stderr);
		safe_print_short_svalue (stderr,
					 (union anything *) &p, T_PROGRAM);
	      }
	      else
		fprintf (stderr, "\n  Program for destructed object gone too, "
			 "its id was: %d", x->program_id);
	    });
	}
      });

    {
      size_t index;
      for (index = 0; index < pike_type_hash_size; index++) {
	REPORT_LINKED_LIST_LEAKS(pike_type, pike_type_hash[index], NOTHING, PIKE_T_TYPE, "Type", {});
      }
    }

#undef REPORT_LINKED_LIST_LEAKS

    /* Just remove the extra external refs reported above and do
     * another gc so that we don't report the blocks again in the low
     * level dmalloc reports. */

#ifdef PIKE_DEBUG
    /* If we stumble on the real refs whose refcounts we're zapping,
     * we should try to handle it gracefully, and log all frees. */
    gc_external_refs_zapped = 1;
#endif

#ifdef VALGRIND_DO_LEAK_CHECK
    /* Let valgrind print a leak report before we free the leaked
     * blocks. Ideally we should only free the svalues inside them
     * below and make the report afterwards. */
    VALGRIND_DO_LEAK_CHECK;
#endif

#define ZAP_LINKED_LIST_LEAKS(TYPE, START, STATICS) do {		\
      struct TYPE *x, *next;						\
      for (x = START; x; x = next) {					\
	struct marker *m = find_marker (x);				\
	next = x->next;							\
	if (m) {							\
	  int is_static = 0;						\
	  static const struct TYPE *statics[] = {STATICS NULL};		\
	  ptrdiff_t i; /* Use signed type to avoid warnings from gcc. */ \
	  INT32 refs;							\
	  for (i = 0; i < (ptrdiff_t) (NELEM (statics) - 1); i++)	\
	    if (x == statics[i])					\
	      is_static = 1;						\
	  refs = x->refs;						\
	  while (refs > m->refs + is_static) {				\
	    PIKE_CONCAT(free_, TYPE) (x);				\
	    refs--;							\
	  }								\
	}								\
      }									\
    } while (0)

    ZAP_LINKED_LIST_LEAKS (array, first_array, STATIC_ARRAYS);
    ZAP_LINKED_LIST_LEAKS (multiset, first_multiset, NOTHING);
    ZAP_LINKED_LIST_LEAKS (mapping, first_mapping, NOTHING);
    ZAP_LINKED_LIST_LEAKS (program, first_program, NOTHING);
    ZAP_LINKED_LIST_LEAKS (object, first_object, NOTHING);

    {
      size_t index;
      for (index = 0; index < pike_type_hash_size; index++) {
	ZAP_LINKED_LIST_LEAKS(pike_type, pike_type_hash[index], NOTHING);
      }
    }

#undef ZAP_LINKED_LIST_LEAKS

    do_gc (NULL, 1);

    gc_keep_markers = 0;
    exit_gc();
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
#endif /* DO_PIKE_CLEANUP */
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

  enter_compiler(NULL, 1);

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

  exit_compiler();
}

void exit_modules(void)
{
  JMP_BUF recovery;
  volatile int e;

#ifdef DO_PIKE_CLEANUP
  size_t count;

  if (exit_with_cleanup) {
    /* Kill the threads and destruct all remaining objects while we
     * have a proper execution environment. The downside is that the
     * leak report below will always report destructed objects. We use
     * the gc in a special mode for this to get a reasonably sane
     * destruct order. */
#ifdef PIKE_THREADS
    cleanup_all_other_threads();
#endif
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
#ifdef PIKE_DEBUG
    if (!count) {
      struct object *o;
      for (o = first_object; o; o = o->next)
	if (gc_object_is_live (o))
	  gc_fatal (o, 0, "Object missed in gc_destruct_everything mode.\n");
      for (o = objects_to_destruct; o; o = o->next)
	if (gc_object_is_live (o))
	  gc_fatal (o, 0, "Object missed in gc_destruct_everything mode"
		    " (is on objects_to_destruct list).\n");
    }
#endif /* PIKE_DEBUG */
    gc_destruct_everything = 0;
    exit_cleanup_in_progress = 1; /* Warn about object creation from now on. */
  }

  /* Unload dynamic modules before static ones. */
  exit_dynamic_load();
#endif /* DO_PIKE_CLEANUP */

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
