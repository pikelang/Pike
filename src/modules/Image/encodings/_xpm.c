#include "global.h"
RCSID("$Id: _xpm.c,v 1.1 1999/04/06 11:37:17 per Exp $");

#include "config.h"

#include "interpret.h"
#include "svalue.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "array.h"
#include "error.h"
#include "constants.h"
#include "mapping.h"
#include "stralloc.h"
#include "multiset.h"
#include "pike_types.h"
#include "rusage.h"
#include "operators.h"
#include "fsort.h"
#include "callback.h"
#include "gc.h"
#include "backend.h"
#include "main.h"
#include "pike_memory.h"
#include "threads.h"
#include "time_stuff.h"
#include "version.h"
#include "encode.h"
#include "module_support.h"
#include "module.h"
#include "opcodes.h"
#include "cyclic.h"
#include "signal_handler.h"
#include "security.h"


#include "image.h"
#include "colortable.h"

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

static rgba_group decode_color( struct pike_string *s )
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
  if(!parse_color)
  {
    push_text("Colors");
    push_int(0);
    SAFE_APPLY_MASTER( "resolv", 2 );
    if(IS_ZERO(sp-1)) error("Internal error: No Colors module!\n");
    push_text("parse_color");
    f_index(2);
    if(IS_ZERO(sp-1)) error("Internal error: No Colors.parse_color function!\n");
    _parse_color = sp[-1];
    parse_color = &_parse_color;
    sp--;
  }
  ref_push_string( s );
  apply_svalue( parse_color, 1 );
  if(sp[-1].type == T_ARRAY && sp[-1].u.array->size == 3)
  {
    res.r = sp[-1].u.array->item[0].u.integer;
    res.g = sp[-1].u.array->item[1].u.integer;
    res.b = sp[-1].u.array->item[2].u.integer;
  } else {
    res.r = res.g = res.b = 0;
  }
  pop_stack();
  return res;
}

static rgba_group qsearch( struct pike_string *s,
                          struct array *c )
{
  int start = c->size/2;
  int lower = 0;
  int upper = c->size-1;
  struct pike_string *cn;
  while( 1 )
  {
    int i, ok=1;
    cn = c->item[start].u.string;
    for(i=0; i<s->len; i++)
      if(cn->str[i] < s->str[i])
      {
        lower = start;
        start += (upper-start)/2;
        ok=0;
        break;
      } else if(cn->str[i] > s->str[i]) {
        upper = start;
        start -= (start-lower)/2;
        ok=0;
        break;
      }

    if(ok)
    {
      int toggle = 0;
      for(i=s->len; i<cn->len; i++)
      {
        switch(cn->str[i])
        {
         case ' ':
         case '\t':
           if(toggle)
           {
             rgba_group res;
             push_string(make_shared_binary_string(cn->str+i+1,cn->len-i-1));
             res=decode_color(sp[-1].u.string);
             pop_stack();
             return res;
           }
         default:
           toggle=1;
        }
      }
    }
    if(upper-lower < 2)
    {
      rgba_group res;
      res.r = res.g = res.b = 0;
      res.alpha = 0;
      return res;
    }
  }
}



static rgba_group mapsearch( struct pike_string *s,
                            struct mapping *in )
{
  struct svalue *res;
  if((res = low_mapping_string_lookup( in, s )))
  {
    switch(res->type)
    {
     case T_STRING:
       return decode_color(res->u.string);
     case T_MAPPING:
       if((res = simple_mapping_string_lookup( res->u.mapping, "c" ) ))
         return decode_color(res->u.string);
       if((res = simple_mapping_string_lookup( res->u.mapping, "g" ) ))
         return decode_color(res->u.string);
       if((res = simple_mapping_string_lookup( res->u.mapping, "g4" ) ))
         return decode_color(res->u.string);
       if((res = simple_mapping_string_lookup( res->u.mapping, "m" ) ))
         return decode_color(res->u.string);
    }
  }
  {
    rgba_group res;
    res.r=res.g=res.b=0;
    res.alpha = 255;
    return res;
  }
}

void f__xpm_write_row( INT32 args )
{
  struct object *img;
  struct object *alpha;
  struct array *pixels;
  struct svalue *colors;
  struct image *iimg, *ialpha;
  rgb_group *dst, *adst;
  int x, row;
  get_all_args( "_xpm_write_row", args, "%d%o%o%a%*", &row,
                &img, &alpha, &pixels,&colors);

  iimg = (struct image *)get_storage( img, image_program );
  ialpha = (struct image *)get_storage( alpha, image_program );
  if(!iimg || !ialpha)
    error("Sluta pilla på interna saker..\n");
  if(iimg->xsize != pixels->size ||
     ialpha->xsize != pixels->size)
    error("Sluta pilla på interna saker..\n");

  dst = iimg->img + (iimg->xsize * row);
  adst = ialpha->img + (ialpha->xsize * row);

  switch(colors->type)
  {
   case T_ARRAY:
     for(x = 0; x<pixels->size; x++)
     {
       rgba_group color = qsearch(pixels->item[x].u.string, colors->u.array);
       dst->r = color.r;
       dst->g = color.g;
       (dst++)->b = color.b;
       adst->r = adst->g = (adst++)->b = color.alpha;
     }
     break;
   case T_MAPPING:
     for(x = 0; x<pixels->size; x++)
     {
       rgba_group color = mapsearch(pixels->item[x].u.string, colors->u.mapping);
       dst->r = color.r;
       dst->g = color.g;
       dst->b = color.b;
       adst->r = adst->g = adst->b = color.alpha;
     }
     break;
  }
}

static struct program *image_encoding__xpm_program=NULL;
void init_image__xpm( )
{
  start_new_program();
  add_function( "_xpm_write_row", f__xpm_write_row, "function(mixed...:void)", 0);
  image_encoding__xpm_program=end_program();

  push_object(clone_object(image_encoding__xpm_program,0));
  {
    struct pike_string *s=make_shared_string("_XPM");
    add_constant(s,sp-1,0);
    free_string(s);
  }
}

void exit_image__xpm(void)
{
  if(image_encoding__xpm_program)
  {
    free_program(image_encoding__xpm_program);
    image_encoding__xpm_program=0;
  }
}
