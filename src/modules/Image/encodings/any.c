/* $Id: any.c,v 1.2 1999/02/24 03:33:34 mirar Exp $ */

/*
**! module Image
**! note
**!	$Id: any.c,v 1.2 1999/02/24 03:33:34 mirar Exp $
**! submodule ANY
**!
**!	This method calls the other decoding methods
**!	and has some heuristics for what type of image
**!	this is.
**!
**!	Methods:
**!	<ref>decode</ref>, <ref>decode_trans</ref>,
**!	<ref>_decode</ref>
**!
**! see also: Image
**!
*/
#include "global.h"

#include <math.h>
#include <ctype.h>

#include "stralloc.h"
RCSID("$Id: any.c,v 1.2 1999/02/24 03:33:34 mirar Exp $");
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "threads.h"

void image_any__decode(INT32 args)
{
   if (args!=1 || sp[-args].type!=T_STRING)
      error("Image.ANY.decode: illegal arguments\n");
   
   if (sp[-args].u.string->len<4)
      error("Image.ANY.decode: too short string\n");

#define CHAR2(a,b) ((((unsigned char)(a))<<8)|((unsigned char)(b)))
   /* ok, try the heuristics */
   switch (CHAR2(sp[-args].u.string->str[0],sp[-args].u.string->str[1]))
   {
      case CHAR2('P','1'):
      case CHAR2('P','2'):
      case CHAR2('P','3'):
      case CHAR2('P','4'):
      case CHAR2('P','5'):
      case CHAR2('P','6'):
      case CHAR2('P','7'):
	 /* ok, a PNM */
	 img_pnm_decode(1);
	 push_text("image/x-pnm");
	 goto simple_image;

      case CHAR2(255,216):
	 /* JFIF */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("JPEG");
	 f_index(2);
	 push_text("decode");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 push_text("image/jpeg");
	 goto simple_image;

      case CHAR2('P','N'):
	 /* PNG */
	 push_text("Image");
	 push_int(0);
	 SAFE_APPLY_MASTER("resolv",2);
	 push_text("PNG");
	 f_index(2);
	 push_text("_decode");
	 f_index(2);
	 stack_swap();
	 f_call_function(2);
	 return;

      case CHAR2('G','I'):
	 /* GIF */
	 image_gif__decode(1);
	 error("can't handle gif's yet\n");
	 return;

      case CHAR2(0,0):
	 switch (CHAR2(sp[-args].u.string->str[2],sp[-args].u.string->str[3]))
	 {
	    case CHAR2(0,'k'):
	       /* XWD */
	       image_xwd__decode(1);
	       return; /* done */
	 }
	 
      default:
	 error("Unknown image format.\n");	 
   }

simple_image:
   /* on stack: object image,string type */
   f_aggregate(2);
   push_text("image");
   push_text("type");
   f_aggregate(2);
   stack_swap();
   f_mkmapping(2);
   return;
}

/** module *******************************************/

void init_image_any(void)
{
   struct program *p;

   start_new_program();
   
   add_function("_decode",image_any__decode,
		"function(string:mapping)",0);
   /** done **/

   p=end_program();
   push_object(clone_object(p,0));
   {
     struct pike_string *s=make_shared_string("ANY");
     add_constant(s,sp-1,0);
     free_string(s);
   }
   free_program(p);
   pop_stack();
}

void exit_image_any(void)
{
}
