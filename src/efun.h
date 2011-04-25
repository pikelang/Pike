/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef EFUN_H
#define EFUN_H

typdef void (*efun_t) (INT32);

/* I need:
 * a) one type that can point to a callable function.
 *    (C function, or object->fun)
 * b) one type that once the object/program is known can point
 *    to the C/Pike function body.
 * c) A number of flags to send to 'add_simul_efun' to specify side effects
 *    and such.
 * d) A type struct
 */

#define EFUN_CONST 1
#define EFUN_LOCAL_SIDE_EFFECT 2
#define EFUN_GLOBAL_SIDE_EFFECT 4
#define EFUN_OTHER_SIDE_EFFECT 8
#define EFUN_SIDE_EFFECT (EFUN_LOCAL_SIDE_EFFECT |\
			  EFUN_GLOBAL_SIDE_EFFECT |\
			  EFUN_OTHER_SIDE_EFFECT )

struct type
{
  INT16 type;
  struct type *car,*cdr;
};


#endif
