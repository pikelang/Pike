pushdef([AC_PROG_CC],
[
  popdef([AC_PROG_CC])

  AC_PROG_CC

  case "`$CC -V 2>&1|head -1`" in
    tcc*)
      TCC="yes"
      if echo "$CC $CFLAGS $CPPFLAGS" | grep " -Y" >/dev/null; then :; else
	# We want to use the system API's...
	CPPFLAGS="-Ysystem $CPPFLAGS"
      fi
    ;;
    *) TCC="no" ;;
  esac
])

define([MY_AC_PROG_CC],
[
define(ac_cv_prog_CC,pike_cv_prog_CC)
AC_PROG_CC
undefine([ac_cv_prog_CC])
])

pushdef([AC_CONFIG_HEADER],
[
  CONFIG_HEADERS="$1"
  popdef([AC_CONFIG_HEADER])
  AC_CONFIG_HEADER($1)
])

define([AC_LOW_MODULE_INIT],
[
# $Id: aclocal.m4,v 1.8 1999/04/25 18:53:29 grubba Exp $

MY_AC_PROG_CC

AC_DEFINE(POSIX_SOURCE)

AC_SUBST(CONFIG_HEADERS)

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
