/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: module.c,v 1.22 2003/03/26 18:52:04 nilsson Exp $
*/

#include "global.h"
#include "module.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include "main.h"
#include "svalue.h"
#include "interpret.h"
#include "stralloc.h"
#include "object.h"
#include "mapping.h"
#include "program_id.h"
#include "language.h"
#include "lex.h"

#include "modules/modlist_headers.h"
#ifndef IN_TPIKE
#include "post_modules/modlist_headers.h"
#endif

RCSID("$Id: module.c,v 1.22 2003/03/26 18:52:04 nilsson Exp $");

/* Define this to trace the initialization and cleanup of static modules. */
/* #define TRACE_MODULE */

#if defined(TRACE_MAIN) || defined(TRACE_MODULE)
#define TRACE(X)	fprintf X
#else /* !TRACE_MAIN */
#define TRACE(X)
#endif /* TRACE_MAIN */

typedef void (*modfun)(void);

struct static_module
{
  char *name;
  modfun init;
  modfun exit;
};

static struct static_module module_list[] = {
  { "Builtin", low_init_main, low_exit_main }
#include "modules/modlist.h"
#ifndef IN_TPIKE
#include "post_modules/modlist.h"
#endif
};

void init_modules(void)
{
  struct program *p;
  unsigned int e;
  struct lex save_lex;

  save_lex = lex;
  lex.current_line=1;
  lex.current_file=make_shared_string("-");

  start_new_program();
  Pike_compiler->new_program->id=PROG___BUILTIN_ID;

  for(e=0;e<NELEM(module_list);e++)
  {
    JMP_BUF recovery;
    start_new_program();
    if(SETJMP(recovery)) {
      free_program(end_program());
      call_handle_error();
    } else {
      TRACE((stderr, "Initializing static module #%d: \"%s\"...\n",
	     e, module_list[e].name));
      module_list[e].init();
      debug_end_class(module_list[e].name,strlen(module_list[e].name),0);
    }
    UNSETJMP(recovery);
  }
  push_text("_static_modules");
  push_object(low_clone(p=end_program()));
  f_add_constant(2);
  free_program(p);
  free_string(lex.current_file);
  lex = save_lex;

  init_main();
}

void exit_modules(void)
{
  JMP_BUF recovery;
  int e;

  exit_main();

  for(e=NELEM(module_list)-1;e>=0;e--)
  {
    if(SETJMP(recovery))
      call_handle_error();
    else {
      TRACE((stderr, "Exiting static module #%d: \"%s\"...\n",
	     e, module_list[e].name));
      module_list[e].exit();
    }
    UNSETJMP(recovery);
  }
}
