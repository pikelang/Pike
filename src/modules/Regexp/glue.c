/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: glue.c,v 1.37 2004/10/07 22:49:58 nilsson Exp $
*/

#include "global.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "program.h"
#include "pike_macros.h"
#include "threads.h"
#include "module_support.h"
#include "builtin_functions.h"

#include "pike_regexp.h"

struct regexp_glue
{
  struct regexp *regexp;
};

#ifdef THIS
#undef THIS
#endif

#define THIS ((struct regexp_glue *)(Pike_fp->current_storage))

/*! @module Regexp
 *!
 *! @class SimpleRegexp
 *!
 *! This class implements the interface to a simple regexp engine
 *! with the following capabilities:
 *!
 *! @xml{<matrix>
 *! <r><c>.</c>     <c>Matches any character.</c></r>
 *! <r><c>[abc]</c> <c>Matches a, b or c.</c></r>
 *! <r><c>[a-z]</c> <c>Matches any character a to z inclusive.</c></r>
 *! <r><c>[^ac]</c> <c>Matches any character except a and c.</c></r>
 *! <r><c>(x)</c>   <c>Matches x (x might be any regexp) If used with split, this
 *!                    also puts the string matching x into the result array.</c></r>
 *! <r><c>x*</c>    <c>Matches zero or more occurances of 'x'
 *!                    (x may be any regexp).</c></r>
 *! <r><c>x+</c>    <c>Matches one or more occurances of 'x'
 *!                    (x may be any regexp).</c></r>
 *! <r><c>x|y</c>   <c>Matches x or y. (x or y may be any regexp).</c></r>
 *! <r><c>xy</c>    <c>Matches xy (x and y may be any regexp).</c></r>
 *! <r><c>^</c>     <c>Matches beginning of string (but no characters).</c></r>
 *! <r><c>$</c>     <c>Matches end of string (but no characters).</c></r>
 *! <r><c>\&lt;</c> <c>Matches the beginning of a word (but no characters).</c></r>
 *! <r><c>\&gt;</c> <c>Matches the end of a word (but no characters).</c></r>
 *! </matrix>@}
 *!
 *! Note that \ can be used to quote these characters in which case
 *! they match themselves, nothing else. Also note that when quoting
 *! these something in Pike you need two \ because Pike also uses
 *! this character for quoting.
 */

static void do_free(void)
{
  if(THIS->regexp)
  {
    free((char *)THIS->regexp);
    THIS->regexp=0;
  }
}

/*! @decl void create(string re)
 *!
 *! When create is called, the current regexp bound to this object is
 *! cleared. If a string is sent to create(), this string will be compiled
 *! to an internal representation of the regexp and bound to this object
 *! for laters calls to e.g. @[match] or @[split]. Calling create() without
 *! an argument can be used to free up a little memory after the regexp has
 *! been used.
 */
static void regexp_create(INT32 args)
{
  const char *str;

  do_free();
  if(args)
  {
    get_all_args("Regexp.SimpleRegexp->create", args, "%s", &str);
    THIS->regexp=pike_regcomp(Pike_sp[-args].u.string->str, 0);
  }
}

/*! @decl int match(string str)
 *!
 *! Returns 1 if @[str] matches the regexp bound to the regexp object.
 *! Zero otherwise.
 *!
 *! @decl array(string) match(array(string) strs)
 *!
 *! Returns an array containing strings in @[strs] that match the
 *! regexp bound to the regexp object.
 *!
 *! @bugs
 *!   The current implementation doesn't support searching
 *!   in strings containing the NUL character or any
 *!   wide character.
 *!
 *! @seealso
 *!   @[split]
 */
static void regexp_match(INT32 args)
{
  int i;
  struct regexp *regexp = THIS->regexp;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Regexp.SimpleRegexp->match", 1);
  
  if(Pike_sp[-args].type == T_STRING)
  {
    if(Pike_sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("Regexp.SimpleRegexp->match", 1,
			   "Expected string (8bit)");
    
    i = pike_regexec(regexp, (char *)STR0(Pike_sp[-args].u.string));
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
	SIMPLE_BAD_ARG_ERROR("Regexp.SimpleRegexp->match", 1,
			     "Expected string (8bit)");

      if(pike_regexec(regexp, (char *)STR0(sv->u.string)))
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
    SIMPLE_BAD_ARG_ERROR("Regexp.SimpleRegexp->match", 1,
			 "string|array(string)");
}

/*! @decl array(string) split(string s)
 *! Works as @[match], but returns an array of the strings that
 *! matched the subregexps. Subregexps are those contained in "( )" in
 *! the regexp. Subregexps that were not matched will contain zero.
 *! If the total regexp didn't match, zero is returned.
 *!
 *! @bugs
 *!   You can currently only have 39 subregexps.
 *!
 *! @bugs
 *!   The current implementation doesn't support searching
 *!   in strings containing the NUL character or any
 *!   wide character.
 *!
 *! @seealso
 *!   @[match]
 */
static void regexp_split(INT32 args)
{
  struct pike_string *s;
  struct regexp *r;

  get_all_args("Regexp.SimpleRegexp->split", args, "%S", &s);

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
}

static void init_regexp_glue(struct object *o)
{
  THIS->regexp=0;
}

static void exit_regexp_glue(struct object *o)
{
  do_free();
}

/*! @endclass
 */

/*! @endmodule
 */

PIKE_MODULE_EXIT {}

PIKE_MODULE_INIT
{
  start_new_program();
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
  end_class("_SimpleRegexp", 0);
}
