/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"
#include "threads.h"
#include "module_support.h"

RCSID("$Id: glue.c,v 1.15 1999/11/30 23:04:41 hubbe Exp $");

#ifdef USE_SYSTEM_REGEXP
#include <regexp.h>

struct regexp_glue
{
  regex_t regexp;
  int num_paren;
};
#else
#include "pike_regexp.h"

struct regexp_glue
{
  struct regexp *regexp;
};

#endif /* USE_SYSTEM_REGEXP */


#ifdef THIS
#undef THIS
#endif

#define THIS ((struct regexp_glue *)(fp->current_storage))

static void do_free(void)
{
  if(THIS->regexp)
  {
#ifdef USE_SYSTEM_REGEXP
    regfree(&(THIS->regexp));
#else
    free((char *)THIS->regexp);
    THIS->regexp=0;
#endif /* USE_SYSTEM_REGEXP */
  }
}

static void regexp_create(INT32 args)
{
  const char *str;

  do_free();
  if(args)
  {
    get_all_args("Regexp.regexp->create", args, "%s", &str);

#ifdef USE_SYSTEM_REGEXP
    {
      int err = regcomp(&(THIS->regexp), str, 0);
      int i;
      char *paren_ptr;

      if (err) {
	char buf[1024];

	regerror(err, &(THIS->regexp), buf, 1024);
	error("Regexp.regexp->create(): Compilation failed:%s\n", buf);
      }
      for (i=0,paren_ptr=str; paren_ptr = strchr(paren_ptr, '('); i++) {
	paren_ptr++;
      }
      THIS->num_paren = i;
    }
#else
    THIS->regexp=pike_regcomp(sp[-args].u.string->str, 0);
#endif
  }
}

static void regexp_match(INT32 args)
{
  int i;
  char *str;
#ifdef USE_SYSTEM_REGEXP
  regex_t *regexp = &(THIS->regexp);
#else
  struct regexp *regexp = THIS->regexp;
#endif /* USE_SYSTEM_REGEXP */

  get_all_args("Regexp.regexp->match", args, "%s", &str);
  
#ifdef USE_SYSTEM_REGEXP
  ALLOW_THREADS();
  i = !regexec(regexp, str, 0, NULL, 0);
  DISALLOW_THREADS();
#else
  i=pike_regexec(regexp, str);
#endif /* USE_SYSTEM_REGEXP */
  pop_n_elems(args);
  push_int(i);
}

static void regexp_split(INT32 args)
{
  struct pike_string *s;
#ifdef USE_SYSTEM_REGEXP
  regex_t *r;
  regmatch_t *pmatch;
  size_t nmatch;
  int match;
#else  
  struct regexp *r;
#endif /* USE_SYSTEM_REGEXP */

  get_all_args("Regexp.regexp->split", args, "%S", &s);

#ifdef USE_SYSTEM_REGEXP
  r = &(THIS->regexp);
  nmatch = THIS->num_paren+1;
  pmatch = xalloc(sizeof(regmatch_t)*nmatch);

  THREADS_ALLOW();
  match = !regexec(r, s->str, nmatch, pmatch, 0);
  THREADS_DISALLOW();

  if (match) {
    int i,j;
    
    add_ref(s);
    pop_n_elems(args);

    for (i=1; i < nmatch; i++) {
      if (pmatch[i].rm_sp && pmatch[i].rm_so != -1) {
	push_string(make_shared_binary_string(pmatch[i].rm_sp,
					      pmatch[i].rm_eo-pmatch[i].rm_so));
      } else {
	push_int(0);
      }
    }
    f_aggregate_array(nmatch-1);
    free_string(s);
  } else {
    pop_n_elems(args);
    push_int(0);
  }
#else
  if(pike_regexec(r=THIS->regexp, s->str))
  {
    int i,j;
    add_ref(s);
    pop_n_elems(args);
    for(j=i=1;i<NSUBEXP;i++)
    {
      if(!r->startp[i] || !r->endp[i])
      {
	push_int(0);
      }else{
	push_string(make_shared_binary_string(r->startp[i],
					      r->endp[i]-r->startp[i]));
	j=i;
      }
    }
    if(j<i-1) pop_n_elems(i-j-1);
    push_array(aggregate_array(j));
    free_string(s);
  }else{
    pop_n_elems(args);
    push_int(0);
  }
#endif /* USE_SYSTEM_REGEXP */
}

static void init_regexp_glue(struct object *o)
{
#ifdef USE_SYSTEM_REGEXP
  MEMCLR(THIS, sizeof(struct regexp_glue));
#else
  THIS->regexp=0;
#endif /* USE_SYSTEM_REGEXP */
}

static void exit_regexp_glue(struct object *o)
{
  do_free();
}


void pike_module_exit(void) {}

void pike_module_init(void)
{
  ADD_STORAGE(struct regexp_glue);
  
  /* function(void|string:void) */
  ADD_FUNCTION("create",regexp_create,tFunc(tOr(tVoid,tStr),tVoid),0);
  /* function(string:int) */
  ADD_FUNCTION("match",regexp_match,tFunc(tStr,tInt),0);
  /* function(string:string*) */
  ADD_FUNCTION("split",regexp_split,tFunc(tStr,tArr(tStr)),0);

  set_init_callback(init_regexp_glue);
  set_exit_callback(exit_regexp_glue);
}
