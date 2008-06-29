/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: charsetmod.h,v 1.1 2008/06/29 12:52:03 mast Exp $
*/

#ifndef CHARSETMOD_H
#define CHARSETMOD_H

static void DECLSPEC(noreturn) transcode_error_va (
  struct pike_string *str, ptrdiff_t pos, struct pike_string *charset,
  int encode, const char *reason, va_list args) ATTRIBUTE((noreturn));

void DECLSPEC(noreturn) transcode_error (
  struct pike_string *str, ptrdiff_t pos, struct pike_string *charset,
  int encode, const char *reason, ...);

/* This picks out the charset by indexing Pike_fp->current_object with
 * "charset". */
void DECLSPEC(noreturn) transcoder_error (
  struct pike_string *str, ptrdiff_t pos, int encode, const char *reason, ...);

#endif	/* !CHARSETMOD_H */
