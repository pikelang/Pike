/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lex.c,v 1.120 2004/11/01 01:33:30 mast Exp $
*/

#include "global.h"
#include "lex.h"
#include "stuff.h"
#include "bignum.h"

#include <ctype.h>

#define LEXDEBUG 0

struct lex lex;

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
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX: Calling lexer at 0x%08lx\n",
	  (long)lex.current_lexer);
#endif /* LEXDEBUG > 8 */
  return(lex.current_lexer(yylval));
}
