/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: iso2022.h,v 1.7 2004/07/24 22:55:32 nilsson Exp $
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
  char const *name;
  UNICHAR const *table;
  const int mode;
};
