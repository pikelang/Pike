#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: parser.c,v 1.3 1999/06/12 13:42:42 mirar Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "object.h"
#include "operators.h"

#include "parser.h"

#define PARSER_INITER

/*#define DEBUG*/

#define PARSER_CLASS(name,init,exit,prog) \
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
} initclass[]=
{
#undef PARSER_CLASS
#undef PARSER_SUBMODULE
#undef PARSER_FUNCTION
#undef PARSER_SUBMODMAG
#define PARSER_SUBMODMAG(a,b,c) 
#define PARSER_FUNCTION(a,b,c,d)
#define PARSER_CLASS(name,init,exit,prog) { name,init,exit,&prog },
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
#define PARSER_CLASS(name,init,exit,prog) 
#define PARSER_SUBMODULE(name,init,exit) { name,init,exit },
#include "initstuff.h"
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
};

#ifdef PIKE_DEBUG
#define PARSER_CHECK_STACK(X)	do { if (save_sp != sp) { fatal("%s:%d: %d droppings on stack! previous init: %s\n", __FILE__, __LINE__, sp - save_sp,X); } } while(0)
#else
#define PARSER_CHECK_STACK(X)
#endif /* PIKE_DEBUG */


static void parser_magic_index(INT32 args)
{
   struct svalue tmp;
   int i;

   if (args!=1) 
      error("Parser.`[]: Too few or too many arguments\n");
   if (sp[-1].type!=T_STRING)
      error("Parser.`[]: Illegal type of argument\n");

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
      push_text("_Parser_");
      stack_swap();
      f_add(2);
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
   }
   if (sp[-1].type==T_INT)
   {
      pop_stack();
      stack_dup();
      push_text("_Parser");
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
      stack_swap();
      f_index(2);
   }
   stack_swap();
   pop_stack();
}

void pike_module_init(void)
{
#ifdef PIKE_DEBUG
   struct svalue *save_sp = sp;
#endif

   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      start_new_program();

#ifdef DEBUG
      fprintf(stderr,"Parser: initiating class \"Parser.%s\"...\n",
	      initclass[i].name);
#endif

      (initclass[i].init)();
      PARSER_CHECK_STACK(initclass[i].name);
      initclass[i].dest[0]=end_program();
      add_program_constant(initclass[i].name,initclass[i].dest[0],0);
   }

   for (i=0; i<(int)NELEM(initsubmodule); i++)
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

   for (i=0; i<(int)NELEM(submagic); i++)
      submagic[i].ps=make_shared_string(submagic[i].name);

#undef PARSER_FUNCTION
#define PARSER_FUNCTION(name,func,def0,def1) ADD_FUNCTION(name,func,def0,def1);
#include "initstuff.h"

   ADD_FUNCTION("`[]",parser_magic_index,
		tFunc(tString,tMixed),0);
}

void pike_module_exit(void) 
{
   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      (initclass[i].exit)();
      free_program(initclass[i].dest[0]);
   }
   for (i=0; i<(int)NELEM(initsubmodule); i++)
      (initsubmodule[i].exit)();
   for (i=0; i<(int)NELEM(submagic); i++)
      if (submagic[i].o)
      {
	 (submagic[i].exit)();
	 free_object(submagic[i].o);
      }
}
