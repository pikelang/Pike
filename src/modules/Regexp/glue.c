/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/

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
#include "builtin_functions.h"

RCSID("$Id: glue.c,v 1.22 2001/04/19 14:33:39 grubba Exp $");

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

/* must be included last */
#include "module_magic.h"


#ifdef THIS
#undef THIS
#endif

#define THIS ((struct regexp_glue *)(Pike_fp->current_storage))

/*! @modeule Regexp
 */

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

/*! @decl void create(string re)
 */
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
	Pike_error("Regexp.regexp->create(): Compilation failed:%s\n", buf);
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

#ifdef USE_SYSTEM_REGEXP
int regexp_match_low(regex_t *regexp, char *str)
{
  int i;
  THREADS_ALLOW();
  i = !regexec(regexp, str, 0, NULL, 0);
  THREADS_DISALLOW();
  return i;
}
#else
#define regexp_match_low pike_regexec
#endif /* USE_SYSTEM_REGEXP */

/*! @decl int match(string str)
 *!
 *! Returns 1 if @[str] matches the regexp bound to the regexp object.
 *! Zero otherwise.
 *!
 *! @decl array(string) match(array(string) strs)
 *!
 *! Returns an array containing strings in @[strs] that matches the
 *! regexp bound to the regexp object.
 *!
*/
static void regexp_match(INT32 args)
{
  int i;
#ifdef USE_SYSTEM_REGEXP
  regex_t *regexp = &(THIS->regexp);
#else
  struct regexp *regexp = THIS->regexp;
#endif /* USE_SYSTEM_REGEXP */

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Regexp.regexp->match", 1);
  
  if(Pike_sp[-args].type == T_STRING)
  {
    if(Pike_sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("Regexp.regexp->match", 1, "Expected string (8bit)");
    
    i = regexp_match_low(regexp, STR0(Pike_sp[-args].u.string));
    pop_n_elems(args);
    push_int(i);
    return;
  }
  else if(Pike_sp[-args].type == T_ARRAY)
  {
    struct array *arr;
    int i, n;

    arr = Pike_sp[-args].u.array;

    for(i = n = 0; i < arr->size; i++)
    {
      struct svalue *sv = ITEM(arr) + i;
      
      if(sv->type != T_STRING || sv->u.string->size_shift)
	SIMPLE_BAD_ARG_ERROR("Regexp.regexp->match",1,"Expected string (8bit)");

      if(regexp_match_low(regexp, STR0(sv->u.string)))
      {
	ref_push_string(sv->u.string);
	n++;
      }
    }
    
    f_aggregate(n);
    stack_pop_n_elems_keep_top(args);
    return;
  }
  else
    SIMPLE_BAD_ARG_ERROR("Regexp.regexp->match", 1, "string|array(string)");
}

/*! @decl array(string) split(string s)
 */
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

/*! @endmodule
 */

void pike_module_exit(void) {}

void pike_module_init(void)
{
  ADD_STORAGE(struct regexp_glue);
  
  /* function(void|string:void) */
  ADD_FUNCTION("create",regexp_create,tFunc(tOr(tVoid,tStr),tVoid),0);
  /* function(string:int) */
  ADD_FUNCTION("match",regexp_match,
	       tOr(tFunc(tStr, tInt),
		   tFunc(tArr(tStr), tArr(tStr))), 0);
  /* function(string:string*) */
  ADD_FUNCTION("split",regexp_split,tFunc(tStr,tArr(tStr)),0);

  set_init_callback(init_regexp_glue);
  set_exit_callback(exit_regexp_glue);
}
