/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/* TODO: use ONERROR to cleanup fsp */

/*
  Pike Sprintf v2.0 By Fredrik Hubinette (Profezzorn@nannymud)
  Should be reasonably compatible and somewhat faster than v1.05+ of Lynscar's
  sprintf. It requires the buffering function provided in dynamic_buffer.c
   Fail-safe memory-leak-protection is implemented through a stack that can
   be deallocated at any time. If something fails horribly this stack will be
  deallocated at next call of sprintf. Most operators doesn't need this
  feature though as they allocate their buffers with alloca() or simply use
  pointers into other strings.
  It also has a lot more features:

  Modifiers:
    0  Zero pad numbers (implies right justification)
    !  Toggle truncation
       pad positive integers with a space
    +  pad positive integers with a plus sign
    -  left adjusted within field size (default is right)
    |  centered within field size
    =  column mode if strings are greater than field size
    /  Rough linebreak (break at exactly fieldsize instead of between words)
    #  table mode, print a list of '\n' separated word (top-to-bottom order)
    $  Inverse table mode (left-to-right order)
    n  (where n is a number or *) a number specifies field size
   .n  set precision
   :n  set field size & precision
   ;n  Set column width
    *  if n is a * then next argument is used
    ~  get pad string from argument list
   'X'  Set a pad string. ' cannot be a part of the pad_string (yet)
    <  Use same arg again
    ^  repeat this on every line produced
    @  do this format for each entry in argument array
    > Put the string at the bottom end of column instead of top
    _ Set width to size of length of data

 Operators:
   %% percent
   %d signed decimal int
   %u unsigned decimal int
   %o unsigned octal int
   %x lowercase unsigned hexadecimal int
   %X uppercase unsigned hexadecimal int
   %c char
   %f float
   %g heuristically chosen representation of float
   %e exponential notation float
   %s string
   %O any type (prettyprint)
   %n nop
   %t type of argument
   %<modifiers>{format%}  do a format for every index in an array.

   Most flags and operators are combinable in any fashion, but _really_
   strange results can arise from things like:
      sprintf("%+080#{%s\n%}",get_dir("/"))

   Ideas yet to be implemented:
   Line-break with fill? Lower-case? Upper case? Capitalize?
   Replace? Justify on decimal point?
   Change european/american decimal notation?
   nroff-format? Crack root password and erase all disks?
   Print-optimize? (space to tab, strip trailing spaces)
   '>' Kill this field in all lines but the first one

   examples:

   A short 'people' function (without sort)

   sprintf("%{%-14s %2d %-30s %s\n%}\n",map_array(users(),lambda(object x)
       {
           return ({x->query_real_name(),
		x->query_level(),
		query_ip_name(x),
		file_name(environment(x))
	      });
       }))


   an 'ls'
   sprintf("%-#*{%s\n%}\n",width,get_dir(dir));

   debug-dump
   sprintf("%78*O",width,foo);

   A newspape
   sprintf("%-=*s %-=*s\n",width/2,article1,width/2,article2);

   A 'dotted-line' pricelist row 
   sprintf("%'.'-10s.%'.'4d\n",item,cost);

*/

#include "global.h"
RCSID("$Id: sprintf.c,v 1.32 2003/10/16 16:23:21 grubba Exp $");
#include "error.h"
#include "array.h"
#include "svalue.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "pike_types.h"
#include "constants.h"
#include "interpret.h"
#include "pike_memory.h"
#include "pike_macros.h"
#include <ctype.h>

#ifdef PC
#undef PC
#endif /* PC */

#include <math.h>
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_FP_CLASS_H
#include <fp_class.h>
#endif

#define FORMAT_INFO_STACK_SIZE 200
#define RETURN_SHARED_STRING

struct format_info
{
  char *fi_free_string;
  PCHARP b;
  int len;
  int width;
  int precision;
  PCHARP pad_string;
  int pad_length;
  int column_entries;
  short flags;
  char pos_pad;
  int column_width;
  int column_modulo;
};

static struct format_info format_info_stack[FORMAT_INFO_STACK_SIZE];
static struct format_info *fsp;

#define MINIMUM(X,Y) ((X)<(Y)?(X):(Y))

#define FIELD_LEFT 1
#define FIELD_CENTER 2
#define PAD_POSITIVE 4 
#define LINEBREAK 8
#define COLUMN_MODE 16
#define ZERO_PAD 32
#define ROUGH_LINEBREAK 64
#define DO_TRUNC 128
#define REPEAT 256
#define SNURKEL 512
#define INVERSE_COLUMN_MODE 1024
#define MULTI_LINE 2048
#define WIDTH_OF_DATA 4096
#define MULTI_LINE_BREAK 8192

#define MULTILINE (LINEBREAK | COLUMN_MODE | ROUGH_LINEBREAK | \
		   INVERSE_COLUMN_MODE | MULTI_LINE | REPEAT)


/* Generate binary IEEE strings on a machine which uses a different kind
   of floating point internally */

#ifndef FLOAT_IS_IEEE_BIG
#ifndef FLOAT_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#ifndef NEED_CUSTOM_IEEE
#ifndef DOUBLE_IS_IEEE_BIG
#ifndef DOUBLE_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#endif

#ifdef NEED_CUSTOM_IEEE

#ifndef HAVE_FPCLASS
#ifdef HAVE_FP_CLASS_D
#define fpclass fp_class_d
#define FP_NZERO FP_NEG_ZERO
#define FP_PZERO FP_POS_ZERO
#define FP_NINF FP_NEG_INF
#define FP_PINF FP_POS_INF
#define FP_NNORM FP_NEG_NORM
#define FP_PNORM FP_POS_NORM
#define FP_NDENORM FP_NEG_DENORM
#define FP_PDENORM FP_POS_DENORM
#define HAVE_FPCLASS
#endif
#endif

#ifdef HAVE_FREXP
#define FREXP frexp
#else
extern double FREXP(double x, int *exp); /* defined in encode.c */
#endif

#if HAVE_LDEXP
#define LDEXP ldexp
#else
extern double LDEXP(double x, int exp); /* defined in encode.c */
#endif

INLINE static void low_write_IEEE_float(char *b, double d, int sz)
{
  int maxexp;
  unsigned INT32 maxf;
  int s = 0, e = -1;
  unsigned INT32 f = 0, extra_f=0;

  if(sz==4) {
    maxexp = 255;
    maxf   = 0x7fffff;
  } else {
    maxexp = 2047;
    maxf   = 0x0fffff; /* This is just the high part of the mantissa... */
  }

#ifdef HAVE_FPCLASS
  switch(fpclass(d)) {
  case FP_SNAN:
    e = maxexp; f = 2; break;
  case FP_QNAN:
    e = maxexp; f = maxf; break;
  case FP_NINF:
    s = 1; /* FALLTHRU */
  case FP_PINF:
    e = maxexp; break;
  case FP_NZERO:
    s = 1; /* FALLTHRU */
  case FP_PZERO:
    e = 0; break;
  case FP_NNORM:
  case FP_NDENORM:
    s = 1; d = fabs(d); break;
  case FP_PNORM:
  case FP_PDENORM:
    break;
  default:
    if(d<0.0) {
      s = 1;
      d = fabs(d);
    }
    break;
  }
#else
#ifdef HAVE_ISINF
  if(isinf(d))
    e = maxexp;
  else
#endif
#ifdef HAVE_ISNAN
  if(isnan(d)) {
    e = maxexp; f = maxf;
  } else
#endif
#ifdef HAVE_ISZERO
  if(iszero(d))
    e = 0;
  else
#endif
#ifdef HAVE_FINITE
  if(!finite(d))
    e = maxexp;
#endif
  ; /* Terminate any remaining else */
#ifdef HAVE_SIGNBIT
  if((s = signbit(d)))
    d = fabs(d);
#else
  if(d<0.0) {
    s = 1;
    d = fabs(d);
  }
#endif
#endif

  if(e<0) {
    d = FREXP(d, &e);
    if(d == 1.0) {
      d=0.5;
      e++;
    }
    if(d == 0.0) {
      e = 0;
      f = 0;
    } else if(sz==4) {
      e += 126;
      d *= 16777216.0;
      if(e<=0) {
	d = LDEXP(d, e-1);
	e = 0;
      }
      f = ((INT32)floor(d))&maxf;
    } else {
      double d2;
      e += 1022;
      d *= 2097152.0;
      if(e<=0) {
	d = LDEXP(d, e-1);
	e = 0;
      }
      d2 = floor(d);
      f = ((INT32)d2)&maxf;
      d -= d2;
      d += 1.0;
      extra_f = (unsigned INT32)(floor(d * 4294967296.0)-4294967296.0);
    }

    if(e>=maxexp) {
      e = maxexp;
      f = extra_f = 0;
    }
  }

  if(sz==4) {
    b[0] = (s? 128:0)|((e&0xfe)>>1);
    b[1] = ((e&1)<<7)|((f&0x7f0000)>>16);
    b[2] = (f&0xff00)>>8;
    b[3] = f&0xff;
  } else {
    b[0] = (s? 128:0)|((e&0x7f0)>>4);
    b[1] = ((e&0xf)<<4)|((f&0x0f0000)>>16);
    b[2] = (f&0xff00)>>8;
    b[3] = f&0xff;
    b[4] = (extra_f&0xff000000)>>24;
    b[5] = (extra_f&0xff0000)>>16;
    b[6] = (extra_f&0xff00)>>8;
    b[7] = extra_f&0xff;
  }
}
#endif

/* Position a string inside a field with fill */

INLINE static void fix_field(struct string_builder *r,
			     PCHARP b,
			     int len,
			     int flags,
			     int width,
			     PCHARP pad_string,
			     int pad_length,
			     char pos_pad)
{
  int e,d;
  if(!width)
  {
    if(pos_pad && EXTRACT_PCHARP(b)!='-') string_builder_putchar(r,pos_pad);
    string_builder_append(r,b,len);
    return;
  }

  d=0;
  if(!(flags & DO_TRUNC) && len+(pos_pad && EXTRACT_PCHARP(b)!='-')>=width)
  {
    if(pos_pad && EXTRACT_PCHARP(b)!='-') string_builder_putchar(r,pos_pad);
    string_builder_append(r,b,len);
    return;
  }
  if(flags & ZERO_PAD)		/* zero pad is kind of special... */
  {
    if(EXTRACT_PCHARP(b)=='-')
    {
      string_builder_putchar(r,'-');
      INC_PCHARP(b,1);
      len--;
      width--;
    }else{
      if(pos_pad)
      {
        string_builder_putchar(r,pos_pad);
        width--;
      }
    }
#if 1
    string_builder_fill(r,width-len,MKPCHARP("0",0),1,0);
#else
    for(;width>len;width--) string_builder_putchar(r,'0');
#endif
    string_builder_append(r,b,len);
    return;
  }

  if(flags & FIELD_CENTER)
  {
    e=len;
    if(pos_pad && EXTRACT_PCHARP(b)!='-') e++;
    e=(width-e)/2;
    if(e>0)
    {
      string_builder_fill(r,e,pad_string, pad_length, 0);
      width-=e;
    }
    flags|=FIELD_LEFT;
  }

  if(flags & FIELD_LEFT)
  {
    if(pos_pad && EXTRACT_PCHARP(b)!='-')
    {
      string_builder_putchar(r,pos_pad);
      width--;
      d++;
    }

    d+=MINIMUM(width,len);
    while(len && width)
    {
      string_builder_putchar(r,EXTRACT_PCHARP(b));
      INC_PCHARP(b,1);
      len--;
      width--;
    }

    if(width>0)
    {
      d%=pad_length;
      string_builder_fill(r,width,pad_string,pad_length,d);
    }
    
    return;
  }

  /* Right-justification */

  if(pos_pad && EXTRACT_PCHARP(b)!='-') len++;
  e=width-len;
  if(e>0)
  {
    string_builder_fill(r,e,pad_string, pad_length, 0);
    width-=e;
  }

  if(pos_pad && EXTRACT_PCHARP(b)!='-' && len==width)
  {
    string_builder_putchar(r,pos_pad);
    len--;
    width--;
  }
  INC_PCHARP(b,len-width);
  string_builder_append(r,b,width);
}

static struct svalue temp_svalue = { T_INT };

static void free_sprintf_strings(void)
{
  free_svalue(&temp_svalue);
  temp_svalue.type=T_INT;
  for(;fsp>=format_info_stack;fsp--)
  {
    if(fsp->fi_free_string)
      free(fsp->fi_free_string);
    fsp->fi_free_string=0;
  }
}

static void sprintf_error(char *s,...) ATTRIBUTE((noreturn,format (printf, 1, 2)));
static void sprintf_error(char *s,...)
{
  char buf[100];
  va_list args;
  va_start(args,s);
  free_sprintf_strings();

  sprintf(buf,"Sprintf: %s",s);
  va_error(buf,args);
  va_end(args);
}

/* This is called once for every '%' on every ouputted line
 * it takes care of linebrak and column mode. It returns 1
 * if there is more for next line.
 */

INLINE static int do_one(struct string_builder *r,
			 struct format_info *f)
{
  PCHARP rest;
  int e,d,lastspace;

  rest.ptr=0;
  if(f->flags & (LINEBREAK|ROUGH_LINEBREAK))
  {
    if(!f->width)
      sprintf_error("Must have field width for linebreak.\n");
    lastspace=-1;
    for(e=0;e<f->len && e<=f->width;e++)
    {
      switch(INDEX_PCHARP(f->b,e))
      {
	case '\n':
	  lastspace=e;
	  rest=ADD_PCHARP(f->b,e+1);
	  break;

	case ' ':
	  if(f->flags & LINEBREAK)
	  {
	    lastspace=e;
	    rest=ADD_PCHARP(f->b,e+1);
	  }
	default:
	  continue;
      }
      break;
    }
    if(e==f->len && f->len<=f->width)
    {
      lastspace=e;
      rest=ADD_PCHARP(f->b,lastspace);
    }else if(lastspace==-1){
      lastspace=MINIMUM(f->width,f->len);
      rest=ADD_PCHARP(f->b,lastspace);
    }
    fix_field(r,
	      f->b,
	      lastspace,
	      f->flags,
	      f->width,
	      f->pad_string,
	      f->pad_length,
	      f->pos_pad);
  }
  else if(f->flags & INVERSE_COLUMN_MODE)
  {
    if(!f->width)
      sprintf_error("Must have field width for column mode.\n");
    e=f->width/(f->column_width+1);
    if(!f->column_width || e<1) e=1;

    rest=f->b;
    for(d=0;INDEX_PCHARP(rest,d) && e;d++)
    {
#if 0
      if(rest != f->b)
	fix_field(" ",1,0,1," ",1,0);
#endif

      while(INDEX_PCHARP(rest,d) && INDEX_PCHARP(rest,d)!='\n')
	d++;

      fix_field(r,
		rest,
		d,
		f->flags,
		f->column_width,
		f->pad_string,
		f->pad_length,
		f->pos_pad);

      e--;
      INC_PCHARP(rest,d);
      d=-1;
      if(EXTRACT_PCHARP(rest)) INC_PCHARP(rest,1);
    }
  }
  else if(f->flags & COLUMN_MODE)
  {
    int mod,col;
    PCHARP end;

    if(!f->width)
      sprintf_error("Must have field width for column mode.\n");
    mod=f->column_modulo;
    col=f->width/(f->column_width+1);
    if(!f->column_width || col<1) col=1;
    rest=f->b;
    end=ADD_PCHARP(rest,f->len);

    for(d=0;rest.ptr && d<col;d++)
    {
#if 0
      if(rest != f->b)
	fix_field(" ",1,0,1," ",1,0);
#endif

      /* Find end of entry */
      for(e=0;COMPARE_PCHARP(ADD_PCHARP(rest, e),<,end) &&
	    INDEX_PCHARP(rest,e)!='\n';e++);

      fix_field(r,rest,e,f->flags,f->column_width,
		f->pad_string,f->pad_length,f->pos_pad);

      f->column_entries--;

      /* Advance to after entry */
      INC_PCHARP(rest,e);
      if(!COMPARE_PCHARP(rest,<,end)) break;
      INC_PCHARP(rest,1);

      for(e=1;e<mod;e++)
      {
	PCHARP s=MEMCHR_PCHARP(rest,'\n',SUBTRACT_PCHARP(end,rest));
	if(s.ptr)
	{
	  rest=ADD_PCHARP(s,1);
	}else{
	  rest.ptr=0;
	  break;
	}
      }
    }
    if(f->column_entries>0)
    {
      for(rest=f->b;COMPARE_PCHARP(rest,<,end) &&
	    EXTRACT_PCHARP(rest)!='\n';INC_PCHARP(rest,1));
      if(COMPARE_PCHARP(rest,<,end)) INC_PCHARP(rest,1);
    }else{
      rest.ptr=0;
    }
  }
  else
  {
    fix_field(r,f->b,f->len,f->flags,f->width,f->pad_string,f->pad_length,f->pos_pad);
  }

  if(f->flags & REPEAT) return 0;
  if(rest.ptr)
  {
    f->len-=SUBTRACT_PCHARP(rest,f->b);
    f->b=rest;
  }else{
    f->len=0;
    f->b=MKPCHARP("",0);
  }
  return f->len>0;
}


#define GET_SVALUE(VAR) \
  if(arg) \
  { \
    VAR=arg; \
    arg=0; \
  }else{ \
    if(argument >= num_arg) \
    { \
      sprintf_error("Too few arguments to sprintf.\n"); \
      break; /* make gcc happy */ \
    } \
    VAR=lastarg=argp+(argument++); \
  }

#define GET(VAR,PIKE_TYPE,TYPE_NAME,EXTENSION) \
  { \
    struct svalue *tmp_; \
    GET_SVALUE(tmp_); \
    if(tmp_->type!=PIKE_TYPE) \
    { \
      sprintf_error("Wrong type for argument %d: expected %s, got %s.\n",argument+1,TYPE_NAME, \
	get_name_of_type(tmp_->type)); \
      break; /* make gcc happy */ \
    } \
    VAR=tmp_->u.EXTENSION; \
  }

#define GET_INT(VAR) GET(VAR,T_INT,"integer",integer)
#define GET_STRING(VAR) GET(VAR,T_STRING,"string",string)
#define GET_FLOAT(VAR) GET(VAR,T_FLOAT,"float",float_number)
#define GET_ARRAY(VAR) GET(VAR,T_ARRAY,"array",array)
#define GET_OBJECT(VAR) GET(VAR,T_OBJECT,"object",object)

#define DO_OP() \
   if(fsp->flags & SNURKEL) \
   { \
     ONERROR _e; \
     struct array *_v; \
     struct string_builder _b; \
     init_string_builder(&_b,0); \
     SET_ONERROR(_e, free_string_builder, &_b); \
     GET_ARRAY(_v); \
     for(tmp=0;tmp<_v->size;tmp++) \
     { \
       struct svalue *save_sp=sp; \
       array_index_no_free(sp,_v,tmp); \
       sp++; \
       low_pike_sprintf(&_b,begin,SUBTRACT_PCHARP(a,begin)+1,sp-1,1,nosnurkel+1); \
       if(save_sp < sp) pop_stack(); \
     } \
     fsp->b=MKPCHARP_STR(_b.s); \
     fsp->len=_b.s->len; \
     fsp->fi_free_string=(char *)_b.s; \
     fsp->pad_string=MKPCHARP(" ",0); \
     fsp->pad_length=1; \
     fsp->column_width=0; \
     fsp->pos_pad=fsp->flags=fsp->width=fsp->precision=0; \
     UNSET_ONERROR(_e); \
     break; \
   }


/* This is the main pike_sprintf function, note that it calls itself
 * recursively during the '%{ %}' parsing. The string is stored in
 * the buffer in save_objectII.c
 */

static void low_pike_sprintf(struct string_builder *r,
			     PCHARP format,
			     int format_len,
			     struct svalue *argp,
			     int num_arg,
			     int nosnurkel)
{
  int argument=0;
  int tmp,setwhat,pos,d,e;
  char buffer[140];
  struct format_info *f,*start;
  float tf;
  struct svalue *arg=0;	/* pushback argument */
  struct svalue *lastarg=0;

  PCHARP a,begin;
  PCHARP format_end=ADD_PCHARP(format,format_len);

  start=fsp;
  for(a=format;COMPARE_PCHARP(a,<,format_end);INC_PCHARP(a,1))
  {
    int num_snurkel;

    fsp++;
#ifdef PIKE_DEBUG
    if(fsp < format_info_stack)
      fatal("sprintf: fsp out of bounds.\n");
#endif
    if(fsp-format_info_stack==FORMAT_INFO_STACK_SIZE)
      sprintf_error("Sprintf stack overflow.\n");
    fsp->pad_string=MKPCHARP(" ",0);
    fsp->pad_length=1;
    fsp->fi_free_string=0;
    fsp->column_width=0;
    fsp->pos_pad=fsp->flags=fsp->width=fsp->precision=0;

    if(EXTRACT_PCHARP(a)!='%')
    {
      for(e=0;INDEX_PCHARP(a,e)!='%' &&
	    COMPARE_PCHARP(ADD_PCHARP(a,e),<,format_end);e++);
      fsp->b=a;
      fsp->len=e;
      fsp->width=e;
      INC_PCHARP(a,e-1);
      continue;
    }
    num_snurkel=0;
    arg=NULL;
    setwhat=pos=0;
    begin=a;

    for(INC_PCHARP(a,1);;INC_PCHARP(a,1))
    {
/*      fprintf(stderr,"sprintf-flop: %d (%c)\n",EXTRACT_PCHARP(a),EXTRACT_PCHARP(a)); */
      switch(EXTRACT_PCHARP(a))
      {
      default:
	if(EXTRACT_PCHARP(a) < 256 && 
	   isprint(EXTRACT_PCHARP(a)))
	{
	  sprintf_error("Error in format string, %c is not a format.\n",EXTRACT_PCHARP(a));
	}else{
	  sprintf_error("Error in format string, \\%o is not a format.\n",EXTRACT_PCHARP(a));
	}
	fatal("Foo, you shouldn't be here!\n");

        /* First the modifiers */

      case '0': fsp->flags|=ZERO_PAD; continue;
      case '1': case '2': case '3':
      case '4': case '5': case '6':
      case '7': case '8': case '9':
	tmp=STRTOL_PCHARP(a,&a,10);
	INC_PCHARP(a,-1);
	goto got_arg;

      case '*':
	GET_INT(tmp);

      got_arg:
	if(tmp<1)
	  sprintf_error("Illegal width.\n");
	switch(setwhat)
	{
	case 0: fsp->width=tmp; break;
	case 1: fsp->width=tmp;
	case 2: fsp->precision=tmp; break;
	case 3: fsp->column_width=tmp; break;
	}
	continue;

      case ';': setwhat=3; continue;
      case '.': setwhat=2; continue;
      case ':': setwhat=1; continue;

      case '=': fsp->flags|=LINEBREAK; continue;
      case '/': fsp->flags|=ROUGH_LINEBREAK; continue;
      case '#': fsp->flags|=COLUMN_MODE; continue;
      case '$': fsp->flags|=INVERSE_COLUMN_MODE; continue;

      case '-': fsp->flags|=FIELD_LEFT; continue;
      case '|': fsp->flags|=FIELD_CENTER; continue;
      case ' ': fsp->pos_pad=' '; continue;
      case '+': fsp->pos_pad='+'; continue;
      case '!': fsp->flags^=DO_TRUNC; continue;
      case '^': fsp->flags|=REPEAT; continue;
      case '>': fsp->flags|=MULTI_LINE_BREAK; continue;
      case '_': fsp->flags|=WIDTH_OF_DATA; continue;
      case '@':
	if(++num_snurkel > nosnurkel)
	  fsp->flags|=SNURKEL;
	continue;

      case '\'':
	tmp=0;
	for(INC_PCHARP(a,1);INDEX_PCHARP(a,tmp)!='\'';tmp++)
	{
/*	  fprintf(stderr,"Sprinf-glop: %d (%c)\n",INDEX_PCHARP(a,tmp),INDEX_PCHARP(a,tmp)); */
	  if(COMPARE_PCHARP(a,>=,format_end))
	    sprintf_error("Unfinished pad string in format string.\n");
	}
	if(tmp)
	{
	  fsp->pad_string=a;
	  fsp->pad_length=tmp;
	}
	INC_PCHARP(a,tmp);
	continue;

      case '~':
      {
	struct pike_string *s;
	GET_STRING(s);
	if (s->len) {
	  fs->fsp->pad_string=MKPCHARP_STR(s);
	  fs->fsp->pad_length=s->len;
	}
	continue;
      }

      case '<':
	if(!lastarg)
	  sprintf_error("No last argument.\n");
	arg=lastarg;
	continue;

        /* now the real operators */

      case '{':
      {
	struct array *w;
	struct string_builder b;
#ifdef PIKE_DEBUG
	struct format_info *fsp_save=fsp;
#endif
	DO_OP();
	for(e=1,tmp=1;tmp;e++)
	{
	  if(!INDEX_PCHARP(a,e))
	  {
	    sprintf_error("Missing %%} in format string.\n");
	    break;		/* UNREACHED */
	  }
	  if(INDEX_PCHARP(a,e)=='%')
	  {
	    switch(INDEX_PCHARP(a,e+1))
	    {
	    case '%': e++; break;
	    case '}': tmp--; break;
	    case '{': tmp++; break;
	    }
	  }
	}
            
	GET_ARRAY(w);
	if(!w->size)
	{
	  fsp->b=MKPCHARP("",0);
	  fsp->len=0;
	}else{
	  ONERROR err;
	  init_string_builder(&b,0);
	  SET_ONERROR(err,free_string_builder,&b);
	  for(tmp=0;tmp<w->size;tmp++)
	  {
	    struct svalue *s;
	    union anything *q;

/*	    check_threads_etc(); */
	    q=low_array_get_item_ptr(w,tmp,T_ARRAY);
	    s=sp;
	    if(q)
	    {
	      q->array->refs++;
	      push_array_items(q->array);
	    }else{
	      array_index_no_free(sp,w,tmp);
	      sp++;
	    }
	    low_pike_sprintf(&b,ADD_PCHARP(a,1),e-2,s,sp-s,0);
	    pop_n_elems(sp-s);
	  }
#ifdef PIKE_DEBUG
	  if(fsp < format_info_stack)
	    fatal("sprintf: fsp out of bounds.\n");
	  if(fsp!=fsp_save)
	    fatal("sprintf: fsp incorrect after recursive sprintf.\n");
#endif
	  fsp->b=MKPCHARP_STR(b.s);
	  fsp->len=b.s->len;
	  fsp->fi_free_string=(char *)b.s;
	  UNSET_ONERROR(err);
	}
	
	INC_PCHARP(a,e);
	break;
      }

      case '%':
	fsp->b=MKPCHARP("%",0);
	fsp->len=fsp->width=1;
	break;

      case 'n':
	DO_OP();
	fsp->b=MKPCHARP("",0);
	fsp->len=0;
	break;

      case 't':
      {
	struct svalue *t;
	DO_OP();
	GET_SVALUE(t);
	fsp->b=MKPCHARP(get_name_of_type(t->type),0);
	fsp->len=strlen((char *)fsp->b.ptr);
	break;
      }

      case 'c':
      {
        INT32 l,tmp;
	char *x;
        DO_OP();
        l=1;
        if(fsp->width > 0) l=fsp->width;
	x=(char *)alloca(l);
	fsp->b=MKPCHARP(x,0);
	fsp->len=l;
	GET_INT(tmp);
        while(--l>=0)
        {
          x[l]=tmp & 0xff;
          tmp>>=8;
        }
	break;
      }

      case 'o':
      case 'd':
      case 'u':
      case 'x':
      case 'X':
      {
	char *x;
	DO_OP();
	GET_INT(tmp);
	buffer[0]='%';
	buffer[1]=EXTRACT_PCHARP(a);
	buffer[2]=0;
	x=(char *)alloca(100);
	fsp->b=MKPCHARP(x,0);
	sprintf(x,buffer,tmp);
	fsp->len=strlen(x);
	break;
      }

      case 'e':
      case 'f':
      case 'g':
      case 'E':
      case 'G':
      {
	char *x;
	DO_OP();
	x=(char *)xalloc(100+MAXIMUM(fsp->width,8)+
			      MAXIMUM(fsp->precision,3));
	fsp->b=MKPCHARP(x,0);
	sprintf(buffer,"%%*.*%c",EXTRACT_PCHARP(a));
	GET_FLOAT(tf);
	sprintf(x,buffer,
		fsp->width?fsp->width:8,
		fsp->precision?fsp->precision:3,tf);
	fsp->len=strlen(x);
	fsp->fi_free_string=x;
	break;
      }

      case 'F':
      {
        INT32 l;
	char *x;
#if defined(DOUBLE_IS_IEEE_LITTLE) || defined(DOUBLE_IS_IEEE_BIG)
	double td;
#endif
        DO_OP();
        l=4;
        if(fsp->width > 0) l=fsp->width;
	if(l != 4 && l != 8)
	  sprintf_error("Invalid IEEE width %d.\n", l);
	x=(char *)alloca(l);
	fsp->b=MKPCHARP(x,0);
	fsp->len=l;
	GET_FLOAT(tf);
	switch(l) {
	case 4:
#ifdef FLOAT_IS_IEEE_BIG
	  MEMCPY(x, &tf, 4);
#else
#ifdef FLOAT_IS_IEEE_LITTLE
	  x[0] = ((char *)&tf)[3];
	  x[1] = ((char *)&tf)[2];
	  x[2] = ((char *)&tf)[1];
	  x[3] = ((char *)&tf)[0];
#else
	  low_write_IEEE_float(x, (double)tf, 4);
#endif
#endif
	  break;
	case 8:
#ifdef DOUBLE_IS_IEEE_BIG
	  td = (double)tf;
	  MEMCPY(x, &td, 8);
#else
#ifdef DOUBLE_IS_IEEE_LITTLE
	  td = (double)tf;

	  x[0] = ((char *)&td)[7];
	  x[1] = ((char *)&td)[6];
	  x[2] = ((char *)&td)[5];
	  x[3] = ((char *)&td)[4];
	  x[4] = ((char *)&td)[3];
	  x[5] = ((char *)&td)[2];
	  x[6] = ((char *)&td)[1];
	  x[7] = ((char *)&td)[0];
#else
	  low_write_IEEE_float(x, (double)tf, 8);
#endif
#endif
	}
	break;
      }

      case 'O':
      {
	string s;
	struct svalue *t;
	DO_OP();
	GET_SVALUE(t);
	init_buf();
	describe_svalue(t,0,0);
	s=complex_free_buf();
	fsp->b=MKPCHARP(s.str,0);
	fsp->len=s.len;
	fsp->fi_free_string=s.str;
	break;
      }

      case 's':
      {
	struct pike_string *s;
	DO_OP();
	GET_STRING(s);
	fsp->b=MKPCHARP_STR(s);
	fsp->len=s->len;
	break;
      }
      }
      break;
    }
  }

  for(f=fsp;f>start;f--)
  {
    if(f->flags & WIDTH_OF_DATA) f->width=f->len;

    if(((f->flags & INVERSE_COLUMN_MODE) && !f->column_width) ||
       (f->flags & COLUMN_MODE))
    {
      int max_len,nr,columns;
      tmp=1;
      for(max_len=nr=e=0;e<f->len;e++)
      {
	if(INDEX_PCHARP(f->b,e)=='\n')
	{
	  nr++;
	  if(max_len<tmp) max_len=tmp;
	  tmp=0;
	}
	tmp++;
      }
      nr++;
      if(max_len<tmp) max_len=tmp;
      if(!f->column_width) f->column_width=max_len;
      f->column_entries=nr;
      columns=f->width/(f->column_width+1);
      if(f->column_width<1 || columns<1) columns=1;
      f->column_modulo=(nr+columns-1)/columns;
    }
  }

  /* Here we do some DWIM */
  for(f=fsp-1;f>start;f--)
  {
    if((f[1].flags & MULTILINE) &&
       !(f[0].flags & (MULTILINE|MULTI_LINE_BREAK)))
    {
      if(! MEMCHR_PCHARP(f->b, '\n', f->len).ptr ) f->flags|=MULTI_LINE;
    }
  }
  for(f=start+1;f<=fsp;)
  {
    for(;f<=fsp && !(f->flags&MULTILINE);f++) do_one(r,f);
    do
    {
      d=0;
      for(e=0;f+e<=fsp && (f[e].flags & MULTILINE);e++) d|=do_one(r,f+e);
      if(d) string_builder_putchar(r,'\n');
    }while(d);

    for(;f<=fsp && (f->flags&MULTILINE); f++);
  }

  while(fsp>start)
  {
#ifdef PIKE_DEBUG
    if(fsp < format_info_stack)
      fatal("sprintf: fsp out of bounds.\n");
#endif
    if(fsp->fi_free_string) free(fsp->fi_free_string);
    fsp->fi_free_string=0;
    fsp--;
  }
}


/* An C-callable pike_sprintf
string pike_sprintf(char *format,struct svalue *argp,int num_arg)
{
  string prefix;
  prefix.str=0;
  prefix.len=0;

  free_sprintf_strings();
  fsp=format_info_stack-1;
  return low_pike_sprintf(format,strlen(format),argp,num_arg,prefix,0);
}
 */

/* The efun */
void f_sprintf(INT32 num_arg)
{
  ONERROR tmp;
  struct pike_string *ret;
  struct svalue *argp;
  struct string_builder r;

  argp=sp-num_arg;
  free_sprintf_strings();
  fsp=format_info_stack-1;

  if(argp[0].type != T_STRING)
    error("Bad argument 1 to sprintf.\n");

  init_string_builder(&r,0);
  SET_ONERROR(tmp, free_string_builder, &r);
  low_pike_sprintf(&r,
		   MKPCHARP_STR(argp->u.string),
		   argp->u.string->len,
		   argp+1,
		   num_arg-1,
		   0);
  UNSET_ONERROR(tmp);
  ret=finish_string_builder(&r);

  free_svalue(&temp_svalue);
  temp_svalue.type=T_INT;
  pop_n_elems(num_arg);
  push_string(ret);
}

void pike_module_init(void)
{
  add_efun("sprintf", f_sprintf,"function(string, mixed ... : string)",
	   OPT_TRY_OPTIMIZE);
}

void pike_module_exit(void)
{
  free_sprintf_strings();
}
