/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: computedgoto.h,v 1.7 2002/10/11 01:39:39 nilsson Exp $
*/

#define UPDATE_PC()

#define PROG_COUNTER pc

#define READ_INCR_BYTE(PC)	((INT32)(ptrdiff_t)((PC)++)[0])
