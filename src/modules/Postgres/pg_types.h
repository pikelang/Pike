/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef _PG_TYPES_H_
#define _PG_TYPES_H_

#include "program.h"
#include "svalue.h"

#define FETCHSIZESTR	"64"
#define CURSORNAME	"_pikecursor"
#define FETCHCMD	"FETCH " FETCHSIZESTR " IN " CURSORNAME

#define BINARYCUTOFF	32	 /* binding parameters at least this size
				    are presumed to be in binary format */

#define PIKE_PG_COMMIT	1
#define PIKE_PG_FETCH	2

#endif
