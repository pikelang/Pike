/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "object.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "program.h"
#include "stralloc.h"
#include "svalue.h"
#include "pike_macros.h"
#include "pike_memory.h"
#include "pike_error.h"
#include "main.h"
#include "array.h"
#include "gc.h"
#include "backend.h"
#include "callback.h"
#include "cpp.h"
#include "builtin_functions.h"
#include "cyclic.h"
#include "pike_security.h"
#include "module_support.h"
#include "fdlib.h"
#include "mapping.h"
#include "multiset.h"
#include "constants.h"
#include "encode.h"
#include "pike_types.h"
#include "operators.h"

#include "block_alloc.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include <sys/stat.h>

#include "dmalloc.h"

#define sp Pike_sp

/* #define GC_VERBOSE */
/* #define DEBUG */


#ifndef SEEK_SET
#ifdef L_SET
#define SEEK_SET	L_SET
#else /* !L_SET */
#define SEEK_SET	0
#endif /* L_SET */
#endif /* SEEK_SET */
#ifndef SEEK_CUR
#ifdef L_INCR
#define SEEK_SET	L_INCR
#else /* !L_INCR */
#define SEEK_CUR	1
#endif /* L_INCR */
#endif /* SEEK_CUR */
#ifndef SEEK_END
#ifdef L_XTND
#define SEEK_END	L_XTND
#else /* !L_XTND */
#define SEEK_END	2
#endif /* L_XTND */
#endif /* SEEK_END */


struct object *master_object = 0;
struct program *master_program =0;
static int master_is_cleaned_up = 0;
PMOD_EXPORT struct object *first_object;

struct object *gc_internal_object = 0;
static struct object *gc_mark_object_pos = 0;

#undef COUNT_OTHER

#define COUNT_OTHER() do{			\
  struct object *o;                             \
  for(o=first_object;o;o=o->next)		\
    if(o->prog)					\
      size+=o->prog->storage_needed;		\
						\
  for(o=objects_to_destruct;o;o=o->next)	\
    if(o->prog)					\
      size+=o->prog->storage_needed;		\
}while(0)
BLOCK_ALLOC_FILL_PAGES(object, 2)

#undef COUNT_OTHER
#define COUNT_OTHER()

PMOD_EXPORT struct object *low_clone(struct program *p)
{
  struct object *o;

  if(!(p->flags & PROGRAM_PASS_1_DONE))
    Pike_error("Attempting to clone an unfinished program\n");

#ifdef PROFILING
  p->num_clones++;
#endif /* PROFILING */

  o=alloc_object();

  o->storage=p->storage_needed ? (char *)xcalloc(p->storage_needed, 1) : (char *)NULL;

  GC_ALLOC(o);

#ifdef DO_PIKE_CLEANUP
  if (exit_cleanup_in_progress) {
    INT_TYPE line;
    struct pike_string *file = get_program_line (p, &line);
    fprintf (stderr,
	     "Warning: Object %p created during exit cleanup from %s:%ld\n",
	     o, file->str, (long)line);
  }
#endif

#ifdef DEBUG_MALLOC
  if(!debug_malloc_copy_names(o, p)) 
  {
    struct pike_string *tmp;
    INT_TYPE line;

    tmp=get_program_line(p, &line);
    debug_malloc_name(o, tmp->str, line);
    free_string(tmp);
  }
  dmalloc_set_mmap_from_template(o,p);
#endif

  add_ref( o->prog=p );

  if(p->flags & PROGRAM_USES_PARENT)
  {
    LOW_PARENT_INFO(o,p)->parent=0;
    LOW_PARENT_INFO(o,p)->parent_identifier=0;
  }

  DOUBLELINK(first_object,o);

  INIT_PIKE_MEMOBJ(o, T_OBJECT);

#ifdef PIKE_DEBUG
  o->program_id=p->id;
#endif

  return o;
}

#define LOW_PUSH_FRAME2(O, P)			\
  pike_frame=alloc_pike_frame();		\
  pike_frame->next=Pike_fp;			\
  pike_frame->current_object=O;			\
  pike_frame->current_program=P;		\
  pike_frame->locals=0;				\
  pike_frame->num_locals=0;			\
  pike_frame->fun = FUNCTION_BUILTIN;		\
  pike_frame->pc=0;				\
  pike_frame->context=NULL;                     \
  Pike_fp = pike_frame

#define PUSH_FRAME2(O, P) do{			\
    LOW_PUSH_FRAME2(O, P);			\
    add_ref(pike_frame->current_object);	\
    add_ref(pike_frame->current_program);	\
  }while(0)

#define LOW_SET_FRAME_CONTEXT(X)					\
  Pike_fp->context = (X);						\
  Pike_fp->fun = FUNCTION_BUILTIN;					\
  Pike_fp->current_storage = o->storage + Pike_fp->context->storage_offset

#define SET_FRAME_CONTEXT(X)						\
  LOW_SET_FRAME_CONTEXT(X)
  
#define LOW_UNSET_FRAME_CONTEXT()		\
  Pike_fp->context = NULL;			\
  Pike_fp->current_storage = NULL
  

#ifdef DEBUG
#define CHECK_FRAME() do { \
    if(pike_frame != Pike_fp) \
      Pike_fatal("Frame stack out of whack.\n"); \
  } while(0)
#else
#define CHECK_FRAME()	0
#endif

#define POP_FRAME2()				\
  do{CHECK_FRAME();				\
  Pike_fp=pike_frame->next;			\
  pike_frame->next=0;				\
  free_pike_frame(pike_frame);}while(0)

PMOD_EXPORT void call_c_initializers(struct object *o)
{
  int e;
  struct program *p=o->prog;
  struct pike_frame *pike_frame=0;
  int frame_pushed = 0;

  /* NOTE: This function is only called for objects straight after
   *       low_clone(), or after an explicit xcalloc(), which implies
   *       that the storage (if any) has been zeroed.
   */

  /* clear globals and call C initializers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    struct program *prog = p->inherits[e].prog;
    int n=(int)prog->num_variable_index;
    if( n )
    {
      char *storage = o->storage+p->inherits[e].storage_offset;
      int q;
      for(q=0;q<n;q++)
      {
	int d=prog->variable_index[q];

	if (IDENTIFIER_IS_ALIAS(prog->identifiers[d].identifier_flags))
	  continue;

	if(prog->identifiers[d].run_time_type == T_MIXED)
	{
	  struct svalue *s;
	  s=(struct svalue *)(storage + prog->identifiers[d].func.offset);
	  SET_SVAL_TYPE(*s, T_INT);
#ifdef NEED_CUSTOM_IEEE
	} else if (prog->identifiers[d].run_time_type == T_FLOAT) {
	  /* Note: In IEEE representations the value 0.0 is represented as all
	   *       zeros, and the default initialization is thus sufficient.
	   */
	  union anything *u;
	  u=(union anything *)(storage + prog->identifiers[d].func.offset);
	  u->float_number=0.0;
#endif /* NEED_CUSTOM_IEEE */
	}
	(void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" clear_global"));
      }
    }
    if(prog->event_handler)
    {
      if( !frame_pushed )
      {
	PUSH_FRAME2(o, p);
	frame_pushed = 1;
      }
      SET_FRAME_CONTEXT(p->inherits + e);
      prog->event_handler(PROG_EVENT_INIT);
    }
  }
  if( frame_pushed )
    POP_FRAME2();
}


PMOD_EXPORT void call_prog_event(struct object *o, int event)
{
  int e;
  struct program *p=o->prog;
  struct pike_frame *pike_frame=0;
  int frame_pushed = 0;

  /* call event handlers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    struct program *prog = p->inherits[e].prog;
    if(prog->event_handler)
    {
      if( !frame_pushed )
      {
	PUSH_FRAME2(o, p);
	frame_pushed = 1;
      }
      SET_FRAME_CONTEXT(p->inherits + e);
      prog->event_handler(event);
    }
  }
  if( frame_pushed )
    POP_FRAME2();
}


void call_pike_initializers(struct object *o, int args)
{
  ptrdiff_t fun;
  struct program *p=o->prog;
  if(!p) return;
  STACK_LEVEL_START(args);
  fun=FIND_LFUN(p, LFUN___INIT);
  if(fun!=-1)
  {
    apply_low(o,fun,0);
    Pike_sp--;
  }
  STACK_LEVEL_CHECK(args);
  fun=FIND_LFUN(p, LFUN_CREATE);
  if(fun!=-1)
  {
    apply_low(o,fun,args);
    pop_stack();
  } else {
    pop_n_elems(args);
  }
  STACK_LEVEL_DONE(0);
}

PMOD_EXPORT void do_free_object(struct object *o)
{
  if (o)
    free_object(o);
}

PMOD_EXPORT struct object *debug_clone_object(struct program *p, int args)
{
  ONERROR tmp;
  struct object *o;
  if(p->flags & PROGRAM_NEEDS_PARENT)
    Pike_error("Parent lost, cannot clone program.\n");

  if(!(p->flags & PROGRAM_PASS_1_DONE))
    Pike_error("Attempting to clone an unfinished program\n");

  o=low_clone(p);
  if (!args) {
    push_object(o);
  } else {
    SET_ONERROR(tmp, do_free_object, o);
    debug_malloc_touch(o);
  }
  call_c_initializers(o);
  debug_malloc_touch(o);
  call_pike_initializers(o,args);
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);
  if (!args) {
    /* Pop o from the stack. */
    Pike_sp--;
  } else {
    UNSET_ONERROR(tmp);
  }
  return o;
}

/* Clone and initialize an object, but do not call the pike initializers.
 *
 * WARNING: Only use this function if you know what you are doing...
 */
PMOD_EXPORT struct object *fast_clone_object(struct program *p)
{
  struct object *o=low_clone(p);
  push_object(o);
  call_c_initializers(o);
  Pike_sp--;
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);
  return o;
}

PMOD_EXPORT struct object *parent_clone_object(struct program *p,
					       struct object *parent,
					       ptrdiff_t parent_identifier,
					       int args)
{
  ONERROR tmp;
  struct object *o=low_clone(p);
  if (!args) {
    push_object(o);
  } else {
    SET_ONERROR(tmp, do_free_object, o);
    debug_malloc_touch(o);
  }

  if(!(p->flags & PROGRAM_PASS_1_DONE))
    Pike_error("Attempting to clone an unfinished program\n");

  if(p->flags & PROGRAM_USES_PARENT)
  {
    add_ref( PARENT_INFO(o)->parent=parent );
    PARENT_INFO(o)->parent_identifier = DO_NOT_WARN((INT32)parent_identifier);
  }
  call_c_initializers(o);
  call_pike_initializers(o,args);
  if (!args) {
    Pike_sp--;
  } else {
    UNSET_ONERROR(tmp);
  }
  return o;
}

PMOD_EXPORT struct object *clone_object_from_object(struct object *o, int args)
{
  if (o->prog->flags & PROGRAM_USES_PARENT)
    return parent_clone_object(o->prog,
			       PARENT_INFO(o)->parent,
			       PARENT_INFO(o)->parent_identifier,
			       args);
  else
    return clone_object(o->prog, args);
}

/* BEWARE: This function does not call create() or __INIT() */
struct object *decode_value_clone_object(struct svalue *prog)
{
  struct object *o, *parent;
  ONERROR tmp;
  INT32 parent_identifier;
  struct program *p=program_from_svalue(prog);
  if(!p) return NULL;

  o=low_clone(p);
  SET_ONERROR(tmp, do_free_object, o);
  debug_malloc_touch(o);
  
  if(TYPEOF(*prog) == T_FUNCTION)
  {
    parent=prog->u.object;
    parent_identifier = SUBTYPEOF(*prog);
  }else{
    parent=0;
    parent_identifier=0;
  }

  if(p->flags & PROGRAM_USES_PARENT)
  {
    
    PARENT_INFO(o)->parent=parent;
    if(parent) add_ref(parent);
    PARENT_INFO(o)->parent_identifier = DO_NOT_WARN((INT32)parent_identifier);
  }
  call_c_initializers(o);
  UNSET_ONERROR(tmp);
  return o;
}

static struct pike_string *low_read_file(const char *file)
{
  struct pike_string *s;
  PIKE_OFF_T len;
  FD f;

  while((f = fd_open(file,fd_RDONLY|fd_BINARY,0666)) <0 && errno==EINTR)
    check_threads_etc();
  if(f >= 0)
  {
    PIKE_OFF_T tmp, pos = 0;

    len = fd_lseek(f, 0, SEEK_END);
    fd_lseek(f, 0, SEEK_SET);

    if (len > MAX_INT32)
      Pike_fatal ("low_read_file(%s): File too large: %"PRINTPIKEOFFT"d b.\n",
		  file, len);

    s = begin_shared_string (DO_NOT_WARN ((ptrdiff_t) len));

    while(pos<len)
    {
      tmp = fd_read(f,s->str+pos, DO_NOT_WARN ((ptrdiff_t) len) - pos);
      if(tmp<=0)
      {
	if (tmp < 0) {
	  if(errno==EINTR) {
	    check_threads_etc();
	    continue;
	  }
	  Pike_fatal("low_read_file(%s) failed, errno=%d\n",file,errno);
	}
	Pike_fatal("low_read_file(%s) failed, short read: "
		   "%"PRINTPIKEOFFT"d < %"PRINTPIKEOFFT"d\n",
		   file, pos, len);
      }
      pos+=tmp;
    }
    fd_close(f);
    return end_shared_string(s);
  }
  return 0;
}

static void get_master_cleanup (void *UNUSED(dummy))
{
  if (master_object) {
    free_object (master_object);
    master_object = 0;
  }
}

PMOD_EXPORT struct object *get_master(void)
{
  static int inside=0;

  if(master_object && master_object->prog)
    return master_object;

  if(inside || master_is_cleaned_up) return 0;

  if(master_object)
  {
    free_object(master_object);
    master_object=0;
  }

  inside = 1;

  /* fprintf(stderr, "Need a new master object...\n"); */

  if(!master_program)
  {
    struct pike_string *s;
    char *tmp;
    PIKE_STAT_T stat_buf;

    if(!get_builtin_constants() ||
       !simple_mapping_string_lookup(get_builtin_constants(),
				     "_static_modules"))
    {
      inside = 0;
      /* fprintf(stderr, "Builtin_constants: %p\n", get_builtin_constants()); */
      /* fprintf(stderr,"Cannot load master object yet!\n"); */
      return 0;
    }

    /* fprintf(stderr, "Master file: \"%s\"\n", master_file); */

    tmp=xalloc(strlen(master_file)+3);

    MEMCPY(tmp, master_file, strlen(master_file)+1);
    strcat(tmp,".o");

    s = NULL;
    if (!fd_stat(tmp, &stat_buf)) {
      time_t ts1 = stat_buf.st_mtime;
      time_t ts2 = 0;

      if (!fd_stat(master_file, &stat_buf)) {
	ts2 = stat_buf.st_mtime;
      }

      if (ts1 >= ts2) {
	s = low_read_file(tmp);
      }
    }
    free(tmp);
    if(s)
    {
      JMP_BUF tmp;

      /* fprintf(stderr, "Trying precompiled file \"%s.o\"...\n",
       *         master_file);
       */

      /* Moved here to avoid gcc warning: "might be clobbered". */
      push_string(s);
      push_int(0);

      if(SETJMP_SP(tmp, 2))
      {
#ifdef DEBUG
	if(d_flag) {
	  struct svalue sval;
	  /* Note: Save the svalue before attempting to describe it,
	   *       since it might get overwritten... */
	  assign_svalue_no_free(&sval, &throw_value);
	  debug_describe_svalue(&sval);
	  free_svalue(&sval);
	}
#endif
	/* do nothing */
	UNSETJMP(tmp);
	free_svalue(&throw_value);
	mark_free_svalue (&throw_value);
      }else{
	f_decode_value(2);
	UNSETJMP(tmp);

	if(TYPEOF(sp[-1]) == T_PROGRAM)
	  goto compiled;

	pop_stack();

      }
#ifdef DEBUG
      if(d_flag)
	fprintf(stderr,"Failed to import dumped master!\n");
#endif

    }

    /* fprintf(stderr, "Reading master: \"%s\"...\n", master_file); */

    s=low_read_file(master_file);
    if(s)
    {
      push_string(s);
      push_text(master_file);

      /* fprintf(stderr, "Calling cpp()...\n"); */

      f_cpp(2);

      /* fprintf(stderr, "Calling compile()...\n"); */

      f_compile(1);

    compiled:
      if(TYPEOF(sp[-1]) != T_PROGRAM)
      {
	pop_stack();
	return 0;
      }
      master_program=sp[-1].u.program;
      sp--;
      dmalloc_touch_svalue(sp);
    }else{
      throw_error_object(fast_clone_object(master_load_error_program), 0, 0, 0,
			 "Couldn't load master program from %s.\n", master_file);
    }
  }

  {
    int f;
    ONERROR uwp;

    /* fprintf(stderr, "Cloning master...\n"); */

    master_object=low_clone(master_program);
    debug_malloc_touch(master_object);
    debug_malloc_touch(master_object->storage);

    /* Make sure master_object doesn't point to a broken master. */
    SET_ONERROR (uwp, get_master_cleanup, NULL);

    /* fprintf(stderr, "Initializing master...\n"); */

    call_c_initializers(master_object);
    call_pike_initializers(master_object,0);

    f = find_identifier ("is_pike_master", master_program);
    if (f >= 0)
      object_low_set_index (master_object, f, (struct svalue *)&svalue_int_one);

    /* fprintf(stderr, "Master loaded.\n"); */

    UNSET_ONERROR (uwp);
  }

  inside = 0;
  return master_object;
}

PMOD_EXPORT struct object *debug_master(void)
{
  struct object *o;
  o=get_master();
  if(!o) Pike_fatal("Couldn't load master object.\n");
  return o;
}

struct destroy_called_mark
{
  struct destroy_called_mark *next;
  void *data;
  struct program *p; /* for magic */
};

PTR_HASH_ALLOC(destroy_called_mark,128)

PMOD_EXPORT struct program *get_program_for_object_being_destructed(struct object * o)
{
  struct destroy_called_mark * tmp;
  if(( tmp = find_destroy_called_mark(o)))
    return tmp->p;
  return 0;
}

static void call_destroy(struct object *o, enum object_destruct_reason reason)
{
  volatile int e;

  debug_malloc_touch(o);
  if(!o || !o->prog) {
#ifdef GC_VERBOSE
    if (Pike_in_gc > GC_PASS_PREPARE)
      fprintf(stderr, "|   Not calling destroy() in "
	      "destructed %p with %d refs.\n", o, o->refs);
#endif
    return; /* Object already destructed */
  }

  e=FIND_LFUN(o->prog,LFUN_DESTROY);
  if(e != -1
#ifdef DO_PIKE_CLEANUP
     && Pike_interpreter.evaluator_stack
#endif
    && (o->prog->flags & PROGRAM_FINISHED))
  {
#ifdef PIKE_DEBUG
    if(Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
      Pike_fatal("Calling destroy() inside gc.\n");
#endif
    if(check_destroy_called_mark_semaphore(o))
    {
      JMP_BUF jmp;

#ifdef GC_VERBOSE
      if (Pike_in_gc > GC_PASS_PREPARE)
	fprintf(stderr, "|   Calling destroy() in %p with %d refs.\n",
		o, o->refs);
#endif

      free_svalue (&throw_value);
      mark_free_svalue (&throw_value);

      if (SETJMP (jmp)) {
	UNSETJMP (jmp);
	if (gc_destruct_everything) {
	  struct svalue err;
	  move_svalue (&err, &throw_value);
	  mark_free_svalue (&throw_value);
	  if (!SETJMP (jmp)) {
	    push_svalue (&err);
	    push_int (0);
	    push_text ("Got error during final cleanup:\n");
	    push_svalue (&err);
	    push_int (0);
	    o_index();
	    f_add (2);
	    f_index_assign (3);
	    pop_stack();
	  }
	  UNSETJMP (jmp);
	  move_svalue (&throw_value, &err);
	}
	call_handle_error();
      }

      else {
	push_int (reason);
	apply_low(o, e, 1);
	pop_stack();
	UNSETJMP (jmp);
      }

#ifdef GC_VERBOSE
      if (Pike_in_gc > GC_PASS_PREPARE)
	fprintf(stderr, "|   Called destroy() in %p with %d refs.\n",
		o, o->refs);
#endif
    }
  }
#ifdef GC_VERBOSE
  else
    if (Pike_in_gc > GC_PASS_PREPARE)
      fprintf(stderr, "|   No destroy() to call in %p with %d refs.\n",
	      o, o->refs);
#endif
}

static int object_has_destroy( struct object *o )
{
    return QUICK_FIND_LFUN( o->prog, LFUN_DESTROY ) != -1;
}

PMOD_EXPORT void destruct_object (struct object *o, enum object_destruct_reason reason)
{
  int e;
  struct program *p;
  struct pike_frame *pike_frame=0;
  int frame_pushed = 0, destroy_called = 0;

#ifdef PIKE_DEBUG
  ONERROR uwp;

  fatal_check_c_stack(8192);

  SET_ONERROR(uwp, fatal_on_error,
	      "Shouldn't get an exception in destruct().\n");
  if(d_flag > 20) do_debug();

  if (Pike_in_gc >= GC_PASS_PRETOUCH && Pike_in_gc < GC_PASS_FREE)
    gc_fatal (o, 1, "Destructing objects is not allowed inside the gc.\n");
#endif

#ifdef GC_VERBOSE
  if (Pike_in_gc > GC_PASS_PREPARE) {
    fprintf(stderr, "|   Destructing %p with %d refs", o, o->refs);
    if (o->prog) {
      INT32 line;
      struct pike_string *file = get_program_line (o->prog, &line);
      fprintf(stderr, ", prog %s:%d\n", file->str, line);
      free_string(file);
    }
    else fputs(", is destructed\n", stderr);
  }
#endif
  if( !(p = o->prog) ) {
#ifdef PIKE_DEBUG
      UNSET_ONERROR(uwp);
#endif
      return;
  }
  add_ref( o );
  if( object_has_destroy( o ) )
  {
      call_destroy(o, reason);
      destroy_called = 1;
      /* destructed in destroy() */
      if(!(p=o->prog))
      {
          free_object(o);
#ifdef PIKE_DEBUG
          UNSET_ONERROR(uwp);
#endif
          return;
      }
      get_destroy_called_mark(o)->p=p;
  }
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);
  debug_malloc_touch(p);
  o->prog=0;

#ifdef GC_VERBOSE
  if (Pike_in_gc > GC_PASS_PREPARE)
    fprintf(stderr, "|   Zapping references in %p with %d refs.\n", o, o->refs);
#endif

  /* free globals and call C de-initializers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    int q;
    struct program *prog = p->inherits[e].prog;
    char *storage = o->storage+p->inherits[e].storage_offset;

    if(prog->event_handler)
    {
      if( !frame_pushed )
      {
	PUSH_FRAME2(o, p);
	frame_pushed = 1;
      }
      SET_FRAME_CONTEXT(p->inherits + e);
      prog->event_handler(PROG_EVENT_EXIT);
    }

    for(q=0;q<(int)prog->num_variable_index;q++)
    {
      int d=prog->variable_index[q];
      struct identifier *id = prog->identifiers + d;
      int identifier_flags = id->identifier_flags;
      int rtt = id->run_time_type;

      if (IDENTIFIER_IS_ALIAS(identifier_flags))
	continue;

      if(rtt == T_MIXED)
      {
	struct svalue *s;
	s=(struct svalue *)(storage + id->func.offset);
	dmalloc_touch_svalue(s);
	if ((TYPEOF(*s) != T_OBJECT && TYPEOF(*s) != T_FUNCTION) ||
	    s->u.object != o ||
	    !(identifier_flags & IDENTIFIER_NO_THIS_REF)) {
	  (void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" free_global"));
	  free_svalue(s);
#ifdef DEBUG_MALLOC
	} else {
	  (void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" skip_global"));
	  dmalloc_touch_svalue(s);
#endif /* DEBUG_MALLOC */
	}
      } else if ((rtt != PIKE_T_GET_SET) && (rtt != PIKE_T_FREE)) {
	union anything *u;
	u=(union anything *)(storage + id->func.offset);
#ifdef DEBUG_MALLOC
	if (REFCOUNTED_TYPE(rtt)) {debug_malloc_touch(u->refs);}
#endif
	if (rtt != T_OBJECT || u->object != o ||
	    !(identifier_flags & IDENTIFIER_NO_THIS_REF)) {
	  (void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" free_global"));
	  free_short_svalue(u, rtt);
#ifdef DEBUG_MALLOC
	} else {
	  (void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" skip_global"));
#endif /* DEBUG_MALLOC */
	}
	DO_IF_DMALLOC(u->refs=(void *)-1);
      }
    }
  }

  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if(p->flags & PROGRAM_USES_PARENT)
  {
    if(LOW_PARENT_INFO(o,p)->parent)
    {
      debug_malloc_touch(o);
      /* fprintf(stderr, "destruct(): Zapping parent.\n"); */
      free_object(LOW_PARENT_INFO(o,p)->parent);
      LOW_PARENT_INFO(o,p)->parent=0;
    }
  }

  if( frame_pushed )
    POP_FRAME2();

  free_object( o );
  free_program(p);
  if( destroy_called )
      remove_destroy_called_mark(o);

#ifdef PIKE_DEBUG
  UNSET_ONERROR(uwp);
#endif
}


struct object *objects_to_destruct = NULL;
static struct callback *destruct_object_evaluator_callback = NULL;

/* This function destructs the objects that are scheduled to be
 * destructed by schedule_really_free_object. It links the object back into the
 * list of objects first. Adds a reference, destructs it and then frees it.
 */
void low_destruct_objects_to_destruct(void)
{
  struct object *o, *next;

#ifdef PIKE_DEBUG
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
    Pike_fatal("Can't meddle with the object link list in gc pass %d.\n", Pike_in_gc);
#endif

  /* We unlink the list from objects_to_destruct before processing it,
   * to avoid that reentrant calls to this function go through all
   * objects instead of just the newly added ones. This way we avoid
   * extensive recursion in this function and also avoid destructing
   * the objects arbitrarily late.
   *
   * Note that we might start a gc during this, but since the unlinked
   * objects have no refs, the gc never encounters them. */
  while (objects_to_destruct) {
    o = objects_to_destruct, objects_to_destruct = 0;

    /* When we remove the object list from objects_to_destruct, it
     * becomes invisible to gc_touch_all_objects (and other list walk
     * functions), so the object count it returns would be wrong. We
     * therefore use this hack to avoid complaining about it in the
     * gc. */
    got_unlinked_things++;

    do {
#ifdef GC_VERBOSE
      if (Pike_in_gc > GC_PASS_PREPARE)
	fprintf(stderr, "|   Destructing %p on objects_to_destruct.\n", o);
#endif

      next = o->next;
      debug_malloc_touch(o);
      debug_malloc_touch(next);

      /* Link object back to list of objects */
      DOUBLELINK(first_object,o);

      /* call destroy, keep one ref */
      add_ref(o);
      destruct_object (o, DESTRUCT_NO_REFS);
      free_object(o);
    } while ((o = next));

    got_unlinked_things--;
  }
}

void destruct_objects_to_destruct_cb(void)
{
  low_destruct_objects_to_destruct();
  if(destruct_object_evaluator_callback)
  {
    remove_callback(destruct_object_evaluator_callback);
    destruct_object_evaluator_callback=0;
  }
}


/* schedule_really_free_object:
 * This function is called when an object runs out of references.
 * It frees the object if it is destructed, otherwise it moves it to
 * a separate list of objects which will be destructed later.
 */

PMOD_EXPORT void schedule_really_free_object(struct object *o)
{
#ifdef PIKE_DEBUG
  if (o->refs) {
#ifdef DEBUG_MALLOC
    if (o->refs > 0) {
      fprintf (stderr, "Object got %d references in schedule_really_free_object():\n", o->refs);
      describe_something(o, T_OBJECT, 0,2,0, NULL);
    }
#endif
    Pike_fatal("Object got %d references in schedule_really_free_object().\n", o->refs);
  }
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE &&
      /* Fake objects are invisible to the gc. */
      o->next != o)
    gc_fatal(o, 0, "Freeing objects is not allowed inside the gc.\n");
#endif

  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if(o->prog && (o->prog->flags & PROGRAM_DESTRUCT_IMMEDIATE))
  {
    add_ref(o);
    destruct_object (o, DESTRUCT_NO_REFS);
    if(sub_ref(o)) return;
  }

  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if (o->next != o) {
    /* Don't unlink fake objects... */
    DOUBLEUNLINK(first_object,o);
  }

  if(o->prog)
  {
    o->next = objects_to_destruct;
#ifdef PIKE_DEBUG
    o->prev = (void *) -3;
#endif
    PIKE_MEM_WO(o->prev);
    objects_to_destruct = o;

#ifdef GC_VERBOSE
    if (Pike_in_gc > GC_PASS_PREPARE)
      fprintf(stderr, "|   Putting %p in objects_to_destruct.\n", o);
#endif

    if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_DESTRUCT)
      /* destruct_objects_to_destruct() called by gc instead. */
      return;
    if(!destruct_object_evaluator_callback)
    {
      destruct_object_evaluator_callback=
	add_to_callback(&evaluator_callbacks,
			(callback_func)destruct_objects_to_destruct_cb,
			0,0);
    }
  } else {
#if 0    /* Did I just cause a leak? -Hubbe */
    if(o->prog && o->prog->flags & PROGRAM_USES_PARENT)
    {
      if(PARENT_INFO(o)->parent)
      {
	/* fprintf(stderr, "schedule_really_free_object(): Zapping parent.\n"); */

	free_object(PARENT_INFO(o)->parent);
	PARENT_INFO(o)->parent=0;
      }
    }
#endif

#ifdef GC_VERBOSE
    if (Pike_in_gc > GC_PASS_PREPARE)
      fprintf(stderr, "|   Freeing storage for %p.\n", o);
#endif

    if (o->next != o)
      /* As far as the gc is concerned, the fake objects doesn't exist. */
      GC_FREE(o);

    EXIT_PIKE_MEMOBJ(o);

    if(o->storage)
    {
      free(o->storage);
      o->storage=0;
    }
    really_free_object(o);
  }
}

static void assign_svalue_from_ptr_no_free(struct svalue *to,
					   struct object *o,
					   int run_time_type,
					   union idptr func)
{
  void *ptr = PIKE_OBJ_STORAGE(o) + func.offset;
  switch(run_time_type)
  {
  case T_SVALUE_PTR:
    {
      struct svalue *s = func.sval;
      check_destructed(s);
      assign_svalue_no_free(to, s);
      break;
    }

  case T_OBJ_INDEX:
    {
      SET_SVAL(*to, T_FUNCTION, DO_NOT_WARN(func.offset), object, o);
      add_ref(o);
      break;
    }

  case PIKE_T_GET_SET:
    {
      int fun = func.gs_info.getter;
      if (fun >= 0) {
	DECLARE_CYCLIC();
	if (!BEGIN_CYCLIC(o, (size_t) fun)) {
	  SET_CYCLIC_RET(1);
	  apply_low(o, fun, 0);
	} else {
	  END_CYCLIC();
	  Pike_error("Cyclic loop on getter.\n");
	}
	END_CYCLIC();
	*to = *(--Pike_sp);
      } else {
	Pike_error("No getter for variable.\n");
      }
      break;
    }

  case T_MIXED:
  case PIKE_T_NO_REF_MIXED:
    {
      struct svalue *s=(struct svalue *)ptr;
      check_destructed(s);
      assign_svalue_no_free(to, s);
      break;
    }

  case T_FLOAT:
  case PIKE_T_NO_REF_FLOAT:
    SET_SVAL(*to, T_FLOAT, 0, float_number, *(FLOAT_TYPE *)ptr);
    break;

  case T_INT:
  case PIKE_T_NO_REF_INT:
    SET_SVAL(*to, T_INT, NUMBER_NUMBER, integer, *(INT_TYPE *)ptr);
    break;

  default:
    {
      struct ref_dummy *dummy;

      if((dummy=*(struct ref_dummy  **)ptr))
      {
	SET_SVAL(*to, run_time_type & ~PIKE_T_NO_REF_FLAG, 0,
		 dummy, dummy);
	add_ref(dummy);
      }else{
	SET_SVAL(*to, T_INT, NUMBER_NUMBER, integer, 0);
      }
      break;
    }
  }
}


/* Get a variable through internal indexing, i.e. directly by
 * identifier index without going through `->= or `[]= lfuns.
 *
 * NOTE: This function may be called by the compiler on objects
 *       cloned from unfinished programs (ie placeholder
 *       objects). Degenerated cases may thus occur.
 *       It may also be called via lfuns in the currently
 *       compiling program (notably lfun::`->()) and thus
 *       execute in place-holder objacts.
 */
PMOD_EXPORT void low_object_index_no_free(struct svalue *to,
					  struct object *o,
					  ptrdiff_t f)
{
  struct identifier *i;
  struct program *p = NULL;
  struct reference *ref;

  while(1) {
    struct external_variable_context loc;

    if(!o || !(p = o->prog)) {
      Pike_error("Cannot access variables in destructed object.\n");
    }
    debug_malloc_touch(o);
    debug_malloc_touch(o->storage);

    if ((ref = PTR_FROM_INT(p, f))->run_time_type != PIKE_T_UNKNOWN) {
      /* Cached vtable lookup.
       *
       * The most common cases of object indexing should match these.
       */
      assign_svalue_from_ptr_no_free(to, o, ref->run_time_type, ref->func);
      return;
    }

    i=ID_FROM_PTR(p, ref);

    if (!IDENTIFIER_IS_ALIAS(i->identifier_flags)) break;

    loc.o = o;
    loc.inherit = INHERIT_FROM_INT(p, f);
    loc.parent_identifier = f;
    find_external_context(&loc, i->func.ext_ref.depth);
    f = i->func.ext_ref.id + loc.inherit->identifier_level;
    o = loc.o;
  }

  switch(i->identifier_flags & IDENTIFIER_TYPE_MASK)
  {
  case IDENTIFIER_PIKE_FUNCTION:
    if (i->func.offset == -1 && p->flags & PROGRAM_FINISHED) {
      /* Prototype. In the first pass we must be able to get a
       * function anyway.
       *
       * We also need to get a function anyway if we're currently
       * in the second pass of compiling this program, since the
       * function might be defined further ahead.
       *
       * FIXME: Consider looking at IDENTIFIER_HAS_BODY in pass 2?
       *
       * On the other hand, consider mixins.
       */
      SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
      break;
    }
    /* Fall through. */

  case IDENTIFIER_C_FUNCTION:
    ref->func.offset = f;
    ref->run_time_type = T_OBJ_INDEX;
    SET_SVAL(*to, T_FUNCTION, DO_NOT_WARN(f), object, o);
    add_ref(o);
    break;

  case IDENTIFIER_CONSTANT:
    {
      if (i->func.const_info.offset >= 0) {
	struct svalue *s;
	s=& PROG_FROM_INT(p,f)->constants[i->func.const_info.offset].sval;
	if(TYPEOF(*s) == T_PROGRAM &&
	   (s->u.program->flags & PROGRAM_USES_PARENT))
	{
	  SET_SVAL(*to, T_FUNCTION, DO_NOT_WARN(f), object, o);
	  add_ref(o);
	}else{
	  if (p->flags & PROGRAM_OPTIMIZED) {
	    ref->func.sval = s;
	    ref->run_time_type = T_SVALUE_PTR;
	  }
	  check_destructed(s);
	  assign_svalue_no_free(to, s);
	}
      } else {
	/* Prototype constant. */
	SET_SVAL(*to, T_INT, NUMBER_NUMBER, integer, 0);
      }
      break;
    }

  case IDENTIFIER_VARIABLE:
    if (i->run_time_type == PIKE_T_GET_SET) {
      int fun = i->func.gs_info.getter;
      if (fun >= 0) {
	fun += p->inherits[ref->inherit_offset].identifier_level;
      }
      ref->func.gs_info.getter = fun;
      fun = i->func.gs_info.setter;
      if (fun >= 0) {
	fun += p->inherits[ref->inherit_offset].identifier_level;
      }
      ref->func.gs_info.setter = fun;
      ref->run_time_type = PIKE_T_GET_SET;
      assign_svalue_from_ptr_no_free(to, o, ref->run_time_type, ref->func);
    } else if (!(p->flags & PROGRAM_FINISHED) ||
	       (i->run_time_type == PIKE_T_FREE) ||
	       !PIKE_OBJ_STORAGE(o)) {
      /* Variable storage not allocated. */
#ifdef PIKE_DEBUG
      if ((i->run_time_type != PIKE_T_FREE) && (p->flags & PROGRAM_FINISHED)) {
	Pike_fatal("Object without variable storage!\n");
      }
#endif /* PIKE_DEBUG */
      SET_SVAL(*to, T_INT, ((i->run_time_type == PIKE_T_FREE)?
			    NUMBER_UNDEFINED:NUMBER_NUMBER),
	       integer, 0);
    } else {
      ref->func.offset = INHERIT_FROM_INT(o->prog, f)->storage_offset +
	i->func.offset;
      ref->run_time_type = i->run_time_type;
      if (i->identifier_flags & IDENTIFIER_NO_THIS_REF) {
	ref->run_time_type |= PIKE_T_NO_REF_FLAG;
      }
      assign_svalue_from_ptr_no_free(to, o, ref->run_time_type, ref->func);
    }
    break;

  default:;
#ifdef PIKE_DEBUG
    Pike_fatal ("Unknown identifier type.\n");
#endif
  }
}

/* Get a variable without going through `->= or `[]= lfuns. If index
 * is a string then the externally visible identifiers are indexed. If
 * index is T_OBJ_INDEX then any identifier is accessed through
 * identifier index. */
PMOD_EXPORT void object_index_no_free2(struct svalue *to,
				       struct object *o,
				       int inherit_number,
				       struct svalue *index)
{
  struct program *p;
  struct inherit *inh;
  int f = -1;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  p = (inh = p->inherits + inherit_number)->prog;

  switch(TYPEOF(*index))
  {
  case T_STRING:
    f=find_shared_string_identifier(index->u.string, p);
    if (f >= 0) f += inh->identifier_level;
    break;

  case T_OBJ_INDEX:
    f=index->u.identifier;
    break;

  default:
#ifdef PIKE_DEBUG
    if (TYPEOF(*index) > MAX_TYPE)
      Pike_fatal ("Invalid type %d in index.\n", TYPEOF(*index));
#endif
    Pike_error("Lookup in object with non-string index.\n");
  }

  if(f < 0)
  {
    SET_SVAL(*to, T_INT, NUMBER_UNDEFINED, integer, 0);
  }else{
    low_object_index_no_free(to, o, f);
  }
}

#define ARROW_INDEX_P(X) (TYPEOF(*(X)) == T_STRING && SUBTYPEOF(*(X)))

/* Get a variable through external indexing, i.e. by going through
 * `-> or `[] lfuns, not seeing private and protected etc. */
PMOD_EXPORT void object_index_no_free(struct svalue *to,
				      struct object *o,
				      int inherit_number,
				      struct svalue *index)
{
  struct program *p = NULL;
  struct inherit *inh;
  int lfun,l;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  p = (inh = p->inherits + inherit_number)->prog;

#ifdef PIKE_DEBUG
  if (TYPEOF(*index) > MAX_TYPE)
    Pike_fatal ("Invalid index type %d.\n", TYPEOF(*index));
#endif

  lfun=ARROW_INDEX_P(index) ? LFUN_ARROW : LFUN_INDEX;

  if(p->flags & PROGRAM_FIXED)
  {
    l=p->lfuns[lfun];
  }else{
    if(!(p->flags & PROGRAM_PASS_1_DONE))
    {
      if(report_compiler_dependency(p))
      {
#if 0
	struct svalue tmp;
	fprintf(stderr,"Placeholder deployed for %p when indexing ", p);
	SET_SVAL(tmp, T_OBJECT, 0, object, o);
	print_svalue (stderr, &tmp);
	fputs (" with ", stderr);
	print_svalue (stderr, index);
	fputc ('\n', stderr);
#endif
	SET_SVAL(*to, T_OBJECT, 0, object, placeholder_object);
	add_ref(placeholder_object);
	return;
      }
    }
    l=low_find_lfun(p, lfun);
  }
  if(l != -1)
  {
    l += inh->identifier_level;
    push_svalue(index);
    apply_lfun(o, lfun, 1);
    *to=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
  } else {
    object_index_no_free2(to, o, inherit_number, index);
  }
}

static void object_lower_set_index(struct object *o, union idptr func, int rtt,
				   struct svalue *from)
{
  int is_zero = UNSAFE_IS_ZERO(from);
  do {
    void *ptr = PIKE_OBJ_STORAGE(o) + func.offset;
    union anything *u = (union anything *)ptr;
    struct svalue *to = (struct svalue *)ptr;
    switch(rtt) {
    case T_SVALUE_PTR: case T_OBJ_INDEX:
      Pike_error("Cannot assign functions or constants.\n");
      continue;
    case PIKE_T_FREE:
      Pike_error("Attempt to store data in extern variable.\n");
      continue;

    case PIKE_T_GET_SET:
      {
	int fun = func.gs_info.setter;
	if (fun >= 0) {
	  DECLARE_CYCLIC();
	  if (!BEGIN_CYCLIC(o, (size_t) fun)) {
	    SET_CYCLIC_RET(1);
	    push_svalue(from);
	    apply_low(o, fun, 1);
	    pop_stack();
	  } else {
	    END_CYCLIC();
	    Pike_error("Cyclic loop on setter.\n");
	  }
	  END_CYCLIC();
	} else {
	  Pike_error("No setter for variable.\n");
	}
	continue;
      }

    /* Partial code duplication from assign_to_short_svalue. */
    case T_MIXED:
      /* Count references to ourselves. */
      assign_svalue(to, from);
      continue;
    case PIKE_T_NO_REF_MIXED:
      /* Don't count references to ourselves to help the gc. DDTAH. */
      dmalloc_touch_svalue(to);
      if ((TYPEOF(*to) != T_OBJECT && TYPEOF(*to) != T_FUNCTION) ||
	  to->u.object != o) {
	(void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" free_global"));
	free_svalue(to);
#ifdef DEBUG_MALLOC
      } else {
	(void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" skip_global"));
	dmalloc_touch_svalue (to);
#endif /* DEBUG_MALLOC */
      }
      *to = *from;
      dmalloc_touch_svalue (to);
      if ((TYPEOF(*to) != T_OBJECT && TYPEOF(*to) != T_FUNCTION) ||
	  (to->u.object != o)) {
	if(REFCOUNTED_TYPE(TYPEOF(*to))) {
	  (void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" store_global"));
	  add_ref(to->u.dummy);
#ifdef DEBUG_MALLOC
	} else {
	  (void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" store_global"));
#endif /* DEBUG_MALLOC */
	}
#ifdef DEBUG_MALLOC
      } else {
	(void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" self_global"));
	dmalloc_touch_svalue (to);
#endif /* DEBUG_MALLOC */
      }
      continue;

    case T_INT:
    case PIKE_T_NO_REF_INT:
      if (TYPEOF(*from) != T_INT) break;
      u->integer=from->u.integer;
      continue;
    case T_FLOAT:
    case PIKE_T_NO_REF_FLOAT:
      if (TYPEOF(*from) != T_FLOAT) break;
      u->float_number=from->u.float_number;
      continue;

    case PIKE_T_NO_REF_OBJECT:
      /* Don't count references to ourselves to help the gc. */
      if ((TYPEOF(*from) != T_OBJECT) && !is_zero) break;
      debug_malloc_touch(u->object);
      if ((u->object != o) && u->refs && !sub_ref(u->dummy)) {
	debug_malloc_touch(o);
	really_free_short_svalue(u,rtt);
#ifdef DEBUG_MALLOC
      } else {
	debug_malloc_touch(o);
#endif /* DEBUG_MALLOC */
      }
      if (is_zero) {
	debug_malloc_touch(u->ptr);
	u->refs = NULL;
	continue;
      }
      u->refs = from->u.refs;
      debug_malloc_touch(u->refs);
      if (u->object != o) {
	debug_malloc_touch(o);
	add_ref(u->dummy);
#ifdef DEBUG_MALLOC
      } else {
	debug_malloc_touch(o);
#endif /* DEBUG_MALLOC */
      }
      continue;

    default:
      rtt &= ~PIKE_T_NO_REF_FLAG;
      if ((rtt != TYPEOF(*from)) && !is_zero) break;	/* Error. */
      debug_malloc_touch(u->refs);
      if(u->refs && !sub_ref(u->dummy))
	really_free_short_svalue(u, rtt);
      if (is_zero) {
	debug_malloc_touch(u->ptr);
	u->refs = NULL;
	continue;
      }
      u->refs = from->u.refs;
      add_ref(u->dummy);
      continue;
    }

    Pike_error("Wrong type in assignment, expected %s, got %s.\n",
	       get_name_of_type(rtt & ~PIKE_T_NO_REF_FLAG),
	       get_name_of_type(TYPEOF(*from)));
  } while(0);
}

/* Assign a variable through internal indexing, i.e. directly by
 * identifier index without going through `->= or `[]= lfuns. */
PMOD_EXPORT void object_low_set_index(struct object *o,
				      int f,
				      struct svalue *from)
{
  struct identifier *i;
  struct program *p = NULL;
  struct reference *ref;
  int rtt, id_flags;

  while(1) {
    struct external_variable_context loc;

    if(!o || !(p=o->prog))
    {
      Pike_error("Lookup in destructed object.\n");
      return; /* make gcc happy */
    }

    debug_malloc_touch(o);
    debug_malloc_touch(o->storage);

    if ((ref = PTR_FROM_INT(p, f))->run_time_type != PIKE_T_UNKNOWN) {
      /* Cached vtable lookup.
       *
       * The most common cases of object indexing should match these.
       */
      object_lower_set_index(o, ref->func, ref->run_time_type, from);
      return;
    }

    i=ID_FROM_INT(p, f);

    if (!IDENTIFIER_IS_ALIAS(i->identifier_flags)) break;

    loc.o = o;
    loc.inherit = INHERIT_FROM_INT(p, f);
    loc.parent_identifier = f;
    find_external_context(&loc, i->func.ext_ref.depth);
    f = i->func.ext_ref.id + loc.inherit->identifier_level;
    o = loc.o;
  }

  check_destructed(from);
  rtt = i->run_time_type;
  id_flags = i->identifier_flags;

  if(!IDENTIFIER_IS_VARIABLE(id_flags))
  {
    Pike_error("Cannot assign functions or constants.\n");
  }
  else if(rtt == T_MIXED)
  {
    ref->func.offset = INHERIT_FROM_INT(p, f)->storage_offset +
      i->func.offset;
    ref->run_time_type = i->run_time_type;
    if (id_flags & IDENTIFIER_NO_THIS_REF) {
      ref->run_time_type |= PIKE_T_NO_REF_FLAG;
    }
    object_lower_set_index(o, ref->func, ref->run_time_type, from);
  }
  else if (rtt == PIKE_T_GET_SET)
  {
    /* Getter/setter type variable. */
    struct reference *ref = p->identifier_references + f;
    struct program *pp = p->inherits[ref->inherit_offset].prog;
    int fun = i->func.gs_info.getter;
    if (fun >= 0) {
      fun += p->inherits[ref->inherit_offset].identifier_level;
    }
    ref->func.gs_info.getter = fun;
    fun = i->func.gs_info.setter;
    if (fun >= 0) {
      fun += p->inherits[ref->inherit_offset].identifier_level;
    }
    ref->func.gs_info.setter = fun;
    ref->run_time_type = PIKE_T_GET_SET;
    object_lower_set_index(o, ref->func, ref->run_time_type, from);
  }
  else if (rtt == PIKE_T_FREE) {
    Pike_error("Attempt to store data in extern variable %S.\n", i->name);
  } else {
    ref->func.offset = INHERIT_FROM_INT(p, f)->storage_offset +
      i->func.offset;
    ref->run_time_type = i->run_time_type;
    if (id_flags & IDENTIFIER_NO_THIS_REF) {
      ref->run_time_type |= PIKE_T_NO_REF_FLAG;
    }
    object_lower_set_index(o, ref->func, ref->run_time_type, from);
  }
}

/* Assign a variable without going through `->= or `[]= lfuns. If
 * index is a string then the externally visible identifiers are
 * indexed. If index is T_OBJ_INDEX then any identifier is accessed
 * through identifier index. */
PMOD_EXPORT void object_set_index2(struct object *o,
				   int inherit_number,
				   struct svalue *index,
				   struct svalue *from)
{
  struct program *p;
  struct inherit *inh;
  int f = -1;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  p = (inh = p->inherits + inherit_number)->prog;

  switch(TYPEOF(*index))
  {
  case T_STRING:
    f=find_shared_string_identifier(index->u.string, p);
    if (f >= 0) f += inh->identifier_level;
    break;

  case T_OBJ_INDEX:
    f=index->u.identifier;
    break;

  default:
#ifdef PIKE_DEBUG
    if (TYPEOF(*index) > MAX_TYPE)
      Pike_fatal ("Invalid type %d in index.\n", TYPEOF(*index));
#endif
    Pike_error("Lookup on non-string value.\n");
  }

  if(f < 0)
  {
    if (TYPEOF(*index) == T_STRING && index->u.string->len < 1024)
      Pike_error("No such variable (%S) in object.\n", index->u.string);
    else
      Pike_error("No such variable in object.\n");
  }else{
    object_low_set_index(o, f, from);
  }
}

/* Assign a variable through external indexing, i.e. by going through
 * `->= or `[]= lfuns, not seeing private and protected etc. */
PMOD_EXPORT void object_set_index(struct object *o,
				  int inherit_number,
				  struct svalue *index,
				  struct svalue *from)
{
  int lfun;
  int fun;
  struct program *p = NULL;
  struct inherit *inh;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  p = (inh = p->inherits + inherit_number)->prog;

#ifdef PIKE_DEBUG
  if (TYPEOF(*index) > MAX_TYPE)
    Pike_fatal ("Invalid index type %d.\n", TYPEOF(*index));
#endif

  lfun=ARROW_INDEX_P(index) ? LFUN_ASSIGN_ARROW : LFUN_ASSIGN_INDEX;

  if((fun = FIND_LFUN(p, lfun)) != -1)
  {
    push_svalue(index);
    push_svalue(from);
    apply_low(o, fun + inh->identifier_level, 2);
    pop_stack();
  } else {
    object_set_index2(o, inherit_number, index, from);
  }
}

static union anything *object_low_get_item_ptr(struct object *o,
					       int f,
					       TYPE_T type)
{
  struct identifier *i;
  struct program *p;

  debug_malloc_touch(o);

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return 0; /* make gcc happy */
  }

  debug_malloc_touch(p);

  i=ID_FROM_INT(p, f);

#ifdef PIKE_DEBUG
  if (type > MAX_TYPE)
    Pike_fatal ("Invalid type %d.\n", type);
  if (type == T_OBJECT || type == T_FUNCTION)
    Pike_fatal ("Dangerous with the refcount-less this-pointers.\n");
#endif

  while (IDENTIFIER_IS_ALIAS(i->identifier_flags)) {
    struct external_variable_context loc;
    loc.o = o;
    loc.inherit = INHERIT_FROM_INT(p, f);
    loc.parent_identifier = 0;
    find_external_context(&loc, i->func.ext_ref.depth);
    f = i->func.ext_ref.id;
    p = (o = loc.o)->prog;
    i = ID_FROM_INT(p, f);
  }

  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
  {
    Pike_error("Cannot assign functions or constants.\n");
  } else if(i->run_time_type == T_MIXED)
  {
    struct svalue *s;
    s=(struct svalue *)LOW_GET_GLOBAL(o,f,i);
    if(TYPEOF(*s) == type) return & s->u;
  }
  else if(i->run_time_type == type)
  {
    return (union anything *) LOW_GET_GLOBAL(o,f,i);
  }
  return 0;
}


union anything *object_get_item_ptr(struct object *o,
				    int inherit_number,
				    struct svalue *index,
				    TYPE_T type)
{
  struct program *p;
  struct inherit *inh;
  int f;

  debug_malloc_touch(o);
  dmalloc_touch_svalue(index);

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return 0; /* make gcc happy */
  }

  p = (inh = p->inherits + inherit_number)->prog;

  switch(TYPEOF(*index))
  {
  case T_STRING:
    f=ARROW_INDEX_P(index) ? LFUN_ASSIGN_ARROW : LFUN_ASSIGN_INDEX;

    if(FIND_LFUN(p,f) != -1)
    {
      return 0;
      
      /* Pike_error("Cannot do incremental operations on overloaded index (yet).\n");
       */
    }
    
    f=find_shared_string_identifier(index->u.string, p);
    if (f >= 0) f += inh->identifier_level;
    break;

  case T_OBJ_INDEX:
    f=index->u.identifier;
    break;

  default:
#ifdef PIKE_DEBUG
    if (TYPEOF(*index) > MAX_TYPE)
      Pike_fatal ("Invalid type %d in index.\n", TYPEOF(*index));
#endif
/*    Pike_error("Lookup on non-string value.\n"); */
    return 0;
  }

  if(f < 0)
  {
    if (TYPEOF(*index) == T_STRING && index->u.string->len < 1024)
      Pike_error("No such variable (%S) in object.\n", index->u.string);
    else
      Pike_error("No such variable in object.\n");
  }else{
    return object_low_get_item_ptr(o, f, type);
  }
  return 0;
}


PMOD_EXPORT int object_equal_p(struct object *a, struct object *b, struct processing *p)
{
  struct processing curr;

  if(a == b) return 1;
  if(a->prog != b->prog) return 0;

  curr.pointer_a = a;
  curr.pointer_b = b;
  curr.next = p;

  for( ;p ;p=p->next)
    if(p->pointer_a == (void *)a && p->pointer_b == (void *)b)
      return 1;

  /* NOTE: At this point a->prog and b->prog are equal (see test 2 above). */
  if(a->prog)
  {
    int e;

    if(a->prog->flags & PROGRAM_HAS_C_METHODS) return 0;

    for(e=0;e<(int)a->prog->num_identifier_references;e++)
    {
      struct identifier *i;
      i=ID_FROM_INT(a->prog, e);

      if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags) ||
	 IDENTIFIER_IS_ALIAS(i->identifier_flags))
	continue;

      if(i->run_time_type == T_MIXED)
      {
	if(!low_is_equal((struct svalue *)LOW_GET_GLOBAL(a,e,i),
			 (struct svalue *)LOW_GET_GLOBAL(b,e,i),
			 &curr))
	  return 0;
      }else{
	if(!low_short_is_equal((union anything *)LOW_GET_GLOBAL(a,e,i),
			       (union anything *)LOW_GET_GLOBAL(b,e,i),
			       i->run_time_type,
			       &curr))
	  return 0;
      }
    }
  }

  return 1;
}

PMOD_EXPORT struct array *object_indices(struct object *o, int inherit_number)
{
  struct program *p;
  struct inherit *inh;
  struct array *a;
  int fun;
  int e;

  p=o->prog;
  if(!p)
    Pike_error("indices() on destructed object.\n");

  p = (inh = p->inherits + inherit_number)->prog;

  if((fun = FIND_LFUN(p, LFUN__INDICES)) == -1)
  {
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      SET_SVAL(ITEM(a)[e], T_STRING, 0, string,
	       ID_FROM_INT(p,p->identifier_index[e])->name);
      add_ref(ITEM(a)[e].u.string);
    }
    a->type_field = BIT_STRING;
  }else{
    apply_low(o, fun + inh->identifier_level, 0);
    if(TYPEOF(sp[-1]) != T_ARRAY)
      Pike_error("Bad return type from o->_indices()\n");
    a=sp[-1].u.array;
    sp--;
    dmalloc_touch_svalue(sp);
  }
  return a;
}

PMOD_EXPORT struct array *object_values(struct object *o, int inherit_number)
{
  struct program *p;
  struct inherit *inh;
  struct array *a;
  int fun;
  int e;
  
  p=o->prog;
  if(!p)
    Pike_error("values() on destructed object.\n");

  p = (inh = p->inherits + inherit_number)->prog;

  if((fun = FIND_LFUN(p, LFUN__VALUES)) == -1)
  {
    TYPE_FIELD types = 0;
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      low_object_index_no_free(ITEM(a)+e, o,
			       p->identifier_index[e] + inh->identifier_level);
      types |= 1 << TYPEOF(ITEM(a)[e]);
    }
    a->type_field = types;
  }else{
    apply_low(o, fun + inh->identifier_level, 0);
    if(TYPEOF(sp[-1]) != T_ARRAY)
      Pike_error("Bad return type from o->_values()\n");
    a=sp[-1].u.array;
    sp--;
    dmalloc_touch_svalue(sp);
  }
  return a;
}

PMOD_EXPORT struct array *object_types(struct object *o, int inherit_number)
{
  struct program *p;
  struct inherit *inh;
  struct array *a;
  int fun;
  int e;

  p=o->prog;
  if(!p)
    Pike_error("types() on destructed object.\n");

  p = (inh = p->inherits + inherit_number)->prog;

  if((fun = low_find_lfun(p, LFUN__TYPES)) == -1)
  {
    if ((fun = FIND_LFUN(p, LFUN__VALUES)) != -1) {
      /* Some compat for the case where lfun::_values() is overloaded,
       * but not lfun::_types(). */
      a = object_values(o, inherit_number);
      for(e=0;e<a->size;e++) {
	struct pike_type *t = get_type_of_svalue(ITEM(a) + e);
	free_svalue(ITEM(a) + e);
	SET_SVAL(ITEM(a)[e], PIKE_T_TYPE, 0, type, t);
      }
      a->type_field = BIT_TYPE;
      return a;
    }
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      struct identifier *id = ID_FROM_INT(p,p->identifier_index[e]);
      SET_SVAL(ITEM(a)[e], PIKE_T_TYPE, 0, type, id->type);
    }
    a->type_field = BIT_TYPE;
  }else{
    apply_low(o, fun + inh->identifier_level, 0);
    if(TYPEOF(sp[-1]) != T_ARRAY)
      Pike_error("Bad return type from o->_types()\n");
    a=sp[-1].u.array;
    sp--;
    dmalloc_touch_svalue(sp);
  }
  return a;
}


PMOD_EXPORT void visit_object (struct object *o, int action)
{
  struct program *p = o->prog;

  if (o->next == o) return; /* Fake object used by compiler */

  switch (action) {
#ifdef PIKE_DEBUG
    default:
      Pike_fatal ("Unknown visit action %d.\n", action);
    case VISIT_NORMAL:
    case VISIT_COMPLEX_ONLY:
      break;
#endif
    case VISIT_COUNT_BYTES:
      mc_counted_bytes += sizeof (struct object);
      if (p) mc_counted_bytes += p->storage_needed;
      break;
  }

  if (PIKE_OBJ_INITED (o)) {
    struct pike_frame *pike_frame = NULL;
    struct inherit *inh = p->inherits;
    char *storage = o->storage;
    int e;

    debug_malloc_touch (p);
    debug_malloc_touch (o);
    debug_malloc_touch (storage);

    visit_program_ref (p, REF_TYPE_NORMAL);

    for (e = p->num_inherits - 1; e >= 0; e--) {
      struct program *inh_prog = inh[e].prog;
      unsigned INT16 *inh_prog_var_idxs = inh_prog->variable_index;
      struct identifier *inh_prog_ids = inh_prog->identifiers;
      char *inh_storage = storage + inh[e].storage_offset;

      int q, num_vars = (int) inh_prog->num_variable_index;

      for (q = 0; q < num_vars; q++) {
	int d = inh_prog_var_idxs[q];
	struct identifier *id = inh_prog_ids + d;
	int id_flags = id->identifier_flags;
	int rtt = id->run_time_type;
	void *var;
	union anything *u;

	if (IDENTIFIER_IS_ALIAS (id_flags))
	  continue;

	var = inh_storage + id->func.offset;
	u = (union anything *) var;
#ifdef DEBUG_MALLOC
	if (REFCOUNTED_TYPE(rtt))
	  debug_malloc_touch (u->ptr);
#endif

	switch (rtt) {
	  case T_MIXED: {
	    struct svalue *s = (struct svalue *) var;
	    dmalloc_touch_svalue (s);
	    if ((TYPEOF(*s) != T_OBJECT && TYPEOF(*s) != T_FUNCTION) ||
		s->u.object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      visit_svalue (s, REF_TYPE_NORMAL);
	    break;
	  }

	  case T_ARRAY:
	    if (u->array)
	      visit_array_ref (u->array, REF_TYPE_NORMAL);
	    break;
	  case T_MAPPING:
	    if (u->mapping)
	      visit_mapping_ref (u->mapping, REF_TYPE_NORMAL);
	    break;
	  case T_MULTISET:
	    if (u->multiset)
	      visit_multiset_ref (u->multiset, REF_TYPE_NORMAL);
	    break;
	  case T_PROGRAM:
	    if (u->program)
	      visit_program_ref (u->program, REF_TYPE_NORMAL);
	    break;

	  case T_OBJECT:
	    if (u->object && (u->object != o ||
			      !(id_flags & IDENTIFIER_NO_THIS_REF)))
	      visit_object_ref (u->object, REF_TYPE_NORMAL);
	    break;

	  case T_STRING:
	    if (u->string && !(action & VISIT_COMPLEX_ONLY))
	      visit_string_ref (u->string, REF_TYPE_NORMAL);
	    break;
	  case T_TYPE:
	    if (u->type && !(action & VISIT_COMPLEX_ONLY))
	      visit_type_ref (u->type, REF_TYPE_NORMAL);
	    break;

#ifdef PIKE_DEBUG
	  case PIKE_T_GET_SET:
	  case PIKE_T_FREE:
	  case T_INT:
	  case T_FLOAT:
	    break;
	  default:
	    Pike_fatal ("Invalid runtime type %d.\n", rtt);
#endif
	}
      }

      if (inh_prog->event_handler) {
	if (!pike_frame) PUSH_FRAME2 (o, p);
	SET_FRAME_CONTEXT (inh + e);
	inh_prog->event_handler (PROG_EVENT_GC_RECURSE);
      }
    }

    if (pike_frame) POP_FRAME2();

    /* Strong ref follows. It must be last. */
    if (p->flags & PROGRAM_USES_PARENT)
      if (PARENT_INFO (o)->parent)
	visit_object_ref (PARENT_INFO (o)->parent, REF_TYPE_STRONG);
  }
}

PMOD_EXPORT void visit_function (const struct svalue *s, int ref_type)
{
#ifdef PIKE_DEBUG
  if (TYPEOF(*s) != T_FUNCTION)
    Pike_fatal ("Should only be called for a function svalue.\n");
#endif

  if (SUBTYPEOF(*s) == FUNCTION_BUILTIN)
    /* Could avoid this if we had access to the action from the caller
     * and check if it's VISIT_COMPLEX_ONLY. However, visit_callable
     * will only return first thing. */
    visit_callable_ref (s->u.efun, ref_type);
  else
    visit_object_ref (s->u.object, ref_type);
}

static void gc_check_object(struct object *o);

PMOD_EXPORT void gc_mark_object_as_referenced(struct object *o)
{
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if(gc_mark(o, T_OBJECT)) {
    if(o->next == o) return; /* Fake object used by compiler */

    GC_ENTER (o, T_OBJECT) {
      int e;
      struct program *p = o->prog;

      if (o == gc_mark_object_pos)
	gc_mark_object_pos = o->next;
      if (o == gc_internal_object)
	gc_internal_object = o->next;
      else {
	DOUBLEUNLINK(first_object, o);
	DOUBLELINK(first_object, o); /* Linked in first. */
      }

      if(p && PIKE_OBJ_INITED(o)) {
	debug_malloc_touch(p);

	gc_mark_program_as_referenced (p);

	if(o->prog->flags & PROGRAM_USES_PARENT)
	  if(PARENT_INFO(o)->parent)
	    gc_mark_object_as_referenced(PARENT_INFO(o)->parent);

        GC_SET_CALL_FRAME(o, p);

	for(e=p->num_inherits-1; e>=0; e--)
	{
	  int q;
      
	  LOW_SET_FRAME_CONTEXT(p->inherits + e);

          for(q=0;q<(int)Pike_fp->context->prog->num_variable_index;q++)
	  {
            int d = Pike_fp->context->prog->variable_index[q];
            struct identifier *id = Pike_fp->context->prog->identifiers + d;
	    int id_flags = id->identifier_flags;
	    int rtt = id->run_time_type;

	    if (IDENTIFIER_IS_ALIAS(id_flags) || (rtt == PIKE_T_GET_SET) ||
		(rtt == PIKE_T_FREE))
	      continue;
	
	    if(rtt == T_MIXED)
	    {
	      struct svalue *s;
              s=(struct svalue *)(Pike_fp->current_storage + id->func.offset);
	      dmalloc_touch_svalue(s);
	      if ((TYPEOF(*s) != T_OBJECT && TYPEOF(*s) != T_FUNCTION) ||
		  s->u.object != o || !(id_flags & IDENTIFIER_NO_THIS_REF))
		gc_mark_svalues(s, 1);
	    }else{
	      union anything *u;
              u=(union anything *)(Pike_fp->current_storage + id->func.offset);
#ifdef DEBUG_MALLOC
	      if (REFCOUNTED_TYPE(rtt)) debug_malloc_touch(u->refs);
#endif
	      if (rtt != T_OBJECT || u->object != o ||
		  !(id_flags & IDENTIFIER_NO_THIS_REF))
		gc_mark_short_svalue(u, rtt);
	    }
	  }

          if (Pike_fp->context->prog->event_handler)
            Pike_fp->context->prog->event_handler(PROG_EVENT_GC_RECURSE);

	  LOW_UNSET_FRAME_CONTEXT();
	}

        GC_UNSET_CALL_FRAME();
      }
    } GC_LEAVE;
  }
}

PMOD_EXPORT void real_gc_cycle_check_object(struct object *o, int weak)
{
  if(o->next == o) return; /* Fake object used by compiler */

  GC_CYCLE_ENTER_OBJECT(o, weak) {
    int e;
    struct program *p = o->prog;

    if (p && PIKE_OBJ_INITED(o)) {
#if 0
      struct object *o2;
      for (o2 = gc_internal_object; o2 && o2 != o; o2 = o2->next) {}
      if (!o2) Pike_fatal("Object not on gc_internal_object list.\n");
#endif

      GC_SET_CALL_FRAME(o, p);

      for(e=p->num_inherits-1; e>=0; e--)
      {
	int q;
      
	LOW_SET_FRAME_CONTEXT(p->inherits + e);

        for(q=0;q<(int)Pike_fp->context->prog->num_variable_index;q++)
	{
          int d = Pike_fp->context->prog->variable_index[q];
          struct identifier *id = Pike_fp->context->prog->identifiers + d;
	  int id_flags = id->identifier_flags;
	  int rtt = id->run_time_type;
	
	  if (IDENTIFIER_IS_ALIAS(id_flags) || (rtt == PIKE_T_GET_SET) ||
	      (rtt == PIKE_T_FREE))
	    continue;
	
	  if(rtt == T_MIXED)
	  {
	    struct svalue *s;
            s=(struct svalue *)(Pike_fp->current_storage + id->func.offset);
	    dmalloc_touch_svalue(s);
	    if ((TYPEOF(*s) != T_OBJECT && TYPEOF(*s) != T_FUNCTION) ||
		s->u.object != o || !(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_cycle_check_svalues(s, 1);
	  }else{
	    union anything *u;
            u=(union anything *)(Pike_fp->current_storage + id->func.offset);
#ifdef DEBUG_MALLOC
	    if (REFCOUNTED_TYPE(rtt)) debug_malloc_touch(u->refs);
#endif
	    if (rtt != T_OBJECT || u->object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_cycle_check_short_svalue(u, rtt);
	  }
	}

        if (Pike_fp->context->prog->event_handler)
          Pike_fp->context->prog->event_handler(PROG_EVENT_GC_RECURSE);

	LOW_UNSET_FRAME_CONTEXT();
      }
    
      GC_UNSET_CALL_FRAME();

      /* Even though it's essential that the program isn't freed
       * before the object, it doesn't need a strong link. That since
       * programs can't be destructed before they run out of
       * references. */
      gc_cycle_check_program (p, 0);

      /* Strong ref follows. It must be last. */
      if(o->prog->flags & PROGRAM_USES_PARENT)
	if(PARENT_INFO(o)->parent)
	  gc_cycle_check_object(PARENT_INFO(o)->parent, -1);
    }
  } GC_CYCLE_LEAVE;
}

static void gc_check_object(struct object *o)
{
  int e;
  struct program *p;

  GC_ENTER (o, T_OBJECT) {
    if((p=o->prog) && PIKE_OBJ_INITED(o))
    {
      debug_malloc_touch(p);
      debug_gc_check (p, " as the program of an object");

      if(p->flags & PROGRAM_USES_PARENT && PARENT_INFO(o)->parent)
	debug_gc_check (PARENT_INFO(o)->parent, " as parent of an object");

      GC_SET_CALL_FRAME(o, p);
    
      for(e=p->num_inherits-1; e>=0; e--)
      {
	int q;
	LOW_SET_FRAME_CONTEXT(p->inherits + e);

        for(q=0;q<(int)Pike_fp->context->prog->num_variable_index;q++)
	{
          int d = Pike_fp->context->prog->variable_index[q];
          struct identifier *id = Pike_fp->context->prog->identifiers + d;
	  int id_flags = id->identifier_flags;
	  int rtt = id->run_time_type;
	
	  if (IDENTIFIER_IS_ALIAS(id_flags) || (rtt == PIKE_T_GET_SET) ||
	      (rtt == PIKE_T_FREE))
	    continue;
	
	  if(rtt == T_MIXED)
	  {
	    struct svalue *s;
            s=(struct svalue *)(Pike_fp->current_storage + id->func.offset);
	    dmalloc_touch_svalue(s);
	    if ((TYPEOF(*s) != T_OBJECT && TYPEOF(*s) != T_FUNCTION) ||
		s->u.object != o || !(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_check_svalues(s, 1);
	  }else{
	    union anything *u;
            u=(union anything *)(Pike_fp->current_storage + id->func.offset);
#ifdef DEBUG_MALLOC
	    if (REFCOUNTED_TYPE(rtt)) debug_malloc_touch(u->refs);
#endif
	    if (rtt != T_OBJECT || u->object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_check_short_svalue(u, rtt);
	  }
	}
      
        if(Pike_fp->context->prog->event_handler)
          Pike_fp->context->prog->event_handler(PROG_EVENT_GC_CHECK);

	LOW_UNSET_FRAME_CONTEXT();
      }

      GC_UNSET_CALL_FRAME();
    }
  } GC_LEAVE;
}

unsigned gc_touch_all_objects(void)
{
  unsigned n = 0;
  struct object *o;
  if (first_object && first_object->prev)
    Pike_fatal("Error in object link list.\n");
  for (o = first_object; o; o = o->next) {
    debug_gc_touch(o);
    n++;
    if (o->next && o->next->prev != o)
      Pike_fatal("Error in object link list.\n");
  }
  for (o = objects_to_destruct; o; o = o->next) n++;
  return n;
}

void gc_check_all_objects(void)
{
  struct object *o;
  for(o=first_object;o;o=o->next)
    gc_check_object(o);
}

void gc_mark_all_objects(void)
{
  gc_mark_object_pos = gc_internal_object;
  while (gc_mark_object_pos) {
    struct object *o = gc_mark_object_pos;
    gc_mark_object_pos = o->next;
    if(o->refs && gc_is_referenced(o))
      /* Refs check since objects without refs are left around during
       * gc by schedule_really_free_object(). */
      gc_mark_object_as_referenced(o);
  }

#ifdef PIKE_DEBUG
  if(d_flag) {
    struct object *o;
    for(o=objects_to_destruct;o;o=o->next)
      debug_malloc_touch(o);
  }
#endif
}

void gc_cycle_check_all_objects(void)
{
  struct object *o;
  for (o = gc_internal_object; o; o = o->next) {
    real_gc_cycle_check_object(o, 0);
    gc_cycle_run_queue();
  }
}

void gc_zap_ext_weak_refs_in_objects(void)
{
  gc_mark_object_pos = first_object;
  while (gc_mark_object_pos != gc_internal_object && gc_ext_weak_refs) {
    struct object *o = gc_mark_object_pos;
    gc_mark_object_pos = o->next;
    if (o->refs)
      gc_mark_object_as_referenced(o);
  }
  gc_mark_discard_queue();
}

size_t gc_free_all_unreferenced_objects(void)
{
  struct object *o,*next;
  size_t unreferenced = 0;
  enum object_destruct_reason reason =
#ifdef DO_PIKE_CLEANUP
    gc_destruct_everything ? DESTRUCT_CLEANUP :
#endif
    DESTRUCT_GC;

  for(o=gc_internal_object; o; o=next)
  {
    if(gc_do_free(o))
    {
      /* Got an extra ref from gc_cycle_pop_object(). */
#ifdef PIKE_DEBUG
      if (gc_object_is_live (o) &&
	  !find_destroy_called_mark(o))
	gc_fatal(o,0,"Can't free a live object in gc_free_all_unreferenced_objects().\n");
#endif
      debug_malloc_touch(o);
      destruct_object (o, reason);

      gc_free_extra_ref(o);
      SET_NEXT_AND_FREE(o,free_object);
    }else{
      next=o->next;
    }
    unreferenced++;
  }

  return unreferenced;
}

struct magic_index_struct
{
  struct inherit *inherit;
  struct object *o;
};

#define MAGIC_THIS ((struct magic_index_struct *)(CURRENT_STORAGE))
#define MAGIC_O2S(o) ((struct magic_index_struct *)(o->storage))

struct program *magic_index_program=0;
struct program *magic_set_index_program=0;
struct program *magic_indices_program=0;
struct program *magic_values_program=0;
struct program *magic_types_program=0;

void push_magic_index(struct program *type, int inherit_no, int parent_level)
{
  struct external_variable_context loc;
  struct object *magic;

  loc.o = Pike_fp->current_object;
  if(!loc.o) Pike_error("Illegal magic index call.\n");

  loc.parent_identifier = Pike_fp->fun;
  loc.inherit = Pike_fp->context;

  find_external_context(&loc, parent_level);

  if (!loc.o->prog)
    Pike_error ("Cannot index in destructed parent object.\n");

  magic=low_clone(type);
  add_ref(MAGIC_O2S(magic)->o=loc.o);
  MAGIC_O2S(magic)->inherit = loc.inherit + inherit_no;
#ifdef DEBUG
  if(loc.inherit + inherit_no >= loc.o->prog->inherits + loc.o->prog->num_inherits)
     Pike_fatal("Magic index blahonga!\n");
#endif
  push_object(magic);
}

/*! @namespace ::
 *!
 *! Symbols implicitly inherited from the virtual base class.
 *!
 *! These symbols exist mainly to simplify implementation of
 *! the corresponding lfuns.
 *!
 *! @seealso
 *!   @[lfun::]
 */

/* Historical API notes:
 *
 * In Pike 7.3.14 to 7.9.5 the context and access arguments were instead
 * represented by a single type argument.
 *
 * The type argument to the magic index functions is intentionally
 * undocumented since I'm not sure this API is satisfactory. The
 * argument would be explained as follows. /mast
 *
 * The indexing normally involves the externally accessable
 * identifiers (i.e. those which aren't protected or private) in the
 * current class and any inherited classes. If @[type] is 1 then
 * locally accessible identifiers are indexed too. If @[type] is 2
 * then all externally accessible identifiers in the object, i.e. also
 * those in inheriting classes, are indexed. And if @[type] is 3
 * then all locally accessible identifiers in the object, i.e. also
 * those in inheriting classes, are indexed.
 */


/*! @decl mixed ::`->(string index, object|void context, int|void access)
 *!
 *! Builtin arrow operator.
 *!
 *! @param index
 *!   Symbol in @[context] to access.
 *!
 *! @param context
 *!   Context in the current object to start the search from.
 *!   If @expr{UNDEFINED@} or left out, @expr{this_program::this@}
 *!   will be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! This function indexes the current object with the string @[index].
 *! This is useful when the arrow operator has been overloaded.
 *!
 *! @seealso
 *!   @[::`->=()]
 */
static void f_magic_index(INT32 args)
{
  struct inherit *inherit = NULL;
  int type = 0, f;
  struct pike_string *s;
  struct object *o = NULL;

  switch(args) {
    default:
    case 3:
      if (TYPEOF(sp[2-args]) != T_INT)
	SIMPLE_BAD_ARG_ERROR ("::`->", 2, "void|int");
      type = sp[2-args].u.integer & 1;
      /* FALL THROUGH */
    case 2:
      if (TYPEOF(sp[1-args]) == T_INT) {
	/* Compat with old-style args. */
	type |= (sp[1-args].u.integer & 1);
	if (sp[1-args].u.integer & 2) {
	  if(!(o=MAGIC_THIS->o))
	    Pike_error("Magic index error\n");
	  if(!o->prog)
	    Pike_error("::`-> on destructed object.\n");
	  inherit = o->prog->inherits + 0;
	}
      } else if (TYPEOF(sp[1-args]) == T_OBJECT) {
	o = sp[1-args].u.object;
	if (o != MAGIC_THIS->o)
	  Pike_error("::`-> context is not the current object.\n");
	if(!o->prog)
	  Pike_error("::`-> on destructed object.\n");
	inherit = o->prog->inherits + SUBTYPEOF(sp[1-args]);
      } else {
	SIMPLE_BAD_ARG_ERROR ("::`->", 2, "void|object|int");
      }
      /* FALL THROUGH */
    case 1:
      if (TYPEOF(sp[-args]) != T_STRING)
	SIMPLE_BAD_ARG_ERROR ("::`->", 1, "string");
      s = sp[-args].u.string;
      break;
    case 0:
      SIMPLE_TOO_FEW_ARGS_ERROR ("::`->", 1);
  }

  if(!o && !(o = MAGIC_THIS->o))
    Pike_error("Magic index error\n");

  if(!o->prog)
    Pike_error("::`-> on destructed object.\n");

  if (!inherit) {
    inherit = MAGIC_THIS->inherit;
  }

  /* NB: We could use really_low_find_shared_string_identifier()
   *     in both cases, but we get the added benefit of the
   *     identifier cache if we use find_shared_string_identifier().
   */
  if (type) {
    f = really_low_find_shared_string_identifier(s, inherit->prog,
						 SEE_PROTECTED);
  } else {
    f = find_shared_string_identifier(s, inherit->prog);
  }

  pop_n_elems(args);
  if(f<0)
  {
    push_undefined();
  }else{
    struct svalue sval;
    low_object_index_no_free(&sval,o,f+
			     inherit->identifier_level);
    *sp=sval;
    sp++;
    dmalloc_touch_svalue(Pike_sp-1);
  }
}

/*! @decl void ::`->=(string index, mixed value, @
 *!                   object|void context, int|void access)
 *!
 *! Builtin arrow set operator.
 *!
 *! @param index
 *!   Symbol in @[context] to change the value of.
 *!
 *! @param value
 *!   The new value.
 *!
 *! @param context
 *!   Context in the current object to start the search from.
 *!   If @expr{UNDEFINED@} or left out, @expr{this_program::this@}
 *!   will be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! This function indexes the current object with the string @[index],
 *! and sets it to @[value].
 *! This is useful when the arrow set operator has been overloaded.
 *!
 *! @seealso
 *!   @[::`->()]
 */
static void f_magic_set_index(INT32 args)
{
  int type = 0, f;
  struct pike_string *s;
  struct object *o = NULL;
  struct svalue *val;
  struct inherit *inherit = NULL;

  switch (args) {
    default:
    case 4:
      if (TYPEOF(sp[3-args]) != T_INT)
	SIMPLE_BAD_ARG_ERROR ("::`->=", 4, "void|int");
      type = sp[3-args].u.integer & 1;
      /* FALL THROUGH */
    case 3:
      if (TYPEOF(sp[2-args]) == T_INT) {
	/* Compat with old-style args. */
	type |= (sp[2-args].u.integer & 1);
	if (sp[2-args].u.integer & 2) {
	  if(!(o=MAGIC_THIS->o))
	    Pike_error("Magic index error\n");
	  if(!o->prog)
	    Pike_error("::`-> on destructed object.\n");
	  inherit = o->prog->inherits + 0;
	}
      } else if (TYPEOF(sp[2-args]) == T_OBJECT) {
	o = sp[2-args].u.object;
	if (o != MAGIC_THIS->o)
	  Pike_error("::`->= context is not the current object.\n");
	if(!o->prog)
	  Pike_error("::`->= on destructed object.\n");
	inherit = o->prog->inherits + SUBTYPEOF(sp[2-args]);
      } else {
	SIMPLE_BAD_ARG_ERROR ("::`->=", 3, "void|object|int");
      }
      /* FALL THROUGH */
    case 2:
      val = sp-args+1;
      if (TYPEOF(sp[-args]) != T_STRING)
	SIMPLE_BAD_ARG_ERROR ("::`->=", 1, "string");
      s = sp[-args].u.string;
      break;
    case 1:
    case 0:
      SIMPLE_TOO_FEW_ARGS_ERROR ("::`->=", 2);
  }

  if(!o && !(o = MAGIC_THIS->o))
    Pike_error("Magic index error\n");

  if(!o->prog)
    Pike_error("::`->= on destructed object.\n");

  if (!inherit) {
    inherit = MAGIC_THIS->inherit;
  }

  /* NB: We could use really_low_find_shared_string_identifier()
   *     in both cases, but we get the added benefit of the
   *     identifier cache if we use find_shared_string_identifier().
   */
  if (type) {
    f = really_low_find_shared_string_identifier(s, inherit->prog,
						 SEE_PROTECTED);
  } else {
    f = find_shared_string_identifier(s, inherit->prog);
  }

  if(f<0)
  {
    if (s->len < 1024)
      Pike_error("No such variable (%S) in object.\n", s);
    else
      Pike_error("No such variable in object.\n");
  }else{
    object_low_set_index(o, f+inherit->identifier_level,
			 val);
    pop_n_elems(args);
    push_int(0);
  }
}

/*! @decl mixed ::_indices(object|void context, int|void access)
 *!
 *! @param context
 *!   Context in the current object to start the list from.
 *!   If @expr{UNDEFINED@} or left out, this_program::this
 *!   will be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! Builtin function to list the identifiers of an object.
 *! This is useful when @[lfun::_indices] has been overloaded.
 *!
 *! @seealso
 *!   @[::_values()], @[::_types()], @[::`->()]
 */
static void f_magic_indices (INT32 args)
{
  struct object *obj = NULL;
  struct program *prog = NULL;
  struct inherit *inherit = NULL;
  struct array *res;
  int type = 0, e, i;

  switch(args) {
    default:
    case 2:
      if (TYPEOF(sp[1-args]) != T_INT)
	SIMPLE_BAD_ARG_ERROR ("::_indices", 2, "void|int");
      type = sp[-args].u.integer;
      /* FALL THROUGH */
    case 1:
      if (TYPEOF(sp[-args]) == T_INT) {
	/* Compat with old-style args. */
	type |= (sp[-args].u.integer & 1);
	if (sp[-args].u.integer & 2) {
	  if(!(obj=MAGIC_THIS->o))
	    Pike_error("Magic index error\n");
	  if(!obj->prog)
	    Pike_error("Object is destructed.\n");
	  inherit = obj->prog->inherits + 0;
	}
      } else if (TYPEOF(sp[2-args]) == T_OBJECT) {
	obj = sp[2-args].u.object;
	if (obj != MAGIC_THIS->o)
	  Pike_error("::_indices context is not the current object.\n");
	if(!obj->prog)
	  Pike_error("::_indices on destructed object.\n");
	inherit = obj->prog->inherits + SUBTYPEOF(sp[2-args]);
      } else {
	SIMPLE_BAD_ARG_ERROR ("::_indices", 1, "void|object|int");
      }
      /* FALL THROUGH */
    case 0:
      break;
  }

  if (!obj && !(obj = MAGIC_THIS->o)) Pike_error ("Magic index error\n");
  if (!obj->prog) Pike_error ("Object is destructed.\n");

  if (!inherit) {
    /* FIXME: This has somewhat odd behavior if there are local identifiers
     *        before inherits that are overridden by them
     *        (e.g. listing duplicate identifiers). But then again, this is
     *        not the only place where that gives strange effects, imho.
     *   /mast
     */
    inherit = MAGIC_THIS->inherit;
  }

  prog = inherit->prog;

  if (type & 1) {
    pop_n_elems (args);
    push_array (res = allocate_array(prog->num_identifier_references));
    for (e = i = 0; e < (int) prog->num_identifier_references; e++) {
      struct reference *ref = prog->identifier_references + e;
      struct identifier *id = ID_FROM_PTR (prog, ref);
      if (ref->id_flags & ID_HIDDEN) continue;
      if ((ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	  (ID_INHERITED|ID_PRIVATE)) continue;
      SET_SVAL(ITEM(res)[i], T_STRING, 0, string, id->name);
      add_ref(id->name);
      i++;
    }
    res->type_field |= BIT_STRING;
    sp[-1].u.array = resize_array (res, i);
    res->type_field = BIT_STRING;
    return;
  }

  pop_n_elems (args);
  push_array (res = allocate_array_no_init (prog->num_identifier_index, 0));
  for (e = 0; e < (int) prog->num_identifier_index; e++) {
    SET_SVAL(ITEM(res)[e], T_STRING, 0, string,
	     ID_FROM_INT (prog, prog->identifier_index[e])->name);
    add_ref(ITEM(res)[e].u.string);
  }
  res->type_field = BIT_STRING;
}

/*! @decl mixed ::_values(object|void context, int|void access)
 *!
 *! @param context
 *!   Context in the current object to start the list from.
 *!   If @expr{UNDEFINED@} or left out, this_program::this
 *!   will be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! Builtin function to list the values of the identifiers of an
 *! object. This is useful when @[lfun::_values] has been overloaded.
 *!
 *! @seealso
 *!   @[::_indices()], @[::_types()], @[::`->()]
 */
static void f_magic_values (INT32 args)
{
  struct object *obj = NULL;
  struct program *prog = NULL;
  struct inherit *inherit = NULL;
  struct array *res;
  TYPE_FIELD types;
  int type = 0, e, i;

  switch(args) {
    default:
    case 2:
      if (TYPEOF(sp[1-args]) != T_INT)
	SIMPLE_BAD_ARG_ERROR ("::_indices", 2, "void|int");
      type = sp[-args].u.integer;
      /* FALL THROUGH */
    case 1:
      if (TYPEOF(sp[-args]) == T_INT) {
	/* Compat with old-style args. */
	type |= (sp[-args].u.integer & 1);
	if (sp[-args].u.integer & 2) {
	  if(!(obj=MAGIC_THIS->o))
	    Pike_error("Magic index error\n");
	  if(!obj->prog)
	    Pike_error("Object is destructed.\n");
	  inherit = obj->prog->inherits + 0;
	}
      } else if (TYPEOF(sp[2-args]) == T_OBJECT) {
	obj = sp[2-args].u.object;
	if (obj != MAGIC_THIS->o)
	  Pike_error("::_values context is not the current object.\n");
	if(!obj->prog)
	  Pike_error("::_values on destructed object.\n");
	inherit = obj->prog->inherits + SUBTYPEOF(sp[2-args]);
      } else {
	SIMPLE_BAD_ARG_ERROR ("::_values", 1, "void|object|int");
      }
      /* FALL THROUGH */
    case 0:
      break;
  }

  if (!obj && !(obj = MAGIC_THIS->o)) Pike_error ("Magic index error\n");
  if (!obj->prog) Pike_error ("Object is destructed.\n");

  if (!inherit) {
    /* FIXME: This has somewhat odd behavior if there are local identifiers
     *        before inherits that are overridden by them
     *        (e.g. listing duplicate identifiers). But then again, this is
     *        not the only place where that gives strange effects, imho.
     *   /mast
     */
    inherit = MAGIC_THIS->inherit;
  }

  prog = inherit->prog;

  if (type & 1) {
    pop_n_elems (args);
    push_array (res = allocate_array(prog->num_identifier_references));
    types = 0;
    for (e = i = 0; e < (int) prog->num_identifier_references; e++) {
      struct reference *ref = prog->identifier_references + e;
      struct identifier *id = ID_FROM_PTR (prog, ref);
      if (ref->id_flags & ID_HIDDEN) continue;
      if ((ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	  (ID_INHERITED|ID_PRIVATE)) continue;
      low_object_index_no_free (ITEM(res) + i, obj,
				e + inherit->identifier_level);
      types |= 1 << TYPEOF(ITEM(res)[i]);
      i++;
    }
    res->type_field |= types;
    sp[-1].u.array = resize_array (res, i);
    res->type_field = types;
    return;
  }

  pop_n_elems (args);
  push_array (res = allocate_array_no_init (prog->num_identifier_index, 0));
  types = 0;
  for (e = 0; e < (int) prog->num_identifier_index; e++) {
    low_object_index_no_free (ITEM(res) + e, obj,
			      prog->identifier_index[e] + inherit->identifier_level);
    types |= 1 << TYPEOF(ITEM(res)[e]);
  }
  res->type_field = types;
}

/*! @decl mixed ::_types(object|void context, int|void access)
 *!
 *! @param context
 *!   Context in the current object to start the list from.
 *!   If @expr{UNDEFINED@} or left out, this_program::this
 *!   will be used (ie start at the current context and ignore
 *!   any overloaded symbols).
 *!
 *! @param access
 *!   Access permission override. One of the following:
 *!   @int
 *!     @value 0
 *!     @value UNDEFINED
 *!       See only public symbols.
 *!     @value 1
 *!       See protected symbols as well.
 *!   @endint
 *!
 *! Builtin function to list the types of the identifiers of an
 *! object. This is useful when @[lfun::_types] has been overloaded.
 *!
 *! @seealso
 *!   @[::_indices()], @[::_values()], @[::`->()]
 */
static void f_magic_types (INT32 args)
{
  struct object *obj = NULL;
  struct program *prog = NULL;
  struct inherit *inherit = NULL;
  struct array *res;
  TYPE_FIELD types;
  int type = 0, e, i;

  switch(args) {
    default:
    case 2:
      if (TYPEOF(sp[1-args]) != T_INT)
	SIMPLE_BAD_ARG_ERROR ("::_types", 2, "void|int");
      type = sp[-args].u.integer;
      /* FALL THROUGH */
    case 1:
      if (TYPEOF(sp[-args]) == T_INT) {
	/* Compat with old-style args. */
	type |= (sp[-args].u.integer & 1);
	if (sp[-args].u.integer & 2) {
	  if(!(obj=MAGIC_THIS->o))
	    Pike_error("Magic index error\n");
	  if(!obj->prog)
	    Pike_error("Object is destructed.\n");
	  inherit = obj->prog->inherits + 0;
	}
      } else if (TYPEOF(sp[2-args]) == T_OBJECT) {
	obj = sp[2-args].u.object;
	if (obj != MAGIC_THIS->o)
	  Pike_error("::_types context is not the current object.\n");
	if(!obj->prog)
	  Pike_error("::_types on destructed object.\n");
	inherit = obj->prog->inherits + SUBTYPEOF(sp[2-args]);
      } else {
	SIMPLE_BAD_ARG_ERROR ("::_types", 1, "void|object|int");
      }
      /* FALL THROUGH */
    case 0:
      break;
  }

  if (!obj && !(obj = MAGIC_THIS->o)) Pike_error ("Magic index error\n");
  if (!obj->prog) Pike_error ("Object is destructed.\n");

  if (!inherit) {
    /* FIXME: This has somewhat odd behavior if there are local identifiers
     *        before inherits that are overridden by them
     *        (e.g. listing duplicate identifiers). But then again, this is
     *        not the only place where that gives strange effects, imho.
     *   /mast
     */
    inherit = MAGIC_THIS->inherit;
  }

  prog = inherit->prog;

  if (type & 1) {
    pop_n_elems (args);
    push_array (res = allocate_array(prog->num_identifier_references));
    types = 0;
    for (e = i = 0; e < (int) prog->num_identifier_references; e++) {
      struct reference *ref = prog->identifier_references + e;
      struct identifier *id = ID_FROM_PTR (prog, ref);
      if (ref->id_flags & ID_HIDDEN) continue;
      if ((ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	  (ID_INHERITED|ID_PRIVATE)) continue;
      SET_SVAL(ITEM(res)[i], PIKE_T_TYPE, 0, type, id->type);
      add_ref(id->type);
      i++;
      types = BIT_TYPE;
    }
    res->type_field |= types;
    sp[-1].u.array = resize_array (res, i);
    res->type_field = types;
    return;
  }

  pop_n_elems (args);
  push_array (res = allocate_array_no_init (prog->num_identifier_index, 0));
  types = 0;
  for (e = 0; e < (int) prog->num_identifier_index; e++) {
    struct identifier *id = ID_FROM_INT(prog, prog->identifier_index[e]);
    SET_SVAL(ITEM(res)[e], PIKE_T_TYPE, 0, type, id->type);
    add_ref(id->type);
    types = BIT_TYPE;
  }
  res->type_field = types;
}

/*! @endnamespace
 */

void low_init_object(void)
{
  init_destroy_called_mark_hash();
}

void init_object(void)
{
  ptrdiff_t offset;

  enter_compiler(NULL, 0);

  /* ::`->() */
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  MAP_VARIABLE("__obj", tObj, ID_PROTECTED,
	       offset + OFFSETOF(magic_index_struct, o), T_OBJECT);
  ADD_FUNCTION("`()", f_magic_index,
	       tFunc(tStr tOr3(tVoid,tObj,tDeprecated(tInt)) tOr(tVoid,tInt),
		     tMix), ID_PROTECTED);
  magic_index_program=end_program();

  /* ::`->=() */
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  MAP_VARIABLE("__obj", tObj, ID_PROTECTED,
	       offset + OFFSETOF(magic_index_struct, o), T_OBJECT);
  ADD_FUNCTION("`()", f_magic_set_index,
	       tFunc(tStr tMix tOr3(tVoid,tObj,tDeprecated(tInt))
		     tOr(tVoid,tInt), tVoid), ID_PROTECTED);
  magic_set_index_program=end_program();

  /* ::_indices() */
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  MAP_VARIABLE("__obj", tObj, ID_PROTECTED,
	       offset + OFFSETOF(magic_index_struct, o), T_OBJECT);
  ADD_FUNCTION("`()", f_magic_indices,
	       tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) tOr(tVoid,tInt),
		     tArr(tStr)), ID_PROTECTED);
  magic_indices_program=end_program();

  /* ::_values() */
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  MAP_VARIABLE("__obj", tObj, ID_PROTECTED,
	       offset + OFFSETOF(magic_index_struct, o), T_OBJECT);
  ADD_FUNCTION("`()", f_magic_values,
	       tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) tOr(tVoid,tInt),
		     tArray), ID_PROTECTED);
  magic_values_program=end_program();

  /* ::_types() */
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  MAP_VARIABLE("__obj", tObj, ID_PROTECTED,
	       offset + OFFSETOF(magic_index_struct, o), T_OBJECT);
  ADD_FUNCTION("`()", f_magic_types,
	       tFunc(tOr3(tVoid,tObj,tDeprecated(tInt)) tOr(tVoid,tInt),
		     tArray), ID_PROTECTED);
  magic_types_program=end_program();

  exit_compiler();
}

void exit_object(void)
{
  if (destruct_object_evaluator_callback) {
    remove_callback(destruct_object_evaluator_callback);
    destruct_object_evaluator_callback = NULL;
  }

  master_is_cleaned_up = 1;
  if (master_object) {
    call_destroy (master_object, 1);
    destruct_object (master_object, DESTRUCT_CLEANUP);
    free_object(master_object);
    master_object=0;
  }

  if (master_program) {
    free_program(master_program);
    master_program=0;
  }

  destruct_objects_to_destruct();

  if(magic_index_program)
  {
    free_program(magic_index_program);
    magic_index_program=0;
  }

  if(magic_set_index_program)
  {
    free_program(magic_set_index_program);
    magic_set_index_program=0;
  }

  if(magic_indices_program)
  {
    free_program(magic_indices_program);
    magic_indices_program=0;
  }

  if(magic_values_program)
  {
    free_program(magic_values_program);
    magic_values_program=0;
  }

  if(magic_types_program)
  {
    free_program(magic_types_program);
    magic_types_program=0;
  }
}

#ifdef PIKE_DEBUG
void check_object_context(struct object *o,
			  struct program *context_prog,
			  char *current_storage)
{
  int q;
  if(o == Pike_compiler->fake_object) return;
  if( ! o->prog ) return; /* Variables are already freed */
  for(q=0;q<(int)context_prog->num_variable_index;q++)
  {
    int d=context_prog->variable_index[q];
    struct identifier *id = context_prog->identifiers + d;

    if(d<0 || d>=context_prog->num_identifiers)
      Pike_fatal("Illegal index in variable_index!\n");

    if(id->run_time_type == T_MIXED)
    {
      struct svalue *s;
      s=(struct svalue *)(current_storage + id->func.offset);
      check_svalue(s);
    }else{
      union anything *u;
      u=(union anything *)(current_storage + id->func.offset);
      if ((id->run_time_type != PIKE_T_GET_SET) &&
	  (id->run_time_type != PIKE_T_FREE))
	check_short_svalue(u, id->run_time_type);
    }
  }
}

void check_object(struct object *o)
{
  int e;
  struct program *p;
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if(o == Pike_compiler->fake_object) return;

  if(o->next)
  {
    if (o->next == o)
    {
      describe(o);
      Pike_fatal("Object check: o->next == o\n");
    }
    if (o->next->prev !=o)
    {
      describe(o);
      Pike_fatal("Object check: o->next->prev != o\n");
    }
  }

  if(o->prev)
  {
    if (o->prev == o)
    {
      describe(o);
      Pike_fatal("Object check: o->prev == o\n");
    }
    if(o->prev->next != o)
    {
      describe(o);
      Pike_fatal("Object check: o->prev->next != o\n");
    }
    
    if(o == first_object)
      Pike_fatal("Object check: o->prev !=0 && first_object == o\n");
  } else {
    if(first_object != o)
      Pike_fatal("Object check: o->prev ==0 && first_object != o\n");
  }
  
  if(o->refs <= 0)
    Pike_fatal("Object refs <= zero.\n");

  if(!(p=o->prog)) return;

  if(id_to_program(o->prog->id) != o->prog)
    Pike_fatal("Object's program not in program list.\n");

  if(!(o->prog->flags & PROGRAM_PASS_1_DONE)) return;

  for(e=p->num_inherits-1; e>=0; e--)
  {
    check_object_context(o,
			 p->inherits[e].prog,
			 o->storage + p->inherits[e].storage_offset);
  }
}

void check_all_objects(void)
{
  struct object *o, *next;
  for(o=first_object;o;o=next)
  {
    add_ref(o);
    check_object(o);
    SET_NEXT_AND_FREE(o,free_object);
  }

  for(o=objects_to_destruct;o;o=o->next)
    if(o->refs)
      Pike_fatal("Object to be destructed has references.\n");
}

#endif
