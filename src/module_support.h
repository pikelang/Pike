/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
\*/

/*
 * $Id: module_support.h,v 1.11 2002/05/31 22:41:25 nilsson Exp $
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

/* This should be used in module_init */
#define PIKE_MODULE_EXPORT(MOD, SYM) \
  pike_module_export_symbol(#MOD "." #SYM, CONSTANT_STRLEN(#MOD "." #SYM), SYM)

#define PIKE_MODULE_IMPORT(MOD, SYM) \
  pike_module_import_symbol(#MOD "." #SYM, CONSTANT_STRLEN(#MOD "." #SYM), #MOD, CONSTANT_STRLEN(#MOD))


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
PMOD_EXPORT void pike_module_export_symbol(char *str,
					   int len,
					   void *ptr);
PMOD_EXPORT void *pike_module_import_symbol(char *str,
					    int len,
					    char *module,
					    int module_len);
void cleanup_module_support (void);
/* Prototypes end here */

#endif
