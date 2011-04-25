/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef REGEXP_H
#define REGEXP_H

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */

#define NSUBEXP  40
typedef struct regexp
{
  char *startp[NSUBEXP];
  char *endp[NSUBEXP];
  char regstart;		/* Internal use only. */
  char reganch;			/* Internal use only. */
  char *regmust;		/* Internal use only. */
  size_t regmlen;		/* Internal use only. */
  char program[1];		/* Unwarranted chumminess with compiler. */
} regexp;



/* Prototypes begin here */
regexp *pike_regcomp(char *exp,int excompat);
int pike_regexec(regexp *prog, char *string);
char *pike_regsub(regexp *prog, char *source, char *dest, int n);
/* Prototypes end here */

#endif
