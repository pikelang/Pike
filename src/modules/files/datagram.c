/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"

RCSID("$Id: datagram.c,v 1.2 1998/03/28 14:00:23 grubba Exp $");

struct datagram
{
  int fd;
  int errno;
  struct svalue read_callback;
  struct svalue write_callback;
  struct svalue id;
};
