/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#ifndef REGEXP_H
#define REGEXP_H
/*
 * $Id: pike_regexp.h,v 1.4 2000/08/11 13:04:32 grubba Exp $
 *
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
