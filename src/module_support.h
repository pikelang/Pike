/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#ifndef MODULE_SUPPORT_H
#include <stdarg.h>

/* Prototypes begin here */
int va_check_args(struct svalue *s, int minargs, va_list arglist);
int check_args(int args, int minargs, ...);
void check_all_args(const char *fnname, int args, int minargs, ... );
int va_get_args(struct svalue *s,
		INT32 num_args,
		char *fmt,
		va_list ap);
int get_args(struct svalue *s,
	     INT32 num_args,
	     char *fmt, ...);
void get_all_args(char *fname, INT32 args, char *format,  ... );
/* Prototypes end here */

#endif
