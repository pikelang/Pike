/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
\*/
/**/

#include "global.h"

RCSID("$Id: datagram.c,v 1.5 2002/05/31 22:31:40 nilsson Exp $");

struct datagram
{
  int fd;
  int errno;
  struct svalue read_callback;
  struct svalue write_callback;
  struct svalue id;
};
