/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
 */
#include "global.h"
#include "readline_machine.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"
#include "threads.h"

RCSID("$Id: readlinemod.c,v 1.8 1998/03/28 13:54:12 grubba Exp $");

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

struct svalue *complete_callback = NULL;

char *my_copy_string(struct pike_string *s)
{
  char *res = malloc(s->len+1);
  MEMCPY(res,s->str,s->len+1);
  return res;
}

char *low_do_rl_complete(char *string, int state)
{
  JMP_BUF tmp;
  rl_completer_quote_characters = "`'\"";
  if(complete_callback)
  {
    push_text(string);
    push_int(state);
    push_string(make_shared_binary_string(rl_line_buffer, rl_end));
    push_int(rl_point);
    if(SETJMP(tmp))
    {
      fprintf(stderr, "error in completion function");
      return 0;
    }
    else
      apply_svalue(complete_callback, 4);
     UNSETJMP(tmp);

    if(sp[-1].type == T_STRING)
      return my_copy_string(sp[-1].u.string);
    /* Note that we do _not_ pop the stack here...
     * All strings will be pop()ed when f_readline returns.
     * DO NOT USE // as comments!!! // Hubbe
     */
  }
  return 0;
}

char *my_rl_complete(char *text, int status)
{
#ifdef _REENTRANT
  struct thread_state *state;
  char *res;
  if((state = thread_state_for_id(th_self()))!=NULL)
  {
    /* This is a pike thread.  Do we have the interpreter lock? */
    if(!state->swapped) {
      /* Yes.  Go for it... */
      res=low_do_rl_complete(text,status);
    } else {
      /* Nope, let's get it... */
      mt_lock(&interpreter_lock);
      SWAP_IN_THREAD(state);
      res=low_do_rl_complete(text,status);
      /* Restore */
      SWAP_OUT_THREAD(state);
      mt_unlock(&interpreter_lock);
    }
  } else
    fatal("Bad idea!\n");
  return res;
#else
  return low_do_rl_comlete(text,status);
#endif
}

static void f_readline(INT32 args)
{
  char *r;
  struct pike_string *str;
  struct svalue *osp = sp;
  if(args < 1)
    error("Too few arguments to readline().\n");
  if(args > 1) 
    complete_callback = sp-args+1;
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
  complete_callback = 0;
  pop_n_elems(args+(sp-osp));
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
  rl_completion_entry_function = (void *)my_rl_complete;
  add_function_constant("_module_value",f_readline,
			"function(string,void|function:string)",
			OPT_SIDE_EFFECT);
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
  add_function_constant("_module_value",f_readline,"function(string,function|void:string)",OPT_SIDE_EFFECT);
}

#endif

void pike_module_exit(void) {}


