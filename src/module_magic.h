/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: module_magic.h,v 1.6 2002/10/08 20:22:23 nilsson Exp $
\*/

#ifndef MODULE_MAGIC_H
#define MODULE_MAGIC_H

#ifdef DYNAMIC_MODULE
#ifdef __NT__

#define pike_module_init mYDummyFunctioN1(void); __declspec(dllexport) void pike_module_init
#define pike_module_exit mYDummyFunctioN2(void); __declspec(dllexport) void pike_module_exit

#endif /* __NT__ */
#endif /* DYNAMIC_MODULE */


#ifndef PMOD_EXPORT
#define PMOD_EXPORT
#endif

#endif /* MODULE_MAGIC_H */


