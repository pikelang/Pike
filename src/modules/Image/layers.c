/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: layers.c,v 1.99 2006/03/08 01:22:01 peter Exp $
*/

/*
**! module Image
**! class Layer
**! see also: layers
*/

#include "global.h"

#include <math.h> /* floor */

#include "image_machine.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
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

#ifdef TRY_USE_MMX
#ifdef HAVE_MMX_H
#include <mmx.h>
#else
#include <asm/mmx.h>
#endif
#endif


extern struct program *image_program;
extern struct program *image_layer_program;
extern struct program *image_colortable_program;

static struct mapping *colors=NULL;
static struct object *colortable=NULL;
static struct array *colornames=NULL;

static const rgb_group black={0,0,0};
static const rgb_group white={COLORMAX,COLORMAX,COLORMAX};

typedef void lm_row_func(rgb_group *s,
			 rgb_group *l,
			 rgb_group *d,
			 rgb_group *sa,
			 rgb_group *la, /* may be NULL */
			 rgb_group *da,
			 int len,
			 double alpha);


#define SNUMPIXS 64 /* pixels in short-stroke buffer */

struct layer
{
   INT32 xsize;            /* underlaying image size */
   INT32 ysize;

   INT32 xoffs,yoffs;      /* clip offset */

   struct object *image; /* image object */
   struct object *alpha; /* alpha object or null */

   struct image *img;    /* image object storage */
   struct image *alp;    /* alpha object storage */

   FLOAT_TYPE alpha_value;    /* overall alpha value (1.0=opaque) */

   rgb_group fill;       /* fill color ("outside" the layer) */
   rgb_group fill_alpha; /* fill alpha */

   rgb_group sfill[SNUMPIXS];       /* pre-calculated rows */
   rgb_group sfill_alpha[SNUMPIXS];

   int tiled;            /* true if tiled */

   lm_row_func *row_func;/* layer mode */
   int optimize_alpha;
   int really_optimize_alpha;

   struct mapping     *misc; /* Misc associated data. Added by per,
                                rather useful for some things... */
};

#undef THIS
#define THIS ((struct layer *)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

#define LMFUNC(X) \
   static void X(rgb_group *s,rgb_group *l,rgb_group *d, \
                 rgb_group *sa,rgb_group *la,rgb_group *da, \
	         int len,double alpha)

LMFUNC(lm_normal);

LMFUNC(lm_add);
LMFUNC(lm_subtract);
LMFUNC(lm_multiply);
LMFUNC(lm_divide);
LMFUNC(lm_negdivide);
LMFUNC(lm_modulo);
LMFUNC(lm_invsubtract);
LMFUNC(lm_invdivide);
LMFUNC(lm_invmodulo);
LMFUNC(lm_imultiply);
LMFUNC(lm_idivide);
LMFUNC(lm_invidivide);
LMFUNC(lm_difference);
LMFUNC(lm_min);
LMFUNC(lm_max);
LMFUNC(lm_bitwise_and);
LMFUNC(lm_bitwise_or);
LMFUNC(lm_bitwise_xor);

LMFUNC(lm_replace);
LMFUNC(lm_red);
LMFUNC(lm_green);
LMFUNC(lm_blue);
LMFUNC(lm_hardlight);

LMFUNC(lm_replace_hsv);
LMFUNC(lm_hue);
LMFUNC(lm_saturation);
LMFUNC(lm_value);
LMFUNC(lm_value_mul);
LMFUNC(lm_color);
LMFUNC(lm_darken);
LMFUNC(lm_lighten);
LMFUNC(lm_saturate);
LMFUNC(lm_desaturate);

LMFUNC(lm_hls_replace);
LMFUNC(lm_hls_hue);
LMFUNC(lm_hls_saturation);
LMFUNC(lm_hls_lightness);
LMFUNC(lm_hls_lightness_mul);
LMFUNC(lm_hls_color);
LMFUNC(lm_hls_darken);
LMFUNC(lm_hls_lighten);
LMFUNC(lm_hls_saturate);
LMFUNC(lm_hls_desaturate);

LMFUNC(lm_dissolve);
LMFUNC(lm_behind);
LMFUNC(lm_erase);

LMFUNC(lm_screen);
LMFUNC(lm_overlay);

LMFUNC(lm_equal);
LMFUNC(lm_not_equal);
LMFUNC(lm_less);
LMFUNC(lm_more);
LMFUNC(lm_less_or_equal);
LMFUNC(lm_more_or_equal);

LMFUNC(lm_logic_equal);
LMFUNC(lm_logic_not_equal);
LMFUNC(lm_logic_strict_less);
LMFUNC(lm_logic_strict_more);
LMFUNC(lm_logic_strict_less_or_equal);
LMFUNC(lm_logic_strict_more_or_equal);

static void lm_spec_burn_alpha(struct layer *ly,
			       rgb_group *l, rgb_group *la,
			       rgb_group *s, rgb_group *sa,
			       rgb_group *d, rgb_group *da,
			       int len);

struct layer_mode_desc
{
   char *name;
   lm_row_func *func;
   int optimize_alpha; /* alpha 0 -> skip layer */
   struct pike_string *ps;
   char *desc;
} layer_mode[]=
{
   {"normal",        lm_normal,        1, NULL,
    "D=L applied with alpha: "
    "D=(L*aL+S*(1-aL)*aS) / (aL+(1-aL)*aS), "
    "aD=(aL+(1-aL)*aS)"},

   {"add",           lm_add,           1, NULL,
    "D=L+S applied with alpha, aD=aS"},
   {"subtract",      lm_subtract,      1, NULL,
    "D=S-L applied with alpha, aD=aS"},
   {"multiply",      lm_multiply,      1, NULL,
    "D=S*L applied with alpha, aD=aS"},
   {"divide",        lm_divide,        1, NULL,
    "D=S/L applied with alpha, aD=aS"},
   {"negdivide",     lm_negdivide,     1, NULL, 
    "D=1.0-S/L applied with alpha, aD=aS"},
   {"modulo",        lm_modulo,        1, NULL,
    "D=S%L applied with alpha, aD=aS"},

   {"invsubtract",   lm_invsubtract,   1, NULL,
    "D=L-S applied with alpha, aD=aS"},
   {"invdivide",     lm_invdivide,     1, NULL,
    "D=L/S applied with alpha, aD=aS"},
   {"invmodulo",     lm_invmodulo,     1, NULL,
    "D=L%S applied with alpha, aD=aS"},

   {"imultiply",     lm_imultiply,     1, NULL,
    "D=(1-L)*S applied with alpha, aD=aS"},
   {"idivide",       lm_idivide,       1, NULL,
    "D=S/(1-L) applied with alpha, aD=aS"},
   {"invidivide",    lm_invidivide,    1, NULL,
    "D=L/(1-S) applied with alpha, aD=aS"},

   {"difference",    lm_difference,    1, NULL,
    "D=abs(L-S) applied with alpha, aD=aS"},
   {"max",           lm_max,           1, NULL,
    "D=max(L,S) applied with alpha, aD=aS"},
   {"min",           lm_min,           1, NULL,
    "D=min(L,S) applied with alpha, aD=aS"},
   {"bitwise_and",   lm_bitwise_and,   1, NULL,
    "D=L&S applied with alpha, aD=aS"},
   {"bitwise_or",    lm_bitwise_or,    1, NULL,
    "D=L|S applied with alpha, aD=aS"},
   {"bitwise_xor",   lm_bitwise_xor,   1, NULL,
    "D=L^S applied with alpha, aD=aS"},

   {"replace",       lm_replace,       1, NULL,
    "D=(L*aL+S*(1-aL)*aS) / (aL+(1-aL)*aS), aD=aS"},
   {"red",           lm_red,           1, NULL,
    "Dr=(Lr*aLr+Sr*(1-aLr)*aSr) / (aLr+(1-aLr)*aSr), Dgb=Sgb, aD=aS"},
   {"green",         lm_green,         1, NULL,
    "Dg=(Lg*aLg+Sg*(1-aLg)*aSg) / (aLg+(1-aLg)*aSg), Drb=Srb, aD=aS"},
   {"blue",          lm_blue,          1, NULL,
    "Db=(Lb*aLb+Sb*(1-aLb)*aSb) / (aLb+(1-aLb)*aSb), Drg=Srg, aD=aS"},

   {"hardlight",          lm_hardlight,          1, NULL,
    "Like photoshop hardlight layer mode, aD=aS"},

   {"replace_hsv",   lm_replace_hsv,   1, NULL,
    "Dhsv=Lhsv apply with alpha, aD=aS"},
   {"hue",           lm_hue,           1, NULL,
    "Dh=Lh apply with alpha, Dsv=Lsv, aD=aS"},
   {"saturation",    lm_saturation,    1, NULL,
    "Ds=Ls apply with alpha, Dhv=Lhv, aD=aS"},
   {"value",         lm_value,         1, NULL,
    "Dv=Lv apply with alpha, Dhs=Lhs, aD=aS"},
   {"color",         lm_color,         1, NULL,
    "Dhs=Lhs apply with alpha, Dv=Lv, aD=aS"},
   {"value_mul",     lm_value_mul,     1, NULL,
    "Dv=Lv*Sv apply with alpha, Dhs=Lhs, aD=aS"},
   {"darken",        lm_darken,        1, NULL,
    "Dv=min(Lv,Sv) apply with alpha, Dhs=Lhs, aD=aS"},
   {"lighten",       lm_lighten,       1, NULL,
    "Dv=max(Lv,Sv) apply with alpha, Dhs=Lhs, aD=aS"},
   {"saturate",      lm_saturate,      1, NULL,
    "Ds=max(Ls,Ss) apply with alpha, Dhv=Lhv, aD=aS"},
   {"desaturate",    lm_desaturate,    1, NULL,
    "Ds=min(Ls,Ss) apply with alpha, Dhv=Lhv, aD=aS"},

   {"hls_replace",   lm_hls_replace,       1, NULL,
    "Dhls=Lhls apply with alpha, aD=aS"},
   {"hls_hue",       lm_hls_hue,           1, NULL,
    "Dh=Lh apply with alpha, Dsv=Lsv, aD=aS"},
   {"hls_saturation",lm_hls_saturation,    1, NULL,
    "Ds=Ls apply with alpha, Dhv=Lhv, aD=aS"},
   {"hls_lightness", lm_hls_lightness,     1, NULL,
    "Dl=Ll apply with alpha, Dhs=Lhs, aD=aS"},
   {"hls_color",     lm_hls_color,         1, NULL,
    "Dhs=Lhs apply with alpha, Dl=Ll, aD=aS"},
   {"hls_lightness_mul",lm_hls_lightness_mul,     1, NULL,
    "Dl=Ll*Sl apply with alpha, Dhs=Lhs, aD=aS"},
   {"hls_darken",    lm_hls_darken,        1, NULL,
    "Dl=min(Ll,Sl) apply with alpha, Dhs=Lhs, aD=aS"},
   {"hls_lighten",   lm_hls_lighten,       1, NULL,
    "Dl=max(Ll,Sl) apply with alpha, Dhs=Lhs, aD=aS"},
   {"hls_saturate",  lm_hls_saturate,      1, NULL,
    "Ds=max(Ls,Ss) apply with alpha, Dhl=Lhl, aD=aS"},
   {"hls_desaturate",lm_hls_desaturate,    1, NULL,
    "Ds=min(Ls,Ss) apply with alpha, Dhl=Lhl, aD=aS"},

   {"dissolve",      lm_dissolve,      1, NULL,
    "i=random 0 or 1, D=i?L:S, aD=i+aS"},
   {"behind",        lm_behind,        1, NULL,
    "D=(S*aS+L*(1-aS)*aL) / (aS+(1-aS)*aL), aD=(aS+(1-aS)*aL); "
    "simply swap S and L"},
   {"erase",         lm_erase,         1, NULL,
    "D=S, aD=aS*(1-aL)"},

   {"screen",        lm_screen,        1, NULL,
    "1-(1-S)*(1-L) applied with alpha, aD=aS"},
   {"overlay",       lm_overlay,       1, NULL,
    "(1-(1-a)*(1-b)-a*b)*a+a*b applied with alpha, aD=aS"},
   {"burn_alpha",    (lm_row_func*)lm_spec_burn_alpha, 1, NULL,
    "aD=aL+aS applied with alpha, D=L+S;" 
    " experimental, may change or be removed"},

   {"equal",         lm_equal,         0, NULL,
    "each channel D=max if L==S, 0 otherwise, apply with alpha"},
   {"not_equal",     lm_not_equal,     0, NULL,
    "each channel D=max if L!=S, 0 otherwise, apply with alpha"},
   {"less",          lm_less,          0, NULL,
    "each channel D=max if L<S, 0 otherwise, apply with alpha"},
   {"more",          lm_more,          0, NULL,
    "each channel D=max if L>S, 0 otherwise, apply with alpha"},
   {"less_or_equal", lm_less_or_equal, 0, NULL,
    "each channel D=max if L<=S, 0 otherwise, apply with alpha"},
   {"more_or_equal", lm_more_or_equal, 0, NULL,
    "each channel D=max if L>=S, 0 otherwise, apply with alpha"},

   {"logic_equal",   lm_logic_equal,   0, NULL,
    "logic: D=white and opaque if L==S, black and transparent otherwise"},
   {"logic_not_equal",lm_logic_not_equal,0, NULL,
    "logic: D=white and opaque if any L!=S, "
    "black and transparent otherwise"},
   {"logic_strict_less",lm_logic_strict_less,0, NULL,
    "logic: D=white and opaque if all L<S, black and transparent otherwise"},
   {"logic_strict_more",lm_logic_strict_more,0, NULL,
    "logic: D=white and opaque if all L>S, black and transparent otherwise"},
   {"logic_strict_less_equal",lm_logic_strict_less_or_equal,0, NULL,
    "logic: D=white and opaque if all L<=L, "
    "black and transparent otherwise"},
   {"logic_strict_more_equal",lm_logic_strict_more_or_equal,0, NULL,
    "logic: D=white and opaque if all L>=L, "
    "black and transparent otherwise"},
} ;

#define LAYER_MODES ((int)NELEM(layer_mode))




/*** layer helpers ****************************************/

#define COMBINE_METHOD_INT
#define CCUT_METHOD_INT

#define qMAX (1.0/COLORMAX)
#define C2F(Z) (qMAX*(Z))

#ifdef CCUT_METHOD_FLOAT
#define CCUT(Z) (DOUBLE_TO_COLORTYPE(qMAX*Z))
#else /* CCUT_METHOD_INT */
#define CCUT(Z) (DOUBLE_TO_COLORTYPE((Z)/COLORMAX))
#endif

#ifdef COMBINE_METHOD_INT


#define COMBINE_ALPHA_SUM(aS,aL) \
    CCUT((COLORMAX*DOUBLE_TO_INT(aL))+(COLORMAX-DOUBLE_TO_INT(aL))*(aS))
#define COMBINE_ALPHA_SUM_V(aS,aL,V) \
    COMBINE_ALPHA_SUM(aS,(aL)*(V))

#define COMBINE_ALPHA(S,L,aS,aL) \
    ( (COLORTYPE)((((S)*(DOUBLE_TO_INT(COLORMAX-(aL)))*(aS))+ \
	           ((L)*(DOUBLE_TO_INT(aL))*COLORMAX))/ \
    	          (((COLORMAX*DOUBLE_TO_INT(aL))+(COLORMAX-DOUBLE_TO_INT(aL))*(aS))) ) )

#define COMBINE_ALPHA_V(S,L,aS,aL,V) \
    COMBINE_ALPHA((S),(L),(aS),((aL)*(V)))

#else
#ifdef COMBINE_METHOD_FLOAT

#define COMBINE_ALPHA(S,L,aS,aL) \
    ( (COLORTYPE)( ( (S)*(1.0-C2F(aL))*C2F(aS) + (L)*C2F(aL) ) / \
	       ( (C2F(aL)+(1-C2F(aL))*C2F(aS))) ) )

#define COMBINE_ALPHA_V(S,L,aS,aL,V) \
    COMBINE_ALPHA(S,(L)*(V),aS,aL)

#define COMBINE_ALPHA_SUM(aS,aL) \
    ((COLORTYPE)(COLORMAX*(C2F(aL)+(1.0-C2F(aL))*C2F(aS))))
#define COMBINE_ALPHA_SUM_V(aS,aL,V) \
    COMBINE_ALPHA_SUM(aS,(aL)*(V))

#else /* unknown COMBINE_METHOD */
#error unknown COMBINE_METHOD
#endif /* COMBINE_METHOD_FLOAT  */

#endif

#define COMBINE(P,A) CCUT((DOUBLE_TO_INT(P))*(A))
#define COMBINE_A(P,A) (DOUBLE_TO_COLORTYPE((P)*(A)))
#define COMBINE_V(P,V,A) CCUT((V)*(P)*(A))

#define F2C(Z) (DOUBLE_TO_COLORTYPE(COLORMAX*(Z)))

#define ALPHA_ADD(S,L,D,SA,LA,DA,C)					 \
	    if (!(LA)->C) (D)->C=(S)->C,(DA)->C=(SA)->C;		 \
	    else if (!(SA)->C) (D)->C=(L)->C,(DA)->C=(LA)->C;		 \
	    else if ((LA)->C==COLORMAX) (D)->C=(L)->C,(DA)->C=(LA)->C; \
	    else							 \
	       (D)->C=COMBINE_ALPHA((S)->C,(L)->C,(SA)->C,(LA)->C),	 \
		  (DA)->C=COMBINE_ALPHA_SUM((SA)->C,(LA)->C);

#define ALPHA_ADD_V_NOLA(L,S,D,SA,DA,V,C)		  \
            do {							  \
               if (!(SA)->C) (D)->C=(L)->C,(DA)->C=0;			  \
               else							  \
               {							  \
                 if ((SA)->C==COLORMAX)					  \
		  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,COLORMAX,255,V); \
  	         else 							  \
                  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,(SA)->C,255,V);	  \
     	         (DA)->C=COMBINE_ALPHA_SUM_V((SA)->C,255,V);		  \
               }							  \
	    } while(0)

#define ALPHA_ADD_V(L,S,D,LA,SA,DA,V,C)			   \
            do {							   \
 	       if (!(LA)->C) (D)->C=(S)->C,(DA)->C=(SA)->C;		   \
	       else if (!(SA)->C)					   \
	       {							   \
		  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,0,(LA)->C,V);	   \
		  (DA)->C=COMBINE_ALPHA_SUM_V((LA)->C,0,V);		   \
	       }							   \
	       else							   \
	       {							   \
		  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,(SA)->C,(LA)->C,V); \
		  (DA)->C=COMBINE_ALPHA_SUM_V((LA)->C,(SA)->C,V);	   \
	       }							   \
	    } while (0)

#define ALPHA_ADD_nA(S,L,D,SA,LA,DA,C)					\
            do {							\
	       if (!(LA)->C) (D)->C=(S)->C;				\
	       else if (!(SA)->C) (D)->C=(L)->C;			\
	       else if ((LA)->C==COLORMAX) (D)->C=(L)->C;		\
	       else							\
		  (D)->C=COMBINE_ALPHA((S)->C,(L)->C,(SA)->C,(LA)->C);	\
	    } while(0)

#define ALPHA_ADD_V_NOLA_nA(L,S,D,SA,DA,V,C)				\
            do {							\
               if (!(SA)->C) (D)->C=(L)->C;				\
               else							\
               {							\
                 if ((SA)->C==COLORMAX)					\
		  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,COLORMAX,255,V);	\
  	         else							\
                  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,(SA)->C,255,V);	\
               }							\
	    } while(0)

#define ALPHA_ADD_V_nA(L,S,D,LA,SA,DA,V,C)				   \
            do {							   \
 	       if (!(LA)->C) (D)->C=(S)->C;				   \
	       else if (!(SA)->C)					   \
	       {							   \
		  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,0,(LA)->C,V);	   \
	       }							   \
	       else							   \
	       {							   \
		  (D)->C=COMBINE_ALPHA_V((S)->C,(L)->C,(SA)->C,(LA)->C,V); \
	       }							   \
	    } while (0)


static INLINE void smear_color(rgb_group *d,rgb_group s,int len)
{
   while (len--)
      *(d++)=s;
}

#define MAX3(X,Y,Z) MAXIMUM(MAXIMUM(X,Y),Z)
#define MIN3(X,Y,Z) MINIMUM(MINIMUM(X,Y),Z)

/* Pike_sp was renamed to not_sp by hubbe */
static INLINE void rgb_to_hsv(rgb_group color,
			      double *hp,
			      double *not_sp,
			      double *vp)
{
   double max,min,delta;
   double r,g,b;

   if (color.r==color.g && color.g==color.b)
   {
      *hp=*not_sp=0.0;
      *vp=C2F(color.r);
      return;
   }

   r=C2F(color.r);
   g=C2F(color.g);
   b=C2F(color.b);

   max = MAX3(r,g,b);
   min = MIN3(r,g,b);

   *vp = max;

   *not_sp = (max - min)/max;

   delta = max-min;

   if(r==max) *hp = 6+(g-b)/delta;
   else if(g==max) *hp = 2+(b-r)/delta;
   else /*if(b==max)*/ *hp = 4+(r-g)/delta;
}

static INLINE void hsv_to_rgb(double h,double s,double v,rgb_group *colorp)
{
   if (s==0.0)
   {
      colorp->r=colorp->g=colorp->b=F2C(v);
      return;
   }

#define i floor(h)
#define f (h-i)
#define p F2C(v * (1 - s))
#define q F2C(v * (1 - (s * f)))
#define t F2C(v * (1 - (s * (1 -f))))
#define V F2C(v)
   switch(DOUBLE_TO_INT(i))
   {
      case 6:
      case 0: 	colorp->r = V;	colorp->g = t;	colorp->b = p;	 break;
      case 7:
      case 1: 	colorp->r = q;	colorp->g = V;	colorp->b = p;	 break;
      case 2: 	colorp->r = p;	colorp->g = V;	colorp->b = t;	 break;
      case 3: 	colorp->r = p;	colorp->g = q;	colorp->b = V;	 break;
      case 4: 	colorp->r = t;	colorp->g = p;	colorp->b = V;	 break;
      case 5: 	colorp->r = V;	colorp->g = p;	colorp->b = q;	 break;
      default: Pike_fatal("unhandled case\n");
   }
#undef V
#undef i
#undef f
#undef p
#undef q
#undef t
}

static INLINE int hls_value(double n1, double n2, double hue)
{
   double value;

   if (hue > 255)
      hue -= 255;
   else if (hue < 0)
      hue += 255;
   if (hue < 42.5)
      value = n1 + (n2 - n1) * (hue / 42.5);
   else if (hue < 127.5)
      value = n2;
   else if (hue < 170)
      value = n1 + (n2 - n1) * ((170 - hue) / 42.5);
   else
      value = n1;

   return (int) (value * 255);
}


static INLINE void hls_to_rgb(double h,double l,double s,rgb_group *rgb)
{
   double m1, m2;

   if (s == 0)
   {
/*  achromatic case  */
      rgb->r = (COLORTYPE)l;
      rgb->g = (COLORTYPE)l;
      rgb->b = (COLORTYPE)l;
   }
   else
   {
      if (l < 128)
	 m2 = (l * (255 + s)) / 65025.0;
      else
	 m2 = (l + s - (l * s) / 255.0) / 255.0;

      m1 = (l / 127.5) - m2;

/*  chromatic case  */
      rgb->r = hls_value (m1, m2, h + 85);
      rgb->g = hls_value (m1, m2, h);
      rgb->b = hls_value (m1, m2, h - 85);
   }
}

static INLINE void rgb_to_hls(rgb_group color,
			      double *hue, 
			      double *lightness, 
			      double *saturation)
{
   int    r, g, b;
   double h, l, s;
   int    min, max;
   int    delta;

   r = color.r;
   g = color.g;
   b = color.b;

   if (r > g)
   {
      max = MAXIMUM(r, b);
      min = MINIMUM(g, b);
   }
   else
   {
      max = MAXIMUM(g, b);
      min = MINIMUM(r, b);
   }

   l = (max + min) / 2.0;

   if (max == min)
   {
      s = 0.0;
      h = 0.0;
   }
   else
   {
      delta = (max - min);

      if (l < 128)
	 s = 255 * (double) delta / (double) (max + min);
      else
	 s = 255 * (double) delta / (double) (511 - max - min);

      if (r == max)
	 h = (g - b) / (double) delta;
      else if (g == max)
	 h = 2 + (b - r) / (double) delta;
      else
	 h = 4 + (r - g) / (double) delta;

      h = h * 42.5;

      if (h < 0)
	 h += 255;
      else if (h > 255)
	 h -= 255;
   }

   *hue        = h;
   *lightness  = l;
   *saturation = s;
}       
             

/*** helper ***********************************************/

static int really_optimize_p(struct layer *l)
{
   return
      l->optimize_alpha &&
      l->fill_alpha.r==0 &&
      l->fill_alpha.g==0 &&
      l->fill_alpha.b==0 &&
      !l->tiled;
}



/*** layer object : init and exit *************************/

static void init_layer(struct object *dummy)
{
   THIS->xsize=0;
   THIS->ysize=0;
   THIS->xoffs=0;
   THIS->yoffs=0;
   THIS->image=NULL;
   THIS->alpha=NULL;
   THIS->img=NULL;
   THIS->alp=NULL;
   THIS->fill=black;
   THIS->fill_alpha=black;
   THIS->tiled=0;
   THIS->alpha_value=1.0;
   THIS->row_func=lm_normal;
   THIS->optimize_alpha=1;
   THIS->really_optimize_alpha=1;
   THIS->misc=0;

   smear_color(THIS->sfill,THIS->fill,SNUMPIXS);
   smear_color(THIS->sfill_alpha,THIS->fill_alpha,SNUMPIXS);
}

static void free_layer(struct layer *l)
{
   if (THIS->image) free_object(THIS->image);
   if (THIS->alpha) free_object(THIS->alpha);
   if (THIS->misc)  free_mapping(THIS->misc);
   THIS->image=NULL;
   THIS->alpha=NULL;
   THIS->img=NULL;
   THIS->alp=NULL;
}

static void exit_layer(struct object *dummy)
{
   free_layer(THIS);
}

/*
**! method object set_image(object(Image.Image)	image)
**! method object set_image(object(Image.Image)	image,object(Image.Image) alpha_channel)
**! method object|int(0) image()
**! method object|int(0) alpha()
**!	Set/change/get image and alpha channel for the layer.
**!	You could also cancel the channels giving 0
**!	instead of an image object.
**! note:
**!	image and alpha channel must be of the same size,
**!	or canceled.
*/

static void image_layer_set_image(INT32 args)
{
   struct image *img;

   if (THIS->image)
      free_object(THIS->image);
   THIS->image=NULL;
   THIS->img=NULL;

   if (THIS->alpha)
      free_object(THIS->alpha);
   THIS->alpha=NULL;
   THIS->alp=NULL;

   if (args>=1) {
      if ( Pike_sp[-args].type!=T_OBJECT )
      {
	 if (Pike_sp[-args].type!=T_INT ||
	     Pike_sp[-args].u.integer!=0)
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",1,
				 "object(Image)|int(0)");
      }
      else if ((img=(struct image*)
		get_storage(Pike_sp[-args].u.object,image_program)))
      {
	 THIS->image=Pike_sp[-args].u.object;
	 add_ref(THIS->image);
	 THIS->img=img;
	 THIS->xsize=img->xsize;
	 THIS->ysize=img->ysize;
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",1,
			      "object(Image)|int(0)");
   }

   if (args>=2) {
      if ( Pike_sp[1-args].type!=T_OBJECT )
      {
	 if (Pike_sp[1-args].type!=T_INT ||
	     Pike_sp[1-args].u.integer!=0)
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",2,
				 "object(Image)|int(0)");
      }
      else if ((img=(struct image*)
		get_storage(Pike_sp[1-args].u.object,image_program)))
      {
	 if (THIS->img &&
	     (img->xsize!=THIS->xsize ||
	      img->ysize!=THIS->ysize))
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",2,
				 "image of same size");
	 if (!THIS->img)
	 {
	    THIS->xsize=img->xsize;
	    THIS->ysize=img->ysize;
	 }
	 THIS->alpha=Pike_sp[1-args].u.object;
	 add_ref(THIS->alpha);
	 THIS->alp=img;
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Image.Layer->set_image",2,
			      "object(Image)|int(0)");
   }

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_get_misc_value( INT32 args )
{
  if( args != 1 )
    Pike_error("Wrong number of arguments to get_misc_value\n");
  if( THIS->misc )
  {
    ref_push_mapping( THIS->misc );
    stack_swap();
    f_index( 2 );
  }  else {
    pop_n_elems( args );
    push_int( 0 );
  }
}

/*
**! method mixed set_misc_value( mixed what, mixed to )
**! method mixed get_misc_value( mixed what )
**!   Set or query misc. attributes for the layer.
**!
**!   As an example, the XCF and PSD image decoders set the 'name' attribute
**!   to the name the layer had in the source file.
*/
static void image_layer_set_misc_value( INT32 args )
{
  if( args != 2 )
    Pike_error("Wrong number of arguments to set_misc_value\n");
  if( !THIS->misc )
    THIS->misc = allocate_mapping( 4 );

  mapping_insert( THIS->misc, Pike_sp-2, Pike_sp-1 );
  stack_swap();
  pop_stack();
}

static void image_layer_image(INT32 args)
{
   pop_n_elems(args);
   if (THIS->image)
      ref_push_object(THIS->image);
   else
      push_int(0);
}


static void image_layer_alpha(INT32 args)
{
   pop_n_elems(args);
   if (THIS->alpha)
      ref_push_object(THIS->alpha);
   else
      push_int(0);
}

/*
**! method object set_alpha_value(float value)
**! method float alpha_value()
**!	Set/get the general alpha value of this layer.
**!	This is a float value between 0 and 1,
**!	and is multiplied with the alpha channel.
*/

static void image_layer_set_alpha_value(INT32 args)
{
   FLOAT_TYPE f;
   get_all_args("Image.Layer->set_alpha_value",args,"%F",&f);
   if (f<0.0 || f>1.0)
      SIMPLE_BAD_ARG_ERROR("Image.Layer->set_alpha_value",1,"float(0..1)");
   THIS->alpha_value=f;
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_alpha_value(INT32 args)
{
   pop_n_elems(args);
   push_float(THIS->alpha_value);
}

/*
**! method object set_mode(string mode)
**! method string mode()
**! method array(string) available_modes()
**!	Set/get layer mode. Mode is one of these:
**!
**! <dl compact>
**! <dt><i>The variables in the expression:</i></dt>
**! <dt>L</dt><dd><i>The active layer</i></dd>
**! <dt>S</dt><dd><i>The source layer (the sum of the layers below)</i></dd>
**! <dt>D</dt><dd><i>The destintion layer (the result)</i></dd>
**! <dt>Xrgb</dt><dd><i>Layer red (<b>Xr</b>), green (<b>Xg</b>) or blue channel (<b>Xb</b>) </i></dd>
**! <dt>Xhsv</dt><dd><i>Layer hue (<b>Xh</b>), saturation (<b>Xs</b>) or value channel (<b>Xv</b>) (virtual channels)</i></dd>
**! <dt>Xhls</dt><dd><i>Layer hue (<b>Xh</b>), lightness channel (<b>Xl</b>) or saturation (<b>Xs</b>) (virtual channels)</i></dd>
**! <dt>aX</dt><dd><i>Layer alpha, channel in layer alpha</i></dd>
**! </dl>
**! <i>All channels are calculated separately, if nothing else is specified.</i>
**! <execute>
**! import Image;
**!
**! void write_image(string desc,
**! 		     string filename,
**! 		     Image img,
**!                  string longdesc)
**! {
**!    longdesc = replace(longdesc, ([ "&lt;":"&amp;lt;", "&gt;":"&amp;gt;", "&amp;":"&amp;amp;" ]));
**!    write(begin_tag("tr"));
**!    write(mktag("td",(["align":"left","colspan":"2"]),
**!          mktag("b",0,desc)));
**!    write(end_tag());
**!    write(begin_tag("tr"));
**!    write(mktag("td",(["align":"right"]),illustration_jpeg(img,(["dpi":150.0,"quality":90]))));
**!    write(mktag("td",(["align":"left","valign":"center"]),longdesc));
**           (replace(longdesc,({",",";",")"}),
**                    ({",<wbr>",";<wbr>",")<wbr>"}))/
**            "<wbr>")/1*({mktag("wbr")}) ) );
**!    write(end_tag()+"\n");
**! }
**!
**! int main()
**! {
**!    object ltrans=Layer((["image":
**! 			     Image(32,32,160,160,160)->
**! 			     box(0,0,15,15,96,96,96)->
**! 			     box(16,16,31,31,96,96,96)->scale(0.5),
**! 			     "tiled":1,
**! 			     "mode":"normal"]));
**!
**!    object circle=load(fix_image_path("circle50.pnm"));
**!    object image_test=load(fix_image_path("image_ill.pnm"));
**!
**!    object lc1=
**! 	     Layer((["image":circle->clear(255,0,0),
**! 		     "alpha":circle,
**! 		     "xoffset":5,
**! 		     "yoffset":5]));
**!
**!    object lc2=
**! 	  Layer((["image":circle->clear(0,0,255),
**! 		  "alpha":circle,
**! 		  "xoffset":25,
**! 		  "yoffset":25]));
**!    object lc2b=
**! 	  Layer((["image":circle,
**! 		  "alpha":circle*({0,0,255}),
**! 		  "xoffset":25,
**! 		  "yoffset":25]));
**!
**!    object li1=
**! 	  Layer((["image":image_test->scale(80,0),
**! 		  "mode":"normal",
**! 		  "xoffset":0,
**! 		  "yoffset":0]));
**!
**!    object li2=
**! 	  lay(
**! 	     ({
**! 	     (["image":circle->clear(255,0,0),"alpha":circle,
**! 	       "xoffset":5,"yoffset":0,"mode":"add"]),
**! 	     (["image":circle->clear(0,255,0),"alpha":circle,
**! 	       "xoffset":25,"yoffset":0,"mode":"add"]),
**! 	     (["image":circle->clear(0,0,255),"alpha":circle,
**! 	       "xoffset":15,"yoffset":20,"mode":"add"]),
**! 	     (["image":circle->clear(0,0,0)->scale(0.5),
**! 	       "alpha":circle->scale(0.5),
**! 	       "xoffset":0,"yoffset":55]),
**! 	     (["image":circle->clear(255,255,255)->scale(0.5),
**! 	       "alpha":circle->scale(0.5),
**! 	       "xoffset":55,"yoffset":55])
**! 	     }));
**!
**!    Layer li2b=Layer(li2->image()->clear(255,255,255),li2->image());
**!
**!    object lzo0=
**! 	  lay( ({
**! 	     Layer(Image(4,10)
**! 		   ->tuned_box(0,0,3,8,({({255,255,255}),({255,255,255}),
**! 					 ({0,0,0}),({0,0,0})}))
**! 		   ->scale(40,80))
**! 	     ->set_offset(0,0),
**! 	     Layer(Image(40,80)
**! 		   ->tuned_box(0,0,40,78,({({255,255,255}),({255,255,255}),
**! 					  ({0,0,0}),({0,0,0})})))
**! 	     ->set_offset(40,0),
**! 	     Layer(Image(80,80)
**! 		   ->tuned_box(0,0,80,80,({({255,0,0}),({255,255,0}),
**! 					  ({0,0,255}),({0,255,0})})))
**! 	     ->set_offset(80,0),
**! 	  }) );
**!
**!    object scale=
**! 	  Image(4,80)
**! 	  ->tuned_box(0,0,40,78,({({255,255,255}),({255,255,255}),
**! 				  ({0,0,0}),({0,0,0})}));
**!    object lzo1=
**! 	  lay( ({
**! 	     Layer(scale)->set_offset(2,0),
**! 	     Layer(scale->invert())->set_offset(6,0),
**! 	     Layer(Image(26,80)->test())->set_offset(12,0),
**! 	     Layer(scale)->set_offset(42,0),
**! 	     Layer(scale->invert())->set_offset(46,0),
**! 	     Layer(Image(26,80)->test())->set_offset(52,0),
**! 	     Layer(scale)->set_offset(82,0),
**! 	     Layer(scale->invert())->set_offset(86,0),
**! 	     Layer(Image(26,80)->test())->set_offset(92,0),
**! 	     Layer(scale)->set_offset(122,0),
**! 	     Layer(scale->invert())->set_offset(126,0),
**! 	     Layer(Image(26,80,"white"),
**! 		   Image(26,80)->test())->set_offset(132,0),
**! 	  }));
**!
**!    object lca1;
**!
**!    object a=
**! 	  lay( ({ lca1=lay(({Layer((["fill":"white"])),
**!                          lc1}),0,0,80,80),
**! 		  lay(({lc1}),0,0,80,80)->set_offset(80,0),
**! 		  lay(({li1}),0,0,80,80)->set_offset(160,0),
**! 		  lay(({li1}),0,0,80,80)->set_offset(240,0),
**! 		  lzo0->set_offset(320,0)}),
**! 	       0,0,480,80);
**!
**!    object b=
**! 	  lay( ({ lay(({lc2}),0,0,80,80),
**! 		  lay(({lc2b}),0,0,80,80)->set_offset(80,0),
**! 		  lay(({li2}),0,0,80,80)->set_offset(160,0),
**! 		  lay(({li2b}),0,0,80,80)->set_offset(240,0),
**! 		  lzo1->set_offset(320,0)}),
**! 	       0,0,480,80);
**!
**    xv(a); xv(b);
**!
**!    write(begin_tag("table",(["cellspacing":"0","cellpadding":"1"])));
**!
**!    write_image("top layer","b",lay(({ltrans,b}))->image(),
**!		   "");
**!    write_image("bottom layer","a",lay(({ltrans,a}))->image(),
**!                "");
**!
**!    foreach (Array.transpose(({Layer()->available_modes(),
**!                               Layer()->descriptions()})),
**!             [string mode,string desc])
**!    {
**!	  if ((&lt;"add","equal","replace","replace_hsv","darken",
**!	        "dissolve","screen","logic_equal">)[mode])
**!          write(mktag("tr",0,mktag("td",0,"\240")));
**!
**! 	  ({lc2,lc2b,li2,li2b,lzo1})->set_mode(mode);
**!
**! 	  object r=
**! 	     lay( ({ lay(({lca1,lc2}),0,0,80,80),
**! 		     lay(({lc1,lc2b}),0,0,80,80)->set_offset(80,0),
**! 		     lay(({li1,li2}),0,0,80,80)->set_offset(160,0),
**! 		     lay(({li1,li2b}),0,0,80,80)->set_offset(240,0),
**! 		     lay(({lzo0,lzo1}),320,0,160,80) }),
**! 		  0,0,480,80);
**       xv(r);
**!
**! 	  write_image(mode,mode,lay(({ltrans,r}))->image(),desc);
**!    }
**!
**!    write(end_tag());
**!    return 0;
**! }
**!
**! </execute>
**!
**!	<ref>available_modes</ref>() simply gives an array
**!	containing the names of these modes.
**!
**! note:
**!	image and alpha channel must be of the same size,
**!	or canceled.
*/

static void image_layer_set_mode(INT32 args)
{
   int i;
   if (args!=1)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Layer->set_mode",1);
   if (Pike_sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("Image.Layer->set_mode",1,"string");

   for (i=0; i<LAYER_MODES; i++)
      if (Pike_sp[-args].u.string==layer_mode[i].ps)
      {
	 THIS->row_func=layer_mode[i].func;
	 THIS->optimize_alpha=layer_mode[i].optimize_alpha;
	 THIS->really_optimize_alpha=really_optimize_p(THIS);

	 pop_n_elems(args);
	 ref_push_object(THISOBJ);
	 return;
      }

   SIMPLE_BAD_ARG_ERROR("Image.Layer->set_mode",1,"existing mode");
}

static void image_layer_mode(INT32 args)
{
   int i;
   pop_n_elems(args);

   for (i=0; i<LAYER_MODES; i++)
      if (THIS->row_func==layer_mode[i].func)
      {
	 ref_push_string(layer_mode[i].ps);
	 return;
      }

   Pike_fatal("illegal mode: %p\n", (void *)layer_mode[i-1].func);
}

static void image_layer_available_modes(INT32 args)
{
   int i;
   pop_n_elems(args);

   for (i=0; i<LAYER_MODES; i++)
      ref_push_string(layer_mode[i].ps);

   f_aggregate(LAYER_MODES);
}

/*
**! method array(string) description()
**!     Layer descriptions
*/
static void image_layer_descriptions(INT32 args)
{
   int i;
   pop_n_elems(args);

   for (i=0; i<LAYER_MODES; i++)
     push_text(layer_mode[i].desc);

   f_aggregate(LAYER_MODES);
}

/*
**! method object set_fill(Color color)
**! method object set_fill(Color color,Color alpha)
**! method object fill()
**! method object fill_alpha()
**!	Set/query fill color and alpha, ie the color used "outside" the
**!	image. This is mostly useful if you want to "frame"
**!	a layer.
*/

static void image_layer_set_fill(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Layer->set_fill",1);

   if (Pike_sp[-args].type==T_INT && !Pike_sp[-args].u.integer)
      THIS->fill=black;
   else
      if (!image_color_arg(-args,&(THIS->fill)))
	 SIMPLE_BAD_ARG_ERROR("Image.Layer->set_fill",1,"color");

   smear_color(THIS->sfill,THIS->fill,SNUMPIXS);

   THIS->fill_alpha=white;
   if (args>1) {
      if (Pike_sp[1-args].type==T_INT && !Pike_sp[1-args].u.integer)
	 ; /* white is good */
      else
	 if (!image_color_arg(1-args,&(THIS->fill_alpha)))
	 {
	    smear_color(THIS->sfill_alpha,THIS->fill_alpha,SNUMPIXS);
	    SIMPLE_BAD_ARG_ERROR("Image.Layer->set_fill",2,"color");
	 }
   }
   smear_color(THIS->sfill_alpha,THIS->fill_alpha,SNUMPIXS);

#if 0
   {
      int i;
      for (i=0; i<SNUMPIXS; i++)
	 fprintf(stderr,"#%02x%02x%02x ",THIS->sfill_alpha[i].r,THIS->sfill_alpha[i].g,THIS->sfill_alpha[i].b);
      fprintf(stderr,"\n");
      for (i=0; i<SNUMPIXS; i++)
	 fprintf("stderr,#%02x%02x%02x ",THIS->sfill[i].r,THIS->sfill[i].g,THIS->sfill[i].b);
      fprintf(stderr,"\n");
   }
#endif

   THIS->really_optimize_alpha=really_optimize_p(THIS);

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_fill(INT32 args)
{
   pop_n_elems(args);
   _image_make_rgb_color(THIS->fill.r,THIS->fill.g,THIS->fill.b);
}

static void image_layer_fill_alpha(INT32 args)
{
   pop_n_elems(args);
   _image_make_rgb_color(THIS->fill_alpha.r,
			 THIS->fill_alpha.g,
			 THIS->fill_alpha.b);
}

/*
**! method object set_offset(int x,int y)
**! method int xoffset()
**! method int yoffset()
**!	Set/query layer offset.
**!
**! method int xsize()
**! method int ysize()
**!	Query layer offset. This is the same as layer
**!	image/alpha image size.
*/

static void image_layer_set_offset(INT32 args)
{
   get_all_args("Image.Layer->set_offset",args,"%d%d", /* INT32! */
		&(THIS->xoffs),&(THIS->yoffs));
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_xoffset(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->xoffs);
}

static void image_layer_yoffset(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->yoffs);
}

static void image_layer_xsize(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->xsize);
}

static void image_layer_ysize(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->ysize);
}

/*
**! method object set_tiled(int yes)
**! method int tiled()
**!	Set/query <i>tiled</i> flag. If set, the
**!	image and alpha channel will be tiled rather
**!	then framed by the <ref>fill</ref> values.
*/

static void image_layer_set_tiled(INT32 args)
{
   INT_TYPE tiled;
   get_all_args("Image.Layer->set_offset",args,"%i",&tiled);
   THIS->tiled=!!tiled;
   THIS->really_optimize_alpha=really_optimize_p(THIS);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void image_layer_tiled(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->tiled);
}

/*
**! method void create(object image,object alpha,string mode)
**! method void create(mapping info)
**! method void create()
**
**! method void create(int xsize,int ysize,object color)
**! method void create(object color)
**!	The Layer construct either three arguments,
**!	the image object, alpha channel and mode, or
**!	a mapping with optional elements:
**!	<pre>
**!	"image":image,
**!		// default: black
**!
**!	"alpha":alpha,
**!		// alpha channel object
**!		// default: full opaque
**!
**!	"mode":string mode,
**!		// layer mode, see <ref>mode</ref>.
**!		// default: "normal"
**!
**!	"alpha_value":float(0.0-1.0),
**!		// layer general alpha value
**!		// default is 1.0; this is multiplied
**!		// with the alpha channel.
**!
**!	"xoffset":int,
**!	"yoffset":int,
**!		// offset of this layer
**!
**!	"fill":Color,
**!	"fill_alpha":Color,
**!		// fill color, ie what color is used
**!		// "outside" the image. default: black
**!		// and black (full transparency).
**!
**!	"tiled":int(0|1),
**!		// select tiling; if 1, the image
**!		// will be tiled. deafult: 0, off
**!	</pre>
**!	The layer can also be created "empty",
**!	either giving a size and color -
**!	this will give a filled opaque square,
**!	or a color, which will set the "fill"
**!	values and fill the whole layer with an
**!	opaque color.
**!
**!	All values can be modified after object creation.
**!
**! note:
**!	image and alpha channel must be of the same size.
**!
*/

static INLINE void try_parameter(char *a,void (*f)(INT32))
{
   stack_dup();
   push_text(a);
   f_index(2);

   if (!IS_UNDEFINED(Pike_sp-1))
      f(1);
   pop_stack();
}

static INLINE void try_parameter_pair(char *a,char *b,void (*f)(INT32))
{
   stack_dup();  /* map map */
   stack_dup();  /* map map map */
   push_text(a); /* map map map a */
   f_index(2);   /* map map map[a] */
   stack_swap(); /* map map[a] map */
   push_text(b); /* map map[a] map b */
   f_index(2);   /* map map[a] map[b] */

   if (!IS_UNDEFINED(Pike_sp-2) ||
       !IS_UNDEFINED(Pike_sp-1))
   {
      f(2);
      pop_stack();
   }
   else
      pop_n_elems(2);
}

static void image_layer_create(INT32 args)
{
   if (!args)
      return;
   if (Pike_sp[-args].type==T_MAPPING)
   {
      pop_n_elems(args-1);
      try_parameter_pair("image","alpha",image_layer_set_image);

      try_parameter("mode",image_layer_set_mode);
      try_parameter("alpha_value",image_layer_set_alpha_value);

      try_parameter_pair("xoffset","yoffset",image_layer_set_offset);
      try_parameter_pair("fill","fill_alpha",image_layer_set_fill);
      try_parameter("tiled",image_layer_set_tiled);
      pop_stack();
      return;
   }
   else if (Pike_sp[-args].type==T_INT && args>1
	    && Pike_sp[1-args].type==T_INT)
   {
      rgb_group col=black,alpha=white;

      get_all_args("Image.Layer",args,"%d%d", /* watch the type: INT32 */
		   &(THIS->xsize),&(THIS->ysize));
      if (args>2)
	 if (!image_color_arg(2-args,&col))
	    SIMPLE_BAD_ARG_ERROR("Image.Layer",3,"Image.Color");

      if (args>3)
	 if (!image_color_arg(3-args,&alpha))
	    SIMPLE_BAD_ARG_ERROR("Image.Layer",4,"Image.Color");

      push_int(THIS->xsize);
      push_int(THIS->ysize);
      push_int(col.r);
      push_int(col.g);
      push_int(col.b);
      push_object(clone_object(image_program,5));

      push_int(THIS->xsize);
      push_int(THIS->ysize);
      push_int(alpha.r);
      push_int(alpha.g);
      push_int(alpha.b);
      push_object(clone_object(image_program,5));

      image_layer_set_image(2);

      pop_n_elems(args);
   }
   else if (Pike_sp[-args].type==T_OBJECT || args>1)
   {
      if (args>2)
      {
	 image_layer_set_mode(args-2);
	 pop_stack();
	 args=2;
      }
      image_layer_set_image(args);
      pop_stack();
   }
   else
      SIMPLE_BAD_ARG_ERROR("Image.Layer",1,"mapping|int|Image.Image");
}

/*** layer object *****************************************/

/*
**! method mapping(string:mixed)|string cast()
**! ([ "xsize":int,
**!    "ysize":int,
**!    "image":image,
**!    "alpha":image,
**!    "xoffset":int,
**!    "yoffset":int,
**!    "fill":image,
**!    "fill_alpha":image
**!    "tiled":int,
**!    "mode":string
**! ])
 */
static void image_layer_cast(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.Layer->cast",1);
   if (Pike_sp[-args].type==T_STRING||Pike_sp[-args].u.string->size_shift)
   {
      if (strncmp(Pike_sp[-args].u.string->str,"mapping",7)==0)
      {
	 int n=0;
	 pop_n_elems(args);

	 push_constant_text("xsize");       push_int(THIS->xsize);         n++;
	 push_constant_text("ysize");       push_int(THIS->ysize);         n++;
	 push_constant_text("image");       image_layer_image(0);          n++;
	 push_constant_text("alpha");       image_layer_alpha(0);          n++;
	 push_constant_text("xoffset");     push_int(THIS->xoffs);         n++;
	 push_constant_text("yoffset");     push_int(THIS->yoffs);         n++;
	 push_constant_text("alpha_value"); push_float(THIS->alpha_value); n++;
	 push_constant_text("fill");        image_layer_fill(0);           n++;
	 push_constant_text("fill_alpha");  image_layer_fill_alpha(0);     n++;
	 push_constant_text("tiled");       push_int(THIS->tiled);         n++;
	 push_constant_text("mode");        image_layer_mode(0);           n++;

	 f_aggregate_mapping(n*2);

	 return;
      }
      else if (strncmp(Pike_sp[-args].u.string->str,"string",6)==0)
      {
	size_t size = THIS->xsize*THIS->ysize, i;
	struct pike_string *s = begin_shared_string(size*4);
	rgb_group *img = 0;
	rgb_group *alp = 0;

	pop_n_elems(args);
	if(THIS->img)
	  img = THIS->img->img;
	if(THIS->alp)
	  alp = THIS->alp->img;

	if(img && alp)
	  for(i=0; i<size; i++) {
	    s->str[i*4+0] = img[i].r;
	    s->str[i*4+1] = img[i].g;
	    s->str[i*4+2] = img[i].b;
	    s->str[i*4+3] = alp[i].r;
	  }
	else if(img)
	  for(i=0; i<size; i++) {
	    s->str[i*4+0] = img[i].r;
	    s->str[i*4+1] = img[i].g;
	    s->str[i*4+2] = img[i].b;
	    s->str[i*4+3] = 255;
	  }
	else if(alp)
	  for(i=0; i<size; i++) {
	    s->str[i*4+0] = 255;
	    s->str[i*4+1] = 255;
	    s->str[i*4+2] = 255;
	    s->str[i*4+3] = alp[i].r;
	  }
	else
	  memset(s->str, 0, size*4);

	push_string(end_shared_string(s));
	return;
      }
   }
   SIMPLE_BAD_ARG_ERROR("Image.Colortable->cast",1,
			"string(\"mapping\"|\"string\")");

}

/*** layer mode definitions ***************************/

static void lm_normal(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,double alpha)
{
   /* la may be NULL, no other */

   if (alpha==0.0) /* optimized */
   {
#ifdef LAYERS_DUAL
      MEMCPY(d,s,sizeof(rgb_group)*len);
      MEMCPY(da,sa,sizeof(rgb_group)*len);
#endif
      return;
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
      {
	 MEMCPY(d,l,sizeof(rgb_group)*len);
	 smear_color(da,white,len);
      }
      else
      {
	 while (len--)
	 {
	    if (la->r==0 && la->g==0 && la->b==0)
	    {
	       *d=*s;
	       *da=*sa;
	    }
	    else if (la->r==COLORMAX && la->g==COLORMAX && la->b==COLORMAX)
	    {
	       *d=*l;
	       *da=*la;
	    }
	    else if (l->r==COLORMAX && l->g==COLORMAX && l->b==COLORMAX)
	    {
	       ALPHA_ADD(s,&white,d,sa,la,da,r);
	       ALPHA_ADD(s,&white,d,sa,la,da,g);
	       ALPHA_ADD(s,&white,d,sa,la,da,b);
	    }
	    else
	    {
	       ALPHA_ADD(s,l,d,sa,la,da,r);
	       ALPHA_ADD(s,l,d,sa,la,da,g);
	       ALPHA_ADD(s,l,d,sa,la,da,b);
	    }
	    l++; s++; la++; sa++; d++; da++;
	 }
      }
   }
   else
   {
      if (!la)  /* no layer alpha => alpha value opaque */
	 while (len--)
	 {
	    ALPHA_ADD_V_NOLA(s,l,d,sa,da,alpha,r);
	    ALPHA_ADD_V_NOLA(s,l,d,sa,da,alpha,g);
	    ALPHA_ADD_V_NOLA(s,l,d,sa,da,alpha,b);

	    l++; s++; sa++; da++; d++;
	 }
      else
	 while (len--)
	 {
	    ALPHA_ADD_V(s,l,d,sa,la,da,alpha,r);
	    ALPHA_ADD_V(s,l,d,sa,la,da,alpha,g);
	    ALPHA_ADD_V(s,l,d,sa,la,da,alpha,b);

	    l++; s++; la++; sa++; da++; d++;
	 }
      return;
   }
}


#if defined(__ECL) && 0
#define WARN_TRACE(X)	static char PIKE_CONCAT(foo__, X) (double d) { return (char)d; };
#else /* !__ECL */
#define WARN_TRACE(X) 
#endif /* __ECL */

/* operators from template */

#define L_COPY_ALPHA

#define LM_FUNC lm_add
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) ((A)+(int)(B))
#define L_MMX_OPER(A,MMXR) paddusb_m2r(A,MMXR)
WARN_TRACE(1)
#include "layer_oper.h"
#undef L_MMX_OPER
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_a_add
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) ((A)+(int)(B))
#define L_MMX_OPER(A,MMXR) paddusb_m2r(A,MMXR)
WARN_TRACE(1)
#include "layer_oper.h"
#undef L_MMX_OPER
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_subtract
#define L_TRUNC(X) MAXIMUM(0,(X))
#define L_OPER(A,B) ((A)-(int)(B))
#define L_MMX_OPER(A,MMXR) psubusb_m2r(A,MMXR)
WARN_TRACE(2)
#include "layer_oper.h"
#undef L_MMX_OPER
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_multiply
#define L_TRUNC(X) (X)
#define L_OPER(A,B) CCUT((A)*(int)(B))
WARN_TRACE(3)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_divide
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) MINIMUM( DOUBLE_TO_INT((A)/C2F(1+(int)(B))), COLORMAX)
WARN_TRACE(4)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_negdivide
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) 1.0-MINIMUM( DOUBLE_TO_INT((A)/C2F(1+(int)(B))), COLORMAX)
WARN_TRACE(4)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_modulo
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) ((A)%((B)?(B):1))
WARN_TRACE(5)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define L_USE_SA

#define LM_FUNC lm_invsubtract
#define L_TRUNC(X) MAXIMUM(0,(X))
#define L_OPER(A,B) ((B)-(int)(A))
WARN_TRACE(6)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_invdivide
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) MINIMUM( DOUBLE_TO_INT((B)/C2F(1+(int)(A))), COLORMAX)
WARN_TRACE(7)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_invmodulo
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) ((B)%((A)?(A):1))
WARN_TRACE(8)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_idivide
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) MINIMUM( DOUBLE_TO_INT((A)/C2F(COLORMAX+1-(int)(B))), COLORMAX)
WARN_TRACE(4)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_imultiply
#define L_TRUNC(X) (X)
#define L_OPER(A,B) CCUT((A)*(COLORMAX-(int)(B)))
WARN_TRACE(3)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_invidivide
#define L_TRUNC(X) MINIMUM(255,(X))
#define L_OPER(A,B) MINIMUM( DOUBLE_TO_INT((B)/C2F(COLORMAX+1-(int)(A))), COLORMAX)
WARN_TRACE(7)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#undef L_USE_SA

#define LM_FUNC lm_difference
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) abs((A)-(B))
WARN_TRACE(9)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_max
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) MAXIMUM((A),(B))
WARN_TRACE(10)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_min
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) MINIMUM((A),(B))
WARN_TRACE(11)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_bitwise_and
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) ((A)&(B))
WARN_TRACE(12)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_bitwise_or
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) ((A)|(B))
WARN_TRACE(13)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_bitwise_xor
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) ((A)^(B))
WARN_TRACE(14)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#undef L_COPY_ALPHA

#define LM_FUNC lm_equal
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) (((A)==(B))?COLORMAX:0)
WARN_TRACE(15)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_not_equal
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) (((A)!=(B))?COLORMAX:0)
WARN_TRACE(16)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_less
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) (((A)>(B))?COLORMAX:0)
WARN_TRACE(17)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_more
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) (((A)<(B))?COLORMAX:0)
WARN_TRACE(18)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_less_or_equal
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) (((A)>=(B))?COLORMAX:0)
WARN_TRACE(19)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_more_or_equal
#define L_TRUNC(X) (DOUBLE_TO_COLORTYPE(X))
#define L_OPER(A,B) (((A)<=(B))?COLORMAX:0)
WARN_TRACE(20)
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

/* logic functions */

#define L_LOGIC(A,B,C) (((A)&&(B)&&(C))?white:black)
#define LM_FUNC lm_logic_equal
#define L_OPER(A,B) ((A)==(B))
#define L_TRANS white
WARN_TRACE(21)
#include "layer_oper.h"
#undef L_TRANS
#undef L_OPER
#undef LM_FUNC
#undef L_LOGIC

#define L_LOGIC(A,B,C) (((A)||(B)||(C))?white:black)
#define LM_FUNC lm_logic_not_equal
#define L_OPER(A,B) ((A)!=(B))
#define L_TRANS black
WARN_TRACE(22)
#include "layer_oper.h"
#undef L_TRANS
#undef L_OPER
#undef LM_FUNC
#undef L_LOGIC

#define L_LOGIC(A,B,C) (((A)&&(B)&&(C))?white:black)
#define LM_FUNC lm_logic_strict_less
#define L_OPER(A,B) ((A)>(B))
#define L_TRANS white
WARN_TRACE(23)
#include "layer_oper.h"
#undef L_TRANS
#undef L_OPER
#undef LM_FUNC
#undef L_LOGIC

#define L_LOGIC(A,B,C) (((A)&&(B)&&(C))?white:black)
#define LM_FUNC lm_logic_strict_more
#define L_OPER(A,B) ((A)<(B))
#define L_TRANS white
WARN_TRACE(24)
#include "layer_oper.h"
#undef L_TRANS
#undef L_OPER
#undef LM_FUNC
#undef L_LOGIC

#define L_LOGIC(A,B,C) (((A)&&(B)&&(C))?white:black)
#define LM_FUNC lm_logic_strict_less_or_equal
#define L_OPER(A,B) ((A)>=(B))
#define L_TRANS white
WARN_TRACE(25)
#include "layer_oper.h"
#undef L_TRANS
#undef L_OPER
#undef LM_FUNC
#undef L_LOGIC

#define L_LOGIC(A,B,C) (((A)&&(B)&&(C))?white:black)
#define LM_FUNC lm_logic_strict_more_or_equal
#define L_OPER(A,B) ((A)<=(B))
#define L_TRANS white
WARN_TRACE(26)
#include "layer_oper.h"
#undef L_TRANS
#undef L_OPER
#undef LM_FUNC
#undef L_LOGIC

/* channels from template */

/* replace rgb by alpha channel */

#define LM_FUNC lm_replace
#define L_CHANNEL_DO(S,L,D,A) 						\
  ((D).r=COMBINE_ALPHA((S).r,(L).r,COLORMAX,(A).r),			\
   (D).g=COMBINE_ALPHA((S).g,(L).g,COLORMAX,(A).g),			\
   (D).b=COMBINE_ALPHA((S).b,(L).b,COLORMAX,(A).b))
#define L_CHANNEL_DO_V(S,L,D,A,V) 					\
  ((D).r=COMBINE_ALPHA_V((S).r,(L).r,COLORMAX,(A).r,(V)),		\
   (D).g=COMBINE_ALPHA_V((S).g,(L).g,COLORMAX,(A).g,(V)),		\
   (D).b=COMBINE_ALPHA_V((S).b,(L).b,COLORMAX,(A).b,(V)))
#include "layer_channel.h"
#undef L_CHANNEL_DO
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* replace r, g or b (by alpha channel, only that color counts) */

#define LM_FUNC lm_red
#define L_CHANNEL_DO(S,L,D,A) 						\
  ((D).r=COMBINE_ALPHA((S).r,(L).r,COLORMAX,(A).r),			\
   (D).g=(S).g,(D).b=(S).b)
#define L_CHANNEL_DO_V(S,L,D,A,V) 					\
  ((D).r=COMBINE_ALPHA_V((S).r,(L).r,COLORMAX,(A).r,(V)),		\
   (D).g=(S).g,(D).b=(S).b)
#include "layer_channel.h"
#undef L_CHANNEL_DO
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_green
#define L_CHANNEL_DO(S,L,D,A) 						\
  ((D).g=COMBINE_ALPHA((S).g,(L).g,COLORMAX,(A).g),			\
   (D).r=(S).r,(D).b=(S).b)
#define L_CHANNEL_DO_V(S,L,D,A,V) 					\
  ((D).g=COMBINE_ALPHA_V((S).g,(L).g,COLORMAX,(A).g,(V)),		\
   (D).r=(S).r,(D).b=(S).b)
#include "layer_channel.h"
#undef L_CHANNEL_DO
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_blue
#define L_CHANNEL_DO(S,L,D,A) 						\
  ((D).b=COMBINE_ALPHA((S).b,(L).b,COLORMAX,(A).b),			\
   (D).g=(S).g,(D).r=(S).r)
#define L_CHANNEL_DO_V(S,L,D,A,V) 					\
  ((D).b=COMBINE_ALPHA_V((S).b,(L).b,COLORMAX,(A).b,(V)),		\
   (D).g=(S).g,(D).r=(S).r)
#include "layer_channel.h"
#undef L_CHANNEL_DO
#undef L_CHANNEL_DO_V
#undef LM_FUNC



#define LM_FUNC lm_hardlight
#define L_CHANNEL_DO(S,L,D,A) L_CHANNEL_DO_V(S,L,D,A,1.0)
#define L_CHANNEL_DO_V(S,L,D,A,V) do {					 \
     int v;								 \
     rgb_group tmp;							 \
      int tr, tg, tb;                                                    \
     if( (L).r > 128 )							 \
       v = 255 - (((255 - (S).r) * (256 - (((L).r - 128)<<1))) >> 8);	 \
     else								 \
       v = ((S).r * ((L).r<<1))>>8;					 \
     tmp.r = MAXIMUM(MINIMUM(v,255),0);					 \
									 \
     if( (L).g > 128 )							 \
       v = 255 - (((255 - (S).g) * (256 - (((L).g - 128)<<1))) >> 8);	 \
     else								 \
       v = ((S).g * ((L).g<<1))>>8;					 \
     tmp.g = MAXIMUM(MINIMUM(v,255),0);					 \
									 \
     if( (L).b > 128 )							 \
       v = 255 - (((255 - (S).b) * (256 - (((L).b - 128)<<1))) >> 8);	 \
     else								 \
       v = ((S).b * ((L).b<<1))>>8;					 \
     tmp.b = MAXIMUM(MINIMUM(v,255),0);                                  \
                                                                         \
      tr = (int)((tmp.r*(V*C2F((A).r))) + ((S).r*(1-(V)*C2F((A).r))));   \
      tg = (int)((tmp.g*(V*C2F((A).g))) + ((S).g*(1-(V)*C2F((A).g))));   \
      tb = (int)((tmp.b*(V*C2F((A).b))) + ((S).b*(1-(V)*C2F((A).b))));   \
      (D).r = MAXIMUM(MINIMUM(tr,255),0);                                \
      (D).g = MAXIMUM(MINIMUM(tg,255),0);                                \
      (D).b = MAXIMUM(MINIMUM(tb,255),0);                                \
 }while(0)
#include "layer_channel.h"
#undef L_CHANNEL_DO
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* replace hsv by alpha channel (r=h, g=s, b=v) */

#define L_CHANNEL_DO(S,L,D,A) L_CHANNEL_DO_V(S,L,D,A,1.0)


#define LM_HSV_DO(HSV_X,S,L,D,A,V)                                       \
   do {									 \
      double lh,lv,ls;							 \
      double sh,sv,ss;							 \
      rgb_group tmp;                                                     \
      int tr, tg, tb;                                                    \
      rgb_to_hsv((S),&sh,&ss,&sv);					 \
      rgb_to_hsv((L),&lh,&ls,&lv);					 \
      HSV_X;                                                             \
      hsv_to_rgb(sh,ss,sv,&(tmp));					 \
      tr = (int)((tmp.r*(V*C2F((A).r))) + ((S).r*(1-(V)*C2F((A).r))));   \
      tg = (int)((tmp.g*(V*C2F((A).g))) + ((S).g*(1-(V)*C2F((A).g))));   \
      tb = (int)((tmp.b*(V*C2F((A).b))) + ((S).b*(1-(V)*C2F((A).b))));   \
      (D).r = MAXIMUM(MINIMUM(tr,255),0);                                \
      (D).g = MAXIMUM(MINIMUM(tg,255),0);                                \
      (D).b = MAXIMUM(MINIMUM(tb,255),0);                                \
   } while (0)


#define LM_FUNC lm_replace_hsv
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sh=lh;ss=ls;sv=lv,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* replace h, s or v (by alpha channel (r=h, g=s, b=v), only that one used) */

#define LM_FUNC lm_hue
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sh=lh,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_saturation
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(ss=ls,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_value
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sv=lv,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_value_mul
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sv*=lv,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* h, s */

#define LM_FUNC lm_color
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sh=lh;ss=ls,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* lighten: max v */

#define LM_FUNC lm_lighten
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sv=MAXIMUM(sv,lv),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* darken: min v */

#define LM_FUNC lm_darken
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(sv=MINIMUM(sv,lv),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* saturate: max s */

#define LM_FUNC lm_saturate
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(ss=MAXIMUM(ss,ls),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* desaturate: min s */

#define LM_FUNC lm_desaturate
#define L_CHANNEL_DO_V(S,L,D,A,V)	LM_HSV_DO(ss=MINIMUM(ss,ls),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#undef LM_HSV_DO

/******************************************************************/ 

#define LM_HLS_DO(HLS_X,S,L,D,A,V)                                       \
   do {									 \
      double lh,ll,ls;							 \
      double sh,sl,ss;							 \
      rgb_group tmp;                                                     \
      int tr, tg, tb;                                                    \
      rgb_to_hls((S),&sh,&sl,&ss);					 \
      rgb_to_hls((L),&lh,&ll,&ls);					 \
      HLS_X;                                                             \
      hls_to_rgb(sh,sl,ss,&(tmp));					 \
      tr = (int)((tmp.r*(V*C2F((A).r))) + ((S).r*(1-(V)*C2F((A).r))));   \
      tg = (int)((tmp.g*(V*C2F((A).g))) + ((S).g*(1-(V)*C2F((A).g))));   \
      tb = (int)((tmp.b*(V*C2F((A).b))) + ((S).b*(1-(V)*C2F((A).b))));   \
      (D).r = MAXIMUM(MINIMUM(tr,255),0);                                \
      (D).g = MAXIMUM(MINIMUM(tg,255),0);                                \
      (D).b = MAXIMUM(MINIMUM(tb,255),0);                                \
   } while (0)

	
#define LM_FUNC lm_hls_replace
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sh=lh;sl=ll;ss=ls,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* replace h, l or s (by alpha channel (r=h, g=l, b=s), only that one used) */

#define LM_FUNC lm_hls_hue
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sh=lh,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_hls_saturation
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(ss=ls,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_hls_lightness
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sl=ll,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#define LM_FUNC lm_hls_lightness_mul
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sl*=ll,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* h, s */

#define LM_FUNC lm_hls_color
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sh=lh;ss=ls,S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* lighten: max v */

#define LM_FUNC lm_hls_lighten
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sl=MAXIMUM(sl,ll),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* darken: min v */

#define LM_FUNC lm_hls_darken
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sl=MINIMUM(sl,ll),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC


/* saturate: max s */

#define LM_FUNC lm_hls_saturate
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sl=MAXIMUM(ss,ls),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

/* desaturate: min s */

#define LM_FUNC lm_hls_desaturate
#define L_CHANNEL_DO_V(S,L,D,A,V) LM_HLS_DO(sl=MINIMUM(ss,ls),S,L,D,A,V)
#include "layer_channel.h"
#undef L_CHANNEL_DO_V
#undef LM_FUNC

#undef LM_HLS_DO

/******************************************************************/

/* screen: 255 - ((255-A)*(255-B)/255) */

#define L_COPY_ALPHA

#define LM_FUNC lm_screen
#define L_TRUNC(X) (X<0?0:(X>255?255:X))
#define L_OPER(A,B) 255 - CCUT((255-A)*(int)(255-B))
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#define LM_FUNC lm_overlay
#define L_TRUNC(X) (X)
#define INT_MULT(a,b)  (((a) * (b) + 0x80)>>8)
#define INT_BLEND(a,b,alpha)  (INT_MULT((a)-(b), alpha) + (b))
#define L_OPER(A,B) INT_BLEND((255-INT_MULT((255-A),(255-B))),INT_MULT(A,B),A)
/*                     screen                             mult        source */
#include "layer_oper.h"
#undef LM_FUNC
#undef L_TRUNC
#undef L_OPER

#undef L_COPY_ALPHA
#undef L_CHANNEL_DO

/* special modes */

static void lm_dissolve(rgb_group *s,rgb_group *l,rgb_group *d,
			rgb_group *sa,rgb_group *la,rgb_group *da,
			int len,double alpha)
{
   if (alpha==0.0)
   {
#ifdef LAYERS_DUAL
      MEMCPY(d,s,sizeof(rgb_group)*len);
      MEMCPY(da,sa,sizeof(rgb_group)*len);
#endif
      return;
   }
   else if (alpha==1.0)
   {
      if (!la)  /* no layer alpha => full opaque */
      {
	 MEMCPY(d,l,sizeof(rgb_group)*len);
	 smear_color(da,white,len);
      }
      else
	 while (len--)
	 {
	    if ((my_rand()%(255*255)) <
		(unsigned)(la->r*87 + la->g*127 + la->b*41))
	       *d=*l,*da=white;
	    else
	       *d=*s,*da=*sa;
	    l++; s++; la++; sa++; da++; d++;
	 }
   }
   else
   {
      int v = DOUBLE_TO_INT(COLORMAX*alpha);
      if (!la)  /* no layer alpha => full opaque */
	 while (len--)
	 {
	    if ((my_rand()&255) < (unsigned)v)
	       *d=*l,*da=white;
	    else
	       *d=*s,*da=*sa;
	    l++; s++; sa++; da++; d++;
	 }
      else
      {
	 while (len--)
	 {
	    if ((my_rand()%(255*255)) <
		(unsigned)((la->r*87 + la->g*127 + la->b*41)>>8)*v)
	       *d=*l,*da=white;
	    else
	       *d=*s,*da=*sa;
	    l++; s++; la++; sa++; da++; d++;
	 }
      }
   }
}

static void lm_behind(rgb_group *s,rgb_group *l,rgb_group *d,
		      rgb_group *sa,rgb_group *la,rgb_group *da,
		      int len,double alpha)
{
   /* la may be NULL, no other */

   if (alpha==0.0) /* optimized */
   {
#ifdef LAYERS_DUAL
      MEMCPY(d,s,sizeof(rgb_group)*len);
      MEMCPY(da,sa,sizeof(rgb_group)*len);
#endif
      return;
   }
   else if (alpha==1.0)
      while (len--)
      {
	 if (sa->r==COLORMAX && sa->g==COLORMAX && sa->b==COLORMAX)
	 {
	    *d=*s;
	    *da=*sa;
	 }
	 else if (sa->r==0 && sa->g==0 && sa->b==0)
	 {
	    *d=*l;
	    if (la) *da=*la; else *da=white;
	 }
	 else if (la)
	 {
	    ALPHA_ADD(l,s,d,la,sa,da,r);
	    ALPHA_ADD(l,s,d,la,sa,da,g);
	    ALPHA_ADD(l,s,d,la,sa,da,b);
	 }
	 else
	 {
	    ALPHA_ADD(l,s,d,(&white),sa,da,r);
	    ALPHA_ADD(l,s,d,(&white),sa,da,g);
	    ALPHA_ADD(l,s,d,(&white),sa,da,b);
	 }
	 l++; s++; sa++; d++; da++;
	 if (la) la++;
      }
   else
      while (len--)
      {
	 if (sa->r==COLORMAX && sa->g==COLORMAX && sa->b==COLORMAX)
	 {
	    *d=*s;
	    *da=*sa;
	 }
	 else if (sa->r==0 && sa->g==0 && sa->b==0)
	 {
	    *d=*l;
	    if (la) *da=*la; else *da=white;
	 }
	 else if (la)
	 {
	    ALPHA_ADD_V(l,s,d,la,sa,da,alpha,r);
	    ALPHA_ADD_V(l,s,d,la,sa,da,alpha,g);
	    ALPHA_ADD_V(l,s,d,la,sa,da,alpha,b);
	 }
	 else
	 {
	    ALPHA_ADD_V(l,s,d,(&white),sa,da,alpha,r);
	    ALPHA_ADD_V(l,s,d,(&white),sa,da,alpha,g);
	    ALPHA_ADD_V(l,s,d,(&white),sa,da,alpha,b);
	 }
	 l++; s++; sa++; d++; da++;
	 if (la) la++;
      }
}

static void lm_erase(rgb_group *s,rgb_group *l,rgb_group *d,
		     rgb_group *sa,rgb_group *la,rgb_group *da,
		     int len,double alpha)
{
   /* la may be NULL, no other */

#ifdef LAYERS_DUAL
   MEMCPY(d,s,sizeof(rgb_group)*len);
#endif

   if (alpha==1.0)
      if (!la) /* full opaque */
	 smear_color(da,black,len);
      else
	 while (len--)
	 {
	    da->r=CCUT(sa->r*(int)(COLORMAX-la->r));
	    da->g=CCUT(sa->g*(int)(COLORMAX-la->g));
	    da->b=CCUT(sa->b*(int)(COLORMAX-la->b));

	    la++; sa++; da++;
	 }
   else
      if (!la) /* full opaque */
      {
	 rgb_group a;
	 a.r=a.g=a.b=COLORMAX-DOUBLE_TO_COLORTYPE(alpha*COLORMAX);
	 smear_color(da,a,len);
      }
      else
	 while (len--)
	 {
	    da->r=CCUT(sa->r*DOUBLE_TO_INT(COLORMAX-alpha*la->r));
	    da->g=CCUT(sa->g*DOUBLE_TO_INT(COLORMAX-alpha*la->g));
	    da->b=CCUT(sa->b*DOUBLE_TO_INT(COLORMAX-alpha*la->b));

	    la++; sa++; da++;
	 }
}

static void lm_spec_burn_alpha(struct layer *ly,
			       rgb_group *l, rgb_group *la,
			       rgb_group *s, rgb_group *sa,
			       rgb_group *d, rgb_group *da,
			       int len)

{
   /* special optimized */
   if (!la)
   {
#ifdef LAYERS_DUAL
      MEMCPY(d,s,len*sizeof(rgb_group));
      MEMCPY(da,sa,len*sizeof(rgb_group));
#endif
      return;
   }

   if (ly->alpha_value==1.0)
      if (!l)
	 if (ly->fill.r!=0 ||
	     ly->fill.g!=0 ||
	     ly->fill.b!=0)
	 {
	    while (len--)
	    {
	       d->r=MINIMUM(s->r+la->r,COLORMAX);
	       d->g=MINIMUM(s->g+la->g,COLORMAX);
	       d->b=MINIMUM(s->b+la->b,COLORMAX);
	       da->r=MINIMUM(sa->r+la->r,COLORMAX);
	       da->g=MINIMUM(sa->g+la->g,COLORMAX);
	       da->b=MINIMUM(sa->b+la->b,COLORMAX);
	       da++; sa++; la++; d++; s++;
	    }
	 }
	 else
	 {
#ifdef LAYERS_DUAL
	    MEMCPY(d,s,len*sizeof(rgb_group));
#endif
	    while (len--)
	    {
	       da->r=MINIMUM(sa->r+la->r,COLORMAX);
	       da->g=MINIMUM(sa->g+la->g,COLORMAX);
	       da->b=MINIMUM(sa->b+la->b,COLORMAX);
	       da++; sa++; la++;
	    }
	 }
      else
	 while (len--)
	 {
	    if ((s->r==COLORMAX &&
		 s->g==COLORMAX &&
		 s->b==COLORMAX))
	    {
	       *d=*s;
	    }
	    else
	    {
	       d->r=MINIMUM(s->r+l->r,COLORMAX);
	       d->g=MINIMUM(s->g+l->g,COLORMAX);
	       d->b=MINIMUM(s->b+l->b,COLORMAX);
	    }
	    da->r=MINIMUM(sa->r+la->r,COLORMAX);
	    da->g=MINIMUM(sa->g+la->g,COLORMAX);
	    da->b=MINIMUM(sa->b+la->b,COLORMAX);
	    da++; sa++; la++; s++; d++;
	    if (l) l++;
	 }
   else
   {
      double alpha=ly->alpha_value;
      while (len--)
      {
	 if ((s->r==COLORMAX &&
	      s->g==COLORMAX &&
	      s->b==COLORMAX)
	     || !l)
	 {
	    *d=*s;
	    da->r=MINIMUM(sa->r+DOUBLE_TO_COLORTYPE(alpha*la->r),COLORMAX);
	    da->g=MINIMUM(sa->g+DOUBLE_TO_COLORTYPE(alpha*la->g),COLORMAX);
	    da->b=MINIMUM(sa->b+DOUBLE_TO_COLORTYPE(alpha*la->b),COLORMAX);
	 }
	 else
	 {
	    d->r=s->r+DOUBLE_TO_COLORTYPE(alpha*l->r);
	    d->g=s->g+DOUBLE_TO_COLORTYPE(alpha*l->g);
	    d->b=s->b+DOUBLE_TO_COLORTYPE(alpha*l->b);

	    da->r=MINIMUM(sa->r+DOUBLE_TO_COLORTYPE(alpha*l->r),COLORMAX);
	    da->g=MINIMUM(sa->g+DOUBLE_TO_COLORTYPE(alpha*l->g),COLORMAX);
	    da->b=MINIMUM(sa->b+DOUBLE_TO_COLORTYPE(alpha*l->b),COLORMAX);
	 }
	 da++; sa++; la++; s++; d++;
      }
   }
}

/*** the add-layer function ***************************/

static INLINE void img_lay_first_line(struct layer *l,
				      int xoffs,int xsize,
				      int y, /* in _this_ layer */
				      rgb_group *d,rgb_group *da)
{
   if (!l->tiled)
   {
      rgb_group *s,*sa;
      int len;

      if (y<0 ||
	  y>=l->ysize ||
	  l->xoffs+l->xsize<xoffs ||
	  l->xoffs>xoffs+xsize) /* outside */
      {
	 smear_color(d,l->fill,xsize);
	 smear_color(da,l->fill_alpha,xsize);
	 return;
      }

      if (l->img) s=l->img->img+y*l->xsize; else s=NULL;
      if (l->alp) sa=l->alp->img+y*l->xsize; else sa=NULL;
      len=l->xsize;

      if (l->xoffs>xoffs)
      {
	 /* fill to the left */
	 smear_color(d,l->fill,l->xoffs-xoffs);
	 smear_color(da,l->fill_alpha,l->xoffs-xoffs);

	 xsize-=l->xoffs-xoffs;
	 d+=l->xoffs-xoffs;
	 da+=l->xoffs-xoffs;
      }
      else
      {
	 if (s) s+=xoffs-l->xoffs;
	 if (sa) sa+=xoffs-l->xoffs;
	 len-=xoffs-l->xoffs;
      }
      if (len<xsize) /* copy bit, fill right */
      {
	 if (s) MEMCPY(d,s,len*sizeof(rgb_group));
	 else smear_color(d,l->fill,len);
	 if (sa) MEMCPY(da,sa,len*sizeof(rgb_group));
	 else smear_color(da,white,len);

	 smear_color(d+len,l->fill,xsize-len);
	 smear_color(da+len,l->fill_alpha,xsize-len);
      }
      else /* copy rest */
      {
	 if (s) MEMCPY(d,s,xsize*sizeof(rgb_group));
	 else smear_color(d,l->fill,xsize);
	 if (sa) MEMCPY(da,sa,xsize*sizeof(rgb_group));
	 else smear_color(da,white,xsize);
      }
      return;
   }
   else
   {
      rgb_group *s,*sa;

      y%=l->ysize;
      if (y<0) y+=l->ysize;

      if (l->img) s=l->img->img+y*l->xsize;
      else smear_color(d,l->fill,xsize),s=NULL;

      if (l->alp) sa=l->alp->img+y*l->xsize;
      else smear_color(da,white,xsize),sa=NULL;

      xoffs-=l->xoffs; /* position in source */
      xoffs%=l->xsize;
      if (xoffs<0) xoffs+=l->xsize;
      if (xoffs)
      {
	 int len=l->xsize-xoffs;
	 if (len>l->xsize) len=l->xsize;
	 if (s) MEMCPY(d,s+xoffs,len*sizeof(rgb_group));
	 if (sa) MEMCPY(da,sa+xoffs,len*sizeof(rgb_group));
	 da+=len;
	 d+=len;
	 xsize-=len;
      }
      while (xsize>l->xsize)
      {
	 if (s) MEMCPY(d,s,l->xsize*sizeof(rgb_group));
	 if (sa) MEMCPY(d,sa,l->xsize*sizeof(rgb_group));
	 da+=l->xsize;
	 d+=l->xsize;
	 xsize-=l->xsize;
      }
      if (s) MEMCPY(d,s,xsize*sizeof(rgb_group));
      if (sa) MEMCPY(d,sa,xsize*sizeof(rgb_group));
   }
}

static INLINE void img_lay_stroke(struct layer *ly,
				  rgb_group *l,
				  rgb_group *la,
				  rgb_group *s,
				  rgb_group *sa,
				  rgb_group *d,
				  rgb_group *da,
				  int len)
{
   if (len<0) Pike_error("internal error: stroke len < 0\n");
   if (!ly->row_func) Pike_error("internal error: row_func=NULL\n");

   if (ly->row_func==(lm_row_func*)lm_spec_burn_alpha)
   {
      lm_spec_burn_alpha(ly,l,la,s,sa,d,da,len);
      return;
   }

   if (l)
   {
      (ly->row_func)(s,l,d,sa,la,da,len,ly->alpha_value);
      return;
   }
   if (!la && ly->really_optimize_alpha)
   {
/*       fprintf(stderr,"fast skip ly->yoffs=%d\n",ly->yoffs); */
#ifdef LAYERS_DUAL
      MEMCPY(d,s,len*sizeof(rgb_group));
      MEMCPY(da,sa,len*sizeof(rgb_group));
#endif
      return;
   }

   if (!la &&
       ly->fill_alpha.r==COLORMAX &&
       ly->fill_alpha.g==COLORMAX &&
       ly->fill_alpha.b==COLORMAX)
   {
      while (len>SNUMPIXS)
      {
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,NULL,da,
			SNUMPIXS,ly->alpha_value);
	 s+=SNUMPIXS; d+=SNUMPIXS;
	 sa+=SNUMPIXS;da+=SNUMPIXS;
	 if (l) l+=SNUMPIXS;
	 len-=SNUMPIXS;
      }
      if (len)
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,NULL,da,len,ly->alpha_value);
   }
   else
   {
/* fprintf(stderr,"ly=%p len=%d\n",ly,len); */

      while (len>SNUMPIXS)
      {
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,la?la:ly->sfill_alpha,da,
			SNUMPIXS,ly->alpha_value);
	 s+=SNUMPIXS; d+=SNUMPIXS;
	 sa+=SNUMPIXS; da+=SNUMPIXS;
	 if (l) l+=SNUMPIXS;
	 if (la) la+=SNUMPIXS;
	 len-=SNUMPIXS;
      }
      if (len)
	 (ly->row_func)(s,l?l:ly->sfill,d,sa,la?la:ly->sfill_alpha,
			da,len,ly->alpha_value);
   }
}

static INLINE void img_lay_line(struct layer *ly,
				rgb_group *s,rgb_group *sa,
				int xoffs,int xsize,
				int y, /* y in ly layer */
				rgb_group *d,rgb_group *da)
{
/*     fprintf(stderr,"tiled:%d xoffs:%d xsize:%d y:%d\n", */
/*  	   ly->tiled,xoffs,xsize,y); */

   if (!ly->tiled)
   {
      int len;
      rgb_group *l,*la;

      if (y<0 ||
	  y>=ly->ysize ||
	  ly->xoffs+ly->xsize<xoffs ||
	  ly->xoffs>xoffs+xsize) /* outside */
      {
	 img_lay_stroke(ly,NULL,NULL,s,sa,d,da,xsize);
	 return;
      }

      if (ly->img) l=ly->img->img+y*ly->xsize; else l=NULL;
      if (ly->alp) la=ly->alp->img+y*ly->xsize; else la=NULL;
      len=ly->xsize;

      if (ly->xoffs>xoffs)
      {
	 /* fill to the left */
	 img_lay_stroke(ly,NULL,NULL,s,sa,d,da,ly->xoffs-xoffs);

	 xsize-=ly->xoffs-xoffs;
	 d+=ly->xoffs-xoffs;
	 da+=ly->xoffs-xoffs;
	 s+=ly->xoffs-xoffs;
	 sa+=ly->xoffs-xoffs;
      }
      else
      {
	 if (l) l+=xoffs-ly->xoffs;
	 if (la) la+=xoffs-ly->xoffs;
	 len-=xoffs-ly->xoffs;
      }
      if (len<xsize) /* copy stroke, fill right */
      {
         img_lay_stroke(ly,l,la,s,sa,d,da,len);

	 img_lay_stroke(ly,NULL,NULL,s+len,sa+len,d+len,da+len,xsize-len);
      }
      else /* copy rest */
      {
	 img_lay_stroke(ly,l,la,s,sa,d,da,xsize);
      }
      return;
   }
   else
   {
      rgb_group *l,*la;

      y%=ly->ysize;
      if (y<0) y+=ly->ysize;

      if (ly->img) l=ly->img->img+y*ly->xsize; else l=NULL;
      if (ly->alp) la=ly->alp->img+y*ly->xsize; else la=NULL;

      xoffs-=ly->xoffs; /* position in source */
      if ((xoffs=xoffs%ly->xsize))
      {
	 int len;
	 if (xoffs<0) xoffs+=ly->xsize;
	 len=ly->xsize-xoffs;
	 if (len>xsize) len=xsize;

/*  	 fprintf(stderr,"a xoffs=%d len=%d xsize=%d ly->xsize:%d\n",xoffs,len,xsize,ly->xsize); */

	 img_lay_stroke(ly,l?l+xoffs:NULL,
			la?la+(xoffs%ly->xsize):NULL,
			s,sa,d,da,len);
	 da+=len;
	 d+=len;
	 sa+=len;
	 s+=len;
	 xsize-=len;
      }
      while (xsize>ly->xsize)
      {
/*  	 fprintf(stderr,"b xsize=%d\n",xsize); */

	 img_lay_stroke(ly,l,la,s,sa,d,da,ly->xsize);
	 da+=ly->xsize;
	 d+=ly->xsize;
	 sa+=ly->xsize;
	 s+=ly->xsize;
	 xsize-=ly->xsize;
      }
      if (xsize)
      {
/*  	 fprintf(stderr,"c xsize=%d\n",xsize); */
	 img_lay_stroke(ly,l,la,s,sa,d,da,xsize);
      }
   }
}


void img_lay(struct layer **layer,
	     int layers,
	     struct layer *dest)
{
#ifdef LAYERS_DUAL
   rgb_group *line1,*aline1;
   rgb_group *line2,*aline2;
#endif
   rgb_group *d,*da;
   int width=dest->xsize;
   int y,z;
   int xoffs=dest->xoffs,xsize=dest->xsize;

#ifdef LAYERS_DUAL
   line1=malloc(sizeof(rgb_group)*width);
   aline1=malloc(sizeof(rgb_group)*width);
   line2=malloc(sizeof(rgb_group)*width);
   aline2=malloc(sizeof(rgb_group)*width);
   if (!line1 || !aline1
       !line2 || !aline2)
   {
      if (line1) free(line1);
      if (aline1) free(aline1);
      if (line2) free(line2);
      if (aline2) free(aline2);
      resource_error(NULL,0,0,"memory",sizeof(rgb_group)*4*width,
		     "Out of memory.\n");
   }
#endif

   da=dest->alp->img;
   d=dest->img->img;

   /* loop over lines */
   for (y=0; y<dest->ysize; y++)
   {
      if (layers>1 || layer[0]->row_func!=lm_normal ||
	  layer[0]->tiled)
      {
	 /* add the bottom layer first */
	 if (layer[0]->row_func==lm_normal &&
	     !layer[0]->tiled)                 /* cheat */
	 {
	    img_lay_first_line(layer[0],xoffs,xsize,
			       y+dest->yoffs-layer[0]->yoffs,
#ifdef LAYERS_DUAL
			       line1,aline1
#else
			       d,da
#endif
			       ),z=1;
	    z=1;
	 }
	 else
	 {
#ifdef LAYERS_DUAL
	    smear_color(line1,black,xsize);
	    smear_color(aline1,black,xsize);
#else
	    smear_color(d,black,xsize);
	    smear_color(da,black,xsize);
#endif
	    z=0;
	 }

	 /* loop over the rest of the layers, except the last */
	 for (; z<layers-1; z++)
	    if (!layer[z]->really_optimize_alpha ||
		(layer[z]->yoffs<=y+dest->yoffs &&
		 y+dest->yoffs<layer[z]->yoffs+layer[z]->ysize))
	    {
/* 	       if (!layer[z]->really_optimize_alpha) */
/* 		  fprintf(stderr,"huh %d\n",z); */
/* 	       if (!(layer[z]->yoffs>=y+dest->yoffs &&  */
/* 		     y+dest->yoffs<layer[z]->yoffs+layer[z]->ysize)) */
/* 		  fprintf(stderr,"hmm %d %d<=%d<%d\n", */
/* 			  z, */
/* 			  layer[z]->yoffs, */
/* 			  y+dest->yoffs, */
/* 			  layer[z]->yoffs+layer[z]->ysize); */

	       img_lay_line(layer[z],
#ifdef LAYERS_DUAL
			    line1,aline1,
#else
			    d,da,
#endif
			    xoffs,xsize,
			    y+dest->yoffs-layer[z]->yoffs,
#ifdef LAYERS_DUAL
			    line2,aline2
#else
			    d,da
#endif
			    );
#ifdef LAYERS_DUAL
	       /* swap buffers */
	       tmp=line1; line1=line2; line2=tmp;
	       tmp=aline1; aline1=aline2; aline2=tmp;
#endif
	    }
/* 	    else */
/* 	       fprintf(stderr,"skip %d\n",z); */

	 /* make the last layer on the destionation */
	 img_lay_line(layer[layers-1],
#ifdef LAYERS_DUAL
		      line1,aline1,
#else
		      d,da,
#endif
		      xoffs,xsize,y+dest->yoffs-layer[layers-1]->yoffs,
		      d,da);
      }
      else
      {
	 /* make the layer to destination*/
	 img_lay_first_line(layer[0],xoffs,xsize,
			    y+dest->yoffs-layer[0]->yoffs,d,da);
      }
      d+=dest->xsize;
      da+=dest->xsize;
   }

#ifdef LAYERS_DUAL
   free(line1);
   free(aline1);
   free(line2);
   free(aline2);
#endif
}

/*
**! module Image
**! method Image.Layer lay(array(Image.Layer|mapping) layers)
**! method Image.Layer lay(array(Image.Layer|mapping) layers,int xoffset,int yoffset,int xsize,int ysize)
**!	Combine layers.
**! returns a new layer object.
**!
**! see also: Image.Layer
*/

void image_lay(INT32 args)
{
   int layers,i,j;
   struct layer **l;
   struct object *o;
   struct layer *dest;
   struct array *a;
   INT_TYPE xoffset=0,yoffset=0,xsize=0,ysize=0;
   ONERROR err;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("Image.lay",1);

   if (Pike_sp[-args].type!=T_ARRAY)
      SIMPLE_BAD_ARG_ERROR("Image.lay",1,
			   "array(Image.Layer|mapping)");

   if (args>1)
   {
      get_all_args("Image.lay",args-1,"%i%i%i%i",
		   &xoffset,&yoffset,&xsize,&ysize);
      if (xsize<1)
	 SIMPLE_BAD_ARG_ERROR("Image.lay",4,"int(1..)");
      if (ysize<1)
	 SIMPLE_BAD_ARG_ERROR("Image.lay",5,"int(1..)");
   }

   layers=(a=Pike_sp[-args].u.array)->size;

   if (!layers) /* dummy return empty layer */
   {
      pop_n_elems(args);
      push_object(clone_object(image_layer_program,0));
      return;
   }

   l=(struct layer**)xalloc(sizeof(struct layer)*layers);

   SET_ONERROR(err, free, l);

   for (i=j=0; i<layers; i++)
   {
      if (a->item[i].type==T_OBJECT)
      {
	 if (!(l[j]=(struct layer*)get_storage(a->item[i].u.object,
					       image_layer_program)))
	    SIMPLE_BAD_ARG_ERROR("Image.lay",1,
				 "array(Image.Layer|mapping)");
      }
      else if (a->item[i].type==T_MAPPING)
      {
	 push_svalue(a->item+i);
	 push_object(o=clone_object(image_layer_program,1));
	 args++;
	 l[j]=(struct layer*)get_storage(o,image_layer_program);
      }
      else
	 SIMPLE_BAD_ARG_ERROR("Image.lay",1,
			      "array(Image.Layer|mapping)");
      if (l[j]->xsize && l[j]->ysize)
	 j++;
   }

   if (!(layers = j))	/* dummy return empty layer */
   {
      CALL_AND_UNSET_ONERROR(err);
      pop_n_elems(args);
      push_object(clone_object(image_layer_program,0));
      return;
   }

   if (xsize==0) /* figure offset and size */
   {
      xoffset=l[0]->xoffs;
      yoffset=l[0]->yoffs;
      xsize=l[0]->xsize;
      ysize=l[0]->ysize;
      if (l[0]->tiled) /* set size from the first non-tiled */
      {
	 for (i=1; i<layers; i++)
	    if (!l[i]->tiled)
	    {
	       xoffset=l[i]->xoffs;
	       yoffset=l[i]->yoffs;
	       xsize=l[i]->xsize;
	       ysize=l[i]->ysize;
	       break;
	    }
      }
      else i=1;
      for (; i<layers; i++)
	 if (!l[i]->tiled)
	 {
	    int t;
	    if (l[i]->xoffs<xoffset)
	       t=xoffset-l[i]->xoffs,xoffset-=t,xsize+=t;
	    if (l[i]->yoffs<yoffset)
	       t=yoffset-l[i]->yoffs,yoffset-=t,ysize+=t;
	    if (l[i]->xsize+l[i]->xoffs-xoffset>xsize)
	       xsize=l[i]->xsize+l[i]->xoffs-xoffset;
	    if (l[i]->ysize+l[i]->yoffs-yoffset>ysize)
	       ysize=l[i]->ysize+l[i]->yoffs-yoffset;
	 }
   }

   /* get destination layer */
   push_int(xsize);
   push_int(ysize);
   push_object(o=clone_object(image_layer_program,2));

   dest=(struct layer*)get_storage(o,image_layer_program);
   dest->xoffs=xoffset;
   dest->yoffs=yoffset;

   /* ok, do it! */
   img_lay(l,layers,dest);

   CALL_AND_UNSET_ONERROR(err);

   Pike_sp--;
   pop_n_elems(args);
   push_object(o);
}

/**  image-object operations  *************************/

static INLINE struct layer *push_new_layer(void)
{
   push_object(clone_object(image_layer_program,0));
   return (struct layer*)get_storage(Pike_sp[-1].u.object,image_layer_program);
}

static INLINE struct layer *clone_this_layer(void)
{
   struct layer *l;
   l=push_new_layer();
   l->xsize=THIS->xsize;
   l->ysize=THIS->ysize;
   l->xoffs=THIS->xoffs;
   l->yoffs=THIS->yoffs;
   l->image=THIS->image;
   l->alpha=THIS->alpha;
   l->img=THIS->img;
   l->alp=THIS->alp;
   if (l->image) add_ref(l->image);
   if (l->alpha) add_ref(l->alpha);
   l->alpha_value=THIS->alpha_value;
   l->fill=THIS->fill;
   l->fill_alpha=THIS->fill_alpha;
   MEMCPY(l->sfill,THIS->sfill,sizeof(rgb_group)*SNUMPIXS);
   MEMCPY(l->sfill_alpha,THIS->sfill_alpha,sizeof(rgb_group)*SNUMPIXS);
   l->tiled=THIS->tiled;
   l->row_func=THIS->row_func;
   l->optimize_alpha=THIS->optimize_alpha;
   l->really_optimize_alpha=THIS->really_optimize_alpha;
   if (THIS->misc) l->misc = copy_mapping( THIS->misc );
   return l;
}

/*
**! module Image
**! class Layer


**! method object clone()
**!	Creates a copy of the called object.
**! returns the copy
*/

static void image_layer_clone(INT32 args)
{
   pop_n_elems(args);
   clone_this_layer();
}

/*
**! method object crop(int xoff,int yoff,int xsize,int ysize)
**!	Crops this layer at this offset and size.
**!	Offset is not relative the layer offset, so this
**!	can be used to crop a number of layers simuntaneously.
**!
**!	The <ref>fill</ref> values are used if the layer is
**!	enlarged.
**! returns a new layer object
**! note:
**!	The new layer object may have the same image object,
**!	if there was no cropping to be done.
*/

static void image_layer_crop(INT32 args)
{
   struct layer *l;
   INT_TYPE x,y,xz,yz,xi,yi;
   int zot=0;
   struct image *img = NULL;

   get_all_args("Image.Layer->crop",args,"%i%i%i%i",&x,&y,&xz,&yz);

   l=clone_this_layer();
   if (x<=l->xoffs) x=l->xoffs; else zot++;
   if (y<=l->yoffs) y=l->yoffs; else zot++;
   if (l->xsize+l->xoffs<=x+xz) xz=l->xsize-(x-l->xoffs); else zot++;
   if (l->ysize+l->yoffs<=y+yz) yz=l->ysize-(y-l->yoffs); else zot++;

   xi=x-l->xoffs;
   yi=y-l->yoffs;
   l->xoffs=x;
   l->yoffs=y;

   if (zot && l->image)
   {
      ref_push_object(l->image);
      push_constant_text("copy");
      f_index(2);
      push_int(xi);
      push_int(yi);
      push_int(xz+xi-1);
      push_int(yz+yi-1);
      push_int(THIS->fill.r);
      push_int(THIS->fill.g);
      push_int(THIS->fill.b);
      f_call_function(8);
      if (Pike_sp[-1].type!=T_OBJECT ||
	  !(img=(struct image*)get_storage(Pike_sp[-1].u.object,image_program)))
	 Pike_error("No image returned from image->copy\n");
      if (img->xsize!=xz || img->ysize!=yz)
	 Pike_error("Image returned from image->copy had "
	       "unexpected size (%"PRINTPIKEINT"d,%"PRINTPIKEINT"d,"
		    " expected %"PRINTPIKEINT"d,%"PRINTPIKEINT"d)\n",
		img->xsize,img->ysize,xz,yz);

      free_object(l->image);
      l->image=Pike_sp[-1].u.object;
      Pike_sp--;
      dmalloc_touch_svalue(Pike_sp);
      l->img=img;
   }

   if (zot && l->alpha)
   {
      ref_push_object(l->alpha);
      push_constant_text("copy");
      f_index(2);
      push_int(xi);
      push_int(yi);
      push_int(xz+xi-1);
      push_int(yz+yi-1);
      push_int(THIS->fill_alpha.r);
      push_int(THIS->fill_alpha.g);
      push_int(THIS->fill_alpha.b);
      f_call_function(8);
      if (Pike_sp[-1].type!=T_OBJECT ||
	  !(img=(struct image*)get_storage(Pike_sp[-1].u.object,image_program)))
	 Pike_error("No image returned from alpha->copy\n");
      if (img->xsize!=xz || img->ysize!=yz)
	 Pike_error("Image returned from alpha->copy had "
	       "unexpected size (%"PRINTPIKEINT"d,%"PRINTPIKEINT"d, "
		    "expected %"PRINTPIKEINT"d,%"PRINTPIKEINT"d)\n",
	       img->xsize,img->ysize,xz,yz);
      free_object(l->alpha);
      l->alpha=Pike_sp[-1].u.object;
      Pike_sp--;
      dmalloc_touch_svalue(Pike_sp);
      l->alp=img;
   }

   l->xoffs=x;
   l->yoffs=y;
   l->xsize=xz;
   l->ysize=yz;

   stack_pop_n_elems_keep_top(args);
}

/*
**! method object autocrop()
**! method object autocrop(int(0..1) left,int(0..1) right,int(0..1) top,int(0..1) bottom)
**! method array(int) find_autocrop()
**! method array(int) find_autocrop(int(0..1) left,int(0..1) right,int(0..1) top,int(0..1) bottom)
**!	This crops (of finds) a suitable crop, non-destructive crop.
**!	The layer alpha channel is checked, and edges that is
**!	transparent is removed.
**!
**!	(What really happens is that the image and alpha channel is checked,
**!	and edges equal the fill setup is cropped away.)
**!
**!	<ref>find_autocrop</ref>() returns an array of xoff,yoff,xsize,ysize,
**!	which can be fed to <ref>crop</ref>().
**!
**! note:
**!	A tiled image will not be cropped at all.
**!
**!	<tt>left</tt>...<tt>bottom</tt> arguments can be used
**!	to tell what sides cropping are ok on.
**!
**! see also: crop, Image.Image->autocrop
*/

static void image_layer_find_autocrop(INT32 args)
{
   INT32 x1=0,y1=0,x2=THIS->xsize-1,y2=THIS->ysize-1;
   INT_TYPE l=1,r=1,t=1,b=1;

   if (args>3)
      get_all_args("find_autocrop",args,"%i%i%i%i",&l,&r,&t,&b);

   if (!THIS->tiled) {
      if (THIS->alpha)
      {
	 img_find_autocrop(THIS->alp, &x1,&y1,&x2,&y2,
			   0,l,r,t,b,1,THIS->fill_alpha);
	 if (THIS->image &&
	     (THIS->fill_alpha.r!=0 ||
	      THIS->fill_alpha.g!=0 ||  /* non-transparent fill */
	      THIS->fill_alpha.b!=0))   /* check image too      */
	 {
	    INT32 ix1,iy1,ix2,iy2;
	    img_find_autocrop(THIS->img, &ix1,&iy1,&ix2,&iy2,
			      0,(int)l,(int)r,(int)t,(int)b,1,THIS->fill);
	    if (ix1<x1) x1=ix1;
	    if (ix2>x2) x2=ix2;
	    if (iy1<y1) y1=iy1;
	    if (iy2>y2) y2=iy2;
	 }
      }
      else if (THIS->image && /* image alpha is 'white'... */
	       (THIS->fill_alpha.r==255 ||
		THIS->fill_alpha.g==255 ||  /* opaque fill */
		THIS->fill_alpha.b==255))   /* we may be able to crop */
      {
	 img_find_autocrop(THIS->img, &x1,&y1,&x2,&y2,
			   0,(int)l,(int)r,(int)t,(int)b,1,THIS->fill);
      }
   }
   push_int(x1+THIS->xoffs);
   push_int(y1+THIS->yoffs);
   push_int(x2-x1+1);
   push_int(y2-y1+1);
   f_aggregate(4);
}

void image_layer_autocrop(INT32 args)
{
   image_layer_find_autocrop(args);
   Pike_sp--;
   dmalloc_touch_svalue(Pike_sp);
   push_array_items(Pike_sp->u.array); /* frees */
   image_layer_crop(4);
}


static void image_layer__sprintf( INT32 args )
{
  int x;
  if (args != 2 )
    SIMPLE_TOO_FEW_ARGS_ERROR("_sprintf",2);
  if (Pike_sp[-args].type!=T_INT)
    SIMPLE_BAD_ARG_ERROR("_sprintf",0,"integer");
  if (Pike_sp[1-args].type!=T_MAPPING)
    SIMPLE_BAD_ARG_ERROR("_sprintf",1,"mapping");

  x = Pike_sp[-2].u.integer;

  pop_n_elems( 2 );
  switch( x )
  {
   case 't':
     push_constant_text("Image.Layer");
     return;
   case 'O':
     push_constant_text( "Image.Layer(%O i=%O a=%O)" );
     image_layer_mode(0);
     if( THIS->image ) ref_push_object( THIS->image ); else push_int( 0 );
     if( THIS->alpha ) ref_push_object( THIS->alpha ); else push_int( 0 );
     f_sprintf( 4 );
     return;
   default:
     push_int(0);
     return;
  }
}

/******************************************************/

void init_image_layers(void)
{
   int i;

   for (i=0; i<LAYER_MODES; i++)
      layer_mode[i].ps=make_shared_string(layer_mode[i].name);

   /* layer object */

   ADD_STORAGE(struct layer);
   set_init_callback(init_layer);
   set_exit_callback(exit_layer);


   ADD_FUNCTION("create",image_layer_create,
		tOr4(tFunc(tNone,tVoid),
		     tFunc(tObj tOr(tObj,tVoid) tOr(tString,tVoid),tVoid),
		     tFunc(tLayerMap,tVoid),
		     tFunc(tInt tInt
			   tOr(tColor,tVoid) tOr(tColor,tVoid),tVoid)),0);


   ADD_FUNCTION("_sprintf",image_layer__sprintf,
                tFunc(tInt tMapping,tString),0);
   ADD_FUNCTION("cast",image_layer_cast,
		tFunc(tString,tMapping),0);


   ADD_FUNCTION("clone",image_layer_clone,
		tFunc(tNone,tObj),0);

   /* set */

   ADD_FUNCTION("set_offset",image_layer_set_offset,tFunc(tInt tInt,tObj),0);
   ADD_FUNCTION("set_image",image_layer_set_image,
		tFunc(tOr(tObj,tVoid) tOr(tObj,tVoid),tObj),0);
   ADD_FUNCTION("set_fill",image_layer_set_fill,
		tFunc(tOr(tObj,tVoid) tOr(tObj,tVoid),tObj),0);
   ADD_FUNCTION("set_mode",image_layer_set_mode,tFunc(tStr,tObj),0);
   ADD_FUNCTION("set_alpha_value",image_layer_set_alpha_value,
                tFunc(tFloat,tObj),0);
   ADD_FUNCTION("set_tiled",image_layer_set_tiled,tFunc(tInt,tObj),0);

   ADD_FUNCTION("set_misc_value", image_layer_set_misc_value,
                tFunc(tMixed tMixed, tMixed),0 );


   /* query */

   ADD_FUNCTION("image",image_layer_image,tFunc(tNone,tObj),0);
   ADD_FUNCTION("alpha",image_layer_alpha,tFunc(tNone,tObj),0);
   ADD_FUNCTION("mode",image_layer_mode,tFunc(tNone,tStr),0);

   ADD_FUNCTION("available_modes",image_layer_available_modes,
		tFunc(tNone,tArr(tStr)),0);
   ADD_FUNCTION("descriptions",image_layer_descriptions,
		tFunc(tNone,tArr(tStr)),0);

   ADD_FUNCTION("xsize",image_layer_xsize,tFunc(tNone,tInt),0);
   ADD_FUNCTION("ysize",image_layer_ysize,tFunc(tNone,tInt),0);

   ADD_FUNCTION("xoffset",image_layer_xoffset,tFunc(tNone,tInt),0);
   ADD_FUNCTION("yoffset",image_layer_yoffset,tFunc(tNone,tInt),0);

   ADD_FUNCTION("alpha_value",image_layer_alpha_value,tFunc(tNone,tFloat),0);
   ADD_FUNCTION("fill",image_layer_fill,tFunc(tNone,tObj),0);
   ADD_FUNCTION("fill_alpha",image_layer_fill_alpha,tFunc(tNone,tObj),0);

   ADD_FUNCTION("tiled",image_layer_tiled,tFunc(tNone,tInt01),0);

   ADD_FUNCTION("get_misc_value", image_layer_get_misc_value,
                tFunc(tMixed, tMixed),0 );

   /* image-object operations */

   ADD_FUNCTION("crop",image_layer_crop,
		tFunc(tInt tInt tInt tInt,tObj),0);
   ADD_FUNCTION("autocrop",image_layer_autocrop,
		tFuncV(tNone,tOr(tVoid,tInt),tObj),0);
   ADD_FUNCTION("find_autocrop",image_layer_find_autocrop,
		tFuncV(tNone,tOr(tVoid,tInt),tObj),0);

   /*
   ADD_FUNCTION("rotate",image_layer_rotate,tFunc(tOr(tInt,tFloat),tObj),0);
   ADD_FUNCTION("scale",image_layer_scale,tOr3(tFunc(tInt tInt,tObj),
					       tFunc(tFloat,tObj),
					       tFunc(tFloat,tFloat,tObj)),0);
   ADD_FUNCTION("translate",image_layer_translate,
		tFunc(tOr(tInt,tFlt) tOr(tInt,tFlt),tObj),0);
   */
}

void exit_image_layers(void)
{
   int i;

   for (i=0; i<LAYER_MODES; i++)
      free_string(layer_mode[i].ps);
}
