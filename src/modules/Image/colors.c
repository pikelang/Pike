/*
**! module Image
**! note
**!	$Id: colors.c,v 1.14 1999/04/25 20:34:00 grubba Exp $
**! submodule Color
**!
**!	This module keeps names and easy handling 
**!	for easy color support. It gives you an easy
**!	way to get colors from names.
**!
**!	A color is here an object, containing color
**!	information and methods for conversion, see below.
**!
**!	<ref>Image.Color</ref> can be called to make a color object.
**!	<ref>Image.Color()</ref> takes the following arguments:
**!	<pre>
**!	Image.Color(string name)          // "red"
**!	Image.Color(string prefix_string) // "lightblue"
**!	Image.Color(string hex_name)      // "#ff00ff"
**!	Image.Color(string cmyk_string)   // "%17,42,0,19.4"
**!	Image.Color(string hsv_string)    // "%@327,90,32"
**!	Image.Color(int red, int green, int blue) 
**!     </pre>
**!
**!	The color names available can be listed by using indices 
**!	on Image.Color. The colors are available by name directly 
**!	as <tt>Image.Color.name</tt>, too:
**!	<pre>
**!	...Image.Color.red...
**!	...Image.Color.green...
**!	or, maybe
**!	import Image.Color;
**!	...red...
**!	...green...
**!	...lightgreen...
**!	</pre>
**!
**!	Giving red, green and blue values is equal to calling
**!	<ref>Image.Color.rgb</ref>().
**!	
**!	The prefix_string method is a form for getting modified 
**!	colors, it understands all modifiers
**!	(<link to=Image.Color.Color.light>light</link>,
**!	<link to=Image.Color.Color.dark>dark</link>,
**!	<link to=Image.Color.Color.bright>bright</link>,
**!	<link to=Image.Color.Color.dull>dull</link> and 
**!	<link to=Image.Color.Color.neon>neon</link>). Simply  use
**!	"method"+"color"; (as in <tt>lightgreen</tt>, 
**!	<tt>dullmagenta</tt>, <tt>lightdullorange</tt>).
**!
**!	The <tt>hex_name</tt> form is a simple 
**!	<tt>#rrggbb</tt> form, as in HTML or X-program argument.
**!	A shorter form (<tt>#rgb</tt>) is also accepted. This
**!	is the inverse to the <ref>Image.Color.Color->hex</ref>()
**!	method.
**!
**!	The <tt>cmyk_string</tt> is a string form of giving
**!	<i>cmyk</i> (cyan, magenta, yellow, black) color. These
**!	values are floats representing percent.
**!
**!	The <tt>hsv_string</tt> is another hue, saturation, value
**!	representation, but in floats; hue is in degree range (0..360),
**!	and saturation and value is given in percent. <i>This is not
**!	the same as returned or given to the <ref>hsv</ref>() methods!</i>
**!
**! see also: Image.Color.Color->name, Image.Color.Color->rgb
**!
**! added:
**!	pike 0.7
**!	
**! note: 
**!	<tt>Image.Color["something"]</tt> will never(!) generate an error, 
**!	but a zero_type 0, if the color is unknown. This is enough
**!	to give the error "not present in module", if used 
**!	as <tt>Image.Color.something</tt>, though.
**!
**!     If you are using colors from for instance a webpage, you might
**!	want to create the color from <ref>Image.Color.guess</ref>(),
**!     since that method is more tolerant for mistakes and errors.
**!
**!     <tt>Image.Color</tt>() is case- and space-sensitive.
**!	Use <ref>Image.Color.guess</ref>() to catch all variants.
**!
**!	and subtract with a space (lower_case(x)-" ") to make
**!	sure you get all variants.
**!	
**! see also: Image.Color.Color, Image.Color.guess, Image, Image.Colortable
**!
**! class Color
**!	This is the color object. It has six readable variables,
**!	<tt>r</tt>, <tt>g</tt>, <tt>b</tt>, for the <i>red</i>, 
**!	<i>green</i> and <i>blue</i> values, 
**!	and <tt>h</tt>, <tt>s</tt>, <tt>v</tt>, for
**!	the <i>hue</i>, <i>saturation</i> anv <i>value</i> values.
*/

#include "global.h"
#include <config.h>

RCSID("$Id: colors.c,v 1.14 1999/04/25 20:34:00 grubba Exp $");

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
#include "opcodes.h"

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

static struct pike_string *no_name;

/* forward */
static void _image_make_rgbl_color(INT32 r,INT32 g,INT32 b); 
static void _image_make_rgbf_color(float r,float g,float b);
static void image_make_hsv_color(INT32 args); 
static void image_make_cmyk_color(INT32 args);
static void image_make_color(INT32 args);
static void image_make_rgb_color(INT32 args);

struct html_color
{
   int r,g,b;
   char *name;
   struct pike_string *pname;
} html_color[]=
{{0,0,0,"black",NULL}, {255,255,255,"white",NULL},
 {0,128,0,"green",NULL}, {192,192,192,"silver",NULL},
 {0,255,0,"lime",NULL}, {128,128,128,"gray",NULL}, 
 {128,128,0,"olive",NULL}, {255,255,0,"yellow",NULL}, 
 {128,0,0,"maroon",NULL}, {0,0,128,"navy",NULL}, 
 {255,0,0,"red",NULL}, {0,0,255,"blue",NULL},
 {128,0,128,"purple",NULL}, {0,128,128,"teal",NULL}, 
 {255,0,255,"fuchsia",NULL}, {0,255,255,"aqua",NULL}};

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

   for (i=0; (size_t)i<sizeof(html_color)/sizeof(html_color[0]); i++)
      html_color[i].pname=make_shared_string(html_color[i].name);

   for (i=0;i<n;i++)
   {
      struct color_struct *cs;
      push_text(c[i].name);
      copy_shared_string(c[i].pname,sp[-1].u.string);

      push_object(clone_object(image_color_program,0)); 
      cs=(struct color_struct*)
	 get_storage(sp[-1].u.object,image_color_program);
      cs->rgb.r=(COLORTYPE)c[i].r;
      cs->rgb.g=(COLORTYPE)c[i].g;
      cs->rgb.b=(COLORTYPE)c[i].b;
      RGB_TO_RGBL(cs->rgbl,cs->rgb);
      copy_shared_string(cs->name,c[i].pname);
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
   if (!colortable)
      fatal("couldn't create colortable\n");

   push_int(12);
   push_int(12);
   push_int(12);
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

static void init_color_struct(struct object *dummy)
{
   THIS->rgb.r=THIS->rgb.g=THIS->rgb.b=0;
   THIS->name=NULL;
}

static void exit_color_struct(struct object *dummy)
{
   if (THIS->name) 
   {
      free_string(THIS->name);
      THIS->name=NULL;
   }
}

void _img_nct_map_to_flat_cubicles(rgb_group *s,
				   rgb_group *d,
				   int n,
				   struct neo_colortable *nct,
				   struct nct_dither *dith,
				   int rowlen);

static void try_find_name(struct color_struct *this)
{
   rgb_group d;
   static struct nct_dither dith = { NCTD_NONE,NULL,NULL,NULL,NULL,-1 };

   if (!colors)
      make_colors();

   if (this->name) 
      fatal("try_find_name called twice\n");

   if (this->rgbl.r!=COLOR_TO_COLORL(this->rgb.r) ||
       this->rgbl.g!=COLOR_TO_COLORL(this->rgb.g) ||
       this->rgbl.b!=COLOR_TO_COLORL(this->rgb.b))
   {
      copy_shared_string(this->name,no_name);
      return; 
   }

   _img_nct_map_to_flat_cubicles(&(this->rgb),&d,1,
				 (struct neo_colortable*)colortable->storage,
				 &dith,1);
   
   if (d.r==this->rgb.r &&
       d.g==this->rgb.g &&
       d.b==this->rgb.b)
   {
      unsigned short d2;
      image_colortable_index_16bit_image(
	 (struct neo_colortable*)colortable->storage,
	 &(this->rgb),&d2,1,1);

      if (d2<colornames->size)
      {
	 copy_shared_string(this->name,
			    colornames->item[d2].u.string);
	 return;
      }
   }
   copy_shared_string(this->name,no_name);
}

/*
**! method void create(int r,int g,int b)
**!	This is the main <ref>Image.Color.Color</ref> creation
**!	method, mostly for internal use. 
**----- internal note: it takes a fourth argument, name of color ---
**!
*/
/*
**! method array(int) rgb()
**! method array(int) hsv()
**! method array(int) cmyk()
**! method int greylevel()
**! method int greylevel(int r, int g, int b)
**!	This is methods of getting information from an
**!	<ref>Image.Color.Color</ref> object. 
**!	
**!	They give an array of 
**!	red, green and blue (rgb) values (color value),<br>
**!	hue, saturation and value (hsv) values (range as color value), <br>
**!	cyan, magenta, yellow, black (cmyk) values (in percent)	<br>
**!     or the greylevel value (range as color value). 
**!
**!	The greylevel is calculated by weighting red, green
**!	and blue. Default weights are 87, 127 and 41, respective, 
**!	and could be given by argument.
**!
**! returns array(int) respective int
**! see also: Image.Color.Color, grey
*/

static void image_color_rgb(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->rgb.r);
   push_int(THIS->rgb.g);
   push_int(THIS->rgb.b);
   f_aggregate(3);
}

static void image_color_rgbf(INT32 args)
{
   pop_n_elems(args);
   push_float(COLORL_TO_FLOAT(THIS->rgbl.r));
   push_float(COLORL_TO_FLOAT(THIS->rgbl.g));
   push_float(COLORL_TO_FLOAT(THIS->rgbl.b));
   f_aggregate(3);
}

static void image_color_greylevel(INT32 args)
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
      get_all_args("Image.Color.Color->greylevel()",args,"%i%i%i",&r,&g,&b);
   }
   pop_n_elems(args);
   if (r+g+b==0) r=g=b=1;
   push_int((r*THIS->rgb.r+g*THIS->rgb.g+b*THIS->rgb.b)/(r+g+b));
}

#define MAX3(X,Y,Z) MAXIMUM(MAXIMUM(X,Y),Z)

static void image_color_hsvf(INT32 args)
{
   float max, min;
   float r,g,b, delta;
   float h, s, v;

   pop_n_elems(args);

   if((THIS->rgb.r==THIS->rgb.g) && (THIS->rgb.g==THIS->rgb.b)) 
   {
      push_float(0);
      push_float(0);
      push_float(COLORL_TO_FLOAT(THIS->rgbl.r));
      f_aggregate(3);
      return;
   }
  
   r = COLORL_TO_FLOAT(THIS->rgbl.r);
   g = COLORL_TO_FLOAT(THIS->rgbl.g);
   b = COLORL_TO_FLOAT(THIS->rgbl.b);
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
   h *= 60; /* now in degrees. */
   if(h<0) h+=360;

   push_float(h);
   push_float(s);
   push_float(v);
   f_aggregate(3);
}

static void image_color_hsv(INT32 args)
{
   float h,s,v;
   image_color_hsvf(args);
   h=sp[-1].u.array->item[0].u.float_number;
   s=sp[-1].u.array->item[1].u.float_number;
   v=sp[-1].u.array->item[2].u.float_number;

   pop_stack();
   push_int(FLOAT_TO_COLOR(h/360.0));
   push_int(FLOAT_TO_COLOR(s));
   push_int(FLOAT_TO_COLOR(v));
   f_aggregate(3);
}

static void image_color_cmyk(INT32 args)
{
   float c,m,y,k;
   float r,g,b;
   pop_n_elems(args);

   r=COLORL_TO_FLOAT(THIS->rgbl.r);
   g=COLORL_TO_FLOAT(THIS->rgbl.g);
   b=COLORL_TO_FLOAT(THIS->rgbl.b);

   k=1.0-MAX3(r,g,b);

   c=1.0-r-k;
   m=1.0-g-k;
   y=1.0-b-k;

   push_float(c*100.0);
   push_float(m*100.0);
   push_float(y*100.0);
   push_float(k*100.0);
   f_aggregate(4);
}

/*
**! method object grey()
**! method object grey(int red,int green,int blue)
**!	Gives a new color, containing a grey color,
**!	which is calculated by the <ref>greylevel</ref> method.
**! returns a new <ref>Image.Color.Color</ref> object
**! see also: greylevel
*/

static void image_color_grey(INT32 args)
{
   image_color_greylevel(args);
   stack_dup();
   stack_dup();
   image_make_rgb_color(3);
}

/*
**! method string hex()
**! method string hex(int n)
**! method string name()
**! method string html()
**!	Information methods.
**!
**!	<ref>hex</ref>() simply gives a string on the <tt>#rrggbb</tt>
**!	format. If <tt>n</tt> is given, the number of significant
**!	digits is set to this number. 
**!     (Ie, <tt>n=3</tt> gives <tt>#rrrgggbbb</tt>.)
**!
**!	<ref>name</ref>() is a simplified method; 
**!	if the color exists in the database, the name is returned,
**!	per default is the <ref>hex</ref>() method use.
**!
**!	<ref>html</ref>() gives the <tt>HTML</tt> name of 
**!	the color, or the <ref>hex</ref>(2) if it isn't one
**!	of the 16 <tt>HTML</tt> colors.
**!
**! returns a new <ref>Image.Color.Color</ref> object
**! see also: rgb, hsv, Image.Color
*/

static void image_color_hex(INT32 args)
{
   char buf[80];
   INT32 i=sizeof(COLORTYPE)*2;

   if (args)
      get_all_args("Image.Color.Color->hex()",args,"%i",&i);

   pop_n_elems(args);
   if (i<1)
   {
      push_text("#");  /* stupid */
      return;
   }
   else if (i!=sizeof(COLORTYPE)*2)
   {
      int sh;
      if (i>8) i=8;

      sh=4*(sizeof(COLORTYPE)*2-i);
      if (sh>0)
	 sprintf(buf,"#%0*x%0*x%0*x",i,THIS->rgb.r>>sh,
		 i,THIS->rgb.g>>sh,i,THIS->rgb.b>>sh); 
      else
      {
	 unsigned INT32 r=THIS->rgbl.r;
	 unsigned INT32 g=THIS->rgbl.g;
	 unsigned INT32 b=THIS->rgbl.b;
	 sh=COLORLBITS-i*4;
	 if (sh<0) 
	 {
	    r=(r<<-sh)+(r>>(COLORLBITS+sh));
	    g=(g<<-sh)+(g>>(COLORLBITS+sh));
	    b=(b<<-sh)+(b>>(COLORLBITS+sh));
	    sh=0;
	 }
	 sprintf(buf,"#%0*x%0*x%0*x",
		 i,r>>sh,i,g>>sh,i,b>>sh);
      }
   }
   else
      switch (sizeof(COLORTYPE)) /* constant */
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
	 default:
	    error("unknown size of colortype\n");
      }
   push_text(buf);
}

static void image_color_html(INT32 args)
{
   int i;

   if (!colors) make_colors();

   pop_n_elems(args);

   for (i=0; (size_t)i<sizeof(html_color)/sizeof(html_color[0]); i++)
      if (THIS->rgb.r==html_color[i].r && 
	  THIS->rgb.g==html_color[i].g && 
	  THIS->rgb.b==html_color[i].b)
      {
	 ref_push_string(html_color[i].pname);
	 return;
      }

   push_int(2); 
   image_color_hex(1);
}

static void image_color_name(INT32 args)
{
   pop_n_elems(args);
   if (!THIS->name) try_find_name(THIS);

   if (THIS->name==no_name)
      image_color_hex(0);
   else
      ref_push_string(THIS->name);
}

/*
**! method array|string cast()
**!	cast the object to an array, representing red, green
**!	and blue (equal to <tt>-><ref>rgb</ref>()</tt>), or
**!	to a string, giving the name (equal to <tt>-><ref>name</ref>()</tt>).
**! returns the name as string or rgb as array
**! see also: rgb, name
*/

static void image_color_cast(INT32 args)
{
   if (args!=1 ||
       sp[-1].type!=T_STRING)
      error("Image.Color.Color->cast(): Illegal argument(s)\n");
   
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
   error("Image.Color.Color->cast(): Can't cast to that\n");
}

static void image_color_index(INT32 args)
{
   struct svalue s;

   if (args!=1)
      error("Image.Color[]: illegal number of arguments\n");

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
**! method int `==(object other_color)
**! method int `==(array(int) rgb)
**! method int `==(int greylevel)
**! method int `==(string name)
**!	Compares this object to another color,
**!	or color name. Example:
**!	<pre>
**!	object red=Image.Color.red;
**!	object other=Image.Color. ...;
**!	object black=Image.Color.black;
**!	
**!	if (red==other) ...
**!     if (red==({255,0,0})) ...
**!     if (black==0) ... 
**!     if (red=="red") ...
**!	</pre>
**!
**! returns 1 or 0
**! see also: rgb, grey, name
**! note:
**!	The other datatype (not color object) must be to the right!
*/

static void image_color_equal(INT32 args)
{
   if (args!=1) 
      error("Image.Color.Color->`==: illegal number of arguments");

   if (sp[-1].type==T_OBJECT)
   {
      struct color_struct *other;
      other=(struct color_struct*)
	 get_storage(sp[-1].u.object,image_color_program);
      if (other&&
	  other->rgbl.r==THIS->rgbl.r &&
	  other->rgbl.g==THIS->rgbl.g &&
	  other->rgbl.b==THIS->rgbl.b)
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
   else if (sp[-1].type==T_STRING)
   {
      if (!THIS->name)
	 try_find_name(THIS);
      if (sp[-1].u.string==THIS->name && THIS->name!=no_name)
      {
	 pop_stack();
	 push_int(1);
	 return;
      }
   }
   pop_stack();
   push_int(0);
}

static void image_color___hash(INT32 args)
{
   pop_n_elems(args);
   push_int(((THIS->rgb.r<<16)+(THIS->rgb.g<<8)+THIS->rgb.b)
	    +(THIS->rgbl.r+THIS->rgbl.g+THIS->rgbl.b));
}


/*
**! method object light()
**! method object dark()
**! method object neon()
**! method object bright()
**! method object dull()
**!	Color modification methods. These returns
**!	a new color.
**!	<table>
**!	<tr><th>method</th><th width=50%>effect</th>
**!	<th>h</th><th>s</th><th>v</th><th>as</th></tr>
**!	<tr><td>light </td><td>raise light level</td><td>±0</td><td> ±0</td><td>+50</td>
**!	<td><illustration>return Image(20,20,@(array)Color["#693e3e"])</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->light())</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->light()->light())</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->light()->light()->light())</illustration></td></tr>
**!
**!	<tr><td>dark  </td><td>lower light level</td><td>±0</td><td> ±0</td><td>-50</td>
**!	<td><illustration>return Image(20,20,@(array)Color["#693e3e"])</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->dark())</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->dark()->dark())</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->dark()->dark()->dark())</illustration></td></tr>
**!
**!	<tr><td>bright</td><td>brighter color   </td><td>±0</td><td>+50</td><td>+50</td>
**!	<td><illustration>return Image(20,20,@(array)Color["#693e3e"])</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->bright())</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->bright()->bright())</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->bright()->bright()->bright())</illustration></td></tr>
**!
**!	<tr><td>dull  </td><td>greyer color     </td><td>±0</td><td>-50</td><td>-50</td>
**!	<td><illustration>return Image(20,20,@(array)Color.red)</illustration>
**!	<illustration>return Image(20,20,@(array)Color.red->dull())</illustration>
**!	<illustration>return Image(20,20,@(array)Color.red->dull()->dull())</illustration>
**!	<illustration>return Image(20,20,@(array)Color.red->dull()->dull()->dull())</illustration></td></tr>
**!
**!	<tr><td>neon  </td><td>set to extreme   </td><td>±0</td><td>max</td><td>max</td>
**!	<td><illustration>return Image(20,20,@(array)Color["#693e3e"])</illustration>
**!	<illustration>return Image(20,20,@(array)Color["#693e3e"]->neon())</illustration></td></tr>
**!
**!     </table>
**! returns the new color object
**! note:
**!	The opposites may not always take each other out.
**!	The color is maximised at white and black levels,
**!	so, for instance 
**!	<ref>Image.Color</ref>.white-><ref>light</ref>()-><ref>dark</ref>()
**!	doesn't give the white color back, but the equal to
**!	<ref>Image.Color</ref>.white-><ref>dark</ref>(), since
**!	white can't get any <ref>light</ref>er.
*/


static void image_color_light(INT32 args)
{
   pop_n_elems(args);
   image_color_hsvf(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-1].u.float_number+=+0.2;
   image_make_hsv_color(3);
}

static void image_color_dark(INT32 args)
{
   pop_n_elems(args);
   image_color_hsvf(0);
   sp--;
   push_array_items(sp->u.array); /* frees */
   sp[-1].u.float_number-=0.2;
   image_make_hsv_color(3);
}

static void image_color_neon(INT32 args)
{
   pop_n_elems(args);
   image_color_hsvf(0);
   sp--;
   push_array_items(sp->u.array); /* frees */

   if (sp[-1].u.float_number==0.0 ||
       sp[-2].u.float_number==0.0)
   {
      if (sp[-2].u.float_number<0.5)
	 sp[-2].u.float_number=0.0;
      else
	 sp[-2].u.float_number=1.0;
   }
   else
   {
      sp[-1].u.float_number=1.0;
      sp[-2].u.float_number=1.0;
   }
   image_make_hsv_color(3);
}

static void image_color_dull(INT32 args)
{
   pop_n_elems(args);

   image_color_hsvf(0);
   sp--;
   push_array_items(sp->u.array); /* frees */

   if (sp[-1].u.float_number==0.0)
   {
      pop_n_elems(3);
      ref_push_object(THISOBJ);
      return;
   }

   sp[-2].u.float_number-=0.2;
   image_make_hsv_color(3);
}

static void image_color_bright(INT32 args)
{
   pop_n_elems(args);
   image_color_hsvf(0);
   sp--;
   push_array_items(sp->u.array); /* frees */

   if (sp[-1].u.float_number==0.0)
   {
      pop_n_elems(3);
      ref_push_object(THISOBJ);
      return;
   }

   sp[-2].u.float_number+=0.2;
   image_make_hsv_color(3);
}


static void image_color_mult(INT32 args)
{
   float x=0.0;
   get_all_args("Image.Color.Color->`*",args,"%f",&x);
   pop_n_elems(args);
   _image_make_rgb_color((int)(THIS->rgb.r*x),
			 (int)(THIS->rgb.g*x),
			 (int)(THIS->rgb.b*x));
}

int image_color_svalue(struct svalue *v,rgb_group *rgb)
{
   if (v->type==T_OBJECT)
   {
      struct color_struct *cs=(struct color_struct*)
	 get_storage(v->u.object,image_color_program);

      if (cs) 
      {
	 *rgb=cs->rgb;
	 return 1;
      }
   }
   else if (v->type==T_ARRAY)
   {
      if (v->u.array->size==3 &&
	  v->u.array->item[0].type==T_INT &&
	  v->u.array->item[1].type==T_INT &&
	  v->u.array->item[2].type==T_INT)
      {
	 rgb->r=(COLORTYPE)(v->u.array->item[0].u.integer);
	 rgb->g=(COLORTYPE)(v->u.array->item[1].u.integer);
	 rgb->b=(COLORTYPE)(v->u.array->item[2].u.integer);
	 return 1;
      }
   }
   else if (v->type==T_STRING)
   {
      push_svalue(v);
      image_make_color(1);
      if (sp[-1].type==T_OBJECT)
      {
	 struct color_struct *cs=(struct color_struct*)
	    get_storage(v->u.object,image_color_program);
	 *rgb=cs->rgb;
	 pop_stack();
	 return 1;
      }
      pop_stack();
   }
   return 0;
}

int image_color_arg(INT32 args,rgb_group *rgb)
{
   if (args>=0) return 0;
   return image_color_svalue(sp+args,rgb);
}


static void image_color_add(INT32 args)
{
   rgb_group rgb;

   if (!image_color_arg(-args,&rgb))
      SIMPLE_BAD_ARG_ERROR("Image.Color.Color->`+",1,"Color");

   pop_n_elems(args);
   _image_make_rgb_color((int)(THIS->rgb.r+rgb.r),
			 (int)(THIS->rgb.g+rgb.g),
			 (int)(THIS->rgb.b+rgb.b));
}



#define HEXTONUM(C) \
	(((C)>='0' && (C)<='9')?(C)-'0': \
	 ((C)>='a' && (C)<='f')?(C)-'a'+10: \
	 ((C)>='A' && (C)<='F')?(C)-'A'+10:-1)

static void image_get_color(INT32 args)
{
   struct svalue s;
   int n;
   static char *callables[]={"light","dark","neon","dull","bright"};

   if (args!=1) 
      error("Image.Color[]: illegal number of args");
   
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
	    /* #rgb, #rrggbb, #rrrgggbbb, etc */
	 
	    unsigned long i=sp[-1].u.string->len-1,j,k,rgb[3];
	    unsigned char *src=sp[-1].u.string->str+1;
	    if (!(i%3))
	    {
	       i/=3;
	       for (j=0; j<3; j++)
	       {
		  unsigned INT32 z=0;
		  for (k=0; k<i; k++)
		  {
		     if (HEXTONUM(*src)==-1)
		     {
			pop_stack();
			push_int(0);
			sp[-1].subtype=NUMBER_UNDEFINED;
			return;
		     }
		     z=z*16+HEXTONUM(*src),src++;
		  }
		  switch (i)
		  {
		     case 1: z=(z*0x11111111)>>(32-COLORLBITS); break;
		     case 2: z=(z*0x01010101)>>(32-COLORLBITS); break;
		     case 3: z=(z*0x00100100+(z>>8))>>(32-COLORLBITS); break;

		     case 4:
		     case 5: 
		     case 6: 
		     case 7: 
		     case 8:
			if (i*4<COLORLBITS)
			   z=(z<<(COLORLBITS-i*4))+(z>>(i*8-COLORLBITS));
			else
			   z=z>>(i*4-COLORLBITS);
			break;
		  }
		  rgb[j]=z;
	       }
	       pop_n_elems(args);
	       _image_make_rgbl_color((INT32)rgb[0],
				      (INT32)rgb[1],
				      (INT32)rgb[2]);
	       return;
	    }
	 }
	 if (sp[-1].u.string->len>=4 &&
	     sp[-1].u.string->str[0]=='@')
	 {
	    /* @h,s,v; h=0..359, s,v=0..100 */
	    stack_dup();
	    push_text("@%f,%f,%f\n");
	    f_sscanf(2);
	    if (sp[-1].type==T_ARRAY &&
		sp[-1].u.array->size==3)
	    {
	       float h,s,v;
	       stack_swap();
	       pop_stack();
	       sp--;
	       push_array_items(sp->u.array);
	       get_all_args("Image.Color()",3,"%f%f%f",&h,&s,&v);
	       pop_n_elems(3);
	       push_int((INT32)(h/360.0*256.0));
	       push_int((INT32)(s/100.0*255.4));
	       push_int((INT32)(v/100.0*255.4));
	       image_make_hsv_color(3);
	       return;
	    }
	    pop_stack();
	 }
	 if (sp[-1].u.string->len>=4 &&
	     sp[-1].u.string->str[0]=='%')
	 {
	    /* @c,m,y,k; 0..100 */
	    stack_dup();
	    push_text("%%%f,%f,%f,%f\n");
	    f_sscanf(2);
	    if (sp[-1].type==T_ARRAY &&
		sp[-1].u.array->size==4)
	    {
	       stack_swap();
	       pop_stack();
	       sp--;
	       push_array_items(sp->u.array);
	       image_make_cmyk_color(4);
	       return;
	    }
	    pop_stack();
	 }
	 for (n=0; (size_t)n<sizeof(callables)/sizeof(callables[0]); n++)
	    if (sp[-1].u.string->len>(INT32)strlen(callables[n]) &&
	     memcmp(sp[-1].u.string->str,callables[n],strlen(callables[n]))==0)
	 {
	    push_int(strlen(callables[n]));
	    push_int(1000000);
	    f_index(3);
	    image_get_color(1);
	    if (sp[-1].type!=T_OBJECT) return; /* no way */
	    safe_apply(sp[-1].u.object,callables[n],0);
	    stack_swap();
	    pop_stack();
	    return;
	 }
	 if (sp[-1].u.string->len>=4 &&
	     sp[-1].u.string->str[0]=='g')
	 {
	    /* greyx; x=0..99 */
	    stack_dup();
	    push_text("grey%f\n");
	    f_sscanf(2);
	    if (sp[-1].type==T_ARRAY &&
		sp[-1].u.array->size==1)
	    {
	       float f;
	       f=sp[-1].u.array->item[0].u.float_number;
	       pop_stack();
	       sp--;
	       
	       return;
	    }
	    pop_stack();
	 }
      }

      /* try other stuff here */

      pop_stack();
      push_int(0);
      sp[-1].subtype=NUMBER_UNDEFINED;
      return;
   }

   pop_stack();
   *(sp++)=s;
}

static void image_guess_color(INT32 args)
{
   struct svalue s;

   if (args!=1 && sp[-args].type!=T_STRING) 
      error("Image.Color->guess(): illegal argument(s)\n");
   
   f_lower_case(1);
   push_text(" ");
   o_subtract();

   stack_dup();
   image_get_color(1);
   if (sp[-1].type==T_OBJECT)
   {
      stack_swap();
      pop_stack();
      return;
   }
   pop_stack();
   push_text("#");
   stack_swap();
   f_add(2);

   image_get_color(1);
}

static void image_make_color(INT32 args)
{
   struct svalue s;

   if (args==1 && sp[-args].type==T_STRING) 
   {
      image_get_color(args);
      return;
   }
   image_make_rgb_color(args);
}


/* 
**! module Image
**! submodule Color
**!
**! method object guess(string)
**!	This is equivalent to
**!	<tt><ref>Image.Color</ref>(lower_case(str)-" ")</tt>,
**!	and tries the color with a prepending '#' if no 
**!	corresponding color is found.
**!
**! returns a color object or zero_type
*/

/*
**! method object rgb(int red, int green, int blue)
**! method object hsv(int hue, int saturation, int value)
**! method object cmyk(float c,float m,float y,float k)
**! method object greylevel(int level)
**! method object html(string html_color)
**!	Creates a new color object from given red, green and blue,
**!	hue, saturation and value, or greylevel, in color value range.
**!	It could also be created from <i>cmyk</i> values in percent.
**!
**!	The <ref>html</ref>() method only understands the HTML color names,
**!	or the <tt>#rrggbb</tt> form. It is case insensitive.
**!
**! returns the created object.
*/

static void _image_make_rgbl_color(INT32 r,INT32 g,INT32 b)
{
   struct color_struct *cs;

   if (r<0) r=0; else if (r>COLORLMAX) r=COLORLMAX; /* >=2^31? no way... */
   if (g<0) g=0; else if (g>COLORLMAX) g=COLORLMAX;
   if (b<0) b=0; else if (b>COLORLMAX) b=COLORLMAX;

   push_object(clone_object(image_color_program,0));

   cs=(struct color_struct*)
      get_storage(sp[-1].u.object,image_color_program);

   cs->rgbl.r=(INT32)r;
   cs->rgbl.g=(INT32)g;
   cs->rgbl.b=(INT32)b;
   RGBL_TO_RGB(cs->rgb,cs->rgbl);
}

static void _image_make_rgbf_color(float r,float g,float b)
{
#define FOO(X) FLOAT_TO_COLORL((X)<0.0?0.0:(X)>1.0?1.0:(X))
   _image_make_rgbl_color(FOO(r),FOO(g),FOO(b));
#undef FOO
}

void _image_make_rgb_color(INT32 r,INT32 g,INT32 b)
{
   struct color_struct *cs;

   if (r<0) r=0; else if (r>COLORMAX) r=COLORMAX;
   if (g<0) g=0; else if (g>COLORMAX) g=COLORMAX;
   if (b<0) b=0; else if (b>COLORMAX) b=COLORMAX;

   push_object(clone_object(image_color_program,0));

   cs=(struct color_struct*)
      get_storage(sp[-1].u.object,image_color_program);

   cs->rgb.r=(COLORTYPE)r;
   cs->rgb.g=(COLORTYPE)g;
   cs->rgb.b=(COLORTYPE)b;
   RGB_TO_RGBL(cs->rgbl,cs->rgb);
}

static void image_make_rgb_color(INT32 args)
{
   INT32 r=0,g=0,b=0;

   get_all_args("Image.Color.rgb()",args,"%i%i%i",&r,&g,&b);

   _image_make_rgb_color(r,g,b);
}

static void image_make_hsv_color(INT32 args)
{
   float h,s,v;
   float r=0,g=0,b=0; /* to avoid warning */

   if (args && sp[-args].type==T_INT)
   {
      INT32 hi,si,vi;
      get_all_args("Image.Color.hsv()",args,"%i%i%i",
		   &hi,&si,&vi);
      pop_n_elems(args);

      if (hi<0) hi=(hi%COLORMAX)+COLORMAX; 
      else if (hi>COLORMAX) hi%=COLORMAX; /* repeating */
      if (si<0) si=0; else if (si>COLORMAX) si=COLORMAX;
      if (vi<0) vi=0; else if (vi>COLORMAX) vi=COLORMAX;
   
      h = (hi/((float)COLORMAX))*(360.0/60.0);
      s = si/((float)COLORMAX);
      v = vi/((float)COLORMAX);
   }
   else
   {
      get_all_args("Image.Color.hsv()",args,"%f%f%f",
		   &h,&s,&v);
      pop_n_elems(args);
      if (h<0) h=360+h-(((int)h/360)*360);
      if (h>360.0) h-=(((int)h/360)*360);
      h/=60;
   }
     
   if(s==0.0)
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
	 default: error("internal error\n");
      }
   }
#undef i
#undef f
#undef p
#undef q
#undef t

   _image_make_rgbf_color(r,g,b);
}

static void image_make_cmyk_color(INT32 args)
{
   float c,m,y,k,r,g,b;
   get_all_args("Image.Color.cmyk()",args,"%F%F%F%F",&c,&m,&y,&k);
   pop_n_elems(args);

   r=100-(c+k);
   g=100-(m+k);
   b=100-(y+k);

   _image_make_rgbf_color(r*0.01,g*0.01,b*0.01);
}

static void image_make_greylevel_color(INT32 args)
{
   INT32 i;

   get_all_args("Image.Color.greylevel()",args,"%i",&i);
   pop_n_elems(args);

   _image_make_rgb_color(i,i,i);
}

static void image_make_html_color(INT32 args)
{
   int i;

   if (args!=1 ||
       sp[-1].type!=T_STRING) 
   {
      error("Image.Color.html(): illegal arguments\n");
      return;
   }
   
   f_lower_case(1);
   for (i=0; (size_t)i<sizeof(html_color)/sizeof(html_color[0]); i++)
      if (html_color[i].pname==sp[-1].u.string)
      {
	 _image_make_rgb_color(html_color[i].r,
			       html_color[i].g,
			       html_color[i].b);
	 return;
      }

   if (sp[-1].u.string->len>0 &&
       sp[-1].u.string->str[0]=='#')
      image_get_color(1);
   else
   {
      push_text("#");
      stack_swap();
      f_add(2);
      image_get_color(1);
   }
}

/*
**! method array(string) _indices()
**! method array(object) _values()
**!	(ie as <tt>indices(Image.Color)</tt> or <tt>values(Image.Color)</tt>)
**!	<tt>indices</tt> gives a list of all the known color names,
**!	<tt>values</tt> gives there corresponding objects.
**! see also: Image.Color
*/

static void image_colors_indices(INT32 args)
{
   pop_n_elems(args);
   if (!colors) make_colors();
   ref_push_mapping(colors);
   f_indices(1);
}

static void image_colors_values(INT32 args)
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

   no_name=make_shared_string("");

   start_new_program();

   ADD_STORAGE(struct color_struct);
   set_init_callback(init_color_struct);
   set_exit_callback(exit_color_struct);

   /* color info methods */

   ADD_FUNCTION("cast",image_color_cast,tFunc(tStr,tOr(tArray,tStr)),0);
   ADD_FUNCTION("`[]",image_color_index,tFunc(tOr(tStr,tInt),tOr(tInt,tFunction)),0);
   ADD_FUNCTION("`->",image_color_index,tFunc(tOr(tStr,tInt),tOr(tInt,tFunction)),0);
   ADD_FUNCTION("`==",image_color_equal,tFunc(tOr(tObj,tInt),tInt),0);
   ADD_FUNCTION("__hash",image_color___hash,tFunc(,tInt),0);

   ADD_FUNCTION("name",image_color_name,tFunc(,tStr),0);
   ADD_FUNCTION("hex",image_color_hex,tFunc(,tStr),0);
   ADD_FUNCTION("html",image_color_html,tFunc(,tStr),0);

   ADD_FUNCTION("rgb",image_color_rgb,tFunc(,tArr(tInt)),0);
   ADD_FUNCTION("rgbf",image_color_rgbf,tFunc(,tArr(tFlt)),0);
   ADD_FUNCTION("hsv",image_color_hsv,tFunc(,tArr(tInt)),0);
   ADD_FUNCTION("hsvf",image_color_hsvf,tFunc(,tArr(tFlt)),0);
   ADD_FUNCTION("cmyk",image_color_cmyk,tFunc(,tArr(tFlt)),0);
   ADD_FUNCTION("greylevel",image_color_greylevel,tOr(tFunc(,tInt),tFunc(tInt tInt tInt,tInt)),0);

   /* color conversion methods */

   ADD_FUNCTION("grey",image_color_grey,
		tOr(tFunc(,tObj),tFunc(tInt tInt tInt,tObj)),0);

   ADD_FUNCTION("light",image_color_light,tFunc(,tObj),0);
   ADD_FUNCTION("dark",image_color_dark,tFunc(,tObj),0);
   ADD_FUNCTION("neon",image_color_neon,tFunc(,tObj),0);
   ADD_FUNCTION("bright",image_color_bright,tFunc(,tObj),0);
   ADD_FUNCTION("dull",image_color_dull,tFunc(,tObj),0);

   ADD_FUNCTION("`*",image_color_mult,tFunc(tFlt,tObj),0);
   ADD_FUNCTION("`+",image_color_add,tFunc(tObj,tObj),0);

   image_color_program=end_program();
   
   start_new_program();
   ADD_FUNCTION("`[]",image_get_color,tFunc(tStr,tObj),0);
   ADD_FUNCTION("`()",image_make_color,tFuncV(,tOr(tStr,tInt),tObj),0);
   ADD_FUNCTION("rgb",image_make_rgb_color,tFunc(tInt tInt tInt,tObj),0);
   ADD_FUNCTION("hsv",image_make_hsv_color,
		tOr(tFunc(tInt tInt tInt,tObj),
		    tFunc(tFlt tFlt tFlt,tObj)) ,0);
   ADD_FUNCTION("cmyk",image_make_cmyk_color,tFunc(tOr(tInt,tFlt) 
						   tOr(tInt,tFlt) 
						   tOr(tInt,tFlt) 
						   tOr(tInt,tFlt),tObj), 0);
   ADD_FUNCTION("html",image_make_html_color,tFunc(tStr,tObj),0);
   ADD_FUNCTION("guess",image_guess_color,tFunc(tStr,tObj),0);
   ADD_FUNCTION("greylevel",image_make_greylevel_color,tFunc(tInt,tObj),0);
   ADD_FUNCTION("_indices",image_colors_indices,tFunc(,tArr(tStr)),0);
   ADD_FUNCTION("_values",image_colors_values,tFunc(,tArr(tObj)),0);

   add_program_constant("Color",image_color_program,0);

   prg=end_program();
   push_object(clone_object(prg,0));
   free_program(prg);
   str=make_shared_string("Color");
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
      int i;

      free_mapping(colors);
      free_object(colortable);
      free_array(colornames);

      colors=NULL;
      colortable=NULL;
      colornames=NULL;

      for (i=0; (size_t)i<sizeof(html_color)/sizeof(html_color[0]); i++)
	 free_string(html_color[i].pname);
   }
   free_string(str_array);
   free_string(str_string);
   free_string(str_r);
   free_string(str_g);
   free_string(str_b);
   free_string(str_h);
   free_string(str_s);
   free_string(str_v);

   free_string(no_name);
}
