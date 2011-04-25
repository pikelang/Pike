/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
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

struct charset_def {
  char const *name;
  UNICHAR const *table;
  int mode;
};
