/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: cpp.h,v 1.11 2004/06/29 11:19:19 nilsson Exp $
*/

#ifndef CPP_H
#define CPP_H

#ifndef STRUCT_HASH_ENTRY_DECLARED
struct hash_entry;
#define STRUCT_HASH_ENTRY_DECLARED
#endif

/* Prototypes begin here */
struct define_part;
struct define_argument;
struct define;
struct cpp;
static void cpp_error(struct cpp *this, const char *err);
static void cpp_error_vsprintf (struct cpp *this, const char *fmt,
				va_list args);
static void cpp_error_sprintf(struct cpp *this, const char *fmt, ...)
  ATTRIBUTE((format(printf,2,3)));
static void cpp_handle_exception(struct cpp *this,
				 const char *cpp_error_fmt, ...)
  ATTRIBUTE((format(printf,2,3)));
static void cpp_warning(struct cpp *this, const char *cpp_warn_fmt, ...)
  ATTRIBUTE((format(printf,2,3)));
void PUSH_STRING(char *str,
		 INT32 len,
		 dynamic_buffer *buf);
void free_one_define(struct hash_entry *h);
void f_cpp(INT32 args);
void init_cpp(void);
void add_predefine(char *s);
void exit_cpp(void);
/* Prototypes end here */

/* Return true if compat version is equal or less than MAJOR.MINOR */
#define CPP_TEST_COMPAT(THIS,MAJOR,MINOR)      \
  (THIS->compat_major < (MAJOR) ||	       \
   (THIS->compat_major == (MAJOR) &&	       \
    THIS->compat_minor <= (MINOR)))

#endif
