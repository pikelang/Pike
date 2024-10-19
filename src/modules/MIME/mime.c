/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * @rfc{1521@} and @rfc{4648@} functionality for Pike
 *
 * Marcus Comstedt 1996-1999
 */

#include "module.h"
#include "config.h"

#include "pike_macros.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "pike_error.h"

#ifdef __CHAR_UNSIGNED__
#define SIGNED signed
#else
#define SIGNED
#endif


#define sp Pike_sp

/** Forward declarations of functions implementing Pike functions **/

static void f_decode_base64( INT32 args );
static void f_decode_base64url( INT32 args );
static void f_decode_base32( INT32 args );
static void f_decode_base32hex( INT32 args );
static void f_decode_crypt64( INT32 args );
static void f_encode_base64( INT32 args );
static void f_encode_base64url( INT32 args );
static void f_encode_base32( INT32 args );
static void f_encode_base32hex( INT32 args );
static void f_encode_crypt64( INT32 args );
static void f_decode_qp( INT32 args );
static void f_encode_qp( INT32 args );
static void f_decode_uue( INT32 args );
static void f_encode_uue( INT32 args );

static void f_tokenize( INT32 args );
static void f_tokenize_labled( INT32 args );
static void f_quote( INT32 args );
static void f_quote_labled( INT32 args );


/** Global tables **/

static const char base64tab[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64urltab[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const char base32tab[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char base32hextab[32] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
static const char crypt64tab[64] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static SIGNED char base64rtab[(1<<(CHAR_BIT-1))-' '];
static SIGNED char base64urlrtab[(1<<(CHAR_BIT-1))-' '];
static SIGNED char base32rtab[(1<<(CHAR_BIT-1))-' '];
static SIGNED char base32hexrtab[(1<<(CHAR_BIT-1))-' '];
static SIGNED char crypt64rtab[(1<<(CHAR_BIT-1))-' '];
static const char qptab[16] = "0123456789ABCDEF";
static SIGNED char qprtab[(1<<(CHAR_BIT-1))-'0'];

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
unsigned char rfc822ctype[1<<CHAR_BIT];

/*! @module MIME
 */

/*! @decl constant TOKENIZE_KEEP_ESCAPES
 *!
 *! Don't unquote backslash-sequences in quoted strings during tokenizing.
 *! This is used for bug-compatibility with Microsoft...
 *!
 *! @seealso
 *!   @[tokenize()], @[tokenize_labled()]
 */
#define TOKENIZE_KEEP_ESCAPES	1

/** Externally available functions **/

/* Initialize and start module */

PIKE_MODULE_INIT
{
  int i;

  Pike_compiler->new_program->id = PROG_MODULE_MIME_ID;

  /* Init reverse base64 mapping */
  memset( base64rtab, -1, sizeof(base64rtab) );
  for (i = 0; i < 64; i++)
    base64rtab[base64tab[i] - ' '] = i;

  /* Init reverse base64url mapping */
  memset( base64urlrtab, -1, sizeof(base64urlrtab) );
  for (i = 0; i < 64; i++)
    base64urlrtab[base64urltab[i] - ' '] = i;

  /* Init reverse base32 mapping */
  memset( base32rtab, -1, sizeof(base32rtab) );
  for (i = 0; i < 32; i++) {
    base32rtab[base32tab[i] - ' '] = i;
    if (base32tab[i] >= 'A') {
      /* Support lower-case too. */
      base32rtab[base32tab[i] - ' ' + ('a' - 'A')] = i;
    }
  }

  /* Init reverse base32 mapping */
  memset( base32hexrtab, -1, sizeof(base32hexrtab) );
  for (i = 0; i < 32; i++) {
    base32hexrtab[base32hextab[i] - ' '] = i;
    if (base32hextab[i] >= 'A') {
      /* Support lower-case too. */
      base32hexrtab[base32hextab[i] - ' ' + ('a' - 'A')] = i;
    }
  }

  /* Init reverse crypt64 mapping */
  memset( crypt64rtab, -1, sizeof(crypt64rtab) );
  for (i = 0; i < 64; i++)
    crypt64rtab[crypt64tab[i] - ' '] = i;

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
  for(i=0; i<9; i++)
    rfc822ctype[(int)"<>@,;:\\/?"[i]] = CT_SPECIAL;

  /* Add global functions */

  ADD_FUNCTION2( "decode_base64", f_decode_base64,
                 tFunc(tStr7, tStr8), 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "decode_base64url", f_decode_base64url,
                 tFunc(tStr7, tStr8), 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "decode_base32", f_decode_base32,
                 tFunc(tStr7, tStr8), 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "decode_base32hex", f_decode_base32hex,
                 tFunc(tStr7, tStr8), 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "decode_crypt64", f_decode_crypt64,
                 tFunc(tStr7, tStr8), 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_base64", f_encode_base64,
                 tFunc(tStr tOr(tVoid,tInt) tOr(tVoid,tInt), tStr7),
		 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_base64url", f_encode_base64url,
                 tFunc(tStr tOr(tVoid,tInt) tOr(tVoid,tInt), tStr7),
		 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_base32", f_encode_base32,
                 tFunc(tStr8 tOr(tVoid,tInt) tOr(tVoid,tInt), tStr7),
		 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_base32hex", f_encode_base32hex,
                 tFunc(tStr8 tOr(tVoid,tInt) tOr(tVoid,tInt), tStr7),
		 0, OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_crypt64", f_encode_crypt64,
                 tFunc(tStr8, tStr7), 0, OPT_TRY_OPTIMIZE );

  add_function_constant( "decode_qp", f_decode_qp,
			 "function(string:string)", OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_qp", f_encode_qp,
                 tFunc(tStr8 tOr(tVoid, tInt), tStr7), 0, OPT_TRY_OPTIMIZE );

  add_function_constant( "decode_uue", f_decode_uue,
			 "function(string:string)", OPT_TRY_OPTIMIZE );

  ADD_FUNCTION2( "encode_uue", f_encode_uue,
                 tFunc(tStr8 tOr(tVoid, tStr7), tStr7), 0, OPT_TRY_OPTIMIZE );

  add_integer_constant("TOKENIZE_KEEP_ESCAPES", TOKENIZE_KEEP_ESCAPES, 0);

  add_function_constant( "tokenize", f_tokenize,
			 "function(string, int|void:array(string|int))",
			 OPT_TRY_OPTIMIZE );
  add_function_constant( "tokenize_labled", f_tokenize_labled,
			 "function(string, int|void:array(array(string|int)))",
			 OPT_TRY_OPTIMIZE );
  add_function_constant( "quote", f_quote,
			 "function(array(string|int):string)",
			 OPT_TRY_OPTIMIZE );
  add_function_constant( "quote_labled", f_quote_labled,
			 "function(array(array(string|int)):string)",
			 OPT_TRY_OPTIMIZE );
}

/* Restore and exit module */

PIKE_MODULE_EXIT
{
}

static void decode_base64( INT32 args, const char *name, const SIGNED char *tab)
{
  /* Decode the string in sp[-1].u.string.  Any whitespace etc must be
     ignored, so the size of the result can't be exactly calculated
     from the input size.  We'll use a string builder instead. */

  struct string_builder buf;
  SIGNED char *src;
  ptrdiff_t cnt;
  INT32 d = 1;
  int pads = 3;

  if(args != 1)
    Pike_error( "Wrong number of arguments to MIME.%s()\n",name );
  if (TYPEOF(sp[-1]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.%s()\n",name );
  if (sp[-1].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.%s()\n",name );

  init_string_builder( &buf, 0 );

  for (src = (SIGNED char *)sp[-1].u.string->str, cnt = sp[-1].u.string->len;
       cnt--; src++)
    if(*src>=' ' && tab[*src-' ']>=0) {
      /* 6 more bits to put into d */
      if((d=(d<<6)|tab[*src-' '])>=0x1000000) {
        /* d now contains 24 valid bits.  Put them in the buffer */
        string_builder_putchar( &buf, (d>>16)&0xff );
        string_builder_putchar( &buf, (d>>8)&0xff );
        string_builder_putchar( &buf, d&0xff );
        d=1;
      }
    } else if (*src=='=') {
      /* A pad character has been encountered. */
      break;
    }

  /* If data size not an even multiple of 3 bytes, output remaining data */
  if (d & 0x3f000000) {
    /* NOT_REACHED, but here for symmetry. */
    pads = 0;
  } else if (d & 0xfc0000) {
    pads = 1;
    /* Remove unused bits from d. */
    d >>= 2;
  } else if (d & 0x3f000) {
    pads = 2;
    /* Remove unused bits from d. */
    d >>= 4;
  }
  switch(pads) {
  case 0:
    /* NOT_REACHED, but here for symmetry. */
    string_builder_putchar( &buf, (d>>16)&0xff );
    /* FALLTHRU */
  case 1:
    string_builder_putchar( &buf, (d>>8)&0xff );
    /* FALLTHRU */
  case 2:
    string_builder_putchar( &buf, d&0xff );
  }

  /* Return result */
  pop_n_elems( 1 );
  push_string( finish_string_builder( &buf ) );
}

static void decode_base32( INT32 args, const char *name, const SIGNED char *tab)
{
  /* Decode the string in sp[-1].u.string.  Any whitespace etc must be
     ignored, so the size of the result can't be exactly calculated
     from the input size.  We'll use a string builder instead. */

  struct string_builder buf;
  SIGNED char *src;
  ptrdiff_t cnt;
  unsigned INT64 d = 1;
  int pads = 5;

  if(args != 1)
    Pike_error( "Wrong number of arguments to MIME.%s()\n",name );
  if (TYPEOF(sp[-1]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.%s()\n",name );
  if (sp[-1].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.%s()\n",name );

  init_string_builder( &buf, 0 );

  for (src = (SIGNED char *)sp[-1].u.string->str, cnt = sp[-1].u.string->len;
       cnt--; src++)
    if(*src>=' ' && tab[*src-' ']>=0) {
      /* 5 more bits to put into d */
      if((d=(d<<5)|tab[*src-' '])>=0x10000000000) {
        /* d now contains 40 valid bits.  Put them in the buffer */
        string_builder_putchar( &buf, (d>>32)&0xff );
        string_builder_putchar( &buf, (d>>24)&0xff );
        string_builder_putchar( &buf, (d>>16)&0xff );
        string_builder_putchar( &buf, (d>>8)&0xff );
        string_builder_putchar( &buf, d&0xff );
        d=1;
      }
    } else if (*src=='=') {
      /* A pad character has been encountered. */
      break;
    }

  /* If data size not an even multiple of 5 bytes, output remaining data */
  if (d & 0x1f0000000000) {
    /* 40 bits received. */
    /* NOT_REACHED, but here for symmetry. */
    /* 1  aaaa aaaa  bbbb bbbb  cccc cccc  dddd dddd  eeee eeee */
    pads = 0;
  } else if (d & 0xf800000000) {
    /* 35 bits received. */
    /* 0  0000 1aaa  aaaa abbb  bbbb bccc  cccc cddd  dddd dxxx */
    pads = 1;
    /* Remove unused bits from d. */
    d >>= 3;
  } else if (d & 0xec0000000) {
    /* 30 bits received (irregular). */
    /* 0  0000 0000  01aa aaaa  aabb bbbb  bbcc cccc  ccxx xxxx */
    pads = 2;
    /* Remove unused bits from d. */
    d >>= 6;
  } else if (d & 0x3e000000) {
    /* 25 bits received. */
    /* 0  0000 0000  0000 001a  aaaa aaab  bbbb bbbc  cccc cccx */
    pads = 2;
    /* Remove unused bits from d. */
    d >>= 1;
  } else if (d & 0x1f00000) {
    /* 20 bits received. */
    /* 0  0000 0000  0000 0000  0001 aaaa  aaaa bbbb  bbbb xxxx */
    pads = 3;
    /* Remove unused bits from d. */
    d >>= 4;
  } else if (d & 0xf8000) {
    /* 15 bits received (irregular). */
    /* 0  0000 0000  0000 0000  0000 0000  1aaa aaaa  axxx xxxx */
    pads = 4;
    /* Remove unused bits from d. */
    d >>= 7;
  } else if (d & 0x7c00) {
    /* 10 bits received. */
    /* 0  0000 0000  0000 0000  0000 0000  0000 01aa  aaaa aaxx */
    pads = 4;
    /* Remove unused bits from d. */
    d >>= 2;
  }
  switch(pads) {
  case 0:
    /* NOT_REACHED, but here for symmetry. */
    string_builder_putchar( &buf, (d>>32)&0xff );
    /* FALLTHRU */
  case 1:
    string_builder_putchar( &buf, (d>>24)&0xff );
    /* FALLTHRU */
  case 2:
    string_builder_putchar( &buf, (d>>16)&0xff );
    /* FALLTHRU */
  case 3:
    string_builder_putchar( &buf, (d>>8)&0xff );
    /* FALLTHRU */
  case 4:
    string_builder_putchar( &buf, d&0xff );
  }

  /* Return result */
  pop_n_elems( 1 );
  push_string( finish_string_builder( &buf ) );
}

/** Functions implementing Pike functions **/

/*! @decl string decode_base64(string encoded_data)
 *!
 *! This function decodes data encoded using the @tt{base64@}
 *! transfer encoding.
 *!
 *! @seealso
 *! @[MIME.encode_base64()], @[MIME.decode()]
 */
static void f_decode_base64( INT32 args )
{
  decode_base64(args, "decode_base64", base64rtab);
}

/*! @decl string decode_base64url(string encoded_data)
 *!
 *! Decode strings according to @rfc{4648@} base64url encoding.
 *!
 *! @seealso
 *! @[MIME.decode_base64]
 */
static void f_decode_base64url( INT32 args )
{
  decode_base64(args, "decode_base64url", base64urlrtab);
}

/*! @decl string(8bit) decode_crypt64(string(7bit) encoded_data)
 *!
 *! This function decodes data encoded using the @tt{crypt64@}
 *! encoding.
 *!
 *! This is an ad hoc encoding similar to @tt{base64@} that several
 *! password hashing algorithms use for entries in the password database.
 *!
 *! @note
 *!   This is NOT a MIME-compliant encoding, and MUST NOT
 *!   be used as such.
 *!
 *! @seealso
 *! @[MIME.encode_crypt64()]
 */
static void f_decode_crypt64( INT32 args )
{
  struct string_builder s;
  const unsigned char *in;
  const unsigned char *end;
  unsigned int bitbuf = 0;
  int bits = 0;

  if(args != 1)
    Pike_error( "Wrong number of arguments to MIME.decode_crypt64()\n");
  if (TYPEOF(Pike_sp[-args]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.decode_crypt64()\n" );
  if (Pike_sp[-args].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.decode_crypt64()\n" );

  in = STR0(Pike_sp[-args].u.string);
  end = in + Pike_sp[-args].u.string->len;
  init_string_builder(&s, 0);

  while (in < end) {
    int c = *in;
    in++;
    if ((c < ' ') || ((c = crypt64rtab[c - ' ']) < 0)) continue;
    bitbuf |= c << bits;
    bits += 6;

    if (bits >= 8) {
      string_builder_putchar(&s, bitbuf & 0xff);
      bitbuf >>= 8;
      bits -= 8;
    }
  }

  push_string(finish_string_builder(&s));
}

/*! @decl string decode_base32(string encoded_data)
 *!
 *! This function decodes data encoded using the @tt{base32@}
 *! transfer encoding.
 *!
 *! @seealso
 *! @[MIME.encode_base32()], @[MIME.decode()]
 */
static void f_decode_base32( INT32 args )
{
  decode_base32(args, "decode_base32", base32rtab);
}

/*! @decl string decode_base32hex(string encoded_data)
 *!
 *! Decode strings according to @rfc{4648@} base32hex encoding.
 *!
 *! @seealso
 *! @[MIME.decode_base32]
 */
static void f_decode_base32hex( INT32 args )
{
  decode_base32(args, "decode_base32hex", base32hexrtab);
}

/*  Convenience function for encode_base64();  Encode groups*3 bytes from
 *  *srcp into groups*4 bytes at *destp.
 */
static void do_b64_encode( ptrdiff_t groups, unsigned char **srcp, char **destp,
			   int insert_crlf, const char *tab )
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
    *dest++ = tab[d>>18];
    *dest++ = tab[(d>>12)&63];
    *dest++ = tab[(d>>6)&63];
    *dest++ = tab[d&63];

    /* Insert a linebreak once in a while... */
    if(insert_crlf && ++g == 19) {
      *dest++ = 13;
      *dest++ = 10;
      g = 0;
    }
  }

  /* Update pointers */
  *srcp = src;
  *destp = dest;
}

static void encode_base64( INT32 args, const char *name, const char *tab,
                           int pad )
{
  ptrdiff_t groups;
  ptrdiff_t last;
  int insert_crlf;
  ptrdiff_t length;
  struct pike_string *str;
  unsigned char *src;
  char *dest;

  if(args != 1 && args != 2 && args != 3)
    Pike_error( "Wrong number of arguments to MIME.%s()\n",name );
  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.%s()\n",name );
  if (sp[-args].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.%s()\n",name );
  if (sp[-args].u.string->len == 0)
  {
    pop_n_elems(args-1);
    return;
  }


  /* Encode the string in sp[-args].u.string.  First, we need to know
     the number of 24 bit groups in the input, and the number of bytes
     actually present in the last group. */

  groups = (sp[-args].u.string->len+2)/3;
  last = (sp[-args].u.string->len+2)%3+1;

  insert_crlf = !(args >= 2 && TYPEOF(sp[1-args]) == T_INT &&
		  sp[1-args].u.integer != 0);

  if (args == 3 && (TYPEOF(sp[2-args]) == T_INT) &&
      (sp[2-args].u.integer || !SUBTYPEOF(sp[2-args]))) {
    pad = !sp[2-args].u.integer;
  }

  /* We need 4 bytes for each 24 bit group, and 2 bytes for each linebreak.
   * Note that we do not add any linebreak for the last group.  */
  length = groups*4+(insert_crlf? ((groups - 1)/19)*2 : 0);
  str = begin_shared_string( length );

  src = (unsigned char *)sp[-args].u.string->str;
  dest = str->str;

  if (groups) {
    /* Temporary storage for the last group, as we may have to read an
       extra byte or two and don't want to get any page-faults.  */
    unsigned char tmp[3], *tmpp = tmp;
    int i;

    do_b64_encode( groups-1, &src, &dest, insert_crlf, tab );

    /* Copy the last group to temporary storage */
    tmp[1] = tmp[2] = 0;
    for (i = 0; i < last; i++)
      tmp[i] = *src++;

    /* Encode the last group, and replace output codes with pads as needed */
    do_b64_encode( 1, &tmpp, &dest, 0, tab );
    switch (last) {
    case 1:
      *--dest = '=';
      /* FALLTHRU */
    case 2:
      *--dest = '=';
    }
  }

  /* Return the result */
  pop_n_elems( args );
  if( pad )
    push_string( end_shared_string( str ) );
  else
    push_string( end_and_resize_shared_string( str, length-(3-last) ) );
}

/*  Convenience function for encode_base32();  Encode groups*5 bytes from
 *  *srcp into groups*8 bytes at *destp.
 */
static void do_b32_encode( ptrdiff_t groups, unsigned char **srcp, char **destp,
			   int insert_crlf, const char *tab )
{
  unsigned char *src = *srcp;
  char *dest = *destp;
  int g = 0;

  while (groups--) {
    /* Get 24 bits from src */
    INT64 d = *src++<<8;
    d = (*src++|d)<<8;
    d = (*src++|d)<<8;
    d = (*src++|d)<<8;
    d |= *src++;

    /* Output in encoded from to dest */
    *dest++ = tab[d>>35];
    *dest++ = tab[(d>>30)&31];
    *dest++ = tab[(d>>25)&31];
    *dest++ = tab[(d>>20)&31];
    *dest++ = tab[(d>>15)&31];
    *dest++ = tab[(d>>10)&31];
    *dest++ = tab[(d>>5)&31];
    *dest++ = tab[d&31];

    /* Insert a linebreak once in a while... */
    if(insert_crlf && ++g == 8) {
      *dest++ = 13;
      *dest++ = 10;
      g = 0;
    }
  }

  /* Update pointers */
  *srcp = src;
  *destp = dest;
}

static void encode_base32( INT32 args, const char *name, const char *tab,
                           int pad )
{
  ptrdiff_t groups;
  ptrdiff_t last;
  int insert_crlf;
  ptrdiff_t length;
  struct pike_string *str;
  unsigned char *src;
  char *dest;

  if(args != 1 && args != 2 && args != 3)
    Pike_error( "Wrong number of arguments to MIME.%s()\n",name );
  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.%s()\n",name );
  if (sp[-args].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.%s()\n",name );
  if (sp[-args].u.string->len == 0)
  {
    pop_n_elems(args-1);
    return;
  }


  /* Encode the string in sp[-args].u.string.  First, we need to know
     the number of 40 bit groups in the input, and the number of bytes
     actually present in the last group. */

  groups = (sp[-args].u.string->len+4)/5;
  last = (sp[-args].u.string->len-1)%5+1;

  insert_crlf = !(args >= 2 && TYPEOF(sp[1-args]) == T_INT &&
		  sp[1-args].u.integer != 0);

  if (args == 3 && (TYPEOF(sp[2-args]) == T_INT) &&
      (sp[2-args].u.integer || !SUBTYPEOF(sp[2-args]))) {
    pad = !sp[2-args].u.integer;
  }

  /* We need 8 bytes for each 40 bit group, and 2 bytes for each linebreak */
  length = groups*8+(insert_crlf? ((groups - 1)/8)*2 : 0);
  str = begin_shared_string( length );

  src = (unsigned char *)sp[-args].u.string->str;
  dest = str->str;

  if (groups) {
    /* Temporary storage for the last group, as we may have to read an
       extra byte or two and don't want to get any page-faults.  */
    unsigned char tmp[5], *tmpp = tmp;
    int i;

    do_b32_encode( groups-1, &src, &dest, insert_crlf, tab );

    /* Copy the last group to temporary storage */
    tmp[1] = tmp[2] = tmp[3] = tmp[4] = 0;
    for (i = 0; i < last; i++)
      tmp[i] = *src++;

    /* Encode the last group, and replace output codes with pads as needed */
    do_b32_encode( 1, &tmpp, &dest, 0, tab );
    switch (last) {
    case 1:
      *--dest = '=';
      *--dest = '=';
      /* FALLTHRU */
    case 2:
      *--dest = '=';
      /* FALLTHRU */
    case 3:
      *--dest = '=';
      *--dest = '=';
      /* FALLTHRU */
    case 4:
      *--dest = '=';
    }
  }

  /* Return the result */
  pop_n_elems( args );
  if( pad )
    push_string( end_shared_string( str ) );
  else {
    last += last>>1;
    push_string( end_and_resize_shared_string( str, length-(7-last) ) );
  }
}

/*! @decl string encode_base64(string data, void|int no_linebreaks, @
 *!                            void|int no_pad)
 *!
 *! This function encodes data using the @tt{base64@} transfer encoding.
 *!
 *! If a nonzero value is passed as @[no_linebreaks], the result string
 *! will not contain any linebreaks.
 *!
 *! @seealso
 *! @[MIME.decode_base64()], @[MIME.encode()]
 */
static void f_encode_base64( INT32 args )
{
  encode_base64(args, "encode_base64", base64tab, 1);
}

/*! @decl string encode_base64url(string data, void|int no_linebreaks, @
 *!                               void|int no_pad)
 *!
 *! Encode strings according to @rfc{4648@} base64url encoding. No
 *! padding is performed and no_linebreaks defaults to true.
 *!
 *! @seealso
 *! @[MIME.encode_base64]
 */
static void f_encode_base64url( INT32 args )
{
  if( args==1 )
  {
    push_int(1);
    args++;
  }
  encode_base64(args, "encode_base64url", base64urltab, 0);
}

/*! @decl string encode_base32(string data, void|int no_linebreaks, @
 *!                            void|int no_pad)
 *!
 *! Encode strings according to @rfc{4648@} base32 encoding.
 *!
 *! If a nonzero value is passed as @[no_linebreaks], the result string
 *! will not contain any linebreaks.
 *!
 *! @seealso
 *! @[MIME.decode_base32()], @[MIME.encode()]
 */
static void f_encode_base32( INT32 args )
{
  encode_base32(args, "encode_base32", base32tab, 1);
}

/*! @decl string encode_base32hex(string data, void|int no_linebreaks, @
 *!                               void|int no_pad)
 *!
 *! Encode strings according to @rfc{4648@} base32hex encoding.
 *!
 *! If a nonzero value is passed as @[no_linebreaks], the result string
 *! will not contain any linebreaks.
 *!
 *! @seealso
 *! @[MIME.encode_base32hex]
 */
static void f_encode_base32hex( INT32 args )
{
  encode_base32(args, "encode_base32hex", base32hextab, 1);
}

/*! @decl string encode_crypt64(string(8bit) data)
 *!
 *! This function encodes data using the @tt{crypt64@} encoding.
 *!
 *! This is an ad hoc encoding similar to @tt{base64@} that several
 *! password hashing algorithms use for entries in the password database.
 *!
 *! @note
 *!   This is NOT a MIME-compliant encoding, and MUST NOT
 *!   be used as such.
 *!
 *! @seealso
 *! @[MIME.decode_crypt64()]
 */
static void f_encode_crypt64( INT32 args )
{
  const p_wchar0 *in_bytes;
  const p_wchar0 *end_bytes;
  p_wchar0 *out_bytes;
  struct pike_string *res;
  unsigned int bitbuf = 0;
  int bits = 0;

  if(args != 1)
    Pike_error( "Wrong number of arguments to MIME.encode_crypt64()\n" );
  if(TYPEOF(sp[-args]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.encode_crypt64()\n" );
  if (sp[-args].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.encode_crypt64()\n" );

  in_bytes = STR0(Pike_sp[-args].u.string);
  end_bytes = in_bytes + Pike_sp[-args].u.string->len;

  res = begin_shared_string(Pike_sp[-args].u.string->len +
			    (Pike_sp[-args].u.string->len + 2) / 3);
  out_bytes = STR0(res);

  while (in_bytes < end_bytes) {
    if (bits < 6) {
      bitbuf |= *in_bytes << bits;
      in_bytes++;
      bits += 8;
    }
    *out_bytes = crypt64tab[bitbuf & 0x3f];
    out_bytes++;
    bitbuf >>= 6;
    bits -= 6;
  }

  if (bits) {
    *out_bytes = crypt64tab[bitbuf & 0x3f];
  }

  push_string(end_shared_string(res));
}

/*! @decl string decode_qp(string encoded_data)
 *!
 *! This function decodes data encoded using the @tt{quoted-printable@}
 *! (a.k.a. quoted-unreadable) transfer encoding.
 *!
 *! @seealso
 *! @[MIME.encode_qp()], @[MIME.decode()]
 */
static void f_decode_qp( INT32 args )
{
  if(args != 1)
    Pike_error( "Wrong number of arguments to MIME.decode_qp()\n" );
  else if(TYPEOF(sp[-1]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.decode_qp()\n" );
  else if (sp[-1].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.decode_qp()\n" );
  else {

    /* Decode the string in sp[-1].u.string.  We have absolutely no idea
       how much of the input is raw data and how much is encoded data,
       so we'll use a string builder to hold the result. */

    struct string_builder buf;
    SIGNED char *src;
    ptrdiff_t cnt;

    init_string_builder(&buf, 0);

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
	  string_builder_putchar( &buf, (qprtab[src[1]-'0']<<4)|qprtab[src[2]-'0'] );
	  cnt -= 2;
	  src += 2;
	}
      } else
	/* Raw data */
	string_builder_putchar( &buf, *(unsigned char *)src );

    /* Return the result */
    pop_n_elems( 1 );
    push_string( finish_string_builder( &buf ) );
  }
}

/*! @decl string encode_qp(string data, void|int no_linebreaks)
 *!
 *! This function encodes data using the @tt{quoted-printable@}
 *! (a.k.a. quoted-unreadable) transfer encoding.
 *!
 *! If a nonzero value is passed as @[no_linebreaks], the result
 *! string will not contain any linebreaks.
 *!
 *! @note
 *! Please do not use this function.  QP is evil, and there's no
 *! excuse for using it.
 *!
 *! @seealso
 *! @[MIME.decode_qp()], @[MIME.encode()]
 */
static void f_encode_qp( INT32 args )
{
  if (args != 1 && args != 2)
    Pike_error( "Wrong number of arguments to MIME.encode_qp()\n" );
  else if (TYPEOF(sp[-args]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.encode_qp()\n" );
  else if (sp[-args].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.encode_qp()\n" );
  else {

    /* Encode the string in sp[-args].u.string.  We don't know how
       much of the data has to be encoded, so let's use that trusty
       string builder once again. */

    struct string_builder buf;
    unsigned char *src = (unsigned char *)sp[-args].u.string->str;
    ptrdiff_t cnt;
    int col = 0;
    int insert_crlf = !(args == 2 && TYPEOF(sp[-1]) == T_INT &&
			sp[-1].u.integer != 0);

    init_string_builder( &buf, 0 );

    for (cnt = sp[-args].u.string->len; cnt--; src++) {
      if ((*src >= 33 && *src <= 60) ||
	  (*src >= 62 && *src <= 126))
	/* These characters can always be encoded as themselves */
	string_builder_putchar( &buf, *(unsigned char *)src );
      else {
	/* Better safe than sorry, eh?  Use the dreaded hex escape */
	string_builder_putchar( &buf, '=' );
	string_builder_putchar( &buf, qptab[(*src)>>4] );
	string_builder_putchar( &buf, qptab[(*src)&15] );
	col += 2;
      }
      /* We'd better not let the lines get too long */
      if (++col >= 73 && insert_crlf) {
	string_builder_putchar( &buf, '=' );
	string_builder_putchar( &buf, 13 );
	string_builder_putchar( &buf, 10 );
	col = 0;
      }
    }

    /* Return the result */
    pop_n_elems( args );
    push_string( finish_string_builder( &buf ) );
  }
}

/* MIME.decode_uue() */

/*! @decl string decode_uue(string encoded_data)
 *!
 *! This function decodes data encoded using the @tt{x-uue@} transfer encoding.
 *! It can also be used to decode generic UUEncoded files.
 *!
 *! @seealso
 *! @[MIME.encode_uue()], @[MIME.decode()]
 */
static void f_decode_uue( INT32 args )
{
  if (args != 1)
    Pike_error( "Wrong number of arguments to MIME.decode_uue()\n" );
  else if(TYPEOF(sp[-1]) != T_STRING)
    Pike_error( "Wrong type of argument to MIME.decode_uue()\n" );
  else if (sp[-1].u.string->size_shift != 0)
    Pike_error( "Char out of range for MIME.decode_uue()\n" );
  else {

    /* Decode string in sp[-1].u.string.  This is done much like in
       the base64 case, but we'll look for the "begin" line first.  */

    struct string_builder buf;
    char *src;
    ptrdiff_t cnt;

    init_string_builder( &buf, 0 );

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
	string_builder_putchar( &buf, (d>>16)&0xff );
	string_builder_putchar( &buf, (d>>8)&0xff );
	string_builder_putchar( &buf, d&0xff );
      }

      /* If the line didn't contain an even multiple of 24 bits, remove
	 spurious bytes from the buffer */

      /*  while (l++)
	    string_builder_allocate( &buf, -1 ); */
      /* Hmm...  string_builder_allocate is static.  Cheat a bit... */
      if (l<0)
	buf.s->len += l;

      /* Skip to EOL */
      while (cnt-- && *src++!=10);
    }

    /* Return the result */
    pop_n_elems( 1 );
    push_string( finish_string_builder( &buf ) );
  }
}

/*  Convenience function for encode_uue();  Encode groups*3 bytes from
 *  *srcp into groups*4 bytes at *destp, and reserve space for last more.
 */
static void do_uue_encode(ptrdiff_t groups, unsigned char **srcp, char **destp,
			  ptrdiff_t last )
{
  unsigned char *src = *srcp;
  char *dest = *destp;

  while (groups || last) {
    /* A single line can hold at most 15 groups */
    ptrdiff_t g = (groups >= 15? 15 : groups);

    if (g<15) {
      /* The line isn't filled completely.  Add space for the "last" bytes */
      *dest++ = ' ' + (char)(3*g + last);
      last = 0;
    } else
      *dest++ = ' ' + (char)(3*g);

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

/*! @decl string(7bit) encode_uue(string(8bit) encoded_data, @
 *!                               void|string(7bit) filename)
 *!
 *! This function encodes data using the @tt{x-uue@} transfer encoding.
 *!
 *! The optional argument @[filename] specifies an advisory filename to include
 *! in the encoded data, for extraction purposes.
 *!
 *! This function can also be used to produce generic UUEncoded files.
 *!
 *! @seealso
 *! @[MIME.decode_uue()], @[MIME.encode()]
 */
static void f_encode_uue( INT32 args )
{
  if (args != 1 && args != 2)
    Pike_error( "Wrong number of arguments to MIME.encode_uue()\n" );
  else if (TYPEOF(sp[-args]) != T_STRING ||
	   (args == 2 && TYPEOF(sp[-1]) != T_VOID &&
	    TYPEOF(sp[-1]) != T_STRING && TYPEOF(sp[-1]) != T_INT))
    Pike_error( "Wrong type of argument to MIME.encode_uue()\n" );
  else if (sp[-args].u.string->size_shift != 0 ||
	   (args == 2 && TYPEOF(sp[-1]) == T_STRING &&
	    sp[-1].u.string->size_shift != 0))
    Pike_error( "Char out of range for MIME.encode_uue()\n" );
  else {

    /* Encode string in sp[-args].u.string.  If args == 2, there may be
       a filename in sp[-1].u.string.  If we don't get a filename, use
       the generic filename "attachment"... */

    char *dest, *filename = "attachment";
    struct pike_string *str;
    unsigned char *src = (unsigned char *) sp[-args].u.string->str;
    /* Calculate number of 24 bit groups, and actual # of bytes in last grp */
    ptrdiff_t groups = (sp[-args].u.string->len + 2)/3;
    ptrdiff_t last= (sp[-args].u.string->len - 1)%3 + 1;

    /* Get the filename if provided */
    if (args == 2 && TYPEOF(sp[-1]) == T_STRING)
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

      /* Replace final nulls with pad characters if necessary */
      switch (last) {
      case 1:
	dest[-2] = '`';
	/* FALLTHRU */
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


static void low_tokenize( INT32 args, int mode )
{

  /* Tokenize string in sp[-args].u.string.  We'll just push the
     tokens on the stack, and then do an aggregate_array just
     before exiting. */

  unsigned char *src;
  int flags = 0;
  struct array *arr;
  struct pike_string *str;
  ptrdiff_t cnt;
  INT32 n = 0, l, e, d;
  char *p;

  get_all_args(NULL, args, "%n.%d", &str, &flags);

  src = STR0(str);
  cnt = str->len;

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
	  push_static_text("encoded-word");
	push_string( make_shared_binary_string( (char *)src, l ) );
	if(mode)
	  f_aggregate(2);
	n++;
	src += l;
	cnt -= l;
	break;
      }
      /* FALLTHRU */
    case CT_SPECIAL:
    case CT_RBRACK:
    case CT_RPAR:
      /* Individual special character, push as a char (= int) */
      if(mode)
	push_static_text("special");
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
	push_static_text("word");
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
	  if ((src[l] == '\\') && !(flags & TOKENIZE_KEEP_ESCAPES)) {
	    e++;
	    l++;
	  }

      /* Push the resulting string */
      if(mode)
	push_static_text("word");

      if (e) {
	/* l is the distance to the ending ", and e is the number of	\
	   escapes encountered on the way */
	str = begin_shared_string( l-e-1 );

	/* Copy the string and remove \ escapes */
	for (p = str->str, e = 1; e < l; e++)
	  *p++ = (src[e] == '\\'? src[++e] : src[e]);

	push_string( end_shared_string( str ) );
      } else {
	/* No escapes. */
	push_string(make_shared_binary_string( (char *)src+1, l-1));
      }
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
	  push_static_text("special");
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
	push_static_text("domain-literal");
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
	push_static_text("comment");

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
      if(*src == '\0') {
	/* Multiple occurance header.  Ignore all but first. */
	cnt = 0;
	break;
      }
      Pike_error( "Invalid character in header field\n" );
    }

  /* Create the resulting array and push it */
  arr = aggregate_array( n );
  pop_n_elems( args );
  push_array( arr );
}

/*! @decl array(string|int) tokenize(string header, int|void flags)
 *!
 *! A structured header field, as specified by @rfc{822@}, is constructed from
 *! a sequence of lexical elements.
 *!
 *! @param header
 *!   The header value to parse.
 *!
 *! @param flags
 *!   An optional set of flags. Currently only one flag is defined:
 *!   @int
 *!     @value TOKENIZE_KEEP_ESCAPES
 *!       Keep backslash-escapes in quoted-strings.
 *!   @endint
 *!
 *! The lexical elements parsed are:
 *! @dl
 *!   @item
 *!     individual special characters
 *!   @item
 *!     quoted-strings
 *!   @item
 *!     domain-literals
 *!   @item
 *!     comments
 *!   @item
 *!     atoms
 *! @enddl
 *!
 *! This function will analyze a string containing the header value,
 *! and produce an array containing the lexical elements.
 *!
 *! Individual special characters will be returned as characters (i.e.
 *! @expr{int@}s).
 *!
 *! Quoted-strings, domain-literals and atoms will be decoded and returned
 *! as strings.
 *!
 *! Comments are not returned in the array at all.
 *!
 *! @note
 *! As domain-literals are returned as strings, there is no way to tell the
 *! domain-literal @tt{[127.0.0.1]@} from the quoted-string
 *! @tt{"[127.0.0.1]"@}. Hopefully this won't cause any problems.
 *! Domain-literals are used seldom, if at all, anyway...
 *!
 *! The set of special-characters is the one specified in @rfc{1521@}
 *! (i.e. @expr{"<", ">", "@@", ",", ";", ":", "\", "/", "?", "="@}),
 *! and not the set specified in @rfc{822@}.
 *!
 *! @seealso
 *!   @[MIME.quote()], @[tokenize_labled()],
 *!   @[decode_words_tokenized_remapped()].
 */
static void f_tokenize( INT32 args )
{
  low_tokenize(args, 0);
}

/*! @decl array(array(string|int)) tokenize_labled(string header, @
 *!						   int|void flags)
 *!
 *! Similar to @[tokenize()], but labels the contents, by making
 *! arrays with two elements; the first a label, and the second
 *! the value that @[tokenize()] would have put there, except
 *! for that comments are kept.
 *!
 *! @param header
 *!   The header value to parse.
 *!
 *! @param flags
 *!   An optional set of flags. Currently only one flag is defined:
 *!   @int
 *!     @value TOKENIZE_KEEP_ESCAPES
 *!       Keep backslash-escapes in quoted-strings.
 *!   @endint
 *!
 *! The following labels exist:
 *! @string
 *!   @value "encoded-word"
 *!     Word encoded according to =?...
 *!   @value "special"
 *!     Special character.
 *!   @value "word"
 *!     Word.
 *!   @value "domain-literal"
 *!     Domain literal.
 *!   @value "comment"
 *!     Comment.
 *! @endstring
 *!
 *! @seealso
 *!   @[MIME.quote()], @[tokenize()],
 *!   @[decode_words_tokenized_labled_remapped()]
 */
static void f_tokenize_labled( INT32 args )
{
  low_tokenize(args, 1);
}


/*  Convenience function for quote() which determines if a sequence of
 *  characters can be stored as an atom.
 */
static int check_atom_chars( unsigned char *str, ptrdiff_t len )
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
static int check_encword( unsigned char *str, ptrdiff_t len )
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

/*! @decl string quote(array(string|int) lexical_elements);
 *!
 *! This function is the inverse of the @[MIME.tokenize] function.
 *!
 *! A header field value is constructed from a sequence of lexical elements.
 *! Characters (@expr{int@}s) are taken to be special-characters, whereas
 *! strings are encoded as atoms or quoted-strings, depending on whether
 *! they contain any special characters.
 *!
 *! @note
 *!   There is no way to construct a domain-literal using this function.
 *!   Neither can it be used to produce comments.
 *!
 *! @seealso
 *!   @[MIME.tokenize()]
 */
static void f_quote( INT32 args )
{
  struct svalue *item;
  INT32 cnt;
  struct string_builder buf;
  int prev_atom = 0;

  if (args != 1)
    Pike_error( "Wrong number of arguments to MIME.quote()\n" );
  else if (TYPEOF(sp[-1]) != T_ARRAY)
    Pike_error( "Wrong type of argument to MIME.quote()\n" );

  /* Quote array in sp[-1].u.array.  Once again we'll rely on a
     string_builder to collect the output string. */

  init_string_builder( &buf, 0 );

  for (cnt=sp[-1].u.array->size, item=sp[-1].u.array->item; cnt--; item++) {

    if (TYPEOF(*item) == T_INT) {

      /* Single special character */
      string_builder_putchar( &buf, item->u.integer );
      prev_atom = 0;

    } else if (TYPEOF(*item) != T_STRING) {

      /* Neither int or string.  Too bad... */
      free_string_builder( &buf );
      Pike_error( "Wrong type of argument to MIME.quote()\n" );

    } else if (item->u.string->size_shift != 0) {

      free_string_builder( &buf );
      Pike_error( "Char out of range for MIME.quote()\n" );

    } else {

      /* It's a string, so we'll store it either as an atom, or
	 as a quoted-string */
      struct pike_string *str = item->u.string;

      /* In case the previous item was also a string, we'll add a single
	 whitespace as a delimiter */
      if (prev_atom)
	string_builder_putchar( &buf, ' ' );

      if ((str->len>5 && str->str[0]=='=' && str->str[1]=='?' &&
	   check_encword((unsigned char *)str->str, str->len)) ||
	  check_atom_chars((unsigned char *)str->str, str->len)) {

	/* Valid atom without quotes... */
	string_builder_binary_strcat( &buf, str->str, str->len );

      } else {

	/* Have to use quoted-string */
	ptrdiff_t len = str->len;
	char *src = str->str;
	string_builder_putchar( &buf, '"' );
	while(len--) {
	  if(*src=='"' || *src=='\\' || *src=='\r')
	    /* Some characters have to be escaped even within quotes... */
	    string_builder_putchar( &buf, '\\' );
	  string_builder_putchar( &buf, (*src++)&0xff );
	}
	string_builder_putchar( &buf, '"' );

      }

      prev_atom = 1;

    }
  }

  /* Return the result */
  pop_n_elems( 1 );
  push_string( finish_string_builder( &buf ) );
}

/*! @decl string quote_labled(array(array(string|int)) tokens)
 *!
 *! This function performs the reverse operation of @[tokenize_labled()].
 *!
 *! @seealso
 *!   @[MIME.quote()], @[MIME.tokenize_labled()]
 */
static void f_quote_labled( INT32 args )
{
  struct svalue *item;
  INT32 cnt;
  struct string_builder buf;
  int prev_atom = 0;

  if (args != 1)
    Pike_error( "Wrong number of arguments to MIME.quote_labled()\n" );
  else if (TYPEOF(sp[-1]) != T_ARRAY)
    Pike_error( "Wrong type of argument to MIME.quote_labled()\n" );

  /* Quote array in sp[-1].u.array.  Once again we'll rely on a
     string_builder to collect the output string. */

  init_string_builder( &buf, 0 );

  for (cnt=sp[-1].u.array->size, item=sp[-1].u.array->item; cnt--; item++) {

    if (TYPEOF(*item) != T_ARRAY || item->u.array->size<2 ||
	TYPEOF(item->u.array->item[0]) != T_STRING) {
      free_string_builder( &buf );
      Pike_error( "Wrong type of argument to MIME.quote_labled()\n" );
    }

    if (c_compare_string( item->u.array->item[0].u.string, "special", 7 )) {

      if(TYPEOF(item->u.array->item[1]) != T_INT) {
	free_string_builder( &buf );
	Pike_error( "Wrong type of argument to MIME.quote_labled()\n" );
      }

      /* Single special character */
      string_builder_putchar( &buf, item->u.array->item[1].u.integer );
      prev_atom = 0;

    } else if(TYPEOF(item->u.array->item[1]) != T_STRING) {

      /* All the remaining lexical items require item[1] to be a string */
      free_string_builder( &buf );
      Pike_error( "Wrong type of argument to MIME.quote_labled()\n" );

    } else if (item->u.array->item[1].u.string->size_shift != 0) {

      free_string_builder( &buf );
      Pike_error( "Char out of range for MIME.quote_labled()\n" );

    } else if (c_compare_string( item->u.array->item[0].u.string, "word", 4 )){

      /* It's a word, so we'll store it either as an atom, or
	 as a quoted-string */
      struct pike_string *str = item->u.array->item[1].u.string;

      /* In case the previous item was also a string, we'll add a single
	 whitespace as a delimiter */
      if (prev_atom)
	string_builder_putchar( &buf, ' ' );

      if ((str->len>5 && str->str[0]=='=' && str->str[1]=='?' &&
	   check_encword((unsigned char *)str->str, str->len)) ||
	  check_atom_chars((unsigned char *)str->str, str->len)) {

	/* Valid atom without quotes... */
	string_builder_binary_strcat( &buf, str->str, str->len );

      } else {

	/* Have to use quoted-string */
	ptrdiff_t len = str->len;
	char *src = str->str;
	string_builder_putchar( &buf, '"' );
	while(len--) {
	  if(*src=='"' || *src=='\\' || *src=='\r')
	    /* Some characters have to be escaped even within quotes... */
	    string_builder_putchar( &buf, '\\' );
	  string_builder_putchar( &buf, (*src++)&0xff );
	}
	string_builder_putchar( &buf, '"' );

      }

      prev_atom = 1;

    } else if (c_compare_string( item->u.array->item[0].u.string,
				 "encoded-word", 12 )) {

      struct pike_string *str = item->u.array->item[1].u.string;

      /* Insert 'as is'. */
      string_builder_binary_strcat( &buf, str->str, str->len );

      prev_atom = 1;

    } else if (c_compare_string( item->u.array->item[0].u.string,
				 "comment", 7 )) {

      struct pike_string *str = item->u.array->item[1].u.string;

      /* Encode comment */
      ptrdiff_t len = str->len;
      char *src = str->str;
      string_builder_putchar( &buf, '(' );
      while(len--) {
	if(*src=='(' || *src==')' || *src=='\\' || *src=='\r')
	  /* Some characters have to be escaped even within comments... */
	  string_builder_putchar( &buf, '\\' );
	string_builder_putchar( &buf, (*src++)&0xff );
      }
      string_builder_putchar( &buf, ')' );

      prev_atom = 0;

    } else if (c_compare_string( item->u.array->item[0].u.string,
				 "domain-literal", 14 )) {

      struct pike_string *str = item->u.array->item[1].u.string;

      /* Encode domain-literal */
      ptrdiff_t len = str->len;
      char *src = str->str;

      if (len<2 || src[0] != '[' || src[len-1] != ']') {
	free_string_builder( &buf );
	Pike_error( "Illegal domain-literal passed to MIME.quote_labled()\n" );
      }

      len -= 2;
      src++;

      string_builder_putchar( &buf, '[' );
      while(len--) {
	if(*src=='[' || *src==']' || *src=='\\' || *src=='\r')
	  /* Some characters have to be escaped within domain-literals... */
	  string_builder_putchar( &buf, '\\' );
	string_builder_putchar( &buf, (*src++)&0xff );
      }
      string_builder_putchar( &buf, ']' );

      prev_atom = 0;

    } else {

      /* Unknown label.  Too bad... */
      free_string_builder( &buf );
      Pike_error( "Unknown label passed to MIME.quote_labled()\n" );

    }

  }

  /* Return the result */
  pop_n_elems( 1 );
  push_string( finish_string_builder( &buf ) );
}

/*! @endmodule
 */
