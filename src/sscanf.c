/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "interpret.h"
#include "array.h"
#include "stralloc.h"
#include "pike_error.h"
#include "fd_control.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"
#include "bignum.h"
#include "pike_float.h"
#include "pike_types.h"
#include "sscanf.h"
#include "bitvector.h"

#define sp Pike_sp

/*
 * helper functions for sscanf %O
 */

/* Calling convention:
 *   val: Integer to fill in.
 *   str: string to parse.
 *   len: length of the string.
 *
 * Returns:
 *   NULL on failure.
 *   continuation point in str on success.
 */
static void *pcharp_extract_char_const(INT_TYPE *val,
				       PCHARP str, ptrdiff_t len)
{
  int c;

  /* use of macros to keep similar to lexer.h: char_const */
#define LOOK() (len>0?EXTRACT_PCHARP(str):0)
#define GETC() ((len > 0)?(INC_PCHARP(str, 1), len--, INDEX_PCHARP(str, -1)):0)
      
  switch (c=GETC())
  {
  case 0:
    return NULL;

  case '\n': return NULL;	/* Newline in character constant. */
      
  case 'a': c = 7; break;       /* BEL */
  case 'b': c = 8; break;       /* BS */
  case 't': c = 9; break;       /* HT */
  case 'n': c = 10; break;      /* LF */
  case 'v': c = 11; break;      /* VT */
  case 'f': c = 12; break;      /* FF */
  case 'r': c = 13; break;      /* CR */
  case 'e': c = 27; break;      /* ESC */
      
  case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7':
    /* Octal escape. */
    c-='0';
    while(LOOK()>='0' && LOOK()<='8')
      c=c*8+(GETC()-'0');
    break;
      
  case 'x':
    /* Hexadecimal escape. */
    c=0;
    while(1)
    {
      switch(LOOK())
      {
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
      case '8': case '9':
	c=c*16+GETC()-'0';
	continue;
	    
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	c=c*16+GETC()-'a'+10;
	continue;
	    
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	c=c*16+GETC()-'A'+10;
	continue;
      }
      break;
    }
    break;

  case 'd':
    /* Decimal escape. */
    c=0;
    while(1)
    {
      switch(LOOK())
      {
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
      case '8': case '9':
	c=c*10+GETC()-'0';
	continue;
      }
      break;
    }
    break;
#undef LOOK
#undef GETC
  }
  *val = c;
  return str.ptr;
}

/* Calling convention:
 *   res: svalue to fill in.
 *   str: string to parse.
 *   len: length of the string.
 *
 * Returns:
 *   NULL on failure.
 *   continuation point in str on success.
 */
static void *pcharp_to_svalue_percent_o(struct svalue *res,
					PCHARP str,
					ptrdiff_t len)
{
  SET_SVAL(*res, T_INT, NUMBER_UNDEFINED, integer, 0);

  for (;len>0; INC_PCHARP(str, 1), len--)
  {
    switch (EXTRACT_PCHARP(str))
    {
    case ' ':  /* whitespace */
    case '\t':
    case '\n':
    case '\r':  
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      {
	/* fixme: grok floats */
	if (!pcharp_to_svalue_inumber(res, str, &str, 0, len)) {
	  return NULL;
	}
	return str.ptr;
      }

    case '\"':
      {
	struct string_builder tmp;
	PCHARP start;
	int cnt;
	init_string_builder(&tmp,0);

	INC_PCHARP(str, 1);	/* Skip the quote. */
	len--;

	start = str;
	cnt = 0;
	for (;len;)
	{
	  switch(EXTRACT_PCHARP(str))
	  {
	  case '\"':
	    /* End of string -- done. */
	    INC_PCHARP(str, 1);	/* Skip the quote. */

	    if (cnt) string_builder_append(&tmp, start, cnt);
	    SET_SVAL(*res, T_STRING, 0, string, finish_string_builder(&tmp));
	    return str.ptr;

	  case '\\':
	    {
	      /* Escaped character */
	      INT_TYPE val;

	      if (cnt) string_builder_append(&tmp, start, cnt);
	      INC_PCHARP(str, 1);
	      len--;
	      start.ptr = pcharp_extract_char_const(&val, str, len);
	      if (!start.ptr) break;
	      string_builder_putchar(&tmp, val);
				     
	      /* Continue parsing after the escaped character. */
	      len -= LOW_SUBTRACT_PCHARP(start, str);
	      cnt = 0;
	      str = start;
	    }
	    continue;

	  case '\n':
	    /* Newline in string -- fail. */
	    break;

	  default:
	    len--;
	    cnt++;
	    INC_PCHARP(str, 1);
	    continue;
	  }
	  break;
	}
	/* Unterminated string -- fail. */
	free_string_builder(&tmp);
	return NULL; /* end of data */
      }

    case '\'':
      if (len>2)
      {
	INC_PCHARP(str, 1);	/* Skip the quote. */

	SET_SVAL(*res, T_INT, NUMBER_NUMBER, integer, EXTRACT_PCHARP(str));
	INC_PCHARP(str, 1);

	len -= 2;

	if (res->u.integer == '\\')
	{
	  PCHARP tmp;
	  tmp.ptr = pcharp_extract_char_const(&res->u.integer, str, len);
	  if (!tmp.ptr) return NULL;
	  tmp.shift = str.shift;
	  len -= LOW_SUBTRACT_PCHARP(tmp, str);
	  str.ptr = tmp.ptr;
	}
	if (!len || (EXTRACT_PCHARP(str) != '\''))
	  return NULL;
	INC_PCHARP(str, 1);	/* Skip the ending quote. */
	return str.ptr;
      }
      return NULL;
	    
      /* fixme: arrays, multisets, mappings */
    }
  }
  return NULL;
}

/*
  flags:
   *
  operators:
  %d
  %H
  %s
  %f
  %c
  %n
  %[
  %%
  %O
*/


struct sscanf_set
{
  int neg;
  char c[256];
  struct array *a;
};

/* FIXME:
 * This implementation will break in certain cases, especially cases
 * like this [\1000-\1002\1001] (ie, there is a single character which
 * is also a part of a range
 *   /Hubbe
 */


static ptrdiff_t read_set(PCHARP match, ptrdiff_t cnt, struct sscanf_set *set,
                          ptrdiff_t match_len, INT32 flags) {
  p_wchar2 e, last = 0;
  int set_size=0;

  if(cnt>=match_len)
    Pike_error("Error in sscanf format string.\n");

  MEMSET(set->c, 0, sizeof(set->c));
  set->a=0;

  if(INDEX_PCHARP(match, cnt)=='^' &&
     (cnt+2>=match_len || INDEX_PCHARP(match, cnt+1)!='-' ||
      INDEX_PCHARP(match, cnt+2)==']' || (flags & SSCANF_FLAG_76_COMPAT)))
  {
    set->neg=1;
    cnt++;
    if(cnt>=match_len)
      Pike_error("Error in sscanf format string.\n");
  }else{
    set->neg=0;
  }

  if(INDEX_PCHARP(match, cnt)==']' || INDEX_PCHARP(match, cnt)=='-')
  {
    set->c[last=INDEX_PCHARP(match, cnt)]=1;
    cnt++;
    if(cnt>=match_len)
      Pike_error("Error in sscanf format string.\n");
  }

  for(;INDEX_PCHARP(match, cnt)!=']';)
  {
    if(INDEX_PCHARP(match, cnt)=='-')
    {
      cnt++;
      if(cnt>=match_len)
	Pike_error("Error in sscanf format string.\n");

      if(INDEX_PCHARP(match, cnt)==']')
      {
	set->c['-']=1;
	break;
      }

      if(last > INDEX_PCHARP(match, cnt))
	Pike_error("Error in sscanf format string.\n");

      if (!match.shift) {
	  for(e=last;e<=INDEX_PCHARP(match, cnt);e++) set->c[e]=1;
      } else if(last < (p_wchar2)sizeof(set->c) && last >= 0) {
	if(INDEX_PCHARP(match, cnt) < (p_wchar2)sizeof(set->c))
	{
	  for(e=last;e<=INDEX_PCHARP(match, cnt);e++) set->c[e]=1;
	}else{
	  for(e=last;e<(p_wchar2)sizeof(set->c);e++)
            set->c[e]=1;

	  check_stack(2);
	  push_int(256);
	  push_int(INDEX_PCHARP(match, cnt));
	  set_size++;
	}
      }
      else
      {
	sp[-1].u.integer=INDEX_PCHARP(match, cnt);
      }
    } else {
      last=INDEX_PCHARP(match, cnt);
      if(!match.shift || (last < (p_wchar2)sizeof(set->c) && last >= 0)) {
        set->c[last]=1;
      } else {
        if(set_size &&
	   ((p_wchar2)sp[-1].u.integer) == last-1)
        {
	  sp[-1].u.integer++;
        }else{
	  check_stack(2);
	  push_int(last);
	  push_int(last);
	  set_size++;
        }
      }
    }
    cnt++;
    if(cnt>=match_len) {
      Pike_error("Error in sscanf format string: %s %d\n", match.ptr, cnt);
    }
  }


  if(match.shift && set_size)
  {
    INT32 *order;
    set->a=aggregate_array(set_size*2);
    order=get_switch_order(set->a);
    for(e=0;e<(p_wchar2)set->a->size;e+=2)
    {
      if(order[e]+1 != order[e+1] &&
         order[e+1]+1 != order[e]) {
        free_array(set->a);
        set->a=0;
        free(order);
	Pike_error("Overlapping ranges in sscanf not supported.\n");
      }
    }

    order_array(set->a,order);
    free(order);
  }
  return cnt;
}



/* Parse binary IEEE strings on a machine which uses a different kind
   of floating point internally */

#ifdef NEED_CUSTOM_IEEE

static INLINE FLOAT_TYPE low_parse_IEEE_float(char *b, int sz)
{
  unsigned INT32 f, extra_f;
  int s, e;
  unsigned char x[4];
  double r;
  DECLARE_INF
  DECLARE_NAN

  x[0] = EXTRACT_UCHAR(b);
  x[1] = EXTRACT_UCHAR(b+1);
  x[2] = EXTRACT_UCHAR(b+2);
  x[3] = EXTRACT_UCHAR(b+3);
  s = ((x[0]&0x80)? 1 : 0);

  if(sz==4) {
    e = (((int)(x[0]&0x7f))<<1)|((x[1]&0x80)>>7);
    f = (((unsigned INT32)(x[1]&0x7f))<<16)|(((unsigned INT32)x[2])<<8)|x[3];
    extra_f = 0;
    if(e==255)
      e = 9999;
    else if(e>0) {
      f |= 0x00800000;
      e -= 127+23;
    } else
      e -= 126+23;
  } else {
    e = (((int)(x[0]&0x7f))<<4)|((x[1]&0xf0)>>4);
    f = (((unsigned INT32)(x[1]&0x0f))<<16)|(((unsigned INT32)x[2])<<8)|x[3];
    extra_f = (((unsigned INT32)EXTRACT_UCHAR(b+4))<<24)|
      (((unsigned INT32)EXTRACT_UCHAR(b+5))<<16)|
      (((unsigned INT32)EXTRACT_UCHAR(b+6))<<8)|
      ((unsigned INT32)EXTRACT_UCHAR(b+7));
    if(e==2047)
      e = 9999;
    else if(e>0) {
      f |= 0x00100000;
      e -= 1023+20;
    } else
      e -= 1022+20;
  }
  if(e>=9999)
    if(f||extra_f) {
      /* NAN */
      return (FLOAT_TYPE)MAKE_NAN();
    } else {
      /* +/- Infinity */
      return (FLOAT_TYPE)MAKE_INF(s? -1:1);
    }

  r = (double)f;
  if(extra_f)
    r += ((double)extra_f)/4294967296.0;
  return (FLOAT_TYPE)(s? -LDEXP(r, e):LDEXP(r, e));
}

#endif

static float extract_float_be(const char * x) {
    float f;

#ifdef FLOAT_IS_IEEE_BIG
    memcpy(&f, x, sizeof(f));
#elif FLOAT_IS_IEEE_LITTLE
    unsigned INT32 tmp = get_unaligned32(x);
    tmp = bswap32(tmp);
    memcpy(&f, &tmp, sizeof(f));
#else
    f = low_parse_IEEE_float(x, 4);
#endif
    return f;
}

static double extract_double_be(const char * x) {
    double f;

#ifdef FLOAT_IS_IEEE_BIG
    memcpy(&f, x, sizeof(f));
#elif FLOAT_IS_IEEE_LITTLE
    unsigned INT64 tmp = get_unaligned64(x);
    tmp = bswap64(tmp);
    memcpy(&f, &tmp, sizeof(f));
#else
    f = low_parse_IEEE_float(x, 8);
#endif
    return f;
}

static float extract_float_le(const char * x) {
    float f;

#ifdef FLOAT_IS_IEEE_LITTLE
    memcpy(&f, x, sizeof(f));
#elif FLOAT_IS_IEEE_BIG
    unsigned INT32 tmp = get_unaligned32(x);
    tmp = bswap32(tmp);
    memcpy(&f, &tmp, sizeof(f));
#else
    unsigned INT32 tmp = get_unaligned32(x);
    tmp = bswap32(tmp);
    f = low_parse_IEEE_float((char*)&tmp, sizeof(tmp));
#endif
    return f;
}

static double extract_double_le(const char * x) {
    double f;

#ifdef FLOAT_IS_IEEE_LITTLE
    memcpy(&f, x, sizeof(f));
#elif FLOAT_IS_IEEE_BIG
    unsigned INT64 tmp = get_unaligned64(x);
    tmp = bswap64(tmp);
    memcpy(&f, &tmp, sizeof(f));
#else
    unsigned INT64 tmp = get_unaligned64(x);
    tmp = bswap64(tmp);
    f = low_parse_IEEE_float((char*)&tmp, sizeof(tmp));
#endif
    return f;
}

#define EXTRACT_FLOAT(SVAL, input, shift, fun)    do {      \
    char x[4];                                              \
    if (shift == 0) {                                       \
        memcpy(x, input, sizeof(x));                        \
    } else {                                                \
        PCHARP tmp = MKPCHARP(input, shift);                \
        size_t i;                                           \
        for (i = 0; i < sizeof(x); INC_PCHARP(tmp, 1), i++) \
            x[i] = EXTRACT_PCHARP(tmp);                     \
    }                                                       \
    (SVAL).u.float_number = fun(x);                         \
} while (0)

#define EXTRACT_DOUBLE(SVAL, input, shift, fun)   do {      \
    char x[8];                                              \
    if (shift == 0) {                                       \
        memcpy(x, input, sizeof(x));                        \
    } else {                                                \
        PCHARP tmp = MKPCHARP(input, shift);                \
        size_t i;                                           \
        for (i = 0; i < sizeof(x); INC_PCHARP(tmp, 1), i++) \
            x[i] = EXTRACT_PCHARP(tmp);                     \
    }                                                       \
    (SVAL).u.float_number = fun(x);                         \
} while (0)


/* Avoid some warnings about loss of precision */
#ifdef __ECL
static INLINE INT32 TO_INT32(ptrdiff_t x)
{
  return DO_NOT_WARN((INT32)x);
}
#else /* !__ECL */
#define TO_INT32(x)	((INT32)(x))
#endif /* __ECL */

/* INT32 very_low_sscanf_{0,1,2}(p_wchar *input, ptrdiff_t input_len,
 *				 PCHARP match, ptrdiff_t match_len,
 *				 ptrdiff_t *chars_matched,
 *				 int *success)
 *
 * Perform the actual parsing.
 *
 * Arguments:
 *   input, input_len		Input data to parse.
 *   match, match_len		Format string.
 *   chars_matched		Gets set to the number of characters
 *				in the input that were advanced.
 *   success			Gets set to 1 on success.
 *
 * Returns:
 *   Returns the number of %-directives that were successfully matched.
 *   Pushes non-ignored matches on the Pike stack in the order they
 *   were matched.
 *
 * FIXME: chars_matched and success are only used internally, and
 *        should probably be gotten rid of.
 */
#define MK_VERY_LOW_SSCANF(INPUT_SHIFT)			                 \
static INT32 PIKE_CONCAT(very_low_sscanf_,INPUT_SHIFT)(	         \
                         PIKE_CONCAT(p_wchar, INPUT_SHIFT) *input,	 \
			 ptrdiff_t input_len,				 \
			 PCHARP match,	                                 \
			 ptrdiff_t match_len,				 \
			 ptrdiff_t *chars_matched,			 \
			 int *success, INT32 flags)			 \
{									 \
  struct svalue sval;							 \
  INT32 matches, arg;							 \
  ptrdiff_t cnt, eye, start_eye, e, field_length = 0, truncated = 0;	 \
  int no_assign = 0, minus_flag = 0, plus_flag = 0, truncate = 0;	 \
  struct sscanf_set set;						 \
									 \
									 \
  set.a = 0;								 \
  success[0] = 0;							 \
									 \
  eye = arg = matches = 0;						 \
									 \
  for(cnt = 0; cnt < match_len; cnt++)					 \
  {									 \
    for(;cnt<match_len;cnt++)						 \
    {									 \
      if(INDEX_PCHARP(match, cnt)=='%')					 \
      {									 \
        if(INDEX_PCHARP(match, cnt+1)=='%')				 \
        {								 \
          cnt++;							 \
        }else{								 \
          break;							 \
        }								 \
      }									 \
      if(eye>=input_len || input[eye]!=INDEX_PCHARP(match, cnt))	 \
      {									 \
	chars_matched[0]=eye;						 \
	return matches;							 \
      }									 \
      eye++;								 \
    }									 \
    if(cnt>=match_len)							 \
    {									 \
      chars_matched[0]=eye;						 \
      return matches;							 \
    }									 \
									 \
    DO_IF_DEBUG(							 \
    if(INDEX_PCHARP(match, cnt)!='%' || INDEX_PCHARP(match, cnt+1)=='%') \
    {									 \
      Pike_fatal("Error in sscanf.\n");					 \
    }									 \
    );									 \
									 \
    no_assign=0;							 \
    field_length=-1;							 \
    minus_flag=0;							 \
    plus_flag=0;							 \
    truncate=0;								 \
    start_eye = eye;							 \
									 \
    cnt++;								 \
    if(cnt>=match_len)							 \
      Pike_error("Error in sscanf format string.\n");			 \
									 \
    while(1)								 \
    {									 \
      switch(INDEX_PCHARP(match, cnt))					 \
      {									 \
	case '*':							 \
	  no_assign=1;							 \
	  cnt++;							 \
	  if(cnt>=match_len)						 \
	    Pike_error("Error in sscanf format string.\n");		 \
	  continue;							 \
									 \
	case '0': case '1': case '2': case '3': case '4':		 \
	case '5': case '6': case '7': case '8': case '9':		 \
	{								 \
	  PCHARP t;							 \
	  field_length = STRTOL_PCHARP(ADD_PCHARP(match, cnt), &t,10);   \
	  cnt = SUBTRACT_PCHARP(t, match);                      	 \
	  continue;							 \
	}								 \
									 \
        case '-':							 \
	  minus_flag=1;							 \
	  cnt++;							 \
	  continue;							 \
									 \
        case '+':							 \
	  plus_flag=1;							 \
	  cnt++;							 \
	  continue;							 \
									 \
        case '!':							 \
	  truncate=1;							 \
	  cnt++;							 \
	  continue;							 \
									 \
	case '{':							 \
	{								 \
	  ONERROR err;							 \
	  ptrdiff_t tmp;						 \
	  for(e=cnt+1,tmp=1;tmp;e++)					 \
	  {								 \
	    if(e>=match_len)						 \
	    {								 \
	      Pike_error("Missing %%} in format string.\n");		 \
	      break;		/* UNREACHED */				 \
	    }								 \
	    if(INDEX_PCHARP(match, e)=='%')				 \
	    {								 \
	      switch(INDEX_PCHARP(match, e+1))				 \
	      {								 \
		case '%': e++; break;					 \
		case '}': tmp--; break;					 \
		case '{': tmp++; break;					 \
	      }								 \
	    }								 \
	  }								 \
	  SET_SVAL(sval, T_ARRAY, 0, array, allocate_array(0));		 \
	  SET_ONERROR(err, do_free_array, sval.u.array);		 \
									 \
	  while(input_len-eye)						 \
	  {								 \
	    int yes;							 \
	    struct svalue *save_sp=sp;					 \
	    PIKE_CONCAT(very_low_sscanf_, INPUT_SHIFT)( \
                         input+eye,					 \
			 input_len-eye,					 \
			 ADD_PCHARP(match, cnt+1),			 \
			 e-cnt-2,					 \
			 &tmp,						 \
			 &yes, flags);					 \
	    if(yes && tmp)						 \
	    {								 \
	      f_aggregate(TO_INT32(sp-save_sp));			 \
	      sval.u.array=append_array(sval.u.array,sp-1);		 \
	      pop_stack();						 \
	      eye+=tmp;							 \
	    }else{							 \
	      pop_n_elems(sp-save_sp);					 \
	      break;							 \
	    }								 \
	  }								 \
	  cnt=e;							 \
	  UNSET_ONERROR(err);						 \
	  break;							 \
	}								 \
									 \
	case 'c':							 \
        {								 \
INPUT_IS_WIDE(								 \
          int e;							 \
)									 \
          SET_SVAL(sval, T_INT, NUMBER_NUMBER, integer, 0);		 \
          if(field_length == -1)					 \
          { 								 \
	    if(eye+1 > input_len)					 \
	    {								 \
	      chars_matched[0]=eye;					 \
	      return matches;						 \
	    }								 \
            sval.u.integer=input[eye];					 \
	    eye++;							 \
            break;							 \
          }								 \
	  if(eye+field_length > input_len)				 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
INPUT_IS_WIDE(								 \
          for(e=0;e<field_length;e++)					 \
          {								 \
	    if((unsigned INT32) input[eye+e] > 255)			\
             {								 \
               chars_matched[0]=eye;					 \
               return matches;						 \
             }								 \
          }								 \
)									 \
	  sval.u.integer=0;						 \
	  if (minus_flag)						 \
	  {								 \
	     int pos=0;						 \
	     if (field_length >= 0) {					 \
	       pos = (eye += field_length);				 \
	     }								 \
	     if (plus_flag && (--field_length >= 0)) {			 \
	       sval.u.integer = (signed char)input[--pos];		 \
	     }								 \
	     while(--field_length >= 0)					 \
	     {								 \
	       if(INT_TYPE_LSH_OVERFLOW(sval.u.integer, 8))		 \
	       {							 \
		 push_int(sval.u.integer);				 \
		 convert_stack_top_to_bignum();				 \
									 \
		 while(field_length-- >= 0)				 \
		 {							 \
		   push_int(8);						 \
		   o_lsh();						 \
		   push_int(input[--pos]);				 \
		   o_or();						 \
		 }							 \
                 dmalloc_touch_svalue(Pike_sp-1);			 \
		 sval=*--sp;						 \
		 break;							 \
	       }							 \
	       sval.u.integer<<=8;					 \
	       sval.u.integer |= input[--pos];				 \
	     }								 \
	  } else {							 \
	     if (plus_flag && (--field_length >= 0)) {			 \
	       sval.u.integer = (signed char)input[eye++];		 \
	     }								 \
	     while(--field_length >= 0)					 \
	     {								 \
	       if(INT_TYPE_LSH_OVERFLOW(sval.u.integer, 8))		 \
	       {							 \
		 push_int(sval.u.integer);				 \
		 convert_stack_top_to_bignum();				 \
									 \
		 while(field_length-- >= 0)				 \
		 {							 \
		   push_int(8);						 \
		   o_lsh();						 \
		   push_int(input[eye]);				 \
		   o_or();						 \
		   eye++;						 \
		 }							 \
                 dmalloc_touch_svalue(Pike_sp-1);			 \
		 sval=*--sp;						 \
		 break;							 \
	       }							 \
	       sval.u.integer<<=8;					 \
	       sval.u.integer |= input[eye];				 \
	       eye++;							 \
	     }								 \
	  }								 \
	  break;							 \
        }								 \
 								         \
      case 'H':								\
	{								\
          unsigned long len=0;                                           \
          if(field_length == -1)					\
	    field_length=1;						\
          if(field_length == 0)                                         \
            Pike_error("%%H size field is 0.\n");                       \
          if(eye+field_length > input_len)				\
	  {								\
	    chars_matched[0]=eye;					\
	    return matches;						\
	  }								\
	  INPUT_IS_WIDE (						\
	    for(e=0;e<field_length;e++)					\
	    {								\
	      if((unsigned INT32) input[eye+e] > 255)			\
	      {								\
		chars_matched[0]=eye;					\
		return matches;						\
	      }								\
	    }								\
	  );								\
	  if (minus_flag)						\
	  {								\
	    int pos=0;							\
	    pos = (eye += field_length);				\
            while(--field_length >= 0)					\
	    {								\
	      len<<=8;							\
	      len |= input[--pos];					\
	    }								\
	  } else {							\
	    while(--field_length >= 0)					\
	    {								\
	      len<<=8;							\
	      len |= input[eye];					\
	      eye++;							\
	    }								\
	  }								\
	  if(len > (unsigned long)(input_len-eye))                      \
	  {								\
	    chars_matched[0]=eye-field_length;				\
	    return matches;						\
	  }								\
	  if (no_assign) {						\
	    no_assign = 2;						\
	  } else {							\
	    SET_SVAL(sval, T_STRING, 0, string,				\
		     PIKE_CONCAT(make_shared_binary_string,		\
				 INPUT_SHIFT)(input+eye, len));		\
	  }								\
	  eye+=len;							\
	  break;							\
        }								\
									 \
        case 'b':							 \
        case 'o':							 \
        case 'd':							 \
        case 'x':							 \
        case 'D':							 \
        case 'i':							 \
	{								 \
	  int base = 0;							 \
	  PIKE_CONCAT(p_wchar, INPUT_SHIFT) *t;				 \
									 \
	  if(eye>=input_len)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
									 \
	  switch(INDEX_PCHARP(match, cnt))				 \
	  {								 \
	  case 'b': base =  2; break;					 \
	  case 'o': base =  8; break;					 \
	  case 'd': base = 10; break;					 \
	  case 'x': base = 16; break;					 \
	  }								 \
									 \
	  wide_string_to_svalue_inumber(&sval, input+eye, &t,		 \
					base, field_length,		 \
					INPUT_SHIFT);			 \
									 \
	  if(input + eye == t)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  eye=t-input;							 \
	  break;							 \
	}								 \
									 \
        case 'f':							 \
	{								 \
	  PIKE_CONCAT(p_wchar, INPUT_SHIFT) *t;				 \
	  PCHARP t2;							 \
	  FLOAT_TYPE f;							 \
									 \
	  if(eye>=input_len)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  f = (FLOAT_TYPE)STRTOD_PCHARP(MKPCHARP(input+eye,		 \
						 INPUT_SHIFT),&t2);	 \
	  t = (PIKE_CONCAT(p_wchar, INPUT_SHIFT) *)(t2.ptr);		 \
	  if(input + eye == t)						 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  eye=t-input;							 \
	  SET_SVAL(sval, T_FLOAT, 0, float_number, f);			 \
	  break;							 \
	}								 \
									 \
	case 'F':							 \
	  if(field_length == -1) field_length = 4;			 \
	  if(field_length != 4 && field_length != 8)			 \
	    Pike_error("Invalid IEEE width %ld in sscanf format string.\n",	 \
		  PTRDIFF_T_TO_LONG(field_length));			 \
	  if(eye+field_length > input_len)				 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  SET_SVAL(sval, T_FLOAT, 0, float_number, 0.0);		 \
	  switch(field_length) {					 \
	    case 4:							 \
              if (minus_flag) EXTRACT_FLOAT(sval, input+eye, INPUT_SHIFT, extract_float_le);    \
              else EXTRACT_FLOAT(sval, input+eye, INPUT_SHIFT, extract_float_be);               \
	      eye += 4;							 \
	      break;							 \
	    case 8:							 \
              if (minus_flag) EXTRACT_DOUBLE(sval, input+eye, INPUT_SHIFT, extract_double_le);    \
              else EXTRACT_DOUBLE(sval, input+eye, INPUT_SHIFT, extract_double_be);               \
	      eye += 8;							 \
	      break;							 \
	  }								 \
	  break;							 \
									 \
	case 's':							 \
	  if(field_length != -1)					 \
	  {								 \
	    if(input_len - eye < field_length)				 \
	    {								 \
	      chars_matched[0]=eye;					 \
	      return matches;						 \
	    }								 \
									 \
	    if (no_assign) {						 \
	      no_assign = 2;						 \
	    } else {							 \
	      SET_SVAL(sval, T_STRING, 0, string,			 \
		       PIKE_CONCAT(make_shared_binary_string,		 \
				   INPUT_SHIFT)(input+eye,		 \
						field_length));		 \
	    }								 \
	    eye+=field_length;						 \
	    break;							 \
	  }								 \
									 \
	  if(cnt+1>=match_len)						 \
	  {								 \
	    if (no_assign) {						 \
	      no_assign = 2;						 \
	    } else {							 \
	      SET_SVAL(sval, T_STRING, 0, string,			 \
		       PIKE_CONCAT(make_shared_binary_string,		 \
				   INPUT_SHIFT)(input+eye,		 \
						input_len-eye));	 \
	    }								 \
	    eye=input_len;						 \
	    break;							 \
	  }else{							 \
            PCHARP end_str_start, end_str_end, s;                        \
	    int contains_percent_percent;				 \
            ptrdiff_t start, new_eye;					 \
									 \
	    e = cnt;							 \
	    start=eye;							 \
	    end_str_start=ADD_PCHARP(match, cnt+1);			 \
									 \
	    s=end_str_start;    					 \
      test_again:							 \
	    if(EXTRACT_PCHARP(s)=='%')					 \
	    {								 \
	      INC_PCHARP(s, 1);						 \
	      if(EXTRACT_PCHARP(s)=='*') INC_PCHARP(s, 1);		 \
              set.neg=0;						 \
	      switch(EXTRACT_PCHARP(s))					 \
	      {								 \
                case 0:							 \
                  /* FIXME: Should really look at the match len */	 \
		  Pike_error("%% without conversion specifier.\n");	 \
                  break;						 \
									 \
		case 'n':						 \
		  INC_PCHARP(s, 1);					 \
                  /* Advance the end string start pointer */		 \
                  end_str_start = s;					 \
		  e = SUBTRACT_PCHARP(s, match);			 \
	          goto test_again;					 \
									 \
		case 's':						 \
		  Pike_error("Illegal to have two adjecent %%s.\n");	 \
		  return 0;		/* make gcc happy */		 \
									 \
	  /* sscanf("foo-bar","%s%d",a,b) might not work as expected */	 \
		case 'd':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['-']=0;						 \
		  goto match_set;					 \
									 \
		case 'o':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='7';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'x':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  for(e='a';e<='f';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'D':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['-']=0;						 \
		  goto match_set;					 \
									 \
		case 'f':						 \
		  MEMSET(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['.']=set.c['-']=0;				 \
		  goto match_set;					 \
									 \
		case '[':		/* oh dear */			 \
		  read_set(match, SUBTRACT_PCHARP(s, match)+1,           \
                           &set, match_len, flags);	                 \
		  set.neg=!set.neg;					 \
		  goto match_set;					 \
	      }								 \
	    }								 \
									 \
	    contains_percent_percent=0;					 \
									 \
	    for(;e<match_len;e++)					 \
	    {								 \
	      if(INDEX_PCHARP(match, e)=='%')				 \
	      {								 \
		if(INDEX_PCHARP(match, e+1)=='%')			 \
		{							 \
		  contains_percent_percent=1;				 \
		  e++;							 \
		}else{							 \
		  break;						 \
		}							 \
	      }								 \
	    }								 \
									 \
	    end_str_end=ADD_PCHARP(match, e);				 \
									 \
	    if (COMPARE_PCHARP(end_str_end, ==,  end_str_start)) {	 \
	      if (no_assign) {						 \
		no_assign = 2;						 \
	      } else {							 \
		SET_SVAL(sval, T_STRING, 0, string,			 \
			 PIKE_CONCAT(make_shared_binary_string,		 \
				     INPUT_SHIFT)(input+eye,		 \
						  input_len-eye));	 \
	      }								 \
	      eye=input_len;						 \
	      break;							 \
	    } else if(!contains_percent_percent)			 \
	    {								 \
	      struct pike_mem_searcher searcher;			 \
	      PIKE_CONCAT(p_wchar, INPUT_SHIFT) *s2;			 \
	      pike_init_memsearch(&searcher, end_str_start,	         \
                                  SUBTRACT_PCHARP(end_str_end, end_str_start),	 \
                                  input_len - eye);		         \
              s2 = searcher.mojt.vtab-> PIKE_CONCAT(func,INPUT_SHIFT)	 \
                     (searcher.mojt.data, input+eye, input_len-eye);	 \
	      if(!s2)							 \
	      {								 \
		chars_matched[0]=eye;					 \
		return matches;						 \
	      }								 \
	      eye=s2-input;						 \
	      new_eye=eye+SUBTRACT_PCHARP(end_str_end, end_str_start);	 \
	    }else{							 \
	      PIKE_CONCAT(p_wchar, INPUT_SHIFT) *p2 = NULL;		 \
	      for(;eye<input_len;eye++)					 \
	      {								 \
		p2=input+eye;						 \
		for(s=end_str_start;COMPARE_PCHARP(s, <, end_str_end);INC_PCHARP(s, 1),p2++)		 \
		{							 \
		  if(EXTRACT_PCHARP(s)!=*p2) break;			 \
		  if(EXTRACT_PCHARP(s)=='%') INC_PCHARP(s, 1);		 \
		}							 \
		if(COMPARE_PCHARP(s, ==, end_str_end))			 \
		  break;						 \
	      }								 \
	      if(eye==input_len)					 \
	      {								 \
		chars_matched[0]=eye;					 \
		return matches;						 \
	      }								 \
	      new_eye=p2-input;						 \
	    }								 \
									 \
	    if (no_assign) {						 \
	      no_assign = 2;						 \
	    } else {							 \
	      SET_SVAL(sval, T_STRING, 0, string,			 \
		       PIKE_CONCAT(make_shared_binary_string,		 \
				   INPUT_SHIFT)(input+start,		 \
						eye-start));		 \
	    }								 \
									 \
	    cnt=SUBTRACT_PCHARP(end_str_end, match)-1;			 \
	    eye=new_eye;						 \
	    break;							 \
	  }								 \
									 \
	case '[':							 \
	  cnt=read_set(match,cnt+1, &set,match_len, flags);	         \
									 \
	match_set:							 \
	  {								 \
	    int len = input_len;					 \
	    if (field_length != -1) {					 \
	      len = eye + field_length;					 \
	      if (len > input_len) {					 \
	  	/* Mismatch -- too little data */			 \
	        chars_matched[0]=eye;					 \
	        return matches;						 \
	      }								 \
	    }								 \
	    for(e=eye;eye<len;eye++)					 \
	    {								 \
INPUT_IS_WIDE(								 \
	      if((unsigned INT32) input[eye] < sizeof(set.c))		\
	      {								 \
)									 \
	        if(set.c[input[eye]] == set.neg)			 \
		  break;						 \
INPUT_IS_WIDE(								 \
	      }else{							 \
	        if(set.a)						 \
	        {							 \
		  INT32 x;						 \
		  struct svalue tmp;					 \
		  SET_SVAL(tmp, T_INT, NUMBER_NUMBER,			 \
			   integer, input[eye]);			 \
		  x=switch_lookup(set.a, &tmp);				 \
		  if( set.neg != (x<0 && (x&1)) ) break;		 \
	        }else{							 \
		  if(!set.neg) break;					 \
	        }							 \
	      }								 \
)									 \
	    }								 \
	    if ((field_length != -1) && (eye != len)) {			 \
	      /* Couldn't read the entire field. Fail. */		 \
	      chars_matched[0]=e;					 \
	      return matches;						 \
	    }								 \
	  }								 \
          if(set.a) { free_array(set.a); set.a=0; }			 \
	  if (no_assign) {						 \
	    no_assign = 2;						 \
	  } else {							 \
	    SET_SVAL(sval, T_STRING, 0, string,				 \
		     PIKE_CONCAT(make_shared_binary_string,		 \
				 INPUT_SHIFT)(input+e,eye-e));		 \
	  }								 \
	  break;							 \
									 \
        case 'O':							 \
        {								 \
 	  PIKE_CONCAT(p_wchar, INPUT_SHIFT) *cont;			 \
          if(eye>=input_len)						 \
          {								 \
            chars_matched[0]=eye;					 \
            return matches;						 \
          }								 \
	  if ((field_length == -1) ||					 \
	      ((input_len - eye) < field_length))			 \
	    field_length = input_len - eye;				 \
									 \
          cont =							 \
	    pcharp_to_svalue_percent_o(&sval,				 \
				       MKPCHARP(input+eye, INPUT_SHIFT), \
				       field_length);			 \
          if(!cont)							 \
          {								 \
            chars_matched[0]=eye;					 \
            return matches;						 \
          }								 \
          eye = cont-input;						 \
									 \
          break;							 \
        }								 \
									 \
	case 'n':							 \
	  SET_SVAL(sval, T_INT, NUMBER_NUMBER, integer,			 \
		   TO_INT32(eye - truncated));				 \
	  break;							 \
									 \
	default:							 \
	  Pike_error("Unknown sscanf token %%%c(0x%02x)\n",		 \
		INDEX_PCHARP(match, cnt), INDEX_PCHARP(match, cnt));	 \
      }									 \
      break;								 \
    }									 \
    matches++;								 \
    if (truncate) {							 \
      truncated += eye - start_eye;					 \
    }									 \
									 \
    if(no_assign)							 \
    {									 \
      if (no_assign == 1)						 \
	free_svalue(&sval);						 \
    } else {								 \
      check_stack(1);							 \
      *sp++=sval;							 \
      dmalloc_touch_svalue(Pike_sp-1);					 \
      DO_IF_DEBUG(INVALIDATE_SVAL(sval));				 \
    }									 \
  }									 \
  chars_matched[0]=eye;							 \
  success[0]=1;								 \
  return matches;							 \
}


/* Confusing? Yes - Hubbe */
/* Now slightly less confusing macro names, at least. /mast */

/* INPUT_IS_WIDE(X) is X if the input is wide.  */
#define INPUT_IS_WIDE(X)

MK_VERY_LOW_SSCANF(0)

#undef INPUT_IS_WIDE
#define INPUT_IS_WIDE(X) X

MK_VERY_LOW_SSCANF(1)
MK_VERY_LOW_SSCANF(2)

/* Simplified interface to very_low_sscanf_{0,1,2}_{0,1,2}(). */
INT32 low_sscanf(struct pike_string *data, struct pike_string *format, INT32 flags)
{
  ptrdiff_t matched_chars;
  int x;
  INT32 i;
  PCHARP match = MKPCHARP_STR(format);

  check_c_stack(sizeof(struct sscanf_set)*2 + 512);

  switch(data->size_shift) {
  case 0:
    i = very_low_sscanf_0(STR0(data), data->len,
			  match, format->len,
			  &matched_chars, &x, flags);
    break;
  case 1:
    i = very_low_sscanf_1(STR1(data), data->len,
			  match, format->len,
			  &matched_chars, &x, flags);
    break;
#ifdef PIKE_DEBUG
  default:
    Pike_fatal("Unsupported shift-combination to low_sscanf(): %d:%d\n",
	       data->size_shift, format->size_shift);
    break;
  case 2:
#else
  default:
#endif
    i = very_low_sscanf_2(STR2(data), data->len,
			  match, format->len,
			  &matched_chars, &x, flags);
    break;
  }
  return i;
}

/*! @decl int sscanf(string data, string format, mixed ... lvalues)
 *!
 *! The purpose of sscanf is to match a string @[data] against a @[format]
 *! string and place the matching results into a list of variables. The list
 *! of @[lvalues] are destructively modified (which is only possible because
 *! sscanf really is an opcode, rather than a pike function) with the values
 *! extracted from the @[data] according to the @[format] specification. Only
 *! the variables up to the last matching directive of the format string are
 *! touched.
 *!
 *! The @[format] string can contain strings separated by special matching
 *! directives like @tt{%d@}, @tt{%s@} @tt{%c@} and @tt{%f@}. Every such
 *! directive corresponds to one of the @[lvalues], in order they are listed.
 *! An lvalue is the name of a variable, a name of a local variable, an index
 *! in an array, mapping or object. It is because of these lvalues that sscanf
 *! can not be implemented as a normal function.
 *!
 *! Whenever a percent character is found in the format string, a match is
 *! performed, according to which operator and modifiers follow it:
 *!
 *! @string
 *!   @value "%b"
 *!     Reads a binary integer (@expr{"0101"@} makes @expr{5@})
 *!   @value "%d"
 *!     Reads a decimal integer (@expr{"0101"@} makes @expr{101@}).
 *!   @value "%o"
 *!     Reads an octal integer (@expr{"0101"@} makes @expr{65@}).
 *!   @value "%x"
 *!     Reads a hexadecimal integer (@expr{"0101"@} makes @expr{257@}).
 *!   @value "%D"
 *!     Reads an integer that is either octal (leading zero),
 *!     hexadecimal (leading 0x) or decimal. (@expr{"0101"@} makes
 *!     @expr{65@}).
 *!   @value "%c"
 *!     Reads one character and returns it as an integer
 *!     (@expr{"0101"@} makes @expr{48@}, or @expr{'0'@}, leaving
 *!     @expr{"101"@} for later directives). Using the field width and
 *!     endianness modifiers, you can decode integers of any size and
 *!     endianness. For example @expr{"%-2c"@} decodes @expr{"0101"@}
 *!     into @expr{12592@}, leaving @expr{"01"@} fot later directives.
 *!     The sign modifiers can be used to modify the signature of the
 *!     data, making @expr{"%+1c"@} decode @expr{"ä"@} into
 *!     @expr{-28@}.
 *!   @value "%n"
 *!     Returns the current character offset in @[data].
 *!     Note that any characters matching fields scanned with the
 *!     @expr{"!"@}-modifier are removed from the count (see below).
 *!   @value "%f"
 *!     Reads a float ("0101" makes 101.0).
 *!   @value "%F"
 *!     Reads a float encoded according to the IEEE single precision
 *!     binary format (@expr{"0101"@} makes @expr{6.45e-10@},
 *!     approximately). Given a field width modifier of 8 (4 is the
 *!     default), the data will be decoded according to the IEEE
 *!     double precision binary format instead. (You will however
 *!     still get a float, unless your pike was compiled with the
 *!     configure argument @tt{--with-double-precision@}.)
 *!   @value "%s"
 *!     Reads a string. If followed by %d, %s will only read non-numerical
 *!     characters. If followed by a %[], %s will only read characters not
 *!     present in the set. If followed by normal text, %s will match all
 *!     characters up to but not including the first occurrence of that text.
 *!   @value "%H"
 *!     Reads a Hollerith-encoded string, i.e. first reads the length
 *!     of the string and then that number of characters. The size and
 *!     byte order of the length descriptor can be modified in the
 *!     same way as @tt{%c@}. As an example @expr{"%2H"@} first reads
 *!     @expr{"%2c"@} and then the resulting number of characters.
 *!   @value "%[set]"
 *!     Matches a string containing a given set of characters (those given
 *!     inside the brackets). Ranges of characters can be defined by using
 *!     a minus character between the first and the last character to be
 *!     included in the range. Example: %[0-9H] means any number or 'H'.
 *!     Note that sets that includes the character '-' must have it first
 *!     (not possible in complemented sets, see below) or last in the brackets
 *!     to avoid having a range defined. Sets including the character ']' must
 *!     list this first too. If both '-' and ']' should be included
 *!     then put ']' first and '-' last.  It is not possible to make a range
 *!     that ends with ']'; make the range end with '\' instead and put ']'
 *!     at the beginning of the set.  Likewise it is generally not possible
 *!     to have a range start with '-'; make the range start with '.' instead
 *!     and put '-' at the end of the set.  If the first character after the
 *!     [ bracket is '^' (%[^set]), and this character does not begin a
 *!     range, it means that the set is complemented, which is to say that
 *!     any character except those inside brackets is matched.  To include '-'
 *!     in a complemented set, it must be put last, not first.  To include '^'
 *!     in a non-complemented set, it can be put anywhere but first, or be
 *!     specified as a range ("^-^").
 *!   @value "%{format%}"
 *!     Repeatedly matches 'format' as many times as possible and assigns an
 *!     array of arrays with the results to the lvalue.
 *!   @value "%O"
 *!     Match a Pike constant, such as string or integer (currently only
 *!     integer, string and character constants are functional).
 *!   @value "%%"
 *!     Match a single percent character (hence this is how you quote the %
 *!     character to just match, and not start an lvalue matcher directive).
 *! @endstring
 *!
 *! Similar to @[sprintf], you may supply modifiers between the % character
 *! and the operator, to slightly change its behaviour from the default:
 *!
 *! @string
 *!   @value "*"
 *!     The operator will only match its argument, without assigning any
 *!     variable.
 *!   @value number
 *!     You may define a field width by supplying a numeric modifier.
 *!     This means that the format should match that number of
 *!     characters in the input data; be it a @i{number@} characters
 *!     long string, integer or otherwise (@expr{"0101"@} using the
 *!     format %2c would read an unsigned short @expr{12337@}, leaving
 *!     the final @expr{"01"@} for later operators, for instance).
 *!   @value "-"
 *!     Supplying a minus sign toggles the decoding to read the data encoded
 *!     in little-endian byte order, rather than the default network
 *!     (big-endian) byte order.
 *!   @value "+"
 *!     Interpret the data as a signed entity. In other words,
 *!     @expr{"%+1c"@} will read @expr{"\xFF"@} as @expr{-1@} instead
 *!     of @expr{255@}, as @expr{"%1c"@} would have.
 *!   @value "!"
 *!     Ignore the matched characters with respect to any following
 *!     @expr{"%n"@}.
 *! @endstring
 *!
 *! @note
 *! Sscanf does not use backtracking. Sscanf simply looks at the format string
 *! up to the next % and tries to match that with the string. It then proceeds
 *! to look at the next part. If a part does not match, sscanf immediately
 *! returns how many % were matched. If this happens, the lvalues for % that
 *! were not matched will not be changed.
 *!
 *! @example
 *! // a will be assigned "oo" and 1 will be returned
 *! sscanf("foo", "f%s", a);
 *!
 *! // a will be 4711 and b will be "bar", 2 will be returned
 *! sscanf("4711bar", "%d%s", a, b);
 *!
 *! // a will be 4711, 2 will be returned
 *! sscanf("bar4711foo", "%*s%d", a);
 *!
 *! // a will become "test", 2 will be returned
 *! sscanf(" \t test", "%*[ \t]%s", a);
 *!
 *! // Remove "the " from the beginning of a string
 *! // If 'str' does not begin with "the " it will not be changed
 *! sscanf(str, "the %s", str);
 *!
 *! // It is also possible to declare a variable directly in the sscanf call;
 *! // another reason for sscanf not to be an ordinary function:
 *!
 *! sscanf("abc def", "%s %s", string a, string b);
 *!
 *! @returns
 *!   The number of directives matched in the format string. Note that a string
 *!   directive (%s or %[]) counts as a match even when matching just the empty
 *!   string (which either may do).
 *! @seealso
 *!   @[sprintf], @[array_sscanf]
 *    @[parse_format]
 */
void o_sscanf(INT32 args, INT32 flags)
{
  INT32 i=0;
  int x;
  struct svalue *save_sp=sp;

  if(TYPEOF(sp[-args]) != T_STRING)
    SIMPLE_BAD_ARG_ERROR("sscanf", 1, "string");

  if(TYPEOF(sp[1-args]) != T_STRING)
    SIMPLE_BAD_ARG_ERROR("sscanf", 2, "string");

  i = low_sscanf(sp[-args].u.string, sp[1-args].u.string, flags);

  if(sp-save_sp > args/2-1)
    Pike_error("Too few arguments for sscanf format.\n");

  for(x=0;x<sp-save_sp;x++)
    assign_lvalue(save_sp-args+2+x*2,save_sp+x);
  pop_n_elems(sp-save_sp +args);

#ifdef PIKE_DEBUG
  if(Pike_interpreter.trace_level >2)
  {
    int nonblock;
    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    fprintf(stderr,"-    Matches: %ld\n",(long)i);
    if(nonblock)
      set_nonblocking(2,1);
  }
#endif
  push_int(i);
}

/*! @decl array array_sscanf(string data, string format)
 *!
 *! This function works just like @[sscanf()], but returns the matched
 *! results in an array instead of assigning them to lvalues. This is often
 *! useful for user-defined sscanf strings.
 *!
 *! @seealso
 *!   @[sscanf()], @[`/()]
 */
PMOD_EXPORT void f_sscanf(INT32 args)
{
  INT32 i;
  struct svalue *save_sp=sp;
  struct array *a;

  check_all_args("array_sscanf",args,BIT_STRING, BIT_STRING,0);

  i = low_sscanf(sp[-args].u.string, sp[1-args].u.string, 0);

  a = aggregate_array(DO_NOT_WARN(sp - save_sp));
  pop_n_elems(args);
  push_array(a);
}

void f_sscanf_76(INT32 args)
{
  INT32 i;
  struct svalue *save_sp=sp;
  struct array *a;

  check_all_args("array_sscanf",args,BIT_STRING, BIT_STRING,0);

  i = low_sscanf(sp[-args].u.string, sp[1-args].u.string, SSCANF_FLAG_76_COMPAT);

  a = aggregate_array(DO_NOT_WARN(sp - save_sp));
  pop_n_elems(args);
  push_array(a);
}

static void push_sscanf_argument_types(PCHARP format, ptrdiff_t format_len,
				       int cnt, int flags)
{
  for(; cnt < format_len; cnt++)
  {
    int no_assign=0;

    while((cnt<format_len) && (INDEX_PCHARP(format, cnt) != '%'))
      cnt++;
    cnt++;
    if (cnt >= format_len) break;

    while(1)
    {
      switch(INDEX_PCHARP(format, cnt))
      {
      case '%':
	break;

      case '*':
	no_assign=1;
	/* FALL_THROUGH */

      case '-':
      case '+':
      case '!':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	cnt++;
	if(cnt>=format_len) {
	  yyreport(REPORT_ERROR, type_check_system_string,
		   0, "Error in sscanf format string.");
	  break;
	}
	continue;

	case '{':
	{
	  int e;
	  int depth = 1;
	  for(e=cnt+1;depth;e++)
	  {
	    if(e>=format_len)
	    {
	      yyreport(REPORT_ERROR, type_check_system_string,
		       0, "Missing %%} in format string.");
	      break;
	    }
	    if(INDEX_PCHARP(format, e)=='%')
	    {
	      switch(INDEX_PCHARP(format, e+1))
	      {
		case '%': e++; break;
		case '}': depth--; break;
		case '{': depth++; break;
	      }
	    }
	  }
	  if (!no_assign) {
	    type_stack_mark();
	    push_sscanf_argument_types(format, e, cnt+1, flags);
	    if (!(depth = pop_stack_mark())) {
	      push_type(PIKE_T_ZERO);
	    } else {
	      /* Join the argument types. */
	      while (depth > 1) {
		push_type(T_OR);
		depth--;
	      }
	    }
	    push_type(PIKE_T_ARRAY);
	    push_type(PIKE_T_ARRAY);
	  }
	  cnt = e;
	  break;
	}

	case 'n':
        case 'b':
        case 'o':
        case 'd':
        case 'x':
        case 'D':
        case 'i':
	case 'c':
	  if (!no_assign)
	    push_finished_type(int_type_string);
	  break;

	case '[':
	  {
	    int ch;
	    cnt++;
	    if (cnt >= format_len) {
	      yyreport(REPORT_ERROR, type_check_system_string,
		       0, "Error in sscanf format string.");
	      break;
	    }
	    if((INDEX_PCHARP(format, cnt)=='^') &&
	       (cnt+2>=format_len ||
		(INDEX_PCHARP(format, cnt+1)!='-') ||
		(INDEX_PCHARP(format, cnt+2)==']') ||
		(flags & SSCANF_FLAG_76_COMPAT)))
	    {
	      cnt++;
	      if(cnt >= format_len) {
		yyreport(REPORT_ERROR, type_check_system_string,
			 0, "Error in sscanf format string.");
		break;
	      }
	    }

	    if(((ch = INDEX_PCHARP(format, cnt))==']') || (ch=='-'))
	    {
	      cnt++;
	      if(cnt >= format_len) {
		yyreport(REPORT_ERROR, type_check_system_string,
			 0, "Error in sscanf format string.");
		break;
	      }
	      ch = INDEX_PCHARP(format, cnt);
	    }

	    while(ch != ']')
	    {
	      if(ch == '-')
	      {
		cnt++;
		if(cnt >= format_len) {
		  yyreport(REPORT_ERROR, type_check_system_string,
			   0, "Error in sscanf format string.");
		  break;
		}

		if(INDEX_PCHARP(format, cnt)==']')
		{
		  break;
		}
	      }
	      cnt++;
	      if(cnt>=format_len) {
		yyreport(REPORT_ERROR, type_check_system_string,
			 0, "Error in sscanf format string.");
		break;
	      }
	      ch = INDEX_PCHARP(format, cnt);
	    }
	  }
	  /* FALL_THROUGH */
      case 's':
      case 'H':
	if (!no_assign)
	  push_finished_type(string_type_string);
	break;

	case 'F':
        case 'f':
	  if (!no_assign)
	    push_finished_type(float_type_string);
	  break;

	  break;

        case 'O':
	  if (!no_assign)
	    push_finished_type(mixed_type_string);
	  break;

	default:
	  yyreport(REPORT_ERROR, type_check_system_string,
		   0, "Unknown sscanf token %%%c(0x%02x) at offset %d.",
		   INDEX_PCHARP(format, cnt), INDEX_PCHARP(format, cnt),
		   cnt-1);
	  break;
      }
      break;
    }
  }
}

void f___handle_sscanf_format(INT32 args)
{
  struct pike_type *res;
  struct pike_type *tmp;
  struct pike_string *attr;
  struct pike_string *fmt;
  struct pike_string *sscanf_format_string;
  struct pike_string *sscanf_76_format_string;
  int flags = 0;
  int found = 0;
  int fmt_count;

#if 0
  fprintf(stderr, "__handle_sprintf_format()\n");
#endif /* 0 */
  if (args != 4)
    SIMPLE_WRONG_NUM_ARGS_ERROR("__handle_sscanf_format", 4);
  if (TYPEOF(Pike_sp[-4]) != PIKE_T_STRING)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 1, "string");
  if (TYPEOF(Pike_sp[-3]) != PIKE_T_STRING)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 2, "string");
  if (TYPEOF(Pike_sp[-2]) != PIKE_T_TYPE)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 3, "type");
  if (TYPEOF(Pike_sp[-1]) != PIKE_T_TYPE)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 4, "type");

  tmp = Pike_sp[-1].u.type;
  if ((tmp->type != PIKE_T_FUNCTION) && (tmp->type != T_MANY)) {
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 4, "type(function)");
  }

  MAKE_CONST_STRING(sscanf_format_string, "sscanf_format");  
  MAKE_CONST_STRING(sscanf_76_format_string, "sscanf_76_format");  

  if (Pike_sp[-4].u.string != sscanf_format_string) {
    if (Pike_sp[-4].u.string != sscanf_76_format_string) {
      pop_n_elems(args);
      push_undefined();
      return;
    }
    flags = SSCANF_FLAG_76_COMPAT;
  }

  fmt = Pike_sp[-3].u.string;
  MAKE_CONST_STRING(attr, "sscanf_args");  

#if 0
  fprintf(stderr, "Checking sscanf format: \"%s\": ", fmt->str);
  simple_describe_type(Pike_sp[-1].u.type);
  fprintf(stderr, "\n");
#endif /* 0 */

  type_stack_mark();
  type_stack_mark();
  for (; tmp; tmp = tmp->cdr) {
    struct pike_type *arg = tmp->car;
    int array_cnt = 0;
    while(arg) {
      switch(arg->type) {
      case PIKE_T_ATTRIBUTE:
	if (arg->car == (struct pike_type *)attr)
	  break;
	/* FALL_THROUGH */
      case PIKE_T_NAME:
	arg = arg->cdr;
	continue;
      case PIKE_T_ARRAY:
	array_cnt++;
	arg = arg->car;
	continue;
      default:
	arg = NULL;
	break;
      }
      break;
    }
    if (arg) {
      type_stack_mark();
      push_sscanf_argument_types(MKPCHARP(fmt->str, fmt->size_shift),
				 fmt->len, 0, flags);
      if (!array_cnt) {
	pop_stack_mark();
	push_type(T_VOID);	/* No more args */
	while (tmp->type == PIKE_T_FUNCTION) {
	  tmp = tmp->cdr;
	}
	push_finished_type(tmp->cdr);	/* return type */
	push_reverse_type(T_MANY);
	fmt_count = pop_stack_mark();
	while (fmt_count > 1) {
	  push_reverse_type(T_FUNCTION);
	  fmt_count--;
	}
	res = pop_unfinished_type();
	pop_n_elems(args);
	push_type_value(res);
#if 0
	fprintf(stderr, " ==> ");
	simple_describe_type(res);
	fprintf(stderr, "\n");
#endif /* 0 */
	return;
      } else {
	if (!(fmt_count = pop_stack_mark())) {
	  push_type(PIKE_T_ZERO);
	} else {
	  /* Join the argument types into the array. */
	  while (fmt_count > 1) {
	    push_type(T_OR);
	    fmt_count--;
	  }
	}
	while (array_cnt--) {
	  push_type(PIKE_T_ARRAY);
	}
	found = 1;
      }
    } else {
      push_finished_type(tmp->car);
    }
    if (tmp->type == T_MANY) {
      /* Get the return type. */
      tmp = tmp->cdr;
      break;
    }
  }
  if (found) {
    /* Found, but inside an array, so we need to build the function
     * type here.
     */
    push_finished_type(tmp);	/* Return type. */
    push_reverse_type(T_MANY);
    fmt_count = pop_stack_mark();
    while (fmt_count > 1) {
      push_reverse_type(T_FUNCTION);
      fmt_count--;
    }
    res = pop_unfinished_type();
    pop_n_elems(args);
    push_type_value(res);
#if 0
    fprintf(stderr, " ==> ");
    simple_describe_type(res);
    fprintf(stderr, "\n");
#endif /* 0 */
    return;
  } else if (tmp) {
    /* Check if it's in the return type. */
    struct pike_type *arg = tmp;
    int array_cnt = 0;
    while(arg) {
      switch(arg->type) {
      case PIKE_T_ATTRIBUTE:
	if (arg->car == (struct pike_type *)attr)
	  break;
	/* FALL_THROUGH */
      case PIKE_T_NAME:
	arg = arg->cdr;
	continue;
      case PIKE_T_ARRAY:
	array_cnt++;
	arg = arg->car;
	continue;
      default:
	arg = NULL;
	break;
      }
      break;
    }
    if (arg) {
      type_stack_mark();
      push_sscanf_argument_types(MKPCHARP(fmt->str, fmt->size_shift),
				 fmt->len, 0, flags);
      /* Join the argument types. */
      if (!(fmt_count = pop_stack_mark())) {
	push_type(PIKE_T_ZERO);
      } else while (fmt_count > 2) {
	push_type(T_OR);
	fmt_count--;
      }
      while (array_cnt--) {
	push_type(PIKE_T_ARRAY);
      }

      /* Rebuild the function type backwards. */
      push_reverse_type(T_MANY);
      fmt_count = pop_stack_mark();
      while (fmt_count > 1) {
	push_reverse_type(T_FUNCTION);
	fmt_count--;
      }
      res = pop_unfinished_type();
      pop_n_elems(args);
      push_type_value(res);
#if 0
      fprintf(stderr, " ==> ");
      simple_describe_type(res);
      fprintf(stderr, "\n");
#endif /* 0 */
      return;
    }
  }

  /* No marker found. */
#if 0
  fprintf(stderr, " ==> No marker found.\n");
#endif /* 0 */
  pop_stack_mark();
  type_stack_pop_to_mark();
  pop_n_elems(args);
  push_undefined();
}


