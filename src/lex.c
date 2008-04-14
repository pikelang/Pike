/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lex.c,v 1.121 2008/04/14 10:14:40 grubba Exp $
*/

#include "global.h"
#include "lex.h"
#include "stuff.h"
#include "bignum.h"
#include "pike_compiler.h"
#include "interpret.h"

#include <ctype.h>

#define LEXDEBUG 0

/* Must do like this since at least gcc is a little too keen on
 * optimizing INT_TYPE_MUL_OVERFLOW otherwise. */
static unsigned INT32 eight = 8, sixteen = 16, ten = 10;

/* Make lexers for shifts 0, 1 and 2. */

#define SHIFT	0
#include "lexer0.h"
#undef SHIFT
#define SHIFT	1
#include "lexer1.h"
#undef SHIFT
#define SHIFT	2
#include "lexer2.h"
#undef SHIFT

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
