#include <config.h>

/* $Id: colortable.c,v 1.3 1997/05/29 19:37:21 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: colortable.c,v 1.3 1997/05/29 19:37:21 mirar Exp $<br>
**! class colortable
**!
**!	This object keeps colortable information,
**!	mostly for image re-coloring (quantization).
**!
**! see also: Image, Image.image, Image.font
*/


#include "global.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <errno.h>

#include "config.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "threads.h"
#include "builtin_functions.h"

#include "image.h"


struct program *colortable_program;

#define THIS (*(struct colortable **)(fp->current_storage))
#define THISOBJ (fp->current_object)

#if 0

/***************** init & exit *********************************/

static inline void free_colortable_struct(struct colortable *colortable)
{
   if (colortable)
   {
      if (colortable->mem)
      {
#ifdef HAVE_MMAP
	 munmap(colortable->mem,colortable->mmaped_size);
#else
	 free(colortable->mem);
#endif
      }
      free(colortable);
   }
}

static void init_colortable_struct(struct object *o)
{
  THIS=NULL;
}

static void exit_colortable_struct(struct object *obj)
{
   free_colortable_struct(THIS);
   THIS=NULL;
}

/***************** global init etc *****************************/

/*

*/

void init_colortable_programs(void)
{
   start_new_program();
   add_storage(sizeof(struct colortable*));

   set_init_callback(init_colortable_struct);
   set_exit_callback(exit_colortable_struct);
  
   colortable_program=end_program();
   add_program_constant("colortable",colortable_program, 0);
}

void exit_colortable(void) 
{
  if(colortable_program)
  {
    free_program(colortable_program);
    colortable_program=0;
  }
}


#endif
