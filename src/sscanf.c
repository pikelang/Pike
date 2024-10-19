/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "interpret.h"
#include "array.h"
#include "mapping.h"
#include "multiset.h"
#include "stralloc.h"
#include "pike_error.h"
#include "fd_control.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "operators.h"
#include "bignum.h"
#include "pike_compiler.h"
#include "pike_float.h"
#include "pike_types.h"
#include "sscanf.h"
#include "bitvector.h"
#include "pike_search.h"

/*
 * helper functions for sscanf %O
 */

/* Calling convention:
 *   res: svalue to fill in.
 *   str: string to parse.
 *   len: length of the string.
 *
 * Returns:
 *   NULL on failure.
 *   continuation point in str on success.
 */

#define CONSUME(X) do{if(*len>=X){INC_PCHARP(*str,X);*len-=X;}}while(0)
static p_wchar2 next_char( PCHARP *str, ptrdiff_t *len )
{
  p_wchar2 res = *len ? EXTRACT_PCHARP(*str) : 0;
  CONSUME(1);
  return res;
}
#define READ() next_char(str,len)

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#define NOINLINE_UNALIGNED
#else
/* Workaround for gcc 4.7.4 and others "optimizing" away calls to memcpy(),
 * and replacing them with direct (unaligned) memory accesses.
 * This generates broken code for eg %F on sparc.
 * cf https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50569
 * Note that the patch for the above bug is in gcc 4.7.4, but isn't sufficient.
 */
#define NOINLINE_UNALIGNED	ATTRIBUTE((noinline)) DECLSPEC(noinline)
#endif

#if SIZEOF_FLOAT_TYPE > SIZEOF_DOUBLE
#define STRTOFLOAT_PCHARP STRTOLD_PCHARP
#else
#define STRTOFLOAT_PCHARP STRTOD_PCHARP
#endif

static void skip_comment( PCHARP *str, ptrdiff_t *len )
{
  CONSUME(1); // Start '/' 
  switch(READ())
  {
    case '*':
      while(*len)
      {
	while( READ() != '*' )
	  ;
	if( READ() == '/' )
	  return;
      }
      break;

    case '/':
      while( *len && READ()!= '\n' )
	;
  }
}

static void skip_to_token( PCHARP *str, ptrdiff_t *len )
{
  int worked;
  do
  {
    worked=0;
    while(*len && wide_isspace(EXTRACT_PCHARP(*str)))
    {
      worked = 1;
      CONSUME(1);
    }
    if( EXTRACT_PCHARP(*str) == '/' )
    {
      skip_comment(str,len);
      worked=1;
    }
  }while(worked);
}

/* Note: Serious code-duplication from the lexer. */
static int pcharp_to_svalue_rec(PCHARP *str,
				ptrdiff_t *len)
{
  extern int parse_esc_seq_pcharp (PCHARP buf, p_wchar2 *chr, ptrdiff_t *len);
  struct svalue *begin = Pike_sp;
  PCHARP start = *str;
  check_stack(100);
  check_c_stack(1000);

  while(1)
  {
    skip_to_token(str,len);

    switch( READ() )
    {
      default: goto fail;

      case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      {
	int base = 10;
	CONSUME(-1);
	goto read_number;

      case '0': 
	switch( EXTRACT_PCHARP(*str) )
	{
	  case 'x':case 'X': base = 16; CONSUME(1); break;
	  case 'b':case 'B': base = 2;  CONSUME(1); break;
	  case '.':
	    CONSUME(-1);
	    goto read_float;
	  case 'e':case 'E':
	    CONSUME(-1);
	    push_float(0.0);
	    return 1;
	  default:
	    base = 8;
	    CONSUME(-1);
	    break;
	}
      read_number:

	/* Integer or float. */
	push_int(0);
	if( pcharp_to_svalue_inumber(Pike_sp-1,*str,str,base,*len) )
	{
	  if( (EXTRACT_PCHARP(*str) == '.' || EXTRACT_PCHARP(*str)=='e'|| EXTRACT_PCHARP(*str)=='E') )
	  {
	  read_float:
	    {
	      void *integer_parse = str->ptr;
	      FLOAT_TYPE res;
	      res = STRTOFLOAT_PCHARP(start,str);
	      if( integer_parse < str->ptr )
	      {
		pop_stack();
		push_float(res);
	      }
	    }
	  *len -= SUBTRACT_PCHARP(*str,start);
	  if( *len < 0 )
	    /* this is possible for floats combined with %<len>O format. */
	    goto fail;
	  }
	  return 1;
	}
      }
      goto fail;

    case '\'':
      // single character. 
    {
      unsigned int l = 0;
      struct svalue res = svalue_int_zero;
      ptrdiff_t used;
      MP_INT bigint;
      while(1)
      {
	p_wchar2 tmp;
	switch( (tmp=READ()) )
	{
	  case 0:
	    goto fail;

	  case '\\':
	    if( parse_esc_seq_pcharp(*str,&tmp,&used) )
	      return 0;
	    CONSUME(used);
	    /* fallthrough. */
	  default:
	    l++;
	    if( l == sizeof(INT_TYPE)-1 )
	    {
	      /* overflow possible. Switch to bignums. */
	      mpz_init(&bigint);
	      mpz_set_ui(&bigint,res.u.integer);
	      TYPEOF(res) = PIKE_T_OBJECT;
	    }

	    if( l >= sizeof(INT_TYPE)-1 )
	    {
	      mpz_mul_2exp(&bigint,&bigint,8);
	      mpz_add_ui(&bigint,&bigint,tmp);
	    }
	    else
	    {
	      res.u.integer <<= 8;
	      res.u.integer |= tmp;
	    }
	    break;

	  case '\'':
	    if( TYPEOF(res) == PIKE_T_OBJECT )
	    {
	      push_bignum( &bigint );
	      mpz_clear(&bigint);
	      reduce_stack_top_bignum();
	      return 1;
	    }
	    *Pike_sp++ = res;
	    return 1;
	}
      }
    }

    case '"':
    {
      struct string_builder tmp;
      PCHARP start;
      int cnt;
      init_string_builder(&tmp,0);

      start = *str;
      cnt = 0;
      for (;*len;)
      {
	switch(READ())
	{
	  case '\"':
	    /* End of string -- done. */
	    if (cnt) string_builder_append(&tmp, start, cnt);
	    push_string(finish_string_builder(&tmp));
	    return 1;

	  case '\\':
	  {
	    /* Escaped character */
	    p_wchar2 val=0;
	    ptrdiff_t consumed;
	    if( !parse_esc_seq_pcharp(*str,&val,&consumed) )
	      CONSUME(consumed);
	    if (cnt) string_builder_append(&tmp, start, cnt);
	    string_builder_putchar(&tmp, val);
	    *str=start;
	  }
	  continue;

	  case '\n':
	    free_string_builder(&tmp);
	    goto fail;

	  default:
	    cnt++;
	    continue;
	}
      }
      /* Unterminated string -- fail. */
      free_string_builder(&tmp);
      goto fail;
    }

    case '(':
      // container.
    {
      int num=0;
      p_wchar2 tmp;

#define CHECK_STACK_ADD(ADD) do{		\
	if(Pike_sp-begin > 100 ) {		\
	  ADD(Pike_sp-begin);			\
	  begin= Pike_sp;			\
	  num++;				\
	}					\
      }while(0)

#define FINISH(ADD) do {					\
	num++;							\
	ADD(Pike_sp-begin);					\
	goto container_finished;				\
      } while(0)

      switch( READ() )
      {
	case '[':
	  while(1)
	  {
	    skip_to_token(str,len);
	    if(EXTRACT_PCHARP(*str) == ']' )
	    {
	      CONSUME(1);
	      FINISH(f_aggregate_mapping);
	    }
	    if( !pcharp_to_svalue_rec(str,len) )
	      goto fail;
	    skip_to_token(str,len);
	    if( READ() != ':' )
	      goto fail;
	    if(!pcharp_to_svalue_rec(str,len))
	      goto fail;
	    skip_to_token(str,len);
	    tmp = READ();
	    if( tmp == ']' )
	      FINISH(f_aggregate_mapping);
	    if( tmp != ',' )
	      goto fail;
	    CHECK_STACK_ADD(f_aggregate_mapping);
	  }
	  break;

	case '{':
	  while(1)
	  {
	    skip_to_token(str,len);
	    if(EXTRACT_PCHARP(*str)=='}' )
	    {
	      CONSUME(1);
	      FINISH(f_aggregate);
	    }
	    if( !pcharp_to_svalue_rec(str,len) )
	      goto fail;
	    skip_to_token(str,len);
	    tmp=READ();
	    if(tmp == '}' )
	      FINISH(f_aggregate);
	    if( tmp != ',' )
	      goto fail;
	    CHECK_STACK_ADD(f_aggregate);
	  }
	  break;
	case '<':
	  while(1)
	  {
	    skip_to_token(str,len);
	    if(EXTRACT_PCHARP(*str)=='>' )
	    {
	      CONSUME(1);
	      FINISH(f_aggregate_multiset);
	    }
            if( !pcharp_to_svalue_rec(str,len) )
              goto fail;
	    skip_to_token(str,len);
	    tmp=READ();
	    if(tmp == '>' )
	      FINISH(f_aggregate_multiset);
	    if( tmp != ',' )
	      goto fail;
	    CHECK_STACK_ADD(f_aggregate_multiset);
	  }
	  break;
	default:
	  /* Not a valid container. */
	  goto fail;
      }
#undef FINISH
#undef CHECK_STACK_ADD
    /* end of container. */
    container_finished:
      if( READ() != ')' )
	goto fail;
      if(num > 1 )
	f_add(num);
      return 1;
    }
    }
  }

 fail:
  pop_n_elems(Pike_sp-begin);
  return 0;
}


static void *pcharp_to_svalue_percent_o(struct svalue *res,
					PCHARP str,
					ptrdiff_t len)
{
  SET_SVAL(*res, T_INT, NUMBER_UNDEFINED, integer, 0);
  if( pcharp_to_svalue_rec( &str, &len ) )
  {
    *res = *--Pike_sp;
    return str.ptr;
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


#define MKREADSET(SIZE)						\
static ptrdiff_t PIKE_CONCAT(read_set,SIZE) (			\
  PIKE_CONCAT(p_wchar,SIZE) *match,				\
  ptrdiff_t cnt,						\
  struct sscanf_set *set,					\
  ptrdiff_t match_len)                                          \
{								\
  p_wchar2 e, last = 0;						\
  MATCH_IS_WIDE( int set_size=0; )				\
								\
  if(cnt>=match_len)						\
    Pike_error("Unterminated sscanf set.\n");			\
								\
  memset(set->c, 0, sizeof(set->c));				\
  set->a=0;							\
								\
  if(match[cnt]=='^' &&						\
     (cnt+2>=match_len || match[cnt+1]!='-' ||			\
      match[cnt+2]==']'))                                       \
  {								\
    set->neg=1;							\
    cnt++;							\
    if(cnt>=match_len)						\
      Pike_error("Unterminated negated sscanf set.\n");         \
  }else{							\
    set->neg=0;							\
  }								\
								\
  if(match[cnt]==']' || match[cnt]=='-')			\
  {								\
    set->c[last=match[cnt]]=1;					\
    cnt++;							\
    if(cnt>=match_len)						\
      Pike_error("Empty sscanf range.\n");			\
  }								\
								\
  for(;match[cnt]!=']';)					\
  {								\
    if(match[cnt]=='-')						\
    {								\
      cnt++;							\
      if(cnt>=match_len)					\
	Pike_error("Unterminated sscanf range.\n");		\
								\
      if(match[cnt]==']')					\
      {								\
	set->c['-']=1;						\
	break;							\
      }								\
								\
      if(last > match[cnt])					\
	Pike_error("Inverted sscanf range [%c-%c].\n",		\
		   last, match[cnt]);				\
								\
MATCH_IS_WIDE(							\
      if(last < (p_wchar2)sizeof(set->c) && last >= 0)		\
      {								\
	if(match[cnt] < (p_wchar2)sizeof(set->c))		\
	{							\
)								\
	  for(e=last;e<=match[cnt];e++) set->c[e]=1;		\
MATCH_IS_WIDE(							\
	}else{							\
	  for(e=last;e<(p_wchar2)sizeof(set->c);e++)		\
            set->c[e]=1;					\
								\
	  check_stack(2);					\
	  push_int(256);					\
	  push_int(match[cnt]);					\
	  set_size++;						\
	}							\
      }								\
      else							\
      {								\
        Pike_sp[-1].u.integer=match[cnt];                       \
      }								\
)								\
    } else {							\
      last=match[cnt];						\
MATCH_IS_WIDE(							\
      if(last < (p_wchar2)sizeof(set->c) && last >= 0)		\
)								\
        set->c[last]=1;						\
MATCH_IS_WIDE(							\
      else{							\
        if(set_size &&						\
	   ((p_wchar2)Pike_sp[-1].u.integer) == last-1)		\
        {							\
          Pike_sp[-1].u.integer++;                              \
        }else{							\
	  check_stack(2);					\
	  push_int(last);					\
	  push_int(last);					\
	  set_size++;						\
        }							\
      }								\
  )								\
    }								\
    cnt++;							\
    if(cnt>=match_len)						\
      Pike_error("Unterminated sscanf set.\n");			\
  }								\
								\
MATCH_IS_WIDE(							\
  if(set_size)							\
  {								\
    INT32 *order;						\
    set->a=aggregate_array(set_size*2);				\
    order = get_set_order(set->a);				\
    for(e=0;e<(p_wchar2)set->a->size;e+=2)			\
    {								\
      if(order[e]+1 != order[e+1] &&				\
         order[e+1]+1 != order[e]) {				\
        free_array(set->a);					\
        set->a=0;						\
        free(order);                                            \
	Pike_error("Overlapping ranges in sscanf not supported.\n"); \
      }								\
    }								\
								\
    order_array(set->a,order);					\
    free(order);                                                \
  }								\
)								\
  return cnt;							\
}



/* Parse binary IEEE strings on a machine which uses a different kind
   of floating point internally */

#ifdef NEED_CUSTOM_IEEE

static inline FLOAT_TYPE low_parse_IEEE_float(const char *b, int sz)
{
  unsigned INT32 f, extra_f;
  int s, e;
  unsigned char x[4];
  double r;

  if (sz < 0) {
    x[0] = EXTRACT_UCHAR(b-sz-1);
    x[1] = EXTRACT_UCHAR(b-sz-2);
    x[2] = EXTRACT_UCHAR(b-sz-3);
    x[3] = EXTRACT_UCHAR(b-sz-4);
  } else {
    x[0] = EXTRACT_UCHAR(b);
    x[1] = EXTRACT_UCHAR(b+1);
    x[2] = EXTRACT_UCHAR(b+2);
    x[3] = EXTRACT_UCHAR(b+3);
  }
  s = ((x[0]&0x80)? 1 : 0);

  if(sz==4 || sz==-4) {
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
    if (sz < 0)
      extra_f = (((unsigned INT32)EXTRACT_UCHAR(b+3))<<24)|
	(((unsigned INT32)EXTRACT_UCHAR(b+2))<<16)|
	(((unsigned INT32)EXTRACT_UCHAR(b+1))<<8)|
	((unsigned INT32)EXTRACT_UCHAR(b));
    else
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
  {
    if(f||extra_f) {
      /* NAN */
      return (FLOAT_TYPE)MAKE_NAN();
    } else {
      /* +/- Infinity */
      return (FLOAT_TYPE)MAKE_INF() * (s? -1:1);
    }
  }

  r = (double)f;
  if(extra_f)
    r += ((double)extra_f)/4294967296.0;
  return (FLOAT_TYPE)(s? -ldexp(r, e):ldexp(r, e));
}

#endif

static FLOAT_TYPE NOINLINE_UNALIGNED extract_float_be(const char * x) {
#ifdef FLOAT_IS_IEEE_BIG
    float f;
    memcpy(&f, x, sizeof(f));
    return f;
#elif FLOAT_IS_IEEE_LITTLE
    float f;
    unsigned INT32 tmp = get_unaligned32(x);
    tmp = bswap32(tmp);
    memcpy(&f, &tmp, sizeof(f));
    return f;
#else
    return low_parse_IEEE_float(x, 4);
#endif
}

static FLOAT_TYPE NOINLINE_UNALIGNED extract_double_be(const char * x) {
#ifdef DOUBLE_IS_IEEE_BIG
    double f;
    memcpy(&f, x, sizeof(f));
    return f;
#elif DOUBLE_IS_IEEE_LITTLE
    double f;
#ifdef UINT64
    UINT64 tmp = get_unaligned64(x);
    tmp = bswap64(tmp);
#else
    char tmp[8];
    tmp[7] = x[0];
    tmp[6] = x[1];
    tmp[5] = x[2];
    tmp[4] = x[3];
    tmp[3] = x[4];
    tmp[2] = x[5];
    tmp[1] = x[6];
    tmp[0] = x[7];
#endif
    memcpy(&f, &tmp, sizeof(f));
    return f;
#else
    return low_parse_IEEE_float(x, 8);
#endif
}

static FLOAT_TYPE NOINLINE_UNALIGNED extract_float_le(const char * x) {
#ifdef FLOAT_IS_IEEE_LITTLE
    float f;
    memcpy(&f, x, sizeof(f));
    return f;
#elif FLOAT_IS_IEEE_BIG
    float f;
    unsigned INT32 tmp = get_unaligned32(x);
    tmp = bswap32(tmp);
    memcpy(&f, &tmp, sizeof(f));
    return f;
#else
    return low_parse_IEEE_float(x, -4);
#endif
}

static FLOAT_TYPE NOINLINE_UNALIGNED extract_double_le(const char * x) {
#ifdef DOUBLE_IS_IEEE_LITTLE
    double f;
    memcpy(&f, x, sizeof(f));
    return f;
#elif DOUBLE_IS_IEEE_BIG
    double f;
#ifdef UINT64
    UINT64 tmp = get_unaligned64(x);
    tmp = bswap64(tmp);
#else
    char tmp[8];
    tmp[7] = x[0];
    tmp[6] = x[1];
    tmp[5] = x[2];
    tmp[4] = x[3];
    tmp[3] = x[4];
    tmp[2] = x[5];
    tmp[1] = x[6];
    tmp[0] = x[7];
#endif
    memcpy(&f, &tmp, sizeof(f));
    return f;
#else
    return low_parse_IEEE_float(x, -8);
#endif
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

static NOINLINE_UNALIGNED struct pike_string *
  get_string_slice( void *input, int shift,
		    ptrdiff_t offset, ptrdiff_t len,
		    struct pike_string *str )
{
    if( !shift && str )
        return string_slice( str, offset, len );
    return make_shared_binary_pcharp(MKPCHARP(((char *)input)+(offset<<shift),shift),
                                     len);
}

/* INT32 very_low_sscanf_{0,1,2}_{0,1,2}(p_wchar *input, ptrdiff_t input_len,
 *					 p_wchar *match, ptrdiff_t match_len,
 *					 ptrdiff_t *chars_matched,
 *					 int *success, int flags)
 *
 * Perform the actual parsing.
 *
 * Arguments:
 *   input, input_len		Input data to parse.
 *   match, match_len		Format string.
 *   chars_matched		Gets set to the number of characters
 *				in the input that were advanced.
 *   success			Gets set to 1 on success.
 *   flags			Operating mode.
 *
 * Returns:
 *   Returns the number of %-directives that were successfully matched.
 *   Pushes non-ignored matches on the Pike stack in the order they
 *   were matched.
 *
 * FIXME: success is only used internally, and should probably be
 * gotten rid of.
 */
#define MK_VERY_LOW_SSCANF(INPUT_SHIFT, MATCH_SHIFT)			 \
static INT32 PIKE_CONCAT4(very_low_sscanf_,INPUT_SHIFT,_,MATCH_SHIFT)(	 \
                         PIKE_CONCAT(p_wchar, INPUT_SHIFT) *input,	 \
			 ptrdiff_t input_len,				 \
			 PIKE_CONCAT(p_wchar, MATCH_SHIFT) *match,	 \
			 ptrdiff_t match_len,				 \
			 ptrdiff_t *chars_matched,			 \
			 int *success,                                   \
                         struct pike_string *pstr,			 \
			 int flags)					 \
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
      if(match[cnt]=='%')						 \
      {									 \
        if(match[cnt+1]=='%')						 \
        {								 \
          cnt++;							 \
        }else{								 \
          break;							 \
        }								 \
      }									 \
      if(eye>=input_len || input[eye]!=match[cnt])			 \
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
    if(match[cnt]!='%' || match[cnt+1]=='%')				 \
    {									 \
      Pike_fatal("Failed to escape in sscanf.\n");			 \
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
      Pike_error("Missing format specifier in sscanf format string.\n"); \
									 \
    while(1)								 \
    {									 \
      switch(match[cnt])						 \
      {									 \
	case '*':							 \
	  no_assign=1;							 \
	  cnt++;							 \
	  if(cnt>=match_len)						 \
	    Pike_error("Missing format specifier in ignored sscanf "	 \
		       "format string.\n");				 \
	  continue;							 \
									 \
	case '0': case '1': case '2': case '3': case '4':		 \
	case '5': case '6': case '7': case '8': case '9':		 \
	{								 \
	  PCHARP t;							 \
	  field_length = STRTOL_PCHARP(MKPCHARP(match+cnt, MATCH_SHIFT), \
				       &t,10);				 \
	  cnt = SUBTRACT_PCHARP(t, MKPCHARP(match, MATCH_SHIFT));	 \
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
              UNREACHABLE();                                             \
	    }								 \
	    if(match[e]=='%')						 \
	    {								 \
	      switch(match[e+1])					 \
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
            struct svalue *save_sp=Pike_sp;                              \
	    PIKE_CONCAT4(very_low_sscanf_, INPUT_SHIFT, _, MATCH_SHIFT)( \
                         input+eye,					 \
			 input_len-eye,					 \
			 match+cnt+1,					 \
			 e-cnt-2,					 \
			 &tmp,						 \
                         &yes,0,flags);					 \
	    if(yes && tmp)						 \
	    {								 \
              f_aggregate((INT32)(Pike_sp-save_sp));                     \
              sval.u.array=append_array(sval.u.array,Pike_sp-1);         \
	      pop_stack();						 \
	      eye+=tmp;							 \
	    }else{							 \
              pop_n_elems(Pike_sp-save_sp);                              \
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
            if((unsigned INT32) input[eye+e] > 255)			 \
             {								 \
               chars_matched[0]=eye;					 \
               return matches;						 \
             }								 \
          }								 \
)									 \
	  sval.u.integer=0;						 \
	  if (minus_flag)						 \
	  {								 \
             int pos=0;                                                  \
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
                 sval=*--Pike_sp;                                        \
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
                 sval=*--Pike_sp;                                        \
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
      case 'H':								 \
        {								 \
          unsigned long len=0;                                           \
          if(field_length == -1)					 \
            field_length=1;						 \
          if(field_length == 0)                                          \
            Pike_error("%%H size field is 0.\n");                        \
          if(eye+field_length > input_len)				 \
          {								 \
            chars_matched[0]=eye;					 \
            return matches;						 \
          }								 \
          INPUT_IS_WIDE (						 \
            for(e=0;e<field_length;e++)					 \
            {								 \
              if((unsigned INT32) input[eye+e] > 255)			 \
              {								 \
                chars_matched[0]=eye;					 \
                return matches;						 \
              }								 \
            }								 \
          );								 \
          if (minus_flag)						 \
          {								 \
            int pos=0;							 \
            pos = (eye += field_length);				 \
            while(--field_length >= 0)					 \
            {								 \
              len<<=8;							 \
              len |= input[--pos];					 \
            }								 \
          } else {							 \
            while(--field_length >= 0)					 \
            {								 \
              len<<=8;							 \
              len |= input[eye];					 \
              eye++;							 \
            }								 \
          }								 \
          if(len > (unsigned long)(input_len-eye))                       \
          {								 \
            chars_matched[0]=eye-field_length;				 \
            return matches;						 \
          }								 \
          if (no_assign) {						 \
            no_assign = 2;						 \
          } else {							 \
            SET_SVAL(sval, T_STRING, 0, string,				 \
                     PIKE_CONCAT(make_shared_binary_string,		 \
                                 INPUT_SHIFT)(input+eye, len));		 \
          }								 \
          eye+=len;							 \
          break;							 \
        }								 \
									 \
        case 'b':							 \
        case 'o':							 \
        case 'd':							 \
        case 'u':							 \
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
	  switch(match[cnt])						 \
	  {								 \
	  case 'b': base = -2; break;					 \
	  case 'o': base = -8; break;					 \
	  case 'd': base = 10; break;					 \
	  case 'u': base =-10; break;					 \
	  case 'x': base =-16; break;					 \
	  }								 \
									 \
	  if (flags & SSCANF_FLAG_80_COMPAT) {				 \
	    /* All integer formats were signed in Pike 8.0 */		 \
	    if (base < 0) base = -base;					 \
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
	  f = (FLOAT_TYPE)STRTOFLOAT_PCHARP(MKPCHARP(input+eye,		 \
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
            Pike_error("Invalid IEEE width %ld in sscanf format string.\n", \
                       (long)field_length);                              \
          if(eye+field_length > input_len)				 \
	  {								 \
	    chars_matched[0]=eye;					 \
	    return matches;						 \
	  }								 \
	  SET_SVAL(sval, T_FLOAT, 0, float_number, 0.0);		 \
	  switch(field_length) {					 \
	    case 4:							 \
              if (minus_flag)                                            \
                EXTRACT_FLOAT(sval, input+eye, INPUT_SHIFT, extract_float_le); \
              else                                                       \
                EXTRACT_FLOAT(sval, input+eye, INPUT_SHIFT, extract_float_be); \
              eye += 4;                                                  \
              break;							 \
	    case 8:							 \
              if (minus_flag)                                            \
                EXTRACT_DOUBLE(sval, input+eye, INPUT_SHIFT, extract_double_le); \
              else                                                       \
                EXTRACT_DOUBLE(sval, input+eye, INPUT_SHIFT, extract_double_be); \
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
              SET_SVAL(sval, T_STRING, 0, string,                        \
                       get_string_slice(input,INPUT_SHIFT,eye,           \
                                        field_length,pstr));             \
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
               SET_SVAL(sval, T_STRING, 0, string,                       \
                        get_string_slice(input,INPUT_SHIFT,eye,          \
                                         input_len-eye,pstr));           \
	    }								 \
	    eye=input_len;						 \
	    break;							 \
	  }else{							 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *end_str_start;		 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *end_str_end;		 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *s=0;			 \
	    PIKE_CONCAT(p_wchar, MATCH_SHIFT) *p=0;			 \
	    int contains_percent_percent;				 \
            ptrdiff_t start, new_eye;					 \
									 \
	    e = cnt;							 \
	    start=eye;							 \
	    end_str_start=match+cnt+1;					 \
									 \
	    s=match+cnt+1;						 \
      test_again:							 \
	    if(*s=='%')							 \
	    {								 \
	      s++;							 \
	      if(*s=='*') s++;						 \
              set.neg=0;						 \
	      switch(*s)						 \
	      {								 \
                case 0:							 \
                  /* FIXME: Should really look at the match len */	 \
		  Pike_error("%% without conversion specifier.\n");	 \
                  break;						 \
									 \
		case 'n':						 \
		  s++;							 \
                  /* Advance the end string start pointer */		 \
                  end_str_start = s;					 \
		  e = s - match;					 \
	          goto test_again;					 \
									 \
		case 's':						 \
		  Pike_error("Illegal to have two adjecent %%s.\n");	 \
                  UNREACHABLE();                                         \
									 \
	  /* sscanf("foo-bar","%s%d",a,b) might not work as expected */	 \
		case 'd':						 \
		  memset(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['-']=0;						 \
		  goto match_set;					 \
									 \
		case 'u':						 \
		  memset(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'o':						 \
		  memset(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='7';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'x':						 \
		  memset(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  for(e='a';e<='f';e++) set.c[e]=0;			 \
		  goto match_set;					 \
									 \
		case 'D':						 \
		case 'i':						 \
		  memset(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['-']=0;						 \
		  goto match_set;					 \
									 \
		case 'f':						 \
		  memset(set.c, 1, sizeof(set.c));			 \
		  for(e='0';e<='9';e++) set.c[e]=0;			 \
		  set.c['.']=set.c['-']=0;				 \
		  goto match_set;					 \
									 \
		case '[':		/* oh dear */			 \
		  PIKE_CONCAT(read_set,MATCH_SHIFT)(match,		 \
						    s-match+1,		 \
						    &set,		 \
                                                    match_len);          \
		  set.neg=!set.neg;					 \
		  goto match_set;					 \
	      }								 \
	    }								 \
									 \
	    contains_percent_percent=0;					 \
									 \
	    for(;e<match_len;e++)					 \
	    {								 \
	      if(match[e]=='%')						 \
	      {								 \
		if(match[e+1]=='%')					 \
		{							 \
		  contains_percent_percent=1;				 \
		  e++;							 \
		}else{							 \
		  break;						 \
		}							 \
	      }								 \
	    }								 \
									 \
	    end_str_end=match+e;					 \
									 \
	    if (end_str_end == end_str_start) {				 \
	      if (no_assign) {						 \
		no_assign = 2;						 \
	      } else {							 \
                SET_SVAL(sval, T_STRING, 0, string,                      \
                         get_string_slice(input,INPUT_SHIFT,eye,         \
                                          input_len-eye,pstr));          \
	      }								 \
	      eye=input_len;						 \
	      break;							 \
	    } else if(!contains_percent_percent)			 \
	    {								 \
	      struct pike_mem_searcher searcher;			 \
	      PIKE_CONCAT(p_wchar, INPUT_SHIFT) *s2;			 \
	      pike_init_memsearch(&searcher,				 \
				  MKPCHARP(end_str_start, MATCH_SHIFT),	 \
				       end_str_end - end_str_start,	 \
				       input_len - eye);		 \
              s2 = searcher.mojt.vtab-> PIKE_CONCAT(func,INPUT_SHIFT)	 \
                     (searcher.mojt.data, input+eye, input_len-eye);	 \
	      if(!s2)							 \
	      {								 \
		chars_matched[0]=eye;					 \
		return matches;						 \
	      }								 \
	      eye=s2-input;						 \
	      new_eye=eye+end_str_end-end_str_start;			 \
	    }else{							 \
	      PIKE_CONCAT(p_wchar, INPUT_SHIFT) *p2 = NULL;		 \
	      for(;eye<input_len;eye++)					 \
	      {								 \
		p2=input+eye;						 \
		for(s=end_str_start;s<end_str_end;s++,p2++)		 \
		{							 \
		  if(*s!=*p2) break;					 \
		  if(*s=='%') s++;					 \
		}							 \
		if(s==end_str_end)					 \
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
              SET_SVAL(sval, T_STRING, 0, string,                        \
                       get_string_slice(input,INPUT_SHIFT,start,         \
                                        eye-start,pstr));                \
	    }								 \
									 \
	    cnt=end_str_end-match-1;					 \
	    eye=new_eye;						 \
	    break;							 \
	  }								 \
									 \
	case '[':							 \
	  cnt=PIKE_CONCAT(read_set,MATCH_SHIFT)(match,cnt+1,		 \
						&set,match_len);         \
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
	      if((unsigned INT32) input[eye] < sizeof(set.c))		 \
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
                  x = set_lookup(set.a, &tmp);                           \
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
            SET_SVAL(sval, T_STRING, 0, string,                          \
                     get_string_slice(input,INPUT_SHIFT,e,               \
                                       eye-e,pstr));                     \
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
		   (INT32)(eye - truncated));				 \
	  break;							 \
									 \
	default:							 \
	  Pike_error("Unknown sscanf token %%%c(0x%02x)\n",		 \
		match[cnt], match[cnt]);				 \
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
      *Pike_sp++=sval;							 \
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

/* MATCH_IS_WIDE(X) is X if the match set is wide.
 * INPUT_IS_WIDE(X) is X if the input is wide.
 */
#define MATCH_IS_WIDE(X)
#define INPUT_IS_WIDE(X)

MKREADSET(0)
MK_VERY_LOW_SSCANF(0,0)

#undef INPUT_IS_WIDE
#define INPUT_IS_WIDE(X) X

MK_VERY_LOW_SSCANF(1,0)
MK_VERY_LOW_SSCANF(2,0)

#undef MATCH_IS_WIDE
#define MATCH_IS_WIDE(X) X

MKREADSET(1)
MKREADSET(2)

#undef INPUT_IS_WIDE
#define INPUT_IS_WIDE(X)

MK_VERY_LOW_SSCANF(0,1)
MK_VERY_LOW_SSCANF(0,2)

#undef INPUT_IS_WIDE
#define INPUT_IS_WIDE(X) X

MK_VERY_LOW_SSCANF(1,1)
MK_VERY_LOW_SSCANF(2,1)
MK_VERY_LOW_SSCANF(1,2)
MK_VERY_LOW_SSCANF(2,2)


/* */
INT32 low_sscanf_pcharp(PCHARP input, ptrdiff_t len,
                        PCHARP format, ptrdiff_t format_len,
                        ptrdiff_t *chars_matched,
			int flags)
{
  int ok;
  check_c_stack(sizeof(struct sscanf_set)*2 + 512);
  switch( input.shift*3 + format.shift )
  {
    case 0:
      return very_low_sscanf_0_0(input.ptr, len,format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 1:
      return very_low_sscanf_0_1(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 2:
      return very_low_sscanf_0_2(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 3:
      return very_low_sscanf_1_0(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 4:
      return very_low_sscanf_1_1(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 5:
      return very_low_sscanf_1_2(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 6:
      return very_low_sscanf_2_0(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 7:
      return very_low_sscanf_2_1(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
    case 8:
      return very_low_sscanf_2_2(input.ptr, len, format.ptr, format_len,
                                 chars_matched, &ok, 0, flags);
  }
  UNREACHABLE();
}

/* Simplified interface to very_low_sscanf_{0,1,2}_{0,1,2}(). */
INT32 low_sscanf(struct pike_string *data, struct pike_string *format,
		 INT32 flags)
{
  ptrdiff_t matched_chars;
  int x;

  check_c_stack(sizeof(struct sscanf_set)*2 + 512);

  switch(data->size_shift*3 + format->size_shift) {
    /* input_shift : match_shift */
  case 0:
    /*      0      :      0 */
    return very_low_sscanf_0_0(STR0(data), data->len,
                               STR0(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 1:
    /*      0      :      1 */
    return very_low_sscanf_0_1(STR0(data), data->len,
                               STR1(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 2:
    /*      0      :      2 */
    return very_low_sscanf_0_2(STR0(data), data->len,
                               STR2(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 3:
    /*      1      :      0 */
    return very_low_sscanf_1_0(STR1(data), data->len,
                               STR0(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 4:
    /*      1      :      1 */
    return very_low_sscanf_1_1(STR1(data), data->len,
                               STR1(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 5:
    /*      1      :      2 */
    return very_low_sscanf_1_2(STR1(data), data->len,
                               STR2(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 6:
    /*      2      :      0 */
    return very_low_sscanf_2_0(STR2(data), data->len,
                               STR0(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 7:
    /*      2      :      1 */
    return very_low_sscanf_2_1(STR2(data), data->len,
                               STR1(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  case 8:
    /*      2      :      2 */
    return very_low_sscanf_2_2(STR2(data), data->len,
                               STR2(format), format->len,
                               &matched_chars, &x, data, flags);
    break;
  }

  UNREACHABLE();
}

/*! @decl int sscanf(string data, string format, mixed ... lvalues)
 *!
 *! The purpose of sscanf is to match a string @[data] against a @[format]
 *! string and place the matching results into a list of variables. The list
 *! of @[lvalues] are destructively modified (which is only possible because
 *! sscanf really is a special form, rather than a pike function) with the values
 *! extracted from the @[data] according to the @[format] specification. Only
 *! the variables up to the last matching directive of the format string are
 *! touched.
 *!
 *! The @[format] string may contain strings separated by special matching
 *! directives like @tt{%d@}, @tt{%s@} @tt{%c@} and @tt{%f@}. Every such
 *! directive corresponds to one of the @[lvalues], in the order they are listed.
 *! An lvalue is the name of a variable, a name of a local variable, an index
 *! into an array, mapping or object. It is because of these lvalues that sscanf
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
 *!     hexadecimal (leading @expr{0x@}) or decimal. (@expr{"0101"@} makes
 *!     @expr{65@}).
 *!   @value "%c"
 *!     Reads one character and returns it as an integer
 *!     (@expr{"0101"@} makes @expr{48@}, or @expr{'0'@}, leaving
 *!     @expr{"101"@} for later directives). Using the field width and
 *!     endianness modifiers, you can decode integers of any size and
 *!     endianness. For example @expr{"%-2c"@} decodes @expr{"0101"@}
 *!     into @expr{12592@}, leaving @expr{"01"@} for later directives.
 *!     The sign modifiers can be used to modify the signature of the
 *!     data, making @expr{"%+1c"@} decode @expr{"�"@} into
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
 *!     included in the range. Example: @expr{%[0-9H]@} means any number or 'H'.
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
 *! @code
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
 *! @endcode
 *!
 *! @returns
 *!   The number of directives matched in the format string. Note that a string
 *!   directive (%s or %[]) counts as a match even when matching just the empty
 *!   string (which either may do).
 *! @seealso
 *!   @[sprintf], @[array_sscanf]
 *    @[parse_format]
 */
void o_sscanf(INT32 args)
{
  INT32 i=0;
  int x;
  struct svalue *save_sp=Pike_sp;

  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("sscanf", 1, "string");

  if(TYPEOF(Pike_sp[1-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("sscanf", 2, "string");

  i = low_sscanf(Pike_sp[-args].u.string, Pike_sp[1-args].u.string, 0);

  if(Pike_sp-save_sp > args/2-1)
    Pike_error("Too few arguments for sscanf format.\n");

  for(x=0;x<Pike_sp-save_sp;x++)
    assign_lvalue(save_sp-args+2+x*2,save_sp+x);
  pop_n_elems(Pike_sp-save_sp +args);

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

void o_sscanf_80(INT32 args)
{
  INT32 i=0;
  int x;
  struct svalue *save_sp=Pike_sp;

  if(TYPEOF(Pike_sp[-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("sscanf", 1, "string");

  if(TYPEOF(Pike_sp[1-args]) != T_STRING)
    SIMPLE_ARG_TYPE_ERROR("sscanf", 2, "string");

  i = low_sscanf(Pike_sp[-args].u.string, Pike_sp[1-args].u.string,
		 SSCANF_FLAG_80_COMPAT);

  if(Pike_sp-save_sp > args/2-1)
    Pike_error("Too few arguments for sscanf format.\n");

  for(x=0;x<Pike_sp-save_sp;x++)
    assign_lvalue(save_sp-args+2+x*2,save_sp+x);
  pop_n_elems(Pike_sp-save_sp +args);

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
  struct svalue *save_sp=Pike_sp;
  struct array *a;

  check_all_args("array_sscanf",args,BIT_STRING, BIT_STRING,0);

  i = low_sscanf(Pike_sp[-args].u.string, Pike_sp[1-args].u.string, 0);

  a = aggregate_array(Pike_sp - save_sp);
  pop_n_elems(args);
  push_array(a);
}

PMOD_EXPORT void f_sscanf_80(INT32 args)
{
  INT32 i;
  struct svalue *save_sp=Pike_sp;
  struct array *a;

  check_all_args("array_sscanf",args,BIT_STRING, BIT_STRING,0);

  i = low_sscanf(Pike_sp[-args].u.string, Pike_sp[1-args].u.string,
		 SSCANF_FLAG_80_COMPAT);

  a = aggregate_array(Pike_sp - save_sp);
  pop_n_elems(args);
  push_array(a);
}

static void push_sscanf_argument_types(PCHARP format, ptrdiff_t format_len,
				       int cnt, struct mapping *state,
				       int flags)
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
	/* FALLTHRU */

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
	    push_sscanf_argument_types(format, e, cnt+1, state, flags);
	    if (!(depth = pop_stack_mark())) {
	      push_type(PIKE_T_ZERO);
	    } else {
	      /* Join the argument types. */
	      while (depth > 1) {
		push_type(T_OR);
		depth--;
	      }
	    }
	    push_unlimited_array_type(PIKE_T_ARRAY);
	    push_unlimited_array_type(PIKE_T_ARRAY);
	  }
	  cnt = e;
	  break;
	}

	case 'n':	/* FIXME: tIntPos? */
        case 'b':
        case 'o':
        case 'u':
        case 'x':
	  /* Unsigned integers. */
	  if (!no_assign) {
	    if (flags & SSCANF_FLAG_80_COMPAT) {
	      /* All integer formats were signed in Pike 8.0. */
	      push_finished_type(int_type_string);
	    } else {
	      push_int_type(0, 0x7fffffff);
	    }
	  }
	  break;

	case 'c':
	  /* FIXME: Also consider the format_len (eg %4c). */
	  if (!no_assign) {
	    struct svalue *s =
	      simple_mapping_string_lookup(state, "sscanf_input_character");
	    if (s) {
	      if (TYPEOF(*s) == PIKE_T_TYPE) {
		/* Use the width of the input string. */
		push_finished_type(s->u.type);
	      } else {
		push_type(PIKE_T_ZERO);
	      }
	      break;
	    }
	  }
	  /* FALLTHRU */
        case 'd':
        case 'D':
        case 'i':
	  /* Signed integers. */
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
		(INDEX_PCHARP(format, cnt+2)==']') ))
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
	  /* FALLTHRU */
      case 's':
      case 'H':
	if (!no_assign) {
	  struct svalue *s =
	    simple_mapping_string_lookup(state, "sscanf_input_character");
	  if (s) {
	    if (TYPEOF(*s) == PIKE_T_TYPE) {
	      /* Use the width of the input string. */
	      push_finished_type(s->u.type);
	      s = simple_mapping_string_lookup(state, "sscanf_input_length");
	      if (s && (TYPEOF(*s) == PIKE_T_TYPE)) {
		/* Use the length of the input string. */
		push_finished_type(s->u.type);
		push_type(PIKE_T_STRING);
	      } else {
		push_unlimited_array_type(PIKE_T_STRING);
	      }
	    } else {
	      push_finished_type(string0_type_string);
	    }
	  } else {
	    push_finished_type(string_type_string);
	  }
	}
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
  struct pike_string *sscanf_input_string;
  struct pike_string *sscanf_format_string;
  struct pike_string *sscanf_80_format_string;
  struct mapping *state;
  int flags = 0;
  int found = 0;
  int fmt_count;

#if 0
  fprintf(stderr, "__handle_sscanf_format()\n");
#endif /* 0 */
  if (args != 5)
    SIMPLE_WRONG_NUM_ARGS_ERROR("__handle_sscanf_format", 5);
  if (TYPEOF(Pike_sp[-5]) != PIKE_T_STRING)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 1, "string");
  if (TYPEOF(Pike_sp[-3]) != PIKE_T_TYPE)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 3, "type");
  if (TYPEOF(Pike_sp[-2]) != PIKE_T_TYPE)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 4, "type");
  if (TYPEOF(Pike_sp[-1]) != PIKE_T_MAPPING)
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 5, "mapping");

  tmp = Pike_sp[-2].u.type;
  if ((tmp->type != PIKE_T_FUNCTION) && (tmp->type != T_MANY)) {
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 4, "type(function)");
  }

  state = Pike_sp[-1].u.mapping;

  MAKE_CONST_STRING(sscanf_input_string, "sscanf_input");

  if (Pike_sp[-5].u.string == sscanf_input_string) {
    /* Save the type of the input string. */
    mapping_string_insert(state, sscanf_input_string, Pike_sp-3);

    /* Get the type of the characters of the input string. */
    push_type_value(index_type(Pike_sp[-3].u.type, int_type_string, NULL));
    push_constant_text("sscanf_input_character");
    mapping_insert(state, Pike_sp-1, Pike_sp-2);

    /* Get the range of lengths for substrings of the input string. */
    type_stack_mark();
    tmp = key_type(Pike_sp[-5].u.type, NULL);
    if (tmp) {
      push_finished_type(tmp);
      free_type(tmp);
      push_type(PIKE_T_ZERO);
      push_type(PIKE_T_INT_OP_RANGE);
      push_type_value(pop_unfinished_type());
    } else {
      push_undefined();
    }
    push_constant_text("sscanf_input_length");
    mapping_insert(state, Pike_sp-1, Pike_sp-2);

    pop_n_elems(args + 4);
    push_undefined();
    return;
  }

  if (TYPEOF(Pike_sp[-4]) != PIKE_T_STRING) {
    if (TYPEOF(Pike_sp[-4]) == PIKE_T_INT) {
      pop_n_elems(args);
      push_undefined();
      return;
    }
    SIMPLE_ARG_TYPE_ERROR("__handle_sscanf_format", 2, "string");
  }

  MAKE_CONST_STRING(sscanf_format_string, "sscanf_format");
  MAKE_CONST_STRING(sscanf_80_format_string, "sscanf_80_format");

  if (Pike_sp[-5].u.string != sscanf_format_string) {
    if (Pike_sp[-5].u.string != sscanf_80_format_string) {
      pop_n_elems(args);
      push_undefined();
      return;
    }

    flags |= SSCANF_FLAG_80_COMPAT;
  }

  fmt = Pike_sp[-4].u.string;
  MAKE_CONST_STRING(attr, "sscanf_args");

#if 0
  fprintf(stderr, "Checking sscanf format: \"%s\": ", fmt->str);
  simple_describe_type(Pike_sp[-2].u.type);
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
	/* FALLTHRU */
      case PIKE_T_NAME:
	arg = arg->cdr;
	continue;
      case PIKE_T_ARRAY:
	array_cnt++;
	arg = arg->cdr;
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
				 fmt->len, 0, state, flags);
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
	  push_unlimited_array_type(PIKE_T_ARRAY);
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
	/* FALLTHRU */
      case PIKE_T_NAME:
	arg = arg->cdr;
	continue;
      case PIKE_T_ARRAY:
	array_cnt++;
	arg = arg->cdr;
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
				 fmt->len, 0, state, flags);
      /* Join the argument types. */
      if (!(fmt_count = pop_stack_mark())) {
	push_type(PIKE_T_ZERO);
      } else while (fmt_count > 2) {
	push_type(T_OR);
	fmt_count--;
      }
      while (array_cnt--) {
	push_unlimited_array_type(PIKE_T_ARRAY);
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
