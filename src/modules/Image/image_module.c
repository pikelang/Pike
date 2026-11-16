/* -*- mode: C; c-basic-offset: 3; -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "module.h"
#include "pike_macros.h"
#include "interpret.h"
#include "pike_compiler.h"
#include "program_id.h"
#include "operators.h"
#include "module_support.h"
#include "pike_threadlib.h"
#include "callback.h"

#include "image.h"
#include "assembly.h"
#include "image_machine.h"

#include "encodings/encodings.h"

#define sp Pike_sp
#define fp Pike_fp

#define IMAGE_INITER

/* #define DEBUG */

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
#define IMAGE_CHECK_STACK(X)	do { \
    if (save_sp != sp) { \
      Pike_fatal("%s:%d: %ld droppings on stack! previous init: %s\n", \
                 __FILE__, __LINE__, (long)(sp - save_sp), X);         \
    } \
  } while(0)
#else
#define IMAGE_CHECK_STACK(X)
#endif /* PIKE_DEBUG */


static void image_magic_index(INT32 args)
{
   int i;

   if (args!=1)
      Pike_error("Image.`[]: Too few or too many arguments\n");
   if (TYPEOF(sp[-1]) != T_STRING)
      Pike_error("Image.`[]: Illegal type of argument\n");

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
	    enter_compiler(submagic[i].ps, 0);
	    start_new_program();
	    (submagic[i].init)();
	    IMAGE_CHECK_STACK(submagic[i].name);
	    p=end_program();
	    p->id = PROG_IMAGE_SUBMAGIC_START+i;
	    exit_compiler();
	    submagic[i].o=clone_object(p,0);
	    free_program(p);
	 }

	 ref_push_object(submagic[i].o);
	 return;
      }

   if (!Pike_fp->current_object) {
     Pike_fatal("Image: Lost track of module object!\n");
   }

   stack_dup();
   ref_push_object(fp->current_object);
   stack_swap();
   f_arrow(2);

   if (TYPEOF(sp[-1]) == T_INT)
   {
      pop_stack();
      stack_dup();
      push_constant_text("_Image_");
      stack_swap();
      f_add(2);
      SAFE_APPLY_MASTER("resolv",1);
   }
   if (TYPEOF(sp[-1]) == T_INT)
   {
      pop_stack();
      stack_dup();
      push_constant_text("_Image");
      SAFE_APPLY_MASTER("resolv",1);
      stack_swap();
      f_index(2);
   }
   stack_swap();
   pop_stack();
}

int image_cpuid;

/* See IMAGE_THREADS_ALLOW() in image.h. */
int image_threads_unlocked, image_replace_waiting;

/* Waits, with the interpreter lock released, until no drawing loop
 * runs without it; those that start meanwhile keep it, so that the
 * wait ends. */
void image_wait_for_unlocked(void)
{
  if (!image_threads_unlocked) return;
  image_replace_waiting++;
  while (image_threads_unlocked) {
    THREADS_ALLOW();
    sysleep(0.0001);
    THREADS_DISALLOW();
  }
  image_replace_waiting--;
}

/* The memory of images destructed while drawing loops ran without the
 * interpreter lock, and how many bytes of each to clear. */
static struct image_free { void *img; size_t clear; } *image_frees;
static int image_frees_waiting, image_frees_size;
static struct callback *image_frees_callback;

static void image_free_waiting(struct callback *UNUSED(cb),
                               void *UNUSED(a), void *UNUSED(b))
{
  int i;
  if (image_threads_unlocked) return;
  for (i = 0; i < image_frees_waiting; i++) {
    if (image_frees[i].clear)
      memset(image_frees[i].img, 0, image_frees[i].clear);
    free(image_frees[i].img);
  }
  image_frees_waiting = 0;
  image_replace_waiting--;
  remove_callback(image_frees_callback);
  image_frees_callback = NULL;
}

/* Frees an image's memory, after clearing its first CLEAR bytes. An
 * explicit destruct() may come while another thread draws on the image
 * without the interpreter lock, so while any drawing loop runs without
 * it, the memory is freed later, between two steps of the evaluator
 * once none does, when no thread can be in a drawing function. Loops
 * that start meanwhile keep the lock, so that it happens. */
void image_free_later(void *img, size_t clear)
{
  if (image_threads_unlocked) {
    if (image_frees_waiting == image_frees_size) {
      int n = image_frees_size ? image_frees_size * 2 : 8;
      struct image_free *f = realloc(image_frees, n * sizeof(*f));
      if (!f) return;	/* Rather leaked than freed in use. */
      image_frees = f;
      image_frees_size = n;
    }
    if (!image_frees_waiting) {
      image_replace_waiting++;
      image_frees_callback =
        add_to_callback(&evaluator_callbacks, image_free_waiting, NULL, NULL);
    }
    image_frees[image_frees_waiting].img = img;
    image_frees[image_frees_waiting].clear = clear;
    image_frees_waiting++;
    return;
  }
  if (clear) memset(img, 0, clear);
  free(img);
}
#ifdef ASSEMBLY_OK
static void init_cpuidflags( )
{
  unsigned int a, b, c, d;
  char *data = alloca(20);
  memset( data, 0, 20 );

  image_get_cpuid( 0, &a, &b, &c, &d );

  ((int *)data)[0] = a;
  ((int *)data)[1] = b;
  ((int *)data)[2] = c;

  if( strncmp( data, "GenuineIntel", 12 ) )
  {
    if( strncmp( data, "AuthenticAMD", 12 ) )
    {
      if( !strncmp( data, "CyrixInstead", 12 ) )
      {
        if( d != 2 )
          goto normal_test;
        image_get_cpuid( 0x80000000, &a, &b, &c, &d );
        if( d < 0x80000000 )
          goto normal_test;
        image_get_cpuid( 0x80000001, &a, &b, &c, &d );

        if( b & 0x00800000 ) image_cpuid |= IMAGE_MMX;
        if( b & 0x02000000 ) image_cpuid |= IMAGE_SSE;
        if( b & 0x01000000 ) image_cpuid |= IMAGE_EMMX;
        if( b & 0x80000000 ) image_cpuid |= (IMAGE_3DNOW | IMAGE_MMX);
      }
    } else {
      /* It's an AMD cpu. */
      image_get_cpuid( 0x80000000, &a, &b, &c, &d );
      if( d < 0x80000000 )
        goto normal_test;
      image_get_cpuid( 0x80000001, &a, &b, &c, &d );

      if( b & 0x00800000 ) image_cpuid |= IMAGE_MMX;
      if( b & 0x02000000 ) image_cpuid |= IMAGE_SSE;
      if( b & 0x80000000 ) image_cpuid |= (IMAGE_3DNOW | IMAGE_MMX);
    }
  } else {
  normal_test:
    /* It's an intel CPU. */
    image_get_cpuid( 1, &a, &b, &c, &d );
    if( b & 0x00800000 )
      image_cpuid |= IMAGE_MMX;
    if( b & 0x02000000 )
      image_cpuid |= IMAGE_SSE;
  }
#if 0
  fprintf(stderr, "Image CPUID == %d\n", image_cpuid );
#endif
}
#endif

PIKE_MODULE_INIT
{
   char type_of_index[]=
      tFunc(tStr,tOr3(tObj,tPrg(tObj),""))

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


#ifdef ASSEMBLY_OK
     init_cpuidflags( );
#endif

   for (i=0; i<(int)NELEM(initclass); i++)
   {
      start_new_program();

#ifdef DEBUG
      fprintf(stderr,"Image: initiating class \"Image.%s\"...\n",
	      initclass[i].name);
#endif

      (initclass[i].init)();
      IMAGE_CHECK_STACK(initclass[i].name);
      initclass[i].dest[0]=end_program();
      initclass[i].dest[0]->id = i+PROG_IMAGE_CLASS_START;
      add_program_constant(initclass[i].name,initclass[i].dest[0],0);
   }

   for (i=0; i<(int)NELEM(initsubmodule); i++)
   {
      struct program *p;
      struct pike_string *s;

#ifdef DEBUG
      fprintf(stderr,"Image: initiating submodule \"Image.%s\"...\n",
	      initsubmodule[i].name);
#endif

      start_new_program();
      (initsubmodule[i].init)();
      IMAGE_CHECK_STACK(initsubmodule[i].name);
      p=end_program();
      p->id = i+PROG_IMAGE_SUBMODULE_START;
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

#ifndef FAKE_DYNAMIC_LOAD
   PIKE_MODULE_EXPORT(Image, image_program );
   PIKE_MODULE_EXPORT(Image, image_colortable_program);
   PIKE_MODULE_EXPORT(Image, image_layer_program );
#endif
}

PIKE_MODULE_EXIT
{
   int i;
   for (i=0; i<(int)NELEM(initclass); i++)
   {
      (initclass[i].exit)();
      free_program(initclass[i].dest[0]);
   }
   for (i=0; i<(int)NELEM(initsubmodule); i++)
      (initsubmodule[i].exit)();
   for (i=0; i<(int)NELEM(submagic); i++) {
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
   if (image_frees_callback)
      remove_callback(image_frees_callback);
   free(image_frees);
}
