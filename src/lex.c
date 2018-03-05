/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "las.h"
#include "lex.h"
#include "bignum.h"
#include "pike_compiler.h"
#include "interpret.h"

#include <ctype.h>


static double my_strtod(const char *nptr, char **endptr)
{
  double tmp=strtod(nptr,endptr);
  if(*endptr>nptr)
  {
    if(endptr[0][-1]=='.')
      endptr[0]--;
  }
  return tmp;
}

#define LEXDEBUG 0

/* Make lexers for shifts 0, 1 and 2. */

#define SHIFT	0
#include "lexer.h"
#undef SHIFT
#define SHIFT	1
#include "lexer.h"
#undef SHIFT
#define SHIFT	2
#include "lexer.h"
#undef SHIFT

int parse_esc_seq_pcharp (PCHARP buf, p_wchar2 *chr, ptrdiff_t *len)
{
  if(LIKELY(buf.shift == 0))
    return parse_esc_seq0((void*)buf.ptr,chr,len);
  if( buf.shift == 1 )
    return parse_esc_seq1((void*)buf.ptr,chr,len);
  return parse_esc_seq2((void*)buf.ptr,chr,len);
  /* UNREACHABLE */
}

int yylex(YYSTYPE *yylval)
{
  struct lex *lex;
  CHECK_COMPILER();
  lex = &THIS_COMPILATION->lex;
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX: Calling lexer at 0x%08lx\n",
	  (long)lex->current_lexer);
#endif /* LEXDEBUG > 8 */
  return(lex->current_lexer(lex, yylval));
}
