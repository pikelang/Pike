/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: datagram.c,v 1.8 2002/10/11 01:39:54 nilsson Exp $
*/

#include "global.h"

RCSID("$Id: datagram.c,v 1.8 2002/10/11 01:39:54 nilsson Exp $");

struct datagram
{
  int fd;
  int errno;
  struct svalue read_callback;
  struct svalue write_callback;
  struct svalue id;
};
