/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: iso2022.h,v 1.9 2005/12/07 00:03:22 marcus Exp $
*/

#include "global.h"
#include "stralloc.h"

#ifndef NULL
#define NULL (0)
#endif

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
  const int mode;
};

struct multichar_table {
  const unsigned int lo;
  const unsigned int hi;
  const UNICHAR *const table;
};

extern const UNICHAR * const iso2022_94[];
extern const UNICHAR * const iso2022_96[];
extern const UNICHAR * const iso2022_9494[];
extern const UNICHAR * const iso2022_9696[];

