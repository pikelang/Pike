#include "global.h"
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "macros.h"

#include <regexp.h>

struct regexp_glue
{
  struct regexp *regexp;
};

#define THIS ((struct regexp_glue *)(fp->current_storage))

static void do_free()
{
  if(THIS->regexp)
  {
    free((char *)THIS->regexp);
    THIS->regexp=0;
  }
}

static void regexp_create(INT32 args)
{
  do_free();
  if(args)
  {
    if(sp[-args].type != T_STRING)
      error("Bad argument 1 to regexp->create()\n");

    THIS->regexp=regcomp(sp[-args].u.string->str, 0);
  }
}

static void regexp_match(INT32 args)
{
  int i;
  if(!args)
    error("Too few arguments to regexp->match()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to regexp->match()\n");

  i=regexec(THIS->regexp, sp[-args].u.string->str);
  pop_n_elems(args);
  push_int(i);
}

static void regexp_split(INT32 args)
{
  struct lpc_string *s;
  struct regexp *r;
  if(!args)
    error("Too few arguments to regexp->split()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to regexp->split()\n");

  s=sp[-args].u.string;
  if(regexec(r=THIS->regexp, s->str))
  {
    int i, j;
    s->refs++;
    pop_n_elems(args);
    j=0;
    for(i=1;i<NSUBEXP;i++)
    {
      if(!r->startp[i] || !r->endp[i]) break;
      push_string(make_shared_binary_string(r->startp[i],
					    r->endp[i]-r->startp[i]));
      j++;
    }
    push_array(aggregate_array(j,T_STRING));
    free_string(s);
  }else{
    pop_n_elems(args);
    push_int(0);
  }
}

static void init_regexp_glue(char *foo, struct object *o)
{
  THIS->regexp=0;
}

static void exit_regexp_glue(char *foo, struct object *o)
{
  do_free();
}


void init_regexp_efuns(void) {}
void exit_regexp(void) {}

void init_regexp_programs(void)
{
  start_new_program();
  add_storage(sizeof(struct regexp_glue));
  
  add_function("create",regexp_create,"function(void|string:void)",0);
  add_function("match",regexp_match,"function(string:int)",0);
  add_function("split",regexp_split,"function(string:string*)",0);

  set_init_callback(init_regexp_glue);
  set_exit_callback(exit_regexp_glue);

  end_c_program("/precompiled/regexp");
}
