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
