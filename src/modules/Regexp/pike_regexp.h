/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
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
  int regmlen;			/* Internal use only. */
  char program[1];		/* Unwarranted chumminess with compiler. */
} regexp;



extern regexp *regcomp();
extern int regexec();
extern char *regsub();
extern void regerror();
#endif
