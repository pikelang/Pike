/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: encode.h,v 1.4 2002/01/16 02:54:11 nilsson Exp $
 */
#ifndef ENCODE_H
#define ENCODE_H

/* Prototypes begin here */
struct encode_data;
void f_encode_value(INT32 args);
void f_encode_value_canonic(INT32 args);
struct decode_data;
void f_decode_value(INT32 args);
/* Prototypes end here */

#endif
