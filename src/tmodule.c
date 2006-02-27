/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: tmodule.c,v 1.1 2006/02/27 12:33:55 mast Exp $
*/

/* This variant of module.c is used with pre-pike and doesn't link
 * in any post-modules. */
#define PRE_PIKE
#include "module.c"
