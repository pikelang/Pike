pushdef([AC_PROG_CC_WORKS],
[
  popdef([AC_PROG_CC_WORKS])
  if test "${ac_prog_cc_works_this_run-}" != "yes" ; then
    AC_PROG_CC_WORKS
    ac_prog_cc_works_this_run="${ac_cv_prog_cc_works-no}"
    export ac_prog_cc_works_this_run
  else
    AC_MSG_CHECKING([whether the C compiler ($CC $CFLAGS $LDFLAGS) works])
    AC_MSG_RESULT([(cached) yes])
  fi
])

pushdef([AC_PROG_CC],
[
  popdef([AC_PROG_CC])

  AC_PROG_CC

  AC_MSG_CHECKING([if we are using TCC])
  AC_CACHE_VAL(pike_cv_prog_tcc, [
    case "`$CC -V 2>&1|head -1`" in
      tcc*)
        pike_cv_prog_tcc="yes"
      ;;
      *) pike_cv_prog_tcc="no" ;;
    esac
  ])
  if test "x$pike_cv_prog_tcc" = "xyes"; then
    AC_MSG_RESULT(yes)
    TCC="yes"
    if echo "$CC $CFLAGS $CPPFLAGS" | grep " -Y" >/dev/null; then :; else
      # We want to use the system API's...
      CPPFLAGS="-Ysystem $CPPFLAGS"
    fi
  else
    AC_MSG_RESULT(no)
    TCC=no
  fi
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

pushdef([AC_CHECK_SIZEOF],
[
  popdef([AC_CHECK_SIZEOF])
  if test "x$cross_compiling" = "xyes"; then
    changequote(<<, >>)dnl
    define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
    changequote([, ])dnl
    AC_MSG_CHECKING(size of $1 ... crosscompiling)
    AC_CACHE_VAL(AC_CV_NAME,[
      cat > conftest.$ac_ext <<EOF
dnl This sometimes fails to find confdefs.h, for some reason.
dnl [#]line __oline__ "[$]0"
[#]line __oline__ "configure"
#include "confdefs.h"
#include <stdio.h>
char size_info[] = {
  0, 'S', 'i', 'Z', 'e', '_', 'I', 'n', 'F', 'o', '_',
  '0' + sizeof([$1]), 0
};
EOF
      if AC_TRY_EVAL(ac_compile); then
        if test -f "conftest.$ac_objext"; then
	  AC_CV_NAME=`(strings "conftest.$ac_objext" | sed -e '/^SiZe_InFo_[0-9]$/s/SiZe_InFo_//p' -ed; echo ifelse([$2], , 0, [$2])) | head -1`
        else
	  AC_MSG_WARN([Object file not found.])
	  AC_CV_NAME=ifelse([$2], , 0, [$2])
        fi
      else
        AC_CV_NAME=0
      fi
      rm -rf conftest*
    ])    
    AC_MSG_RESULT($AC_CV_NAME)
    undefine([AC_CV_NAME])dnl
  else :; fi
  AC_CHECK_SIZEOF($1,$2)
])

AC_DEFUN(AC_MY_CHECK_TYPE,
[
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_type_$1,
[
AC_TRY_COMPILE([
#include <sys/types.h>

#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif

],[
$1 tmp;
],ac_cv_type_$1=yes,ac_cv_type_$1=no)
])

if test $ac_cv_type_$1 = no; then
  AC_DEFINE($1,$2)
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT(yes)
fi
])


AC_DEFUN(AC_TRY_ASSEMBLE,
[ac_c_ext=$ac_ext
 ac_ext=${ac_s_ext-s}
 cat > conftest.$ac_ext <<EOF
	.file "configure"
[$1]
EOF
if AC_TRY_EVAL(ac_compile); then
  ac_ext=$ac_c_ext
  ifelse([$2], , :, [  $2
  rm -rf conftest*])
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
  ac_ext=$ac_c_ext
ifelse([$3], , , [  rm -rf conftest*
  $3
])dnl
fi
rm -rf conftest*])


define([AC_LOW_MODULE_INIT],
[
# $Id: aclocal.m4,v 1.18 2000/08/13 19:53:36 grubba Exp $

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
  AC_MSG_CHECKING([for the Pike module base directory])

  dynamic_module_makefile=../dynamic_module_makefile
  static_module_makefile=../static_module_makefile

  counter=.

  while test ! -f "$dynamic_module_makefile"
  do
    counter=.$counter
    if test $counter = .......... ; then
      AC_MSG_RESULT(failed)
      exit 1
    else
      :
    fi
    dynamic_module_makefile=../$dynamic_module_makefile
    static_module_makefile=../$static_module_makefile
  done
  AC_MSG_RESULT(found)
])

])

pushdef([AC_OUTPUT],
[
  AC_SET_MAKE

  AC_SUBST(prefix)
  export prefix
  AC_SUBST(exec_prefix)
  export exec_prefix
  AC_SUBST(CC)
  export CC
  AC_SUBST(BINDIR)
  export BINDIR
  AC_SUBST(BUILDDIR)
  export BUILDDIR
  AC_SUBST(TMP_LIBDIR)
  export TMP_BUILDDIR
  AC_SUBST(TMP_BUILDDIR)
  export INSTALL
  AC_SUBST(INSTALL)
  export AR
  AC_SUBST(AR)
  export CFLAGS
  AC_SUBST(CFLAGS)
  export CPPFLAGS
  AC_SUBST(CPPFLAGS)
  export OPTIMIZE
  AC_SUBST(OPTIMIZE)
  export WARN
  AC_SUBST(WARN)
  export CCSHARED
  AC_SUBST(CCSHARED)

ifdef([PIKE_INCLUDE_PATH],
[
  make_variables_in=PIKE_INCLUDE_PATH/make_variables_in
],[
  AC_MSG_CHECKING([for the Pike base directory])

  make_variables_in=make_variables.in

  counter=.

  while test ! -f "$srcdir/$make_variables_in"
  do
    counter=.$counter
    if test $counter = .......... ; then
      AC_MSG_RESULT(failed)
      exit 1
    else
      :
    fi
    make_variables_in=../$make_variables_in
  done

  AC_MSG_RESULT(found)
])

  AC_SUBST_FILE(make_variables)
  make_variables=make_variables

popdef([AC_OUTPUT])
AC_OUTPUT(make_variables:$make_variables_in $][1,$][2,$][3)
])
dnl
dnl
dnl
define(MY_CHECK_FUNCTION,[
AC_MSG_CHECKING(for working $1)
AC_CACHE_VAL(pike_cv_func_$1,[
AC_TRY_RUN([$2],pike_cv_func_$1=yes,pike_cv_func_$1=no,pike_cv_func_$1=no)
])
AC_MSG_RESULT([$]pike_cv_func_$1)
if test [$]pike_cv_func_$1 = yes; then
AC_DEFINE(translit(HAVE_$1,[a-z],[A-Z]))
fi])

