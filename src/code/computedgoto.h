/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#define UPDATE_PC()

#define PROG_COUNTER pc

#define READ_INCR_BYTE(PC)	((INT32)(ptrdiff_t)((PC)++)[0])
