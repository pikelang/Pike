/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: program.c,v 1.111 2004/09/27 15:11:51 grubba Exp $");
#include "program.h"
#include "object.h"
#include "dynamic_buffer.h"
#include "pike_types.h"
#include "stralloc.h"
#include "las.h"
#include "language.h"
#include "lex.h"
#include "pike_macros.h"
#include "fsort.h"
#include "error.h"
#include "docode.h"
#include "interpret.h"
#include "hashtable.h"
#include "main.h"
#include "gc.h"
#include "threads.h"
#include "constants.h"
#include "operators.h"
#include "builtin_functions.h"
#include "stuff.h"
#include "mapping.h"
#include "cyclic.h"

#include <errno.h>
#include <fcntl.h>


#undef ATTRIBUTE
#define ATTRIBUTE(X)


/*
 * Define the size of the cache that is used for method lookup.
 */
#define FIND_FUNCTION_HASHSIZE 4711


#define STRUCT
#include "compilation.h"
#undef STRUCT

#define DECLARE
#include "compilation.h"
#undef DECLARE


char *lfun_names[] = {
  "__INIT",
  "create",
  "destroy",
  "`+",
  "`-",
  "`&",
  "`|",
  "`^",
  "`<<",
  "`>>",
  "`*",
  "`/",
  "`%",
  "`~",
  "`==",
  "`<",
  "`>",
  "__hash",
  "cast",
  "`!",
  "`[]",
  "`[]=",
  "`->",
  "`->=",
  "_sizeof",
  "_indices",
  "_values",
  "`()",
  "``+",
  "``-",
  "``&",
  "``|",
  "``^",
  "``<<",
  "``>>",
  "``*",
  "``/",
  "``%",
};

struct program *first_program = 0;
static int current_program_id=0;

struct program *new_program=0;
struct object *fake_object=0;
struct program *malloc_size_program=0;

int compiler_pass;
int compilation_depth;
long local_class_counter;
int catch_level;
struct compiler_frame *compiler_frame=0;
static INT32 last_line = 0;
static INT32 last_pc = 0;
static struct pike_string *last_file = 0;
dynamic_buffer used_modules;
INT32 num_used_modules;
static struct mapping *resolve_cache=0;
static struct mapping *module_index_cache=0;

/* So what if we don't have templates? / Hubbe */

#ifdef PIKE_DEBUG
#define CHECK_FOO(NUMTYPE,TYPE,NAME)				\
  if(malloc_size_program-> PIKE_CONCAT(num_,NAME) < new_program-> PIKE_CONCAT(num_,NAME))	\
    fatal("new_program->num_" #NAME " is out of order\n");	\
  if(new_program->flags & PROGRAM_OPTIMIZED)			\
    fatal("Tried to reallocate fixed program.\n")

#else
#define CHECK_FOO(NUMTYPE,TYPE,NAME)
#endif

#define FOO(NUMTYPE,TYPE,NAME)						\
void PIKE_CONCAT(add_to_,NAME) (TYPE ARG) {				\
  CHECK_FOO(NUMTYPE,TYPE,NAME);						\
  if(malloc_size_program->PIKE_CONCAT(num_,NAME) ==			\
     new_program->PIKE_CONCAT(num_,NAME)) {				\
    void *tmp;								\
    malloc_size_program->PIKE_CONCAT(num_,NAME) *= 2;			\
    malloc_size_program->PIKE_CONCAT(num_,NAME)++;			\
    tmp=realloc((char *)new_program->NAME,				\
                sizeof(TYPE) *						\
		malloc_size_program->PIKE_CONCAT(num_,NAME));		\
    if(!tmp) fatal("Out of memory.\n");					\
    new_program->NAME=tmp;						\
  }									\
  new_program->NAME[new_program->PIKE_CONCAT(num_,NAME)++]=(ARG);	\
}

#include "program_areas.h"

#define FOO(NUMTYPE,TYPE,NAME)							\
void PIKE_CONCAT(low_add_to_,NAME) (struct program_state *state,		\
                                    TYPE ARG) {					\
  if(state->malloc_size_program->PIKE_CONCAT(num_,NAME) ==			\
     state->new_program->PIKE_CONCAT(num_,NAME)) {				\
    void *tmp;									\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME) *= 2;			\
    state->malloc_size_program->PIKE_CONCAT(num_,NAME)++;			\
    tmp=realloc((char *)state->new_program->NAME,				\
                sizeof(TYPE) *							\
		state->malloc_size_program->PIKE_CONCAT(num_,NAME));		\
    if(!tmp) fatal("Out of memory.\n");						\
    state->new_program->NAME=tmp;						\
  }										\
  state->new_program->NAME[state->new_program->PIKE_CONCAT(num_,NAME)++]=(ARG);	\
}

#include "program_areas.h"


void ins_int(INT32 i, void (*func)(char tmp))
{
  int e;
  for(e=0;e<(long)sizeof(i);e++) func(EXTRACT_UCHAR(((char *)&i)+e));
}

void ins_short(INT16 i, void (*func)(char tmp))
{
  int e;
  for(e=0;e<(long)sizeof(i);e++) func(EXTRACT_UCHAR(((char *)&i)+e));
}

void use_module(struct svalue *s)
{
  if( (1<<s->type) & (BIT_MAPPING | BIT_OBJECT | BIT_PROGRAM))
  {
    num_used_modules++;
    assign_svalue_no_free((struct svalue *)
			  low_make_buf_space(sizeof(struct svalue),
					     &used_modules), s);
    if(module_index_cache)
    {
      free_mapping(module_index_cache);
      module_index_cache=0;
    }
  }else{
    yyerror("Module is neither mapping nor object");
  }
}

void unuse_modules(INT32 howmany)
{
  if(!howmany) return;
#ifdef PIKE_DEBUG
  if(howmany *sizeof(struct svalue) > used_modules.s.len)
    fatal("Unusing too many modules.\n");
#endif
  num_used_modules-=howmany;
  low_make_buf_space(-sizeof(struct svalue)*howmany, &used_modules);
  free_svalues((struct svalue *)low_make_buf_space(0, &used_modules),
	       howmany,
	       BIT_MAPPING | BIT_OBJECT | BIT_PROGRAM);
  if(module_index_cache)
  {
    free_mapping(module_index_cache);
    module_index_cache=0;
  }
}

int low_find_shared_string_identifier(struct pike_string *name,
				      struct program *prog);



static struct node_s *index_modules(struct pike_string *ident)
{
  struct node_s *ret;
  JMP_BUF tmp;

  if(module_index_cache)
  {
    struct svalue *tmp=low_mapping_string_lookup(module_index_cache,ident);
    if(tmp)
    {
      if(!(IS_ZERO(tmp) && tmp->subtype==1))
	return mksvaluenode(tmp);
      return 0;
    }
  }


  if(SETJMP(tmp))
  {
    ONERROR tmp;
    SET_ONERROR(tmp,exit_on_error,"Error in handle_error in master object!");
    assign_svalue_no_free(sp++, & throw_value);
    APPLY_MASTER("handle_error", 1);
    pop_stack();
    UNSET_ONERROR(tmp);
    yyerror("Couldn't index module.");
  }else{
    struct svalue *modules=(struct svalue *)used_modules.s.str;
    int e=used_modules.s.len / sizeof(struct svalue);

    while(--e>=0)
    {
      push_svalue(modules+e);
      ref_push_string(ident);
      f_index(2);
      
      if(!IS_UNDEFINED(sp-1))
      {
	UNSETJMP(tmp);
	if(!module_index_cache)
	  module_index_cache=allocate_mapping(10);
	mapping_string_insert(module_index_cache, ident, sp-1);
	ret=mksvaluenode(sp-1);
	pop_stack();
	return ret;
      }
      pop_stack();
    }
  }
  UNSETJMP(tmp);

  return 0;
}

struct node_s *find_module_identifier(struct pike_string *ident)
{
  struct node_s *ret;

  if((ret=index_modules(ident))) return ret;

     
  {
    struct program_state *p=previous_program_state;
    int n;
    for(n=0;n<compilation_depth;n++,p=p->previous)
    {
      int i=low_find_shared_string_identifier(ident, p->new_program);
      if(i!=-1)
      {
	struct identifier *id;
	id=ID_FROM_INT(p->new_program, i);
	return mkexternalnode(n, i, id);
      }
    }
  }


  if(resolve_cache)
  {
    struct svalue *tmp=low_mapping_string_lookup(resolve_cache,ident);
    if(tmp)
    {
      if(!(IS_ZERO(tmp) && tmp->subtype==1))
	return mkconstantsvaluenode(tmp);

      return 0;
    }
  }

  if(!num_parse_error && get_master())
  {	
    DECLARE_CYCLIC();
    node *ret=0;
    if(BEGIN_CYCLIC(ident, lex.current_file))
    {
      my_yyerror("Recursive module dependency in %s.",
		 ident->str);
    }else{
      SET_CYCLIC_RET(1);

      ref_push_string(ident);
      ref_push_string(lex.current_file);
      SAFE_APPLY_MASTER("resolv", 2);

      if(throw_value.type == T_STRING)
      {
	if(compiler_pass==2)
	  my_yyerror("%s",throw_value.u.string->str);
      }
      else
      {
	if(!resolve_cache)
	  resolve_cache=allocate_mapping(10);
	mapping_string_insert(resolve_cache,ident,sp-1);
	
	if(!(IS_ZERO(sp-1) && sp[-1].subtype==1))
	{
	  ret=mkconstantsvaluenode(sp-1);
	}
      }
      pop_stack();
      END_CYCLIC();
    }
    if(ret) return ret;
  }
  
  return 0;
}

struct program *parent_compilation(int level)
{
  int n;
  struct program_state *p=previous_program_state;
  for(n=0;n<level;n++)
  {
    if(n>=compilation_depth) return 0;
    p=p->previous;
    if(!p) return 0;
  }
  return p->new_program;
}

#define ID_TO_PROGRAM_CACHE_SIZE 512
struct program *id_to_program_cache[ID_TO_PROGRAM_CACHE_SIZE];

struct program *id_to_program(INT32 id)
{
  struct program *p;
  INT32 h;
  if(!id) return 0;
  h=id & (ID_TO_PROGRAM_CACHE_SIZE-1);

  if((p=id_to_program_cache[h]))
    if(p->id==id)
      return p;
  
  if(id) 
    {
      for(p=first_program;p;p=p->next)
	{
	  if(id==p->id)
	    {
	      id_to_program_cache[h]=p;
	      return p;
	    }
	}
    }
  return 0;
}

/* Here starts routines which are used to build new programs */

/* Re-allocate all the memory in the program in one chunk. because:
 * 1) The individual blocks are munch bigger than they need to be
 * 2) cuts down on malloc overhead (maybe)
 * 3) localizes memory access (decreases paging)
 */
void optimize_program(struct program *p)
{
  SIZE_T size=0;
  char *data;

  /* Already done (shouldn't happen, but who knows?) */
  if(p->flags & PROGRAM_OPTIMIZED) return;

#define FOO(NUMTYPE,TYPE,NAME) \
  size=DO_ALIGN(size, ALIGNOF(TYPE)); \
  size+=p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0]);
#include "program_areas.h"

  data=malloc(size);
  if(!data) return; /* We are out of memory, but we don't care! */

  size=0;

#define FOO(NUMTYPE,TYPE,NAME) \
  size=DO_ALIGN(size, ALIGNOF(TYPE)); \
  MEMCPY(data+size,p->NAME,p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0])); \
  free((char *)p->NAME); \
  p->NAME=(TYPE *)(data+size); \
  size+=p->PIKE_CONCAT(num_,NAME)*sizeof(p->NAME[0]);
#include "program_areas.h"

  p->total_size=size + sizeof(struct program);

  p->flags |= PROGRAM_OPTIMIZED;
}

/* internal function to make the index-table */
int program_function_index_compare(const void *a,const void *b)
{
  return
    my_order_strcmp(ID_FROM_INT(new_program, *(unsigned short *)a)->name,
		    ID_FROM_INT(new_program, *(unsigned short *)b)->name);
}

void fixate_program(void)
{
  INT32 i,e,t;
  if(new_program->flags & PROGRAM_FIXED) return;
#ifdef PIKE_DEBUG
  if(new_program->flags & PROGRAM_OPTIMIZED)
    fatal("Cannot fixate optimized program\n");
#endif

  /* Ok, sort for binsearch */
  for(e=i=0;i<(int)new_program->num_identifier_references;i++)
  {
    struct reference *funp;
    struct identifier *fun;
    funp=new_program->identifier_references+i;
    if(funp->id_flags & (ID_HIDDEN|ID_STATIC)) continue;
    if(funp->id_flags & ID_INHERITED)
    {
      if(funp->id_flags & ID_PRIVATE) continue;
      fun=ID_FROM_PTR(new_program, funp);
/*	  if(fun->func.offset == -1) continue; * prototype */
      
      /* check for multiple definitions */
      for(t=i+1;t>=0 && t<(int)new_program->num_identifier_references;t++)
      {
	struct reference *funpb;
	struct identifier *funb;
	
	funpb=new_program->identifier_references+t;
	if(funpb->id_flags & (ID_HIDDEN|ID_STATIC)) continue;
	funb=ID_FROM_PTR(new_program,funpb);
	/* if(funb->func.offset == -1) continue; * prototype */
	if(fun->name==funb->name) t=-10;
      }
      if(t<0) continue;
    }
    add_to_identifier_index(i);
  }
  fsort((void *)new_program->identifier_index,
	new_program->num_identifier_index,
	sizeof(unsigned short),(fsortfun)program_function_index_compare);
  
  
  for(i=0;i<NUM_LFUNS;i++)
    new_program->lfuns[i]=find_identifier(lfun_names[i],new_program);
  
  new_program->flags |= PROGRAM_FIXED;
}

struct program *low_allocate_program(void)
{
  struct program *p;
  p=ALLOC_STRUCT(program);
  MEMSET(p, 0, sizeof(struct program));
  
  GC_ALLOC();
  p->refs=1;
  p->id=++current_program_id;
  
  if((p->next=first_program)) first_program->prev=p;
  first_program=p;
  GETTIMEOFDAY(& p->timestamp);
  return p;
}

/*
 * Start building a new program
 */
void low_start_new_program(struct program *p,
			   struct pike_string *name,
			   int flags)
{
  int e,id=0;

  /* We don't want to change thread, but we don't want to
   * wait for the other threads to complete.
   */
  low_init_threads_disable();

  compilation_depth++;

  /* fprintf(stderr, "low_start_new_program(): compilation_depth:%d\n", compilation_depth); */

  if(!p)
  {
    p=low_allocate_program();
  }else{
    add_ref(p);
  }

  if(name)
  {
    struct svalue s;
    s.type=T_PROGRAM;
    s.u.program=p;
    id=add_constant(name, &s, flags);
  }

  init_type_stack();

#define PUSH
#include "compilation.h"
#undef PUSH

  num_used_modules=0;

  if(p && (p->flags & PROGRAM_FINISHED))
  {
    yyerror("Pass2: Program already done");
    p=0;
  }

  malloc_size_program = ALLOC_STRUCT(program);
#ifdef PIKE_DEBUG
  fake_object=(struct object *)xalloc(sizeof(struct object) + 256*sizeof(struct svalue));
  /* Stipple to find illegal accesses */
  MEMSET(fake_object,0x55,sizeof(struct object) + 256*sizeof(struct svalue));
#else
  fake_object=ALLOC_STRUCT(object);
#endif
  GC_ALLOC();

  fake_object->next=fake_object;
  fake_object->prev=fake_object;
  fake_object->refs=1;
  fake_object->parent=0;
  fake_object->parent_identifier=0;
  fake_object->prog=p;
  add_ref(p);

  if(name)
  {
    if((fake_object->parent=previous_program_state->fake_object))
      add_ref(fake_object->parent);
    fake_object->parent_identifier=id;
  }

  new_program=p;

  if(new_program->program)
  {
#define FOO(NUMTYPE,TYPE,NAME) \
    malloc_size_program->PIKE_CONCAT(num_,NAME)=new_program->PIKE_CONCAT(num_,NAME);
#include "program_areas.h"
  }else{
    static struct pike_string *s;
    struct inherit i;

#define START_SIZE 64
#define FOO(NUMTYPE,TYPE,NAME) \
    malloc_size_program->PIKE_CONCAT(num_,NAME)=START_SIZE; \
    new_program->NAME=(TYPE *)xalloc(sizeof(TYPE) * START_SIZE);
#include "program_areas.h"

    i.prog=new_program;
    i.identifier_level=0;
    i.storage_offset=0;
    i.inherit_level=0;
    i.parent=0;
    i.parent_identifier=0;
    i.parent_offset=1;
    i.name=0;
    add_to_inherits(i);
  }


  init_node=0;
  num_parse_error=0;

  push_compiler_frame();
  add_ref(compiler_frame->current_return_type=void_type_string);

#ifdef PIKE_DEBUG
  if(lex.current_file)
    store_linenumber(last_pc, lex.current_file);
#endif
}

void start_new_program(void)
{
  /* fprintf(stderr, "start_new_program(): threads_disabled:%d, compilation_depth:%d\n", threads_disabled, compilation_depth); */
  low_start_new_program(0,0,0);
}


void really_free_program(struct program *p)
{
  unsigned INT16 e;

  if(id_to_program_cache[p->id & (ID_TO_PROGRAM_CACHE_SIZE-1)]==p)
    id_to_program_cache[p->id & (ID_TO_PROGRAM_CACHE_SIZE-1)]=0;

  if(p->strings)
    for(e=0; e<p->num_strings; e++)
      if(p->strings[e])
	free_string(p->strings[e]);

  if(p->identifiers)
  {
    for(e=0; e<p->num_identifiers; e++)
    {
      if(p->identifiers[e].name)
	free_string(p->identifiers[e].name);
      if(p->identifiers[e].type)
	free_string(p->identifiers[e].type);
    }
  }

  if(p->constants)
    free_svalues(p->constants, p->num_constants, BIT_MIXED);

  if(p->inherits)
    for(e=0; e<p->num_inherits; e++)
    {
      if(p->inherits[e].name)
	free_string(p->inherits[e].name);
      if(e)
      {
	if(p->inherits[e].prog)
	  free_program(p->inherits[e].prog);
      }
      if(p->inherits[e].parent)
	free_object(p->inherits[e].parent);
    }

  if(p->prev)
    p->prev->next=p->next;
  else
    first_program=p->next;
  
  if(p->next)
    p->next->prev=p->prev;
  
  if(p->flags & PROGRAM_OPTIMIZED)
  {
    if(p->program)
      free(p->program);
#define FOO(NUMTYPE,TYPE,NAME) p->NAME=0;
#include "program_areas.h"
  }else{
#define FOO(NUMTYPE,TYPE,NAME) \
    if(p->NAME) { free((char *)p->NAME); p->NAME=0; }
#include "program_areas.h"
  }
  
  free((char *)p);
  
  GC_FREE();
}

#ifdef PIKE_DEBUG
void dump_program_desc(struct program *p)
{
  int e,d,q;
/*  fprintf(stderr,"Program '%s':\n",p->name->str); */

/*
  fprintf(stderr,"All inherits:\n");
  for(e=0;e<p->num_inherits;e++)
  {
    fprintf(stderr,"%3d:",e);
    for(d=0;d<p->inherits[e].inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%s\n",p->inherits[e].prog->name->str);
  }
*/

  fprintf(stderr,"All identifiers:\n");
  for(e=0;e<(int)p->num_identifier_references;e++)
  {
    fprintf(stderr,"%3d:",e);
    for(d=0;d<INHERIT_FROM_INT(p,e)->inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%s;\n",ID_FROM_INT(p,e)->name->str);
  }
  fprintf(stderr,"All sorted identifiers:\n");
  for(q=0;q<(int)p->num_identifier_index;q++)
  {
    e=p->identifier_index[q];
    fprintf(stderr,"%3d (%3d):",e,q);
    for(d=0;d<INHERIT_FROM_INT(p,e)->inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%s;\n", ID_FROM_INT(p,e)->name->str);
  }
}
#endif

static void toss_compilation_resources(void)
{
  if(fake_object)
  {
    free_program(fake_object->prog);
    fake_object->prog=0;
    free_object(fake_object);
    fake_object=0;
  }

  free_program(new_program);
  new_program=0;

  if(malloc_size_program)
    {
      free((char *)malloc_size_program);
      malloc_size_program=0;
    }

  if(module_index_cache)
  {
    free_mapping(module_index_cache);
    module_index_cache=0;
  }
  
  while(compiler_frame)
    pop_compiler_frame();

  if(last_file)
  {
    free_string(last_file);
    last_file=0;
  }

  unuse_modules(num_used_modules);
}

int sizeof_variable(int run_time_type)
{
  switch(run_time_type)
  {
    case T_FUNCTION:
    case T_MIXED: return sizeof(struct svalue);
    case T_INT: return sizeof(INT_TYPE);
    case T_FLOAT: return sizeof(FLOAT_TYPE);
    default: return sizeof(char *);
  }
}

static int alignof_variable(int run_time_type)
{
  switch(run_time_type)
  {
    case T_FUNCTION:
    case T_MIXED:
#ifdef HAVE_ALIGNOF
      return ALIGNOF(struct svalue);
#else
      return ALIGNOF(union anything);
#endif
    case T_INT: return ALIGNOF(INT_TYPE);
    case T_FLOAT: return ALIGNOF(FLOAT_TYPE);
    default: return ALIGNOF(char *);
  }
}

#ifdef PIKE_DEBUG
void check_program(struct program *p)
{
  INT32 size,e;
  unsigned INT32 checksum;

  if(p->id > current_program_id)
    fatal("Program id is out of sync! (p->id=%d, current_program_id=%d)\n",p->id,current_program_id);

  if(p->refs <=0)
    fatal("Program has zero refs.\n");

  if(p->next && p->next->prev != p)
    fatal("Program ->next->prev != program.\n");

  if(p->prev)
  {
    if(p->prev->next != p)
      fatal("Program ->prev->next != program.\n");
  }else{
    if(first_program != p)
      fatal("Program ->prev == 0 but first_program != program.\n");
  }

  if(p->id > current_program_id || p->id <= 0)
    fatal("Program id is wrong.\n");

  if(p->storage_needed < 0)
    fatal("Program->storage_needed < 0.\n");

  if(p->num_identifier_index > p->num_identifier_references)
    fatal("Too many identifier index entries in program!\n");

  for(e=0;e<(int)p->num_constants;e++)
    check_svalue(p->constants + e);

  for(e=0;e<(int)p->num_strings;e++)
    check_string(p->strings[e]);

  for(e=0;e<(int)p->num_identifiers;e++)
  {
    check_string(p->identifiers[e].name);
    check_string(p->identifiers[e].type);

    if(p->identifiers[e].identifier_flags & ~15)
      fatal("Unknown flags in identifier flag field.\n");

    if(p->identifiers[e].run_time_type!=T_MIXED)
      check_type(p->identifiers[e].run_time_type);

    if(IDENTIFIER_IS_VARIABLE(p->identifiers[e].identifier_flags))
    {
      if(p->identifiers[e].func.offset &
	 (alignof_variable(p->identifiers[e].run_time_type)-1))
      {
	fatal("Variable %s offset is not properly aligned (%d).\n",p->identifiers[e].name->str,p->identifiers[e].func.offset);
      }
    }
  }

  for(e=0;e<(int)p->num_identifier_references;e++)
  {
    if(p->identifier_references[e].inherit_offset > p->num_inherits)
      fatal("Inherit offset is wrong!\n");

    if(p->identifier_references[e].identifier_offset >
       p->inherits[p->identifier_references[e].inherit_offset].prog->num_identifiers)
      fatal("Identifier offset is wrong!\n");
  }

  for(e=0;e<(int)p->num_identifier_index;e++)
  {
    if(p->identifier_index[e] > p->num_identifier_references)
      fatal("Program->identifier_indexes[%ld] is wrong\n",(long)e);
  }

  for(e=0;e<(int)p->num_inherits;e++)
  {
    if(p->inherits[e].storage_offset < 0)
      fatal("Inherit->storage_offset is wrong.\n");

#if 0
    /* This test doesn't really work... */
    if(p->inherits[e].storage_offset & (ALIGN_BOUND-1))
    {
      fatal("inherit[%d].storage_offset is not properly aligned (%d).\n",e,p->inherits[e].storage_offset);
    }
#endif
  }
}
#endif

struct program *end_first_pass(int finish)
{
  int e;
  struct program *prog;
  struct pike_string *s;

  MAKE_CONSTANT_SHARED_STRING(s,"__INIT");


  /* Collect references to inherited __INIT functions */
  for(e=new_program->num_inherits-1;e;e--)
  {
    int id;
    if(new_program->inherits[e].inherit_level!=1) continue;
    id=low_reference_inherited_identifier(0, e, s);
    if(id!=-1)
    {
      init_node=mknode(F_ARG_LIST,
		       mkcastnode(void_type_string,
				  mkapplynode(mkidentifiernode(id),0)),
		       init_node);
    }
  }

  /*
   * Define the __INIT function, but only if there was any code
   * to initialize.
   */

  if(init_node)
  {
    union idptr tmp;
    dooptcode(s,
	      mknode(F_ARG_LIST,
		     init_node,mknode(F_RETURN,mkintnode(0),0)),
	      function_type_string,
	      0);
    init_node=0;
  }

  free_string(s);

  pop_compiler_frame(); /* Pop __INIT local variables */

  if(num_parse_error > 0)
  {
    prog=0;
  }else{
    prog=new_program;
    add_ref(prog);

#ifdef PIKE_DEBUG
    check_program(prog);
    if(l_flag)
      dump_program_desc(prog);
#endif

    new_program->flags |= PROGRAM_PASS_1_DONE;

    if(finish)
    {
      fixate_program();
      optimize_program(new_program);
      new_program->flags |= PROGRAM_FINISHED;
    }

  }
  toss_compilation_resources();

#define POP
#include "compilation.h"
#undef POP

  exit_type_stack();

  free_all_nodes();

  compilation_depth--;

  exit_threads_disable(NULL);

  /* fprintf(stderr, "end_first_pass(): compilation_depth:%d\n", compilation_depth); */
  if(!compiler_frame && compiler_pass==2 && resolve_cache)
  {
    free_mapping(resolve_cache);
    resolve_cache=0;
  }
  return prog;
}

/*
 * Finish this program, returning the newly built program
 */
struct program *debug_end_program(void)
{
  return end_first_pass(1);
}


/*
 * Allocate needed for this program in the object structure.
 * An offset to the data is returned.
 */
SIZE_T low_add_storage(SIZE_T size, SIZE_T alignment)
{
  SIZE_T offset;
  offset=DO_ALIGN(new_program->storage_needed, alignment);
  new_program->storage_needed = offset + size;
#ifdef PIKE_DEBUG
  if(alignment <=0) fatal("Alignment must be at least 1\n");
  if(new_program->storage_needed<0)
    fatal("add_storage failed horribly!\n");
#endif
  return offset;
}

SIZE_T add_storage(SIZE_T storage)
{
  return low_add_storage(storage,
			 storage>ALIGN_BOUND? ALIGN_BOUND : storage ? (1<<my_log2(storage)) : 1);
}

/*
 * set a callback used to initialize clones of this program
 * the init function is called at clone time
 */
void set_init_callback(void (*init)(struct object *))
{
  new_program->init=init;
}

/*
 * set a callback used to de-initialize clones of this program
 * the exit function is called at destruct
 */
void set_exit_callback(void (*exit)(struct object *))
{
  new_program->exit=exit;
}

/*
 * This callback is called to allow the object to mark all internal
 * structures as 'used'.
 */
void set_gc_mark_callback(void (*m)(struct object *))
{
  new_program->gc_marked=m;
}

/*
 * Called for all objects and inherits in first pass of gc()
 */
void set_gc_check_callback(void (*m)(struct object *))
{
  new_program->gc_check=m;
}

int low_reference_inherited_identifier(struct program_state *q,
				       int e,
				       struct pike_string *name)
{
  struct program *np=q?q->new_program:new_program;
  struct reference funp;
  struct program *p;
  int i,d;

  p=np->inherits[e].prog;
  i=find_shared_string_identifier(name,p);
  if(i==-1)
  {
    i=really_low_find_shared_string_identifier(name,p,1);
    if(i==-1) return -1;
  }

  if(p->identifier_references[i].id_flags & ID_HIDDEN)
    return -1;

  funp=p->identifier_references[i];
  funp.inherit_offset+=e;
  funp.id_flags|=ID_HIDDEN;

  for(d=0;d<(int)np->num_identifier_references;d++)
  {
    struct reference *fp;
    fp=np->identifier_references+d;

    if(!MEMCMP((char *)fp,(char *)&funp,sizeof funp)) return d;
  }

  if(q)
    low_add_to_identifier_references(q,funp);
  else
    add_to_identifier_references(funp);
  return np->num_identifier_references -1;
}

static int middle_reference_inherited_identifier(
  struct program_state *state,
  struct pike_string *super_name,
  struct pike_string *function_name)
{
  int e,i;
  struct program *p=state?state->new_program:new_program;

#ifdef PIKE_DEBUG
  if(function_name!=debug_findstring(function_name))
    fatal("reference_inherited_function on nonshared string.\n");
#endif
  
  for(e=p->num_inherits-1;e>0;e--)
  {
    if(p->inherits[e].inherit_level!=1) continue;
    if(!p->inherits[e].name) continue;

    if(super_name)
      if(super_name != p->inherits[e].name)
	continue;

    i=low_reference_inherited_identifier(state,e,function_name);
    if(i==-1) continue;
    return i;
  }
  return -1;
}

node *reference_inherited_identifier(struct pike_string *super_name,
				   struct pike_string *function_name)
{
  int i,n;
  struct program_state *p=previous_program_state;

  i=middle_reference_inherited_identifier(0,
					  super_name,
					  function_name);
  if(i!=-1) return mkidentifiernode(i);

  for(n=0;n<compilation_depth;n++,p=p->previous)
  {
    i=middle_reference_inherited_identifier(p,super_name,
					    function_name);
    if(i!=-1)
      return mkexternalnode(n,i,ID_FROM_INT(p->new_program, i));
  }

  return 0;
}

void rename_last_inherit(struct pike_string *n)
{
  if(new_program->inherits[new_program->num_inherits].name)
    free_string(new_program->inherits[new_program->num_inherits].name);
  copy_shared_string(new_program->inherits[new_program->num_inherits].name,
		     n);
}

/*
 * make this program inherit another program
 */
void low_inherit(struct program *p,
		 struct object *parent,
		 int parent_identifier,
		 int parent_offset,
		 INT32 flags,
		 struct pike_string *name)
{
  int e, inherit_offset, storage_offset;
  struct inherit inherit;
  struct pike_string *s;

  
  if(!p)
  {
    yyerror("Illegal program pointer.");
    return;
  }

  if(!(p->flags & (PROGRAM_FINISHED | PROGRAM_PASS_1_DONE)))
  {
    yyerror("Cannot inherit program which is not fully compiled yet.");
    return;
  }

  inherit_offset = new_program->num_inherits;

  storage_offset=add_storage(p->storage_needed);

  for(e=0; e<(int)p->num_inherits; e++)
  {
    inherit=p->inherits[e];
    add_ref(inherit.prog);
    inherit.identifier_level += new_program->num_identifier_references;
    inherit.storage_offset += storage_offset;
    inherit.inherit_level ++;
    if(!e)
    {
      if(parent)
      {
	if(parent->next == parent)
	{
	  struct object *o;
	  for(o=fake_object->parent;o!=parent;o=o->parent)
	  {
#ifdef PIKE_DEBUG
	    if(!o) fatal("low_inherit with odd fake_object as parent!\n");
#endif
	    inherit.parent_offset++;
	  }
	}else{
	  inherit.parent=parent;
	  inherit.parent_identifier=parent_identifier;
	  inherit.parent_offset=0;
	}
      }else{
	inherit.parent_offset+=parent_offset;
      }
    }else{
      if(parent && parent->next != parent && inherit.parent_offset)
      {
	struct object *par=parent;
	int e,pid=parent_identifier;
	for(e=1;e<inherit.parent_offset;e++)
	{
	  struct inherit *in;
	  if(!par->prog)
	  {
	    par=0;
	    pid=0;
	    break;
	  }

	  in=INHERIT_FROM_INT(par->prog, pid);
	  if(in->parent_offset)
	  {
	    pid=par->parent_identifier;
	    par=par->parent;
	    e-=in->parent_offset-1;
	  }else{
	    pid=in->parent_identifier;
	    par=in->parent;
	  }
	}

	inherit.parent=par;
	inherit.parent_identifier=pid;
	inherit.parent_offset=0;
      }else{
	inherit.parent_offset+=parent_offset;
      }
    }
    if(inherit.parent) add_ref(inherit.parent);

    if(name)
    {
      if(e==0)
      {
	copy_shared_string(inherit.name,name);
      }
      else if(inherit.name)
      {
	struct pike_string *s;
	s=begin_shared_string(inherit.name->len + name->len + 2);
	MEMCPY(s->str,name->str,name->len);
	MEMCPY(s->str+name->len,"::",2);
	MEMCPY(s->str+name->len+2,inherit.name->str,inherit.name->len);
	inherit.name=end_shared_string(s);
      }
      else
      {
	inherit.name=0;
      }
    }else{
      inherit.name=0;
    }
    add_to_inherits(inherit);
  }

  for (e=0; e < (int)p->num_identifier_references; e++)
  {
    struct reference fun;
    struct pike_string *name;

    fun = p->identifier_references[e]; /* Make a copy */

    name=ID_FROM_PTR(p,&fun)->name;
    fun.inherit_offset += inherit_offset;

    if (fun.id_flags & ID_NOMASK)
    {
      int n;
      n = isidentifier(name);
      if (n != -1 && ID_FROM_INT(new_program,n)->func.offset != -1)
	my_yyerror("Illegal to redefine 'nomask' function/variable \"%s\"",name->str);
    }

    if(fun.id_flags & ID_PRIVATE) fun.id_flags|=ID_HIDDEN;

    if (fun.id_flags & ID_PUBLIC)
      fun.id_flags |= flags & ~ID_PRIVATE;
    else
      fun.id_flags |= flags;

    fun.id_flags |= ID_INHERITED;
    add_to_identifier_references(fun);
  }
}

void do_inherit(struct svalue *s,
		INT32 flags,
		struct pike_string *name)
{
  struct program *p=program_from_svalue(s);
  low_inherit(p,
	      s->type == T_FUNCTION ? s->u.object : 0,
	      s->subtype,
	      0,
	      flags,
	      name);
}

void compiler_do_inherit(node *n,
			 INT32 flags,
			 struct pike_string *name)
{
  if(!n)
  {
    yyerror("Unable to inherit");
    return;
  }
  switch(n->token)
  {
    case F_EXTERNAL:
    {
      struct identifier *i;
      struct program *p=parent_compilation(n->u.integer.a);
      INT32 numid=n->u.integer.b;
      
      if(!p)
      {
	yyerror("Failed to resolv external constant.\n");
	return;
      }

      i=ID_FROM_INT(p, numid);
    
      if(IDENTIFIER_IS_CONSTANT(i->identifier_flags))
      {
	struct svalue *s=PROG_FROM_INT(p, numid)->constants + i->func.offset;
	if(s->type != T_PROGRAM)
	{
	  do_inherit(s,flags,name);
	  return;
	}else{
	  p=s->u.program;
	}
      }else{
	yyerror("Inherit identifier is not a constant program");
	return;
      }

      low_inherit(p,
		  0,
		  0,
		  n->u.integer.a,
		  flags,
		  name);
      break;
    }

    default:
      resolv_program(n);
      do_inherit(sp-1, flags, name);
      pop_stack();
  }
}
			 

void simple_do_inherit(struct pike_string *s,
		       INT32 flags,
		       struct pike_string *name)
{
  reference_shared_string(s);
  push_string(s);
  ref_push_string(lex.current_file);
  SAFE_APPLY_MASTER("handle_inherit", 2);

  if(sp[-1].type != T_PROGRAM)
  {
    my_yyerror("Couldn't find file to inherit %s",s->str);
    pop_stack();
    return;
  }

  if(name)
  {
    free_string(s);
    s=name;
  }
  do_inherit(sp-1, flags, s);
  free_string(s);
  pop_stack();
}

/*
 * Return the index of the identifier found, otherwise -1.
 */
int isidentifier(struct pike_string *s)
{
  INT32 e;
  for(e=new_program->num_identifier_references-1;e>=0;e--)
  {
    if(new_program->identifier_references[e].id_flags & ID_HIDDEN) continue;
    
    if(ID_FROM_INT(new_program, e)->name == s)
      return e;
  }
  return -1;
}

/* argument must be a shared string */
int low_define_variable(struct pike_string *name,
			struct pike_string *type,
			INT32 flags,
			INT32 offset,
			INT32 run_time_type)
{
  int n;

  struct identifier dummy;
  struct reference ref;

#ifdef PIKE_DEBUG
  if(new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    fatal("Attempting to add variable to fixed program\n");

  if(compiler_pass==2)
    fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n"
	  "Added identifier: \"%s\"\n", name->str);
#endif

  copy_shared_string(dummy.name, name);
  copy_shared_string(dummy.type, type);
  dummy.identifier_flags = 0;
  dummy.run_time_type=run_time_type;
  dummy.func.offset=offset;
#ifdef PROFILING
  dummy.self_time=0;
  dummy.num_calls=0;
  dummy.total_time=0;
#endif

  ref.id_flags=flags;
  ref.identifier_offset=new_program->num_identifiers;
  ref.inherit_offset=0;

  add_to_variable_index(ref.identifier_offset);
  
  add_to_identifiers(dummy);
  
  n=new_program->num_identifier_references;
  add_to_identifier_references(ref);

  return n;
}

int map_variable(char *name,
		 char *type,
		 INT32 flags,
		 INT32 offset,
		 INT32 run_time_type)
{
  int ret;
  struct pike_string *n,*t;
  n=make_shared_string(name);
  t=parse_type(type);
  ret=low_define_variable(n,t,flags,offset,run_time_type);
  free_string(n);
  free_string(t);
  return ret;
}

/* argument must be a shared string */
int define_variable(struct pike_string *name,
		    struct pike_string *type,
		    INT32 flags)
{
  int n, run_time_type;

#ifdef PIKE_DEBUG
  if(name!=debug_findstring(name))
    fatal("define_variable on nonshared string.\n");
#endif

  if(type == void_type_string)
    yyerror("Variables can't be of type void");
  
  n = isidentifier(name);

  if(new_program->flags & PROGRAM_PASS_1_DONE)
  {
    if(n==-1)
      yyerror("Pass2: Variable disappeared!");
    else
      return n;
  }

#ifdef PIKE_DEBUG
  if(new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    fatal("Attempting to add variable to fixed program\n");
#endif

  if(n != -1)
  {

    if (IDENTIFIERP(n)->id_flags & ID_NOMASK)
      my_yyerror("Illegal to redefine 'nomask' variable/functions \"%s\"", name->str);

    if(PROG_FROM_INT(new_program, n) == new_program)
      my_yyerror("Variable '%s' defined twice.",name->str);

    if(!(IDENTIFIERP(n)->id_flags & ID_INLINE) || compiler_pass!=1)
    {
      if(ID_FROM_INT(new_program, n)->type != type)
	my_yyerror("Illegal to redefine inherited variable with different type.");
      
      if(ID_FROM_INT(new_program, n)->identifier_flags != flags)
	my_yyerror("Illegal to redefine inherited variable with different type.");
      return n;
    }
  }

  run_time_type=compile_type_to_runtime_type(type);

  switch(run_time_type)
  {
    case T_FUNCTION:
    case T_PROGRAM:
      run_time_type = T_MIXED;
  }
  
  n=low_define_variable(name,type,flags,
			low_add_storage(sizeof_variable(run_time_type),
					alignof_variable(run_time_type)),
			run_time_type);
  

  return n;
}

int simple_add_variable(char *name,
			char *type,
			INT32 flags)
{
  INT32 ret;
  struct pike_string *name_s, *type_s;
  name_s=make_shared_string(name);
  type_s=parse_type(type);
  
  ret=define_variable(name_s, type_s, flags);
  free_string(name_s);
  free_string(type_s);
  return ret;
}

/* FIXME: add_constant with c==0 means declaration */
int add_constant(struct pike_string *name,
		 struct svalue *c,
		 INT32 flags)
{
  int n;
  struct identifier dummy;
  struct reference ref;

#ifdef PIKE_DEBUG
  if(name!=debug_findstring(name))
    fatal("define_constant on nonshared string.\n");
#endif

  n = isidentifier(name);


  if(new_program->flags & PROGRAM_PASS_1_DONE)
  {
    if(n==-1)
    {
      yyerror("Pass2: Constant disappeared!");
    }else{
#if 1
      struct identifier *id;
      id=ID_FROM_INT(new_program,n);
      if(id->func.offset>=0)
      {
	struct pike_string *s;
	struct svalue *c=PROG_FROM_INT(new_program,n)->constants+
	  id->func.offset;
	s=get_type_of_svalue(c);
	free_string(id->type);
	id->type=s;
      }
#endif
      return n;
    }
  }

#ifdef PIKE_DEBUG
  if(new_program->flags & (PROGRAM_FIXED | PROGRAM_OPTIMIZED))
    fatal("Attempting to add constant to fixed program\n");

  if(compiler_pass==2)
    fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n");
#endif

  copy_shared_string(dummy.name, name);
  dummy.type = get_type_of_svalue(c);
  
  dummy.identifier_flags = IDENTIFIER_CONSTANT;
  dummy.run_time_type=c->type;
  
  dummy.func.offset=store_constant(c, 0);

  ref.id_flags=flags;
  ref.identifier_offset=new_program->num_identifiers;
  ref.inherit_offset=0;

#ifdef PROFILING
  dummy.self_time=0;
  dummy.num_calls=0;
  dummy.total_time=0;
#endif

  add_to_identifiers(dummy);

  if(n != -1)
  {
    if(IDENTIFIERP(n)->id_flags & ID_NOMASK)
      my_yyerror("Illegal to redefine 'nomask' identifier \"%s\"", name->str);

    if(PROG_FROM_INT(new_program, n) == new_program)
      my_yyerror("Identifier '%s' defined twice.",name->str);

    if(!(IDENTIFIERP(n)->id_flags & ID_INLINE))
    {
      /* override */
      new_program->identifier_references[n]=ref;

      return n;
    }
  }
  n=new_program->num_identifier_references;
  add_to_identifier_references(ref);

  return n;
}

int simple_add_constant(char *name, 
			struct svalue *c,
			INT32 flags)
{
  INT32 ret;
  struct pike_string *id;
  id=make_shared_string(name);
  ret=add_constant(id, c, flags);
  free_string(id);
  return ret;
}

int add_integer_constant(char *name,
			 INT32 i,
			 INT32 flags)
{
  struct svalue tmp;
  tmp.u.integer=i;
  tmp.type=T_INT;
  tmp.subtype=NUMBER_NUMBER;
  return simple_add_constant(name, &tmp, flags);
}

int add_float_constant(char *name,
			 double f,
			 INT32 flags)
{
  struct svalue tmp;
  tmp.type=T_FLOAT;
  tmp.u.float_number=f;
  tmp.subtype=0;
  return simple_add_constant(name, &tmp, flags);
}

int add_string_constant(char *name,
			char *str,
			INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  tmp.type=T_STRING;
  tmp.subtype=0;
  tmp.u.string=make_shared_string(str);
  ret=simple_add_constant(name, &tmp, flags);
  free_svalue(&tmp);
  return ret;
}

int add_program_constant(char *name,
			 struct program *p,
			 INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  tmp.type=T_PROGRAM;
  tmp.subtype=0;
  tmp.u.program=p;
  ret=simple_add_constant(name, &tmp, flags);
  return ret;
}

int add_object_constant(char *name,
			struct object *o,
			INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  tmp.type=T_OBJECT;
  tmp.subtype=0;
  tmp.u.object=o;
  ret=simple_add_constant(name, &tmp, flags);
  return ret;
}

int add_function_constant(char *name, void (*cfun)(INT32), char * type, INT16 flags)
{
  struct svalue s;
  struct pike_string *n;
  INT32 ret;

  s.type=T_FUNCTION;
  s.subtype=FUNCTION_BUILTIN;
  s.u.efun=make_callable(cfun, name, type, flags, 0, 0);
  ret=simple_add_constant(name, &s, 0);
  free_svalue(&s);
  return ret;
}


int debug_end_class(char *name, INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  tmp.type=T_PROGRAM;
  tmp.subtype=0;
  tmp.u.program=end_program();
  if(!tmp.u.program)
    fatal("Failed to initialize class '%s'\n",name);
  ret=simple_add_constant(name, &tmp, flags);
  free_svalue(&tmp);
  return ret;
}

/*
 * define a new function
 * if func isn't given, it is supposed to be a prototype.
 */
INT32 define_function(struct pike_string *name,
		      struct pike_string *type,
		      INT16 flags,
		      INT8 function_flags,
		      union idptr *func)
{
  struct identifier *funp,fun;
  struct reference ref;
  INT32 i;

#ifdef PROFILING
  fun.self_time=0;
  fun.num_calls=0;
  fun.total_time=0;
#endif

  i=isidentifier(name);

  if(i >= 0)
  {
    /* already defined */

    funp=ID_FROM_INT(new_program, i);
    ref=new_program->identifier_references[i];

    if(ref.inherit_offset == 0) /* not inherited */
    {
      if(!(!func || func->offset == -1) && !(funp->func.offset == -1))
      {
	my_yyerror("Redeclaration of function %s.",name->str);
	return i;
      }

      /* match types against earlier prototype or vice versa */
      if(!match_types(type, funp->type))
      {
	my_yyerror("Prototype doesn't match for function %s.",name->str);
      }
    }

    if((ref.id_flags & ID_NOMASK) && !(funp->func.offset == -1))
    {
      my_yyerror("Illegal to redefine 'nomask' function %s.",name->str);
    }

    if(!(ref.id_flags & ID_INLINE) || compiler_pass!=1)
    {
      /* We modify the old definition if it is in this program */
      if(ref.inherit_offset==0)
      {
	if(func)
	  funp->func = *func;
	else
	  funp->func.offset = -1;
	
	funp->identifier_flags=function_flags;
      }else{
	/* Otherwise we make a new definition */
	copy_shared_string(fun.name, name);
	copy_shared_string(fun.type, type);
	
	fun.run_time_type=T_FUNCTION;
	
	fun.identifier_flags=function_flags;
	if(function_flags & IDENTIFIER_C_FUNCTION)
	  new_program->flags |= PROGRAM_HAS_C_METHODS;
	
	if(func)
	  fun.func = *func;
	else
	  fun.func.offset = -1;
	
	ref.identifier_offset=new_program->num_identifiers;
	add_to_identifiers(fun);
      }
      
      ref.inherit_offset = 0;
      ref.id_flags = flags;
      new_program->identifier_references[i]=ref;
      return i;
    }
  }

#ifdef PIKE_DEBUG
  if(compiler_pass==2)
    fatal("Internal error: Not allowed to add more identifiers during second compiler pass.\n");
#endif

  /* define a new function */

  copy_shared_string(fun.name, name);
  copy_shared_string(fun.type, type);
  
  fun.identifier_flags=function_flags;
  if(function_flags & IDENTIFIER_C_FUNCTION)
    new_program->flags |= PROGRAM_HAS_C_METHODS;
  
  fun.run_time_type=T_FUNCTION;
  
  if(func)
    fun.func = *func;
  else
    fun.func.offset = -1;
  
  i=new_program->num_identifiers;
  
  add_to_identifiers(fun);
  
  ref.id_flags = flags;
  ref.identifier_offset = i;
  ref.inherit_offset = 0;
  
  i=new_program->num_identifier_references;
  add_to_identifier_references(ref);

  return i;
}


int really_low_find_shared_string_identifier(struct pike_string *name,
					     struct program *prog,
					     int see_static)
{
  struct reference *funp;
  struct identifier *fun;
  int i,t;
  for(i=0;i<(int)prog->num_identifier_references;i++)
  {
    funp = prog->identifier_references + i;
    if(funp->id_flags & ID_HIDDEN) continue;
    if(funp->id_flags & ID_HIDDEN)
      if(!see_static)
	continue;
    fun = ID_FROM_PTR(prog, funp);
    /* if(fun->func.offset == -1) continue; * Prototype */
    if(!is_same_string(fun->name,name)) continue;
    if(funp->id_flags & ID_INHERITED)
    {
      if(funp->id_flags & ID_PRIVATE) continue;
      for(t=0; t>=0 && t<(int)prog->num_identifier_references; t++)
      {
	struct reference *funpb;
	struct identifier *funb;
	
	if(t==i) continue;
	funpb=prog->identifier_references+t;
	if(funpb->id_flags & (ID_HIDDEN|ID_STATIC)) continue;
	if((funpb->id_flags & ID_INHERITED) && t<i) continue;
	funb=ID_FROM_PTR(prog,funpb);
	/* if(funb->func.offset == -1) continue; * prototype */
	if(fun->name==funb->name) t=-10;
      }
      if(t < 0) continue;
    }
    return i;
  }
  return -1;
}

/*
 * lookup the number of a function in a program given the name in
 * a shared_string
 */
int low_find_shared_string_identifier(struct pike_string *name,
				      struct program *prog)
{
  int max,min,tst;
  struct identifier *fun;

  if(prog->flags & PROGRAM_FIXED)
  {
    unsigned short *funindex = prog->identifier_index;

#ifdef PIKE_DEBUG
    if(!funindex)
      fatal("No funindex in fixed program\n");
#endif
  
    max = prog->num_identifier_index;
    min = 0;
    while(max != min)
    {
      tst=(max + min) >> 1;
      fun = ID_FROM_INT(prog, funindex[tst]);
      if(is_same_string(fun->name,name)) return funindex[tst];
      if(my_order_strcmp(fun->name, name) > 0)
	max=tst;
      else
	min=tst+1;
    }
  }else{
    return really_low_find_shared_string_identifier(name,prog,0);
  }
  return -1;
}

#ifdef FIND_FUNCTION_HASHSIZE
#if FIND_FUNCTION_HASHSIZE == 0
#undef FIND_FUNCTION_HASHSIZE
#endif
#endif

#ifdef FIND_FUNCTION_HASHSIZE
struct ff_hash
{
  struct pike_string *name;
  int id;
  int fun;
};

static struct ff_hash cache[FIND_FUNCTION_HASHSIZE];
#endif

int find_shared_string_identifier(struct pike_string *name,
				  struct program *prog)
{
#ifdef FIND_FUNCTION_HASHSIZE
  if(prog -> flags & PROGRAM_FIXED)
  {
    unsigned int hashval;
    hashval=my_hash_string(name);
    hashval+=prog->id;
    hashval^=(unsigned long)prog;
    hashval-=name->str[0];
    hashval%=FIND_FUNCTION_HASHSIZE;
    if(is_same_string(cache[hashval].name,name) &&
       cache[hashval].id==prog->id)
      return cache[hashval].fun;

    if(cache[hashval].name) free_string(cache[hashval].name);
    copy_shared_string(cache[hashval].name,name);
    cache[hashval].id=prog->id;
    return cache[hashval].fun=low_find_shared_string_identifier(name,prog);
  }
#endif /* FIND_FUNCTION_HASHSIZE */

  return low_find_shared_string_identifier(name,prog);
}

int find_identifier(char *name,struct program *prog)
{
  struct pike_string *n;
  if(!prog) {
    if (strlen(name) < 1024) {
      error("Lookup of identifier %s in destructed object.\n", name);
    } else {
      error("Lookup of long identifier in destructed object.\n");
    }
  }
  n=findstring(name);
  if(!n) return -1;
  return find_shared_string_identifier(n,prog);
}

int store_prog_string(struct pike_string *str)
{
  unsigned int i;

  for (i=0;i<new_program->num_strings;i++)
    if (new_program->strings[i] == str)
      return i;

  reference_shared_string(str);
  add_to_strings(str);
  return i;
}

int store_constant(struct svalue *foo, int equal)
{
  struct svalue tmp;
  unsigned int e;

  for(e=0;e<new_program->num_constants;e++)
  {
    struct svalue *s=new_program->constants + e;
    if(equal ? is_equal(s,foo) : is_eq(s,foo))
      return e;
  }

  assign_svalue_no_free(&tmp,foo);
  add_to_constants(tmp);
  return e;
}

/*
 * program examination functions available from Pike.
 */

struct array *program_indices(struct program *p)
{
  int e;
  int n = 0;
  struct array *res;
  for (e = p->num_identifier_references; e--; ) {
    struct identifier *id;
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_STATIC|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      ref_push_string(ID_FROM_INT(p, e)->name);
      n++;
    }
  }
  f_aggregate(n);
  res = sp[-1].u.array;
  add_ref(res);
  pop_stack();
  return(res);
}

struct array *program_values(struct program *p)
{
  int e;
  int n = 0;
  struct array *res;
  for(e = p->num_identifier_references; e--; ) {
    struct identifier *id;
    if (p->identifier_references[e].id_flags &
	(ID_HIDDEN|ID_STATIC|ID_PRIVATE)) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      struct program *p2 = PROG_FROM_INT(p, e);
      push_svalue(p2->constants + id->func.offset);
      n++;
    }
  }
  f_aggregate(n);
  res = sp[-1].u.array;
  add_ref(res);
  pop_stack();
  return(res);
}

void program_index_no_free(struct svalue *to, struct program *p,
			   struct svalue *ind)
{
  int e;
  struct pike_string *s;

  if (ind->type != T_STRING) {
    error("Can't index a program with a %s (expected string)\n",
	  get_name_of_type(ind->type));
  }
  s = ind->u.string;
  for (e = p->num_identifier_references; e--; ) {
    struct identifier *id;
    if (p->identifier_references[e].id_flags & ID_HIDDEN) {
      continue;
    }
    id = ID_FROM_INT(p, e);
    if (id->name != s) {
      continue;
    }
    if (IDENTIFIER_IS_CONSTANT(id->identifier_flags)) {
      struct program *p2 = PROG_FROM_INT(p, e);
      assign_svalue_no_free(to, (p2->constants + id->func.offset));
      return;
    } else {
      if (s->len < 1024) {
	error("Index \"%s\" is not constant.", s->str);
      } else {
	error("Index is not constant.");
      }
    }
  }
  if (s->len < 1024) {
    error("No such index \"%s\".", s->str);
  } else {
    error("No such index.");
  }
}

/*
 * Line number support routines, now also tells what file we are in
 */
static int get_small_number(char **q)
{
  int ret;
  switch(ret=(*(signed char **)q)++[0])
  {
  case -127:
    ret=EXTRACT_WORD((unsigned char*)*q);
    *q+=2;
    return ret;

  case -128:
    ret=EXTRACT_INT((unsigned char*)*q);
    *q+=4;
    return ret;

  default:
    return ret;
  }
}

void start_line_numbering(void)
{
  if(last_file)
  {
    free_string(last_file);
    last_file=0;
  }
  last_pc=last_line=0;
}

static void insert_small_number(INT32 a)
{
  if(a>-127 && a<127)
  {
    add_to_linenumbers(a);
  }else if(a>=-32768 && a<32768){
    add_to_linenumbers(-127);
    ins_short(a, add_to_linenumbers);
  }else{
    add_to_linenumbers(-128);
    ins_int(a, add_to_linenumbers);
  }	
}

void store_linenumber(INT32 current_line, struct pike_string *current_file)
{
  if(last_line!=current_line || last_file != current_file)
  {
    if(last_file != current_file)
    {
      char *tmp;
      if(last_file) free_string(last_file);
      add_to_linenumbers(127);
      for(tmp=current_file->str; *tmp; tmp++)
	add_to_linenumbers(*tmp);
      add_to_linenumbers(0);
      copy_shared_string(last_file, current_file);
    }
    insert_small_number(PC-last_pc);
    insert_small_number(current_line-last_line);
    last_line=current_line;
    last_pc=PC;
  }
}

/*
 * return the file in which we were executing.
 * pc should be the program counter, prog the current
 * program, and line will be initialized to the line
 * in that file.
 */
char *get_line(unsigned char *pc,struct program *prog,INT32 *linep)
{
  static char *file, *cnt;
  static INT32 off,line,pid;
  INT32 offset;

  if (prog == 0) return "Unkown program";
  offset = pc - prog->program;

  if(prog == new_program)
  {
    linep[0]=0;
    return "Optimizer";
  }

#if 0
  if(prog->id != pid || offset < off)
#endif
  {
    cnt=prog->linenumbers;
    off=line=0;
    file="Line not found";
    pid=prog->id;
  }

  if (offset > (INT32)prog->num_program || offset<0)
    return file;

  while(cnt < prog->linenumbers + prog->num_linenumbers)
  {
    int oline;
    if(*cnt == 127)
    {
      file=cnt+1;
      cnt=file+strlen(file)+1;
    }
    off+=get_small_number(&cnt);
    oline=line;
    line+=get_small_number(&cnt);
    if(off > offset)
    {
      linep[0]=oline;
      return file;
    }
  }
  linep[0]=line;
  return file;
}

void my_yyerror(char *fmt,...)  ATTRIBUTE((format(printf,1,2)))
{
  va_list args;
  char buf[8192];

  va_start(args,fmt);

#ifdef HAVE_VSNPRINTF
  vsnprintf(buf, 8190, fmt, args);
#else /* !HAVE_VSNPRINTF */
  VSPRINTF(buf, fmt, args);
#endif /* HAVE_VSNPRINTF */

  if((long)strlen(buf) >= (long)sizeof(buf))
    fatal("Buffer overflow in my_yyerror.\n");

  yyerror(buf);
  va_end(args);
}

struct program *compile(struct pike_string *prog)
{
#ifdef PIKE_DEBUG
  JMP_BUF tmp;
#endif
  struct program *p;
  struct lex save_lex;
  int save_depth=compilation_depth;
  int saved_threads_disabled;
  dynamic_buffer used_modules_save = used_modules;
  INT32 num_used_modules_save = num_used_modules;
  void yyparse(void);

#ifdef PIKE_DEBUG
  if(SETJMP(tmp))
    fatal("Compiler exited with longjump!\n");
#endif

  low_init_threads_disable();
  saved_threads_disabled = threads_disabled;

  num_used_modules=0;
  initialize_buf(&used_modules);
  {
    struct svalue tmp;
    tmp.type=T_MAPPING;
#ifdef __CHECKER__
    tmp.subtype=0;
#endif /* __CHECKER__ */
    tmp.u.mapping=get_builtin_constants();
    use_module(& tmp);
  }

  save_lex=lex;

  lex.end=prog->str+prog->len;
  lex.current_line=1;
  lex.current_file=make_shared_string("-");
  lex.pragmas=0;

  start_new_program();
  compilation_depth=0;

/*  start_line_numbering(); */

  compiler_pass=1;
  lex.pos=prog->str;

  yyparse();  /* Parse da program */

  p=end_first_pass(0);
  
  if(p && !num_parse_error)
  {
    low_start_new_program(p,0,0);
    free_program(p);
    p=0;
    compiler_pass=2;
    lex.pos=prog->str;
    yyparse();  /* Parse da program again */
    p=end_program();
  }

#ifdef PIKE_DEBUG
  if (threads_disabled != saved_threads_disabled) {
    fatal("compile(): threads_disabled:%d saved_threads_disabled:%d\n",
	  threads_disabled, saved_threads_disabled);
  }
#endif /* PIKE_DEBUG */
  /* fprintf(stderr, "compile() Leave: threads_disabled:%d, compilation_depth:%d\n", threads_disabled, compilation_depth); */

  exit_threads_disable(NULL);

  /* if (!p) {
   *   fprintf(stderr, "Failed to compile file: \"%s\"\n", lex.current_file->str);
   * }
   */

  free_string(lex.current_file);
  lex=save_lex;

  unuse_modules(1);
#ifdef PIKE_DEBUG
  if(num_used_modules)
    fatal("Failed to pop modules properly.\n");
#endif

  toss_buffer(&used_modules);
  compilation_depth=save_depth;
  used_modules = used_modules_save;
  num_used_modules = num_used_modules_save ;

#ifdef PIKE_DEBUG
  UNSETJMP(tmp);
#endif

  if(!p) error("Compilation failed.\n");
  return p;
}

int add_function(char *name,void (*cfun)(INT32),char *type,INT16 flags)
{
  int ret;
  struct pike_string *name_tmp,*type_tmp;
  union idptr tmp;
  
  name_tmp=make_shared_string(name);
  type_tmp=parse_type(type);

  if(cfun)
  {
    tmp.c_fun=cfun;
    ret=define_function(name_tmp,
			type_tmp,
			flags,
			IDENTIFIER_C_FUNCTION,
			&tmp);
  }else{
    ret=define_function(name_tmp,
			type_tmp,
			flags,
			IDENTIFIER_C_FUNCTION,
			0);
  }
  free_string(name_tmp);
  free_string(type_tmp);
  return ret;
}

#ifdef PIKE_DEBUG
void check_all_programs(void)
{
  struct program *p;
  for(p=first_program;p;p=p->next)
    check_program(p);

#ifdef FIND_FUNCTION_HASHSIZE
  {
    unsigned long e;
    for(e=0;e<FIND_FUNCTION_HASHSIZE;e++)
    {
      if(cache[e].name)
      {
	check_string(cache[e].name);
	if(cache[e].id<0 || cache[e].id > current_program_id)
	  fatal("Error in find_function_cache[%ld].id\n",(long)e);

	if(cache[e].fun < -1 || cache[e].fun > 65536)
	  fatal("Error in find_function_cache[%ld].fun\n",(long)e);
      }
    }
  }
#endif

}
#endif

void cleanup_program(void)
{
  int e;
#ifdef FIND_FUNCTION_HASHSIZE
  for(e=0;e<FIND_FUNCTION_HASHSIZE;e++)
  {
    if(cache[e].name)
    {
      free_string(cache[e].name);
      cache[e].name=0;
    }
  }
#endif
  if(resolve_cache)
  {
    free_mapping(resolve_cache);
    resolve_cache=0;
  }
}

#ifdef GC2

void gc_mark_program_as_referenced(struct program *p)
{
  if(gc_mark(p))
  {
    int e;
    gc_mark_svalues(p->constants, p->num_constants);

    for(e=0;e<p->num_inherits;e++)
    {
      if(p->inherits[e].parent)
	gc_mark_object_as_referenced(p->inherits[e].parent);

      if(e && p->inherits[e].prog)
	gc_mark_program_as_referenced(p->inherits[e].prog);
    }
      
  }
}

void gc_check_all_programs(void)
{
  struct program *p;
  for(p=first_program;p;p=p->next)
  {
    int e;
    debug_gc_check_svalues(p->constants, p->num_constants, T_PROGRAM, p);

    for(e=0;e<p->num_inherits;e++)
    {
      if(p->inherits[e].parent)
      {
#ifdef PIKE_DEBUG
	if(debug_gc_check(p->inherits[e].parent,T_PROGRAM,p)==-2)
	  fprintf(stderr,"(program at 0x%lx -> inherit[%d].parent)\n",
		  (long)p,
		  e);
#else
	debug_gc_check(p->inherits[e].parent, T_PROGRAM, p);
#endif
      }

      if(d_flag && p->inherits[e].name)
	debug_gc_check(p->inherits[e].name, T_PROGRAM, p);

      if(e && p->inherits[e].prog)
	debug_gc_check(p->inherits[e].prog, T_PROGRAM, p);
    }

#ifdef PIKE_DEBUG
    if(d_flag)
    {
      int e;
      for(e=0;e<(int)p->num_strings;e++)
	debug_gc_check(p->strings[e], T_PROGRAM, p);

      for(e=0;e<(int)p->num_identifiers;e++)
      {
	debug_gc_check(p->identifiers[e].name, T_PROGRAM, p);
	debug_gc_check(p->identifiers[e].type, T_PROGRAM, p);
      }
    }
#endif
  }
}

void gc_mark_all_programs(void)
{
  struct program *p;
  for(p=first_program;p;p=p->next)
    if(gc_is_referenced(p))
      gc_mark_program_as_referenced(p);
}

void gc_free_all_unreferenced_programs(void)
{
  struct program *p,*next;

  for(p=first_program;p;p=next)
  {
    if(gc_do_free(p))
    {
      int e;
      add_ref(p);
      free_svalues(p->constants, p->num_constants, -1);
      for(e=0;e<p->num_inherits;e++)
      {
	if(p->inherits[e].parent)
	{
	  free_object(p->inherits[e].parent);
	  p->inherits[e].parent=0;
	}
      }
      next=p->next;
      free_program(p);
    }else{
      next=p->next;
    }
  }
}

#endif /* GC2 */


void count_memory_in_programs(INT32 *num_, INT32 *size_)
{
  INT32 size=0, num=0;
  struct program *p;
  for(p=first_program;p;p=p->next)
  {
    num++;
    size+=p->total_size;
  }
  *num_=num;
  *size_=size;
}

void push_compiler_frame(void)
{
  struct compiler_frame *f;
  f=ALLOC_STRUCT(compiler_frame);
  f->current_type=0;
  f->current_return_type=0;
  f->current_number_of_locals=0;
  f->max_number_of_locals=0;
  f->previous=compiler_frame;
  compiler_frame=f;
}

void pop_local_variables(int level)
{
  while(compiler_frame->current_number_of_locals > level)
  {
    int e;
    e=--(compiler_frame->current_number_of_locals);
    free_string(compiler_frame->variable[e].name);
    free_string(compiler_frame->variable[e].type);
  }
}


void pop_compiler_frame(void)
{
  struct compiler_frame *f;
  int e;
  f=compiler_frame;
#ifdef PIKE_DEBUG
  if(!f)
    fatal("Popping out of compiler frames\n");
#endif

  pop_local_variables(0);
  if(f->current_type)
    free_string(f->current_type);
  
  if(f->current_return_type)
    free_string(f->current_return_type);

  compiler_frame=f->previous;
  free((char *)f);
}


#define GET_STORAGE_CACHE_SIZE 1024
static struct get_storage_cache
{
  INT32 oid, pid, offset;
} get_storage_cache[GET_STORAGE_CACHE_SIZE];

int low_get_storage(struct program *o, struct program *p)
{
  INT32 oid,pid, offset;
  unsigned INT32 hval;
  if(!o) return 0;
  oid=o->id;
  pid=p->id;
  hval=oid*9248339 + pid;
  hval%=GET_STORAGE_CACHE_SIZE;
#ifdef PIKE_DEBUG
  if(hval>GET_STORAGE_CACHE_SIZE)
    fatal("hval>GET_STORAGE_CACHE_SIZE");
#endif
  if(get_storage_cache[hval].oid == oid &&
     get_storage_cache[hval].pid == pid)
  {
    offset=get_storage_cache[hval].offset;
  }else{
    INT32 e;
    offset=-1;
    for(e=0;e<o->num_inherits;e++)
    {
      if(o->inherits[e].prog==p)
      {
	offset=o->inherits[e].storage_offset;
	break;
      }
    }

    get_storage_cache[hval].oid=oid;
    get_storage_cache[hval].pid=pid;
    get_storage_cache[hval].offset=offset;
  }

  return offset;
}

char *get_storage(struct object *o, struct program *p)
{
  int offset= low_get_storage(o->prog, p);
  if(offset == -1) return 0;
  return o->storage + offset;
}

struct program *low_program_from_function(struct program *p,
					  INT32 i)
{
  struct svalue *f;
  struct identifier *id=ID_FROM_INT(p, i);
  if(!IDENTIFIER_IS_CONSTANT(id->identifier_flags)) return 0;
  if(id->func.offset==-1) return 0;
  f=PROG_FROM_INT(p,i)->constants + id->func.offset;
  if(f->type!=T_PROGRAM) return 0;
  return f->u.program;
}

struct program *program_from_function(struct svalue *f)
{
  struct identifier *id;
  if(f->type != T_FUNCTION) return 0;
  if(f->subtype == FUNCTION_BUILTIN) return 0;
  if(!f->u.object->prog) return 0;
  return low_program_from_function(f->u.object->prog, f->subtype);
}

struct program *program_from_svalue(struct svalue *s)
{
  switch(s->type)
  {
    case T_OBJECT:
    {
      struct program *p;
      push_svalue(s);
      f_object_program(1);
      p=program_from_svalue(sp-1);
      pop_stack();
      return p; /* We trust that there is a reference somewhere... */
    }

  case T_FUNCTION:
    return program_from_function(s);
  case T_PROGRAM:
    return s->u.program;
  default:
    return 0;
  }
}

#define FIND_CHILD_HASHSIZE 5003
struct find_child_cache_s
{
  INT32 pid,cid,id;
};

static struct find_child_cache_s find_child_cache[FIND_CHILD_HASHSIZE];

int find_child(struct program *parent, struct program *child)
{
  unsigned INT32 h=(parent->id  * 9248339 + child->id);
  h= h % FIND_CHILD_HASHSIZE;
#ifdef PIKE_DEBUG
  if(h>=FIND_CHILD_HASHSIZE)
    fatal("find_child failed to hash within boundaries.\n");
#endif  
  if(find_child_cache[h].pid == parent->id &&
     find_child_cache[h].cid == child->id)
  {
    return find_child_cache[h].id;
  }else{
    INT32 i;
    for(i=0;i<parent->num_identifier_references;i++)
    {
      if(low_program_from_function(parent, i)==child)
      {
	find_child_cache[h].pid=parent->id;
	find_child_cache[h].cid=child->id;
	find_child_cache[h].id=i;
	return i;
      }
    }
  }
  return -1;
}

void yywarning(char *fmt, ...) ATTRIBUTE((format(printf,1,2)))
{
  char buf[4711];
  va_list args;
  va_start(args,fmt);
  VSPRINTF(buf, fmt, args);
  va_end(args);

  if(strlen(buf)>sizeof(buf))
    fatal("Buffer overfloat in yywarning!\n");

  if(get_master())
  {
    ref_push_string(lex.current_file);
    push_int(lex.current_line);
    push_text(buf);
    SAFE_APPLY_MASTER("compile_warning",3);
    pop_stack();
  }
}



/* returns 1 if a implements b */
static int low_implements(struct program *a, struct program *b)
{
  int e,num=0;
  struct pike_string *s=findstring("__INIT");
  for(e=0;e<b->num_identifier_references;e++)
  {
    struct identifier *bid=ID_FROM_INT(b,e);
    int i;
    if(s==bid->name) continue;
    i=find_shared_string_identifier(bid->name,a);
    if(i!=-1)
    {
      if(!match_types(ID_FROM_INT(a,i)->type, bid->type))
	return 0;
      num++;
    }
  }
  return num;
}

#define IMPLEMENTS_CACHE_SIZE 4711
struct implements_cache_s { INT32 aid, bid, ret; };
static struct implements_cache_s implements_cache[IMPLEMENTS_CACHE_SIZE];

/* returns 1 if a implements b, but faster */
int implements(struct program *a, struct program *b)
{
  unsigned long hval;
  if(!a || !b) return -1;
  if(a==b) return 1;

  hval = a->id*9248339 + b->id;
  hval %= IMPLEMENTS_CACHE_SIZE;
#ifdef PIKE_DEBUG
  if(hval >= IMPLEMENTS_CACHE_SIZE)
    fatal("Implements_cache failed!\n");
#endif
  if(implements_cache[hval].aid==a->id && implements_cache[hval].bid==b->id)
  {
    return implements_cache[hval].ret;
  }
  /* Do it the tedious way */
  implements_cache[hval].aid=a->id;
  implements_cache[hval].bid=b->id;
  implements_cache[hval].ret=low_implements(a,b);
  return implements_cache[hval].ret;
}

#if 0
void f_encode_program(INT32 args)
{
  check_stack(20);
  f_version();
  push_int(p->flags);
  push_int(p->storage_needed);
  if(p->init || p->exit || p->gc_marked || p->gc_check)
    error("Cannot encode C programs.\n");
  push_int(total_size);
  push_string(make_shared_binary_string(p->program, p->num_program));
  push_string(make_shared_binary_string(p->linenumbers, p->num_linenumbers));
  push_string(make_shared_binary_string((char *)p->identifier_index, p->num_identifier_index * sizeof(unsigned short*)));
  push_string(make_shared_binary_string((char *)p->variable_index, p->num_variable_index * sizeof(unsigned short*)));
  push_string(make_shared_binary_string((char *)p->identifier_references, p->num_identifier_references * sizeof(struct reference)));
  check_stack(p->num_strings);
  for(e=0;e<p->num_strings;e++) ref_push_string(p->strings[e]);
  f_aggregate(p->num_strings);

  check_stack(p->num_inherits * 2);
  for(e=1;e<p->num_strings;e++)
  {
    ref_push_string(p->inherits[e].name);
  }

  f_aggregate(p->num_inherits);
  
  check_stack(NUM_LFUNS);
  for(e=0;e<NUM_LFUNS;e++) push_int(p->lfuns[e]);
  
  UNSET_ONERROR(tmp);

  free_mapping(data->encoded);
  pop_n_elems(args);
  push_string(low_free_buf(&data->buf));
}

#endif
