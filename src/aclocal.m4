dnl $Id: aclocal.m4,v 1.61 2004/03/23 10:40:12 grubba Exp $

dnl Some compatibility with Autoconf 2.50+. Not complete.
dnl newer autoconf call substr m4_substr
ifdef([substr], ,m4_copy(m4_substr,substr))
dnl newer Autoconf calls changequote m4_changequote
ifdef([changequote], ,[m4_copy([m4_changequote],[changequote])])
dnl Autoconf 2.53+ hides their version numbers in m4_PACKAGE_VERSION.
ifdef([AC_ACVERSION], ,[m4_copy([m4_PACKAGE_VERSION],[AC_ACVERSION])])

define([if_autoconf],
[ifelse(ifelse(index(AC_ACVERSION,.),-1,0,[m4_eval(
  substr(AC_ACVERSION, 0, index(AC_ACVERSION,.))-0 >= $1 &&
  (
   substr(AC_ACVERSION, 0, index(AC_ACVERSION,.))-0 > $1 ||
   substr(AC_ACVERSION, index(+AC_ACVERSION,.))-0 >= $2
  )
)]),1,$3,$4)])

pushdef([AC_PROG_CC_WORKS],
[
  popdef([AC_PROG_CC_WORKS])
  if test "x$enable_binary" != "xno"; then
    if test "${ac_prog_cc_works_this_run-}" != "yes" ; then
      AC_PROG_CC_WORKS
      ac_prog_cc_works_this_run="${ac_cv_prog_cc_works-no}"
      export ac_prog_cc_works_this_run
    else
      AC_MSG_CHECKING([whether the C compiler ($CC $CFLAGS $LDFLAGS) works])
      AC_MSG_RESULT([(cached) yes])
    fi
  fi
])

pushdef([AC_PROG_CC],
[
  popdef([AC_PROG_CC])

  AC_PROG_CC

  if test "$ac_test_CFLAGS" = set; then :; else
    if test "$GCC" = yes; then
      # Remove -O2, and use a real test to restore it.
      if test "$ac_cv_prog_cc_g" = yes; then
	CFLAGS="-g"
      else
	CFLAGS=
      fi
    else :; fi
  fi

  AC_MSG_CHECKING([if we are using TCC])
  AC_CACHE_VAL(pike_cv_prog_tcc, [
    case "`$CC -V 2>&1|head -n 1`" in
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

dnl option, descr, with, without, default
define([MY_AC_ARG_WITH], [
  AC_ARG_WITH([$1], [$2], [
    if test "x$withval" = "xno"; then
      ifelse([$4], , :, [$4])
    else
      ifelse([$3], , :, [$3])
    fi
  ], [$5])
])

dnl flag, descr
define([MY_DESCR],
       [  substr([$1][                                  ],0,33) [$2]])

define([MY_AC_PROG_CC],
[
  define(ac_cv_prog_CC,pike_cv_prog_CC)
  AC_PROG_CC
  undefine([ac_cv_prog_CC])
  AC_PROG_CPP
  if test "x$enable_binary" = "no"; then
    # Do the check above even when --disable-binary is used, since we
    # need a real $CPP, and AC_PROG_CPP wants AC_PROG_CC to be called
    # earlier.
    CC="$BINDIR/nobinary_dummy cc"
  fi
])

pushdef([AC_CONFIG_HEADER],
[
  CONFIG_HEADERS="$1"
  popdef([AC_CONFIG_HEADER])
  AC_CONFIG_HEADER($1)
])

define([ORIG_AC_CHECK_SIZEOF], defn([AC_CHECK_SIZEOF]))
pushdef([AC_CHECK_SIZEOF],
[
  changequote(<<, >>)dnl
  define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
  changequote([, ])dnl
  if test "x$cross_compiling" = "xyes" -o "x$TCC" = "xyes"; then
    AC_MSG_CHECKING(size of $1 ... crosscompiling or tcc)
    AC_CACHE_VAL(AC_CV_NAME,[
      cat > conftest.$ac_ext <<EOF
dnl This sometimes fails to find confdefs.h, for some reason.
dnl [#]line __oline__ "[$]0"
[#]line __oline__ "configure"
#include "confdefs.h"

/* The worlds most stringent C compiler? */
#ifdef __TenDRA__
/* We want to be able to use 64bit arithmetic */
#ifdef HAVE_PRAGMA_TENDRA_LONGLONG
#pragma TenDRA longlong type allow
#endif /* HAVE_PRAGMA_TENDRA_LONGLONG */
#ifdef HAVE_PRAGMA_TENDRA_SET_LONGLONG_TYPE
#pragma TenDRA set longlong type : long long
#endif /* HAVE_PRAGMA_TENDRA_SET_LONGLONG_TYPE */

#ifdef _NO_LONGLONG
#undef _NO_LONGLONG
#endif /* _NO_LONGLONG */
#endif /* __TenDRA__ */

#include <stdio.h>

char size_info[[]] = {
  0, 'S', 'i', 'Z', 'e', '_', 'I', 'n', 'F', 'o', '_',
  '0' + sizeof([$1]), 0
};
EOF
      if AC_TRY_EVAL(ac_compile); then
        if test -f "conftest.$ac_objext"; then
	  AC_CV_NAME=`strings "conftest.$ac_objext" | sed -e '/^SiZe_InFo_[[0-9]]$/s/SiZe_InFo_//p' -ed | head -n 1`
          if test "x$AC_CV_NAME" = "x"; then
	    AC_MSG_WARN([Magic cookie not found.])
	    AC_CV_NAME=ifelse([$2], , 0, [$2])
	  else :; fi
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
  elif test "x$enable_binary" = "xno"; then
    AC_CV_NAME=$2
  fi
  undefine([AC_CV_NAME])dnl
  ORIG_AC_CHECK_SIZEOF($1,$2)
])

define([ORIG_CHECK_HEADERS], defn([AC_CHECK_HEADERS]))
pushdef([AC_CHECK_HEADERS],
[
  if test "x$enable_binary" != "xno"; then
    ORIG_CHECK_HEADERS($1,$2,$3)
  else
    for ac_hdr in $1
    do
      ac_safe=`echo "$ac_hdr" | sed 'y%./+-%__p_%'`
      eval "ac_cv_header_$ac_safe=yes"
    done
  fi
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

$3

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

dnl PIKE_CHECK_CONSTANTS(checking_message, constant_names, includes, define_name)
dnl
dnl define_name will be defined to the first constant in
dnl constant_names that exists. It will remain undefined if none of
dnl them exists.
AC_DEFUN(PIKE_CHECK_CONSTANTS,
[
  AC_MSG_CHECKING([$1])
  AC_CACHE_VAL(pike_cv_$4_value, [
    pike_cv_$4_value=""
    for const in $2
    do
      AC_TRY_COMPILE([$3], [int tmp = (int) $const;], [
	pike_cv_$4_value="$const"
	break
      ])
    done
  ])
  if test x"$pike_cv_$4_value" != x; then
    AC_MSG_RESULT($pike_cv_$4_value)
    AC_DEFINE_UNQUOTED([$4], $pike_cv_$4_value)
  else
    AC_MSG_RESULT(none)
  fi
])


dnl 
dnl PIKE_FEATURE_CLEAR()
dnl PIKE_FEATURE(feature,text)
dnl

define(PIKE_FEATURE_CLEAR,[
  rm pike_*.feature 2>/dev/null
])

define(PIKE_FEATURE_RAW,[
  cat >pike_[$1].feature <<EOF
[$2]
EOF])

define([PAD_FEATURE],[substr([$1][................................],0,20) ])

define(PIKE_FEATURE_3,[
  cat >pike_[$1].feature <<EOF
PAD_FEATURE([$2])[$3]
EOF])

define(PIKE_FEATURE,[
  PIKE_FEATURE_3(translit([[$1]],[. ()],[____]),[$1],[$2])
])

define(PIKE_FEATURE_WITHOUT,[
  PIKE_FEATURE([$1],[no (forced without)])
])

define(PIKE_FEATURE_NODEP,[
  PIKE_FEATURE([$1],[no (dependencies failed)])
])

define(PIKE_FEATURE_OK,[
  PIKE_FEATURE([$1],[yes])
])


define([AC_LOW_MODULE_INIT],
[
# $Id: aclocal.m4,v 1.61 2004/03/23 10:40:12 grubba Exp $

MY_AC_PROG_CC

AC_DEFINE(POSIX_SOURCE)

AC_SUBST(CONFIG_HEADERS)

AC_SUBST_FILE(dependencies)
dependencies=$srcdir/dependencies

AC_SUBST_FILE(dynamic_module_makefile)
AC_SUBST_FILE(static_module_makefile)

AC_ARG_WITH(root,   [  --with-root=path      specify a cross-compilation root-directory],[
  case "$with_root" in
    /)
      with_root=""
    ;;
    /*)
    ;;
    no)
      with_root=""
    ;;
    *)
      AC_MSG_WARN([Root path $with_root is not absolute. Ignored.])
      with_root=""
    ;;
  esac
],[with_root=""])

if test "x$enable_binary" = "xno"; then
  # Fix makefile rules as if we're cross compiling, to use pike
  # fallbacks etc. Do this without setting $ac_cv_prog_cc_cross to yes
  # since autoconf macros like AC_TRY_RUN will complain bitterly then.
  CROSS=yes
else
  CROSS="$ac_cv_prog_cc_cross"
  # newer autoconf
  if test x"$CROSS" = x; then
     CROSS="$cross_compiling"
  fi
fi

AC_SUBST(CROSS)

if test "x$enable_binary" = "xno"; then
  RUNPIKE="USE_PIKE"
  RUNTPIKE="USE_PIKE"
elif test "x$ac_cv_prog_cc_cross" = "xyes"; then
  RUNPIKE="DEFAULT_RUNPIKE"
  RUNTPIKE="USE_PIKE"
else
  RUNPIKE="DEFAULT_RUNPIKE"
  RUNTPIKE="USE_TPIKE"
fi
AC_SUBST(RUNPIKE)
AC_SUBST(RUNTPIKE)
])


define([AC_MODULE_INIT],
[
AC_LOW_MODULE_INIT()
PIKE_FEATURE_CLEAR()

if test -d $BUILD_BASE/modules/. ; then
  dynamic_module_makefile=$BUILD_BASE/modules/dynamic_module_makefile
  static_module_makefile=$BUILD_BASE/modules/static_module_makefile
else
  dynamic_module_makefile=$BUILD_BASE/dynamic_module_makefile
  static_module_makefile=$BUILD_BASE/dynamic_module_makefile
fi

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
  AC_SUBST(CPP)
  export CPP
  AC_SUBST(BINDIR)
  export BINDIR
  AC_SUBST(BUILDDIR)
  export BUILDDIR
  AC_SUBST(PIKE_SRC_DIR)
  export PIKE_SRC_DIR
  AC_SUBST(BUILD_BASE)
  export BUILD_BASE
  AC_SUBST(INSTALL)
  export INSTALL
  AC_SUBST(AR)
  export AR
  AC_SUBST(CFLAGS)
  export CFLAGS
  AC_SUBST(CPPFLAGS)
  export CPPFLAGS
  AC_SUBST(OPTIMIZE)
  export OPTIMIZE
  AC_SUBST(WARN)
  export WARN
  AC_SUBST(CCSHARED)
  export CCSHARED
  AC_SUBST(LDSHARED)
  export LDSHARED

  PMOD_TARGETS=`echo $srcdir/*.cmod | sed -e "s/\.cmod/\.c/g" | sed -e "s|$srcdir/|\\$(SRCDIR)/|g"`
  test "$PMOD_TARGETS" = '$(SRCDIR)/*.c' && PMOD_TARGETS=
  AC_SUBST(PMOD_TARGETS)

  AC_MSG_CHECKING([for the Pike base directory])
  if test "x$PIKE_SRC_DIR" != "x" -a -f "${PIKE_SRC_DIR}/make_variables.in"; then
    make_variables_in="${PIKE_SRC_DIR}/make_variables.in"
    AC_MSG_RESULT(${PIKE_SRC_DIR})

    if_autoconf(2,50,,[
      # Kludge for autoconf 2.13 and earlier prefixing all substitution
      # source files with $ac_given_source_dir/ (aka $srcdir/).
      make_variables_in="`cd $srcdir;pwd|sed -e 's@[[[^/]]]*@@g;s@/@../@g'`$make_variables_in"
    ])
  else

    counter=.

    uplevels=
    while test ! -f "$srcdir/${uplevels}make_variables.in"
    do
      counter=.$counter
      if test $counter = .......... ; then
        AC_MSG_RESULT(failed)
        exit 1
      else
        :
      fi
      uplevels=../$uplevels
    done

    make_variables_in=${uplevels}make_variables.in
    AC_MSG_RESULT(${uplevels}.)
  fi

  AC_SUBST(make_variables_in)

  AC_SUBST_FILE(make_variables)
  make_variables=make_variables

  # Autoconf 2.50 and later stupidity...
  if_autoconf(2,50,[
    dnl AC_MSG_WARN(cleaning the environment from autoconf 2.5x pollution)
  
    unset ac_cv_env_build_alias_set
    unset ac_cv_env_build_alias_value
    unset ac_cv_env_host_alias_set
    unset ac_cv_env_host_alias_value
    unset ac_cv_env_target_alias_set
    unset ac_cv_env_target_alias_value
    unset ac_cv_env_CC_set
    unset ac_cv_env_CC_value
    unset ac_cv_env_CFLAGS_set
    unset ac_cv_env_CFLAGS_value
    unset ac_cv_env_LDFLAGS_set
    unset ac_cv_env_LDFLAGS_value
    unset ac_cv_env_CPPFLAGS_set
    unset ac_cv_env_CPPFLAGS_value
    unset ac_cv_env_CPP_set
    unset ac_cv_env_CPP_value
  ])

  popdef([AC_OUTPUT])
  AC_OUTPUT(make_variables:$make_variables_in $][1,$][2,$][3)
])
dnl
dnl
dnl

define(MY_CHECK_FUNCTION,[
  AC_MSG_CHECKING(for working $1)
  AC_CACHE_VAL(pike_cv_func_$1,[
    AC_TRY_RUN([
$2
int main() {
$3;
return 0;
}
], pike_cv_func_$1=yes, pike_cv_func_$1=no, [
      echo $ac_n "crosscompiling... $ac_c" 1>&6
      AC_TRY_LINK([$2], [$3], pike_cv_func_$1=yes, pike_cv_func_$1=no)
    ])
  ])
  AC_MSG_RESULT([$]pike_cv_func_$1)
  if test [$]pike_cv_func_$1 = yes; then
    AC_DEFINE(translit(HAVE_$1,[a-z],[A-Z]))
  else :; fi
])

dnl These are like AC_PATH_PROG etc, but gives a path to
dnl nobinary_dummy when --disable-binary is used. That program will
dnl always return true and have ' ' as output.
define(MY_AC_CHECK_PROG,[
  if test "x$enable_binary" = "xno"; then
    AC_CHECK_PROG($1,nobinary_dummy,$3,$4,$BINDIR)
  else
    AC_CHECK_PROG($1,$2,$3,$4,$5,$6)
  fi
])
define(MY_AC_CHECK_PROGS,[
  if test "x$enable_binary" = "xno"; then
    AC_CHECK_PROGS($1,nobinary_dummy,$3,$BINDIR)
  else
    AC_CHECK_PROGS($1,$2,$3,$4)
  fi
])
define(MY_AC_PATH_PROG,[
  if test "x$enable_binary" = "xno"; then
    AC_PATH_PROG($1,nobinary_dummy,$3,$BINDIR)
  else
    AC_PATH_PROG($1,$2,$3,$4)
  fi
])
define(MY_AC_PATH_PROGS,[
  if test "x$enable_binary" = "xno"; then
    AC_PATH_PROGS($1,nobinary_dummy,$3,$BINDIR)
  else
    AC_PATH_PROGS($1,$2,$3,$4)
  fi
])

dnl MY_AC_CHECK_PRINTF_INT_TYPE(type, alternatives, default, define, result message)
define(MY_AC_CHECK_PRINTF_INT_TYPE, [
  AC_MSG_CHECKING(how to printf $1)
  AC_CACHE_VAL(pike_cv_printf_$1, [
    found=no
    for mod in $2 ; do
      AC_TRY_RUN([
#include <stddef.h>
#include <stdio.h>
#include "confdefs.h"
int main() {
  char buf[50];
  if ((($1)4711) > (($1)-4711)) {
    /* Signed type. */
    if (sizeof($1)>4)
    {
      sprintf(buf, "%${mod}d,%${mod}d,%d",
	      (($1) 4711) << 32, -(($1) 4711) << 32, 17);
      return !!strcmp("20233590931456,-20233590931456,17", buf);
    }
    else
    {
      sprintf(buf, "%${mod}d,%${mod}d,%d", ($1) 4711, ($1)-4711, 17);
      return !!strcmp("4711,-4711,17", buf);
    }
  } else {
    /* Unsigned type. */
    if (sizeof($1)>4)
    {
      sprintf(buf, "%${mod}d,%d",
	      (($1) 4711) << 32, 17);
      return !!strcmp("20233590931456,17", buf);
    }
    else
    {
      sprintf(buf, "%${mod}d,%d", ($1) 4711, 17);
      return !!strcmp("4711,17", buf);
    }
  }
}], [pike_cv_printf_$1="${mod}"; found=yes])
      test ${found} = yes && break
    done
    test ${found} = no && pike_cv_printf_$1=unknown
  ])
  if test x"${pike_cv_printf_$1}" = xunknown ; then
    res=$3
    AC_MSG_RESULT([none found, defaulting to $5])
  else
    res="${pike_cv_printf_$1}"
    AC_MSG_RESULT([$5])
  fi
  AC_DEFINE_UNQUOTED($4, "${res}")
])

dnl MY_AC_CHECK_PRINTF_FLOAT_TYPE(type, alternatives, default, define, result message)
define(MY_AC_CHECK_PRINTF_FLOAT_TYPE, [
  AC_MSG_CHECKING(how to printf $1)
  AC_CACHE_VAL(pike_cv_printf_$1, [
    found=no
    for mod in $2 ; do
      AC_TRY_RUN([
#include <stddef.h>
#include <stdio.h>
#include "confdefs.h"
int main() {
  char buf[50];
  sprintf(buf, "%${mod}4.1f,%d",($1)17.0,17);
  return !!strcmp("17.0,17",buf);
}], [pike_cv_printf_$1="${mod}"; found=yes])
      test ${found} = yes && break
    done
    test ${found} = no && pike_cv_printf_$1=unknown
  ])
  if test x"${pike_cv_printf_$1}" = xunknown ; then
    res=$3
    AC_MSG_RESULT([none found, defaulting to $5])
  else
    res="${pike_cv_printf_$1}"
    AC_MSG_RESULT([$5])
  fi
  AC_DEFINE_UNQUOTED($4, "${res}")
])
