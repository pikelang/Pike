/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sprintf.c,v 1.123 2004/11/16 16:20:44 nilsson Exp $
*/

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
  It also has a lot more features.

 Ideas yet to be implemented:
   Line-break with fill? Lower-case? Upper case? Capitalize?
   Replace? Justify on decimal point?
   Change european/american decimal notation?
   nroff-format? Crack root password and erase all disks?
   Print-optimize? (space to tab, strip trailing spaces)
   '>' Kill this field in all lines but the first one

 Examples:

   A short 'people' function (without sort)

   sprintf("%{%-14s %2d %-30s %s\n%}\n",map(users(),lambda(object x)
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

   A newspaper
   sprintf("%-=*s %-=*s\n",width/2,article1,width/2,article2);

   A 'dotted-line' pricelist row 
   sprintf("%'.'-10s.%'.'4d\n",item,cost);

*/

/*! @decl string sprintf(string format, mixed ... args)
 *!
 *!   Print formated output to string.
 *!
 *!   The @[format] string is a string containing a description of how to
 *!   output the data in @[args]. This string should generally speaking
 *!   have one @tt{%@i{<modifiers>@}@i{<operator>@}@} format specifier
 *!   (examples: @tt{%s@}, @tt{%0d@}, @tt{%-=20s@}) for each of the arguments.
 *!
 *!   The following modifiers are supported:
 *!   @int
 *!     @value '0'
 *!       Zero pad numbers (implies right justification).
 *!     @value '!'
 *!       Toggle truncation.
 *!     @value ' '
 *!       Pad positive integers with a space.
 *!     @value '+'
 *!       Pad positive integers with a plus sign.
 *!     @value '-'
 *!       Left adjust within field size (default is right).
 *!     @value '|'
 *!       Centered within field size.
 *!     @value '='
 *!       Column mode if strings are greater than field size.
 *!     @value '/'
 *!       Rough line break (break at exactly field size instead of between
 *!       words).
 *!     @value '#'
 *!       Table mode, print a list of '\n' separated word (top-to-bottom
 *!       order).
 *!     @value '$'
 *!       Inverse table mode (left-to-right order).
 *!     @value 'n'
 *!       (Where n is a number or *) field size specifier.
 *!     @value '.n'
 *!       Precision specifier.
 *!     @value ':n'
 *!       Field size precision specifier.
 *!     @value ';n'
 *!       Column width specifier.
 *!     @value '*'
 *!       If n is a * then next argument is used for precision/field size.
 *!     @value "'"
 *!       Set a pad string. @tt{'@} cannot be a part of the pad string (yet).
 *!     @value '~'
 *!       Get pad string from argument list.
 *!     @value '<'
 *!       Use same argument again.
 *!     @value '^'
 *!       Repeat this on every line produced.
 *!     @value '@'
 *!       Repeat this format for each element in the argument array.
 *!     @value '>'
 *!       Put the string at the bottom end of column instead of top.
 *!     @value '_'
 *!       Set width to the length of data.
 *!     @value '[n]'
 *!       Select argument number @tt{@i{n@}@}. Use @tt{*@} to use the next
 *!       argument as selector.
 *!   @endint
 *!
 *!   The following operators are supported:
 *!   @int
 *!     @value '%'
 *!       Percent.
 *!     @value 'b'
 *!       Signed binary integer.
 *!     @value 'd'
 *!       Signed decimal integer.
 *!     @value 'u'
 *!       Unsigned decimal integer.
 *!     @value 'o'
 *!       Signed octal integer.
 *!     @value 'x'
 *!       Lowercase signed hexadecimal integer.
 *!     @value 'X'
 *!       Uppercase signed hexadecimal integer.
 *!     @value 'c'
 *!       Character. If a fieldsize has been specified this will output
 *!       the low-order bytes of the integer in network (big endian) byte
 *!       order. To get little endian byte order, negate the field size.
 *!     @value 'f'
 *!       Float. (Locale dependent formatting.)
 *!     @value 'g'
 *!       Heuristically chosen representation of float.
 *!       (Locale dependent formatting.)
 *!     @value 'G'
 *!       Like @tt{%g@}, but uses uppercase @tt{E@} for exponent.
 *!     @value 'e'
 *!       Exponential notation float. (Locale dependent output.)
 *!     @value 'E'
 *!       Like @tt{%e@}, but uses uppercase @tt{E@} for exponent.
 *!     @value 'F'
 *!       Binary IEEE representation of float (@tt{%4F@} gives 
 *!       single precision, @tt{%8F@} gives double precision.)
 *!     @value 's'
 *!       String.
 *!     @value 'O'
 *!       Any value, debug style. Do not rely on the exact formatting;
 *!       how the result looks can vary depending on locale, phase of
 *!       the moon or anything else the _sprintf method implementor
 *!       wanted for debugging.
 *!     @value 'n'
 *!       No operation (ignore the argument).
 *!     @value 't'
 *!       Type of the argument.
 *!     @value '{'
 *!     @value '}'
 *!       Perform the enclosed format for every element of the argument array.
 *!   @endint
 *!
 *!   Most modifiers and operators are combinable in any fashion, but some
 *!   combinations may render strange results.
 *!
 *!   If an argument is an object that implements @[lfun::_sprintf()], that
 *!   callback will be called with the operator as the first argument, and
 *!   the current modifiers as the second. The callback is expected to return
 *!   a string.
 *!
 *! @example
 *! Pike v7.4 release 13 running Hilfe v3.5 (Incremental Pike Frontend)
 *! > sprintf("The unicode character %c has character code %04X.", 'A', 'A');
 *! (1) Result: "The unicode character A has character code 0041."
 *! > sprintf("#%@@02X is the HTML code for purple.", Image.Color.purple->rgb());
 *! (2) Result: "#A020F0 is the HTML code for purple."
 *! > int n=4711;
 *! > sprintf("%d = hexadecimal %x = octal %o = %b binary", n, n, n, n);
 *! (3) Result: "4711 = hexadecimal 1267 = octal 11147 = 1001001100111 binary"
 *!
 *! @note
 *!   sprintf style formatting is applied by many formatting functions, such
 *!   @[write()] and @[werror].
 *!
 *! @example
 *! > write(#"Formatting examples:
 *! Left adjusted  [%-10d]
 *! Centered       [%|10d]
 *! Right adjusted [%10d]
 *! Zero padded    [%010d]
 *! ", n, n, n, n);
 *! Formatting examples:
 *! Left adjusted  [4711      ]
 *! Centered       [   4711   ]
 *! Right adjusted [      4711]
 *! Zero padded    [0000004711]
 *! (5) Result: 142
 *! int screen_width=70;
 *! > write("%-=*s\n", screen_width,
 *! >> "This will wordwrap the specified string within the "+
 *! >> "specified field size, this is useful say, if you let "+
 *! >> "users specify their screen size, then the room "+
 *! >> "descriptions will automagically word-wrap as appropriate.\n"+
 *! >> "slosh-n's will of course force a new-line when needed.\n");
 *! This will wordwrap the specified string within the specified field
 *! size, this is useful say, if you let users specify their screen size,
 *! then the room descriptions will automagically word-wrap as
 *! appropriate.
 *! slosh-n's will of course force a new-line when needed.
 *! (6) Result: 355
 *! > write("%-=*s %-=*s\n", screen_width/2,
 *! >> "Two columns next to each other (any number of columns will "+
 *! >> "of course work) independantly word-wrapped, can be useful.",
 *! >> screen_width/2-1,
 *! >> "The - is to specify justification, this is in addherence "+
 *! >> "to std sprintf which defaults to right-justification, "+
 *! >> "this version also supports centre and right justification.");
 *! Two columns next to each other (any The - is to specify justification,
 *! number of columns will of course    this is in addherence to std
 *! work) independantly word-wrapped,   sprintf which defaults to
 *! can be useful.                      right-justification, this version
 *!                                     also supports centre and right
 *!                                     justification.
 *! (7) Result: 426
 *! > write("%-$*s\n", screen_width,
 *! >> "Given a\nlist of\nslosh-n\nseparated\n'words',\nthis option\n"+
 *! >> "creates a\ntable out\nof them\nthe number of\ncolumns\n"+
 *! >> "be forced\nby specifying a\npresision.\nThe most obvious\n"+
 *! >> "use is for\nformatted\nls output.");
 *! Given a          list of          slosh-n
 *! separated        'words',         this option
 *! creates a        table out        of them
 *! the number of    columns          be forced
 *! by specifying a  presision.       The most obvious
 *! use is for       formatted        ls output.
 *! (8) Result: 312
 *! > write("%-#*s\n", screen_width,
 *! >> "Given a\nlist of\nslosh-n\nseparated\n'words',\nthis option\n"+
 *! >> "creates a\ntable out\nof them\nthe number of\ncolumns\n"+
 *! >> "be forced\nby specifying a\npresision.\nThe most obvious\n"+
 *! >> "use is for\nformatted\nls output.");
 *! Given a          creates a        by specifying a
 *! list of          table out        presision.
 *! slosh-n          of them          The most obvious
 *! separated        the number of    use is for
 *! 'words',         columns          formatted
 *! this option      be forced        ls output.
 *! (9) Result: 312
 *! > sample = ([ "align":"left", "valign":"middle" ]);
 *! (10) Result: ([ @xml{/@}* 2 elements *@xml{/@}
 *!          "align":"left",
 *!          "valign":"middle"
 *!        ])
 *! > write("<td%{ %s='%s'%}>\n", (array)sample);
 *! <td valign='middle' align='left'>
 *! (11) Result: 34
 *! >  write("Of course all the simple printf options "+
 *! >> "are supported:\n %s: %d %x %o %c\n",
 *! >> "65 as decimal, hex, octal and a char",
 *! >> 65, 65, 65, 65);
 *! Of course all the simple printf options are supported:
 *!  65 as decimal, hex, octal and a char: 65 41 101 A
 *! (12) Result: 106
 *! > write("%[0]d, %[0]x, %[0]X, %[0]o, %[0]c\n", 75);
 *! 75, 4b, 4B, 113, K
 *! (13) Result: 19
 *! > write("%|*s\n",screen_width, "THE END");
 *!                                THE END
 *! (14) Result: 71
 *!
 *! @seealso
 *!   @[lfun::_sprintf()]
 */
#include "global.h"
#include "pike_error.h"
#include "array.h"
#include "svalue.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "pike_types.h"
#include "constants.h"
#include "interpret.h"
#include "pike_memory.h"
#include "pike_macros.h"
#include "object.h"
#include "bignum.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "operators.h"
#include "opcodes.h"
#include "cyclic.h"
#include "module.h"
#include "pike_float.h"
#include <ctype.h>

#include "config.h"

#include <math.h>

#define FORMAT_INFO_STACK_SIZE 200
#define RETURN_SHARED_STRING

#define SPRINTF_UNDECIDED -1027

struct format_info
{
  char *fi_free_string;
  struct pike_string *to_free_string;
  PCHARP b;
  ptrdiff_t len;
  ptrdiff_t width;
  int precision;
  PCHARP pad_string;
  ptrdiff_t pad_length;
  int column_entries;
  short flags;
  char pos_pad;
  int column_width;
  ptrdiff_t column_modulo;
};

/* FIXME:
 * re-allocate the format stack if it's too small /Hubbe
 */

struct format_stack
{
  struct format_info *fsp;
  struct format_info format_info_stack[FORMAT_INFO_STACK_SIZE];
};

#define FIELD_LEFT	(1<<0)
#define FIELD_CENTER	(1<<1)
#define PAD_POSITIVE	(1<<2)
#define LINEBREAK	(1<<3)
#define COLUMN_MODE	(1<<4)
#define ZERO_PAD	(1<<5)
#define ROUGH_LINEBREAK	(1<<6)
#define DO_TRUNC	(1<<7)
#define REPEAT		(1<<8)
#define SNURKEL		(1<<9)
#define INVERSE_COLUMN_MODE (1<<10)
#define MULTI_LINE	(1<<11)
#define WIDTH_OF_DATA	(1<<12)
#define MULTI_LINE_BREAK (1<<13)

#define MULTILINE (LINEBREAK | COLUMN_MODE | ROUGH_LINEBREAK | \
		   INVERSE_COLUMN_MODE | MULTI_LINE | REPEAT)


/* Generate binary IEEE strings on a machine which uses a different kind
   of floating point internally */

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
  if(PIKE_ISNAN(d)) {
    e = maxexp; f = maxf;
  } else
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
			     ptrdiff_t len,
			     int flags,
			     ptrdiff_t width,
			     PCHARP pad_string,
			     ptrdiff_t pad_length,
			     char pos_pad)
{
  ptrdiff_t e;
  if(!width || width==SPRINTF_UNDECIDED)
  {
    if(pos_pad && EXTRACT_PCHARP(b)!='-') string_builder_putchar(r,pos_pad);
    string_builder_append(r,b,len);
    return;
  }

  if(!(flags & DO_TRUNC) && len+(pos_pad && EXTRACT_PCHARP(b)!='-')>=width)
  {
    if(pos_pad && EXTRACT_PCHARP(b)!='-') string_builder_putchar(r,pos_pad);
    string_builder_append(r,b,len);
    return;
  }

  if (flags & (ZERO_PAD|FIELD_CENTER|FIELD_LEFT)) {
    /* Some flag is set. */

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
  
    if (flags & (FIELD_CENTER|FIELD_LEFT)) {
      ptrdiff_t d=0;
  
      if(flags & FIELD_CENTER)
      {
  	e=len;
  	if(pos_pad && EXTRACT_PCHARP(b)!='-') e++;
  	e=(width-e)/2;
  	if(e>0)
  	{
  	  string_builder_fill(r, e, pad_string, pad_length, 0);
  	  width-=e;
  	}
      }
  
      /* Left adjust */
      if(pos_pad && EXTRACT_PCHARP(b)!='-')
      {
  	string_builder_putchar(r,pos_pad);
  	width--;
  	d++;
      }

#if 1
      len = MINIMUM(width, len);
      if (len) {
        d += len;
	string_builder_append(r, b, len);
	width -= len;
      }
#else /* 0 */  
      d+=MINIMUM(width,len);
      while(len && width)
      {
  	string_builder_putchar(r,EXTRACT_PCHARP(b));
  	INC_PCHARP(b,1);
  	len--;
  	width--;
      }
#endif /* 1 */
  
      if(width>0)
      {
	d%=pad_length;
	string_builder_fill(r, width, pad_string, pad_length, d);
      }
      
      return;
    }
  }

  /* Right-justification */

  if(pos_pad && EXTRACT_PCHARP(b)!='-') len++;
  e=width-len;
  if(e>0)
  {
    string_builder_fill(r, e, pad_string, pad_length, 0);
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

static void free_sprintf_strings(struct format_stack *fs)
{
  for(;fs->fsp>=fs->format_info_stack;fs->fsp--)
  {
    if(fs->fsp->fi_free_string) free(fs->fsp->fi_free_string);
    fs->fsp->fi_free_string=0;
    if(fs->fsp->to_free_string) free_string(fs->fsp->to_free_string);
    fs->fsp->to_free_string=0;
  }
}

static void sprintf_error(struct format_stack *fs,
			  char *s,...) ATTRIBUTE((noreturn,format (printf, 2, 3)));
static void sprintf_error(struct format_stack *fs,
			  char *s,...)
{
  char buf[100];
  va_list args;
  va_start(args,s);
  free_sprintf_strings(fs);

  sprintf(buf,"sprintf: %s",s);
  va_error(buf,args);
  va_end(args);
}

/* This is called once for every '%' on every ouputted line
 * it takes care of linebrak and column mode. It returns 1
 * if there is more for next line.
 */

INLINE static int do_one(struct format_stack *fs,
			 struct string_builder *r,
			 struct format_info *f)
{
  PCHARP rest;
  ptrdiff_t e, d, lastspace;

  rest.ptr=0;
  if(f->flags & (LINEBREAK|ROUGH_LINEBREAK))
  {
    if(f->width==SPRINTF_UNDECIDED)
      sprintf_error(fs, "Must have field width for linebreak.\n");
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
    if(f->width==SPRINTF_UNDECIDED)
      sprintf_error(fs, "Must have field width for column mode.\n");
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
    ptrdiff_t mod;
    ptrdiff_t col;
    PCHARP end;

    if(f->width==SPRINTF_UNDECIDED)
      sprintf_error(fs, "Must have field width for column mode.\n");
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
    fix_field(r,f->b,f->len,f->flags,f->width,
	      f->pad_string,f->pad_length,f->pos_pad);
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
      sprintf_error(fs, "Too few arguments to sprintf.\n"); \
      break; /* make gcc happy */ \
    } \
    VAR=lastarg=argp+(argument++); \
  }

#define PEEK_SVALUE(VAR) \
  if(arg) \
  { \
    VAR=arg; \
  }else{ \
    if(argument >= num_arg) \
    { \
      sprintf_error(fs, "Too few arguments to sprintf.\n"); \
      break; /* make gcc happy */ \
    } \
    VAR=argp+argument; \
  }

#define GET(VAR,PIKE_TYPE,TYPE_NAME,EXTENSION) \
  { \
    struct svalue *tmp_; \
    GET_SVALUE(tmp_); \
    if(tmp_->type!=PIKE_TYPE) \
    { \
      sprintf_error(fs, "Wrong type for argument %d: expected %s, got %s.\n",argument+1,TYPE_NAME, \
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

#define DO_OP()								\
   if(fs->fsp->flags & SNURKEL)						\
   {									\
     ONERROR _e;							\
     struct array *_v;							\
     struct string_builder _b;						\
     init_string_builder(&_b,0);					\
     SET_ONERROR(_e, free_string_builder, &_b);				\
     GET_ARRAY(_v);							\
     for(tmp=0;tmp<_v->size;tmp++)					\
     {									\
       struct svalue *save_sp=Pike_sp;					\
       array_index_no_free(Pike_sp,_v,tmp);				\
       Pike_sp++;							\
       low_pike_sprintf(fs, &_b,begin,SUBTRACT_PCHARP(a,begin)+1,	\
			Pike_sp-1,1,nosnurkel+1);			\
       if(save_sp < Pike_sp) pop_stack();				\
     }									\
     fs->fsp->b=MKPCHARP_STR(_b.s);					\
     fs->fsp->len=_b.s->len;						\
     fs->fsp->fi_free_string=(char *)_b.s;				\
     fs->fsp->pad_string=MKPCHARP(" ",0);				\
     fs->fsp->pad_length=1;						\
     fs->fsp->column_width=0;						\
     fs->fsp->pos_pad=0;						\
     fs->fsp->flags=0;							\
     fs->fsp->width=fs->fsp->precision=SPRINTF_UNDECIDED;		\
     UNSET_ONERROR(_e);							\
     break;                                                             \
   }

#define CHECK_OBJECT_SPRINTF()						      \
	{								      \
	   /* NOTE: It would be nice if this was a do { } while(0) macro */   \
	   /* but it cannot be since we need to break out of the case... */   \
	  struct svalue *sv;						      \
	  PEEK_SVALUE(sv);						      \
	  if(sv->type == T_OBJECT && sv->u.object->prog)		      \
	  {                                                                   \
            ptrdiff_t fun=FIND_LFUN(sv->u.object->prog, LFUN__SPRINTF);	      \
	    if (fun != -1) {						      \
              DECLARE_CYCLIC();						      \
              if (!BEGIN_CYCLIC(sv->u.object, fun))			      \
              {                                                               \
              	int n=0;						      \
	      	push_int(EXTRACT_PCHARP(a));				      \
	      	if (fs->fsp->precision!=SPRINTF_UNDECIDED)		      \
	      	{							      \
	      	   push_constant_text("precision");			      \
	      	   push_int(fs->fsp->precision);			      \
              	   n+=2;						      \
	      	}							      \
	      	if (fs->fsp->width!=SPRINTF_UNDECIDED)			      \
	      	{							      \
	      	   push_constant_text("width");	           		      \
	      	   push_int64(fs->fsp->width);				      \
              	   n+=2;						      \
	      	}							      \
	      	if ((fs->fsp->flags&FIELD_LEFT))			      \
	      	{							      \
	      	   push_constant_text("flag_left");	       		      \
	      	   push_int(1);						      \
              	   n+=2;						      \
	      	}							      \
	      	f_aggregate_mapping(n);					      \
	      								      \
	      	SET_CYCLIC_RET(1);					      \
	      	apply_low(sv->u.object, fun, 2);                              \
									      \
	      	if(Pike_sp[-1].type == T_STRING)			      \
	      	{	                                                      \
              	  DO_IF_DEBUG( if(fs->fsp->to_free_string)                    \
              		       Pike_fatal("OOps in sprintf\n"); )             \
              	  fs->fsp->to_free_string = (--Pike_sp)->u.string;            \
	      								      \
	      	  fs->fsp->b = MKPCHARP_STR(fs->fsp->to_free_string);	      \
	      	  fs->fsp->len = fs->fsp->to_free_string->len;		      \
	      								      \
              	  /* We have to lift one argument from the format stack. */   \
              	  GET_SVALUE(sv);                                             \
                  END_CYCLIC();						      \
	      	  break;						      \
	      	}							      \
	      	if(!SAFE_IS_ZERO(Pike_sp-1))				      \
	      	{							      \
	      	   sprintf_error(fs,"argument %d (object) returned "	      \
	      			 "illegal value from _sprintf()\n",	      \
				 argument+1);				      \
	      	}							      \
	      	pop_stack();						      \
              }								      \
              END_CYCLIC();						      \
	    }								      \
	  }								      \
	}

/* This is the main pike_sprintf function, note that it calls itself
 * recursively during the '%{ %}' parsing. The string is stored in
 * the buffer in save_objectII.c
 */

static void low_pike_sprintf(struct format_stack *fs,
			     struct string_builder *r,
			     PCHARP format,
			     ptrdiff_t format_len,
			     struct svalue *argp,
			     ptrdiff_t num_arg,
			     int nosnurkel)
{
  int argument=0;
  int tmp,setwhat,d,e;
  char buffer[140];
  struct format_info *f,*start;
  double tf;
  struct svalue *arg=0;	/* pushback argument */
  struct svalue *lastarg=0;

  PCHARP a,begin;
  PCHARP format_end=ADD_PCHARP(format,format_len);

  start=fs->fsp;
  for(a=format;COMPARE_PCHARP(a,<,format_end);INC_PCHARP(a,1))
  {
    int num_snurkel;

    if(fs->fsp-fs->format_info_stack==FORMAT_INFO_STACK_SIZE - 1)
      sprintf_error(fs, "Sprintf stack overflow.\n");
    fs->fsp++;
#ifdef PIKE_DEBUG
    if(fs->fsp < fs->format_info_stack)
      Pike_fatal("sprintf: fs->fsp out of bounds.\n");
#endif
    fs->fsp->pad_string=MKPCHARP(" ",0);
    fs->fsp->pad_length=1;
    fs->fsp->fi_free_string=0;
    fs->fsp->to_free_string=0;
    fs->fsp->column_width=0;
    fs->fsp->pos_pad = 0;
    fs->fsp->flags = 0;
    fs->fsp->width=fs->fsp->precision=SPRINTF_UNDECIDED;

    if(EXTRACT_PCHARP(a)!='%')
    {
      for(e=0;INDEX_PCHARP(a,e)!='%' &&
	    COMPARE_PCHARP(ADD_PCHARP(a,e),<,format_end);e++);
      fs->fsp->b=a;
      fs->fsp->len=e;
      fs->fsp->width=e;
      INC_PCHARP(a,e-1);
      continue;
    }
    num_snurkel=0;
    arg=NULL;
    setwhat=0;
    begin=a;

    for(INC_PCHARP(a,1);;INC_PCHARP(a,1))
    {
#if 0
      fprintf(stderr,"sprintf-flop: %d (%c)\n",
	      EXTRACT_PCHARP(a),EXTRACT_PCHARP(a));
#endif
      switch(EXTRACT_PCHARP(a))
      {
      default:
	if(EXTRACT_PCHARP(a) < 256 && 
	   isprint(EXTRACT_PCHARP(a)))
	{
	  sprintf_error(fs, "Error in format string, %c is not a format.\n",
			EXTRACT_PCHARP(a));
	}else{
	  sprintf_error(fs,"Error in format string, U%08x is not a format.\n",
			EXTRACT_PCHARP(a));
	}

      /* First the modifiers */
      case '0':
	 if (setwhat<2) 
	 { 
	    fs->fsp->flags|=ZERO_PAD; 
	    continue; 
	 }
      case '1': case '2': case '3':
      case '4': case '5': case '6':
      case '7': case '8': case '9':
	tmp=STRTOL_PCHARP(a,&a,10);
	INC_PCHARP(a,-1);
	goto got_arg;

      case '*':
	GET_INT(tmp);

      got_arg:
	switch(setwhat)
	{
	case 0:
	case 1:
	  if(tmp < 0) sprintf_error(fs, "Illegal width %d.\n", tmp);
	  fs->fsp->width=tmp;
	  if (!setwhat) break;
	case 2: fs->fsp->precision=tmp; break;
	case 3: fs->fsp->column_width=tmp; break;
	case 4: fs->fsp->precision=-tmp; break;
	}
	continue;

      case ';': setwhat=3; continue;
      case '.': setwhat=2; continue;
      case ':': setwhat=1; continue;

      case '=': fs->fsp->flags|=LINEBREAK; continue;
      case '/': fs->fsp->flags|=ROUGH_LINEBREAK; continue;
      case '#': fs->fsp->flags|=COLUMN_MODE; continue;
      case '$': fs->fsp->flags|=INVERSE_COLUMN_MODE; continue;

      case '-':
	if(setwhat==2)
	  setwhat=4;
	else
	  fs->fsp->flags|=FIELD_LEFT;
	continue;
      case '|': fs->fsp->flags|=FIELD_CENTER; continue;
      case ' ': fs->fsp->pos_pad=' '; continue;
      case '+': fs->fsp->pos_pad='+'; continue;
      case '!': fs->fsp->flags^=DO_TRUNC; continue;
      case '^': fs->fsp->flags|=REPEAT; continue;
      case '>': fs->fsp->flags|=MULTI_LINE_BREAK; continue;
      case '_': fs->fsp->flags|=WIDTH_OF_DATA; continue;
      case '@':
	if(++num_snurkel > nosnurkel)
	  fs->fsp->flags|=SNURKEL;
	continue;

      case '\'':
	tmp=0;
	for(INC_PCHARP(a,1);INDEX_PCHARP(a,tmp)!='\'';tmp++)
	{
#if 0
	  fprintf(stderr, "Sprinf-glop: %d (%c)\n",
		  INDEX_PCHARP(a,tmp), INDEX_PCHARP(a,tmp));
#endif
	  if(COMPARE_PCHARP(a,>=,format_end))
	    sprintf_error(fs, "Unfinished pad string in format string.\n");
	}
	if(tmp)
	{
	  fs->fsp->pad_string=a;
	  fs->fsp->pad_length=tmp;
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
	  sprintf_error(fs, "No last argument.\n");
	arg=lastarg;
	continue;

      case '[':
	INC_PCHARP(a,1);
	if(EXTRACT_PCHARP(a)=='*') {
	  GET_INT(tmp);
	  INC_PCHARP(a,1);
	} else
	  tmp=STRTOL_PCHARP(a,&a,10);
	if(EXTRACT_PCHARP(a)!=']') 
	  sprintf_error(fs, "Expected ] in format string, not %c.\n",
			EXTRACT_PCHARP(a));
	if(tmp >= num_arg)
	  sprintf_error(fs, "Not enough arguments to [%d].\n",tmp);
	arg = argp+tmp;
	continue;
	
        /* now the real operators */

      case '{':
      {
	struct array *w;
	struct string_builder b;
#ifdef PIKE_DEBUG
	struct format_info *fsp_save=fs->fsp;
#endif
	DO_OP();
	for(e=1,tmp=1;tmp;e++)
	{
	  if (!INDEX_PCHARP(a,e) &&
	      !COMPARE_PCHARP(ADD_PCHARP(a,e),<,format_end)) {
	    sprintf_error(fs, "Missing %%} in format string.\n");
	    break;		/* UNREACHED */
	  } else if(INDEX_PCHARP(a,e)=='%') {
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
	  fs->fsp->b=MKPCHARP("",0);
	  fs->fsp->len=0;
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
	    s=Pike_sp;
	    if(q)
	    {
	      add_ref(q->array);
	      push_array_items(q->array);
	    }else{
	      array_index_no_free(Pike_sp,w,tmp);
	      Pike_sp++;
	    }
	    low_pike_sprintf(fs, &b,ADD_PCHARP(a,1),e-2,s,Pike_sp-s,0);
	    pop_n_elems(Pike_sp-s);
	  }
#ifdef PIKE_DEBUG
	  if(fs->fsp < fs->format_info_stack)
	    Pike_fatal("sprintf: fs->fsp out of bounds.\n");
	  if(fs->fsp!=fsp_save)
	    Pike_fatal("sprintf: fs->fsp incorrect after recursive sprintf.\n");
#endif
	  fs->fsp->b=MKPCHARP_STR(b.s);
	  fs->fsp->len=b.s->len;
	  fs->fsp->fi_free_string=(char *)b.s;
	  UNSET_ONERROR(err);
	}
	
	INC_PCHARP(a,e);
	break;
      }

      case '%':
	fs->fsp->b=MKPCHARP("%",0);
	fs->fsp->len=fs->fsp->width=1;
	break;

      case 'n':
	DO_OP();
	fs->fsp->b=MKPCHARP("",0);
	fs->fsp->len=0;
	break;

      case 't':
      {
	struct svalue *t;
	DO_OP();
	CHECK_OBJECT_SPRINTF()
	GET_SVALUE(t);
	fs->fsp->b=MKPCHARP(get_name_of_type(t->type),0);
	fs->fsp->len=strlen((char *)fs->fsp->b.ptr);
	break;
      }

      case 'c':
      {
        INT_TYPE tmp;
	ptrdiff_t l,n;
	char *x;
        DO_OP();
	CHECK_OBJECT_SPRINTF()
	if(fs->fsp->width == SPRINTF_UNDECIDED)
	{
	  GET_INT(tmp);
	  x=(char *)alloca(4);
	  if(tmp<256) fs->fsp->b=MKPCHARP(x,0);
	  else if(tmp<65536) fs->fsp->b=MKPCHARP(x,1);
	  else  fs->fsp->b=MKPCHARP(x,2);
	  SET_INDEX_PCHARP(fs->fsp->b,0,tmp);
	  fs->fsp->len=1;
	}
	else if ( (fs->fsp->flags&FIELD_LEFT) )
	{
	  l=1;
	  if(fs->fsp->width > 0) l=fs->fsp->width;
	  x=(char *)alloca(l);
	  fs->fsp->b=MKPCHARP(x,0);
	  fs->fsp->len=l;
	  GET_INT(tmp);
	  n=0;
	  while(n<l)
	  {
	    x[n++]=tmp & 0xff;
	    tmp>>=8;
	  }
	}
	else 
	{
	  l=1;
	  if(fs->fsp->width > 0) l=fs->fsp->width;
	  x=(char *)alloca(l);
	  fs->fsp->b=MKPCHARP(x,0);
	  fs->fsp->len=l;
	  GET_INT(tmp);
	  while(--l>=0)
	  {
	    x[l]=tmp & 0xff;
	    tmp>>=8;
	  }
	}
	break;
      }

      /*
	WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	WARNING                                                 WARNING
	WARNING   This routine is not very well tested, so it   WARNING
	WARNING   may give errouneous results.   /Noring        WARNING
	WARNING                                                 WARNING
	WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
       */
      case 'b':
      case 'o':
      case 'd':
      case 'u':
      case 'x':
      case 'X':
      {
	int base = 0, mask_size = 0;
	char mode, *x;
	INT_TYPE val;
	
	DO_OP();
	CHECK_OBJECT_SPRINTF()
	GET_INT(val);

	if(fs->fsp->precision != SPRINTF_UNDECIDED && fs->fsp->precision > 0)
	  mask_size = fs->fsp->precision;
	
	mode=EXTRACT_PCHARP(a);
	x=(char *)alloca(sizeof(val)*CHAR_BIT + 4 + mask_size);
	fs->fsp->b=MKPCHARP(x,0);
	
	switch(mode)
	{
	  case 'b': base = 1; break;
	  case 'o': base = 3; break;
	  case 'x': base = 4; break;
	  case 'X': base = 4; break;
	}

	if(base)
	{
	  char *p = x;
	  ptrdiff_t l;

	  if(mask_size || val>=0)
	  {
	    do {
	      if((*p++ = '0'|(val&((1<<base)-1)))>'9')
		p[-1] += (mode=='X'? 'A'-'9'-1 : 'a'-'9'-1);
	      val >>= base;
	    } while(--mask_size && val);
	    l = p-x;
	  }
	  else
	  {
	    *p++ = '-';
	    val = -val;
	    do {
	      if((*p++ = '0'|(val&((1<<base)-1)))>'9')
		p[-1] += (mode=='X'? 'A'-'9'-1 : 'a'-'9'-1);
	      val = ((unsigned INT_TYPE)val) >> base;
	    } while(val);
	    l = p-x-1;
	  }
	  *p = '\0';
	  while(l>1) {
	    char t = p[-l];
	    p[-l] = p[-1];
	    p[-1] = t;
	    --p;
	    l -= 2;
	  }
	}
	else if(mode == 'u')
	  sprintf(x, "%"PRINTPIKEINT"u", (unsigned INT_TYPE) val);
	else
	  sprintf(x, "%"PRINTPIKEINT"d", val);

	fs->fsp->len=strlen(x);
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
	CHECK_OBJECT_SPRINTF()
	GET_FLOAT(tf);

	/* Special casing for infinity and not a number,
	 * since many libc's forget about them...
	 */
	if (PIKE_ISNAN(tf)) {
	  /* NaN */
	  fs->fsp->b = MKPCHARP("nan", 0);
	  fs->fsp->len = 3;
	  break;
	} else if (tf && (tf+tf == tf)) {
	  /* Infinity. */
	  if (tf > 0.0) {
	    fs->fsp->b = MKPCHARP("inf", 0);
	    fs->fsp->len = 3;
	  } else {
	    fs->fsp->b = MKPCHARP("-inf", 0);
	    fs->fsp->len = 4;
	  }
	  break;
	}

	if (fs->fsp->precision==SPRINTF_UNDECIDED) fs->fsp->precision=3;

	/* FIXME: The constant (320) is good for IEEE double precision
	 * float, but will definitely fail for bigger precision! --aldem
	 */
	x=(char *)xalloc(320+MAXIMUM(fs->fsp->precision,3));
	fs->fsp->fi_free_string=x;
	fs->fsp->b=MKPCHARP(x,0);
	sprintf(buffer,"%%*.*%c",EXTRACT_PCHARP(a));

	if(fs->fsp->precision<0) {
	  double m=pow(10.0, (double)fs->fsp->precision);
	  tf = RINT(tf*m)/m;
	} else if (fs->fsp->precision==0) {
	  tf = RINT(tf);
        }

	debug_malloc_touch(x);
	sprintf(x,buffer,1,fs->fsp->precision<0?0:fs->fsp->precision,tf);
	debug_malloc_touch(x);
	fs->fsp->len=strlen(x);
	
	/* Make sure that the last digits really are zero. */
	if(fs->fsp->precision<0)
	{
	  ptrdiff_t i, j;
	  /* Find the ending of the number.  Yes, this can be made
	     simpler now when the alignment bug for floats is fixed. */
	  for(i=fs->fsp->len-1; i>=0; i--)
 	    if('0'<=x[i] && x[i]<='9')
	    {
	      i+=fs->fsp->precision+1;
	      if(i>=0 && '0'<=x[i] && x[i]<='9')
		for(j=0; j<-fs->fsp->precision; j++)
		  x[i+j]='0';
	      break;
	    }
	}
	break;
      }

      case 'F':
      {
        ptrdiff_t l;
	char *x;
        DO_OP();
        l=4;
        if(fs->fsp->width > 0) l=fs->fsp->width;
	if(l != 4 && l != 8)
	  sprintf_error(fs, "Invalid IEEE width %ld.\n",
			PTRDIFF_T_TO_LONG(l));
	x=(char *)alloca(l);
	fs->fsp->b=MKPCHARP(x,0);
	fs->fsp->len=l;
	GET_FLOAT(tf);
	switch(l) {
	case 4:
	  {
#ifdef FLOAT_IS_IEEE_BIG
	    float f = DO_NOT_WARN((float)tf);
	    MEMCPY(x, &f, 4);
#else
#ifdef FLOAT_IS_IEEE_LITTLE
	    float f = DO_NOT_WARN((float)tf);
	    x[0] = ((char *)&f)[3];
	    x[1] = ((char *)&f)[2];
	    x[2] = ((char *)&f)[1];
	    x[3] = ((char *)&f)[0];
#else
	    low_write_IEEE_float(x, tf, 4);
#endif
#endif
	  }
	  break;
	case 8:
#ifdef DOUBLE_IS_IEEE_BIG
	  MEMCPY(x, &tf, 8);
#else
#ifdef DOUBLE_IS_IEEE_LITTLE
	  x[0] = ((char *)&tf)[7];
	  x[1] = ((char *)&tf)[6];
	  x[2] = ((char *)&tf)[5];
	  x[3] = ((char *)&tf)[4];
	  x[4] = ((char *)&tf)[3];
	  x[5] = ((char *)&tf)[2];
	  x[6] = ((char *)&tf)[1];
	  x[7] = ((char *)&tf)[0];
#else
	  low_write_IEEE_float(x, tf, 8);
#endif
#endif
	}
	break;
      }

      case 'O':
      {
	dynamic_buffer save_buf;
	dynbuf_string s;
	struct svalue *t;
	DO_OP();
	/* No need to do CHECK_OBJECT_SPRINTF() here,
	   it is checked in describe_svalue. */
	GET_SVALUE(t);
	init_buf(&save_buf);
	describe_svalue(t,0,0);
	s=complex_free_buf(&save_buf);
	fs->fsp->b=MKPCHARP(s.str,0);
	fs->fsp->len=s.len;
	fs->fsp->fi_free_string=s.str;
	break;
      }

#if 0
      /* This can be useful when doing low level debugging. */
      case 'p':
      {
	dynamic_buffer save_buf;
	dynbuf_string s;
	char buf[50];
	struct svalue *t;
	DO_OP();
	GET_SVALUE(t);
	init_buf(&save_buf);
	sprintf (buf, "%p", t->u.refs);
	my_strcat (buf);
	s=complex_free_buf(&save_buf);
	fs->fsp->b=MKPCHARP(s.str,0);
	fs->fsp->len=s.len;
	fs->fsp->fi_free_string=s.str;
	break;
      }
#endif

      case 's':
      {
	struct pike_string *s;
	DO_OP();
	CHECK_OBJECT_SPRINTF()
	GET_STRING(s);
	fs->fsp->b=MKPCHARP_STR(s);
	fs->fsp->len=s->len;
	if(fs->fsp->precision != SPRINTF_UNDECIDED && fs->fsp->precision < fs->fsp->len)
	  fs->fsp->len = (fs->fsp->precision < 0 ? 0 : fs->fsp->precision);
	break;
      }
      }
      break;
    }
  }

  for(f=fs->fsp;f>start;f--)
  {
    if(f->flags & WIDTH_OF_DATA)
      f->width=f->len;

    if(((f->flags & INVERSE_COLUMN_MODE) && !f->column_width) ||
       (f->flags & COLUMN_MODE))
    {
      int max_len,nr;
      ptrdiff_t columns;
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
      if(max_len<tmp)
	max_len=tmp;
      if(!f->column_width)
	f->column_width=max_len;
      f->column_entries=nr;
      columns=f->width/(f->column_width+1);
      
      if(f->column_width<1 || columns<1)
	columns=1;
      f->column_modulo=(nr+columns-1)/columns;
    }
  }

  /* Here we do some DWIM */
  for(f=fs->fsp-1;f>start;f--)
  {
    if((f[1].flags & MULTILINE) &&
       !(f[0].flags & (MULTILINE|MULTI_LINE_BREAK)))
    {
      if(! MEMCHR_PCHARP(f->b, '\n', f->len).ptr ) f->flags|=MULTI_LINE;
    }
  }
  
  for(f=start+1;f<=fs->fsp;)
  {
    for(;f<=fs->fsp && !(f->flags&MULTILINE);f++)
      do_one(fs, r, f);
    
    do {
      d=0;
      for(e=0;f+e<=fs->fsp && (f[e].flags & MULTILINE);e++)
	d |= do_one(fs, r, f+e);
      if(d)
	string_builder_putchar(r,'\n');
    } while(d);

    for(;f<=fs->fsp && (f->flags&MULTILINE); f++);
  }

  while(fs->fsp>start)
  {
#ifdef PIKE_DEBUG
    if(fs->fsp < fs->format_info_stack)
      Pike_fatal("sprintf: fsp out of bounds.\n");
#endif
    
    if(fs->fsp->fi_free_string)
      free(fs->fsp->fi_free_string);
    fs->fsp->fi_free_string=0;
    
    if(fs->fsp->to_free_string)
      free_string(fs->fsp->to_free_string);
    fs->fsp->to_free_string=0;
    
    fs->fsp--;
  }
}

/* The efun */
void f_sprintf(INT32 args)
{
  ONERROR err_string_builder, err_format_stack;
  struct pike_string *ret;
  struct svalue *argp;
  struct string_builder r;

  struct format_stack fs;

  argp=Pike_sp-args;
  
  fs.fsp = fs.format_info_stack-1;

  if(argp[0].type != T_STRING) {
    if (argp[0].type == T_OBJECT) {
      /* Try checking if we can cast it to a string... */
      ref_push_object(argp[0].u.object);
      o_cast(string_type_string, PIKE_T_STRING);
      if (Pike_sp[-1].type != T_STRING) {
	/* We don't accept objects... */
	Pike_error("sprintf(): Cast to string failed.\n");
      }
      /* Replace the original object with the new string. */
      assign_svalue(argp, Pike_sp-1);
      /* Clean up the stack. */
      pop_stack();
    } else {
      SIMPLE_BAD_ARG_ERROR("sprintf", 1, "string|object");
    }
  }

  init_string_builder(&r,0);
  SET_ONERROR(err_format_stack, free_sprintf_strings, &fs);
  SET_ONERROR(err_string_builder, free_string_builder, &r);
  low_pike_sprintf(&fs,
		   &r,
		   MKPCHARP_STR(argp->u.string),
		   argp->u.string->len,
		   argp+1,
		   args-1,
		   0);
  UNSET_ONERROR(err_string_builder);
  UNSET_ONERROR(err_format_stack);
  ret=finish_string_builder(&r);

  pop_n_elems(args);
  push_string(ret);
}

static node *optimize_sprintf(node *n)
{
  node **arg0 = my_get_arg(&_CDR(n), 0);
  node **arg1 = my_get_arg(&_CDR(n), 1);
  node *ret;
  int num_args=count_args(CDR(n));
  if(arg0 && arg1 && num_args == 2 &&
     (*arg0)->token == F_CONSTANT &&
     (*arg0)->u.sval.type == T_STRING &&
     (*arg0)->u.sval.u.string->size_shift == 0 &&
     (*arg0)->u.sval.u.string->len == 2 &&
     STR0((*arg0)->u.sval.u.string)[0]=='%')
  {
    switch(STR0((*arg0)->u.sval.u.string)[1])
    {
      case 'c':
	ADD_NODE_REF2(*arg1,
		      ret = mkefuncallnode("int2char",*arg1);
	  );
	return ret;

      case 't':
	ADD_NODE_REF2(*arg1,
		      ret = mkefuncallnode("basetype",*arg1);
	  );
	return ret;

      case 'x':
	ADD_NODE_REF2(*arg1,
		      ret = mkefuncallnode("int2hex",*arg1);
	  );
	return ret;

      default: break;
    }
  }
  return 0;
}

PIKE_MODULE_INIT
{
  /* function(string|object, mixed ... : string) */
  ADD_EFUN2("sprintf", 
	    f_sprintf,
	    tFuncV(tOr(tStr, tObj), tMix, tStr),
	    OPT_TRY_OPTIMIZE,
	    optimize_sprintf,
	    0);
}

PIKE_MODULE_EXIT
{
}
