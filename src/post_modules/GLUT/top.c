/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"

#include "config.h"

RCSID("$Id$");
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "pike_error.h"
#include "module.h"

#ifdef HAVE_LIBGLUT
#ifdef HAVE_GL_GLUT_H
#define GLUT_API_VERSION 4
#include <GL/glut.h>
#endif
#endif


PIKE_MODULE_INIT
{
#ifdef HAVE_LIBGLUT
#ifdef HAVE_GL_GLUT_H
  extern void add_auto_funcs_glut(void);
  add_auto_funcs_glut();
#endif
#endif
}


PIKE_MODULE_EXIT
{
}
