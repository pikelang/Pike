define([MY_AC_PROG_CC],
[
define(ac_cv_prog_CC,pike_cv_prog_CC)
AC_PROG_CC
undefine([ac_cv_prog_CC])
])

define([AC_LOW_MODULE_INIT],
[
# $Id: aclocal.m4,v 1.2 1999/01/29 12:25:24 hubbe Exp $

MY_AC_PROG_CC

AC_DEFINE(POSIX_SOURCE)
AC_SUBST_FILE(dependencies)
dependencies=$srcdir/dependencies

AC_SUBST_FILE(dynamic_module_makefile)
AC_SUBST_FILE(static_module_makefile)
])


define([AC_MODULE_INIT],
[
AC_LOW_MODULE_INIT()

ifdef([PIKE_INCLUDE_PATH],
[
dynamic_module_makefile=PIKE_INCLUDE_PATH/dynamic_module_makefile
static_module_makefile=PIKE_INCLUDE_PATH/dynamic_module_makefile
],[
dynamic_module_makefile=../dynamic_module_makefile
static_module_makefile=../static_module_makefile

counter=.

while test ! -f "$dynamic_module_makefile"
do
  counter=.$counter
  if test $counter = .......... ; then
    exit 1
  else
    :
  fi
  dynamic_module_makefile=../$dynamic_module_makefile
  static_module_makefile=../$static_module_makefile
done
])
])
