/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: object.c,v 1.256 2005/03/18 14:46:59 grubba Exp $
*/

#include "global.h"
RCSID("$Id: object.c,v 1.256 2005/03/18 14:46:59 grubba Exp $");
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
  o->storage=p->storage_needed ? (char *)xalloc(p->storage_needed) : (char *)NULL;

  GC_ALLOC(o);

#ifdef DEBUG_MALLOC
  if(!debug_malloc_copy_names(o, p)) 
  {
    struct pike_string *tmp;
    INT32 line;

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
  INIT_PIKE_MEMOBJ(o);

#ifdef PIKE_DEBUG
  o->program_id=p->id;
#endif

  return o;
}

#define LOW_PUSH_FRAME(O)	do{		\
  struct pike_frame *pike_frame=alloc_pike_frame();		\
  pike_frame->next=Pike_fp;			\
  pike_frame->current_object=O;			\
  pike_frame->locals=0;				\
  pike_frame->num_locals=0;				\
  pike_frame->fun=-1;				\
  pike_frame->pc=0;					\
  pike_frame->context.prog=0;                        \
  pike_frame->context.parent=0;                        \
  Pike_fp= pike_frame


#define LOW_PUSH_FRAME2(O)                      \
  pike_frame=alloc_pike_frame();		\
  pike_frame->next=Pike_fp;			\
  pike_frame->current_object=O;			\
  pike_frame->locals=0;				\
  pike_frame->num_locals=0;				\
  pike_frame->fun=-1;				\
  pike_frame->pc=0;					\
  pike_frame->context.prog=0;                        \
  pike_frame->context.parent=0;                        \
  Pike_fp= pike_frame

#define PUSH_FRAME(O) \
  LOW_PUSH_FRAME(O); \
  add_ref(pike_frame->current_object)

#define PUSH_FRAME2(O) \
  do{LOW_PUSH_FRAME2(O); add_ref(pike_frame->current_object);}while(0)

/* Note: there could be a problem with programs without functions */
#define SET_FRAME_CONTEXT(X)						     \
  if(pike_frame->context.prog) free_program(pike_frame->context.prog);	     \
  pike_frame->context=(X);						     \
  pike_frame->fun=pike_frame->context.identifier_level;                      \
  add_ref(pike_frame->context.prog);					     \
  pike_frame->current_storage=o->storage+pike_frame->context.storage_offset; \
  pike_frame->context.parent=0;
  

#define LOW_SET_FRAME_CONTEXT(X)					     \
  pike_frame->context=(X);						     \
  pike_frame->fun=pike_frame->context.identifier_level;			     \
  pike_frame->current_storage=o->storage+pike_frame->context.storage_offset; \
  pike_frame->context.parent=0;

#define LOW_UNSET_FRAME_CONTEXT()		\
  pike_frame->context.parent=0;			\
  pike_frame->context.prog=0;			\
  pike_frame->current_storage=0;		\
  pike_frame->context.parent=0;
  

#ifdef DEBUG
#define CHECK_FRAME() do { \
    if(pike_frame != Pike_fp) \
      Pike_fatal("Frame stack out of whack.\n"); \
  } while(0)
#else
#define CHECK_FRAME()
#endif

#define POP_FRAME()				\
  CHECK_FRAME()					\
  Pike_fp=pike_frame->next;			\
  pike_frame->next=0;				\
  free_pike_frame(pike_frame); }while(0)

#define POP_FRAME2()				\
  do{CHECK_FRAME()				\
  Pike_fp=pike_frame->next;			\
  pike_frame->next=0;				\
  free_pike_frame(pike_frame);}while(0)

#define LOW_POP_FRAME()				\
  add_ref(Pike_fp->current_object); \
  POP_FRAME();



PMOD_EXPORT void call_c_initializers(struct object *o)
{
  int e;
  struct program *p=o->prog;
  struct pike_frame *pike_frame=0;
  int frame_pushed = 0;
  
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
	  s->type=T_INT;
	  s->u.integer=0;
	  s->subtype=0;
	}else{
	  union anything *u;
	  u=(union anything *)(storage + prog->identifiers[d].func.offset);
	  switch(prog->identifiers[d].run_time_type)
	  {
	    case T_INT: u->integer=0; break;
	    case T_FLOAT: u->float_number=0.0; break;
	    default: u->refs=0; break;
	  }
	}
	(void) debug_malloc_update_location(o, DMALLOC_NAMED_LOCATION(" clear_global"));
      }
    }
    if(prog->event_handler)
    {
      if( !frame_pushed )
      {
	PUSH_FRAME2(o);
	frame_pushed = 1;
      }
      SET_FRAME_CONTEXT(p->inherits[e]);
      prog->event_handler(PROG_EVENT_INIT);
    }
  }
  if( frame_pushed )
    POP_FRAME2();
}


void call_prog_event(struct object *o, int event)
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
	PUSH_FRAME2(o);
	frame_pushed = 1;
      }
      SET_FRAME_CONTEXT(p->inherits[e]);
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
  fun=FIND_LFUN(p, LFUN___INIT);
  if(fun!=-1)
  {
    apply_low(o,fun,0);
    pop_stack();
  }
  fun=FIND_LFUN(p, LFUN_CREATE);
  if(fun!=-1)
  {
    apply_low(o,fun,args);
    pop_stack();
  }
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
  SET_ONERROR(tmp, do_free_object, o);
  debug_malloc_touch(o);
  call_c_initializers(o);
  debug_malloc_touch(o);
  call_pike_initializers(o,args);
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);
  UNSET_ONERROR(tmp);
  return o;
}

/* Clone and initialize an object, but do not call the pike initializers.
 *
 * WARNING: Only use this function if you know what you are doing...
 */
PMOD_EXPORT struct object *fast_clone_object(struct program *p)
{
  ONERROR tmp;
  struct object *o=low_clone(p);
  SET_ONERROR(tmp, do_free_object, o);
  debug_malloc_touch(o);
  call_c_initializers(o);
  UNSET_ONERROR(tmp);
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
  SET_ONERROR(tmp, do_free_object, o);
  debug_malloc_touch(o);

  if(!(p->flags & PROGRAM_PASS_1_DONE))
    Pike_error("Attempting to clone an unfinished program\n");

  if(p->flags & PROGRAM_USES_PARENT)
  {
    add_ref( PARENT_INFO(o)->parent=parent );
    PARENT_INFO(o)->parent_identifier = DO_NOT_WARN((INT32)parent_identifier);
  }
  call_c_initializers(o);
  call_pike_initializers(o,args);
  UNSET_ONERROR(tmp);
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
  
  if(prog->type == T_FUNCTION)
  {
    parent=prog->u.object;
    parent_identifier=prog->subtype;
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

static struct pike_string *low_read_file(char *file)
{
  struct pike_string *s;
  ptrdiff_t len;
  FD f;

  while((f = fd_open(file,fd_RDONLY,0666)) <0 && errno==EINTR)
    check_threads_etc();
  if(f >= 0)
  {
    ptrdiff_t tmp, pos = 0;

    len = fd_lseek(f, 0, SEEK_END);
    fd_lseek(f, 0, SEEK_SET);
    s = begin_shared_string(len);

    while(pos<len)
    {
      tmp = fd_read(f,s->str+pos,len-pos);
      if(tmp<=0)
      {
	if (tmp < 0) {
	  if(errno==EINTR) {
	    check_threads_etc();
	    continue;
	  }
	  Pike_fatal("low_read_file(%s) failed, errno=%d\n",file,errno);
	}
	Pike_fatal("low_read_file(%s) failed, short read: %d < %d\n",
		   file, pos, len);
      }
      pos+=tmp;
    }
    fd_close(f);
    return end_shared_string(s);
  }
  return 0;
}

static void get_master_cleanup (void *dummy)
{
  if (master_object) {
    free_object (master_object);
    master_object = 0;
  }
}

PMOD_EXPORT struct object *get_master(void)
{
  extern char *master_file;
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
      /* fprintf(stderr,"Cannot load master object yet!\n"); */
      return 0;
    }

    /* fprintf(stderr, "Master file: \"%s\"\n", master_file); */

    tmp=xalloc(strlen(master_file)+3);

    MEMCPY(tmp, master_file, strlen(master_file)+1);
    strcat(tmp,".o");

    s = NULL;
    if (!fd_stat(tmp, &stat_buf)) {
      long ts1 = stat_buf.st_mtime;
      long ts2 = 0;		/* FIXME: Should really be MIN_INT, but... */

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
	if(d_flag)
	  debug_describe_svalue(&throw_value);
#endif
	/* do nothing */
	UNSETJMP(tmp);
	free_svalue(&throw_value);
	throw_value.type = T_INT;
      }else{
	f_decode_value(2);
	UNSETJMP(tmp);

	if(sp[-1].type == T_PROGRAM)
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
      if(sp[-1].type != T_PROGRAM)
      {
	pop_stack();
	return 0;
      }
      master_program=sp[-1].u.program;
      sp--;
      dmalloc_touch_svalue(sp);
    }else{
      throw_error_object (low_clone (master_load_error_program), 0, 0, 0,
			  "Couldn't load master program from %s.\n", master_file);
    }
  }

  {
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

static void call_destroy(struct object *o, int foo)
{
  int e;

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
    if(check_destroy_called_mark_semafore(o))
    {
      JMP_BUF jmp;

#ifdef GC_VERBOSE
      if (Pike_in_gc > GC_PASS_PREPARE)
	fprintf(stderr, "|   Calling destroy() in %p with %d refs.\n",
		o, o->refs);
#endif

      free_svalue (&throw_value);
      throw_value.type = T_INT;

      if (SETJMP (jmp)) {
	UNSETJMP (jmp);
	if (gc_destruct_everything) {
	  struct svalue err;
	  move_svalue (&err, &throw_value);
	  throw_value.type = T_INT;
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
	if(foo) push_int(1);
	apply_low(o, e, foo?1:0);
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


void destruct(struct object *o)
{
  int e;
  struct program *p;
  struct pike_frame *pike_frame=0;
  int frame_pushed = 0;

#ifdef PIKE_DEBUG
  ONERROR uwp;

  fatal_check_c_stack(8192);

  SET_ONERROR(uwp, fatal_on_error,
	      "Shouldn't get an exception in destruct().\n");
  if(d_flag > 20) do_debug();
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
  add_ref( o );
  call_destroy(o,0);

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
	PUSH_FRAME2(o);
	frame_pushed = 1;
      }
      SET_FRAME_CONTEXT(p->inherits[e]);
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
	if ((s->type != T_OBJECT && s->type != T_FUNCTION) ||
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
      }else{
	union anything *u;
	u=(union anything *)(storage + id->func.offset);
#ifdef DEBUG_MALLOC
	if (rtt <= MAX_REF_TYPE) {debug_malloc_touch(u->refs);}
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

  remove_destroy_called_mark(o);

#ifdef PIKE_DEBUG
  UNSET_ONERROR(uwp);
#endif
}


struct object *objects_to_destruct = 0;
static struct callback *destruct_object_evaluator_callback =0;

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
   * the objects arbitrarily late. */
  while (objects_to_destruct) {
    o = objects_to_destruct, objects_to_destruct = 0;
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
      destruct(o);
      free_object(o);
    } while ((o = next));
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
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE && o->next != o)
    Pike_fatal("Freeing objects is not allowed inside the gc.\n");
#endif

  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if(o->prog && (o->prog->flags & PROGRAM_DESTRUCT_IMMEDIATE))
  {
    add_ref(o);
    destruct(o);
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
    DO_IF_DMALLOC(o->prev = (void *) -1);
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


/* Get a variable through internal indexing, i.e. directly by
 * identifier index without going through `->= or `[]= lfuns.
 *
 * NOTE: This function may be called by the compiler on objects
 *       cloned from unfinished programs (ie placeholder
 *       objects). Degenerated cases may thus occur.
 */
PMOD_EXPORT void low_object_index_no_free(struct svalue *to,
					  struct object *o,
					  ptrdiff_t f)
{
  struct identifier *i;
  struct program *p=o->prog;
  
  if(!p)
    Pike_error("Cannot access variables in destructed object.\n");

  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  i=ID_FROM_INT(p, f);

  switch(i->identifier_flags & IDENTIFIER_TYPE_MASK)
  {
  case IDENTIFIER_FUNCTION:
    /* Pike function with tentative type from the first pass. */
    /* FALL_THROUGH */
  case IDENTIFIER_PIKE_FUNCTION:
    if (i->func.offset == -1 && p->flags & PROGRAM_FINISHED) {
      /* Prototype. In the first pass we must be able to get a
       * function anyway.
       *
       * We also need to get a function anyway if we're currently
       * in the second pass of compiling this program, since the
       * function might be defined further ahead.
       */
      to->type=T_INT;
      to->subtype=NUMBER_UNDEFINED;
      to->u.integer=0;
      break;
    }
    /* Fall through. */

  case IDENTIFIER_C_FUNCTION:
    to->type=T_FUNCTION;
    to->subtype = DO_NOT_WARN(f);
    to->u.object=o;
    add_ref(o);
    break;

  case IDENTIFIER_CONSTANT:
    {
      if (i->func.offset >= 0) {
	struct svalue *s;
	s=& PROG_FROM_INT(p,f)->constants[i->func.offset].sval;
	if(s->type==T_PROGRAM &&
	   (s->u.program->flags & PROGRAM_USES_PARENT))
	{
	  to->type=T_FUNCTION;
	  to->subtype = DO_NOT_WARN(f);
	  to->u.object=o;
	  add_ref(o);
	}else{
	  check_destructed(s);
	  assign_svalue_no_free(to, s);
	}
      } else {
	/* Prototype constant. */
	to->type = T_INT;
	to->subtype = NUMBER_NUMBER;
	to->u.integer = 0;
      }
      break;
    }

  case IDENTIFIER_VARIABLE:
  {
    void *ptr=LOW_GET_GLOBAL(o,f,i);
    switch(i->run_time_type)
    {
      case T_MIXED:
      {
	struct svalue *s=(struct svalue *)ptr;
	check_destructed(s);
	assign_svalue_no_free(to, s);
	break;
      }

      case T_FLOAT:
	to->type=T_FLOAT;
	to->subtype=0;
	to->u.float_number=*(FLOAT_TYPE *)ptr;
	break;

      case T_INT:
	to->type=T_INT;
	to->subtype=0;
	to->u.integer=*(INT_TYPE *)ptr;
	break;

      default:
      {
	struct ref_dummy *dummy;

	to->subtype=0;
	if((dummy=*(struct ref_dummy  **)ptr))
	{
	  add_ref(to->u.dummy=dummy);
	  to->type=i->run_time_type;
	}else{
	  to->type=T_INT;
	  to->u.integer=0;
	}
	break;
      }
    }
    break;
  }

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
				       struct svalue *index)
{
  struct program *p;
  int f = -1;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  switch(index->type)
  {
  case T_STRING:
    f=find_shared_string_identifier(index->u.string, p);
    break;

  case T_OBJ_INDEX:
    f=index->u.identifier;
    break;

  default:
#ifdef PIKE_DEBUG
    if (index->type > MAX_TYPE)
      Pike_fatal ("Invalid type %d in index.\n", index->type);
#endif
    Pike_error("Lookup in object with non-string index.\n");
  }

  if(f < 0)
  {
    to->type=T_INT;
    to->subtype=NUMBER_UNDEFINED;
    to->u.integer=0;
  }else{
    low_object_index_no_free(to, o, f);
  }
}

#define ARROW_INDEX_P(X) ((X)->type==T_STRING && (X)->subtype)

/* Get a variable through external indexing, i.e. by going through
 * `-> or `[] lfuns, not seeing private and static etc. */
PMOD_EXPORT void object_index_no_free(struct svalue *to,
				      struct object *o,
				      struct svalue *index)
{
  struct program *p = NULL;
  int lfun,l;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

#ifdef PIKE_DEBUG
  if (index->type > MAX_TYPE)
    Pike_fatal ("Invalid index type %d.\n", index->type);
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
	tmp.type = T_OBJECT;
	tmp.u.object = o;
	print_svalue (stderr, &tmp);
	fputs (" with ", stderr);
	print_svalue (stderr, index);
	fputc ('\n', stderr);
#endif
	add_ref(to->u.object=placeholder_object);
	to->type=T_OBJECT;
	return;
      }
    }
    l=low_find_lfun(p, lfun);
  }
  if(l != -1)
  {
    push_svalue(index);
    apply_lfun(o, lfun, 1);
    *to=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
  } else {
    object_index_no_free2(to, o, index);
  }
}


/* Assign a variable through internal indexing, i.e. directly by
 * identifier index without going through `->= or `[]= lfuns. */
PMOD_EXPORT void object_low_set_index(struct object *o,
				      int f,
				      struct svalue *from)
{
  struct identifier *i;
  struct program *p = NULL;
  int rtt, id_flags;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);
  check_destructed(from);

  i=ID_FROM_INT(p, f);
  rtt = i->run_time_type;
  id_flags = i->identifier_flags;

  if(!IDENTIFIER_IS_VARIABLE(id_flags))
  {
    Pike_error("Cannot assign functions or constants.\n");
  }
  else if(rtt == T_MIXED)
  {
    /* Don't count references to ourselves to help the gc. DDTAH. */
    struct svalue *to = (struct svalue *) LOW_GET_GLOBAL(o,f,i);
    dmalloc_touch_svalue(to);
    if ((to->type != T_OBJECT && to->type != T_FUNCTION) ||
	to->u.object != o ||
	!(id_flags & IDENTIFIER_NO_THIS_REF)) {
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
    if ((to->type != T_OBJECT && to->type != T_FUNCTION) ||
	(to->u.object != o) ||
	!(id_flags & IDENTIFIER_NO_THIS_REF)) {
      if(to->type <= MAX_REF_TYPE) {
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
  }
  else
  {
    /* Partial code duplication from assign_to_short_svalue. */
    union anything *u = (union anything *) LOW_GET_GLOBAL(o,f,i);
    if(from->type == rtt)
    {
      switch(rtt)
      {
	case T_INT: u->integer=from->u.integer; break;
	case T_FLOAT: u->float_number=from->u.float_number; break;
	case T_OBJECT:
	  if (id_flags & IDENTIFIER_NO_THIS_REF) {
	    /* Don't count references to ourselves to help the gc. */
	    debug_malloc_touch(u->object);
	    if ((u->object != o) && u->refs && !sub_ref(u->dummy)) {
	      debug_malloc_touch(o);
	      really_free_short_svalue(u,rtt);
#ifdef DEBUG_MALLOC
	    } else {
	      debug_malloc_touch(o);
#endif /* DEBUG_MALLOC */
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
	    break;
	  }
	  /* FALL THROUGH */
	default:
	  debug_malloc_touch(u->refs);
	  if(u->refs && !sub_ref(u->dummy))
	    really_free_short_svalue(u,rtt);
	  u->refs = from->u.refs;
	  add_ref(u->dummy);
      }
    }else if(rtt<=MAX_REF_TYPE && UNSAFE_IS_ZERO(from)){
      if((rtt != T_OBJECT || u->object != o || !(id_flags & IDENTIFIER_NO_THIS_REF)) &&
	 u->refs && !sub_ref(u->dummy)) {
	debug_malloc_touch(u->ptr);
	really_free_short_svalue(u,rtt);
      }
      debug_malloc_touch(u->ptr);
      u->refs=0;
    }else{
      Pike_error("Wrong type in assignment, expected %s, got %s.\n",
		 get_name_of_type(rtt),
		 get_name_of_type(from->type));
    }
  }
}

/* Assign a variable without going through `->= or `[]= lfuns. If
 * index is a string then the externally visible identifiers are
 * indexed. If index is T_OBJ_INDEX then any identifier is accessed
 * through identifier index. */
PMOD_EXPORT void object_set_index2(struct object *o,
				   struct svalue *index,
				   struct svalue *from)
{
  struct program *p;
  int f = -1;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  switch(index->type)
  {
  case T_STRING:
    f=find_shared_string_identifier(index->u.string, p);
    break;

  case T_OBJ_INDEX:
    f=index->u.identifier;
    break;

  default:
#ifdef PIKE_DEBUG
    if (index->type > MAX_TYPE)
      Pike_fatal ("Invalid type %d in index.\n", index->type);
#endif
    Pike_error("Lookup on non-string value.\n");
  }

  if(f < 0)
  {
    if (index->type == T_STRING && !index->u.string->size_shift &&
	index->u.string->len < 1024)
      Pike_error("No such variable (%s) in object.\n", index->u.string->str);
    else
      Pike_error("No such variable in object.\n");
  }else{
    object_low_set_index(o, f, from);
  }
}

/* Assign a variable through external indexing, i.e. by going through
 * `->= or `[]= lfuns, not seeing private and static etc. */
PMOD_EXPORT void object_set_index(struct object *o,
				  struct svalue *index,
				  struct svalue *from)
{
  int lfun;
  struct program *p = NULL;

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

#ifdef PIKE_DEBUG
  if (index->type > MAX_TYPE)
    Pike_fatal ("Invalid index type %d.\n", index->type);
#endif

  lfun=ARROW_INDEX_P(index) ? LFUN_ASSIGN_ARROW : LFUN_ASSIGN_INDEX;

  if(FIND_LFUN(p, lfun) != -1)
  {
    push_svalue(index);
    push_svalue(from);
    apply_lfun(o, lfun, 2);
    pop_stack();
  } else {
    object_set_index2(o, index, from);
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

  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
  {
    Pike_error("Cannot assign functions or constants.\n");
  }
  else if(i->run_time_type == T_MIXED)
  {
    struct svalue *s;
    s=(struct svalue *)LOW_GET_GLOBAL(o,f,i);
    if(s->type == type) return & s->u;
  }
  else if(i->run_time_type == type)
  {
    return (union anything *) LOW_GET_GLOBAL(o,f,i);
  }
  return 0;
}


union anything *object_get_item_ptr(struct object *o,
				    struct svalue *index,
				    TYPE_T type)
{

  struct program *p;
  int f;

  debug_malloc_touch(o);
  dmalloc_touch_svalue(index);

  if(!o || !(p=o->prog))
  {
    Pike_error("Lookup in destructed object.\n");
    return 0; /* make gcc happy */
  }


  switch(index->type)
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
    break;

  case T_OBJ_INDEX:
    f=index->u.identifier;
    break;

  default:
#ifdef PIKE_DEBUG
    if (index->type > MAX_TYPE)
      Pike_fatal ("Invalid type %d in index.\n", index->type);
#endif
/*    Pike_error("Lookup on non-string value.\n"); */
    return 0;
  }

  if(f < 0)
  {
    if (index->type == T_STRING && !index->u.string->size_shift &&
	index->u.string->len < 1024)
      Pike_error("No such variable (%s) in object.\n", index->u.string->str);
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

      if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
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

PMOD_EXPORT struct array *object_indices(struct object *o)
{
  struct program *p;
  struct array *a;
  int e;

  p=o->prog;
  if(!p)
    Pike_error("indices() on destructed object.\n");

  if(FIND_LFUN(p,LFUN__INDICES) == -1)
  {
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      copy_shared_string(ITEM(a)[e].u.string,
			 ID_FROM_INT(p,p->identifier_index[e])->name);
      ITEM(a)[e].type=T_STRING;
    }
    a->type_field = BIT_STRING;
  }else{
    apply_lfun(o, LFUN__INDICES, 0);
    if(sp[-1].type != T_ARRAY)
      Pike_error("Bad return type from o->_indices()\n");
    a=sp[-1].u.array;
    sp--;
    dmalloc_touch_svalue(sp);
  }
  return a;
}

PMOD_EXPORT struct array *object_values(struct object *o)
{
  struct program *p;
  struct array *a;
  int e;
  
  p=o->prog;
  if(!p)
    Pike_error("values() on destructed object.\n");

  if(FIND_LFUN(p,LFUN__VALUES)==-1)
  {
    TYPE_FIELD types = 0;
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      low_object_index_no_free(ITEM(a)+e, o, p->identifier_index[e]);
      types |= 1 << ITEM(a)[e].type;
    }
    a->type_field = types;
  }else{
    apply_lfun(o, LFUN__VALUES, 0);
    if(sp[-1].type != T_ARRAY)
      Pike_error("Bad return type from o->_values()\n");
    a=sp[-1].u.array;
    sp--;
    dmalloc_touch_svalue(sp);
  }
  return a;
}


PMOD_EXPORT void gc_mark_object_as_referenced(struct object *o)
{
  debug_malloc_touch(o);
  debug_malloc_touch(o->storage);

  if(gc_mark(o)) {
    if(o->next == o) return; /* Fake object used by compiler */

    GC_ENTER (o, T_OBJECT) {
      int e;
      struct program *p;

      if (o == gc_mark_object_pos)
	gc_mark_object_pos = o->next;
      if (o == gc_internal_object)
	gc_internal_object = o->next;
      else {
	DOUBLEUNLINK(first_object, o);
	DOUBLELINK(first_object, o); /* Linked in first. */
      }

      if(o && (p=o->prog) && PIKE_OBJ_INITED(o)) {
	debug_malloc_touch(p);

	gc_mark_program_as_referenced (p);

	if(o->prog->flags & PROGRAM_USES_PARENT)
	  if(PARENT_INFO(o)->parent)
	    gc_mark_object_as_referenced(PARENT_INFO(o)->parent);

	LOW_PUSH_FRAME(o);

	for(e=p->num_inherits-1; e>=0; e--)
	{
	  int q;
      
	  LOW_SET_FRAME_CONTEXT(p->inherits[e]);

	  for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
	  {
	    int d=pike_frame->context.prog->variable_index[q];
	    struct identifier *id = pike_frame->context.prog->identifiers + d;
	    int id_flags = id->identifier_flags;
	    int rtt = id->run_time_type;

	    if (IDENTIFIER_IS_ALIAS(id_flags)) continue;
	
	    if(rtt == T_MIXED)
	    {
	      struct svalue *s;
	      s=(struct svalue *)(pike_frame->current_storage + id->func.offset);
	      dmalloc_touch_svalue(s);
	      if ((s->type != T_OBJECT && s->type != T_FUNCTION) || s->u.object != o ||
		  !(id_flags & IDENTIFIER_NO_THIS_REF))
		gc_mark_svalues(s, 1);
	    }else{
	      union anything *u;
	      u=(union anything *)(pike_frame->current_storage + id->func.offset);
#ifdef DEBUG_MALLOC
	      if (rtt <= MAX_REF_TYPE) debug_malloc_touch(u->refs);
#endif
	      if (rtt != T_OBJECT || u->object != o ||
		  !(id_flags & IDENTIFIER_NO_THIS_REF))
		gc_mark_short_svalue(u, rtt);
	    }
	  }

	  if(pike_frame->context.prog->event_handler)
	    pike_frame->context.prog->event_handler(PROG_EVENT_GC_RECURSE);

	  LOW_UNSET_FRAME_CONTEXT();
	}

	LOW_POP_FRAME();
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

      LOW_PUSH_FRAME(o);

      for(e=p->num_inherits-1; e>=0; e--)
      {
	int q;
      
	LOW_SET_FRAME_CONTEXT(p->inherits[e]);

	for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
	{
	  int d=pike_frame->context.prog->variable_index[q];
	  struct identifier *id = pike_frame->context.prog->identifiers + d;
	  int id_flags = id->identifier_flags;
	  int rtt = id->run_time_type;
	
	  if (IDENTIFIER_IS_ALIAS(id_flags)) continue;
	
	  if(rtt == T_MIXED)
	  {
	    struct svalue *s;
	    s=(struct svalue *)(pike_frame->current_storage + id->func.offset);
	    dmalloc_touch_svalue(s);
	    if ((s->type != T_OBJECT && s->type != T_FUNCTION) || s->u.object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_cycle_check_svalues(s, 1);
	  }else{
	    union anything *u;
	    u=(union anything *)(pike_frame->current_storage + id->func.offset);
#ifdef DEBUG_MALLOC
	    if (rtt <= MAX_REF_TYPE) debug_malloc_touch(u->refs);
#endif
	    if (rtt != T_OBJECT || u->object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_cycle_check_short_svalue(u, rtt);
	  }
	}

	if(pike_frame->context.prog->event_handler)
	  pike_frame->context.prog->event_handler(PROG_EVENT_GC_RECURSE);

	LOW_UNSET_FRAME_CONTEXT();
      }
    
      LOW_POP_FRAME();

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

static inline void gc_check_object(struct object *o)
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

      LOW_PUSH_FRAME(o);
    
      for(e=p->num_inherits-1; e>=0; e--)
      {
	int q;
	LOW_SET_FRAME_CONTEXT(p->inherits[e]);

	for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
	{
	  int d=pike_frame->context.prog->variable_index[q];
	  struct identifier *id = pike_frame->context.prog->identifiers + d;
	  int id_flags = id->identifier_flags;
	  int rtt = id->run_time_type;
	
	  if (IDENTIFIER_IS_ALIAS(id_flags)) continue;
	
	  if(rtt == T_MIXED)
	  {
	    struct svalue *s;
	    s=(struct svalue *)(pike_frame->current_storage + id->func.offset);
	    dmalloc_touch_svalue(s);
	    if ((s->type != T_OBJECT && s->type != T_FUNCTION) || s->u.object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_check_svalues(s, 1);
	  }else{
	    union anything *u;
	    u=(union anything *)(pike_frame->current_storage + id->func.offset);
#ifdef DEBUG_MALLOC
	    if (rtt <= MAX_REF_TYPE) debug_malloc_touch(u->refs);
#endif
	    if (rtt != T_OBJECT || u->object != o ||
		!(id_flags & IDENTIFIER_NO_THIS_REF))
	      gc_check_short_svalue(u, rtt);
	  }
	}
      
	if(pike_frame->context.prog->event_handler)
	  pike_frame->context.prog->event_handler(PROG_EVENT_GC_CHECK);

	LOW_UNSET_FRAME_CONTEXT();
      }
      LOW_POP_FRAME();
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

  for(o=gc_internal_object; o; o=next)
  {
    if(gc_do_free(o))
    {
      /* Got an extra ref from gc_cycle_pop_object(). */
#ifdef PIKE_DEBUG
      if (o->prog && FIND_LFUN(o->prog, LFUN_DESTROY) != -1 &&
	  !find_destroy_called_mark(o))
	gc_fatal(o,0,"Can't free a live object in gc_free_all_unreferenced_objects().\n");
#endif
      debug_malloc_touch(o);
      destruct(o);

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

void push_magic_index(struct program *type, int inherit_no, int parent_level)
{
  struct external_variable_context loc;
  struct object *magic;

  loc.o=Pike_fp->current_object;
  if(!loc.o) Pike_error("Illegal magic index call.\n");

  loc.parent_identifier=Pike_fp->fun;
  if (loc.o->prog)
    loc.inherit=INHERIT_FROM_INT(loc.o->prog, loc.parent_identifier);
  find_external_context(&loc, parent_level);

  if (!loc.o->prog)
    Pike_error ("Cannot index in destructed parent object.\n");

  magic=low_clone(type);
  add_ref(MAGIC_O2S(magic)->o=loc.o);
  MAGIC_O2S(magic)->inherit = loc.inherit + inherit_no;
#ifdef DEBUG
  if(loc.inherit + inherit_no >= loc.o->prog->inherits + loc.o->prog->num_inherit)
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

/* The type argument to the magic index functions is intentionally
 * undocumented since I'm not sure this API is satisfactory. The
 * argument would be explained as follows. /mast
 *
 * The indexing normally involves the externally accessable
 * identifiers (i.e. those which aren't static or private) in the
 * current class and any inherited classes. If @[type] is 1 then
 * locally accessible identifiers are indexed too. If @[type] is 2
 * then all externally accessible identifiers in the object, i.e. also
 * those in inheriting classes, are indexed. And if @[type] is 3
 * then all locally accessible identifiers in the object, i.e. also
 * those in inheriting classes, are indexed.
 */


/*! @decl mixed ::`->(string index)
 *!
 *! Builtin arrow operator.
 *!
 *! This function indexes the current object with the string @[index].
 *! This is useful when the arrow operator has been overloaded.
 *!
 *! @seealso
 *!   @[::`->=()]
 */
static void f_magic_index(INT32 args)
{
  struct inherit *inherit;
  int type = 0, f;
  struct pike_string *s;
  struct object *o;

  switch (args) {
    default:
    case 2:
      if (sp[-args+1].type != T_INT) SIMPLE_BAD_ARG_ERROR ("::`->", 2, "void|int");
      type = sp[-args+1].u.integer;
      /* FALL THROUGH */
    case 1:
      if (sp[-args].type != T_STRING) SIMPLE_BAD_ARG_ERROR ("::`->", 1, "string");
      s = sp[-args].u.string;
      break;
    case 0:
      SIMPLE_TOO_FEW_ARGS_ERROR ("::`->", 1);
  }

  if(!(o=MAGIC_THIS->o))
    Pike_error("Magic index error\n");

  if(!o->prog)
    Pike_error("::`-> on destructed object.\n");

  switch (type) {
    case 0:
      inherit=MAGIC_THIS->inherit;
      f=find_shared_string_identifier(s,inherit->prog);
      break;
    case 1:
      inherit = MAGIC_THIS->inherit;
      f = really_low_find_shared_string_identifier (s, inherit->prog, SEE_STATIC);
      break;
    case 2:
      inherit = o->prog->inherits + 0;
      f = find_shared_string_identifier (s, inherit->prog);
      break;
    case 3:
      inherit = o->prog->inherits + 0;
      f = really_low_find_shared_string_identifier (s, inherit->prog, SEE_STATIC);
      break;
    default:
      Pike_error("Unknown indexing type: %d.\n", type);
  }

  pop_n_elems(args);
  if(f<0)
  {
    push_int(0);
    sp[-1].subtype=NUMBER_UNDEFINED;
  }else{
    struct svalue sval;
    low_object_index_no_free(&sval,o,f+
			     inherit->identifier_level);
    *sp=sval;
    sp++;
    dmalloc_touch_svalue(Pike_sp-1);
  }
}

/*! @decl void ::`->=(string index, mixed value)
 *!
 *! Builtin arrow set operator.
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
  struct object *o;
  struct svalue *val;
  struct inherit *inherit;

  switch (args) {
    default:
    case 3:
      if (sp[-args+2].type != T_INT) SIMPLE_BAD_ARG_ERROR ("::`->=", 3, "void|int");
      type = sp[-args+2].u.integer;
      /* FALL THROUGH */
    case 2:
      val = sp-args+1;
      if (sp[-args].type != T_STRING) SIMPLE_BAD_ARG_ERROR ("::`->=", 1, "string");
      s = sp[-args].u.string;
      break;
    case 1:
    case 0:
      SIMPLE_TOO_FEW_ARGS_ERROR ("::`->=", 2);
  }

  if(!(o=MAGIC_THIS->o))
    Pike_error("Magic index error\n");

  if(!o->prog)
    Pike_error("::`->= on destructed object.\n");

  switch (type) {
    case 0:
      inherit=MAGIC_THIS->inherit;
      f=find_shared_string_identifier(s,inherit->prog);
      break;
    case 1:
      inherit = MAGIC_THIS->inherit;
      f = really_low_find_shared_string_identifier (s, inherit->prog, SEE_STATIC);
      break;
    case 2:
      inherit = o->prog->inherits + 0;
      f = find_shared_string_identifier (s, inherit->prog);
      break;
    case 3:
      inherit = o->prog->inherits + 0;
      f = really_low_find_shared_string_identifier (s, inherit->prog, SEE_STATIC);
      break;
    default:
      Pike_error("Unknown indexing type.\n");
  }

  if(f<0)
  {
    if (!s->size_shift && s->len < 1024)
      Pike_error("No such variable (%s) in object.\n", s->str);
    else
      Pike_error("No such variable in object.\n");
  }else{
    object_low_set_index(o, f+inherit->identifier_level,
			 val);
    pop_n_elems(args);
    push_int(0);
  }
}

/*! @decl mixed ::_indices()
 *!
 *! Builtin function to list the identifiers of an object.
 *! This is useful when @[lfun::_indices] has been overloaded.
 *!
 *! @seealso
 *!   @[::_values, ::`->]
 */
static void f_magic_indices (INT32 args)
{
  struct object *obj;
  struct program *prog;
  struct array *res;
  int type = 0, e, i;

  if (args >= 1) {
    if (sp[-args].type != T_INT) SIMPLE_BAD_ARG_ERROR ("::_indices", 1, "void|int");
    type = sp[-args].u.integer;
  }

  if (!(obj = MAGIC_THIS->o)) Pike_error ("Magic index error\n");
  if (!obj->prog) Pike_error ("Object is destructed.\n");

  /* FIXME: Both type 0 and 1 have somewhat odd behaviors if there are
   * local identifiers before inherits that are overridden by them
   * (e.g. listing duplicate identifiers). But then again, this is not
   * the only place where that gives strange effects, imho. /mast */

  switch (type) {
    case 0:
      prog = MAGIC_THIS->inherit->prog;
      break;
    case 1:
    case 3:
      if (type == 1) {
	prog = MAGIC_THIS->inherit->prog;
      } else {
	prog = obj->prog;
      }
      pop_n_elems (args);
      push_array (res = allocate_array_no_init (prog->num_identifier_references, 0));
      for (e = i = 0; e < (int) prog->num_identifier_references; e++) {
	struct reference *ref = prog->identifier_references + e;
	struct identifier *id = ID_FROM_PTR (prog, ref);
	if (ref->id_flags & ID_HIDDEN) continue;
	if ((ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	    (ID_INHERITED|ID_PRIVATE)) continue;
	copy_shared_string (ITEM(res)[i].u.string, ID_FROM_PTR (prog, ref)->name);
	ITEM(res)[i++].type = T_STRING;
      }
      res->type_field = BIT_STRING;
      sp[-1].u.array = resize_array (res, i);
      return;
    case 2:
      prog = obj->prog;
      break;
    default:
      Pike_error("Unknown indexing type.\n");
  }

  pop_n_elems (args);
  push_array (res = allocate_array_no_init (prog->num_identifier_index, 0));
  for (e = 0; e < (int) prog->num_identifier_index; e++) {
    copy_shared_string (ITEM(res)[e].u.string,
			ID_FROM_INT (prog, prog->identifier_index[e])->name);
    ITEM(res)[e].type = T_STRING;
  }
  res->type_field = BIT_STRING;
}

/*! @decl mixed ::_values()
 *!
 *! Builtin function to list the values of the identifiers of an
 *! object. This is useful when @[lfun::_values] has been overloaded.
 *!
 *! @seealso
 *!   @[::_indices, ::`->]
 */
static void f_magic_values (INT32 args)
{
  struct object *obj;
  struct program *prog;
  struct inherit *inherit;
  struct array *res;
  TYPE_FIELD types;
  int type = 0, e, i;

  if (args >= 1) {
    if (sp[-args].type != T_INT) SIMPLE_BAD_ARG_ERROR ("::_indices", 1, "void|int");
    type = sp[-args].u.integer;
  }

  if (!(obj = MAGIC_THIS->o)) Pike_error ("Magic index error\n");
  if (!obj->prog) Pike_error ("Object is destructed.\n");

  /* FIXME: Both type 0 and 1 have somewhat odd behaviors if there are
   * local identifiers before inherits that are overridden by them
   * (e.g. listing duplicate identifiers). But then again, this is not
   * the only place where that gives strange effects, imho. /mast */

  switch (type) {
    case 0:
      inherit = MAGIC_THIS->inherit;
      prog = inherit->prog;
      break;
    case 1:
    case 3:
      if (type == 1) {
	inherit = MAGIC_THIS->inherit;
	prog = inherit->prog;
      } else {
	prog = obj->prog;
	inherit = prog->inherits + 0;
      }
      pop_n_elems (args);
      push_array (res = allocate_array_no_init (prog->num_identifier_references, 0));
      types = 0;
      for (e = i = 0; e < (int) prog->num_identifier_references; e++, i++) {
	struct reference *ref = prog->identifier_references + e;
	struct identifier *id = ID_FROM_PTR (prog, ref);
	if (ref->id_flags & ID_HIDDEN) continue;
	if ((ref->id_flags & (ID_INHERITED|ID_PRIVATE)) ==
	    (ID_INHERITED|ID_PRIVATE)) continue;
	low_object_index_no_free (ITEM(res) + i, obj,
				  e + inherit->identifier_level);
	types |= 1 << ITEM(res)[i].type;
      }
      res->type_field = types;
      sp[-1].u.array = resize_array (res, i);
      return;
    case 2:
      prog = obj->prog;
      inherit = prog->inherits + 0;
      break;
    default:
      Pike_error("Unknown indexing type.\n");
  }

  pop_n_elems (args);
  push_array (res = allocate_array_no_init (prog->num_identifier_index, 0));
  types = 0;
  for (e = 0; e < (int) prog->num_identifier_index; e++) {
    low_object_index_no_free (ITEM(res) + e, obj,
			      prog->identifier_index[e] + inherit->identifier_level);
    types |= 1 << ITEM(res)[e].type;
  }
  res->type_field = types;
}

/*! @endnamespace
 */

void init_object(void)
{
  ptrdiff_t offset;

  init_destroy_called_mark_hash();
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  map_variable("__obj","object",ID_STATIC,
	       offset  + OFFSETOF(magic_index_struct, o), T_OBJECT);
  add_function("`()",f_magic_index,"function(string,void|int:mixed)",0);
  magic_index_program=end_program();

  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  map_variable("__obj","object",ID_STATIC,
	       offset  + OFFSETOF(magic_index_struct, o), T_OBJECT);
  add_function("`()",f_magic_set_index,"function(string,mixed,void|int:void)",0);
  magic_set_index_program=end_program();

  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  map_variable("__obj","object",ID_STATIC,
	       offset  + OFFSETOF(magic_index_struct, o), T_OBJECT);
  add_function("`()",f_magic_indices,"function(void|int:array(string))",0);
  magic_indices_program=end_program();

  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  map_variable("__obj","object",ID_STATIC,
	       offset  + OFFSETOF(magic_index_struct, o), T_OBJECT);
  add_function("`()",f_magic_values,"function(void|int:array)",0);
  magic_values_program=end_program();
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
    destruct (master_object);
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
