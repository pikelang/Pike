/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/

#include "global.h"

RCSID("$Id: datagram.c,v 1.4 2002/01/16 02:57:28 nilsson Exp $");

struct datagram
{
  int fd;
  int errno;
  struct svalue read_callback;
  struct svalue write_callback;
  struct svalue id;
};
