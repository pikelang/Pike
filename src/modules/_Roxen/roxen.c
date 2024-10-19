/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "config.h"

#include "machine.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

#include "fdlib.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "backend.h"
#include "threads.h"
#include "operators.h"
#include "bitvector.h"


/*! @module _Roxen
 */

/*! @class HeaderParser
 *!
 *! Class for parsing HTTP-requests.
 */

#define FLAG_THROW_ERROR 1
#define FLAG_KEEP_CASE   2
#define FLAG_NO_FOLD     4

#define THP ((struct header_buf *)Pike_fp->current_storage)
struct  header_buf
{
  unsigned char *headers;	/* Buffer containing the data so far. */
  unsigned char *pnt;		/* End of headers. */
  ptrdiff_t hsize, left;	/* Size of buffer, amount remaining. */
  int slash_n, tslash_n, spc;	/* Number of consecutive nl, nl, spaces. */
  int mode;
};

static void f_hp_init( struct object *UNUSED(o) )
{
  THP->headers = NULL;
  THP->pnt = NULL;
  THP->hsize = 0;
  THP->left = 0;
  THP->spc = THP->slash_n = THP->tslash_n = 0;
  THP->mode = 0;
}

static void f_hp_exit( struct object *UNUSED(o) )
{
  if( THP->headers )
    free( THP->headers );
  THP->headers = NULL;
  THP->pnt = NULL;
  THP->hsize = 0;
}

static void f_hp_feed( INT32 args )
/*! @decl array(string|mapping) feed(string data, void|int(0..1) keep_case)
 *!
 *! Feeds data into the parser. Once enough data has been fed the
 *! result array is returned. Feeding data after that destroys the
 *! state of the parser. For typical use, once the result array is
 *! returned, extract the trailing data, check it against expected
 *! payload (e.g. as defined by the Content-Length header or chunked
 *! transfer encoding), and add incoming data to that until enough is
 *! received. If the connection is pipe-lining, send excess data to a
 *! new header parser object. Note that the trailing data from the
 *! first parse object could contain the start of the next response.
 *!
 *! @param data
 *!   Fragment of data to parse.
 *!
 *! @param keep_case
 *!   By default headers are @[lower_case()]'d in the resulting @tt{Headers@}
 *!   mapping. If this parameter is @expr{1@} the header names are kept as is.
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) if more data is needed, and otherwise an
 *!   array with the following content:
 *!   @array
 *!     @elem string 0
 *!       Trailing data.
 *!     @elem string 1
 *!       First line of request.
 *!     @elem mapping(string:string|array(string)) 2
 *!       Headers.
 *!   @endarray
 */
{
  struct pike_string *str = Pike_sp[-args].u.string;
  struct header_buf *hp = THP;
  int keep_case = hp->mode & FLAG_KEEP_CASE;
  int fold = !(hp->mode & FLAG_NO_FOLD);
  int str_len;
  int tot_slash_n=hp->tslash_n, slash_n = hp->slash_n, spc = hp->spc;
  unsigned char *pp,*ep;
  struct svalue *tmp;
  struct mapping *headers;
  ptrdiff_t os=0, i, j, l;
  unsigned char *in;

  if( !( args == 1 || args == 2 ) )
    Pike_error("Bad number of arguments to feed().\n");
  if( TYPEOF(Pike_sp[-args]) != PIKE_T_STRING )
    Pike_error("Wrong type of argument to feed()\n");
  if( args == 2 )
  {
    /* It doesn't make sense to allow each different feed call to
       handle case sensitivity differently. Deprecate this when we
       rewrite the HTTP module. */
    if( TYPEOF(Pike_sp[-args+1]) == PIKE_T_INT )
      keep_case = Pike_sp[-args+1].u.integer;
    else
      Pike_error("Wrong type of argument to feed()\n");
  }
  if( str->size_shift )
    Pike_error("Wide string headers not supported\n");
  str_len = str->len;
  while( str_len >= hp->left )
  {
    unsigned char *buf;
    if( hp->hsize > 512 * 1024 )
      Pike_error("Too many headers\n");
    hp->hsize += 8192;
    buf = hp->headers;
    hp->headers = realloc( hp->headers, hp->hsize );
    if( !hp->headers )
    {
      free(buf);
      hp->hsize = 0;
      hp->left = 0;
      hp->spc = hp->slash_n = 0;
      hp->pnt = NULL;
      Pike_error("Running out of memory in header parser\n");
    }
    hp->left += 8192;
    hp->pnt = (hp->headers + hp->hsize - hp->left);
  }

  memcpy( hp->pnt, str->str, str_len );
  pop_n_elems( args );

  /* FIXME: The below does not support lines terminated with just \r. */
  for( ep=(hp->pnt+str_len),pp=MAXIMUM(hp->headers,hp->pnt);
       pp<ep && slash_n<2; pp++ )
    if( *pp == ' ' )
    {
      spc++;
      slash_n = 0;
    }
    else if( *pp == '\n' )
    {
      slash_n++;
      tot_slash_n++;
    }
    else if( *pp != '\r' )
    {
      slash_n=0;
    }

  hp->slash_n = slash_n;
  hp->spc = spc;
  hp->tslash_n = tot_slash_n;
  hp->left -= str_len;
  hp->pnt += str_len;
  hp->pnt[0] = 0;

  if( slash_n != 2 )
  {
    /* No header terminating double newline. */
    if( (spc < 2) && tot_slash_n )
    {
      /* one newline, but less than 2 space,
       *    --> HTTP/0.9 or broken request
       */
      push_empty_string();
      /* This includes (all eventual) \r\n etc. */
      push_string(make_shared_binary_string((char *)hp->headers,
					    hp->hsize - hp->left));
      f_aggregate_mapping( 0 );
      f_aggregate( 3 );
      return;
    }
    push_int( 0 );
    return;
  }

  /*leftovers*/
  push_string(make_shared_binary_string((char *)pp, hp->pnt - pp));

  in = hp->headers;
  l = pp - hp->headers;

  /* find first line here */
  for( i = 0; i < l; i++ )
    if( (in[i] == '\n') || (in[i] == '\r') )
      break;

  push_string(make_shared_binary_string((char *)in, i));

  if((in[i] == '\r') && (in[i+1] == '\n'))
    i++;
  i++;

  in += i; l -= i;

  headers = allocate_mapping( 5 );
  push_mapping(headers);

  /* Parse headers. */
  for(i = 0; i < l-2; i++)
  {
    if(!keep_case && (in[i] > 64 && in[i] < 91))
    {
      in[i]+=32;	/* lower_case */
    }
    else if( in[i] == ':' )
    {
      /* Does not support white space before the colon. This is in
         line with RFC 7230 product
         header-field = field-name ":" OWS field-value OWS */
      /* in[os..i-1] == the header */
      int val_cnt = 0;
      push_string(make_shared_binary_string((char*)in+os,i-os));

      /* Skip the colon and initial white space. */
      os = i+1;
      while((in[os]==' ') || (in[os]=='\t')) os++;

      /* NOTE: We need to support MIME header continuation lines
       *       (Opera uses this...).
       */
      do {
	for(j=os;j<l;j++)	/* Find end of line */
	  if( in[j] == '\n' || in[j]=='\r')
	    break;

        /* FIXME: Remove header value trailing spaces. */
	push_string(make_shared_binary_string((char*)in+os,j-os));
	val_cnt++;

	if((in[j] == '\r') && (in[j+1] == '\n')) j++;
	os = j+1;
	i = j;
	/* Check for continuation line. */
      } while ((os < l) && fold && ((in[os] == ' ') || (in[os] == '\t')));

      if (val_cnt > 1) {
	/* Join partial header values. */
	f_add(val_cnt);
      }

      if((tmp = low_mapping_lookup(headers, Pike_sp-2)))
      {
	if( TYPEOF(*tmp) == PIKE_T_ARRAY )
	{
          f_aggregate( 1 );
	  ref_push_array(tmp->u.array);
          stack_swap();
	  map_delete(headers, Pike_sp-3);
	  f_add(2);
	} else {
	  ref_push_string(tmp->u.string);
	  stack_swap();
	  map_delete(headers, Pike_sp-3);
	  f_aggregate(2);
	}
      }
      mapping_insert(headers, Pike_sp-2, Pike_sp-1);

      pop_n_elems(2);
    }
    else if( in[i]=='\r' || in[i]=='\n' )
    {
      if( THP->mode & FLAG_THROW_ERROR )
      {
        /* Reset stack so that backtrace shows faulty header. */
        pop_n_elems(3);
        push_string(make_shared_binary_string((const char*)(hp->pnt - str_len),
                                              str_len));
        Pike_error("Malformed HTTP header.\n");
      }
      else
        os = i+1;
    }
  }
  f_aggregate( 3 );             /* data, firstline, headers */
}

static void f_hp_create( INT32 args )
/*! @decl void create(void|int throw_errors, void|int keep_case, @
 *!                   void|int no_fold)
 *!
 *! @param throw_errors
 *!   If true the parser will throw an error instead of silently
 *!   trying to recover from broken HTTP headers.
 *!
 *! @param keep_case
 *!   If true the parser will not normalize the case of HTTP headers
 *!   throughout the lifetime of the parser object (as opposed to the
 *!   similar argument to @[feed] that only applies for unparsed data
 *!   fed into the parser up to that point).
 *!
 *! @param no_fold
 *!   If true the parser will not parse folded headers. Instead the
 *!   first line of the header will be parsed as normal and any
 *!   subsequent lines ignored (or casue an exception if throw_errors
 *!   is set).
 */
{
  INT_TYPE throw_errors = 0;
  INT_TYPE keep_case = 0;
  INT_TYPE no_fold = 0;
  get_all_args(NULL,args,".%i%i%i", &throw_errors, &keep_case, &no_fold);
  pop_n_elems(args);

  if (THP->headers) {
    free(THP->headers);
    THP->headers = NULL;
  }

  THP->mode = 0;
  if(throw_errors) THP->mode |= FLAG_THROW_ERROR;
  if(keep_case) THP->mode |= FLAG_KEEP_CASE;
  if(no_fold) THP->mode |= FLAG_NO_FOLD;

  THP->headers = xalloc( 8192 );
  THP->pnt = THP->headers;
  THP->hsize = 8192;
  THP->left = 8192;
  THP->spc = THP->slash_n = 0;
}

/*! @endclass
 */

static int valid_header_name(struct pike_string *s)
{
  ptrdiff_t i;
  if (s->size_shift) return 0;
  for (i = 0; i < s->len; i++) {
    int c = s->str[i];
    if ((c == '\n') || (c == '\r') || (c == '\t') || (c == ' ') || (c == ':')) {
      // The header formatting should not be broken by strange header names.
      return 0;
    }
  }
  return 1;
}

static int valid_header_value(struct pike_string *s)
{
  ptrdiff_t i;
  if (s->size_shift) return 0;
  for (i = 0; i < s->len; i++) {
    int c = s->str[i];
    if ((c == '\n') || (c == '\r')) {
      // The header formatting should not be broken by strange header values.
      return 0;
    }
  }
  return 1;
}

static void f_make_http_headers( INT32 args )
/*! @decl string @
 *!          make_http_headers(mapping(string:string|array(string)) headers, @
 *!                            int(0..1)|void no_terminator)
 */
{
  int total_len = 0, e;
  unsigned char *pnt;
  struct mapping *m;
  struct keypair *k;
  struct pike_string *res;
  int terminator = 2;

  if( TYPEOF(Pike_sp[-args]) != PIKE_T_MAPPING )
    Pike_error("Wrong argument type to make_http_headers(mapping heads)\n");
  m = Pike_sp[-args].u.mapping;

  if (args > 1) {
    if (TYPEOF(Pike_sp[1-args]) != PIKE_T_INT)
      Pike_error("Bad argument 2 to make_http_headers(). Expected int.\n");
    if (Pike_sp[1-args].u.integer)
      terminator = 0;
  }

  /* loop to check len */
  NEW_MAPPING_LOOP( m->data )
  {
    if( TYPEOF(k->ind) != PIKE_T_STRING || !valid_header_name(k->ind.u.string) )
      Pike_error("Wrong argument type to make_http_headers("
            "mapping(string(8bit):string(8bit)|array(string(8bit))) heads)\n");
    if( TYPEOF(k->val) == PIKE_T_STRING && valid_header_value(k->val.u.string) )
      total_len +=  k->val.u.string->len + 2 + k->ind.u.string->len + 2;
    else if( TYPEOF(k->val) == PIKE_T_ARRAY )
    {
      struct array *a = k->val.u.array;
      ptrdiff_t i, kl = k->ind.u.string->len + 2 ;
      for( i = 0; i<a->size; i++ )
        if( TYPEOF(a->item[i]) != PIKE_T_STRING ||
	    !valid_header_value(a->item[i].u.string) )
          Pike_error("Wrong argument type to make_http_headers("
                "mapping(string(8bit):string(8bit)|"
                "array(string(8bit))) heads)\n");
        else
          total_len += kl + a->item[i].u.string->len + 2;
    } else
      Pike_error("Wrong argument type to make_http_headers("
            "mapping(string(8bit):string(8bit)|"
            "array(string(8bit))) heads)\n");
  }
  total_len += terminator;

  res = begin_shared_string( total_len );
  pnt = STR0(res);
#define STRADD(X)							\
  for( l=(X).u.string->len, s=STR0((X).u.string), c=0; c<l; c++ )	\
    *(pnt++)=*(s++)

  NEW_MAPPING_LOOP( m->data )
  {
    unsigned char *s;
    ptrdiff_t l, c;
    if( TYPEOF(k->val) == PIKE_T_STRING )
    {
      STRADD( k->ind ); *(pnt++) = ':'; *(pnt++) = ' ';
      STRADD( k->val ); *(pnt++) = '\r'; *(pnt++) = '\n';
    }
    else
    {
      struct array *a = k->val.u.array;
      ptrdiff_t i, kl = k->ind.u.string->len + 2;
      for( i = 0; i<a->size; i++ )
      {
        STRADD( k->ind );    *(pnt++) = ':'; *(pnt++) = ' ';
        STRADD( a->item[i] );*(pnt++) = '\r';*(pnt++) = '\n';
      }
    }
  }
  if (terminator) {
    *(pnt++) = '\r';
    *(pnt++) = '\n';
  }

  pop_n_elems( args );
  push_string( end_shared_string( res ) );
}

static p_wchar2 parse_hexchar(p_wchar2 hex)
{
  if(hex>='0' && hex<='9')
    return hex-'0';
  hex |= 32;
  return hex-'W';
}

static void f_http_decode_string(INT32 args)
/*! @decl string http_decode_string(string encoded)
 *!
 *! Decodes an http transport-encoded string. Knows about @tt{%XX@} and
 *! @tt{%uXXXX@} syntax. Treats @tt{%UXXXX@} as @tt{%uXXXX@}. It will
 *! treat '+' as '+' and not ' ', so form decoding needs to replace that
 *! in a second step.
 *!
 *! It also knows about UTF-16 surrogate pairs when decoding @tt{%UXXXX@}
 *! sequences.
 */
{
   int proc = 0;
   int size_shift;
   int got_surrogates = 0;
   PCHARP foo, end;
   struct string_builder newstr;

   if (!args || TYPEOF(Pike_sp[-args]) != PIKE_T_STRING)
     Pike_error("Invalid argument to http_decode_string(string).\n");

   foo = MKPCHARP_STR(Pike_sp[-args].u.string);
   end = ADD_PCHARP(foo, Pike_sp[-args].u.string->len);

   size_shift = Pike_sp[-args].u.string->size_shift;

   /* Count '%' and wide characters.
    *
    * proc counts the number of characters that are to be removed.
    */
   for (; COMPARE_PCHARP(foo, <, end);) {
     p_wchar2 c = EXTRACT_PCHARP(foo);
     INC_PCHARP(foo, 1);
     if (c != '%') continue;
     /* there are at least 2 more characters */
     if (SUBTRACT_PCHARP(end, foo) <= 1)
       Pike_error("Truncated http transport encoded string.\n");
     c = EXTRACT_PCHARP(foo);
     if (c == 'u' || c == 'U') {
       if (SUBTRACT_PCHARP(end, foo) <= 4)
         Pike_error("Truncated unicode sequence.\n");
       INC_PCHARP(foo, 1);
       if (!isxdigit(INDEX_PCHARP(foo, 0)) ||
           !isxdigit(INDEX_PCHARP(foo, 1)) ||
           !isxdigit(INDEX_PCHARP(foo, 2)) ||
           !isxdigit(INDEX_PCHARP(foo, 3)))
           Pike_error("Illegal transport encoding.\n");
       /* %uXXXX */
       if (EXTRACT_PCHARP(foo) != '0' || INDEX_PCHARP(foo, 1) != '0') {
         if (!size_shift) size_shift = 1;
       }
       proc += 5;
       INC_PCHARP(foo, 4);
     } else {
       if (!isxdigit(INDEX_PCHARP(foo, 0)) ||
           !isxdigit(INDEX_PCHARP(foo, 1)))
           Pike_error("Illegal transport encoding.\n");
       proc += 2;
       INC_PCHARP(foo, 2);
     }
   }

   if (!proc) { pop_n_elems(args-1); return; }

   init_string_builder_alloc(&newstr, Pike_sp[-args].u.string->len - proc,
			     size_shift);

   foo = MKPCHARP_STR(Pike_sp[-args].u.string);

   for (; COMPARE_PCHARP(foo, <, end); INC_PCHARP(foo, 1)) {
     p_wchar2 c = INDEX_PCHARP(foo, 0);
     if (c == '%') {
       c = INDEX_PCHARP(foo, 1);
       /* The above loop checks that the following sequences
        * are correct, i.e. that they are not truncated and consist
        * of hexadecimal chars.
        */
       if (c == 'u' || c == 'U') {
         p_wchar2 hex = INDEX_PCHARP(foo, 2);
         c = parse_hexchar(hex)<<12;
         hex = INDEX_PCHARP(foo, 3);
         c |= parse_hexchar(hex)<<8;
         hex = INDEX_PCHARP(foo, 4);
         c |= parse_hexchar(hex)<<4;
         hex = INDEX_PCHARP(foo, 5);
         c |= parse_hexchar(hex);
	 INC_PCHARP(foo, 5);
	 if ((c & 0xf800) == 0xd800) {
	   got_surrogates = 1;
	 }
       } else {
         p_wchar2 hex = INDEX_PCHARP(foo, 1);
         c = parse_hexchar(hex)<<4;
         hex = INDEX_PCHARP(foo, 2);
         c |= parse_hexchar(hex);
	 INC_PCHARP(foo, 2);
       }
     }
     string_builder_putchar(&newstr, c);
   }

   pop_n_elems(args);

   if (got_surrogates) {
     /* Convert the result string to a byte string. */
     newstr.s->size_shift = 0;
     newstr.known_shift = 0;
     newstr.s->len <<= 1;

     /* Then run unicode_to_string() in native byte-order. */
     push_string(finish_string_builder(&newstr));
     push_int(2);
     f_unicode_to_string(2);
   } else {
     push_string(finish_string_builder(&newstr));
   }
}

static void f_html_encode_string( INT32 args )
/*! @decl string html_encode_string(mixed in)
 *!
 *! Encodes the @[in] data as an HTML safe string.
 */
{
  struct pike_string *str;
  int newlen;
  INT32 min;

  if( args != 1 )
    Pike_error("Wrong number of arguments to html_encode_string\n" );

  switch( TYPEOF(Pike_sp[-1]) )
  {
    void o_cast_to_string();

    case PIKE_T_INT:
    case PIKE_T_FLOAT:
      /* Optimization, no need to check the resultstring for
       * unsafe characters.
       */
      o_cast_to_string();
      return;

    default:
      o_cast_to_string();
    case PIKE_T_STRING:
      break;
  }

  str = Pike_sp[-1].u.string;
  newlen = str->len;

  check_string_range(str, 1, &min, NULL);

  if (min > '>') return;

#define COUNT(T) {							\
    T *s = (T *)str->str;						\
    int i;								\
    for( i = 0; i<str->len; i++ )					\
      switch( s[i] )							\
      {									\
	case 0:   		   /* &#0; 	*/			\
	case '<': 		   /* &lt; 	*/			\
	case '>': newlen+=3; break;/* &gt; 	*/			\
	case '&': 		   /* &amp;	*/			\
	case '"': 		   /* &#34;	*/			\
	case '\'': newlen+=4;break;/* &#39;	*/		  	\
      }									\
    }

#define ADD(X) if(sizeof(X)-sizeof("")==4) ADD4(X); else ADD5(X)

#define ADD4(X) ((d[0] = X[0]), (d[1] = X[1]), (d[2] = X[2]), (d[3] = X[3]),\
                (d+=3))

#define ADD5(X) ((d[0] = X[0]), (d[1] = X[1]), (d[2] = X[2]), (d[3] = X[3]),\
                 (d[4] = X[4]), (d+=4))

#define REPLACE(T) {							\
    T *s = (T *)str->str;						\
    T *d = (T *)res->str;						\
    int i;								\
    for( i = 0; i<str->len; i++,s++,d++ )				\
      switch( *s )							\
      {									\
	case 0:   ADD("&#0;");  break;					\
	case '&': ADD("&amp;"); break;					\
	case '<': ADD("&lt;");  break;					\
	case '>': ADD("&gt;");  break;					\
	case '"': ADD("&#34;"); break;					\
	case '\'':ADD("&#39;"); break;					\
	default: *d = *s;   break;					\
      }									\
  }									\

  switch( str->size_shift )
  {
    case 0: COUNT(p_wchar0); break;
    case 1: COUNT(p_wchar1); break;
    case 2: COUNT(p_wchar2); break;
  }

  if( newlen == str->len )
    return; /* Already on stack. */

  {
    struct pike_string *res = begin_wide_shared_string(newlen,str->size_shift);
    switch( str->size_shift )
    {
      case 0: REPLACE(p_wchar0); break;
      case 1: REPLACE(p_wchar1); break;
      case 2: REPLACE(p_wchar2); break;
    }
    pop_stack();
    push_string( low_end_shared_string( res ) );
  }
}

/*! @decl string websocket_mask(string(8bit) str, string(8bit) mask)
 *! 
 *! Returns @expr{str@} XOR @expr{mask@}.
 */
static void f_websocket_mask( INT32 args ) {
    struct pike_string *str, *mask, *ret;
    const unsigned char *src;
    unsigned char * restrict dst;
    size_t len;
    unsigned INT32 m;

    get_all_args(NULL, args, "%n%n", &str, &mask);

    if (mask->len != 4) Pike_error("Wrong mask length.\n");

    ret = begin_shared_string(str->len);

    len = str->len;
    m = get_unaligned32(STR0(mask));

    dst = STR0(ret);
    src = STR0(str);

    for (;len >= 4; len -= 4, dst += 4, src += 4)
        set_unaligned32(dst, get_unaligned32(src) ^ m);

    if (len) {
#if PIKE_BYTEORDER == 4321
        m = bswap32(m);
#endif

        do {
            *dst++ = *src++ ^ m;
            m >>= 8;
            len --;
        } while (len);
    }

    push_string(end_shared_string(ret));
}

/*! @endmodule
 */

PIKE_MODULE_INIT
{
  ADD_FUNCTION("make_http_headers", f_make_http_headers,
	       tFunc(tMap(tStr,tOr(tStr,tArr(tStr))) tOr(tInt01,tVoid), tStr),
	       0);

  ADD_FUNCTION("http_decode_string", f_http_decode_string,
	       tFunc(tStr,tStr), 0 );

  ADD_FUNCTION("html_encode_string", f_html_encode_string,
	       tFunc(tMix,tStr), 0 );

  ADD_FUNCTION("websocket_mask", f_websocket_mask,
	       tFunc(tStr8 tStr8, tStr8), 0);

  start_new_program();
  ADD_STORAGE( struct header_buf  );
  set_init_callback( f_hp_init );
  set_exit_callback( f_hp_exit );
  ADD_FUNCTION("feed", f_hp_feed,
	       tFunc(tStr tOr(tInt01,tVoid),tArr(tOr(tStr,tMapping))), 0);
  ADD_FUNCTION( "create", f_hp_create, tFunc(tOr(tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid),tVoid), ID_PROTECTED );
  end_class( "HeaderParser", 0 );
}

PIKE_MODULE_EXIT
{
}
