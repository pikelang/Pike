/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MODULE_SUPPORT_H
#define MODULE_SUPPORT_H

#include "module.h"

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

/* These should be used in module_init */
#define PIKE_MODULE_EXPORT_SYMBOL(KEY, SYM) \
  pike_module_export_symbol(KEY, CONSTANT_STRLEN(KEY), (void *)(SYM))

#define PIKE_MODULE_IMPORT_SYMBOL(KEY, MOD_NAME)                       \
  pike_module_import_symbol(KEY, CONSTANT_STRLEN(KEY),                 \
                            MOD_NAME, CONSTANT_STRLEN(MOD_NAME))

#define PIKE_MODULE_EXPORT(MOD, SYM) \
  PIKE_MODULE_EXPORT_SYMBOL(#MOD "." #SYM, SYM)

#define PIKE_MODULE_IMPORT(MOD, SYM) \
  PIKE_MODULE_IMPORT_SYMBOL(#MOD "." #SYM, #MOD)

#define HIDE_MODULE() ADD_INT_CONSTANT("this_program_does_not_exist",1,0)

/* Prototypes begin here */
PMOD_EXPORT int check_args(int args, ...);
PMOD_EXPORT void check_all_args(const char *fnname, int args, ... );
int va_get_args(struct svalue *s,
		INT32 num_args,
		const char *fmt,
		va_list ap);
#ifdef NOT_USED
PMOD_EXPORT int get_args(struct svalue *s,
	     INT32 num_args,
	     const char *fmt, ...);
#endif
PMOD_EXPORT void get_all_args(const char *fname, INT32 args,
			      const char *format,  ... );
PMOD_EXPORT void pike_module_export_symbol(const char *str,
					   int len,
					   void *ptr);
PMOD_EXPORT void *pike_module_import_symbol(const char *str,
					    int len,
					    const char *module,
					    int module_len);
void cleanup_module_support (void);
/* Prototypes end here */

#endif
