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
#include "object.h"
#include "operators.h"
#include "module.h"

#include "pdf_machine.h"


#define sp Pike_sp
#define fp Pike_fp

#define PDF_INITER

/* #define DEBUG */

#define PDF_CLASS(name,init,exit,prog) \
    void init(void); void exit(void); struct program *prog;
#define PDF_SUBMODULE(name,init,exit)  \
    void init(void); void exit(void); 
#define PDF_SUBMODMAG(name,init,exit) \
    void init(void); void exit(void);
#define PDF_FUNCTION(name,func,def0,def1) \
    void func(INT32 args);
#include "initstuff.h"

static struct program *pdf_sentinel = NULL;

static struct 
{
   char *name;
   void (*init)(void);
   void (*exit)(void);
   struct program **dest;
} initclass[]=
{
#undef PDF_CLASS
#undef PDF_SUBMODULE
#undef PDF_FUNCTION
#undef PDF_SUBMODMAG
#define PDF_SUBMODMAG(a,b,c) 
#define PDF_FUNCTION(a,b,c,d)
#define PDF_CLASS(name,init,exit,prog) { name,init,exit,&prog },
#define PDF_SUBMODULE(a,b,c)
#include "initstuff.h"
  PDF_CLASS(NULL,NULL,NULL,pdf_sentinel)
};

static struct 
{
   char *name;
   void (*init)(void);
   void (*exit)(void);
} initsubmodule[]=
{
#undef PDF_CLASS
#undef PDF_SUBMODULE
#define PDF_CLASS(name,init,exit,prog) 
#define PDF_SUBMODULE(name,init,exit) { name,init,exit },
#include "initstuff.h"
  PDF_SUBMODULE(NULL,NULL,NULL)
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
#undef PDF_SUBMODULE
#undef PDF_SUBMODMAG
#define PDF_SUBMODULE(a,b,c)
#define PDF_SUBMODMAG(name,init,exit) { name,init,exit,NULL,NULL },
#include "initstuff.h"
  PDF_SUBMODMAG(NULL,NULL,NULL)
};

/* Avoid loss of precision warnings. */
#ifdef __ECL
static inline long TO_LONG(ptrdiff_t x)
{
  return DO_NOT_WARN((long)x);
}
#else /* !__ECL */
#define TO_LONG(x)	((long)(x))
#endif /* __ECL */

#ifdef PIKE_DEBUG
#define PDF_CHECK_STACK(X)	do { \
    if (save_sp != sp) { \
      Pike_fatal("%s:%d: %ld droppings on stack! previous init: %s\n", \
            __FILE__, __LINE__, TO_LONG(sp - save_sp), X); \
    } \
  } while(0)
#else
#define PDF_CHECK_STACK(X)
#endif /* PIKE_DEBUG */


static void pdf_magic_index(INT32 args)
{
   int i;

   if (args!=1) 
      Pike_error("PDF.`[]: Too few or too many arguments\n");
   if (sp[-1].type!=T_STRING)
      Pike_error("PDF.`[]: Illegal type of argument\n");

   for (i=0; i<(int)NELEM(submagic); i++)
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
	    PDF_CHECK_STACK(submagic[i].name);
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
      push_text("_PDF_");
      stack_swap();
      f_add(2);
      SAFE_APPLY_MASTER("resolv",1);
   }
   if (sp[-1].type==T_INT)
   {
      pop_stack();
      stack_dup();
      push_text("_PDF");
      SAFE_APPLY_MASTER("resolv",1);
      stack_swap();
      f_index(2);
   }
   stack_swap();
   pop_stack();
}

PIKE_MODULE_INIT
{
   char type_of_index[]=
      tFunc(tStr,tOr3(tObj,tPrg(tObj),""))

#undef PDF_FUNCTION
#undef PDF_SUBMODMAG
#define PDF_SUBMODMAG(name,init,exit) 
#define PDF_FUNCTION(name,func,def0,def1) tOr(def0,"")
#include "initstuff.h"
      tFunc(tStr,tMixed); /* this */

#ifdef PIKE_DEBUG
   struct svalue *save_sp = sp;
#endif

   int i;

   for (i=0; i<(int)NELEM(initclass); i++)
   {
      if(!initclass[i].name) continue;
      start_new_program();

#ifdef DEBUG
      fprintf(stderr,"PDF: initiating class \"PDF.%s\"...\n",
	      initclass[i].name);
#endif

      (initclass[i].init)();
      PDF_CHECK_STACK(initclass[i].name);
      initclass[i].dest[0]=end_program();
      add_program_constant(initclass[i].name,initclass[i].dest[0],0);
   }

   for (i=0; i<(int)NELEM(initsubmodule); i++)
   {
      struct program *p;
      struct pike_string *s;

      if(!initsubmodule[i].name) continue;
#ifdef DEBUG
      fprintf(stderr,"PDF: initiating submodule \"PDF.%s\"...\n",
	      initsubmodule[i].name);
#endif
      
      start_new_program();
      (initsubmodule[i].init)();
      PDF_CHECK_STACK(initsubmodule[i].name);
      p=end_program();
      push_object(clone_object(p,0));
      s=make_shared_string(initsubmodule[i].name);
      add_constant(s,sp-1,0);
      free_string(s);
      free_program(p);
      pop_stack();
   }

   for (i=0; i<(int)NELEM(submagic); i++) 
   {
      if(!submagic[i].name) continue;
      submagic[i].ps=make_shared_string(submagic[i].name);
   }

#undef PDF_FUNCTION
#define PDF_FUNCTION(name,func,def0,def1) ADD_FUNCTION(name,func,def0,def1);
#include "initstuff.h"

   quick_add_function("`[]",3,pdf_magic_index,
		      type_of_index,CONSTANT_STRLEN(type_of_index),0,0);
}

PIKE_MODULE_EXIT
{
   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      if(!initclass[i].name) continue;
      (initclass[i].exit)();
      free_program(initclass[i].dest[0]);
   }
   for (i=0; i<(int)NELEM(initsubmodule); i++) {
      if(!initsubmodule[i].name) continue;
      (initsubmodule[i].exit)();
   }
   for (i=0; i<(int)NELEM(submagic); i++) {
      if(!submagic[i].name) continue;
      if (submagic[i].o)
      {
	 (submagic[i].exit)();
	 free_object(submagic[i].o);
      }
      if (submagic[i].ps)
      {
	 free_string(submagic[i].ps);
      }
   }
}
