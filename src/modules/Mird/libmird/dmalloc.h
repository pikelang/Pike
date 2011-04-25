/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
** libMird by Mirar <mirar@mirar.org>
** please submit bug reports and patches to the author
**
** also see http://www.mirar.org/mird/
*/

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef MEM_DEBUG

void *_smalloc(unsigned long size,char *file,int line);
void  _sfree(void *m,char *file,int line);
void *_srealloc(void *m,unsigned long newsize,char *file,int line);
char *_sstrdup(char *s,char *file,int line);

#define smalloc(sz) _smalloc(sz,__FILE__,__LINE__)
#define sfree(m) _sfree(m,__FILE__,__LINE__)
#define srealloc(m,sz) _srealloc(m,sz,__FILE__,__LINE__)
#define sstrdup(s) _sstrdup(s,__FILE__,__LINE__)

#ifndef IS_MEM_C

#ifdef malloc
#undef malloc
#endif /* malloc */

#define free error error
#define malloc error error
#define realloc error error

#endif /* IS_MEM_C */

#else /* !MEM_DEBUG */

#define smalloc malloc
#define sfree free
#define srealloc realloc
#define sstrdup strdup

#endif /* MEM_DEBUG */
