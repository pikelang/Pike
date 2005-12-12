/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: _xpm.c,v 1.36 2005/12/12 20:25:34 nilsson Exp $
*/

#include "global.h"
#include "image_machine.h"

#include "interpret.h"
#include "svalue.h"
#include "program.h"
#include "pike_error.h"
#include "stralloc.h"
#include "operators.h"
#include "threads.h"
#include "module_support.h"

#include "image.h"
#include "colortable.h"


#define sp Pike_sp

/*! @module Image
 *!
 *! @module _XPM
 */

extern struct program *image_program;

static int hextoint( int what )
{
  if(what >= '0' && what <= '9')
    return what-'0';
  if(what >= 'a' && what <= 'f')
    return 10+what-'a';
  if(what >= 'A' && what <= 'F')
    return 10+what-'A';
  return 0;
}

struct buffer
{
  ptrdiff_t len;
  char *str;
};

static rgba_group decode_color( struct buffer *s )
{
  static struct svalue _parse_color;
  static struct svalue *parse_color;
  rgba_group res;
  res.alpha = 255;

  if(!s->len)
  {
    res.r=res.g=res.b = 0;
    return res;
  }
  if(s->str[0] == '#' && s->len>3)
  {
    switch(s->len)
    {
     default:
       res.r = hextoint(s->str[1])*0x10;
       res.g = hextoint(s->str[2])*0x10;
       res.b = hextoint(s->str[3])*0x10;
       break;
     case 7:
       res.r = hextoint(s->str[1])*0x10 + hextoint(s->str[2]);
       res.g = hextoint(s->str[3])*0x10 + hextoint(s->str[4]);
       res.b = hextoint(s->str[5])*0x10 + hextoint(s->str[6]);
       break;
     case 13:
       res.r = hextoint(s->str[1])*0x10 + hextoint(s->str[2]);
       res.g = hextoint(s->str[5])*0x10 + hextoint(s->str[6]);
       res.b = hextoint(s->str[9])*0x10 + hextoint(s->str[10]);
       break;
    }
    return res;
  } 
  if(s->len==4&&(!strncmp(s->str,"None",4)||!strncmp(s->str,"none",4)))
  {
#ifdef HIDE_WARNINGS
      res.r = res.g = res.b = 0;
#endif
    res.alpha = 0;
    return res;
  }
  if(!parse_color)
  {
    push_text("Image.Color");
    SAFE_APPLY_MASTER( "resolv_or_error", 1 );
    _parse_color = sp[-1];
    parse_color = &_parse_color;
    sp--;
  }
  push_svalue( parse_color );
  push_string(make_shared_binary_string(s->str,s->len));
  f_index( 2 );
  if(sp[-1].type != T_OBJECT) {
    push_int(0);
    stack_swap();
  } else {
    push_constant_text( "array" );
    apply( sp[-2].u.object, "cast", 1 );
  }
  if(sp[-1].type == T_ARRAY && sp[-1].u.array->size == 3)
  {
    res.r = sp[-1].u.array->item[0].u.integer;
    res.g = sp[-1].u.array->item[1].u.integer;
    res.b = sp[-1].u.array->item[2].u.integer;
  } else {
    res.r = res.g = res.b = 0;
  }
  pop_stack(); /* array */
  pop_stack(); /* object */
  return res;
}


static rgba_group parse_color_line( struct pike_string *cn, int sl )
{
  int toggle = 0;
  struct buffer s;
  rgba_group res;
  int i;
  for(i=sl; i<cn->len; i++)
  {
    switch(cn->str[i])
    {
     case ' ':
     case '\t':
       if(toggle==4) {
	 s.len = i-(s.str-cn->str);
         return decode_color(&s);
       } else if(toggle>=2)
	 toggle=3;
       else
	 toggle=0;
       break;
     case 'c':
       if(!toggle) {
	 toggle=2;
	 break;
       }
     default:
       if(toggle == 3)
       {
	 s.str = cn->str+i;
	 toggle = 4;
       } else if(toggle != 4)
	 toggle=1;
    }
  }
  if(toggle==4) {
    s.len = cn->len-(s.str-cn->str);
    return decode_color(&s);
  }
  res.r = res.g = res.b = 0;
  res.alpha = 255;
  return res;
}

static rgba_group qsearch( char *s,int sl, struct array *c )
{
  int start = c->size/2;
  int lower = 0;
  int upper = c->size-1;
  struct pike_string *cn;
  while( 1 )
  {
    int i, ok=1;
    cn = c->item[start].u.string;
    for(i=0; i<sl; i++)
      if(cn->str[i] < s[i])
      {
        lower = start;
        start += (upper-start)/2;
        ok=0;
        break;
      } else if(cn->str[i] > s[i]) {
        upper = start;
        start -= (start-lower)/2;
        ok=0;
        break;
      }

    if(ok)
      return parse_color_line( cn,sl );
    if(upper-lower < 2)
    {
      rgba_group res;
      res.r = res.g = res.b = 0;
      res.alpha = 0;
      return res;
    }
  }
}

unsigned short extract_short( unsigned char *b )
{
  return (b[0]<<8)|b[1];
}

/*! @decl int(0..0) _xpm_write_rows(Image.Image img, Image.Image alpha, @
 *!                 int bpc, array(string) colors, array(string) pixels)
 *!
 *! Fills in @[img] and @[alpha] according to xpm data in @[bpc], @[colors]
 *! and @[pixels].
 *!
 *! @param bpc
 *!   Bytes per color. Number of bytes used to encode each color in @[pixels].
 *!
 *! @param colors
 *!   Array of color definitions.
 *!
 *!   A color definition is on the format @tt{"ee c #RRGGBB"@},
 *!   where @tt{ee@} is a @[bpc] long string used to encode the color,
 *!   @tt{c@} is a literal @tt{"c"@}, and @tt{RRGGBB@} is a hexadecimal
 *!   RGB code.
 *!
 *! @param pixels
 *!   Raw picture information.
 *!
 *!   @array pixels
 *!     @elem string 0
 *!       Size information on the format
 *!         (@expr{sprintf("%d %d %d %d", h, w, ncolors, bpn)@}).
 *!     @elem string 1..ncolors
 *!       Same as @[colors].
 *!     @elem string ncolors_plus_one..ncolors_plus_h
 *!       Line information. Strings of length @[bpn]*w with encoded
 *!       pixels for each line.
 *!   @endarray
 */
void f__xpm_write_rows( INT32 args )
{
  struct object *img;
  struct object *alpha;
  struct array *pixels;
  struct array *colors;
  struct image *iimg, *ialpha;
  rgb_group *dst, *adst;
  INT_TYPE y,x,  bpc;

  get_all_args("_xpm_write_rows",args,"%o%o%i%a%a",
               &img,&alpha,&bpc,&colors,&pixels);

#if 0
  fprintf(stderr, "_xpm_write_rows(");
  print_svalue(stderr, Pike_sp-5);
  fprintf(stderr, ", ");
  print_svalue(stderr, Pike_sp-4);
  fprintf(stderr, ", ");
  print_svalue(stderr, Pike_sp-3);
  fprintf(stderr, ", ");
  print_svalue(stderr, Pike_sp-2);
  fprintf(stderr, ", ");
  print_svalue(stderr, Pike_sp-1);
  fprintf(stderr, ")\n");
#endif /* 0 */

  iimg = (struct image *)get_storage( img, image_program );
  ialpha = (struct image *)get_storage( alpha, image_program );
  if(!iimg || !ialpha)
    Pike_error("Sluta pilla på interna saker..\n");

  if (pixels->size < iimg->ysize + colors->size) {
    SIMPLE_ARG_ERROR("_xpm_write_rows", 5, "pixel array is too short.");
  }

  for(y = 0; y < iimg->ysize + colors->size + 1; y++) {
    if ((pixels->item[y].type != T_STRING) ||
	(pixels->item[y].u.string->size_shift)) {
      SIMPLE_ARG_ERROR("_xpm_write_rows", 5,
		       "Pixel array contains elements other than 8bit strings.");
    }
    if (y < colors->size) {
      if ((colors->item[y].type != T_STRING) ||
	  (pixels->item[y].u.string->size_shift)) {
	SIMPLE_ARG_ERROR("_xpm_write_rows", 5,
			 "Color array contains elements other than 8bit strings.");
      }
    } else if (y > colors->size) {
      if (pixels->item[y].u.string->len < iimg->xsize*bpc) {
	SIMPLE_ARG_ERROR("_xpm_write_rows", 5,
			 "Pixel array contains too short string (bad bpc?).");
      }
    }
  }

  dst = iimg->img;
  adst = ialpha->img;


  switch(bpc)
  {
   default:
    for(y = 0; y<iimg->ysize; y++)
    {
      char *ss = (char *)pixels->item[y+colors->size+1].u.string->str;
      for(x = 0; x<iimg->xsize; x++)
      {
        rgba_group color=qsearch(ss,bpc,colors);  ss+=bpc;
        if(color.alpha)
        {
          dst->r = color.r;
          dst->g = color.g;
          (dst++)->b = color.b;
          adst++;
        } else {
          dst++;
          adst->r = adst->g = adst->b = 0;
	  adst++;
        }
      }
    }
    break;
   case 3: 
   {
     rgba_group **p_colors;
     int i;
     p_colors = (rgba_group **)xalloc(sizeof(rgba_group *)*65536);
     MEMSET(p_colors, 0, sizeof(rgba_group *)*65536);
     for(i=0; i<colors->size; i++)
     {
       struct pike_string *c = colors->item[i].u.string;
       unsigned char ind = ((unsigned char *)(c->str))[2];
       unsigned short id = extract_short((unsigned char *)c->str);
       if(!p_colors[id])
       {
         p_colors[id] = (rgba_group *)xalloc(sizeof(rgba_group)*128);
         MEMSET(p_colors[id],0,sizeof(rgba_group)*128);
       }
       if(ind > 127) 
       {
         p_colors[id] = (rgba_group *)realloc(p_colors[id],sizeof(rgba_group)*256);
         MEMSET(p_colors[id]+sizeof(rgba_group)*128,0,sizeof(rgba_group)*128);
       }
       p_colors[id][ind]=parse_color_line( c, bpc );
     }
     for(y = 0; y<iimg->ysize; y++)
     {
       unsigned char *ss = (unsigned char *)pixels->item[y+colors->size+1].
         u.string->str;
       rgba_group *color, colorp;
       for(x = 0; x<iimg->xsize; x++)
       {
         color=p_colors[extract_short(ss)];
         if(color)
           colorp = color[((unsigned char *)ss+2)[0]];
         else
           colorp.alpha = 0;
         if(colorp.alpha)
         {
           dst->r = colorp.r;
           dst->g = colorp.g;
           (dst++)->b = colorp.b;
           adst++;
         } else {
           adst->r = adst->g = adst->b = 0;
           dst++;
         }
         ss+=bpc;
       }
     }
     for(i=0; i<65536; i++)
       if(p_colors[i]) free(p_colors[i]);
     free(p_colors);
     break;
   }
   case 2:
   {
     rgba_group p_colors[65536];
     int i;

     for(i=0; i<colors->size; i++)
     {
       unsigned short id = 
         extract_short((unsigned char*)colors->item[i].u.string->str);
       p_colors[id] = parse_color_line( colors->item[i].u.string, bpc );
     }
     for(y = 0; y<iimg->ysize; y++)
     {
       unsigned char *ss = (unsigned char *)pixels->item[y+colors->size+1].
         u.string->str;
       for(x = 0; x<iimg->xsize; x++)
       {
         rgba_group color=p_colors[extract_short(ss)];
         dst->r = color.r;
         dst->g = color.g;
         (dst++)->b = color.b;
         if(!color.alpha)
           adst->r = adst->g = adst->b = 0;
         ss+=bpc;
         adst++;
       }
     }
     break;
   }
   case 1:
   {
     rgba_group p_colors[256];
     int i;

     for(i=0; i<colors->size; i++)
     {
       unsigned char id = *((unsigned char *)colors->item[i].u.string->str);
       p_colors[id] = parse_color_line( colors->item[i].u.string, bpc );
     }
     for(y = 0; y<iimg->ysize; y++)
     {
       unsigned char *ss=(unsigned char *)
         pixels->item[y+colors->size+1].u.string->str;
       for(x = 0; x<iimg->xsize; x++)
       {
         rgba_group color=p_colors[*ss];
         dst->r = color.r;
         dst->g = color.g;
         (dst++)->b = color.b;
         if(!color.alpha)
           adst->r = adst->g = adst->b = 0;
         ss+=bpc;
         adst++;
       }
     }
     break;
   }
  }
  pop_n_elems(args);
  push_int(0);
}

/*! @decl array(string) _xpm_trim_rows(array(string) rows)
 */
void f__xpm_trim_rows( INT32 args )
{
  struct array *a;
  int i,j=0;
  get_all_args("_xpm_trim_rows", args, "%a", &a );
  for(i=0; i<a->size; i++)
  {
    int len,start;
    struct pike_string *s = a->item[i].u.string;
    if(a->item[i].type != T_STRING)
      Pike_error("Array must be array(string).\n");
    if(s->len > 4)
    {
      for(start=0; start<s->len; start++)
        if(s->str[start] == '/' || s->str[start] == '"')
          break;
      if(s->str[start] == '/')
        continue;
      for(len=start+1; len<s->len; len++)
        if(s->str[len] == '"')
          break;
      if(len>=s->len || s->str[len] != '"')
        continue;
      free_string(a->item[j].u.string);
      a->item[j++].u.string=make_shared_binary_string(s->str+start+1,len-start-1);
    }
  }
  pop_n_elems(args-1);
}

/*! @endmodule
 *!
 *! @endmodule
 */

void init_image__xpm( )
{
   ADD_FUNCTION( "_xpm_write_rows", f__xpm_write_rows,
		 tFunc(tObj tObj tInt tArr(tStr) tArr(tStr), tInt), 0);
   ADD_FUNCTION( "_xpm_trim_rows", f__xpm_trim_rows,
		 tFunc(tArr(tStr),tArr(tStr)), 0);
}

void exit_image__xpm(void)
{
}
