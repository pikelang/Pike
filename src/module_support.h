/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: module_support.h,v 1.7 2000/07/28 17:16:55 hubbe Exp $
 */
#ifndef MODULE_SUPPORT_H
#include <stdarg.h>

enum error_type {
  ERR_NONE,
  ERR_TOO_FEW,
  ERR_TOO_MANY,
  ERR_BAD_ARG
};

struct expect_result {
  enum error_type error_type;
  int argno;                 /* Which argument was it */
  unsigned INT32 expected;   /* What type was expected */
  TYPE_T got;               /* What type did we actually receive */
};


/* Prototypes begin here */
PMOD_EXPORT int check_args(int args, ...);
PMOD_EXPORT void check_all_args(const char *fnname, int args, ... );
int va_get_args(struct svalue *s,
		INT32 num_args,
		char *fmt,
		va_list ap);
PMOD_EXPORT int get_args(struct svalue *s,
	     INT32 num_args,
	     char *fmt, ...);
PMOD_EXPORT void get_all_args(char *fname, INT32 args, char *format,  ... );
/* Prototypes end here */

#endif
