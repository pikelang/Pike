/* $Id: pnm.c,v 1.8 1997/11/02 03:46:52 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: pnm.c,v 1.8 1997/11/02 03:46:52 mirar Exp $
**! class image
*/

#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
#include "global.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "error.h"

#include "image.h"

#define THIS ((struct image *)(fp->current_storage))
#define THISOBJ (fp->current_object)

extern struct program *image_program;

/*
**! method string toppm()
**!	<b>compability method</b> - do not use in new programs.
**!	
**!	See <ref>Image.PNM.encode</ref>().
**!
**! returns PPM data
**!
**! method object|string frompnm(string pnm)
**! method object|string fromppm(string pnm)
**!	<b>compability method</b> - do not use in new programs.
**!	
**!	See <ref>Image.PNM.decode</ref>().
**!
**! returns the called object or a hint of what wronged.
**! arg string pnm
**!	pnm data, as a string
*/

void img_pnm_encode_binary(INT32 args);
void img_pnm_decode(INT32 args);

void image_toppm(INT32 args)
{
   pop_n_elems(args);
   
   THISOBJ->refs++;
   push_object(THISOBJ);

   img_pnm_encode_binary(1);
}

void image_frompnm(INT32 args)
{
   struct image *img;
   img_pnm_decode(args);
   
   img=(struct image*)get_storage(sp[-1].u.object,image_program);
   if (THIS->img) free(THIS->img);
   *THIS=*img;
   THIS->img=malloc(img->xsize*img->ysize*sizeof(rgb_group)+1);
   if (!THIS->img) error("out of memory\n");
   MEMCPY(THIS->img,img->img,img->xsize*img->ysize*sizeof(rgb_group));
   pop_n_elems(1);
   THISOBJ->refs++;
   push_object(THISOBJ);
}

