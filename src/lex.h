/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lex.h,v 1.34 2004/03/24 16:42:09 grubba Exp $
*/

#ifndef LEX_H
#define LEX_H

#include "program.h"

#if !defined(INCLUDED_FROM_LANGUAGE_YACC) && !defined(TOK_ARROW)
/* language.c duplicates the definitions in language.h.
 * language.h is usually not protected against multiple inclusion.
 */
#include "language.h"
#endif

#define NEW_LEX

struct lex
{
  char *pos;
  char *end;
  INT32 current_line;
  INT32 pragmas;
  struct pike_string *current_file;
  int (*current_lexer)(YYSTYPE *);
};

extern struct lex lex;

/* Prototypes begin here */

int yylex0(YYSTYPE *);
int yylex1(YYSTYPE *);
int yylex2(YYSTYPE *);

/* Prototypes end here */

#endif	/* !LEX_H */
