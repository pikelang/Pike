/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: iso2022.h,v 1.8 2004/09/27 21:45:35 nilsson Exp $
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
