#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: image_module.c,v 1.1 1999/05/23 17:46:44 mirar Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "object.h"
#include "operators.h"

#include "image.h"

#define IMAGE_INITER


#define IMAGE_CLASS(name,init,exit,prog) \
    void init(void); void exit(void); struct program *prog;
#define IMAGE_SUBMODULE(name,init,exit)  \
    void init(void); void exit(void); 
#define IMAGE_SUBMODMAG(name,init,exit) \
    void init(void); void exit(void);
#define IMAGE_FUNCTION(name,func,def0,def1) \
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
#undef IMAGE_CLASS
#undef IMAGE_SUBMODULE
#undef IMAGE_FUNCTION
#undef IMAGE_SUBMODMAG
#define IMAGE_SUBMODMAG(a,b,c) 
#define IMAGE_FUNCTION(a,b,c,d)
#define IMAGE_CLASS(name,init,exit,prog) { name,init,exit,&prog },
#define IMAGE_SUBMODULE(a,b,c)
#include "initstuff.h"
};

static struct 
{
   char *name;
   void (*init)(void);
   void (*exit)(void);
} initsubmodule[]=
{
#undef IMAGE_CLASS
#undef IMAGE_SUBMODULE
#define IMAGE_CLASS(name,init,exit,prog) 
#define IMAGE_SUBMODULE(name,init,exit) { name,init,exit },
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
#undef IMAGE_SUBMODULE
#undef IMAGE_SUBMODMAG
#define IMAGE_SUBMODULE(a,b,c)
#define IMAGE_SUBMODMAG(name,init,exit) { name,init,exit,NULL,NULL },
#include "initstuff.h"
};

#ifdef PIKE_DEBUG
#define IMAGE_CHECK_STACK(X)	do { if (save_sp != sp) { fatal("%s:%d: %d droppings on stack! previous init: %s\n", __FILE__, __LINE__, sp - save_sp,X); } } while(0)
#else
#define IMAGE_CHECK_STACK(X)
#endif /* PIKE_DEBUG */


static void image_magic_index(INT32 args)
{
   struct svalue tmp;
   int i;

   if (args!=1) 
      error("Image.`[]: Too few or too many arguments\n");
   if (sp[-1].type!=T_STRING)
      error("Image.`[]: Illegal type of argument\n");

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
	    IMAGE_CHECK_STACK(submagic[i].name);
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
      push_text("_Image_");
      stack_swap();
      f_add(2);
      push_int(0);
      SAFE_APPLY_MASTER("resolv",2);
   }
   else
   {
      stack_swap();
      pop_stack();
   }
}

void pike_module_init(void)
{
   char type_of_index[]=
      tFunc(tStr,tOr3(tObj,tPrg,""))
/*       "\004" tStr "\021" tVoid  */
/*       "\373" tObj "\373" tPrg */

#undef IMAGE_FUNCTION
#undef IMAGE_SUBMODMAG
#define IMAGE_SUBMODMAG(name,init,exit) 
#define IMAGE_FUNCTION(name,func,def0,def1) tOr(def0,"")
#include "initstuff.h"
      tFunc(tStr,tMixed); /* this */

#ifdef PIKE_DEBUG
   struct svalue *save_sp = sp;
#endif

   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      start_new_program();
      (initclass[i].init)();
      IMAGE_CHECK_STACK(initclass[i].name);
      initclass[i].dest[0]=end_program();
      add_program_constant(initclass[i].name,initclass[i].dest[0],0);
   }

   for (i=0; i<(int)NELEM(initsubmodule); i++)
   {
      struct program *p;
      struct pike_string *s;
      
      start_new_program();
      (initsubmodule[i].init)();
      IMAGE_CHECK_STACK(initsubmodule[i].name);
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

#undef IMAGE_FUNCTION
#define IMAGE_FUNCTION(name,func,def0,def1) ADD_FUNCTION(name,func,def0,def1);
#include "initstuff.h"

   quick_add_function("`[]",3,image_magic_index,
		      type_of_index,CONSTANT_STRLEN(type_of_index),0,0);

   /* compat stuff */
   add_program_constant("image",image_program,0); 
   add_program_constant("colortable",image_program,0);
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
