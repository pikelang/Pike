/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: roxen.c,v 1.42 2005/04/08 17:23:46 grubba Exp $
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
 */

#define THP ((struct header_buf *)Pike_fp->current_storage)
struct  header_buf
{
  unsigned char *headers;
  unsigned char *pnt;
  ptrdiff_t hsize, left;
  int slash_n, spc;
};

static void f_hp_init( struct object *o )
{
  THP->headers = NULL;
  THP->pnt = NULL;
  THP->hsize = 0;
  THP->left = 0;
  THP->spc = THP->slash_n = 0;
}

static void f_hp_exit( struct object *o )
{
  if( THP->headers )
    free( THP->headers );
  THP->headers = NULL;
  THP->pnt = NULL;
  THP->hsize = 0;
}

static void f_hp_feed( INT32 args )
/*! @decl array(string|mapping) feed(string data)
 */
{
  struct pike_string *str = Pike_sp[-1].u.string;
  struct header_buf *hp = THP;
  int str_len;
  int tot_slash_n=hp->slash_n, slash_n = 0, spc = hp->spc;
  unsigned char *pp,*ep;
  struct svalue *tmp;
  struct mapping *headers;
  ptrdiff_t os=0, i, j, l;
  unsigned char *in;

  if( Pike_sp[-1].type != PIKE_T_STRING )
    Pike_error("Wrong type of argument to feed()\n");
  if( str->size_shift )
    Pike_error("Wide string headers not supported\n");
  str_len = str->len;
  while( str_len >= hp->left )
  {
    unsigned char *buf;
    if( THP->hsize > 512 * 1024 )
      Pike_error("Too many headers\n");
    THP->hsize += 8192;
    buf = THP->headers;
    THP->headers = realloc( THP->headers, THP->hsize );
    if( !THP->headers )
    {
      free(buf);
      THP->hsize = 0;
      THP->left = 0;
      THP->spc = THP->slash_n = 0;
      THP->pnt = NULL;
      Pike_error("Running out of memory in header parser\n");
    }
    THP->left += 8192;
    THP->pnt = (THP->headers + THP->hsize - THP->left);
  }

  MEMCPY( hp->pnt, str->str, str_len );
  pop_n_elems( args );

  /* FIXME: The below does not support lines terminated with just \r. */
  for( ep=(hp->pnt+str_len),pp=MAXIMUM(hp->headers,hp->pnt-3);
       pp<ep && slash_n<2; pp++ )
    if( *pp == ' ' )  spc++;
    else if( *pp == '\n' ) slash_n++, tot_slash_n++;
    else if( *pp != '\r' ) slash_n=0;

  hp->slash_n = tot_slash_n;
  hp->spc = spc;
  
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
      push_constant_text( "" );
      /* This includes (all eventual) \r\n etc. */
      push_text( hp->headers ); 
      f_aggregate_mapping( 0 );
      f_aggregate( 3 );
      return;
    }
    push_int( 0 );
    return;
  }

  push_string( make_shared_binary_string( pp, hp->pnt - pp ) ); /*leftovers*/
  headers = allocate_mapping( 5 );
  in = hp->headers;
  l = pp - hp->headers;

  /* find first line here */
  for( i = 0; i < l; i++ )
    if((in[i] == '\n') || (in[i] == '\r'))
      break;

  push_string( make_shared_binary_string( in, i ) );

  if((in[i] == '\r') && (in[i+1] == '\n'))
    i++;
  i++;

  in += i; l -= i;

  /* Parse headers. */
  for(i = 0; i < l; i++)
  {
    if(in[i] > 64 && in[i] < 91) in[i]+=32;	/* lower_case */
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
	f_aggregate( 1 );
	if( tmp->type == PIKE_T_ARRAY )
	{
	  ref_push_array(tmp->u.array);
	  map_delete(headers, Pike_sp-3);
	  f_add(2);
	} else {
	  ref_push_string(tmp->u.string);
	  f_aggregate(1);
	  map_delete(headers, Pike_sp-3);
	  f_add(2);
	}
      }
      mapping_insert(headers, Pike_sp-2, Pike_sp-1);

      pop_n_elems(2);
    }
  }
  push_mapping( headers );
  f_aggregate( 3 );             /* data, firstline, headers */
}

static void f_hp_create( INT32 args )
/*! @decl void create(void)
 */
{
  if (THP->headers) {
    free(THP->headers);
    THP->headers = NULL;
  }
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
 *!          make_http_headers(mapping(string:string|array(string)) headers)
 */
{
  int total_len = 0, e;
  unsigned char *pnt;
  struct mapping *m;
  struct keypair *k;
  struct pike_string *res;
  if( Pike_sp[-1].type != PIKE_T_MAPPING )
    Pike_error("Wrong argument type to make_http_headers(mapping heads)\n");

  m = Pike_sp[-1].u.mapping;
  /* loop to check len */
  NEW_MAPPING_LOOP( m->data )
  {
    if( k->ind.type != PIKE_T_STRING || k->ind.u.string->size_shift )
      Pike_error("Wrong argument type to make_http_headers("
            "mapping(string(8bit):string(8bit)|array(string(8bit))) heads)\n");
    if( k->val.type == PIKE_T_STRING && !k->val.u.string->size_shift )
      total_len +=  k->val.u.string->len + 2 + k->ind.u.string->len + 2;
    else if( k->val.type == PIKE_T_ARRAY )
    {
      struct array *a = k->val.u.array;
      ptrdiff_t i, kl = k->ind.u.string->len + 2 ;
      for( i = 0; i<a->size; i++ )
        if( a->item[i].type != PIKE_T_STRING||a->item[i].u.string->size_shift )
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
  total_len += 2;

  res = begin_shared_string( total_len );
  pnt = STR0(res);
#define STRADD(X)\
    for( l=X.u.string->len,s=X.u.string->str,c=0; c<l; c++ )\
      *(pnt++)=*(s++);

  NEW_MAPPING_LOOP( m->data )
  {
    unsigned char *s;
    ptrdiff_t l, c;
    if( k->val.type == PIKE_T_STRING )
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
  *(pnt++) = '\r';
  *(pnt++) = '\n';

  pop_n_elems( args );
  push_string( end_shared_string( res ) );
}

static void f_http_decode_string(INT32 args)
/*! @decl string http_decode_string(string encoded)
 *!
 *! Decodes an http transport-encoded string.
 */
{
   int proc;
   int size_shift = 0;
   int adjust_len = 0;
   p_wchar0 *foo, *bar, *end;
   struct pike_string *newstr;

   if (!args || Pike_sp[-args].type != PIKE_T_STRING ||
       Pike_sp[-args].u.string->size_shift)
     Pike_error("Invalid argument to http_decode_string(string(8bit));\n");

   foo = bar = STR0(Pike_sp[-args].u.string);
   end = foo + Pike_sp[-args].u.string->len;

   /* count '%' and wide characters */
   for (proc=0; foo<end; foo++) {
     if (*foo=='%') {
       proc++;
       if (foo[1] == 'u' || foo[1] == 'U') {
	 /* %uXXXX */
	 if (foo[2] != '0' || foo[3] != '0') {
	   size_shift = 1;
	 }
	 foo += 5;
	 if (foo < end) {
	   adjust_len += 5;
	 } else {
	   adjust_len += end - (foo - 4);
	 }
       } else {
	 foo += 2;
	 if (foo < end) {
	   adjust_len += 2;
	 } else {
	   adjust_len += end - (foo - 1);
	 }
       }
     }
   }

   if (!proc) { pop_n_elems(args-1); return; }

   newstr = begin_wide_shared_string(Pike_sp[-args].u.string->len - adjust_len,
				     size_shift);
   if (size_shift) {
     p_wchar1 *dest = STR1(newstr);

     for (proc=0; bar<end; dest++)
       if (*bar=='%') {
	 if (bar[1] == 'u' || bar[1] == 'U') {
	   if (bar<end-5)
	     *dest = (((bar[2]<'A')?(bar[2]&15):((bar[2]+9)&15))<<12)|
	       (((bar[3]<'A')?(bar[3]&15):((bar[3]+9)&15))<<8)|
	       (((bar[4]<'A')?(bar[4]&15):((bar[4]+9)&15))<<4)|
	       ((bar[5]<'A')?(bar[5]&15):((bar[5]+9)&15));
	   else
	     *dest=0;
	   bar+=6;
	 } else {
	   if (bar<end-2)
	     *dest=(((bar[1]<'A')?(bar[1]&15):((bar[1]+9)&15))<<4)|
	       ((bar[2]<'A')?(bar[2]&15):((bar[2]+9)&15));
	   else
	     *dest=0;
	   bar+=3;
	 }
       } else {
	 *dest=*(bar++);
       }
   } else {
     foo = newstr->str;
     for (proc=0; bar<end; foo++)
       if (*bar=='%') {
	 if (bar[1] == 'u' || bar[1] == 'U') {
	   /* We know that the following two characters are zeros. */
	   bar+=3;
	 }
	 if (bar<end-2)
	   *foo=(((bar[1]<'A')?(bar[1]&15):((bar[1]+9)&15))<<4)|
	     ((bar[2]<'A')?(bar[2]&15):((bar[2]+9)&15));
	 else
	   *foo=0;
	 bar+=3;
       } else {
	 *foo=*(bar++);
       }
   }
   pop_n_elems(args);
   push_string(end_shared_string(newstr));
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
  
  switch( Pike_sp[-1].type )
  {
    void o_cast_to_string();

    case PIKE_T_INT:
      /* Optimization, this is basically a inlined cast_int_to_string */
      {
	char buf[21], *b = buf+19;
	int neg, i, j=0;
	i = Pike_sp[-1].u.integer;
	pop_stack();
	if( i < 0 )
	{
	  neg = 1;
	  i = -i;
	}
	else
	  neg = 0;

	buf[20] = 0;

	while( i >= 10 )
	{
	  b[ -j++ ] = '0'+(i%10);
	  i /= 10;
	}
	b[ -j++ ] = '0'+(i%10);
	if( neg )  b[ -j++ ] = '-';
	push_text( b-j+1 );
      }
      return;

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
	       tFunc(tMap(tStr,tOr(tStr,tArr(tStr))),tStr), 0 );

  ADD_FUNCTION("http_decode_string", f_http_decode_string,
	       tFunc(tStr,tStr), 0 );

  ADD_FUNCTION("html_encode_string", f_html_encode_string,
	       tFunc(tMix,tStr), 0 );

  start_new_program();
  ADD_STORAGE( struct header_buf  );
  set_init_callback( f_hp_init );
  set_exit_callback( f_hp_exit );
  ADD_FUNCTION( "feed", f_hp_feed, tFunc(tStr,tArr(tOr(tStr,tMapping))), 0 );
  ADD_FUNCTION( "create", f_hp_create, tFunc(tNone,tVoid), ID_STATIC );
  end_class( "HeaderParser", 0 );
}

PIKE_MODULE_EXIT
{
}
