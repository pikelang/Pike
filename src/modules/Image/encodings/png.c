/* $Id: png.c,v 1.1 1997/11/12 03:37:52 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: png.c,v 1.1 1997/11/12 03:37:52 mirar Exp $
**! submodule PNG
**!
**!	This submodule keep the PNG encode/decode capabilities
**!	of the <ref>Image</ref> module.
**!
**!	PNG is a rather new lossless image storage format, 
**!	usable for most images.
**!
**!	Simple encoding:
**!	<ref>encode</ref>
**!
**! see also: Image, Image.image, Image.colortable
*/

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
RCSID("$Id: png.c,v 1.1 1997/11/12 03:37:52 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "threads.h"

void image_png__module_value(INT32 args)
{
   pop_n_elems(args);
   push_text("_Image_dot_PNG");
   SAFE_APPLY_MASTER("resolv",1);
   if (sp[-1].type==T_INT)
      error("Image.PNG: Can't load submodule\n");
}

void init_image_png(void)
{
   start_new_program();
   
   add_function("_module_value",image_png__module_value,
		"function(:object)",0);

   push_object(clone_object(end_program(),0));
   add_constant(make_shared_string("PNG"),sp-1,0);
   pop_stack();
}

void exit_image_png(void)
{
}
