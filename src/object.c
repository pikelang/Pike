/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: object.c,v 1.41 1998/02/19 21:38:45 hubbe Exp $");
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

struct object *master_object = 0;
struct program *master_program =0;
struct object *first_object;

struct object *low_clone(struct program *p)
{
  int e;
  struct object *o;
  struct frame frame;

  if(!(p->flags & PROGRAM_FINISHED))
    error("Attempting to clone an unfinished program\n");

#ifdef PROFILING
  p->num_clones++;
#endif /* PROFILING */

  GC_ALLOC();

  o=(struct object *)xalloc(sizeof(struct object)-1+p->storage_needed);

  o->prog=p;
  p->refs++;
  o->parent=0;
  o->parent_identifier=0;
  o->next=first_object;
  o->prev=0;
  if(first_object)
    first_object->prev=o;
  first_object=o;
  o->refs=1;
  return o;
}

static void call_c_initializers(struct object *o)
{
  int e;
  struct frame frame;
  struct program *p=o->prog;

  frame.parent_frame=fp;
  frame.current_object=o;
  frame.locals=0;
  frame.fun=-1;
  frame.pc=0;
  fp= & frame;

  frame.current_object->refs++;

  /* clear globals and call C initializers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    int d;

    frame.context=p->inherits[e];
    frame.context.prog->refs++;
    frame.current_storage=o->storage+frame.context.storage_offset;

    for(d=0;d<(int)frame.context.prog->num_identifiers;d++)
    {
      if(!IDENTIFIER_IS_VARIABLE(frame.context.prog->identifiers[d].identifier_flags))
	continue;
      
      if(frame.context.prog->identifiers[d].run_time_type == T_MIXED)
      {
	struct svalue *s;
	s=(struct svalue *)(frame.current_storage +
			    frame.context.prog->identifiers[d].func.offset);
	s->type=T_INT;
	s->u.integer=0;
	s->subtype=0;
      }else{
	union anything *u;
	u=(union anything *)(frame.current_storage +
			     frame.context.prog->identifiers[d].func.offset);
	MEMSET((char *)u,0,sizeof(*u));
      }
    }

    if(frame.context.prog->init)
      frame.context.prog->init(o);

    free_program(frame.context.prog);
  }

  free_object(frame.current_object);
  fp = frame.parent_frame;
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
  parent->refs++;
  o->parent_identifier=parent_identifier;
  call_c_initializers(o);
  call_pike_initializers(o,args);
  UNSET_ONERROR(tmp);
  return o;
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
    INT32 len;
    struct pike_string *s;


    FILE *f=fopen(master_file,"r");
    if(f)
    {
      fseek(f,0,SEEK_END);
      len=ftell(f);
      fseek(f,0,SEEK_SET);
      s=begin_shared_string(len);
      fread(s->str,1,len,f);
      fclose(f);
      push_string(end_shared_string(s));
      push_text(master_file);
      f_cpp(2);
      f_compile(1);
      
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

struct object *master(void)
{
  struct object *o;
  o=get_master();
  if(!o) fatal("Couldn't load master object.\n");
  return o;
}

void destruct(struct object *o)
{
  int e;
  struct frame frame;
  struct program *p;

#ifdef DEBUG
  if(d_flag > 20) do_debug();
#endif

  if(!o || !(p=o->prog)) return; /* Object already destructed */

  o->refs++;

  if(o->parent)
  {
    free_object(o->parent);
    o->parent=0;
  }

  e=FIND_LFUN(o->prog,LFUN_DESTROY);
  if(e != -1)
  {
    safe_apply_low(o, e, 0);
    pop_stack();
  }

  /* destructed in destroy() */
  if(!o->prog)
  {
    free_object(o);
    return;
  }

  o->prog=0;

  frame.parent_frame=fp;
  frame.current_object=o;  /* refs already updated */
  frame.locals=0;
  frame.fun=-1;
  frame.pc=0;
  fp= & frame;

  /* free globals and call C de-initializers */
  for(e=p->num_inherits-1; e>=0; e--)
  {
    int d;

    frame.context=p->inherits[e];
    frame.context.prog->refs++;
    frame.current_storage=o->storage+frame.context.storage_offset;

    if(frame.context.prog->exit)
      frame.context.prog->exit(o);

    for(d=0;d<(int)frame.context.prog->num_identifiers;d++)
    {
      if(!IDENTIFIER_IS_VARIABLE(frame.context.prog->identifiers[d].identifier_flags)) 
	continue;
      
      if(frame.context.prog->identifiers[d].run_time_type == T_MIXED)
      {
	struct svalue *s;
	s=(struct svalue *)(frame.current_storage +
			    frame.context.prog->identifiers[d].func.offset);
	free_svalue(s);
      }else{
	union anything *u;
	u=(union anything *)(frame.current_storage +
			     frame.context.prog->identifiers[d].func.offset);
	free_short_svalue(u, frame.context.prog->identifiers[d].run_time_type);
      }
    }
    free_program(frame.context.prog);
  }

  free_object(frame.current_object);
  fp = frame.parent_frame;

  free_program(p);
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
#ifdef DEBUG
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

    o->refs++; /* Don't free me now! */

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
    o->refs++;
    destruct(o);
    if(--o->refs > 0) return;
  }

  if(o->parent)
  {
    free_object(o->parent);
    o->parent=0;
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

  i=ID_FROM_INT(p, f);

  switch(i->identifier_flags & (IDENTIFIER_FUNCTION | IDENTIFIER_CONSTANT))
  {
  case IDENTIFIER_FUNCTION:
  case IDENTIFIER_C_FUNCTION:
  case IDENTIFIER_PIKE_FUNCTION:
    to->type=T_FUNCTION;
    to->subtype=f;
    to->u.object=o;
    o->refs++;
    break;

  case IDENTIFIER_CONSTANT:
    {
      struct svalue *s;
      s=PROG_FROM_INT(p,f)->constants + i->func.offset;
      if(s->type==T_PROGRAM)
      {
	to->type=T_FUNCTION;
	to->subtype=f;
	to->u.object=o;
	o->refs++;
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
    if(f<0)
      error("No such variable (%s) in object.\n", index->u.string->str);
    break;

  case T_LVALUE:
    f=index->u.integer;
    break;

  default:
    error("Lookup on non-string value.\n");
  }

  if(f < 0)
  {
    error("No such variable (%s) in object.\n", index->u.string->str);
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

  f=ARROW_INDEX_P(index) ? LFUN_ASSIGN_ARROW : LFUN_ASSIGN_INDEX;
  if(FIND_LFUN(p,f) != -1)
    error("Cannot do incremental operations on overloaded index (yet).\n");

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

#ifdef DEBUG
void verify_all_objects(void)
{
  struct object *o;
  struct frame frame;

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

      frame.parent_frame=fp;
      frame.current_object=o;
      frame.locals=0;
      frame.fun=-1;
      frame.pc=0;
      fp= & frame;

      frame.current_object->refs++;

      for(e=0;e<(int)o->prog->num_inherits;e++)
      {
	frame.context=o->prog->inherits[e];
	frame.context.prog->refs++;
	frame.current_storage=o->storage+frame.context.storage_offset;
      }

      free_object(frame.current_object);
      fp = frame.parent_frame;
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
    o->refs++;
    destruct(o);
    next=o->next;
    free_object(o);
  }
  destruct_objects_to_destruct();

  free_object(master_object);
  master_object=0;
  free_program(master_program);
  master_program=0;
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
    struct frame frame;
    struct program *p;

    if(!o || !(p=o->prog)) return; /* Object already destructed */
    o->refs++;

    if(o->parent)
      gc_mark_object_as_referenced(o->parent);

    frame.parent_frame=fp;
    frame.current_object=o;  /* refs already updated */
    frame.locals=0;
    frame.fun=-1;
    frame.pc=0;
    fp= & frame;

    for(e=p->num_inherits-1; e>=0; e--)
    {
      int d;
      
      frame.context=p->inherits[e];
      frame.context.prog->refs++;
      frame.current_storage=o->storage+frame.context.storage_offset;

      if(frame.context.prog->gc_marked)
	frame.context.prog->gc_marked(o);

      for(d=0;d<(int)frame.context.prog->num_identifiers;d++)
      {
	if(!IDENTIFIER_IS_VARIABLE(frame.context.prog->identifiers[d].identifier_flags)) 
	  continue;
	
	if(frame.context.prog->identifiers[d].run_time_type == T_MIXED)
	{
	  struct svalue *s;
	  s=(struct svalue *)(frame.current_storage +
			      frame.context.prog->identifiers[d].func.offset);
	  gc_mark_svalues(s,1);
	}else{
	  union anything *u;
	  u=(union anything *)(frame.current_storage +
			       frame.context.prog->identifiers[d].func.offset);
	  gc_mark_short_svalue(u,frame.context.prog->identifiers[d].run_time_type);
	}
      }
      free_program(frame.context.prog);
    }
    
    free_object(frame.current_object);
    fp = frame.parent_frame;
  }
}

void gc_check_all_objects(void)
{
  struct object *o;

  for(o=first_object;o;o=o->next)
  {
#ifdef DEBUG
    if(o->parent)
      if(debug_gc_check(o->parent,T_OBJECT,o)==-2)
	fprintf(stderr,"(in object at %lx -> parent)\n",(long)o);
#else
    if(o->parent)
      gc_check(o->parent);
#endif
    if(o->prog)
    {
      INT32 e,d;
      
      for(e=0;e<(int)o->prog->num_inherits;e++)
      {
	struct inherit in=o->prog->inherits[e];
	char *base=o->storage + in.storage_offset;

	for(d=0;d<in.prog->num_identifiers;d++)
	{
	  struct identifier *i=in.prog->identifiers+d;
	  
	  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags)) continue;
	  
	  if(i->run_time_type == T_MIXED)
	  {
	    debug_gc_check_svalues((struct svalue *)(base+i->func.offset),1, T_OBJECT, o);
	  }else{
	    debug_gc_check_short_svalue((union anything *)
					(base+i->func.offset),
					i->run_time_type, T_OBJECT,o);
	  }
	}
      }
    }
  }
}

void gc_mark_all_objects(void)
{
  struct object *o;
  for(o=first_object;o;o=o->next)
    if(gc_is_referenced(o))
      gc_mark_object_as_referenced(o);
}

void gc_free_all_unreferenced_objects(void)
{
  struct object *o,*next;

  for(o=first_object;o;o=next)
  {
    if(gc_do_free(o))
    {
      o->refs++;
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
