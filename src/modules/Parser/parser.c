/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id$");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"
#include "module.h"

#include "parser.h"


#define sp Pike_sp
#define fp Pike_fp

#define PARSER_INITER

/*#define DEBUG*/

#define PARSER_CLASS(name,init,exit,prog,id) \
    void init(void); void exit(void); struct program *prog;
#define PARSER_SUBMODULE(name,init,exit)  \
    void init(void); void exit(void); 
#define PARSER_SUBMODMAG(name,init,exit) \
    void init(void); void exit(void);
#define PARSER_FUNCTION(name,func,def0,def1) \
    void func(INT32 args);
#include "initstuff.h"

static struct 
{
   char *name;
   void (*init)(void);
   void (*exit)(void);
   struct program **dest;
   int id;
} initclass[]=
{
#undef PARSER_CLASS
#undef PARSER_SUBMODULE
#undef PARSER_FUNCTION
#undef PARSER_SUBMODMAG
#define PARSER_SUBMODMAG(a,b,c) 
#define PARSER_FUNCTION(a,b,c,d)
#define PARSER_CLASS(name,init,exit,prog,id) { name,init,exit,&prog,id },
#define PARSER_SUBMODULE(a,b,c)
#include "initstuff.h"
};

static struct 
{
   char *name;
   void (*init)(void);
   void (*exit)(void);
} initsubmodule[]=
{
#undef PARSER_CLASS
#undef PARSER_SUBMODULE
#define PARSER_CLASS(name,init,exit,prog,id)
#define PARSER_SUBMODULE(name,init,exit) { name,init,exit },
#include "initstuff.h"
  {0,0,0 }
};

static struct 
{
   char *name;
   void (*init)(void);
   void (*exit)(void);
   struct pike_string *ps;
   struct object *o;
} submagic[]=
{
#undef PARSER_SUBMODULE
#undef PARSER_SUBMODMAG
#define PARSER_SUBMODULE(a,b,c)
#define PARSER_SUBMODMAG(name,init,exit) { name,init,exit,NULL,NULL },
#include "initstuff.h"
  { 0,0,0,0,0 }
};

#ifdef PIKE_DEBUG
#define PARSER_CHECK_STACK(X)	do { \
    if (save_sp != sp) { \
      Pike_fatal("%s:%d: %ld droppings on stack! previous init: %s\n", \
            __FILE__, __LINE__, \
            PTRDIFF_T_TO_LONG(sp - save_sp), X); \
    } \
  } while(0)
#else
#define PARSER_CHECK_STACK(X)
#endif /* PIKE_DEBUG */


static void parser_magic_index(INT32 args)
{
   int i;

   if (args!=1) 
      Pike_error("Parser.`[]: Too few or too many arguments\n");
   if (sp[-1].type!=T_STRING)
      Pike_error("Parser.`[]: Illegal type of argument\n");

   for (i=0; i<(int)NELEM(submagic)-1; i++)
      if (sp[-1].u.string==submagic[i].ps)
      {
#ifdef PIKE_DEBUG
   struct svalue *save_sp;
#endif
	 pop_stack();

#ifdef PIKE_DEBUG
	 save_sp = sp;
#endif

	 if (!submagic[i].o)
	 {
	    struct program *p;
	    start_new_program();
	    (submagic[i].init)();
	    PARSER_CHECK_STACK(submagic[i].name);
	    p=end_program();
	    submagic[i].o=clone_object(p,0);
	    free_program(p);
	 }
	 
	 ref_push_object(submagic[i].o);
	 return;
      }

   stack_dup();
   ref_push_object(fp->current_object);
   stack_swap();
   f_arrow(2);

   if (sp[-1].type==T_INT)
   {
      pop_stack();
      stack_dup();
      push_constant_text("_Parser_");
      stack_swap();
      f_add(2);
      SAFE_APPLY_MASTER("resolv",1);
   }
   if (sp[-1].type==T_INT)
   {
      pop_stack();
      stack_dup();
      push_constant_text("_Parser");
      SAFE_APPLY_MASTER("resolv",1);
      stack_swap();
      if(sp[-2].type == T_INT)
      {
	pop_stack();
      }else{
	f_index(2);
      }
   }
   stack_swap();
   pop_stack();
}

PIKE_MODULE_INIT
{
#ifdef PIKE_DEBUG
   struct svalue *save_sp = sp;
#endif

   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      start_new_program();
      if (initclass[i].id) Pike_compiler->new_program->id = initclass[i].id;

#ifdef DEBUG
      fprintf(stderr,"Parser: initiating class \"Parser.%s\"...\n",
	      initclass[i].name);
#endif

      (initclass[i].init)();
      PARSER_CHECK_STACK(initclass[i].name);
      initclass[i].dest[0]=end_program();
      add_program_constant(initclass[i].name,initclass[i].dest[0],0);
   }

   for (i=0; i<(int)NELEM(initsubmodule)-1; i++)
   {
      struct program *p;
      struct pike_string *s;

#ifdef DEBUG
      fprintf(stderr,"Parser: initiating submodule \"Parser.%s\"...\n",
	      initsubmodule[i].name);
#endif
      
      start_new_program();
      (initsubmodule[i].init)();
      PARSER_CHECK_STACK(initsubmodule[i].name);
      p=end_program();
      push_object(clone_object(p,0));
      s=make_shared_string(initsubmodule[i].name);
      add_constant(s,sp-1,0);
      free_string(s);
      free_program(p);
      pop_stack();
   }

   for (i=0; i<(int)NELEM(submagic)-1; i++)
      submagic[i].ps=make_shared_string(submagic[i].name);

#undef PARSER_FUNCTION
#define PARSER_FUNCTION(name,func,def0,def1) ADD_FUNCTION(name,func,def0,def1);
#include "initstuff.h"

   ADD_FUNCTION("`[]",parser_magic_index,
		tFunc(tString,tMixed),0);
}

PIKE_MODULE_EXIT
{
   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      (initclass[i].exit)();
      free_program(initclass[i].dest[0]);
   }
   for (i=0; i<(int)NELEM(initsubmodule)-1; i++)
      (initsubmodule[i].exit)();
   for (i=0; i<(int)NELEM(submagic)-1; i++)
      if (submagic[i].o)
      {
	 (submagic[i].exit)();
	 free_object(submagic[i].o);
      }
}
