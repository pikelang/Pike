/*
 * $Id: mime.c,v 1.16 1999/03/07 17:47:32 marcus Exp $
 *
 * RFC1521 functionality for Pike
 *
 * Marcus Comstedt 1996-1997
 */

#include "global.h"

#include "config.h"

RCSID("$Id: mime.c,v 1.16 1999/03/07 17:47:32 marcus Exp $");
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "error.h"

#ifdef __CHAR_UNSIGNED__
#define SIGNED signed
#else
#define SIGNED
#endif


/** Forward declarations of functions implementing Pike functions **/

static void f_decode_base64( INT32 args );
static void f_encode_base64( INT32 args );
static void f_decode_qp( INT32 args );
static void f_encode_qp( INT32 args );
static void f_decode_uue( INT32 args );
static void f_encode_uue( INT32 args );

static void f_tokenize( INT32 args );
static void f_tokenize_labled( INT32 args );
static void f_quote( INT32 args );


/** Global tables **/

static char base64tab[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static SIGNED char base64rtab[0x80-' '];
static char qptab[16] = "0123456789ABCDEF";
static SIGNED char qprtab[0x80-'0'];

#define CT_CTL     0
#define CT_WHITE   1
#define CT_ATOM    2
#define CT_SPECIAL 3
#define CT_EQUAL   4
#define CT_LPAR    5
#define CT_RPAR    6
#define CT_LBRACK  7
#define CT_RBRACK  8
#define CT_QUOTE   9
unsigned char rfc822ctype[256];


/** Externally available functions **/

/* Initialize and start module */

void pike_module_init( void )
{
  int i;

  /* Init reverse base64 mapping */
  memset( base64rtab, -1, sizeof(base64rtab) );
  for (i = 0; i < 64; i++)
    base64rtab[base64tab[i] - ' '] = i;

  /* Init reverse qp mapping */
  memset( qprtab, -1, sizeof(qprtab) );
  for (i = 0; i < 16; i++)
    qprtab[qptab[i]-'0'] = i;
  for (i = 10; i < 16; i++)
    /* Lower case hex digits */
    qprtab[qptab[i] - ('0' + 'A' - 'a')] = i;

  /* Init lexical properties of characters for MIME.tokenize() */
  memset( rfc822ctype, CT_ATOM, sizeof(rfc822ctype) );
  for (i = 0; i < 32; i++)
    rfc822ctype[i] = CT_CTL;
  rfc822ctype[127] = CT_CTL;
  rfc822ctype[' '] = CT_WHITE;
  rfc822ctype['\t'] = CT_WHITE;
  rfc822ctype['('] = CT_LPAR;
  rfc822ctype[')'] = CT_RPAR;
  rfc822ctype['['] = CT_LBRACK;
  rfc822ctype[']'] = CT_RBRACK;
  rfc822ctype['"'] = CT_QUOTE;
  rfc822ctype['='] = CT_EQUAL;
  for(i=0; i<10; i++)
    rfc822ctype[(int)"<>@,;:\\/?"[i]] = CT_SPECIAL;

  /* Add global functions */
  add_function_constant( "decode_base64", f_decode_base64,
			 "function(string:string)", OPT_TRY_OPTIMIZE );
  add_function_constant( "encode_base64", f_encode_base64,
			 "function(string,void|int:string)",OPT_TRY_OPTIMIZE );
  add_function_constant( "decode_qp", f_decode_qp,
			 "function(string:string)", OPT_TRY_OPTIMIZE );
  add_function_constant( "encode_qp", f_encode_qp,
			 "function(string,void|int:string)",OPT_TRY_OPTIMIZE );
  add_function_constant( "decode_uue", f_decode_uue,
			 "function(string:string)", OPT_TRY_OPTIMIZE );
  add_function_constant( "encode_uue", f_encode_uue,
			 "function(string,void|string:string)",
			 OPT_TRY_OPTIMIZE);

  add_function_constant( "tokenize", f_tokenize,
			 "function(string:array(string|int))",
			 OPT_TRY_OPTIMIZE );
  add_function_constant( "tokenize_labled", f_tokenize_labled,
			 "function(string:array(array(string|int)))",
			 OPT_TRY_OPTIMIZE );
  add_function_constant( "quote", f_quote,
			 "function(array(string|int):string)",
			 OPT_TRY_OPTIMIZE );
}

/* Restore and exit module */

void pike_module_exit( void )
{
}


/** Functions implementing Pike functions **/

/* MIME.decode_base64() */

static void f_decode_base64( INT32 args )
{
  if(args != 1)
    error( "Wrong number of arguments to MIME.decode_base64()\n" );
  else if (sp[-1].type != T_STRING)
    error( "Wrong type of argument to MIME.decode_base64()\n" );
  else {

    /* Decode the string in sp[-1].u.string.  Any whitespace etc
       must be ignored, so the size of the result can't be exactly
       calculated from the input size.  We'll use a dynamic buffer
       instead. */

    dynamic_buffer buf;
    SIGNED char *src;
    INT32 cnt, d = 1;
    int pads = 0;

    buf.s.str = NULL;
    initialize_buf( &buf );

    for (src = (SIGNED char *)sp[-1].u.string->str, cnt = sp[-1].u.string->len;
	 cnt--; src++)
      if(*src>=' ' && base64rtab[*src-' ']>=0) {
	/* 6 more bits to put into d */
	if((d=(d<<6)|base64rtab[*src-' '])>=0x1000000) {
	  /* d now contains 24 valid bits.  Put them in the buffer */
	  low_my_putchar( (d>>16)&0xff, &buf );
	  low_my_putchar( (d>>8)&0xff, &buf );
	  low_my_putchar( d&0xff, &buf );
	  d=1;
	}
      } else if (*src=='=') {
	/* A pad character has been encountered.
	   Increase pad count, and remove unused bits from d. */
	pads++;
	d>>=2;
      }

    /* If data size not an even multiple of 3 bytes, output remaining data */
    switch(pads) {
    case 1:
      low_my_putchar( (d>>8)&0xff, &buf );
    case 2:
      low_my_putchar( d&0xff, &buf );
    }

    /* Return result */
    pop_n_elems( 1 );
    push_string( low_free_buf( &buf ) );
  }
}

/*  Convenience function for encode_base64();  Encode groups*3 bytes from
 *  *srcp into groups*4 bytes at *destp.
 */
static int do_b64_encode( INT32 groups, unsigned char **srcp, char **destp,
			  int insert_crlf )
{
  unsigned char *src = *srcp;
  char *dest = *destp;
  int g = 0;

  while (groups--) {
    /* Get 24 bits from src */
    INT32 d = *src++<<8;
    d = (*src++|d)<<8;
    d |= *src++;
    /* Output in encoded from to dest */
    *dest++ = base64tab[d>>18];
    *dest++ = base64tab[(d>>12)&63];
    *dest++ = base64tab[(d>>6)&63];
    *dest++ = base64tab[d&63];
    /* Insert a linebreak once in a while... */
    if(insert_crlf && ++g == 19) {
      *dest++ = 13;
      *dest++ = 10;
      g=0;
    }
  }

  /* Update pointers */
  *srcp = src;
  *destp = dest;
  return g;
}

/* MIME.encode_base64() */

static void f_encode_base64( INT32 args )
{
  if(args != 1 && args != 2)
    error( "Wrong number of arguments to MIME.encode_base64()\n" );
  else if(sp[-args].type != T_STRING)
    error( "Wrong type of argument to MIME.encode_base64()\n" );
  else {

    /* Encode the string in sp[-args].u.string.  First, we need to know
       the number of 24 bit groups in the input, and the number of
       bytes actually present in the last group. */

    INT32 groups = (sp[-args].u.string->len+2)/3;
    int last = (sp[-args].u.string->len-1)%3+1;

    int insert_crlf = !(args == 2 && sp[-1].type == T_INT &&
			sp[-1].u.integer != 0);

    /* We need 4 bytes for each 24 bit group, and 2 bytes for each linebreak */
    struct pike_string *str =
      begin_shared_string( groups*4+(insert_crlf? (groups/19)*2 : 0) );

    unsigned char *src = (unsigned char *)sp[-args].u.string->str;
    char *dest = str->str;

    if (groups) {
      /* Temporary storage for the last group, as we may have to read
	 an extra byte or two and don't want to get any page-faults.  */
      unsigned char tmp[3], *tmpp = tmp;
      int i;

      if (do_b64_encode( groups-1, &src, &dest, insert_crlf ) == 18)
	/* Skip the final linebreak if it's not to be followed by anything */
	str->len -= 2;

      /* Copy the last group to temporary storage */
      tmp[1] = tmp[2] = 0;
      for (i = 0; i < last; i++)
	tmp[i] = *src++;

      /* Encode the last group, and replace output codes with pads as needed */
      do_b64_encode( 1, &tmpp, &dest, 0 );
      switch (last) {
      case 1:
	*--dest = '=';
      case 2:
	*--dest = '=';
      }
    }

    /* Return the result */
    pop_n_elems( args );
    push_string( end_shared_string( str ) );
  }
}

/* MIME.decode_qp() */

static void f_decode_qp( INT32 args )
{
  if(args != 1)
    error( "Wrong number of arguments to MIME.decode_qp()\n" );
  else if(sp[-1].type != T_STRING)
    error( "Wrong type of argument to MIME.decode_qp()\n" );
  else {

    /* Decode the string in sp[-1].u.string.  We have absolutely no idea
       how much of the input is raw data and how much is encoded data,
       so we'll use a dynamic buffer to hold the result. */

    dynamic_buffer buf;
    SIGNED char *src;
    INT32 cnt;

    buf.s.str=NULL;
    initialize_buf(&buf);

    for (src = (SIGNED char *)sp[-1].u.string->str, cnt = sp[-1].u.string->len;
	 cnt--; src++)
      if (*src == '=') {
	/* Encoded data */
	if (cnt > 0 && (src[1] == 10 || src[1] == 13)) {
	  /* A '=' followed by CR, LF or CRLF will be simply ignored. */
	  if (src[1] == 13) {
	    --cnt;
	    src++;
	  }
	  if (cnt>0 && src[1]==10) {
	    --cnt;
	    src++;
	  }
	} else if (cnt >= 2 && src[1] >= '0' && src[2] >= '0' &&
		   qprtab[src[1]-'0'] >= 0 && qprtab[src[2]-'0'] >= 0) {
	  /* A '=' followed by a hexadecimal number. */
	  low_my_putchar( (qprtab[src[1]-'0']<<4)|qprtab[src[2]-'0'], &buf );
	  cnt -= 2;
	  src += 2;
	}
      } else
	/* Raw data */
	low_my_putchar( *src, &buf );

    /* Return the result */
    pop_n_elems( 1 );
    push_string( low_free_buf( &buf ) );
  }
}

/* MIME.encode_qp() */

static void f_encode_qp( INT32 args )
{
  if (args != 1 && args != 2)
    error( "Wrong number of arguments to MIME.encode_qp()\n" );
  else if (sp[-args].type != T_STRING)
    error( "Wrong type of argument to MIME.encode_qp()\n" );
  else {

    /* Encode the string in sp[-args].u.string.  We don't know how
       much of the data has to be encoded, so let's use that trusty
       dynamic buffer once again. */

    dynamic_buffer buf;
    unsigned char *src = (unsigned char *)sp[-args].u.string->str;
    INT32 cnt;
    int col = 0;
    int insert_crlf = !(args == 2 && sp[-1].type == T_INT &&
			sp[-1].u.integer != 0);

    buf.s.str = NULL;
    initialize_buf( &buf );

    for (cnt = sp[-args].u.string->len; cnt--; src++) {
      if ((*src >= 33 && *src <= 60) ||
	  (*src >= 62 && *src <= 126))
	/* These characters can always be encoded as themselves */
	low_my_putchar( *src, &buf );
      else {
	/* Better safe than sorry, eh?  Use the dreaded hex escape */
	low_my_putchar( '=', &buf );
	low_my_putchar( qptab[(*src)>>4], &buf );
	low_my_putchar( qptab[(*src)&15], &buf );
	col += 2;
      }
      /* We'd better not let the lines get too long */
      if (++col >= 73 && insert_crlf) {
	low_my_putchar( '=', &buf );
	low_my_putchar( 13, &buf );
	low_my_putchar( 10, &buf );
	col = 0;
      }
    }
    
    /* Return the result */
    pop_n_elems( args );
    push_string( low_free_buf( &buf ) );
  }
}

/* MIME.decode_uue() */

static void f_decode_uue( INT32 args )
{
  if (args != 1)
    error( "Wrong number of arguments to MIME.decode_uue()\n" );
  else if(sp[-1].type != T_STRING)
    error( "Wrong type of argument to MIME.decode_uue()\n" );
  else {

    /* Decode string in sp[-1].u.string.  This is done much like in
       the base64 case, but we'll look for the "begin" line first.  */

    dynamic_buffer buf;
    char *src;
    INT32 cnt;

    buf.s.str = NULL;
    initialize_buf( &buf );

    src = sp[-1].u.string->str;
    cnt = sp[-1].u.string->len;

    while (cnt--)
      if(*src++=='b' && cnt>5 && !memcmp(src, "egin ", 5))
	break;

    if (cnt>=0)
      /* We found a the string "begin".  Now skip to EOL */
      while (cnt--)
	if (*src++=='\n')
	  break;

    if (cnt<0) {
      /* Could not find "begin.*\n", return 0 */
      pop_n_elems( 1 );
      push_int( 0 );
      return;
    }

    for (;;) {
      int l, g;

      /* If we run out of input, or the line starts with "end", we are done */
      if (cnt<=0 || *src=='e')
	break;

      /* Get the length byte, calculate the number of groups, and
	 check that we have sufficient data */
      l=(*src++-' ')&63;
      --cnt;
      g = (l+2)/3;
      l -= g*3;
      if ((cnt -= g*4) < 0)
	break;

      while (g--) {
	/* Read 24 bits of data */
	INT32 d = ((*src++-' ')&63)<<18;
	d |= ((*src++-' ')&63)<<12;
	d |= ((*src++-' ')&63)<<6;
	d |= ((*src++-' ')&63);
	/* Output it into the buffer */
	low_my_putchar( (d>>16)&0xff, &buf );
	low_my_putchar( (d>>8)&0xff, &buf );
	low_my_putchar( d&0xff, &buf );
      }

      /* If the line didn't contain an even multiple of 24 bits, remove
	 spurious bytes from the buffer */
      while (l++)
	low_make_buf_space( -1, &buf );

      /* Skip to EOL */
      while (cnt-- && *src++!=10);
    }

    /* Return the result */
    pop_n_elems( 1 );
    push_string( low_free_buf( &buf ) );
  }
}

/*  Convenience function for encode_uue();  Encode groups*3 bytes from
 *  *srcp into groups*4 bytes at *destp, and reserve space for last more.
 */
static void do_uue_encode( INT32 groups, unsigned char **srcp, char **destp,
			   INT32 last )
{
  unsigned char *src = *srcp;
  char *dest = *destp;

  while (groups || last) {
    /* A single line can hold at most 15 groups */
    int g = (groups >= 15? 15 : groups);

    if (g<15) {
      /* The line isn't filled completely.  Add space for the "last" bytes */
      *dest++ = ' ' + (3*g + last);
      last = 0;
    } else
      *dest++ = ' ' + (3*g);

    groups -= g;

    while (g--) {
      /* Get 24 bits of data */
      INT32 d = *src++<<8;
      d = (*src++|d)<<8;
      d |= *src++;
      /* Output it in encoded form */
      if((*dest++ = ' '+(d>>18)) == ' ') dest[-1]='`';
      if((*dest++ = ' '+((d>>12)&63)) == ' ') dest[-1]='`';
      if((*dest++ = ' '+((d>>6)&63)) == ' ') dest[-1]='`';
      if((*dest++ = ' '+(d&63)) == ' ') dest[-1]='`';
    }

    if(groups || last) {
      /* There's more data to be written, so add a linebreak before looping */
      *dest++ = 13;
      *dest++ = 10;
    }
  }

  /* Update pointers */
  *srcp = src;
  *destp = dest;
}

/* MIME.encode_uue() */

static void f_encode_uue( INT32 args )
{
  if (args != 1 && args != 2)
    error( "Wrong number of arguments to MIME.encode_uue()\n" );
  else if (sp[-args].type != T_STRING ||
	   (args == 2 && sp[-1].type != T_VOID && sp[-1].type != T_STRING &&
	    sp[-1].type != T_INT))
    error( "Wrong type of argument to MIME.encode_uue()\n" );
  else {

    /* Encode string in sp[-args].u.string.  If args == 2, there may be
       a filename in sp[-1].u.string.  If we don't get a filename, use
       the generic filename "attachment"... */

    char *dest, *filename = "attachment";
    struct pike_string *str;
    unsigned char *src = (unsigned char *) sp[-args].u.string->str;
    /* Calculate number of 24 bit groups, and actual # of bytes in last grp */
    INT32 groups = (sp[-args].u.string->len + 2)/3;
    int last= (sp[-args].u.string->len - 1)%3 + 1;

    /* Get the filename if provided */
    if (args == 2 && sp[-1].type == T_STRING)
      filename = sp[-1].u.string->str;

    /* Allocate the space we need.  This included space for the actual
       data, linebreaks and the "begin" and "end" lines (including filename) */
    str = begin_shared_string( groups*4 + ((groups + 14)/15)*3 +
			       strlen( filename ) + 20 );
    dest = str->str;

    /* Write the begin line containing the filename */
    sprintf(dest, "begin 644 %s\r\n", filename);
    dest += 12 + strlen(filename);

    if (groups) {
      /* Temporary storage for the last group, as we may have to read
	 an extra byte or two and don't want to get any page-faults.  */
      unsigned char tmp[3], *tmpp=tmp;
      char *kp, k;
      int i;

      do_uue_encode( groups-1, &src, &dest, last );

      /* Copy the last group into temporary storage */
      tmp[1] = tmp[2] = 0;
      for (i = 0; i < last; i++)
	tmp[i] = *src++;

      /* Remember the address and contents of the last character written.
	 This will get overwritten by a fake length byte which we will
	 then replace with the originial character */
      k = *--dest;
      kp = dest;

      do_uue_encode( 1, &tmpp, &dest, 0 );

      /* Restore the saved character */
      *kp = k;

      /* Replace final nulls with pad characters if neccesary */
      switch (last) {
      case 1:
	dest[-2] = '`';
      case 2:
	dest[-1] = '`';
      }

      /* Add a final linebreak after the last group */
      *dest++ = 13;
      *dest++ = 10;
    }

    /* Put a terminating line (length byte `) and the "end" line into buffer */
    memcpy( dest, "`\r\nend\r\n", 8 );

    /* Return the result */
    pop_n_elems( args );
    push_string( end_shared_string( str ) );
  }
}

/* MIME.tokenize() */

static void low_tokenize( INT32 args, int mode )
{

  /* Tokenize string in sp[-args].u.string.  We'll just push the
     tokens on the stack, and then do an aggregate_array just
     before exiting. */
  
  unsigned char *src = (unsigned char *)sp[-args].u.string->str;
  struct array *arr;
  struct pike_string *str;
  INT32 cnt = sp[-args].u.string->len, n = 0, l, e, d;
  char *p;

  while (cnt>0)
    switch (rfc822ctype[*src]) {
    case CT_EQUAL:
      /* Might be an encoded word.  Check it out. */
      l = 0;
      if (cnt>5 && src[1] == '?') {
	int nq = 0;
	for (l=2; l<cnt && nq<3; l++)
	  if (src[l]=='?')
	    nq ++;
	  else if(rfc822ctype[src[l]]<=CT_WHITE)
	    break;
	if (nq == 3 && l<cnt && src[l] == '=')
	  l ++;
	else
	  l = 0;
      }
      if (l>0) {
	/* Yup.  It's an encoded word, so it must be an atom.  */
	if(mode)
	  push_constant_text("encoded-word");
	push_string( make_shared_binary_string( (char *)src, l ) );
	if(mode)
	  f_aggregate(2);
	n++;
	src += l;
	cnt -= l;
	break;
      }
    case CT_SPECIAL:
    case CT_RBRACK:
    case CT_RPAR:
      /* Individual special character, push as a char (= int) */
      if(mode)
	push_constant_text("special");
      push_int( *src++ );
      if(mode)
	f_aggregate(2);
      n++;
      --cnt;
      break;

    case CT_ATOM:
      /* Atom, find length then push as a string */
      for (l=1; l<cnt; l++)
	if (rfc822ctype[src[l]] != CT_ATOM)
	  break;

      if(mode)
	push_constant_text("word");
      push_string( make_shared_binary_string( (char *)src, l ) );
      if(mode)
	f_aggregate(2);
      n++;
      src += l;
      cnt -= l;
      break;

    case CT_QUOTE:
      /* Quoted-string, find length then push as a string while removing
	 escapes. */
      for (e = 0, l = 1; l < cnt; l++)
	if (src[l] == '"')
	  break;
	else
	  if (src[l] == '\\') {
	    e++;
	    l++;
	  }

      /* l is the distance to the ending ", and e is the number of \
	 escapes encountered on the way */
      str = begin_shared_string( l-e-1 );

      /* Copy the string and remove \ escapes */
      for (p = str->str, e = 1; e < l; e++)
	*p++ = (src[e] == '\\'? src[++e] : src[e]);

      /* Push the resulting string */
      if(mode)
	push_constant_text("word");
      push_string( end_shared_string( str ) );
      if(mode)
	f_aggregate(2);
      n++;
      src += l+1;
      cnt -= l+1;
      break;

    case CT_LBRACK:
      /* Domain literal.  Handled just like quoted-string, except that
	 ] marks the end of the token, not ". */
      for (e = 0, l = 1; l < cnt; l++)
	if(src[l] == ']')
	  break;
	else
	  if(src[l] == '\\') {
	    e++;
	    l++;
	  }

      if (l >= cnt) {
	/* No ]; seems that this was no domain literal after all... */
	if(mode)
	  push_constant_text("special");
	push_int( *src++ );
	if(mode)
	  f_aggregate(2);
	n++;
	--cnt;
	break;
      }

      /* l is the distance to the ending ], and e is the number of \
	 escapes encountered on the way */
      str = begin_shared_string( l-e+1 );

      /* Copy the literal and remove \ escapes */
      for (p = str->str, e = 0; e <= l; e++)
	*p++ = (src[e] == '\\'? src[++e] : src[e]);

      /* Push the resulting string */
      if(mode)
	push_constant_text("domain-literal");
      push_string( end_shared_string( str ) );
      if(mode)
	f_aggregate(2);
      n++;
      src += l+1;
      cnt -= l+1;
      break;

    case CT_LPAR:
      /* Comment.  Nested comments are allowed, so we'll use d to
	 keep track of the nesting level. */
      for (e = 0, d = 1, l = 1; l < cnt; l++)
	if (src[l] == '(')
	  /* One level deeper nesting */
	  d++;
	else if(src[l] == ')') {
	  /* End of comment level.  If nesting reaches 0, we're done */
	  if(!--d)
	    break;
	} else
	  /* Skip escaped characters */
	  if(src[l] == '\\') {
	    e++;
	    l++;
	  }

      if(mode) {
	push_constant_text("comment");

	str = begin_shared_string( l-e-1 );

	/* Copy the comment and remove \ escapes */
	for (p = str->str, e = 1; e < l; e++)
	  *p++ = (src[e] == '\\'? src[++e] : src[e]);

	push_string( end_shared_string( str ) );
	f_aggregate(2);
	n++;
      }

      /* Skip the comment altogether */
      src += l+1;
      cnt -= l+1;
      break;

    case CT_WHITE:
      /* Whitespace, just ignore it */
      src++;
      --cnt;
      break;

    default:
      error( "Invalid character in header field\n" );
    }

  /* Create the resulting array and push it */
  arr = aggregate_array( n );
  pop_n_elems( 1 );
  push_array( arr );
}

static void f_tokenize( INT32 args )
{
  if (args != 1)
    error( "Wrong number of arguments to MIME.tokenize()\n" );

  if (sp[-1].type != T_STRING)
    error( "Wrong type of argument to MIME.tokenize()\n" );

  low_tokenize( args, 0 );
}

static void f_tokenize_labled( INT32 args )
{
  if (args != 1)
    error( "Wrong number of arguments to MIME.tokenize_labled()\n" );

  if (sp[-1].type != T_STRING)
    error( "Wrong type of argument to MIME.tokenize_labled()\n" );

  low_tokenize( args, 1 );
}


/*  Convenience function for quote() which determines if a sequence of
 *  characters can be stored as an atom.
 */
static int check_atom_chars( unsigned char *str, INT32 len )
{
  /* Atoms must contain at least 1 character... */
  if (len < 1)
    return 0;

  /* Check the individual characters */
  while (len--)
    if (*str >= 0x80 || rfc822ctype[*str] != CT_ATOM)
      return 0;
    else
      str++;

  /* Ok, it's safe */
  return 1;
}

/*  This one check is a sequence of charactes is actually an encoded word.
 */
static int check_encword( unsigned char *str, INT32 len )
{
  int q = 0;

  /* An encoded word begins with =?, ends with ?= and contains 2 internal ? */
  if (len < 6 || str[0] != '=' || str[1] != '?' ||
      str[len-2] != '?' || str[len-1] != '=')
    return 0;

  /* Remove =? and ?= */
  len -= 4;
  str += 2;

  /* Count number of internal ? */
  while (len--)
    if (*str++ == '?')
      if(++q > 2)
	return 0;

  /* If we found exactly 2, this is an encoded word. */
  return q == 2;
}

/* MIME.quote() */

static void f_quote( INT32 args )
{
  struct svalue *item;
  INT32 cnt;
  dynamic_buffer buf;
  int prev_atom = 0;

  if (args != 1)
    error( "Wrong number of arguments to MIME.quote()\n" );
  else if (sp[-1].type != T_ARRAY)
    error( "Wrong type of argument to MIME.quote()\n" );

  /* Quote array in sp[-1].u.array.  Once again we'll rely on a
     dynamic_buffer to collect the output string. */

  buf.s.str = NULL;
  initialize_buf( &buf );

  for (cnt=sp[-1].u.array->size, item=sp[-1].u.array->item; cnt--; item++) {

    if (item->type == T_INT) {

      /* Single special character */
      low_my_putchar( item->u.integer, &buf );
      prev_atom = 0;

    } else if (item->type != T_STRING) {

      /* Neither int or string.  Too bad... */
      toss_buffer( &buf );
      error( "Wrong type of argument to MIME.quote()\n" );

    } else {

      /* It's a string, so we'll store it either as an atom, or
	 as a quoted-string */
      struct pike_string *str = item->u.string;

      /* In case the previous item was also a string, we'll add a single
	 whitespace as a delimiter */
      if (prev_atom)
	low_my_putchar( ' ', &buf );

      if ((str->len>5 && str->str[0]=='=' && str->str[1]=='?' &&
	   check_encword((unsigned char *)str->str, str->len)) ||
	  check_atom_chars((unsigned char *)str->str, str->len)) {

	/* Valid atom without quotes... */
	low_my_binary_strcat( str->str, str->len, &buf );

      } else {

	/* Have to use quoted-string */
	INT32 len = str->len;
	char *src = str->str;
	low_my_putchar( '"', &buf );
	while(len--) {
	  if(*src=='"' || *src=='\\' || *src=='\r')
	    /* Some characters have to be escaped even within quotes... */
	    low_my_putchar( '\\', &buf );
	  low_my_putchar( *src++, &buf );
	}
	low_my_putchar( '"', &buf );

      }

      prev_atom = 1;

    }
  }

  /* Return the result */
  pop_n_elems( 1 );
  push_string( low_free_buf( &buf ) );
}
