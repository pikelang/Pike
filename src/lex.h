/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lex.h,v 1.33 2004/03/23 15:14:00 mast Exp $
*/

#ifndef LEX_H
#define LEX_H

#include "program.h"

#ifndef INCLUDED_FROM_LANGUAGE_YACC
/* language.c duplicates the definitions in language.h. */
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
