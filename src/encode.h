/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

/*
 * $Id: encode.h,v 1.3 2000/03/26 01:55:11 mast Exp $
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
