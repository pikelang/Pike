/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#ifndef GLOBAL_H
#define GLOBAL_H

#define POSIX_SOURCE

/*
 * Some structure forward declarations are needed.
 */

/* This is needed for linux */
#ifdef MALLOC_REPLACED
#define NO_FIX_MALLOC
#endif

struct program;
struct function;
struct svalue;
struct sockaddr;
struct object;
struct array;
struct svalue;

#include "machine.h"
#include "config.h"

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
# ifdef alloca
#  undef alloca
# endif
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef DEBUG
#include "debug.h"
#endif

#if defined(__GNUC__) && !defined(DEBUG) && !defined(lint)
#define INLINE inline
#else
#define INLINE
#endif

#include "port.h"


#ifdef BUFSIZ
#define PROT_STDIO(x) PROT(x)
#else
#define PROT_STDIO(x) ()
#endif

#ifdef __STDC__
#define PROT(x) x
#else
#define PROT(x) ()
#endif

#ifdef MALLOC_DECL_MISSING
char *malloc PROT((int));
char *realloc PROT((char *,int));
void free PROT((char *));
char *calloc PROT((int,int));
#endif

#ifdef GETPEERNAME_DECL_MISSING
int getpeername PROT((int, struct sockaddr *, int *));
#endif

#ifdef GETHOSTNAME_DECL_MISSING
void gethostname PROT((char *,int));
#endif

#ifdef POPEN_DECL_MISSING
FILE *popen PROT((char *,char *));
#endif

#ifdef GETENV_DECL_MISSING
char *getenv PROT((char *));
#endif

#endif
