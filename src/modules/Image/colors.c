/*
**! module Image
**! note
**!	$Id: colors.c,v 1.2 1999/01/23 20:57:31 mirar Exp $
**! submodule color
**!
**!	This module keeps names and easy handling 
**!	for easy color support. It gives you an easy
**!	way to get colors from names.
**!
**!	A color is here an object, containing color
**!	information and methods for conversion, see below.
**!	
**! class color
**!	This is the color object. It has six readable variables,
**!	<tt>r</tt>, <tt>g</tt>, <tt>b</tt>, for the <i>red</i>, 
**!	<i>green</i> and <i>blue</i> values, 
**!	and <tt>h</tt>, <tt>s</tt>, <tt>v</tt>, for
**!	the <i>hue</i>, <i>saturation</i> anv <i>value</i> values.
**!
**! see also: Image, Image.image, Image.font, Image.GIF
*/

#include "global.h"
#include <config.h>

RCSID("$Id: colors.c,v 1.2 1999/01/23 20:57:31 mirar Exp $");

#include "config.h"

#include <math.h>

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
#include "operators.h"
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

void image_make_hsv_color(INT32 args); /* forward */

static void make_colors(void)
{
   static struct color
   {
      int r,g,b;
      char *name;
      struct pike_string *pname;
   } c[]={
#define COLOR(name,R,G,B) \
   {R,G,B,name,NULL},
#include "colors.h"
#undef COLOR
   };
   int i;
   const int n=sizeof(c)/sizeof(c[0]);

   for (i=0;i<n;i++)
   {
      push_text(c[i].name);
      copy_shared_string(c[i].pname,sp[-1].u.string);
      push_int(c[i].r);
      push_int(c[i].g);
      push_int(c[i].b);
      push_text(c[i].name);
      push_object(clone_object(image_color_program,4)); 
   }
   f_aggregate_mapping(n*2);
   colors=sp[-1].u.mapping;
   sp--;

   for (i=0;i<n;i++)
   {
      push_int(c[i].r);
      push_int(c[i].g);
      push_int(c[i].b);
      f_aggregate(3);
   }
   f_aggregate(n);
   colortable=clone_object(image_colortable_program,1);

   push_int(5);
   push_int(5);
   push_int(5);
   push_int(1);
   safe_apply(colortable,"cubicles",4);
   pop_stack();

   for (i=0;i<n;i++)
      push_string(c[i].pname);
   f_aggregate(n);

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

/*
**! method void create(int r,int g,int b)
**!	This is the main <ref>Image.color.color</ref> creation
**!	method, mostly for internal use. 
**----- internal note: it takes a fourth argument, name of color ---
**!
*/

void image_color_create(INT32 args)
{
   if (args==4) /* r,g,b,name */
   {
      INT32 r,g,b;
      get_all_args("Image.color->create()",args,"%i%i%i%W",
		   &r,&g,&b,&(THIS->name));

      if (r<0) r=0; else if (r>COLORMAX) r=COLORMAX;
      if (g<0) g=0; else if (g>COLORMAX) g=COLORMAX;
      if (b<0) b=0; else if (b>COLORMAX) b=COLORMAX;

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

      if (r<0) r=0; else if (r>COLORMAX) r=COLORMAX;
      if (g<0) g=0; else if (g>COLORMAX) g=COLORMAX;
      if (b<0) b=0; else if (b>COLORMAX) b=COLORMAX;

      THIS->rgb.r=(COLORTYPE)r;
      THIS->rgb.g=(COLORTYPE)g;
      THIS->rgb.b=(COLORTYPE)b;

      pop_n_elems(args);

      try_find_name();
   }
   else error("Image.color->create(): Illegal argument(s)\n");

   push_int(0);
}

/*
**! method array(int) rgb()
**! method array(int) hsv()
**! method int greylevel()
**! method int greylevel(int r, int g, int b)
**!	This is methods of getting information from an
**!	<ref>Image.color.color</ref> object. 
**!	
**!	They give an array of red, green and blue (rgb) values,
**!	hue, saturation and value (hsv) values,
**!     or the greylevel value. 
**!
**!	The greylevel is calculated by weighting red, green
**!	and blue. Default weights are 87, 127 and 41, respective, 
**!	and could be given by argument.
**!
**! returns array(int) respective int
**! see also: Image.color.color, grey
*/

void image_color_rgb(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->rgb.r);
   push_int(THIS->rgb.g);
   push_int(THIS->rgb.b);
   f_aggregate(3);
}

void image_color_greylevel(INT32 args)
{
   INT32 r,g,b;
   if (args==0)
   {
      r=87;
      g=127;
      b=41;
   }
   else
   {
      get_all_args("Image.color->grey()",args,"%i%i%i",&r,&g,&b);
   }
   pop_n_elems(args);
   if (r+g+b==0) r=g=b=1;
   push_int((r*THIS->rgb.r+g*THIS->rgb.g+b*THIS->rgb.b)/(r+g+b));
}

#define MAX3(X,Y,Z) MAXIMUM(MAXIMUM(X,Y),Z)

void image_color_hsv(INT32 args)
{
   float max, min;
   float r,g,b, delta;
   float h, s, v;

   pop_n_elems(args);

   if((THIS->rgb.r==THIS->rgb.g) && (THIS->rgb.g==THIS->rgb.b)) 
   {
      push_int(0);
      push_int(0);
      push_int(THIS->rgb.r);
      f_aggregate(3);
      return;
   }
  
   r = (float)(THIS->rgb.r)/((float)COLORMAX); 
   g = (float)(THIS->rgb.g)/((float)COLORMAX); 
   b = (float)(THIS->rgb.b)/((float)COLORMAX);
   max = MAX3(r,g,b);
   min = -(MAX3(-r,-g,-b));

   v = max;

   if(max != 0.0)
      s = (max - min)/max;
   else
      error("internal error, max==0.0\n");

   delta = max-min;

   if(r==max) h = (g-b)/delta;
   else if(g==max) h = 2+(b-r)/delta;
   else /*if(b==max)*/ h = 4+(r-g)/delta;
   h *= 60; // now in degrees.
   if(h<0) h+=360;

   push_int((int)((h/360.0)*COLORMAX+0.5));
   push_int((int)(s*COLORMAX+0.5));
   push_int((int)(v*COLORMAX+0.5));
   f_aggregate(3);
}

/*
**! method object grey()
**! method object grey(int red,int green,int blue)
**!	Gives a new color, containing a grey color,
**!	which is calculated by the <ref>greylevel</ref> method.
**! returns a new <ref>Image.color.color</ref> object
**! see also: greylevel
*/

void image_color_grey(INT32 args)
{
   image_color_greylevel(args);
   stack_dup();
   stack_dup();
   push_object(clone_object(image_color_program,3));
}

/*
**! method string name()
**!	Information method, gives the name of this color,
**!	if it exists in the database, or color of format
**!	<tt>"#rrggbb"</tt>, where rr, gg, bb is the hexadecimal
**!	representation of the red, green and blue values, respectively.
**! returns a new <ref>Image.color.color</ref> object
**! see also: rgb, hsv, Image.color
*/

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

/*
**! method array|string cast()
**!	cast the object to an array, representing red, green
**!	and blue (equal to <tt>-><ref>rgb</ref>()</tt>), or
**!	to a string, giving the name (equal to <tt>-><ref>name</ref>()</tt>).
**! returns the name as string or rgb as array
**! see also: rgb, name
*/

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
   struct svalue s;

   if (args!=1)
      error("Image.color[]: illegal number of arguments\n");

   object_index_no_free2(&s,THISOBJ,sp-1);
   if (s.type==T_INT && sp[-1].type==T_STRING)
   {
      if (sp[-1].u.string==str_r)
      {
	 pop_stack();
	 push_int(THIS->rgb.r);
	 return;
      }
      if (sp[-1].u.string==str_g)
      {
	 pop_stack();
	 push_int(THIS->rgb.g);
	 return;
      }
      if (sp[-1].u.string==str_b)
      {
	 pop_stack();
	 push_int(THIS->rgb.b);
	 return;
      }
      if (sp[-1].u.string==str_h)
      {
	 pop_stack();
	 image_color_hsv(0);
	 push_int(0);
	 f_index(2);
	 return;
      }
      if (sp[-1].u.string==str_s)
      {
	 pop_stack();
	 image_color_hsv(0);
	 push_int(1);
	 f_index(2);
	 return;
      }
      if (sp[-1].u.string==str_v)
      {
	 pop_stack();
	 image_color_hsv(0);
	 push_int(2);
	 f_index(2);
	 return;
      }
   }

   pop_stack();
   *(sp++)=s;
}

/*
**! method array|string cast()
**!	cast the object to an array, representing red, green
**!	and blue (equal to <tt>-><ref>rgb</ref>()</tt>), or
**!	to a string, giving the name (equal to <tt>-><ref>name</ref>()</tt>).
**! returns the name as string or rgb as array
**! see also: rgb, name
*/

void image_color_equal(INT32 args)
{
   if (args!=1) 
      error("Image.color->`==: illegal number of arguments");

   if (sp[-1].type==T_OBJECT)
   {
      struct color_struct *other;
      other=(struct color_struct*)
	 get_storage(sp[-1].u.object,image_color_program);
      if (other&&
	  other->rgb.r==THIS->rgb.r &&
	  other->rgb.g==THIS->rgb.g &&
	  other->rgb.b==THIS->rgb.b)
      {
	 pop_stack();
	 push_int(1);
	 return;
      }
   }
   else if (sp[-1].type==T_ARRAY)
   {
      if (sp[-1].u.array->size==3 &&
	  sp[-1].u.array->item[0].type==T_INT &&
	  sp[-1].u.array->item[1].type==T_INT &&
	  sp[-1].u.array->item[2].type==T_INT &&
	  sp[-1].u.array->item[0].u.integer == THIS->rgb.r &&
	  sp[-1].u.array->item[0].u.integer == THIS->rgb.g &&
	  sp[-1].u.array->item[0].u.integer == THIS->rgb.b)
      {
	 pop_stack();
	 push_int(1);
	 return;
      }
   }
   else if (sp[-1].type==T_INT)
   {
      if (sp[-1].u.integer == THIS->rgb.r &&
	  THIS->rgb.r==THIS->rgb.g &&
	  THIS->rgb.r==THIS->rgb.b)
      {
	 pop_stack();
	 push_int(1);
	 return;
      }
   }
   pop_stack();
   push_int(0);
}


void image_color_light(INT32 args)
{
   pop_n_elems(args);
   image_color_hsv(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-1].u.integer+=50;
   image_make_hsv_color(3);
}

void image_color_dark(INT32 args)
{
   pop_n_elems(args);
   image_color_hsv(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-1].u.integer-=50;
   image_make_hsv_color(3);
}

void image_color_neon(INT32 args)
{
   pop_n_elems(args);
   image_color_hsv(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-1].u.integer=255;
   sp[-2].u.integer=255;
   image_make_hsv_color(3);
}

void image_color_dull(INT32 args)
{
   pop_n_elems(args);
   image_color_hsv(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-2].u.integer-=50;
   image_make_hsv_color(3);
}

void image_color_bright(INT32 args)
{
   pop_n_elems(args);
   image_color_hsv(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-2].u.integer+=50;
   image_make_hsv_color(3);
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
      object_index_no_free2(&s,THISOBJ,sp-1);
      if (s.type!=T_INT)
      {
	 pop_stack();
	 *(sp++)=s;
	 return;
      }

      if (sp[-1].type==T_STRING &&
	  sp[-1].u.string->size_shift==0)
      {
	 if (sp[-1].u.string->len>=4 &&
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
		  {
		     if (HEXTONUM(*src)==-1)
			error("Image.color[]: no such color (trying hexadecimal number)\n");
		     z=z*16+HEXTONUM(*src),src++;
		  }
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
	 if (sp[-1].u.string->len>5 &&
	     memcmp(sp[-1].u.string->str,"light",5)==0)
	 {
	    fprintf(stderr,"light\n");
	    push_int(5);
	    push_int(1000000);
	    f_index(3);
	    image_get_color(1);
	    if (sp[-1].type!=T_OBJECT)
	       error("no such color (internal wierdness)\n");
	    safe_apply(sp[-1].u.object,"light",0);
	    stack_swap();
	    pop_stack();
	    return;
	 }
	 if (sp[-1].u.string->len>4 &&
	     memcmp(sp[-1].u.string->str,"dark",4)==0)
	 {
	    push_int(4);
	    push_int(1000000);
	    f_index(3);
	    image_get_color(1);
	    if (sp[-1].type!=T_OBJECT)
	       error("no such color (internal wierdness)\n");
	    safe_apply(sp[-1].u.object,"dark",0);
	    stack_swap();
	    pop_stack();
	    return;
	 }
	 if (sp[-1].u.string->len>4 &&
	     memcmp(sp[-1].u.string->str,"neon",4)==0)
	 {
	    push_int(4);
	    push_int(1000000);
	    f_index(3);
	    image_get_color(1);
	    if (sp[-1].type!=T_OBJECT)
	       error("no such color (internal wierdness)\n");
	    safe_apply(sp[-1].u.object,"neon",0);
	    stack_swap();
	    pop_stack();
	    return;
	 }
	 if (sp[-1].u.string->len>6 &&
	     memcmp(sp[-1].u.string->str,"bright",4)==0)
	 {
	    push_int(6);
	    push_int(1000000);
	    f_index(3);
	    image_get_color(1);
	    if (sp[-1].type!=T_OBJECT)
	       error("no such color (internal wierdness)\n");
	    safe_apply(sp[-1].u.object,"bright",0);
	    stack_swap();
	    pop_stack();
	    return;
	 }
	 if (sp[-1].u.string->len>4 &&
	     memcmp(sp[-1].u.string->str,"dull",4)==0)
	 {
	    push_int(4);
	    push_int(1000000);
	    f_index(3);
	    image_get_color(1);
	    if (sp[-1].type!=T_OBJECT)
	       error("no such color (internal wierdness)\n");
	    safe_apply(sp[-1].u.object,"dull",0);
	    stack_swap();
	    pop_stack();
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

void image_make_rgb_color(INT32 args)
{
   if (args!=3) 
   {
      error("Image.color.rgb(): illegal number of arguments\n");
      return;
   }

   push_object(clone_object(image_color_program,args));
}

void image_make_hsv_color(INT32 args)
{
   float h,s,v;
   INT32 hi,si,vi;
   float r=0,g=0,b=0; /* to avoid warning */

   get_all_args("Image.color.hsv()",args,"%i%i%i",
		&hi,&si,&vi);

   if (hi<0) hi=0; else if (hi>COLORMAX) hi=COLORMAX;
   if (si<0) si=0; else if (si>COLORMAX) si=COLORMAX;
   if (vi<0) vi=0; else if (vi>COLORMAX) vi=COLORMAX;
   
   h = (hi/((float)COLORMAX))*(360.0/60.0);
   s = si/((float)COLORMAX);
   v = vi/((float)COLORMAX);
     
   if(s==0.0 || si==0)
   {
      r = g = b = v;
   } else {
#define i floor(h)
#define f (h-i)
#define p (v * (1 - s))
#define q (v * (1 - (s * f)))
#define t (v * (1 - (s * (1 -f))))
      switch((int)i)
      {
	 case 6: // 360 degrees. Same as 0..
	 case 0: r = v;	 g = t;	 b = p;	 break;
	 case 1: r = q;	 g = v;	 b = p;	 break;
	 case 2: r = p;  g = v;	 b = t;	 break;
	 case 3: r = p;	 g = q;	 b = v;	 break;
	 case 4: r = t;	 g = p;	 b = v;	 break;
	 case 5: r = v;	 g = p;	 b = q;	 break;
      }
   }
#undef i
#undef f
#undef p
#undef q
#undef t
#define FOO(X) ((int)((X)<0.0?0:(X)>1.0?COLORMAX:(int)((X)*((float)COLORMAX)+0.5)))
   push_int(FOO(r));
   push_int(FOO(g));
   push_int(FOO(b));

   push_object(clone_object(image_color_program,args));
}

void image_make_greylevel_color(INT32 args)
{
   INT32 i;

   get_all_args("Image.color.greylevel()",args,"%i",&i);
   
   push_int(i);
   push_int(i);
   push_int(i);

   push_object(clone_object(image_color_program,3));
}

void image_colors_indices(INT32 args)
{
   pop_n_elems(args);
   if (!colors) make_colors();
   ref_push_mapping(colors);
   f_indices(1);
}

void image_colors_values(INT32 args)
{
   pop_n_elems(args);
   if (!colors) make_colors();
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

   /* color info methods */

   add_function("cast",image_color_cast,
		"function(string:array|string)",OPT_TRY_OPTIMIZE);
   add_function("`[]",image_color_index,
		"function(string|int:int|function)",OPT_TRY_OPTIMIZE);
   add_function("`->",image_color_index,
		"function(string|int:int|function)",OPT_TRY_OPTIMIZE);
   add_function("name",image_color_name,
		"function(:string)",OPT_TRY_OPTIMIZE);
   add_function("`==",image_color_equal,
		"function(object|int:int)",OPT_TRY_OPTIMIZE);

   add_function("rgb",image_color_rgb,
		"function(:array)",OPT_TRY_OPTIMIZE);
   add_function("hsv",image_color_hsv,
		"function(:array)",OPT_TRY_OPTIMIZE);
   add_function("greylevel",image_color_greylevel,
		"function(:int)|function(int,int,int:int)",OPT_TRY_OPTIMIZE);

   /* color conversion methods */

   add_function("grey",image_color_grey,
		"function(:object)|function(int,int,int:object)",
		OPT_TRY_OPTIMIZE);

   add_function("light",image_color_light,
		"function(:object)",OPT_TRY_OPTIMIZE);
   add_function("dark",image_color_dark,
		"function(:object)",OPT_TRY_OPTIMIZE);
   add_function("neon",image_color_neon,
		"function(:object)",OPT_TRY_OPTIMIZE);
   add_function("bright",image_color_bright,
		"function(:object)",OPT_TRY_OPTIMIZE);
   add_function("dull",image_color_dull,
		"function(:object)",OPT_TRY_OPTIMIZE);

   image_color_program=end_program();
   
   start_new_program();
   add_function("`[]",image_get_color,
		"function(string:object)",OPT_TRY_OPTIMIZE);
   add_function("`()",image_make_color,
		"function(string|int...:object)",OPT_TRY_OPTIMIZE);
   add_function("rgb",image_make_rgb_color,
		"function(int,int,int:object)",OPT_TRY_OPTIMIZE);
   add_function("hsv",image_make_hsv_color,
		"function(int,int,int:object)",OPT_TRY_OPTIMIZE);
   add_function("greylevel",image_make_greylevel_color,
		"function(int:object)",OPT_TRY_OPTIMIZE);
   add_function("_indices",image_colors_indices,
		"function(:array(string))",OPT_TRY_OPTIMIZE);
   add_function("_values",image_colors_values,
		"function(:array(object))",OPT_TRY_OPTIMIZE);

   add_program_constant("color",image_color_program,0);

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
