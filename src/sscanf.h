/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sscanf.h,v 1.4 2008/05/17 14:10:00 marcus Exp $
*/

#ifndef SSCANF_H
#define SSCANF_H

#define SSCANF_FLAG_76_COMPAT 0x1

INT32 low_sscanf(struct pike_string *data, struct pike_string *format, INT32 flags);
void o_sscanf(INT32 args, INT32 flags);
PMOD_EXPORT void f_sscanf(INT32 args);
void f_sscanf_76(INT32 args);

#endif
