/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lex.c,v 1.119 2004/09/18 20:50:51 nilsson Exp $
*/

#include "global.h"
#include "lex.h"
#include "stuff.h"

#include <ctype.h>

#define LEXDEBUG 0

struct lex lex;

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
