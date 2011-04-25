/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef SSCANF_H
#define SSCANF_H

INT32 low_sscanf(struct pike_string *data, struct pike_string *format);
void o_sscanf(INT32 args);
PMOD_EXPORT void f_sscanf(INT32 args);

#endif
