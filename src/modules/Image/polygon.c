#include "global.h"
#include <config.h>

/* $Id: polygon.c,v 1.1 1998/07/14 21:34:28 hubbe Exp $ */

/*
**! module Image
**! note
**!	$Id: polygon.c,v 1.1 1998/07/14 21:34:28 hubbe Exp $
**! class polygon
**!
**!	This object keep polygon information,
**!	for quick polygon operations.
**!
**! see also: Image, Image.image, Image.image->polyfill
*/

#undef COLORTABLE_DEBUG
#undef COLORTABLE_REDUCE_DEBUG

RCSID("$Id: polygon.c,v 1.1 1998/07/14 21:34:28 hubbe Exp $");

#include <math.h> 

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
#include "polygon.h"
#include "dmalloc.h"

struct program *image_polygon_program;
extern struct program *image_program;


#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct polygon *)(fp->current_storage))
#define THISOBJ (fp->current_object)

/***************** init & exit *********************************/

static void free_polygon_struct(struct polygon *nct)
{
}

static void polygon_init_stuff(struct polygon *nct)
{
}

static void init_polygon_struct(struct object *obj)
{
   polygon_init_stuff(THIS);
}

static void exit_polygon_struct(struct object *obj)
{
   free_polygon_struct(THIS);
}

/***************** internal stuff ******************************/

/***************** called stuff ********************************/

static void image_polygon_create(INT32 args)
{
}

/***************** global init etc *****************************/

void init_polygon_programs(void)
{
   start_new_program();
   add_storage(sizeof(struct polygon));

   set_init_callback(init_polygon_struct);
   set_exit_callback(exit_polygon_struct);

   add_function("create",image_polygon_create,
		"function(object|array(int|float) ...:void)",0);

   image_polygon_program=end_program();
   add_program_constant("polygon",image_polygon_program, 0);
}

void exit_polygon(void) 
{
  if(image_polygon_program)
  {
    free_program(image_polygon_program);
    image_polygon_program=0;
  }
}

