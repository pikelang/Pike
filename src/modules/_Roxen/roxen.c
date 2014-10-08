/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#define NO_PIKE_SHORTHAND

#include "global.h"
#include "config.h"


#include "machine.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "fdlib.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "machine.h"
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


/*! @module _Roxen
 */

/*! @class HeaderParser
 *!
 *! Class for parsing HTTP-requests.
 */

#define THP ((struct header_buf *)Pike_fp->current_storage)
struct  header_buf
{
  unsigned char *headers;
  unsigned char *pnt;
  ptrdiff_t hsize, left;
  int slash_n, tslash_n, spc;
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
/*! @decl array(string|mapping) feed(string data)
 *!
 *! @returns
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
  struct pike_string *str = Pike_sp[-1].u.string;
  struct header_buf *hp = THP;
  int str_len;
  int tot_slash_n=hp->slash_n, slash_n = hp->tslash_n, spc = hp->spc;
  unsigned char *pp,*ep;
  struct svalue *tmp;
  struct mapping *headers;
  ptrdiff_t os=0, i, j, l;
  unsigned char *in;

  if (args != 1)
    Pike_error("Bad number of arguments to feed().\n");
  if( TYPEOF(Pike_sp[-1]) != PIKE_T_STRING )
    Pike_error("Wrong type of argument to feed()\n");
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
  for( ep=(hp->pnt+str_len),pp=MAXIMUM(hp->headers,hp->pnt-3);
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

  hp->slash_n = tot_slash_n;
  hp->spc = spc;
  hp->tslash_n  = slash_n;
  hp->left -= str_len;
  hp->pnt += str_len;
  hp->pnt[0] = 0;

  if( slash_n != 2 )
  {
    /* one newline, but less than 2 space,
     *    --> HTTP/0.9 or broken request 
     */
    if( (spc < 2) && tot_slash_n )
    {
      push_empty_string();
      /* This includes (all eventual) \r\n etc. */
      push_text((char *)hp->headers); 
      f_aggregate_mapping( 0 );
      f_aggregate( 3 );
      return;
    }
    push_int( 0 );
    return;
  }
  
  /*leftovers*/
  push_string(make_shared_binary_string((char *)pp, hp->pnt - pp));
  headers = allocate_mapping( 5 );
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

  /* Parse headers. */
  for(i = 0; i < l; i++)
  {
    if(in[i] > 64 && in[i] < 91)
      in[i]+=32;	/* lower_case */
    else if( in[i] == ':' )
    {
      /* FIXME: Does not support white space before the colon. */
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

	push_string(make_shared_binary_string((char*)in+os,j-os));
	val_cnt++;

	if((in[j] == '\r') && (in[j+1] == '\n')) j++;
	os = j+1;
	i = j;
	/* Check for continuation line. */
      } while ((os < l) && ((in[os] == ' ') || (in[os] == '\t')));

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
      if( THP->mode == 1 )
      {
        /* FIXME: Reset stack so that backtrace shows faulty header. */
        Pike_error("Malformed HTTP header.\n");
      }
      else
        os = i+1;
    }
  }
  push_mapping( headers );
  f_aggregate( 3 );             /* data, firstline, headers */
}

static void f_hp_create( INT32 args )
/*! @decl void create(int throw_errors)
 */
{
  if (THP->headers) {
    free(THP->headers);
    THP->headers = NULL;
  }

  THP->mode = 0;
  get_all_args("create",args,".%i",&THP->mode);

  THP->headers = xalloc( 8192 );
  THP->pnt = THP->headers;
  THP->hsize = 8192;
  THP->left = 8192;
  THP->spc = THP->slash_n = 0;
  pop_n_elems(args);
  push_int(0);
}

/*! @endclass
 */

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
    if( TYPEOF(k->ind) != PIKE_T_STRING || k->ind.u.string->size_shift )
      Pike_error("Wrong argument type to make_http_headers("
            "mapping(string(8bit):string(8bit)|array(string(8bit))) heads)\n");
    if( TYPEOF(k->val) == PIKE_T_STRING && !k->val.u.string->size_shift )
      total_len +=  k->val.u.string->len + 2 + k->ind.u.string->len + 2;
    else if( TYPEOF(k->val) == PIKE_T_ARRAY )
    {
      struct array *a = k->val.u.array;
      ptrdiff_t i, kl = k->ind.u.string->len + 2 ;
      for( i = 0; i<a->size; i++ )
        if( TYPEOF(a->item[i]) != PIKE_T_STRING ||
	    a->item[i].u.string->size_shift )
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

static void f_http_decode_string(INT32 args)
/*! @decl string http_decode_string(string encoded)
 *!
 *! Decodes an http transport-encoded string. Knows about %XX and
 *! %uXXXX syntax. Treats %UXXXX as %uXXXX. It will treat '+' as '+'
 *! and not ' ', so form decoding needs to replace that in a second
 *! step.
 */
{
   int proc = 0;
   int size_shift;
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
   for (; COMPARE_PCHARP(foo, <, end); INC_PCHARP(foo, 1)) {
     p_wchar2 c = INDEX_PCHARP(foo, 0);
     if (c == '%') {
       c = INDEX_PCHARP(foo, 1);
       if (c == 'u' || c == 'U') {
	 /* %uXXXX */
	 if (INDEX_PCHARP(foo, 2) != '0' || INDEX_PCHARP(foo, 3) != '0') {
	   if (!size_shift) size_shift = 1;
	 }
	 proc += 5;
	 INC_PCHARP(foo, 5);
       } else {
	 proc += 2;
	 INC_PCHARP(foo, 2);
       }
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
       if (c == 'u' || c == 'U') {
	 c = 0;
	 if (SUBTRACT_PCHARP(end, foo) > 5) {
	   p_wchar2 hex = INDEX_PCHARP(foo, 2);
	   c = (((hex<'A')?hex:(hex + 9)) & 15)<<12;
	   hex = INDEX_PCHARP(foo, 3);
	   c |= (((hex<'A')?hex:(hex + 9)) & 15)<<8;
	   hex = INDEX_PCHARP(foo, 4);
	   c |= (((hex<'A')?hex:(hex + 9)) & 15)<<4;
	   hex = INDEX_PCHARP(foo, 5);
	   c |= ((hex<'A')?hex:(hex + 9)) & 15;
	 }
	 INC_PCHARP(foo, 5);
       } else {
	 c = 0;
	 if (SUBTRACT_PCHARP(end, foo) > 2) {
	   p_wchar2 hex = INDEX_PCHARP(foo, 1);
	   c = (((hex<'A')?hex:(hex + 9)) & 15)<<4;
	   hex = INDEX_PCHARP(foo, 2);
	   c |= ((hex<'A')?hex:(hex + 9)) & 15;
	 }
	 INC_PCHARP(foo, 2);
       }
     }
     string_builder_putchar(&newstr, c);
   }

   pop_n_elems(args);
   push_string(finish_string_builder(&newstr));
}

static void f_html_encode_string( INT32 args )
/*! @decl string html_encode_string(mixed in)
 *!
 *! Encodes the @[in] data as an HTML safe string.
 */
{
  struct pike_string *str;
  int newlen;

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

  start_new_program();
  ADD_STORAGE( struct header_buf  );
  set_init_callback( f_hp_init );
  set_exit_callback( f_hp_exit );
  ADD_FUNCTION( "feed", f_hp_feed, tFunc(tStr,tArr(tOr(tStr,tMapping))), 0 );
  ADD_FUNCTION( "create", f_hp_create, tFunc(tOr(tInt,tVoid),tVoid), ID_PROTECTED );
  end_class( "HeaderParser", 0 );
}

PIKE_MODULE_EXIT
{
}
