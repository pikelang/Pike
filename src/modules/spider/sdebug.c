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
#include "config.h"
#include "builtin_functions.h"
#include "program.h"

void program_name(struct program *p)
{
  char *f;
  p->refs++;
  push_program(p);
  APPLY_MASTER("program_name", 1);
  if(sp[-1].type == T_STRING)
    return;
  pop_stack();
  f=(char *)(p->linenumbers+1);

  if(!p->linenumbers || !strlen(f))
    push_text("Unknown program");

  push_text(f);
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
      program_name(o->prog);
    else 
      push_string(make_shared_binary_string("No program (Destructed?)",24));
    push_int(o->refs);
    f_aggregate(2);
    ++n;
    o=o->next;
  }
  f_aggregate(n);
}

