#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"
#include "config.h"
#include "program.h"

#ifdef DEBUG
extern void dump_stralloc_strings(void);
void f__dump_string_table(INT32 args)
{
  pop_n_elems(args);
  dump_stralloc_strings();
  push_int(0);
}

void f__string_debug(INT32 args)
{
  pop_n_elems(args);
  push_string(add_string_status(args));
}
#endif
static char *program_name(struct program *p)
{
  char *f;
  f=(char *)(p->linenumbers+1);
  if(!p->linenumbers || !strlen(f))
  {
    p->refs++;
    push_program(p);
    APPLY_MASTER("program_name", 1);
    if(sp[-1].type == T_STRING)
      return 0;
    pop_stack();
    f="Unknown program";
  }
  return f;
}

void f__dump_obj_table(INT32 args)
{
  struct object *o;
  int n=0;
  pop_n_elems(args);
  o=first_object;
  while(o) 
  { 
    if(o->prog)
    {
      char *f;
      f=program_name(o->prog);
      if(f)
	push_string(make_shared_string(f));
    } else {
      push_string(make_shared_binary_string("No program (Destructed?)",24));
    }
    push_int(o->refs);
    f_aggregate(2);
    ++n;
    o=o->next; 
  }
  f_aggregate(n);
}

void f__num_objects(INT32 args)
{
  struct object *o;
  int n=0;
  pop_n_elems(args);
  o=first_object;
  while(o) { ++n; o=o->next; }
  push_int(n);
}

void f__num_mappings(INT32 args)
{
  extern struct mapping *first_mapping;
  struct mapping *o;
  int n=0;
  pop_n_elems(args);
  o=first_mapping;
  while(o) { ++n; o=o->next; }
  push_int(n);
}

void f__num_arrays(INT32 args)
{
  extern struct array empty_array;
  struct array *o;
  int n=0;
  pop_n_elems(args);
  o=(&empty_array)->next;
  while(o && o != &empty_array) { ++n; o=o->next; }
  push_int(n+1);
}

void f__num_dest_objects(INT32 args)
{
  struct object *o;
  int n=0;
  pop_n_elems(args);
  o=first_object;
  while(o) { if(!o->prog) n++; o=o->next; }
  push_int(n);
}

