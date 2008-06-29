/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: charsetmod.h,v 1.2 2008/06/29 13:54:59 mast Exp $
*/

#ifndef CHARSETMOD_H
#define CHARSETMOD_H

#include "global.h"
#include "stralloc.h"

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

typedef p_wchar1 UNICHAR;

#define DEFCHAR (0xfffd)

#define MODE_94   0
#define MODE_96   1
#define MODE_9494 2
#define MODE_9696 3
#define MODE_BIG5 4

#define VARIANT_JP  1
#define VARIANT_CN  2
#define VARIANT_KR  3
#define VARIANT_JP2 4

struct charset_def {
  const char *const name;
  const UNICHAR *const table;
  int mode;
};

struct real_charset_def {
  const char *name;
  p_wchar1 const * table;
  const int lo, hi;
};

struct multichar_table {
  const unsigned int lo;
  const unsigned int hi;
  const UNICHAR *const table;
};

struct multichar_def {
  const char *const name;
  const struct multichar_table *const table;
};

/* From misc.c */
extern const UNICHAR map_ANSI_X3_110_1983[];
extern const UNICHAR map_iso_ir_90[];
extern const UNICHAR map_T_101_G2[];
extern const UNICHAR map_T_61_8bit[];
extern const UNICHAR map_videotex_suppl[];
p_wchar1 const *misc_charset_lookup(const char *name, int *rlo, int *rhi);

/* From tables.c */
extern const UNICHAR map_ANSI_X3_4_1968[];
extern const UNICHAR map_ISO_8859_1_1998[];
extern const UNICHAR map_JIS_C6226_1983[];
extern const UNICHAR * const iso2022_94[];
extern const UNICHAR * const iso2022_96[];
extern const UNICHAR * const iso2022_9494[];
extern const UNICHAR * const iso2022_9696[];
extern const struct multichar_def multichar_map[];
extern const struct charset_def charset_map[];
extern const int num_charset_def;

/* From iso2022.c */
void iso2022_init(void);
void iso2022_exit(void);

#endif	/* !CHARSETMOD_H */
