/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "readline_machine.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"
#include "threads.h"

#include <errno.h>

#ifndef HAVE_LIBTERMCAP
#undef HAVE_LIBREADLINE
#endif

#if !defined(HAVE_READLINE_H) && !defined(HAVE_READLINE_READLINE_H)
#undef HAVE_LIBREADLINE
#endif

#if !defined(HAVE_HISTORY_H) && !defined(HAVE_READLINE_HISTORY_H) && !defined(HAVE_HISTORY_HISTORY_H)
#undef HAVE_LIBREADLINE
#endif

#ifdef HAVE_LIBREADLINE

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#else
#ifdef HAVE_READLINE_H
#include <readline.h>
#endif
#endif

#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#else
#ifdef HAVE_HISTORY_HISTORY_H
#include <history/history.h>
#else
#ifdef HAVE_HISTORY_H
#include <history.h>
#endif
#endif
#endif

static void f_readline(INT32 args)
{
  char *r;
  struct pike_string *str;
  if(args < 1)
    error("Too few arguments to readline().\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to readline()\n");

  str=sp[-args].u.string;
retry:
  THREADS_ALLOW();
  r=readline(str->str);
  THREADS_DISALLOW();

#ifdef _REENTRANT
  /* Kluge! /Hubbe */
  if(!r && errno==EINTR)
  {
    check_threads_etc();
    goto retry;
  }
#endif

  pop_n_elems(args);
  if(r)
  {
    if(*r) add_history(r);
    push_string(make_shared_string(r));
    free(r);
  } else {
    push_int(0);
  }
}

void pike_module_init(void)
{
  rl_bind_key('\t', rl_insert);
  add_function_constant("_module_value",f_readline,"function(string:string)",OPT_SIDE_EFFECT);
}

#else

#include <stdio.h>

#define BLOCK 16384

static void f_readline(INT32 args)
{
  char *prompt;
  int plen;
  char line[BLOCK];
  char *r;
  int tmp;

  if(args < 1)
    error("Too few arguments to readline().\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to readline()\n");

  prompt = sp[-args].u.string->str;
  plen = sp[-args].u.string->len;

  THREADS_ALLOW();

  write(1, prompt, plen);
  r=fgets(line,BLOCK-1,stdin);	/* Should probably get rid of this one */
  line[BLOCK-1] = '\0';		/* Always NUL-terminated */

  THREADS_DISALLOW();

  pop_n_elems(args);

  if (r)
  {
    INT32 len;
    if ((len=strlen(line)))
    {
      if (line[len-1]=='\n')
      {
	push_string(make_shared_binary_string(line,len-1));
	return;
      }
    }
  }
  push_int(0);
}

void pike_module_init(void)
{
  add_function_constant("_module_value",f_readline,"function(string:string)",OPT_SIDE_EFFECT);
}

#endif

void pike_module_exit(void) {}


