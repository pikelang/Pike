/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sscanf.h,v 1.2 2003/12/12 13:22:40 grubba Exp $
*/

#ifndef SSCANF_H
#define SSCANF_H

INT32 low_sscanf(struct pike_string *data, struct pike_string *format);
void o_sscanf(INT32 args);
PMOD_EXPORT void f_sscanf(INT32 args);

#endif
