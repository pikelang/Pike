/*
**! module Image
**! note
**!	$Id: colors.c,v 1.1 1999/01/22 01:01:28 mirar Exp $
**! class colortable
**!
**!	This object keeps colortable information,
**!	mostly for image re-coloring (quantization).
**!
**!	The object has color reduction, quantisation,
**!	mapping and dithering capabilities.
**!
**! see also: Image, Image.image, Image.font, Image.GIF
*/

#include "global.h"
#include <config.h>

RCSID("$Id: colors.c,v 1.1 1999/01/22 01:01:28 mirar Exp $");

#include "config.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "threads.h"
#include "builtin_functions.h"
#include "dmalloc.h"
#include "module_support.h"

#include "image.h"
#include "colortable.h"

static struct mapping *colors=NULL;
static struct object *colortable=NULL;
static struct array *colornames=NULL;

struct program *image_color_program=NULL;
extern struct program *image_colortable_program;

static struct pike_string *str_array;
static struct pike_string *str_string;
static struct pike_string *str_r;
static struct pike_string *str_g;
static struct pike_string *str_b;
static struct pike_string *str_h;
static struct pike_string *str_s;
static struct pike_string *str_v;

struct color_struct
{
   rgb_group rgb;
   struct pike_string *name;
};

static void make_colors(void)
{
   int n=0;

#define COLOR(name,R,G,B) \
   push_text(name); \
   push_int(R); \
   push_int(G); \
   push_int(B); \
   push_text(name); \
   push_object(clone_object(image_color_program,4)); \
   n++;
#include "colors.h"
#undef COLOR

   f_aggregate_mapping(n*2);
   colors=sp[-1].u.mapping;
   sp--;

   n=0;
#define COLOR(name,R,G,B) \
   push_int(R); \
   push_int(G); \
   push_int(B); \
   f_aggregate(3); \
   n++;
#include "colors.h"
#undef COLOR

   f_aggregate(n);
   colortable=clone_object(image_colortable_program,1);

   push_int(5);
   push_int(5);
   push_int(5);
   push_int(1);
   safe_apply(colortable,"cubicles",4);
   pop_stack();

   n=0;
#define COLOR(name,R,G,B) \
   push_text(name); \
   n++;
#include "colors.h"
#undef COLOR

   f_aggregate(n); \
   colornames=sp[-1].u.array;
   sp--;
}

#ifdef THIS
#undef THIS /* Needed for NT */
#endif
#define THIS ((struct color_struct*)(fp->current_storage))
#define THISOBJ (fp->current_object)

void init_color_struct(struct object *dummy)
{
   THIS->rgb.r=THIS->rgb.g=THIS->rgb.b=0;
   THIS->name=NULL;
}

void exit_color_struct(struct object *dummy)
{
   if (THIS->name) 
   {
      free_string(THIS->name);
      THIS->name=NULL;
   }
}

static void try_find_name(void)
{
   rgb_group d;

   if (!colors)
      make_colors();

   if (THIS->name) 
   {
      free_string(THIS->name);
      THIS->name=NULL;
   }

   image_colortable_map_image((struct neo_colortable*)colortable->storage,
			      &(THIS->rgb),&d,1,1);
   
   if (d.r==THIS->rgb.r &&
       d.g==THIS->rgb.g &&
       d.b==THIS->rgb.b)
   {
      unsigned short d2;
      image_colortable_index_16bit_image(
	 (struct neo_colortable*)colortable->storage,
	 &(THIS->rgb),&d2,1,1);

      if (d2<colornames->size)
      {
	 copy_shared_string(THIS->name,
			    colornames->item[d2].u.string);
      }
   }
}

void image_color_create(INT32 args)
{
   if (args==4) /* r,g,b,name */
   {
      INT32 r,g,b;
      get_all_args("Image.color->create()",args,"%i%i%i%W",
		   &r,&g,&b,&(THIS->name));

      THIS->rgb.r=(COLORTYPE)r;
      THIS->rgb.g=(COLORTYPE)g;
      THIS->rgb.b=(COLORTYPE)b;

      reference_shared_string(THIS->name);

      pop_n_elems(args);
   }
   else if (args==3) /* r,g,b */
   {
      INT32 r,g,b;
      get_all_args("Image.color->create()",args,"%i%i%i",
		   &r,&g,&b);

      THIS->rgb.r=(COLORTYPE)r;
      THIS->rgb.g=(COLORTYPE)g;
      THIS->rgb.b=(COLORTYPE)b;

      pop_n_elems(args);

      try_find_name();
   }
   else error("Image.color->create(): Illegal argument(s)\n");

   push_int(0);
}

void image_color_rgb(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->rgb.r);
   push_int(THIS->rgb.g);
   push_int(THIS->rgb.b);
   f_aggregate(3);
}

void image_color_name(INT32 args)
{
   if (THIS->name)
   {
      ref_push_string(THIS->name);
   }
   else
   {
      char buf[20];
      switch (sizeof(COLORTYPE))
      {
	 case 1: 
	    sprintf(buf,"#%02x%02x%02x",THIS->rgb.r,THIS->rgb.g,THIS->rgb.b); 
	    break;
	 case 2: 
	    sprintf(buf,"#%04x%04x%04x",THIS->rgb.r,THIS->rgb.g,THIS->rgb.b); 
	    break;
	 case 4: 
	    sprintf(buf,"#%08x%08x%08x",THIS->rgb.r,THIS->rgb.g,THIS->rgb.b); 
	    break;
      }
      push_text(buf);
   }
}

void image_color_cast(INT32 args)
{
   if (args!=1 ||
       sp[-1].type!=T_STRING)
      error("Image.color->cast(): Illegal argument(s)\n");
   
   if (sp[-1].u.string==str_array)
   {
      image_color_rgb(args);
      return;
   }
   if (sp[-1].u.string==str_string)
   {
      image_color_name(args);
      return;
   }
   error("Image.color->cast(): Can't cast to that\n");
}

void image_color_index(INT32 args)
{
   error("Image.color->`[]: Not implemented\n");
}

#define HEXTONUM(C) \
	(((C)>='0' && (C)<='9')?(C)-'0': \
	 ((C)>='a' && (C)<='f')?(C)-'a'+10: \
	 ((C)>='A' && (C)<='F')?(C)-'A'+10:-1)

void image_get_color(INT32 args)
{
   struct svalue s;

   if (args!=1) 
      error("Image.color[]: illegal number of args");
   
   if (!colors)
      make_colors();

   mapping_index_no_free(&s,colors,sp-1);
   if (s.type==T_INT)
   {
      if (sp[-1].type==T_STRING &&
	  sp[-1].u.string->len>=4 &&
	  sp[-1].u.string->size_shift==0 &&
	  sp[-1].u.string->str[0]=='#')
      {
	 /* #rgb, #rrggbb, #rrrgggbbb or #rrrrggggbbbb */
	 
	 unsigned long i=sp[-1].u.string->len-1,j,k,rgb[3];
	 unsigned char *src=sp[-1].u.string->str+1;
	 if (!(i%3))
	 {
	    i/=3;
	    for (j=0; j<3; j++)
	    {
	       int z=0;
	       for (k=0; k<i; k++)
		  z=z*16+HEXTONUM(*src),src++;
	       switch (i)
	       {
		  case 1: z=(z<<12)+(z<<8)+(z<<4)+(z<<0); break;
		  case 2: z=(z<<8)+(z<<0); break;
		  case 3: z=(z<<4)+(z>>8); break;
	       }
	       switch (sizeof(COLORTYPE))
	       {
		  case 1: z>>=8; break;
		  case 4: z<<=16; break;
	       }
	       rgb[j]=z;
	    }
	    pop_n_elems(args);
	    push_int((INT32)rgb[0]);
	    push_int((INT32)rgb[1]);
	    push_int((INT32)rgb[2]);
	    push_object(clone_object(image_color_program,3));

	    return;
	 }
      }

      /* try other stuff here */
      error("Image.color[]: no such color\n");
   }

   pop_stack();
   *(sp++)=s;
}

void image_make_color(INT32 args)
{
   struct svalue s;

   if (args==1 && sp[-args].type==T_STRING) 
   {
      image_get_color(args);
      return;
   }

   push_object(clone_object(image_color_program,args));
}

void image_colors_indices(INT32 args)
{
   pop_n_elems(args);
   ref_push_mapping(colors);
   f_indices(1);
}

void image_colors_values(INT32 args)
{
   pop_n_elems(args);
   ref_push_mapping(colors);
   f_values(1);
}

void init_image_colors(void)
{
   struct program *prg;
   struct pike_string *str;

   str_array=make_shared_string("array");
   str_string=make_shared_string("string");
   str_r=make_shared_string("r");
   str_g=make_shared_string("g");
   str_b=make_shared_string("b");
   str_h=make_shared_string("h");
   str_s=make_shared_string("s");
   str_v=make_shared_string("v");

   start_new_program();

   add_storage(sizeof(struct color_struct));
   set_init_callback(init_color_struct);
   set_exit_callback(exit_color_struct);

   add_function("create",image_color_create,
		"function(:void)",0);

   add_function("cast",image_color_cast,
		"function(string:array|string)",0);
   add_function("`[]",image_color_index,
		"function(string|int:int|function)",0);
   add_function("rgb",image_color_rgb,
		"function(:array)",0);
   add_function("name",image_color_name,
		"function(:string)",0);

   image_color_program=end_program();
   
   start_new_program();
   add_function("`[]",image_get_color,
		"function(string:object)",0);
   add_function("`()",image_make_color,
		"function(string|int...:object)",0);
   add_function("_indices",image_colors_indices,
		"function(:array(string))",0);
   add_function("_values",image_colors_values,
		"function(:array(object))",0);

   prg=end_program();
   push_object(clone_object(prg,0));
   free_program(prg);
   str=make_shared_string("color");
   add_constant(str,sp-1,0);
   free_string(str);
   pop_stack();
}

void exit_image_colors(void)
{
   if (image_color_program)
   {
      free_program(image_color_program);
      image_color_program=NULL;
   }
   if (colors)
   {
      free_mapping(colors);
      free_object(colortable);
      free_array(colornames);

      colors=NULL;
      colortable=NULL;
      colornames=NULL;
   }
   free_string(str_array);
   free_string(str_string);
   free_string(str_r);
   free_string(str_g);
   free_string(str_b);
   free_string(str_h);
   free_string(str_s);
   free_string(str_v);
}
