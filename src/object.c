/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: object.c,v 1.83 1999/09/28 22:00:20 hubbe Exp $");
#include "object.h"
#include "dynamic_buffer.h"
#include "interpret.h"
#include "program.h"
#include "stralloc.h"
#include "svalue.h"
#include "pike_macros.h"
#include "pike_memory.h"
#include "error.h"
#include "main.h"
#include "array.h"
#include "gc.h"
#include "backend.h"
#include "callback.h"
#include "cpp.h"
#include "builtin_functions.h"
#include "cyclic.h"
#include "security.h"
#include "module_support.h"
#include "fdlib.h"
#include "mapping.h"
#include "constants.h"

#include "block_alloc.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include <sys/stat.h>

#include "dmalloc.h"


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
struct object *first_object;

struct object *low_clone(struct program *p)
{
  int e;
  struct object *o;

  if(!(p->flags & PROGRAM_FINISHED))
    error("Attempting to clone an unfinished program\n");

#ifdef PROFILING
  p->num_clones++;
#endif /* PROFILING */

  GC_ALLOC();

  o=(struct object *)xalloc( ((long)(((struct object *)0)->storage))+p->storage_needed);

  o->prog=p;
  add_ref(p);
  o->parent=0;
  o->parent_identifier=0;
  o->next=first_object;
  o->prev=0;
  if(first_object)
    first_object->prev=o;
  first_object=o;
  o->refs=1;
  INITIALIZE_PROT(o);
  return o;
}

#define LOW_PUSH_FRAME(O)	do{		\
  struct pike_frame *pike_frame=alloc_pike_frame();		\
  pike_frame->next=fp;				\
  pike_frame->current_object=o;			\
  pike_frame->locals=0;				\
  pike_frame->num_locals=0;				\
  pike_frame->fun=-1;				\
  pike_frame->pc=0;					\
  pike_frame->context.prog=0;                        \
  pike_frame->context.parent=0;                        \
  fp= pike_frame

#define PUSH_FRAME(O) \
  LOW_PUSH_FRAME(O); \
  add_ref(pike_frame->current_object)

#define SET_FRAME_CONTEXT(X)						\
  if(pike_frame->context.prog) free_program(pike_frame->context.prog);		\
  pike_frame->context=(X);							\
  add_ref(pike_frame->context.prog);						\
  pike_frame->current_storage=o->storage+pike_frame->context.storage_offset;	\
  pike_frame->context.parent=0;
  

#ifdef DEBUG
#define CHECK_FRAME() if(pike_frame != fp) fatal("Frame stack out of whack.\n");
#else
#define CHECK_FRAME()
#endif

#define POP_FRAME()				\
  CHECK_FRAME()					\
  fp=pike_frame->next;				\
  pike_frame->next=0;				\
  free_pike_frame(pike_frame); }while(0)


void call_c_initializers(struct object *o)
{
  int e;
  struct program *p=o->prog;
  PUSH_FRAME(o);

  /* clear globals and call C initializers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    int q;
    SET_FRAME_CONTEXT(p->inherits[e]);

    for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
    {
      int d=pike_frame->context.prog->variable_index[q];
      if(pike_frame->context.prog->identifiers[d].run_time_type == T_MIXED)
      {
	struct svalue *s;
	s=(struct svalue *)(pike_frame->current_storage +
			    pike_frame->context.prog->identifiers[d].func.offset);
	s->type=T_INT;
	s->u.integer=0;
	s->subtype=0;
      }else{
	union anything *u;
	u=(union anything *)(pike_frame->current_storage +
			     pike_frame->context.prog->identifiers[d].func.offset);
	switch(pike_frame->context.prog->identifiers[d].run_time_type)
	{
	  case T_INT: u->integer=0; break;
	  case T_FLOAT: u->float_number=0.0; break;
	  default: u->refs=0; break;
	}
      }
    }

    if(pike_frame->context.prog->init)
      pike_frame->context.prog->init(o);
  }

  POP_FRAME();
}

static void call_pike_initializers(struct object *o, int args)
{
  apply_lfun(o,LFUN___INIT,0);
  pop_stack();
  apply_lfun(o,LFUN_CREATE,args);
  pop_stack();
}

void do_free_object(struct object *o)
{
  free_object(o);
}

struct object *debug_clone_object(struct program *p, int args)
{
  ONERROR tmp;
  struct object *o=low_clone(p);
  SET_ONERROR(tmp, do_free_object, o);
  debug_malloc_touch(o);
  call_c_initializers(o);
  debug_malloc_touch(o);
  call_pike_initializers(o,args);
  UNSET_ONERROR(tmp);
  return o;
}

struct object *parent_clone_object(struct program *p,
				   struct object *parent,
				   int parent_identifier,
				   int args)
{
  ONERROR tmp;
  struct object *o=low_clone(p);
  SET_ONERROR(tmp, do_free_object, o);
  debug_malloc_touch(o);
  o->parent=parent;
  add_ref(parent);
  o->parent_identifier=parent_identifier;
  call_c_initializers(o);
  call_pike_initializers(o,args);
  UNSET_ONERROR(tmp);
  return o;
}

/* FIXME: use open/read/close instead */
static struct pike_string *low_read_file(char *file)
{
  struct pike_string *s;
  INT32 len;
  FD f;
  while((f=fd_open(file,fd_RDONLY,0666)) <0 && errno==EINTR);
  if(f>=0)
  {
    int tmp,pos=0;

    len=fd_lseek(f,0,SEEK_END);
    fd_lseek(f,0,SEEK_SET);
    s=begin_shared_string(len);

    while(pos<len)
    {
      tmp=fd_read(f,s->str+pos,len-pos);
      if(tmp<0)
      {
	if(errno==EINTR) continue;
	fatal("low_read_file(%s) failed, errno=%d\n",file,errno);
      }
      pos+=tmp;
    }
    fd_close(f);
    return end_shared_string(s);
  }
  return 0;
}

struct object *get_master(void)
{
  extern char *master_file;
  struct pike_string *master_name;
  static int inside=0;

  if(master_object && master_object->prog)
    return master_object;

  if(inside) return 0;

  if(master_object)
  {
    free_object(master_object);
    master_object=0;
  }

  inside = 1;

  if(!master_program)
  {
    struct pike_string *s,*s2;
    char *tmp;
    struct stat stat_buf;

    if(!simple_mapping_string_lookup(get_builtin_constants(),
				     "_static_modules"))
    {
      fprintf(stderr,"Cannot load master object yet!\n");
      return 0; /* crash? */
    }

    tmp=xalloc(strlen(master_file)+3);

    MEMCPY(tmp, master_file, strlen(master_file)+1);
    strcat(tmp,".o");

    s = NULL;
    if (!stat(tmp, &stat_buf)) {
      long ts1 = stat_buf.st_mtime;
      long ts2 = 0;		/* FIXME: Should really be MIN_INT, but... */

      if (!stat(master_file, &stat_buf)) {
	ts2 = stat_buf.st_mtime;
      }

      if (ts1 > ts2) {
	s=low_read_file(tmp);
      }
    }
    free(tmp);
    if(s)
    {
      JMP_BUF tmp;
      if(SETJMP(tmp))
      {
#ifdef DEBUG
	if(d_flag)
	  debug_describe_svalue(&throw_value);
#endif
	/* do nothing */
	UNSETJMP(tmp);
      }else{
	extern void f_decode_value(INT32);

	push_string(s);
	push_int(0);
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
    s2=low_read_file(master_file);
    if(s2)
    {
      push_string(s2);
      push_text(master_file);
      f_cpp(2);
      f_compile(1);

    compiled:
      if(sp[-1].type != T_PROGRAM)
      {
	pop_stack();
	return 0;
      }
      master_program=sp[-1].u.program;
      sp--;
    }else{
      error("Couldn't load master program. (%s)\n",master_file);
    }
  }
  master_object=low_clone(master_program);
  debug_malloc_touch(master_object);

  call_c_initializers(master_object);
  call_pike_initializers(master_object,0);
  
  inside = 0;
  return master_object;
}

struct object *debug_master(void)
{
  struct object *o;
  o=get_master();
  if(!o) fatal("Couldn't load master object.\n");
  return o;
}

struct destroy_called_mark
{
  struct destroy_called_mark *next;
  void *data;
};

PTR_HASH_ALLOC(destroy_called_mark,128)

static void call_destroy(struct object *o, int foo)
{
  int e;
  if(!o || !o->prog) return; /* Object already destructed */

  e=FIND_LFUN(o->prog,LFUN_DESTROY);
  if(e != -1)
  {
    if(check_destroy_called_mark_semafore(o))
    {
      /* fprintf(stderr, "destruct(): Calling destroy().\n"); */
      if(foo) push_int(1);
      safe_apply_low(o, e, foo?1:0);
      pop_stack();
    }
  }
}

void low_destruct(struct object *o,int do_free)
{
  int e;
  struct program *p;

#ifdef PIKE_DEBUG
  if(d_flag > 20) do_debug();
#endif

  add_ref(o);

  call_destroy(o,0);
  remove_destroy_called_mark(o);

  /* destructed in destroy() */
  if(!(p=o->prog))
  {
    free_object(o);
    return;
  }

  o->prog=0;

  if(o->parent)
  {
    /* fprintf(stderr, "destruct(): Zapping parent.\n"); */

    free_object(o->parent);
    o->parent=0;
  }

  LOW_PUSH_FRAME(o);

  /* free globals and call C de-initializers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    int q;

    SET_FRAME_CONTEXT(p->inherits[e]);

    if(pike_frame->context.prog->exit)
      pike_frame->context.prog->exit(o);

    if(!do_free) continue;

    for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
    {
      int d=pike_frame->context.prog->variable_index[q];
      
      if(pike_frame->context.prog->identifiers[d].run_time_type == T_MIXED)
      {
	struct svalue *s;
	s=(struct svalue *)(pike_frame->current_storage +
			    pike_frame->context.prog->identifiers[d].func.offset);
	free_svalue(s);
      }else{
	union anything *u;
	u=(union anything *)(pike_frame->current_storage +
			     pike_frame->context.prog->identifiers[d].func.offset);
	free_short_svalue(u, pike_frame->context.prog->identifiers[d].run_time_type);
      }
    }
  }

  POP_FRAME();

  free_program(p);
}

void destruct(struct object *o)
{
  low_destruct(o,1);
}


static struct object *objects_to_destruct = 0;
static struct callback *destruct_object_evaluator_callback =0;

/* This function destructs the objects that are scheduled to be
 * destructed by really_free_object. It links the object back into the
 * list of objects first. Adds a reference, destructs it and then frees it.
 */
void destruct_objects_to_destruct(void)
{
  struct object *o, *next;

  while((o=objects_to_destruct))
  {
#ifdef PIKE_DEBUG
    if(o->refs)
      fatal("Object to be destructed grew extra references.\n");
#endif
    /* Link object back to list of objects */
    objects_to_destruct=o->next;
    
    if(first_object)
      first_object->prev=o;

    o->next=first_object;
    first_object=o;
    o->prev=0;

    add_ref(o); /* Don't free me now! */

    destruct(o);

    free_object(o);
  }
  objects_to_destruct=0;
  if(destruct_object_evaluator_callback)
  {
    remove_callback(destruct_object_evaluator_callback);
    destruct_object_evaluator_callback=0;
  }
}


/* really_free_objects:
 * This function is called when an object runs out of references.
 * It frees the object if it is destructed, otherwise it moves it to
 * a separate list of objects which will be destructed later.
 */

void really_free_object(struct object *o)
{
  if(o->prog && (o->prog->flags & PROGRAM_DESTRUCT_IMMEDIATE))
  {
    add_ref(o);
    destruct(o);
    if(--o->refs > 0) return;
  }

  if(o->prev)
    o->prev->next=o->next;
  else
    first_object=o->next;

  if(o->next) o->next->prev=o->prev;

  if(o->prog)
  {
    if(!destruct_object_evaluator_callback)
    {
      destruct_object_evaluator_callback=
	add_to_callback(&evaluator_callbacks,
			(callback_func)destruct_objects_to_destruct,
			0,0);
    }
    o->next=objects_to_destruct;
    o->prev=0;
    objects_to_destruct=o;
  } else {
    if(o->parent)
    {
      /* fprintf(stderr, "really_free_object(): Zapping parent.\n"); */

      free_object(o->parent);
      o->parent=0;
    }

    FREE_PROT(o);
    free((char *)o);
    GC_FREE();
  }
}


void low_object_index_no_free(struct svalue *to,
			      struct object *o,
			      INT32 f)
{
  struct identifier *i;
  struct program *p=o->prog;
  
  if(!p)
    error("Cannot access global variables in destructed object.\n");

  debug_malloc_touch(o);

  i=ID_FROM_INT(p, f);

  switch(i->identifier_flags & (IDENTIFIER_FUNCTION | IDENTIFIER_CONSTANT))
  {
  case IDENTIFIER_FUNCTION:
  case IDENTIFIER_C_FUNCTION:
  case IDENTIFIER_PIKE_FUNCTION:
    to->type=T_FUNCTION;
    to->subtype=f;
    to->u.object=o;
    add_ref(o);
    break;

  case IDENTIFIER_CONSTANT:
    {
      struct svalue *s;
      s=& PROG_FROM_INT(p,f)->constants[i->func.offset].sval;
      if(s->type==T_PROGRAM)
      {
	to->type=T_FUNCTION;
	to->subtype=f;
	to->u.object=o;
	add_ref(o);
      }else{
	check_destructed(s);
	assign_svalue_no_free(to, s);
      }
      break;
    }

  case 0:
    if(i->run_time_type == T_MIXED)
    {
      struct svalue *s;
      s=(struct svalue *)LOW_GET_GLOBAL(o,f,i);
      check_destructed(s);
      assign_svalue_no_free(to, s);
    }
    else
    {
      union anything *u;
      u=(union anything *)LOW_GET_GLOBAL(o,f,i);
      check_short_destructed(u,i->run_time_type);
      assign_from_short_svalue_no_free(to, u, i->run_time_type);
    }
  }
}

void object_index_no_free2(struct svalue *to,
			  struct object *o,
			  struct svalue *index)
{
  struct program *p;
  int f;

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  switch(index->type)
  {
  case T_STRING:
    f=find_shared_string_identifier(index->u.string, p);
    break;

  case T_LVALUE:
    f=index->u.integer;
    break;

  default:
    error("Lookup on non-string value.\n");
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

void object_index_no_free(struct svalue *to,
			   struct object *o,
			   struct svalue *index)
{
  struct program *p;
  int lfun;

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }
  lfun=ARROW_INDEX_P(index) ? LFUN_ARROW : LFUN_INDEX;

  if(FIND_LFUN(p, lfun) != -1)
  {
    push_svalue(index);
    apply_lfun(o,lfun,1);
    *to=sp[-1];
    sp--;
  } else {
    object_index_no_free2(to,o,index);
  }
}


void object_low_set_index(struct object *o,
			  int f,
			  struct svalue *from)
{
  struct identifier *i;
  struct program *p;

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  debug_malloc_touch(o);
  check_destructed(from);

  i=ID_FROM_INT(p, f);

  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
  {
    error("Cannot assign functions or constants.\n");
  }
  else if(i->run_time_type == T_MIXED)
  {
    assign_svalue((struct svalue *)LOW_GET_GLOBAL(o,f,i),from);
  }
  else
  {
    assign_to_short_svalue((union anything *) 
			   LOW_GET_GLOBAL(o,f,i),
			   i->run_time_type,
			   from);
  }
}

void object_set_index2(struct object *o,
		      struct svalue *index,
		      struct svalue *from)
{
  struct program *p;
  int f;

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  switch(index->type)
  {
  case T_STRING:
    f=find_shared_string_identifier(index->u.string, p);
    if(f<0) {
      if (index->u.string->len < 1024) {
	error("No such variable (%s) in object.\n", index->u.string->str);
      } else {
	error("No such variable in object.\n");
      }
    }
    break;

  case T_LVALUE:
    f=index->u.integer;
    break;

  default:
    error("Lookup on non-string value.\n");
  }

  if(f < 0)
  {
    if (index->u.string->len < 1024) {
      error("No such variable (%s) in object.\n", index->u.string->str);
    } else {
      error("No such variable in object.\n");
    }
  }else{
    object_low_set_index(o, f, from);
  }
}

void object_set_index(struct object *o,
		       struct svalue *index,
		       struct svalue *from)
{
  struct program *p;
  int lfun;

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return; /* make gcc happy */
  }

  lfun=ARROW_INDEX_P(index) ? LFUN_ASSIGN_ARROW : LFUN_ASSIGN_INDEX;

  if(FIND_LFUN(p,lfun) != -1)
  {
    push_svalue(index);
    push_svalue(from);
    apply_lfun(o,lfun,2);
    pop_stack();
  } else {
    object_set_index2(o,index,from);
  }
}

static union anything *object_low_get_item_ptr(struct object *o,
					       int f,
					       TYPE_T type)
{
  struct identifier *i;
  struct program *p;

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return 0; /* make gcc happy */
  }

  i=ID_FROM_INT(p, f);

  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
  {
    error("Cannot assign functions or constants.\n");
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

  if(!o || !(p=o->prog))
  {
    error("Lookup in destructed object.\n");
    return 0; /* make gcc happy */
  }


  switch(index->type)
  {
  case T_STRING:
    f=ARROW_INDEX_P(index) ? LFUN_ASSIGN_ARROW : LFUN_ASSIGN_INDEX;

    if(FIND_LFUN(p,f) != -1)
    {
      return 0;
      
      /* error("Cannot do incremental operations on overloaded index (yet).\n");
       */
    }
    
    f=find_shared_string_identifier(index->u.string, p);
    break;

  case T_LVALUE:
    f=index->u.integer;
    break;

  default:
/*    error("Lookup on non-string value.\n"); */
    return 0;
  }

  if(f < 0)
  {
    error("No such variable in object.\n");
  }else{
    return object_low_get_item_ptr(o, f, type);
  }
  return 0;
}

#ifdef PIKE_DEBUG
void verify_all_objects(void)
{
  struct object *o;

  for(o=first_object;o;o=o->next)
  {
    if(o->next && o->next->prev !=o)
      fatal("Object check: o->next->prev != o\n");

    if(o->prev)
    {
      if(o->prev->next != o)
	fatal("Object check: o->prev->next != o\n");

      if(o == first_object)
	fatal("Object check: o->prev !=0 && first_object == o\n");
    } else {
      if(first_object != o)
	fatal("Object check: o->prev ==0 && first_object != o\n");
    }

    if(o->refs <= 0)
      fatal("Object refs <= zero.\n");

    if(o->prog)
    {
      extern struct program *first_program;
      struct program *p;
      int e;

      for(p=first_program;p!=o->prog;p=p->next)
	if(!p)
	  fatal("Object's program not in program list.\n");

      for(e=0;e<(int)o->prog->num_identifiers;e++)
      {
	struct identifier *i;
	i=ID_FROM_INT(o->prog, e);
	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  continue;

	if(i->run_time_type == T_MIXED)
	{
	  check_svalue((struct svalue *)LOW_GET_GLOBAL(o,e,i));
	}else{
	  check_short_svalue((union anything *)LOW_GET_GLOBAL(o,e,i),
			     i->run_time_type);
	}
      }

#if 0
      PUSH_FRAME(o);

      for(e=0;e<(int)o->prog->num_inherits;e++)
      {
	SET_FRAME_CONTEXT(o->prog->inherits[e]);
	/* Do pike_frame stuff here */

	free_program(pike_frame->context.prog);
      }

      POP_FRAME();
#endif
    }
  }

  for(o=objects_to_destruct;o;o=o->next)
    if(o->refs)
      fatal("Object to be destructed has references.\n");

}
#endif

int object_equal_p(struct object *a, struct object *b, struct processing *p)
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


  if(a->prog)
  {
    int e;
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

void cleanup_objects(void)
{
  struct object *o, *next;

  for(o=first_object;o;o=next)
  {
    add_ref(o);
    low_destruct(o,1);
    next=o->next;
    free_object(o);
  }
  free_object(master_object);
  master_object=0;
  free_program(master_program);
  master_program=0;
  destruct_objects_to_destruct();
}

struct array *object_indices(struct object *o)
{
  struct program *p;
  struct array *a;
  int e;

  p=o->prog;
  if(!p)
    error("indices() on destructed object.\n");

  if(FIND_LFUN(p,LFUN__INDICES) == -1)
  {
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      copy_shared_string(ITEM(a)[e].u.string,
			 ID_FROM_INT(p,p->identifier_index[e])->name);
      ITEM(a)[e].type=T_STRING;
    }
  }else{
    apply_lfun(o, LFUN__INDICES, 0);
    if(sp[-1].type != T_ARRAY)
      error("Bad return type from o->_indices()\n");
    a=sp[-1].u.array;
    sp--;
  }
  return a;
}

struct array *object_values(struct object *o)
{
  struct program *p;
  struct array *a;
  int e;
  
  p=o->prog;
  if(!p)
    error("values() on destructed object.\n");

  if(FIND_LFUN(p,LFUN__VALUES)==-1)
  {
    a=allocate_array_no_init(p->num_identifier_index,0);
    for(e=0;e<(int)p->num_identifier_index;e++)
    {
      low_object_index_no_free(ITEM(a)+e, o, p->identifier_index[e]);
    }
  }else{
    apply_lfun(o, LFUN__VALUES, 0);
    if(sp[-1].type != T_ARRAY)
      error("Bad return type from o->_values()\n");
    a=sp[-1].u.array;
    sp--;
  }
  return a;
}


void gc_mark_object_as_referenced(struct object *o)
{
  if(gc_mark(o))
  {
    int e;
    struct program *p;

    if(!o || !(p=o->prog)) return; /* Object already destructed */
    add_ref(o);

    if(o->parent)
      gc_mark_object_as_referenced(o->parent);

    LOW_PUSH_FRAME(o);

    for(e=p->num_inherits-1; e>=0; e--)
    {
      int q;
      
      SET_FRAME_CONTEXT(p->inherits[e]);

      if(pike_frame->context.prog->gc_marked)
	pike_frame->context.prog->gc_marked(o);

      for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
      {
	int d=pike_frame->context.prog->variable_index[q];
	
	if(pike_frame->context.prog->identifiers[d].run_time_type == T_MIXED)
	{
	  struct svalue *s;
	  s=(struct svalue *)(pike_frame->current_storage +
			      pike_frame->context.prog->identifiers[d].func.offset);
	  gc_mark_svalues(s,1);
	}else{
	  union anything *u;
	  u=(union anything *)(pike_frame->current_storage +
			       pike_frame->context.prog->identifiers[d].func.offset);
	  gc_mark_short_svalue(u,pike_frame->context.prog->identifiers[d].run_time_type);
	}
      }
    }
    
    POP_FRAME();
  }
}

void gc_check_all_objects(void)
{
  struct object *o,*next;

  for(o=first_object;o;o=next)
  {
    int e;
    struct program *p;
    add_ref(o);

#ifdef PIKE_DEBUG
    if(o->parent)
      if(debug_gc_check(o->parent,T_OBJECT,o)==-2)
	fprintf(stderr,"(in object at %lx -> parent)\n",(long)o);
#else
    if(o->parent)
      gc_check(o->parent);
#endif
    if((p=o->prog))
    {
      PUSH_FRAME(o);
      
      for(e=p->num_inherits-1; e>=0; e--)
      {
	int q;
	SET_FRAME_CONTEXT(p->inherits[e]);
	
	if(pike_frame->context.prog->gc_check)
	  pike_frame->context.prog->gc_check(o);

	for(q=0;q<(int)pike_frame->context.prog->num_variable_index;q++)
	{
	  int d=pike_frame->context.prog->variable_index[q];
	  
	  if(pike_frame->context.prog->identifiers[d].run_time_type == T_MIXED)
	  {
	    struct svalue *s;
	    s=(struct svalue *)(pike_frame->current_storage +
				pike_frame->context.prog->identifiers[d].func.offset);
	    debug_gc_check_svalues(s,1,T_OBJECT,o);
	  }else{
	    union anything *u;
	    u=(union anything *)(pike_frame->current_storage +
				 pike_frame->context.prog->identifiers[d].func.offset);
	    debug_gc_check_short_svalue(u,pike_frame->context.prog->identifiers[d].run_time_type,T_OBJECT,o);
	  }
	}
      }
      POP_FRAME();
    }
    next=o->next;
    free_object(o);
  }
}

void gc_mark_all_objects(void)
{
  struct object *o,*next;
  for(o=first_object;o;o=next)
  {
    if(gc_is_referenced(o))
    {
      add_ref(o);
      gc_mark_object_as_referenced(o);
      next=o->next;
      free_object(o);
    }else{
      next=o->next;
    }
  }
}

void gc_free_all_unreferenced_objects(void)
{
  struct object *o,*next;

  for(o=first_object;o;o=next)
  {
    if(gc_do_free(o))
    {
      add_ref(o);
      call_destroy(o,0);
      next=o->next;
      free_object(o);
    }else{
      next=o->next;
    }
  }

  for(o=first_object;o;o=next)
  {
    if(gc_do_free(o))
    {
      add_ref(o);
      destruct(o);
      next=o->next;
      free_object(o);
    }else{
      next=o->next;
    }
  }
}

void count_memory_in_objects(INT32 *num_, INT32 *size_)
{
  INT32 num=0, size=0;
  struct object *o;
  for(o=first_object;o;o=o->next)
  {
    num++;
    if(o->prog)
    {
      size+=sizeof(struct object)-1+o->prog->storage_needed;
    }else{
      size+=sizeof(struct object);
    }
  }
  for(o=objects_to_destruct;o;o=o->next)
  {
    num++;
    if(o->prog)
    {
      size+=sizeof(struct object)-1+o->prog->storage_needed;
    }else{
      size+=sizeof(struct object);
    }
  }
  *num_=num;
  *size_=size;
}

struct magic_index_struct
{
  struct inherit *inherit;
  struct object *o;
};

#define MAGIC_THIS ((struct magic_index_struct *)(fp->current_storage))
#define MAGIC_O2S(o) ((struct magic_index_struct *)&(o->storage))

struct program *magic_index_program=0;
struct program *magic_set_index_program=0;

void push_magic_index(struct program *type, int inherit_no, int parent_level)
{
  struct inherit *inherit;
  struct object *o,*magic;
  struct program *p;

  o=fp->current_object;
  if(!o) error("Illegal magic index call.\n");
  
  inherit=INHERIT_FROM_INT(fp->context.prog, fp->fun);

  while(parent_level--)
  {
    int i;
    if(inherit->parent_offset)
    {
      i=o->parent_identifier;
      o=o->parent;
      parent_level+=inherit->parent_offset-1;
    }else{
      i=inherit->parent_identifier;
      o=inherit->parent;
    }
    
    if(!o)
      error("Parent was lost!\n");
    
    if(!(p=o->prog))
      error("Attempting to access variable in destructed object\n");
    
    inherit=INHERIT_FROM_INT(p, i);
  }


  magic=low_clone(type);
  add_ref(MAGIC_O2S(magic)->o=o);
  MAGIC_O2S(magic)->inherit = inherit + inherit_no;
#ifdef DEBUG
  if(inherit + inherit_no >= o->prog->inherits + o->prog->num_inherit)
     fatal("Magic index blahonga!\n");
#endif
  push_object(magic);
}

static void f_magic_index(INT32 args)
{
  struct inherit *inherit;
  int f;
  struct pike_string *s;
  struct object *o;

  get_all_args("::`->",args,"%S",&s);

  if(!(o=MAGIC_THIS->o))
    error("Magic index error\n");

  if(!o->prog)
    error("Magic index on destructed object!\n");

  inherit=MAGIC_THIS->inherit;

  f=find_shared_string_identifier(s,inherit->prog);

  if(f<0)
  {
    pop_n_elems(args);
    push_int(0);
    sp[-1].subtype=NUMBER_UNDEFINED;
  }else{
    struct svalue sval;
    low_object_index_no_free(&sval,o,f+
			     inherit->identifier_level);
    pop_stack();
    *sp=sval;
    sp++;
  }
}

static void f_magic_set_index(INT32 args)
{
  int f;
  struct pike_string *s;
  struct object *o;
  struct svalue *val;
  struct inherit *inherit;

  get_all_args("::`->=",args,"%S%*",&s,&val);

  if(!(o=MAGIC_THIS->o))
    error("Magic index error\n");

  if(!o->prog)
    error("Magic index on destructed object!\n");

  inherit=MAGIC_THIS->inherit;

  f=find_shared_string_identifier(s,inherit->prog);

  if(f<0)
  {
    error("No such variable in object.\n");
  }else{
    object_low_set_index(o, f+inherit->identifier_level,
			 val);
    pop_n_elems(args);
    push_int(0);
  }
}

void init_object(void)
{
  int offset;

  init_destroy_called_mark_hash();
  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  map_variable("__obj","object",ID_STATIC,
	       offset  + OFFSETOF(magic_index_struct, o), T_OBJECT);
  add_function("`()",f_magic_index,"function(string:mixed)",0);
  magic_index_program=end_program();

  start_new_program();
  offset=ADD_STORAGE(struct magic_index_struct);
  map_variable("__obj","object",ID_STATIC,
	       offset  + OFFSETOF(magic_index_struct, o), T_OBJECT);
  add_function("`()",f_magic_set_index,"function(string,mixed:void)",0);
  magic_set_index_program=end_program();
}

void exit_object(void)
{
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
}

#ifdef PIKE_DEBUG
void check_object_context(struct object *o,
			  struct program *context_prog,
			  char *current_storage)
{
  int q;
  if(o == fake_object) return;
  if( ! o->prog ) return; /* Variables are already freed */

  for(q=0;q<(int)context_prog->num_variable_index;q++)
  {
    int d=context_prog->variable_index[q];
    if(d<0 || d>=context_prog->num_identifiers)
      fatal("Illegal index in variable_index!\n");

    if(context_prog->identifiers[d].run_time_type == T_MIXED)
    {
      struct svalue *s;
      s=(struct svalue *)(current_storage +
			  context_prog->identifiers[d].func.offset);
      check_svalue(s);
    }else{
      union anything *u;
      u=(union anything *)(current_storage +
			   context_prog->identifiers[d].func.offset);
      check_short_svalue(u, 
			 context_prog->identifiers[d].run_time_type);
    }
  }
}

void check_object(struct object *o)
{
  int e;
  struct program *p;
  debug_malloc_touch(o);

  if(o == fake_object) return;
  if(!(p=o->prog)) return;

  /* clear globals and call C initializers */
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
    next=o->next;
    free_object(o);
  }
}

#endif
