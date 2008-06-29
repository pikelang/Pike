/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lex.h,v 1.38 2008/06/29 21:14:00 marcus Exp $
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
  int (*current_lexer)(struct lex *, YYSTYPE *);
};

/* Prototypes begin here */

int parse_esc_seq0 (p_wchar0 *buf, p_wchar2 *chr, ptrdiff_t *len);
int parse_esc_seq1 (p_wchar1 *buf, p_wchar2 *chr, ptrdiff_t *len);
int parse_esc_seq2 (p_wchar2 *buf, p_wchar2 *chr, ptrdiff_t *len);

int yylex0(struct lex *, YYSTYPE *);
int yylex1(struct lex *, YYSTYPE *);
int yylex2(struct lex *, YYSTYPE *);

/* Prototypes end here */

#endif	/* !LEX_H */
