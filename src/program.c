/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: program.c,v 1.20 1997/02/11 07:20:15 hubbe Exp $");
#include "program.h"
#include "object.h"
#include "dynamic_buffer.h"
#include "pike_types.h"
#include "stralloc.h"
#include "las.h"
#include "language.h"
#include "lex.h"
#include "macros.h"
#include "fsort.h"
#include "error.h"
#include "docode.h"
#include "interpret.h"
#include "hashtable.h"
#include "main.h"
#include "gc.h"
#include "threads.h"
#include "constants.h"

#include <errno.h>
#include <fcntl.h>

/*
 * Define the size of the cache that is used for method lookup.
 */
#define FIND_FUNCTION_HASHSIZE 4711


#define FILE_STATE
#define PROGRAM_STATE

#define STRUCT
#include "compilation.h"
#undef STRUCT

#define DECLARE
#include "compilation.h"
#undef DECLARE

#undef FILE_STATE
#undef PROGRAM_STATE


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
};

struct program *first_program = 0;

struct program fake_program;

static int current_program_id=0;
static INT32 last_line = 0;
static INT32 last_pc = 0;
static struct pike_string *last_file = 0;
dynamic_buffer inherit_names;
dynamic_buffer used_modules;

void use_module(struct svalue *s)
{
  if( (1<<s->type) & (BIT_MAPPING | BIT_OBJECT))
  {
    assign_svalue_no_free((struct svalue *)
			  low_make_buf_space(sizeof(struct svalue),
					     &used_modules), s);
  }else{
    yyerror("Module is neither mapping nor object");
  }
}


static int low_find_shared_string_identifier(struct pike_string *name,
					     struct program *prog);

int find_module_identifier(struct pike_string *ident)
{
  JMP_BUF tmp;

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
      push_string(ident);
      ident->refs++;
      f_index(2);
      
      if(!IS_UNDEFINED(sp-1))
      {
/*	fprintf(stderr,"MOD: %s, %d %d\n",ident->str, current_line, sp[-1].type); */
	UNSETJMP(tmp);
	return 1;
      }
      pop_stack();
    }
  }
  UNSETJMP(tmp);

  {
    struct program_state *p;
    for(p=previous_program_state;p;p=p->previous)
    {
      INT32 i;
      if(previous_file_state &&
	 previous_file_state->previous_program_state==p->previous)
	break;

      i=low_find_shared_string_identifier(ident, &p->fake_program);
      if(i!=-1)
      {
	struct identifier *id;
	id=ID_FROM_INT(&p->fake_program, i);
	if(IDENTIFIER_IS_CONSTANT(id->flags))
	{
	  push_svalue(PROG_FROM_INT(&p->fake_program, i)->constants+
		      id->func.offset);
	  return 1;
	}else{
	  yyerror("Identifier is not a constant");
	  return 0;
	}
      }
    }
  }

  return 0;
}

/* This should be optimized */
struct program *id_to_program(INT32 id)
{
  struct program *p;
  if(id) 
    for(p=first_program;p;p=p->next)
      if(id==p->id)
	return p;
  return 0;
}

#define SETUP(X,Y,TYPE,AREA) \
   fake_program.X=(TYPE *)areas[AREA].s.str; \
   fake_program.Y=areas[AREA].s.len/sizeof(TYPE)

/*
 * This routine sets up the struct fake_program to work almost like a
 * normal program, but all pointers points to the program we are currently
 * compiling
 */
void setup_fake_program()
{
  fake_program.refs=0xffffff;
  SETUP(program, program_size, unsigned char, A_PROGRAM);
  SETUP(strings, num_strings, struct pike_string *, A_STRINGS);
  SETUP(inherits, num_inherits, struct inherit, A_INHERITS);
  SETUP(identifiers, num_identifiers, struct identifier, A_IDENTIFIERS);
  SETUP(identifier_references, num_identifier_references, struct reference, A_IDENTIFIER_REFERENCES);
  SETUP(constants, num_constants, struct svalue, A_CONSTANTS);
  SETUP(linenumbers, num_linenumbers, char, A_LINENUMBERS);

  fake_program.inherits[0].prog=&fake_program;
  fake_program.next=0;
  fake_program.prev=0;
/*
  fake_program.lfuns=0;
  fake_prog.num_lfuns=0;
*/
  fake_object.prog=&fake_program;
}

/* Here starts routines which are used to build new programs */

/*
 * Start building a new program
 */
void start_new_program()
{
  int e;

  threads_disabled++;
  if(local_variables)
    setup_fake_program();
#define PROGRAM_STATE
#define PUSH
#include "compilation.h"
#undef PUSH
#undef PROGRAM_STATE

  if(previous_program_state->fake_program.num_inherits)
    previous_program_state->fake_program.inherits[0].prog=
      &previous_program_state->fake_program;

  for(e=0; e<NUM_AREAS; e++) low_reinit_buf(areas + e);
  low_reinit_buf(& inherit_names);
  low_reinit_buf(& used_modules);
  fake_program.id = ++current_program_id;

  {
    struct inherit inherit;
    struct pike_string *name;
    
    inherit.prog=&fake_program;
    inherit.inherit_level=0;
    inherit.identifier_level=0;
    inherit.storage_offset=0;
    add_to_mem_block(A_INHERITS,(char *)&inherit,sizeof inherit);
    name=make_shared_string("this");
    low_my_binary_strcat((char *)&name,sizeof(name), &inherit_names);
  }
 

  {
    struct svalue tmp;
    tmp.type=T_MAPPING;
    tmp.u.mapping=get_builtin_constants();
    use_module(& tmp);
  }

  num_parse_error=0;
  local_variables=ALLOC_STRUCT(locals);
  local_variables->next=0;
  local_variables->current_number_of_locals=0;
  local_variables->max_number_of_locals=0;
  local_variables->current_type=0;
  local_variables->current_return_type=0;
}

static void low_free_program(struct program *p)
{
  unsigned INT16 e;
  for(e=0; e<p->num_strings; e++)
    free_string(p->strings[e]);

  for(e=0; e<p->num_identifiers; e++)
  {
    free_string(p->identifiers[e].name);
    free_string(p->identifiers[e].type);
  }

  for(e=0; e<p->num_constants; e++)
    free_svalue(p->constants+e);

  for(e=1; e<p->num_inherits; e++)
    free_program(p->inherits[e].prog);
}

void really_free_program(struct program *p)
{
  low_free_program(p);

  if(p->prev)
    p->prev->next=p->next;
  else
    first_program=p->next;

  if(p->next)
    p->next->prev=p->prev;

  free((char *)p);

  GC_FREE();
}

#ifdef DEBUG
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
  for(q=0;q<(int)p->num_identifier_indexes;q++)
  {
    e=p->identifier_index[q];
    fprintf(stderr,"%3d (%3d):",e,q);
    for(d=0;d<INHERIT_FROM_INT(p,e)->inherit_level;d++) fprintf(stderr,"  ");
    fprintf(stderr,"%s;\n", ID_FROM_INT(p,e)->name->str);
  }
}
#endif

static void toss_compilation_resources()
{
  struct pike_string **names;
  struct svalue *modules;
  int e;

  for (e=0; e<NUM_AREAS; e++) toss_buffer(areas+e);

  names=(struct pike_string **)inherit_names.s.str;
  e=inherit_names.s.len / sizeof(struct pike_string *);
  while(--e>=0) if(names[e]) free_string(names[e]);
  toss_buffer(& inherit_names);

  modules=(struct svalue *)used_modules.s.str;
  e=used_modules.s.len / sizeof(struct svalue);
  while(--e>=0) free_svalue(modules+e);
  toss_buffer(& used_modules);

  /* Clean up */
  while(local_variables)
  {
    struct locals *l;
    for(e=0;e<local_variables->current_number_of_locals;e++)
    {
      free_string(local_variables->variable[e].name);
      free_string(local_variables->variable[e].type);
    }
  
    if(local_variables->current_type)
      free_string(local_variables->current_type);

    if(local_variables->current_return_type)
      free_string(local_variables->current_return_type);

    l=local_variables->next;
    free((char *)local_variables);
    local_variables=l;
  }

  if(last_file)
  {
    free_string(last_file);
    last_file=0;
  }
}

/*
 * Something went wrong.
 * toss resources of program we were building
 */
void toss_current_program()
{
  setup_fake_program();
  low_free_program(&fake_program);
  toss_compilation_resources();
}

#ifdef DEBUG
void check_program(struct program *p)
{
  INT32 size,e;
  unsigned INT32 checksum;

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

  size=MY_ALIGN(sizeof(struct program));
  size+=MY_ALIGN(p->num_linenumbers);
  size+=MY_ALIGN(p->program_size);
  size+=MY_ALIGN(p->num_constants * sizeof(struct svalue));
  size+=MY_ALIGN(p->num_strings * sizeof(struct pike_string *));
  size+=MY_ALIGN(p->num_identifiers * sizeof(struct identifier));
  size+=MY_ALIGN(p->num_identifier_references * sizeof(struct reference));
  size+=MY_ALIGN(p->num_inherits * sizeof(struct inherit));

  size+=MY_ALIGN(p->num_identifier_indexes * sizeof(INT16));

  if(size > (INT32)p->total_size)
    fatal("Program size is in error.\n");

  size-=MY_ALIGN(p->num_identifier_indexes * sizeof(INT16));
  size+=MY_ALIGN(p->num_identifier_references * sizeof(INT16));

  if(size < (INT32)p->total_size)
    fatal("Program size is in error.\n");


#define CHECKRANGE(X,Y) if((char *)(p->X) < (char *)p || (char *)(p->X)> ((char *)p)+size) fatal("Program->%s is wrong.\n",Y)

  CHECKRANGE(program,"program");
  CHECKRANGE(strings,"strings");
  CHECKRANGE(inherits,"inherits");
  CHECKRANGE(identifier_references,"identifier_references");
  CHECKRANGE(identifiers,"identifier");
  CHECKRANGE(identifier_index,"identifier_index");
  CHECKRANGE(constants,"constants");
  CHECKRANGE(linenumbers,"linenumbers");

  checksum=hashmem(p->program, p->program_size, p->program_size) +
    hashmem((unsigned char*)p->linenumbers,p->num_linenumbers,p->num_linenumbers);

  if(!checksum) checksum=1;

  if(!p->checksum)
  {
    p->checksum=checksum;
  }else{
    if(p->checksum != checksum)
      fatal("Someone changed a program!!!\n");
  }

  for(e=0;e<(int)p->num_constants;e++)
  {
    check_svalue(p->constants + e);
  }

  for(e=0;e<(int)p->num_strings;e++)
    check_string(p->strings[e]);

  for(e=0;e<(int)p->num_identifiers;e++)
  {
    check_string(p->identifiers[e].name);
    check_string(p->identifiers[e].type);

    if(p->identifiers[e].flags & ~15)
      fatal("Unknown flags in identifier flag field.\n");

    if(p->identifiers[e].run_time_type!=T_MIXED)
      check_type(p->identifiers[e].run_time_type);
  }

  for(e=0;e<(int)p->num_identifier_references;e++)
  {
    if(p->identifier_references[e].inherit_offset > p->num_inherits)
      fatal("Inherit offset is wrong!\n");

    if(p->identifier_references[e].identifier_offset >
       p->inherits[p->identifier_references[e].inherit_offset].prog->num_identifiers)
      fatal("Identifier offset is wrong!\n");
  }

  for(e=0;e<(int)p->num_identifier_indexes;e++)
  {
    if(p->identifier_index[e] > p->num_identifier_references)
      fatal("Program->identifier_indexes[%ld] is wrong\n",(long)e);
  }

  for(e=0;e<(int)p->num_inherits;e++)
  {
    if(p->inherits[e].storage_offset < 0)
      fatal("Inherit->storage_offset is wrong.\n");
  }
}
#endif

/* internal function to make the index-table */
static int funcmp(const void *a,const void *b)
{
  return
    my_order_strcmp(ID_FROM_INT(&fake_program, *(unsigned short *)a)->name,
		    ID_FROM_INT(&fake_program, *(unsigned short *)b)->name);
}

/*
 * Finish this program, returning the newly built program
 */

#define INS_BLOCK(PTR,PTRS,TYPE,AREA) \
prog->PTR=(TYPE *)p; \
if((prog->PTRS = areas[AREA].s.len/sizeof(TYPE))) \
{ \
  MEMCPY(p,areas[AREA].s.str, areas[AREA].s.len); \
  p+=MY_ALIGN(areas[AREA].s.len); \
}

struct program *end_program()
{
  struct pike_string **names;
  int size, i,e,t;
  char *p;
  struct program *prog;

  /*
   * Define the __INIT function, but only if there was any code
   * to initialize.
   */
  if (init_node)
  {
    union idptr tmp;
    struct pike_string *s;
    s=make_shared_string("__INIT");
    tmp.offset=PC;
    ins_byte(0, A_PROGRAM); /* num locals */
    ins_byte(0, A_PROGRAM); /* num args */
    dooptcode(s,mknode(F_ARG_LIST,init_node,mknode(F_RETURN,mkintnode(0),0)),0);
    define_function(s,
		    function_type_string,
		    0,  /* ID_STATIC, */
		    IDENTIFIER_PIKE_FUNCTION,
		    & tmp);
    free_string(s);
    init_node=0;
  }

  if (num_parse_error > 0)
  {
    toss_current_program();
    prog=0;
  }else{
    setup_fake_program();
    size = MY_ALIGN(sizeof (struct program));
    for (i=0; i<NUM_AREAS; i++) size += MY_ALIGN(areas[i].s.len);
    size+=MY_ALIGN(fake_program.num_identifier_references * sizeof(unsigned short));

    p = (char *)xalloc(size);
    prog = (struct program *)p;
    *prog = fake_program;
    prog->total_size = size;
    prog->refs = 1;
    p += MY_ALIGN(sizeof (struct program));

    INS_BLOCK(program,program_size,unsigned char,A_PROGRAM);
    INS_BLOCK(linenumbers,num_linenumbers,char,A_LINENUMBERS);
    INS_BLOCK(identifiers,num_identifiers,struct identifier,A_IDENTIFIERS);
    INS_BLOCK(identifier_references,num_identifier_references,struct reference,A_IDENTIFIER_REFERENCES);
    INS_BLOCK(strings,num_strings,struct pike_string *,A_STRINGS);
    INS_BLOCK(inherits,num_inherits,struct inherit,A_INHERITS);
    INS_BLOCK(constants,num_constants,struct svalue,A_CONSTANTS);

    /* Ok, sort for binsearch */
    prog->identifier_index=(unsigned short *)p;
    for(e=i=0;i<(int)prog->num_identifier_references;i++)
    {
      struct reference *funp;
      struct identifier *fun;
      funp=prog->identifier_references+i;
      if(funp->flags & (ID_HIDDEN|ID_STATIC)) continue;
      if(funp->flags & ID_INHERITED)
      {
	if(funp->flags & ID_PRIVATE) continue;
	fun=ID_FROM_PTR(prog, funp);
	if(fun->func.offset == -1) continue; /* prototype */

	/* check for multiple definitions */
	for(t=0;t>=0 && t<(int)prog->num_identifier_references;t++)
	{
	  struct reference *funpb;
	  struct identifier *funb;

	  if(t==i) continue;
	  funpb=prog->identifier_references+t;
	  if(funpb->flags & (ID_HIDDEN|ID_STATIC)) continue;
	  if((funpb->flags & ID_INHERITED) && t<i) continue;
	  funb=ID_FROM_PTR(prog,funpb);
	  if(funb->func.offset == -1) continue; /* prototype */
	  if(fun->name==funb->name) t=-10;
	}
	if(t<0) continue;
      }
      prog->identifier_index[e]=i;
      e++;
    }
    prog->num_identifier_indexes=e;
    fsort((void *)prog->identifier_index, e,sizeof(unsigned short),(fsortfun)funcmp);

    p+=MY_ALIGN(prog->num_identifier_indexes*sizeof(unsigned short));

    toss_compilation_resources();

    prog->inherits[0].prog=prog;
    prog->prev=0;
    if(prog->next=first_program)
      first_program->prev=prog;
    first_program=prog;

    for(i=0;i<NUM_LFUNS;i++)
      prog->lfuns[i]=find_identifier(lfun_names[i],prog);

#ifdef DEBUG
    check_program(prog);
    if(l_flag)
      dump_program_desc(prog);
#endif

    GC_ALLOC();
  }

#define PROGRAM_STATE
#define POP
#include "compilation.h"
#undef POP
#undef PROGRAM_STATE
  if(fake_program.num_inherits)
    fake_program.inherits[0].prog=&fake_program;
  threads_disabled--;
  free_all_nodes();
  return prog;
}

/*
 * Allocate needed for this program in the object structure.
 * An offset to the data is returned.
 */
SIZE_T add_storage(SIZE_T size)
{
  SIZE_T offset;
  offset=fake_program.storage_needed;
  size=MY_ALIGN(size);
  fake_program.storage_needed += size;
  return offset;
}

/*
 * set a callback used to initialize clones of this program
 * the init function is called at clone time
 */
void set_init_callback(void (*init)(struct object *))
{
  fake_program.init=init;
}

/*
 * set a callback used to de-initialize clones of this program
 * the exit function is called at destruct
 */
void set_exit_callback(void (*exit)(struct object *))
{
  fake_program.exit=exit;
}


int low_reference_inherited_identifier(int e,struct pike_string *name)
{
  struct reference funp;
  struct program *p;
  int i,d;

  p=fake_program.inherits[e].prog;
  i=find_shared_string_identifier(name,p);
  if(i==-1) return i;

  if(p->identifier_references[i].flags & ID_HIDDEN)
    return -1;

  if(ID_FROM_INT(p,i)->func.offset == -1) /* prototype */
    return -1;

  funp=p->identifier_references[i];
  funp.inherit_offset+=e;
  funp.flags|=ID_HIDDEN;

  for(d=0;d<(int)fake_program.num_identifier_references;d++)
  {
    struct reference *fp;
    fp=fake_program.identifier_references+d;

    if(!MEMCMP((char *)fp,(char *)&funp,sizeof funp)) return d;
  }

  add_to_mem_block(A_IDENTIFIER_REFERENCES,(char *)&funp,sizeof funp);
  return fake_program.num_identifier_references;
}



int reference_inherited_identifier(struct pike_string *super_name,
				   struct pike_string *function_name)
{
  struct pike_string **names;
  int e,i;

#ifdef DEBUG
  if(function_name!=debug_findstring(function_name))
    fatal("reference_inherited_function on nonshared string.\n");
#endif

  names=(struct pike_string **)inherit_names.s.str;
  setup_fake_program();

  for(e=fake_program.num_inherits-1;e>0;e--)
  {
    if(fake_program.inherits[e].inherit_level!=1) continue;
    if(!names[e]) continue;

    if(super_name)
    {
      int l;
      l=names[e]->len;
      if(l<super_name->len) continue;
      if(strncmp(super_name->str,
		 names[e]->str+l-super_name->len,
		 super_name->len))
	continue;
    }

    i=low_reference_inherited_identifier(e,function_name);
    if(i==-1) continue;
    return i;
  }
  return -1;
}

void rename_last_inherit(struct pike_string *n)
{
  struct pike_string **names;
  int e;
  names=(struct pike_string **)inherit_names.s.str;
  e=inherit_names.s.len / sizeof(struct pike_string *);
  free_string(names[e-1]);
  copy_shared_string(names[e-1],n);
}

/*
 * make this program inherit another program
 */
void do_inherit(struct program *p,INT32 flags, struct pike_string *name)
{
  int e, inherit_offset, storage_offset;
  struct inherit inherit;
  struct pike_string *s;

  setup_fake_program();

  inherit_offset = fake_program.num_inherits;

  storage_offset=fake_program.storage_needed;
  add_storage(p->storage_needed);

  for(e=0; e<(int)p->num_inherits; e++)
  {
    inherit=p->inherits[e];
    inherit.prog->refs++;
    inherit.identifier_level += fake_program.num_identifier_references;
    inherit.storage_offset += storage_offset;
    inherit.inherit_level ++;
    add_to_mem_block(A_INHERITS,(char *)&inherit,sizeof inherit);
    
    low_my_binary_strcat((char *)&name,sizeof(name),&inherit_names);
    if(name)
    {
      reference_shared_string(name);
      name=0;
    }
  }

  for (e=0; e < (int)p->num_identifier_references; e++)
  {
    struct reference fun;
    struct pike_string *name;

    fun = p->identifier_references[e]; /* Make a copy */

    name=ID_FROM_PTR(p,&fun)->name;
    fun.inherit_offset += inherit_offset;

    if (fun.flags & ID_NOMASK)
    {
      int n;
      n = isidentifier(name);
      if (n != -1 && ID_FROM_INT(&fake_program,n)->func.offset != -1)
	my_yyerror("Illegal to redefine 'nomask' function/variable \"%s\"",name->str);
    }

    if(fun.flags & ID_PRIVATE) fun.flags|=ID_HIDDEN;

    if (fun.flags & ID_PUBLIC)
      fun.flags |= flags & ~ID_PRIVATE;
    else
      fun.flags |= flags;

    fun.flags |= ID_INHERITED;
    add_to_mem_block(A_IDENTIFIER_REFERENCES, (char *)&fun, sizeof fun);
  }

  /* Ska det h{r vara s} h{r? */
  s=findstring("__INIT");
  if(s)
  {
    if(-1 != find_shared_string_identifier(s,p))
    {
      e=reference_inherited_identifier(0, s);
      init_node=mknode(F_ARG_LIST,
		       init_node,
		       mkcastnode(void_type_string,
				  mkapplynode(mkidentifiernode(e),0)));
    }
  }
}

void simple_do_inherit(struct pike_string *s, INT32 flags,struct pike_string *name)
{
  reference_shared_string(s);
  push_string(s);
  reference_shared_string(current_file);
  push_string(current_file);
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
  do_inherit(sp[-1].u.program, flags, s);
  free_string(s);
  pop_stack();
}

/*
 * Return the index of the identifier found, otherwise -1.
 */
int isidentifier(struct pike_string *s)
{
  INT32 e;
  setup_fake_program();
  for(e=0;e<(int)fake_program.num_identifier_references;e++)
  {
    if(fake_program.identifier_references[e].flags & ID_HIDDEN) continue;
    
    if(ID_FROM_INT(& fake_program, e)->name == s)
      return e;
  }
  return -1;
}

/* argument must be a shared string */
int define_variable(struct pike_string *name,
		    struct pike_string *type,
		    INT32 flags)
{
  int n;

#ifdef DEBUG
  if(name!=debug_findstring(name))
    fatal("define_variable on nonshared string.\n");
#endif

  if(type == void_type_string)
    yyerror("Variables can't be of type void");
  
  setup_fake_program();
  n = isidentifier(name);

  if(n != -1)
  {
    setup_fake_program();

    if (IDENTIFIERP(n)->flags & ID_NOMASK)
      my_yyerror("Illegal to redefine 'nomask' variable/functions \"%s\"", name->str);

    if(PROG_FROM_INT(& fake_program, n) == &fake_program)
      my_yyerror("Variable '%s' defined twice.",name->str);

    if(ID_FROM_INT(& fake_program, n)->type != type)
      my_yyerror("Illegal to redefine inherited variable with different type.");

    if(ID_FROM_INT(& fake_program, n)->flags != flags)
      my_yyerror("Illegal to redefine inherited variable with different type.");

  } else {
    struct identifier dummy;
    struct reference ref;

    copy_shared_string(dummy.name, name);
    copy_shared_string(dummy.type, type);
    dummy.flags = 0;
    dummy.run_time_type=compile_type_to_runtime_type(type);

    if(dummy.run_time_type == T_FUNCTION)
      dummy.run_time_type = T_MIXED;

    dummy.func.offset=add_storage(dummy.run_time_type == T_MIXED ?
				  sizeof(struct svalue) :
				  sizeof(union anything));

    ref.flags=flags;
    ref.identifier_offset=areas[A_IDENTIFIERS].s.len / sizeof dummy;
    ref.inherit_offset=0;

    add_to_mem_block(A_IDENTIFIERS, (char *)&dummy, sizeof dummy);
    fake_program.num_identifiers ++;

    n=areas[A_IDENTIFIER_REFERENCES].s.len / sizeof ref;
    add_to_mem_block(A_IDENTIFIER_REFERENCES, (char *)&ref, sizeof ref);
    fake_program.num_identifier_references ++;

  }

  return n;
}

int add_constant(struct pike_string *name,
		 struct svalue *c,
		 INT32 flags)
{
  int n;

#ifdef DEBUG
  if(name!=debug_findstring(name))
    fatal("define_constant on nonshared string.\n");
#endif

  setup_fake_program();
  n = isidentifier(name);

  if(n != -1)
  {
    setup_fake_program();

    if (IDENTIFIERP(n)->flags & ID_NOMASK)
      my_yyerror("Illegal to redefine 'nomask' identifier \"%s\"", name->str);

    if(PROG_FROM_INT(& fake_program, n) == &fake_program)
      my_yyerror("Identifier '%s' defined twice.",name->str);
  } else {
    struct identifier dummy;
    struct reference ref;

    copy_shared_string(dummy.name, name);
    dummy.type = get_type_of_svalue(c);
    
    dummy.flags = IDENTIFIER_CONSTANT;
    dummy.run_time_type=c->type;
    
    dummy.func.offset=store_constant(c, 0);

    ref.flags=flags;
    ref.identifier_offset=areas[A_IDENTIFIERS].s.len / sizeof dummy;
    ref.inherit_offset=0;

    add_to_mem_block(A_IDENTIFIERS, (char *)&dummy, sizeof dummy);
    fake_program.num_identifiers ++;

    n=areas[A_IDENTIFIER_REFERENCES].s.len / sizeof ref;
    add_to_mem_block(A_IDENTIFIER_REFERENCES, (char *)&ref, sizeof ref);
    fake_program.num_identifier_references ++;

  }

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


int end_class(char *name, INT32 flags)
{
  INT32 ret;
  struct svalue tmp;
  tmp.type=T_PROGRAM;
  tmp.subtype=0;
  tmp.u.program=end_program();
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

  i=isidentifier(name);

  setup_fake_program();

  if(i >= 0)
  {
    /* already defined */

    funp=ID_FROM_INT(&fake_program, i);
    ref=fake_program.identifier_references[i];

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

    /* it's just another prototype, don't define anything */
    if(!func || func->offset == -1) return i;

    if((ref.flags & ID_NOMASK) &&
       !(funp->func.offset == -1))
    {
      my_yyerror("Illegal to redefine 'nomask' function %s.",name->str);
    }

    /* We modify the old definition if it is in this program */
    if(ref.inherit_offset==0)
    {
      if(func)
	funp->func = *func;
      else
	funp->func.offset = -1;

      funp->flags=function_flags;
    }else{
      /* Otherwise we make a new definition */
      copy_shared_string(fun.name, name);
      copy_shared_string(fun.type, type);

      fun.run_time_type=T_FUNCTION;

      fun.flags=function_flags;

      if(func)
	fun.func = *func;
      else
	fun.func.offset = -1;

      ref.identifier_offset=fake_program.num_identifiers;
      add_to_mem_block(A_IDENTIFIERS, (char *)&fun, sizeof(fun));
    }

    ref.inherit_offset = 0;
    ref.flags = flags;
    fake_program.identifier_references[i]=ref;
  }else{
    /* define it */

    copy_shared_string(fun.name, name);
    copy_shared_string(fun.type, type);

    fun.flags=function_flags;

    fun.run_time_type=T_FUNCTION;

    if(func)
      fun.func = *func;
    else
      fun.func.offset = -1;

    i=fake_program.num_identifiers;
    add_to_mem_block(A_IDENTIFIERS, (char *)&fun, sizeof(fun));

    ref.flags = flags;
    ref.identifier_offset = i;
    ref.inherit_offset = 0;

    i=fake_program.num_identifier_references;
    add_to_mem_block(A_IDENTIFIER_REFERENCES, (char *)&ref, sizeof ref);

  }
  return i;
}


/*
 * lookup the number of a function in a program given the name in
 * a shared_string
 */
static int low_find_shared_string_identifier(struct pike_string *name,
					     struct program *prog)
{
  int max,min,tst;
  struct reference *funp;
  struct identifier *fun;
  unsigned short *funindex;

  funindex = prog->identifier_index;
  if(funindex)
  {
    max = prog->num_identifier_indexes;
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
    int i,t;
    for(i=0;i<(int)prog->num_identifier_references;i++)
    {
      funp = prog->identifier_references + i;
      if(funp->flags & ID_HIDDEN) continue;
      fun = ID_FROM_PTR(prog, funp);
      if(fun->func.offset == -1) continue; /* Prototype */
      if(!is_same_string(fun->name,name)) continue;
      if(funp->flags & ID_INHERITED)
      {
        if(funp->flags & ID_PRIVATE) continue;
	for(t=0; t>=0 && t<(int)prog->num_identifier_references; t++)
	{
	  if(t == i) continue;

	  if(is_same_string(fun->name, ID_FROM_INT(prog, i)->name))
	    t=-10;
	}
	if(t < 0) continue;
      }
      return i;
    }
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
  if(prog!=&fake_program)
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
  if(!prog)
    error("Identifier lookup in destructed object.\n");
  n=findstring(name);
  if(!n) return -1;
  return find_shared_string_identifier(n,prog);
}

int store_prog_string(struct pike_string *str)
{
  unsigned int i;
  struct pike_string **p;

  p = (struct pike_string **)areas[A_STRINGS].s.str;

  for (i=0;i<areas[A_STRINGS].s.len / sizeof str;i++)
    if (p[i] == str)
      return i;

  reference_shared_string(str);
  add_to_mem_block(A_STRINGS, (char *)&str, sizeof str);
  return i;
}

int store_constant(struct svalue *foo, int equal)
{
  struct svalue *s,tmp;
  unsigned int e;
  s=(struct svalue *)areas[A_CONSTANTS].s.str;

  for(e=0;e<areas[A_CONSTANTS].s.len / sizeof(struct svalue);e++)
    if(equal ? is_equal(s+e,foo) : is_eq(s+e,foo))
      return e;

  assign_svalue_no_free(&tmp,foo);
  add_to_mem_block(A_CONSTANTS,(char *)&tmp,sizeof(struct svalue));
  return e;
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
  if(last_file) { free_string(last_file); last_file=0; }
  last_pc=last_line=0;
}

static void insert_small_number(int a,int area)
{
  if(a>-127 && a<127)
  {
    ins_byte(a,area);
  }else if(a>=-32768 && a<32768){
    ins_signed_byte(-127,area);
    ins_short(a,area);
  }else{
    ins_signed_byte(-128,area);
    ins_int(a,area);
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
      ins_byte(127,A_LINENUMBERS);
      for(tmp=current_file->str; *tmp; tmp++) ins_byte(*tmp,A_LINENUMBERS);
      ins_byte(0,A_LINENUMBERS);
      copy_shared_string(last_file, current_file);
    }
    insert_small_number(PC-last_pc,A_LINENUMBERS);
    insert_small_number(current_line-last_line,A_LINENUMBERS);
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
  char *file;
  INT32 off,line,offset;
  char *cnt;

  if (prog == 0) return "Unkown program";
  offset = pc - prog->program;

  if(prog == & fake_program)
  {
    linep[0]=0;
    return "Optimizer";
  }

#ifdef DEBUG
  if (offset > (INT32)prog->program_size || offset<0)
    fatal("Illegal offset %ld in program.\n", (long)offset);
#endif

  cnt=prog->linenumbers;
  off=line=0;
  file="Line not found";
  while(cnt < prog->linenumbers + prog->num_linenumbers)
  {
    if(*cnt == 127)
    {
      file=cnt+1;
      cnt=file+strlen(file)+1;
    }
    off+=get_small_number(&cnt);
    if(off > offset) break;
    line+=get_small_number(&cnt);
  }
  linep[0]=line;
  return file;
}

void my_yyerror(char *fmt,...)
{
  va_list args;
  char buf[1000];
  va_start(args,fmt);
  VSPRINTF(buf,fmt,args);

  if((long)strlen(buf) >= (long)sizeof(buf))
    fatal("Buffer overflow in my_yyerror.");

  yyerror(buf);
  va_end(args);
}

/*
 * Compile an PIKE file. Input is supposed to be initalized already.
 */
void compile()
{
  void free_all_local_names();
  int yyparse();

  start_line_numbering();

  num_parse_error = 0;
  init_node=0;

  yyparse();  /* Parse da program */
  free_all_local_names();
}

struct program *compile_file(struct pike_string *file_name)
{
  int fd;
  struct program *p;

  while(1)
  {
    fd=open(file_name->str,O_RDONLY);
    if(fd >= 0) break;
    if(errno != EINTR)
    {
#ifdef HAVE_STRERROR
      error("Couldn't open file '%s'. (%s)\n",file_name->str,strerror(errno));
#else
      error("Couldn't open file '%s'. (ERRNO=%d)\n",file_name->str,errno);
#endif
    }
  }

#define FILE_STATE
#define PUSH
#include "compilation.h"
#undef PUSH

  start_new_file(fd,file_name);
  start_new_program();
  compile();
  p=end_program();
  end_new_file();

#define POP
#include "compilation.h"
#undef POP
#undef FILE_STATE

  if(!p) error("Failed to compile %s.\n",file_name->str);
  return p;
}

struct program *compile_string(struct pike_string *prog,
			       struct pike_string *name)
{
  struct program *p;

#define FILE_STATE
#define PUSH
#include "compilation.h"
#undef PUSH

  start_new_string(prog->str,prog->len,name);
  start_new_program();
  compile();
  p=end_program();
  end_new_file();

#define POP
#include "compilation.h"
#undef POP
#undef FILE_STATE

  if(!p) error("Compilation failed.\n");
  return p;
}

void add_function(char *name,void (*cfun)(INT32),char *type,INT16 flags)
{
  struct pike_string *name_tmp,*type_tmp;
  union idptr tmp;
  
  name_tmp=make_shared_string(name);
  type_tmp=parse_type(type);

  if(cfun)
  {
    tmp.c_fun=cfun;
    define_function(name_tmp,
		    type_tmp,
		    flags,
		    IDENTIFIER_C_FUNCTION,
		    &tmp);
  }else{
    define_function(name_tmp,
		    type_tmp,
		    flags,
		    IDENTIFIER_C_FUNCTION,
		    0);
  }
  free_string(name_tmp);
  free_string(type_tmp);
}

#ifdef DEBUG
void check_all_programs()
{
  struct program *p;
  for(p=first_program;p;p=p->next)
    check_program(p);
}
#endif

void cleanup_program()
{
#ifdef FIND_FUNCTION_HASHSIZE
  int e;
  for(e=0;e<FIND_FUNCTION_HASHSIZE;e++)
  {
    if(cache[e].name)
    {
      free_string(cache[e].name);
      cache[e].name=0;
    }
  }
#endif
}

#ifdef GC2

void gc_mark_program_as_referenced(struct program *p)
{
  if(gc_mark(p))
    gc_mark_svalues(p->constants, p->num_constants);
}

void gc_check_all_programs()
{
  struct program *p;
  for(p=first_program;p;p=p->next)
  {
    gc_check_svalues(p->constants, p->num_constants);

    if(d_flag)
    {
      int e;
      for(e=0;e<(int)p->num_strings;e++)
	gc_check(p->strings[e]);

      for(e=0;e<(int)p->num_identifiers;e++)
      {
	gc_check(p->identifiers[e].name);
	gc_check(p->identifiers[e].type);
      }
    }
  }
}

void gc_mark_all_programs()
{
  struct program *p;
  for(p=first_program;p;p=p->next)
    if(gc_is_referenced(p))
      gc_mark_program_as_referenced(p);
}

void gc_free_all_unreferenced_programs()
{
  struct program *p,*next;

  for(p=first_program;p;p=next)
  {
    if(gc_do_free(p))
    {
      p->refs++;
      free_svalues(p->constants, p->num_constants, -1);
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
