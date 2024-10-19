dnl Some compatibility with Autoconf 2.50+. Not complete.
dnl newer Autoconf calls substr m4_substr
ifdef([substr], ,[m4_copy([m4_substr],[substr])])
dnl newer Autoconf calls changequote m4_changequote
ifdef([changequote], ,[m4_copy([m4_changequote],[changequote])])
dnl Autoconf 2.53+ hides their version numbers in m4_PACKAGE_VERSION.
ifdef([AC_ACVERSION], ,[m4_copy([m4_PACKAGE_VERSION],[AC_ACVERSION])])
dnl Old autoconf doesn't have _AC_OUTPUT_SUBDIRS.
ifdef([_AC_OUTPUT_SUBDIRS], ,
      [define([_AC_OUTPUT_SUBDIRS],
	      [AC_OUTPUT_SUBDIRS(AC_LIST_SUBDIRS)])])

dnl Autoconf 2.71+ has put AC_REQUIRE[AC_CANONICAL_HOST] everywhere...
m4_copy([AC_CANONICAL_HOST], [ORIG_AC_CANONICAL_HOST])
m4_defun([AC_CANONICAL_HOST], [])

dnl Not really a prerequisite, but suggest the use of Autoconf 2.50 to
dnl autoconf-wrapper if it is used.  dnl can't be used since the wrapper
dnl checks for it, so just store it in a dummy define.
define([require_autoconf_2_50],[AC_PREREQ(2.50)])

dnl major, minor, on_ge, on_lt
define([if_autoconf],
[ifelse(ifelse(index(AC_ACVERSION,.),-1,0,[m4_eval(
  translit(substr(AC_ACVERSION, 0, index(AC_ACVERSION,.)),[A-Za-z])-0 >= $1 &&
  (
   translit(substr(AC_ACVERSION, 0, index(AC_ACVERSION,.)),[A-Za-z])-0 > $1 ||
   translit(substr(AC_ACVERSION, index(+AC_ACVERSION,.)),[A-Za-z])-0 >= $2
  )
)]),1,[$3],[$4])])

dnl Autoconf 2.60 is the first version that supports C99.
dnl C99-compilers complain about implicit declarations.
dnl For autoconf 2.59 and earlier: Make sure at least
dnl exit(3C) is declared.
dnl
dnl NB: Simply always including <stdlib.h> causes some
dnl     AC_HAVE_FUNC tests to fail due to mismatching
dnl     prototypes.
dnl
dnl We overload _AC_PROG_CC_STDC to insert an extra
dnl call of _AC_PROG_CXX_EXIT_DECLARATION due to it
dnl usually only being used for C++.
if_autoconf(2,60,,[
  m4_copy([_AC_PROG_CC_STDC], [ORIG__AC_PROG_CC_STDC])
  m4_define([_AC_PROG_CC_STDC], [
    ORIG__AC_PROG_CC_STDC
    # Some C99 compilers default to -Werror,-Wimplicit-function-declaration
    # Attempt to find a suitable prototype for exit(3C).
    _AC_COMPILE_IFELSE([@%:@ifdef __cplusplus
      choke me
@%:@endif], [
      patsubst(_AC_PROG_CXX_EXIT_DECLARATION, [ifdef], [ifndef])
    ])
  ])
])

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

  if test "$ac_cv_prog_cc_g" = no; then
    # The -g test is broken for some compilers (eg ecc), since
    # they always have output (they echo the name of the source file).
    AC_MSG_CHECKING(if -g might not be ok after all)
    AC_CACHE_VAL(pike_cv_prog_cc_g, [
      echo 'void f(){}' > conftest.c
      if test "`${CC-cc} -g -c conftest.c 2>&1`" = \
	      "`${CC-cc} -c conftest.c 2>&1`"; then
	pike_cv_prog_cc_g=yes
      else
	pike_cv_prog_cc_g=no
      fi
      rm -f conftest*
    ])
    if test "$pike_cv_prog_cc_g" = "yes"; then
      AC_MSG_RESULT(yes)
      ac_cv_prog_cc_g=yes
    else
      AC_MSG_RESULT(no)
    fi
  fi

  if test "$ac_test_CFLAGS"; then :; else
    if test "$GCC" = yes; then
      # Remove -O2, and use a real test to restore it.
      if test "$ac_cv_prog_cc_g" = yes; then
	CFLAGS="-g"
      else
	CFLAGS=
      fi
    else :; fi
  fi

  AC_MSG_CHECKING([if we are using ICC (Intel C Compiler)])
  AC_CACHE_VAL(pike_cv_prog_icc, [
    if $CC -V 2>&1 | grep -i Intel >/dev/null; then
      pike_cv_prog_icc="yes"
    else
      pike_cv_prog_icc="no"
    fi
  ])
  if test "x$pike_cv_prog_icc" = "xyes"; then
    AC_MSG_RESULT(yes)
    ICC="yes"
    # Make sure libimf et al are linked statically.
    # NB: icc 6, 7 and 8 only have static versions.
    AC_MSG_CHECKING([if it is ICC 9.0 or later])
    icc_version="`$CC -V 2>&1 | sed -e '/^Version /s/Version \([0-9]*\)\..*/\1/p' -ed`"
    if test "0$icc_version" -ge 9; then
      if echo "$CC $LDFLAGS $LIBS" | grep " -i-" >/dev/null; then
        AC_MSG_RESULT(yes - $icc_version)
      else
        AC_MSG_RESULT(yes - $icc_version - Adding -i-static)
        LDFLAGS="-i-static $LDFLAGS"
      fi
    else
      if test "x$icc_version" = x; then
	AC_MSG_RESULT(no - no version information)
      else
	AC_MSG_RESULT(no - $icc_version)
      fi
    fi
  else
    AC_MSG_RESULT(no)
    ICC=no
  fi
])

# Check for libgcc.
define([PIKE_CHECK_LIBGCC],[
  if test $ac_cv_prog_gcc = yes; then
    AC_MSG_CHECKING(for libgcc file name)
    if test -f "$pike_cv_libgcc_filename"; then :; else
      # libgcc has gone away probably due to gcc having been upgraded.
      # Invalidate the entry.
      unset pike_cv_libgcc_filename
    fi
    AC_CACHE_VAL(pike_cv_libgcc_filename,
    [
      pike_cv_libgcc_filename="`${CC-cc} $CCSHARED -print-libgcc-file-name`"
      if test -z "$pike_cv_libgcc_filename"; then
        pike_cv_libgcc_filename=no
      else
         if test -f "$pike_cv_libgcc_filename"; then
           pic_name=`echo "$pike_cv_libgcc_filename"|sed -e 's/\.a$/_pic.a/'`
  	 if test -f "$pic_name"; then
  	   pike_cv_libgcc_filename="$pic_name"
  	 fi
         else
           pike_cv_libgcc_filename=no
         fi
      fi
    ])
    AC_MSG_RESULT($pike_cv_libgcc_filename)
    if test x"$pike_cv_libgcc_filename" = xno; then
      LIBGCC=""
    else
      LIBGCC="$pike_cv_libgcc_filename"
    fi
  else
    LIBGCC=""
  fi
  AC_SUBST(LIBGCC)
])

dnl Like AC_PATH_PROG but if $2 isn't found and $RNTANY is set, tries
dnl to execute "$RNTANY $2 /?" and defines $1 to "$RNTANY $2" if that
dnl succeeds.
define([PIKE_NT_PROG], [
  AC_PATH_PROG([$1], [$2], , [$4])
  if test "x$$1" = x; then
    if test "x$RNTANY" != x; then
      AC_MSG_CHECKING([if $RNTANY $2 /? can be executed])
      AC_CACHE_VAL(pike_cv_exec_rntany_$1, [
	if $RNTANY $2 '/?' >/dev/null 2>&1; then
	  pike_cv_exec_rntany_$1=yes
	else
	  pike_cv_exec_rntany_$1=no
	fi
      ])
      if test "x$pike_cv_exec_rntany_$1" = xyes; then
	AC_MSG_RESULT(yes)
	$1="$RNTANY $2"
      else
	AC_MSG_RESULT(no)
	$1="$3"
      fi
    else
      $1="$3"
    fi
  fi
])

define([ORIG_AC_FUNC_MMAP], defn([AC_FUNC_MMAP]))
define([AC_FUNC_MMAP], [
  if_autoconf(2,50,[],[
    cat >>confdefs.h <<\EOF
/* KLUDGE for broken prototype in the autoconf 1.13 version of the test. */
#include <stdlib.h> /* KLUDGE */
char *my_malloc(sz) unsigned long sz; { return malloc(sz); } /* KLUDGE */
#define malloc	my_malloc	/* KLUDGE */
EOF
  ])
  ORIG_AC_FUNC_MMAP
  if_autoconf(2,50,[],[
    sed -e '/\/\* KLUDGE /d' <confdefs.h >confdefs.h.tmp
    mv confdefs.h.tmp confdefs.h
  ])
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
  dnl Note: It's tricky to speed this up by only doing it once when
  dnl the modules are configured with the base - different autoconfs
  dnl set different variables that might not get propagated, e.g.
  dnl $ac_exeext in 2.59.
  define(ac_cv_prog_CC,pike_cv_prog_CC)
  AC_PROG_CC
  undefine([ac_cv_prog_CC])
  AC_PROG_CPP
  if test "x$enable_binary" = x"no"; then
    # Do the check above even when --disable-binary is used, since we
    # need a real $CPP, and AC_PROG_CPP wants AC_PROG_CC to be called
    # earlier.
    CC="$BINDIR/nobinary_dummy cc"
    ac_cv_c_undeclared_builtin_options='none needed'
  fi
])

dnl Not available before Autoconf 2.60.
ifdef([AC_USE_SYSTEM_EXTENSIONS],[],[
  m4_defun([AC_USE_SYSTEM_EXTENSIONS], [
    AH_VERBATIM([USE_SYSTEM_EXTENSIONS],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
/* Enable non-POSIX declarations on Darwin. */
#ifndef _DARWIN_C_SOURCE
# undef _DARWIN_C_SOURCE
#endif
/* Enable non-POSIX declarations on NetBSD. */
#ifndef _NETBSD_SOURCE
# undef _NETBSD_SOURCE
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# undef _POSIX_PTHREAD_SEMANTICS
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# undef _TANDEM_SOURCE
#endif
])
    AC_DEFINE(_GNU_SOURCE)
    AC_DEFINE(_DARWIN_C_SOURCE)
    AC_DEFINE(_NETBSD_SOURCE)
    AC_DEFINE(_POSIX_PTHREAD_SEMANTICS)
    AC_DEFINE(_TANDEM_SOURCE)
  ])
])

AC_DEFUN([PIKE_USE_SYSTEM_EXTENSIONS],
[
  dnl Autoconf default extensions macro.
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])dnl

  AH_VERBATIM([USE_POSIX_C_EXTENSIONS],
[
#ifndef POSIX_SOURCE
 /* We must define this *always* */
# define POSIX_SOURCE	1
#endif
#ifndef _POSIX_C_SOURCE
  /* Version of POSIX that we want to support.
   * Note that POSIX.1-2001 and later require C99, and the earlier
   * require C89.
   *	undef		Not POSIX.
   *	1		POSIX.1-1990
   *	2		POSIX.2-1992
   *	199309L		POSIX.1b-1993 (Real Time)
   *	199506L		POSIX.1c-1995 (POSIX Threads)
   *	200112L		POSIX.1-2001 (Austin Group Revision)
   *	200809L		POSIX.1-2008
   */
# undef _POSIX_C_SOURCE
#endif
#ifndef _XOPEN_SOURCE
  /* Version of XPG that we want to support.
   * Note that this interacts with _POSIX_C_SOURCE above.
   *	undef		Not XPG (X Open Group).
   *	1		XPG 3 or 4 or 4v2 (see below).
   *	500		XPG 5 (POSIX.1c-1995).
   *	600		XPG 6 (POSIX.1-2001).
   *	700		XPG 7 (POSIX.1-2008).
   */
# undef _XOPEN_SOURCE

# if defined(_XOPEN_SOURCE) && ((_XOPEN_SOURCE + 0) < 500)
   /* Define to 4 for XPG 4. NB: Overrides _XOPEN_SOURCE_EXTENDED below. */
#  undef _XOPEN_VERSION

   /* Define to 1 (and do NOT define _XOPEN_VERSION) for XPG 4v2. */
#  undef _XOPEN_SOURCE_EXTENDED
# endif
#endif
#ifndef _DARWIN_C_SOURCE
#undef _DARWIN_C_SOURCE
#endif
#ifndef _NETBSD_SOURCE
#undef _NETBSD_SOURCE
#endif
#ifndef __BSD_VISIBLE
#undef __BSD_VISIBLE
#endif
])

  AC_DEFINE(POSIX_SOURCE, 1)

  AC_MSG_CHECKING(for level of POSIX to support)
  AC_CACHE_VAL(pike_cv_posix_c_source, [
    ORIG_CPPFLAGS="$CPPFLAGS"
    # NB: Older Solaris fails on attempts to enable newer POSIX
    #     than supported.
    for pike_cv_posix_c_source in 200809L 200112L 199506L 199309L 2 1; do
      CPPFLAGS="$ORIG_CPPFLAGS -D_POSIX_C_SOURCE=$pike_cv_posix_c_source"
      AC_TRY_CPP([
#include <stdio.h>
#include <stdlib.h>
      ],break;)
    done
    CPPFLAGS="$ORIG_CPPFLAGS"
  ])
  AC_MSG_RESULT($pike_cv_posix_c_source)
  AC_DEFINE_UNQUOTED(_POSIX_C_SOURCE, $pike_cv_posix_c_source)

  # NB: _XOPEN_SOURCE overrides _POSIX_C_SOURCE on FreeBSD 10.3.
  if test "$pike_cv_posix_c_source" = "200809L"; then
    AC_DEFINE(_XOPEN_SOURCE, 700)
  elif test "$pike_cv_posix_c_source" = "200112L"; then
    AC_DEFINE(_XOPEN_SOURCE, 600)
  elif test "$pike_cv_posix_c_source" = "199506L"; then
    AC_DEFINE(_XOPEN_SOURCE, 500)
  else
    # Attempt XPG 4v2.
    AC_DEFINE(_XOPEN_SOURCE, 1)
    AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1)
  fi

  AC_DEFINE(_DARWIN_C_SOURCE)
  AC_DEFINE(_NETBSD_SOURCE)
  # Enable non-POSIX types in <sys/types.h> on FreeBSD 10.3.
  AC_DEFINE(__BSD_VISIBLE, 1)
])

dnl Use before the first AC_CHECK_HEADER/AC_CHECK_FUNC call if the
dnl proper declarations are required to test function presence in
dnl AC_CHECK_FUNC. Necessary on Windows since various attributes cause
dnl name mangling there (e.g. __stdcall and __declspec(dllimport)).
dnl
dnl This test method is more correct (it e.g. detects functions that
dnl are implemented as macros) than the standard AC_CHECK_FUNC
dnl implementation and could perhaps be used all the time. It's
dnl however more fragile since AC_CHECK_HEADER must check all
dnl candidate headers before AC_CHECK_FUNC is used, and there might be
dnl conflicts between headers that can't be used simultaneously. It
dnl can also be quite a bit slower than the standard autoconf method.
AC_DEFUN([PIKE_FUNCS_NEED_DECLS],
[
  test "x$1" != x && pike_cv_funcs_need_decls="$1"
  if test "x$pike_cv_funcs_need_decls" = xyes; then
    cat > hdrlist.h <<EOF
/* Some C89 header files. */
#include <math.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

EOF
  fi
])

pushdef([AC_CONFIG_HEADER],
[
  CONFIG_HEADERS="$1"
  popdef([AC_CONFIG_HEADER])
  AC_CONFIG_HEADER($1)
])

# The CHECK_HEADERS_ONCE macro gets broken if CHECK_HEADER gets redefined.
# We redefine it to do an ordinary CHECK_HEADERS.
ifdef([AC_CHECK_HEADERS_ONCE],[
  define([ORIG_AC_CHECK_HEADERS_ONCE], defn([AC_CHECK_HEADERS_ONCE]))
  AC_DEFUN([AC_CHECK_HEADERS_ONCE],[
    dnl ORIG_AC_CHECK_HEADERS_ONCE([$1])
    AC_CHECK_HEADERS([$1])
  ])
])

define([ORIG_AC_CHECK_HEADER], defn([AC_CHECK_HEADER]))
AC_DEFUN([AC_CHECK_HEADER],
[
  AC_REQUIRE([PIKE_FUNCS_NEED_DECLS])
  ORIG_AC_CHECK_HEADER([$1], [
    if test x$pike_cv_funcs_need_decls = xyes; then
      def=HAVE_`echo "$1" | tr '[[a-z]]' '[[A-Z]]' | sed -e 's,[[-./]],_,g'`
      cat >> hdrlist.h <<EOF
#ifdef $def
#include <$1>
#endif
EOF
    fi
    $2
  ], [$3], [$4])
])

AC_DEFUN([PIKE_CHECK_GNU_STUBS_H],[
  AC_CHECK_HEADERS([gnu/stubs.h])
])

# Note: Check headers with AC_CHECK_HEADERS before using this one; see
# blurb at PIKE_FUNCS_NEED_DECLS.
define([ORIG_AC_CHECK_FUNC], defn([AC_CHECK_FUNC]))
AC_DEFUN([AC_CHECK_FUNC],
[AC_REQUIRE([PIKE_CHECK_GNU_STUBS_H])dnl
AC_REQUIRE([PIKE_FUNCS_NEED_DECLS])
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_func_$1,
[
  if test x$pike_cv_funcs_need_decls = xyes; then
    AC_TRY_LINK([
#include "hdrlist.h"
#ifndef $1
void *f = (void *) $1;
#endif
], [
#ifdef $1
/* If there's a macro with this name we assume it takes the place of the function. */
return 0;
#else
return f != $1;
#endif
], eval "ac_cv_func_$1=yes", eval "ac_cv_func_$1=no")
  else
    AC_TRY_LINK([
#ifdef HAVE_GNU_STUBS_H
/* This file contains __stub_ defines for broken functions. */
#include <gnu/stubs.h>
#else
/* Define $1 to an innocuous variant, in case <limits.h> declares $1.
   For example, HP-UX 11i <limits.h> declares gettimeofday.  */
#define $1 innocuous_$1

/* System header to define __stub macros and hopefully few prototypes,
    which can conflict with char $1 (); below.
    Prefer <limits.h> to <assert.h> if __STDC__ is defined, since
    <limits.h> exists even on freestanding compilers.  */

#ifdef __STDC__
# include <limits.h>
#else
# include <assert.h>
#endif

#undef $1
#endif

/* Override any gcc2 internal prototype to avoid an error.  */
#ifdef __cplusplus
extern "C"
{
#endif
/* We use char because int might match the return type of a gcc2
   builtin and then its argument prototype would still apply.  */
char $1 ();
/* The GNU C library defines this for functions which it implements
    to always fail with ENOSYS.  Some functions are actually named
    something starting with __ and the normal name is an alias.  */
#if defined (__stub_$1) || defined (__stub___$1)
choke me
#else
char (*f) () = $1;
#endif
#ifdef __cplusplus
}
#endif
    ], [return f != $1;], eval "ac_cv_func_$1=yes", eval "ac_cv_func_$1=no")
  fi
])
if eval "test \"`echo '$ac_cv_func_'$1`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi
])

AC_DEFUN([PIKE_IS_CROSS_COMPILING], [[${ac_cv_prog_cc_cross:-${cross_compiling}}]])
AC_DEFUN([PIKE_OVERRIDE_CROSS_COMPILING], [[
if [ -n "${ac_cv_prog_cc_cross}" -o -z "${cross_compiling}" ]; then
ac_cv_prog_cc_cross=$1; else cross_compiling=$1; fi
]])

dnl PIKE_SEARCH_LIBS(FUNCTION, CALL-CODE, SEARCH-LIBS,
dnl                  [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
dnl                  [OTHER-LIBRARIES])
dnl
dnl This is like AC_SEARCH_LIBS except that the added argument
dnl CALL-CODE is a complete code snippet that should make a typewise
dnl correct call to the function. It can be necessary in some cases
dnl where we have name mangling, e.g. in Windows libs that use
dnl __stdcall.
dnl
dnl Only applicable if PIKE_FUNCS_NEED_DECLS is enabled, otherwise it
dnl just wraps AC_SEARCH_LIBS.
define([PIKE_SEARCH_LIBS], [
  if test x$pike_cv_funcs_need_decls = xyes; then
    dnl FIXME: Should use AC_LANG_CONFTEST and AC_LINK_IFELSE available
    dnl in more modern autoconfs.
    AC_MSG_CHECKING([for library containing $1])
    AC_CACHE_VAL(pike_cv_search_$1, [
      pike_lib_search_save_LIBS=$LIBS
      test -f hdrlist.h || echo > hdrlist.h
      for pike_lib in '' $3; do
	if test -z "$pike_lib"; then
	  pike_cv_search_$1="none required"
	else
	  pike_cv_search_$1=-l$pike_lib
	  LIBS="-l$pike_lib $6 $pike_lib_search_save_LIBS"
	fi
	AC_TRY_LINK([
#include "hdrlist.h"], [
$2], break, pike_cv_search_$1=no)
      done
      LIBS=$pike_lib_search_save_LIBS
    ])
    if test "$pike_cv_search_$1" = no; then
      AC_MSG_RESULT(no)
      $5
    else
      AC_MSG_RESULT($pike_cv_search_$1)
      test "$pike_cv_search_$1" = "none required" || LIBS="$pike_cv_search_$1 $LIBS"
      $4
    fi
  else
    AC_SEARCH_LIBS([$1],[$3],[
      dnl Since ACTION-IF-FOUND should refer to the pike_cv_search_* variable
      dnl rather than to the ac_cv_search_* variable.
      pike_cv_search_$1=$ac_cv_search_$1
      $4],[$5],[$6])
  fi
])

AC_DEFUN([PIKE_AC_CC_IS_RNT],
[
  if test "x$pike_cc_is_rnt" = "x"; then
    if echo foo "$CC" | $EGREP 'rntc.|rnt.cl' >/dev/null; then
      pike_cc_is_rnt=yes
    else
      pike_cc_is_rnt=no
    fi
  fi
])

AC_DEFUN([PIKE_REQUIRE_AC_CC_IS_RNT], [AC_REQUIRE([PIKE_AC_CC_IS_RNT])])

define([ORIG_AC_CHECK_SIZEOF], defn([AC_CHECK_SIZEOF]))
pushdef([AC_CHECK_SIZEOF],
[
  changequote(<<, >>)dnl
  define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
  changequote([, ])dnl
  PIKE_REQUIRE_AC_CC_IS_RNT()dnl
  if test x"PIKE_IS_CROSS_COMPILING" = x"yes" -o "x$pike_cc_is_rnt" = "xyes"; then
    AC_MSG_CHECKING(size of $1 ... crosscompiling or rnt)
    AC_CACHE_VAL(AC_CV_NAME,[
      cat > conftest.$ac_ext <<EOF
dnl This sometimes fails to find confdefs.h, for some reason.
dnl [#]line __oline__ "[$]0"
[#]line __oline__ "configure"
#include "confdefs.h"
[$3]

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
    AC_CV_NAME=ifelse([$2], , 0, [$2])
  fi
  undefine([AC_CV_NAME])dnl
  ORIG_AC_CHECK_SIZEOF([$1],[$2],[$3])
])

m4_copy([AC_CHECK_HEADERS], [ORIG_CHECK_HEADERS])
ifdef([_AC_CHECK_HEADERS],
	[m4_copy([_AC_CHECK_HEADERS], [_ORIG_CHECK_HEADERS])])
define([AC_CHECK_HEADERS],
[
  if test "x$enable_binary" != "xno"; then
    ORIG_CHECK_HEADERS($1,$2,$3,$4)
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


dnl PIKE_AC_CHECK_OS()
dnl
dnl Check the operating system
AC_DEFUN(PIKE_AC_CHECK_OS, [
  AC_PATH_PROG(uname_prog,uname,no)
  AC_MSG_CHECKING(operating system)
  AC_CACHE_VAL(pike_cv_sys_os, [
    if test "PIKE_IS_CROSS_COMPILING" = "yes"; then
      case "$host_alias" in
	*amigaos*)	pike_cv_sys_os="AmigaOS";;
	*linux*)	pike_cv_sys_os="Linux";;
	*solaris*)	pike_cv_sys_os="Solaris";;
	*sunos*)	pike_cv_sys_os="SunOS";;
	*windows*)	pike_cv_sys_os="Windows_NT";;
	*mingw*|*MINGW*)
			pike_cv_sys_os="Windows_NT"
			pike_cv_is_mingw="yes";;
	*)		pike_cv_sys_os="Unknown";;
      esac
    elif test "$uname_prog" != "no"; then
      # uname on UNICOS doesn't work like other people's uname...
      if getconf CRAY_RELEASE >/dev/null 2>&1; then
	pike_cv_sys_os="UNICOS"
      else
	pike_cv_sys_os="`uname`"
      fi

      case "$pike_cv_sys_os" in
	SunOS)
	  case "`uname -r`" in
	    5.*) pike_cv_sys_os="Solaris" ;;
	  esac
	  ;;
	Monterey64)
	  # According to the release notes, the string "Monterey64"
	  # will be changed to "AIX" in the final release.
	  # (Monterey 64 is also known as AIX 5L).
	  pike_cv_sys_os="AIX"
	;;
	*Windows*|*windows*)
	  pike_cv_sys_os="Windows_NT"
	;;
	*MINGW*|*mingw*)
	  pike_cv_is_mingw="yes"
	  pike_cv_sys_os="Windows_NT"
	;;
      esac
    else
      pike_cv_sys_os="Not Solaris"
    fi
  ])
  AC_MSG_RESULT($pike_cv_sys_os)
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
  PIKE_FEATURE([$1],ifelse([$2], ,[yes],[$2]))
  if test x"$MODULE_NAME" = "x[$2]"; then
    MODULE_OK=yes
  fi
])


define([PIKE_RETAIN_VARIABLES],
[
  if test -f propagated_variables; then
    # Retain values for propagated variables; see make_variables.in.
    sed -e 's/'"'"'/'"'"'"'"'"'"'"'"'/g' -e 's/^\([[^=]]*\)=\(.*\)$/\1=${\1='"'"'\2'"'"'};export \1/' < propagated_variables > propvars.sh
    . ./propvars.sh && rm propvars.sh
  fi

  # This allows module configure scripts to extend these variables.
  CFLAGS=$BASE_CFLAGS
  CXXFLAGS=$BASE_CXXFLAGS
  CPPFLAGS=$BASE_CPPFLAGS
  LDFLAGS=$BASE_LDFLAGS

  # Since BASE_LDFLAGS contains the libs too, we start with an empty LIBS.
  LIBS=

  # Make these known under their old configure script names.
  BUILDDIR=$TMP_BUILDDIR
  BINDIR=$TMP_BINDIR
])


define([AC_LOW_MODULE_INIT],
[
  
  MY_AC_PROG_CC

  AC_PROG_EGREP

  PIKE_USE_SYSTEM_EXTENSIONS

  PIKE_SELECT_ABI

  dnl The following shouldn't be necessary; it comes from the core
  dnl machine.h via global.h anyway. Defining it here makes the
  dnl compiler complain about redefinition.
  dnl AC_DEFINE([POSIX_SOURCE], [], [This should always be defined.])

  AC_SUBST(CONFIG_HEADERS)

  AC_SUBST_FILE(dependencies)
  dependencies=$srcdir/dependencies

  AC_SUBST_FILE(dynamic_module_makefile)
  AC_SUBST_FILE(static_module_makefile)

  AC_ARG_WITH(root, MY_DESCR([--with-root=path],[specify a cross-compilation root-directory]),[
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
    CROSS="PIKE_IS_CROSS_COMPILING"
  fi
  AC_SUBST(CROSS)

  if test "x$enable_binary" = "xno"; then
    RUNPIKE="USE_PIKE"
  else
    if test x"PIKE_IS_CROSS_COMPILING" = x"yes"; then
      RUNPIKE="USE_PIKE"
    else
      RUNPIKE="DEFAULT_RUNPIKE"
    fi
  fi
  AC_SUBST(RUNPIKE)

  PIKE_CHECK_LIBGCC
])


dnl module_name
define([AC_MODULE_INIT],
[
  PIKE_RETAIN_VARIABLES()

  # Initialize the MODULE_{NAME,PATH,DIR} variables
  #
  # MODULE_NAME	Name of module as available from Pike.
  # MODULE_PATH	Module indexing path including the last '.'.
  # MODULE_DIR	Directory where the module should be installed
  #		including the last '/'.
  # MODULE_OK	Initialized to 'no'. Set to 'yes' by FEATURE_OK.
  #		Checked against with at OUTPUT.
  ifelse([$1], , [
    MODULE_NAME="`pwd|sed -e 's@.*/@@g'`"
    MODULE_PATH=""
    MODULE_DIR=""
  ], [
    MODULE_NAME="regexp([$1], [\([^\.]*\)$], [\1])"
    MODULE_PATH="regexp([$1], [\(.*\.\)*], [\&])"
    MODULE_DIR="patsubst(regexp([$1], [\(.*\.\)*], [\&]), [\.], [\.pmod/])"
  ])
  MODULE_OK=no

  AC_SUBST(MODULE_NAME)dnl
  AC_SUBST(MODULE_PATH)dnl
  AC_SUBST(MODULE_DIR)dnl

  echo
  echo '###################################################'
  echo '## Configuring module:' "$MODULE_PATH$MODULE_NAME"
  echo '## Installation dir:  ' "$MODULE_DIR"
  echo

  AC_LOW_MODULE_INIT()
  PIKE_FEATURE_CLEAR()

  if (test -f "$srcdir/module.pmod.in" || test -d "$srcdir/module.pmod.in"); then
    MODULE_PMOD_IN="$srcdir/module.pmod.in"
    MODULE_WRAPPER_PREFIX="___"
  else
    MODULE_PMOD_IN=""
    MODULE_WRAPPER_PREFIX=""
  fi
  AC_SUBST(MODULE_PMOD_IN)
  AC_SUBST(MODULE_WRAPPER_PREFIX)

  if test -d $BUILD_BASE/modules/. ; then
    dynamic_module_makefile=$BUILD_BASE/modules/dynamic_module_makefile
    static_module_makefile=$BUILD_BASE/modules/static_module_makefile
  else
    dynamic_module_makefile=$BUILD_BASE/dynamic_module_makefile
    static_module_makefile=$BUILD_BASE/dynamic_module_makefile
  fi

  PIKE_AC_CHECK_OS()

  if test x"$pike_cv_sys_os" = xWindows_NT ; then
    PIKE_FUNCS_NEED_DECLS(yes)
  fi
])

pushdef([AC_OUTPUT],
[
  AC_SET_MAKE

  AC_CONFIG_FILES([stamp-h], [echo foo >stamp-h])

  PMOD_TARGETS=`echo $srcdir/*.cmod | sed -e "s/\.cmod/\.c/g" | sed -e "s|$srcdir/|\\$(SRCDIR)/|g"`
  test "$PMOD_TARGETS" = '$(SRCDIR)/*.c' && PMOD_TARGETS=
  AC_SUBST(PMOD_TARGETS)

  AC_MSG_CHECKING([for the Pike base directory])
  if test "x$PIKE_SRC_DIR" != "x" -a -f "${PIKE_SRC_DIR}/make_variables.in"; then
    make_variables_in="${PIKE_SRC_DIR}/make_variables.in"
    make_variables_in_file=$make_variables_in
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
    make_variables_in_file=$make_variables_in
    AC_MSG_RESULT(${uplevels}.)
    PIKE_SRC_DIR="`cd $srcdir/${uplevels} && pwd`"
  fi

  AC_SUBST(make_variables_in)

  rm propagated_variables.new 2>/dev/null
  prop_var_changes=""
  for var in `sed -n -e 's/^#propagated_variables:\(.*\)$/\1/p' < $make_variables_in_file`; do
    # NB: If you get a strange error here you probably got a bogus
    # value in IFS.
    eval export $var
    eval echo \"${var}=\$$var\" >> propagated_variables.new
  done

  propvar_diff=yes
  if test -f propagated_variables.old; then
    dnl Compare to propagated_variables.old if there is any. This is
    dnl necessary to discover variable changes when our
    dnl propagated_variables has been updated by a parent configure
    dnl script.
    cmp -s propagated_variables.new propagated_variables.old && propvar_diff=no
    rm propagated_variables.old
  elif test -f propagated_variables; then
    cmp -s propagated_variables propagated_variables.new && propvar_diff=no
  fi

  if test $propvar_diff = no; then
    rm propagated_variables.new
  else
    mv propagated_variables.new propagated_variables
    dnl The propagated variables have changed so we need to propagate
    dnl them again. This is initially done via direct recursion in
    dnl AC_OUTPUT, but recursion is disabled when rechecking so we
    dnl need to explicitly propagate them in that case.
    if test "x$subdirs" != x; then
      for subdir in $subdirs; do
	if test -f "$subdir/config.status"; then
	  echo "$as_me: creating $subdir/propagated_variables (propagated variables have changed)" >&AC_FD_MSG
	  test -f "$subdir/propagated_variables" && mv "$subdir/propagated_variables" "$subdir/propagated_variables.old"
	  cp propagated_variables "$subdir"
	fi
      done
    fi
  fi

  AC_SUBST_FILE(propagated_variables)
  propagated_variables=propagated_variables

  AC_SUBST_FILE(make_variables)
  make_variables=make_variables

  ACCONFIG_H=""
  if test -f "$srcdir/acconfig.h"; then
    ACCONFIG_H='$(SRCDIR)/acconfig.h'
  fi
  AC_SUBST(ACCONFIG_H)

  CONFIG_HEADERS_IN=""
  if test "x$CONFIG_HEADERS" = "x"; then :; else
    for h in $CONFIG_HEADERS; do
      CONFIG_HEADERS_IN="$CONFIG_HEADERS_IN \$(SRCDIR)/$h.in"
    done
  fi
  AC_SUBST(CONFIG_HEADERS_IN)

  dnl Assert that there are configure-scripts in the subdirectories.
  if test "x$subdirs" != x; then
    for subdir in $subdirs; do
      if test -f "$srcdir/$subdir/configure"; then :; else
	echo "$as_me: no configurescript in $srcdir/$subdir/" >&AC_FD_MSG
	echo "$as_me: running ${PIKE_SRC_DIR}/run_autoconfig $srcdir/$subdir" >&AC_FD_MSG
	"${PIKE_SRC_DIR}/run_autoconfig" "$srcdir/${subdir}" >&AC_FD_MSG 2>&AC_FD_MSG
      fi
    done
  fi

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
    unset ac_cv_env_CXX_set
    unset ac_cv_env_CXX_value
    unset ac_cv_env_CXXCPP_set
    unset ac_cv_env_CXXCPP_value
    unset ac_cv_env_CXXFLAGS_set
    unset ac_cv_env_CXXFLAGS_value
    unset ac_cv_env_LDFLAGS_set
    unset ac_cv_env_LDFLAGS_value
    unset ac_cv_env_LIBS_set
    unset ac_cv_env_LIBS_value
    unset ac_cv_env_CPPFLAGS_set
    unset ac_cv_env_CPPFLAGS_value
    unset ac_cv_env_CPP_set
    unset ac_cv_env_CPP_value
  ])

  if test x"$MODULE_OK:$REQUIRE_MODULE_OK" = "xno:yes"; then
    AC_ERROR([Required module failed to configure.])
  fi

  popdef([AC_OUTPUT])
  AC_OUTPUT([make_variables:$make_variables_in $1],[$2],[$3])
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
      echo $ac_n "crosscompiling... $ac_c" 1>&AC_FD_MSG
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
    # The following test (and probably many more) will leak a bogus
    # value in IFS if the last argument is empty. Observed with
    # autoconf 2.59.
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
    AC_TRY_COMPILE([
#define CONFIGURE_TEST
#include "global.h"
#include "pike_int_types.h"
    ], [
$1 tmp;
    ], [
      found=no
      for mod in $2 ; do
	AC_TRY_RUN([
#define CONFIGURE_TEST
#include "global.h"
#include "pike_int_types.h"

#include <stddef.h>
#include <stdio.h>

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
}], [pike_cv_printf_$1="${mod}"; found=yes], [:], [:])
	test ${found} = yes && break
      done
      test ${found} = no && pike_cv_printf_$1=unknown
    ], [
      pike_cv_printf_$1=nonexistent
    ])
  ])
  if test x"${pike_cv_printf_$1}" = xnonexistent; then
    AC_MSG_RESULT([type does not exist])
  else
    if test x"${pike_cv_printf_$1}" = xunknown ; then
      res=$3
      AC_MSG_RESULT([none found, defaulting to $5])
    else
      res="${pike_cv_printf_$1}"
      AC_MSG_RESULT([$5])
    fi
    AC_DEFINE_UNQUOTED($4, "${res}")
  fi
])

dnl MY_AC_CHECK_PRINTF_FLOAT_TYPE(type, alternatives, default, define, result message)
define(MY_AC_CHECK_PRINTF_FLOAT_TYPE, [
  AC_MSG_CHECKING(how to printf $1)
  AC_CACHE_VAL(pike_cv_printf_$1, [
    AC_TRY_COMPILE([
#define CONFIGURE_TEST
#include "global.h"
    ], [
$1 tmp;
    ], [
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
}], [pike_cv_printf_$1="${mod}"; found=yes], [:], [:])
	test ${found} = yes && break
      done
      test ${found} = no && pike_cv_printf_$1=unknown
    ], [
      pike_cv_printf_$1=nonexistent
    ])
  ])
  if test x"${pike_cv_printf_$1}" = xnonexistent; then
    AC_MSG_RESULT([type does not exist])
  else
    if test x"${pike_cv_printf_$1}" = xunknown ; then
      res=$3
      AC_MSG_RESULT([none found, defaulting to $5])
    else
      res="${pike_cv_printf_$1}"
      AC_MSG_RESULT([$5])
    fi
    AC_DEFINE_UNQUOTED($4, "${res}")
  fi
])

dnl PIKE_MSG_WARN(message) 
dnl == AC_MSG_WARN but prints with a bit more emphasis and adds to 
dnl    config.warnings.
define(PIKE_MSG_WARN, [
  AC_MSG_WARN([

$1
])
  cat >>config.warnings <<EOF
WARNING: $1

EOF
])

dnl PIKE_MSG_NOTE(message) 
dnl == AC_MSG_RESULT, but more noticable and adds to config.notes.
define(PIKE_MSG_NOTE, [
  AC_MSG_RESULT([
NOTE: $1
])
  cat >>config.notes <<EOF
NOTE: $1

EOF
])

#############################################################################

AC_DEFUN(AC_SYS_COMPILER_FLAG_REQUIREMENTS,
[
  # The following are needed to detect broken handling of __STDC__ == 1
  # on Solaris with old header files.
  AC_CHECK_HEADER(sys/types.h)
  AC_CHECK_SIZEOF(off64_t, 0)
])

# option, cache_name, variable, do_if_failed, do_if_ok, paranoia_test
AC_DEFUN(AC_SYS_COMPILER_FLAG,
[
  dnl AC_REQUIRE(AC_SYS_COMPILER_FLAG_REQUIREMENTS)

  AC_MSG_CHECKING($1)
  if test "x[$]pike_disabled_option_$2" = "xyes"; then
    AC_MSG_RESULT(disabled)
    $4
  else
    AC_CACHE_VAL(pike_cv_option_$2,
    [
      OLD_CPPFLAGS="[$]CPPFLAGS"
      CPPFLAGS="[$]OLD_CPPFLAGS $1"
      old_ac_link="[$]ac_link"
      ac_link="[$]old_ac_link 2>conftezt.out.2"
      AC_TRY_LINK([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#if SIZEOF_OFF64_T != 0
        /* Make sure that __STDC__ doesn't get set to 1
         * on Solaris with old headers.
         */
        off64_t off64_value = (off64_t)17;
#endif
#endif
        int foo;
        int bar(int argc, char **argv)
        {
          /* The following code triggs gcc:s generation of aline opcodes,
           * which some versions of as does not support.
           */
	  if (argc > 0) argc = 0;
	  return argc;
        }
      ],[
        return bar(0, (void *)0);
      ],pike_cv_option_$2=yes,
        pike_cv_option_$2=no)
      if grep -i 'unrecognized option' <conftezt.out.2 >/dev/null; then
        pike_cv_option_$2=no
      elif grep -i 'unknown option' <conftezt.out.2 >/dev/null; then
        # cc/HPUX says the following regarding -q64:
        #
        # cc: warning 422: Unknown option "6" ignored.
        # cc: warning 422: Unknown option "4" ignored.
        pike_cv_option_$2=no
      elif grep -i 'optimizer bugs' <conftezt.out.2 >/dev/null; then
        # gcc/FreeBSD-4.6/alpha says the following regarding -O2:
        #
        # cc1: warning: 
        # ***
        # ***     The -O2 flag TRIGGERS KNOWN OPTIMIZER BUGS ON THIS PLATFORM
        # ***
        pike_cv_option_$2=no
      elif grep -i 'not found' <conftezt.out.2 >/dev/null; then
        # cc/AIX says the following regarding +O3:
        #
        # cc: 1501-228 input file +O3 not found
        pike_cv_option_$2=no
      elif grep -i 'ignored' <conftezt.out.2 >/dev/null; then
        # gcc/AIX says the following regarding -fpic:
        #
        # cc1: warning: -fpic ignored (all code is position independent)
        pike_cv_option_$2=no
      elif grep -i 'ignoring option'  <conftezt.out.2 >/dev/null; then
        # icc/Linux says the following regarding -Wcomment:
        #
        # icc: Command line warning: ignoring option '-W'; no argument required
        pike_cv_option_$2=no
      elif grep -i 'not supported' <conftezt.out.2 >/dev/null; then
        # icc/Linux says the following regarding -W:
        #
        # icc: Command line remark: option '-W' not supported
        pike_cv_option_$2=no
      elif grep -i 'illegal option' <conftezt.out.2 >/dev/null; then
        # cc/Solaris says the following regarding -xdepend:
        #
        # cc: Warning: illegal option -xdepend
        pike_cv_option_$2=no
      elif grep -i 'is deprecated' <conftezt.out.2 >/dev/null; then
        # cc/Solaris (SunStudio 12) says the following regarding
	# -xarch=generic64:
        #
        # cc: Warning: -xarch=generic64 is deprecated, use -m64 to create 64-bit programs
        pike_cv_option_$2=no
      elif grep -i 'argument unused' <conftezt.out.2 >/dev/null; then
        # clang says the following for -ggdb3:
        #
	# clang: warning: argument unused during compilation: '-ggdb3'
        pike_cv_option_$2=no
      else :; fi
      if test -f conftezt.out.2; then
        cat conftezt.out.2 >&AC_FD_CC
      fi
      ifelse([$6], , , [
      if test "$pike_cv_option_$2" = yes; then
        :
        $6
      fi
      ])dnl
      CPPFLAGS="[$]OLD_CPPFLAGS"
      ac_link="[$]old_ac_link"
      rm conftezt.out.2
    ])
    
    if test x"[$]pike_cv_option_$2" = "xyes" ; then
      $3="[$]$3 $1"
      # FIXME: We assume that the C++ compiler takes the same options
      #        as the C compiler.
      ifelse($3,CC,[CXX="[$]CXX $1"])
      ifelse($3,OPTIMIZE,[CFLAGS="[$]CFLAGS $1"])
      ifelse($3,OPTIMIZE,[CXXFLAGS="[$]CXXFLAGS $1"])
      AC_MSG_RESULT(yes)
      $5
    else
      AC_MSG_RESULT(no)
      $4
    fi
  fi
])

# arch, option, cache_name, variable
AC_DEFUN(AC_SYS_CPU_COMPILER_FLAG,
[
  if test "`uname -m 2>/dev/null`" = "$1" ; then
    AC_SYS_COMPILER_FLAG($2,$3,$4,$5,$6)
    $7
  fi
])

# os, option, cache_name, variable
AC_DEFUN(AC_SYS_OS_COMPILER_FLAG,
[
  if test "x$pike_cv_sys_os" = "x$1" ; then
    AC_SYS_COMPILER_FLAG($2,$3,$4,$5,$6)
    $7
  fi
])

define([DO_IF_OS],
[
  if test "x$pike_cv_sys_os" = "x$1" ; then
    $2
  fi
])

# ABI selection.

dnl variable, file-path
AC_DEFUN(PIKE_CHECK_FILE_ABI,
[
  PIKE_filetype=`file "$2" 2>/dev/null | sed -e 's/[[^:]]*://'`
  PIKE_filetype_L=`file -L "$2" 2>/dev/null | sed -e 's/[[^:]]*://'`
  case "[$]PIKE_filetype:[$]PIKE_filetype_L" in
    *64-bit*)
      $1=64
      ;;
    *32-bit*)
      $1=32
      ;;
    *64*)
      $1=64
      ;;
    *32*)
      $1=32
      ;;
    *386*)
      # Probably NT or SCO file for i386:
      #   iAPX 386 executable (COFF)
      #   80386 COFF executable
      $1=32
      ;;
    *ppc*)
      # Probably 32-bit MacOS X object file:
      #   Mach-O object ppc
      $1=32
      ;;
    *"POSIX shell script"*)
      # Shell scripts do not have an ABI
      $1=noarch
      ;;
    *)
      # Unknown. Probably cross-compiling.
      PIKE_MSG_WARN([Unrecognized object file format: $PIKE_filetype:$PIKE_filetype_L])
      if dd if="$2" count=2 bs=1 2>/dev/null | \
	grep 'L' >/dev/null; then
	# A common case is rntcl...
	# If the file begins with 0x4c 0x01 it's a 80386 COFF executable.
	$1=32
      fi
      ;;
  esac
])

dnl Check the default ABI of the compiler.
dnl
dnl Result: $pike_cv_default_compiler_abi (one of 32, 64 or unknown).
AC_DEFUN(PIKE_CHECK_DEFAULT_ABI,
[
  if test "x$ac_cv_objext" = "x"; then
    AC_MSG_CHECKING([object file extension])
    AC_CACHE_VAL(ac_cv_objext, [
      # In autoconf 2.13 it was named ac_objext.
      ac_cv_objext="$ac_objext"
    ])
    AC_MSG_RESULT($ac_cv_objext)
  fi
  AC_MSG_CHECKING([default compiler ABI])
  AC_CACHE_VAL(pike_cv_default_compiler_abi, [
    cat >"conftest.$ac_ext" <<\EOF
int main(int argc, char **argv)
{
  return 0;
}
EOF
    pike_cv_default_compiler_abi="unknown"
    if (eval $ac_compile) 2>&AC_FD_CC; then
      PIKE_CHECK_FILE_ABI(pike_cv_default_compiler_abi, conftest.$ac_cv_objext)
    fi
    rm -f conftest.$ac_cv_objext conftest.$ac_ext
  ])
  AC_MSG_RESULT($pike_cv_default_compiler_abi)
])

dnl Check the default ABI of the platform.
dnl This is typically the same as that of the compiler,
dnl but on some platforms (eg Solaris) it defaults to
dnl the legacy ABI.
dnl
dnl Result: $pike_cv_native_abi (one of 32, 64 or unknown).
AC_DEFUN(PIKE_CHECK_NATIVE_ABI,
[
  AC_REQUIRE([PIKE_CHECK_DEFAULT_ABI])dnl

  AC_MSG_CHECKING([what the native ABI is])

  # Default to the compiler default.
  pike_cv_native_abi="$pike_cv_default_compiler_abi"

  case "x`uname -m`" in
    x*64)
      pike_cv_native_abi="64"
      ;;
    xalpha)
      pike_cv_native_abi="64"
      ;;
  esac

  if type isainfo 2>/dev/null >/dev/null; then
    # Solaris and rntcl.
    pike_cv_native_abi="`isainfo -b`"
  elif type sysctl 2>/dev/null >/dev/null; then
    # MacOS X or Linux.
    #
    # On MacOS X hw.optional.64bitop is set to 1 if
    # 64bit is supported and useful.
    if test "`sysctl -n hw.optional.64bitops 2>/dev/null`" = "1"; then
      pike_cv_native_abi="64"
    fi
    # On MacOS X hw.cpu64bit_capable is set to 1 if
    # 64bit is supported and useful.
    if test "`sysctl -n hw.cpu64bit_capable 2>/dev/null`" = "1"; then
      pike_cv_wanted_abi="64"
    fi
  fi

  AC_MSG_RESULT($pike_cv_native_abi)
])

dnl Check the ABI wanted by the user.
dnl Defaults to the native ABI for the platform.
dnl
dnl Result: $pike_cv_wanted_abi (one of 32 or 64).
AC_DEFUN(PIKE_WITH_ABI,
[
  AC_REQUIRE([PIKE_CHECK_NATIVE_ABI])dnl

  AC_ARG_WITH(abi, MY_DESCR([--with-abi=32/64],
			    [specify ABI to use in case there are multiple]))

  AC_MSG_CHECKING([which ABI to use])
  AC_CACHE_VAL(pike_cv_wanted_abi, [
    case "x$with_abi" in
      *32)
        pike_cv_wanted_abi="32"
      ;;
      *64)
        pike_cv_wanted_abi="64"
      ;;
      *)
        # Defaults
        pike_cv_wanted_abi="$pike_cv_native_abi"
        if test "x$pike_cv_wanted_abi" = "xunknown"; then
          pike_cv_wanted_abi="32"
        fi
      ;;
    esac
  ])
  AC_MSG_RESULT(attempt $pike_cv_wanted_abi)
])

# if-fail-and-no-default
AC_DEFUN(PIKE_ATTEMPT_ABI32,
[
  #
  # We want 32bit mode if possible.
  #
  AC_SYS_COMPILER_FLAG(-q32, q32, CC)
  AC_SYS_COMPILER_FLAG(-m32, m32, CC)
  # Sun Studio 10
  AC_SYS_COMPILER_FLAG(-xtarget=generic32, xtarget_generic32, CC)
  AC_SYS_COMPILER_FLAG(-xarch=generic32, xarch_generic32, CC)
  if test "$pike_cv_option_q32:$pike_cv_option_m32:$pike_cv_option_xtarget_generic32:$pike_cv_option_xarch_generic32" = "no:no:no:no"; then
    if test "x$pike_cv_default_compiler_abi" = "xunknown"; then
      :
      $1
    else
      PIKE_MSG_WARN([Using compiler default ABI: $pike_cv_default_compiler_abi])
      pike_cv_abi="$pike_cv_default_compiler_abi"
    fi
  else
    pike_cv_abi="32"
  fi
])

# if-fail-and-no-default
AC_DEFUN(PIKE_ATTEMPT_ABI64,
[
  #
  # We want 64bit mode if possible.
  #
  AC_SYS_COMPILER_FLAG(-q64, q64, CC)
  AC_SYS_COMPILER_FLAG(-m64, m64, CC)
  # Sun Studio 10
  AC_SYS_COMPILER_FLAG(-xtarget=generic64, xtarget_generic64, CC)
  AC_SYS_COMPILER_FLAG(-xarch=generic64, xarch_generic64, CC)
  if test "$pike_cv_option_q64:$pike_cv_option_m64:$pike_cv_option_xtarget_generic64:$pike_cv_option_xarch_generic64" = "no:no:no:no"; then
    if test "x$pike_cv_default_compiler_abi" = "xunknown"; then
      :
      $1
    else
      PIKE_MSG_WARN([Using compiler default ABI: $pike_cv_default_compiler_abi])
      pike_cv_abi="$pike_cv_default_compiler_abi"
    fi
  else
    pike_cv_abi="64"
  fi
])

dnl Select the ABI to target.
AC_DEFUN(PIKE_SELECT_ABI,
[
  AC_REQUIRE([PIKE_CHECK_DEFAULT_ABI])dnl
  AC_REQUIRE([PIKE_WITH_ABI])dnl

  if test "x$pike_cv_wanted_abi" = "x$pike_cv_default_compiler_abi"; then
    # The compiler defaults to the wanted ABI.
    pike_cv_abi="$pike_cv_wanted_abi"
  else
    if test "x$pike_cv_wanted_abi" = "x64"; then
      PIKE_ATTEMPT_ABI64([
        PIKE_ATTEMPT_ABI32([
          PIKE_MSG_WARN([Found no option to force 64 bit ABI.])
          # We hope this is correct...
	  pike_cv_abi="64"
        ])
      ])
    else
      PIKE_ATTEMPT_ABI32([
        PIKE_ATTEMPT_ABI64([
          PIKE_MSG_WARN([Found no option to force 32 bit ABI.])
          # We hope this is correct...
	  pike_cv_abi="32"
        ])
      ])
    fi
  fi
  if test "x$pike_cv_abi" = "x32"; then
    #
    # Make sure no later tests will add -q64 or -m64.
    #
    pike_disabled_option_q64=yes
    pike_disabled_option_m64=yes
  fi

  echo
  echo "Using ABI $pike_cv_abi."
  echo

  # ABI-dirs
  AC_MSG_CHECKING(for ABI lib-suffixes)
  AC_CACHE_VAL(pike_cv_abi_suffixes,
  [
    extra_abi_dirs=""
    if type isainfo 2>/dev/null >/dev/null; then
      # Solaris
      # Some installations lack the symlink 64 -> amd64 or sparcv9,
      # or the corresponding 32 link.
      extra_abi_dirs=`isainfo -v 2>/dev/null|awk "/$pike_cv_abi"'-bit/ { print "/" [$]2 }'`
    fi
    if test "x`uname -p`" = "xpowerpc"; then
      # MacOS X
      # The 64-bit libraries are typically in the subdirectory ppc64.
      extra_abi_dirs="$extra_abi_dirs /ppc$pike_cv_abi"
    fi
    pike_cv_abi_suffixes="$pike_cv_abi /$pike_cv_abi $extra_abi_dirs /."
  ])
  AC_MSG_RESULT($pike_cv_abi_suffixes)

  # Prefix for pkg-config and other tools that don't support multiple ABIs
  # natively.
  if test "x$ac_tool_prefix" = x; then
    AC_MSG_CHECKING(For $pike_cv_abi-bit ABI tool prefix)
    AC_CACHE_VAL(pike_cv_tool_prefix, [
      SAVE_IFS="$IFS"
      IFS=":"
      file_abi=""
      for d in $PATH; do
	IFS="$SAVE_IFS"
	for f in "$d/"*-pkg-config"$exeext"; do
	  if test -f "$f"; then
	    PIKE_CHECK_FILE_ABI(file_abi, "$f")
	    if test "x$file_abi" = "x$pike_cv_abi"; then
	      pike_cv_tool_prefix=`echo "$f" | sed -e 's|.*/||g' -e 's|pkg-config.*||'`
	      break;
	    fi
	  fi
	done
        if test -h "$d/pkg-config" && readlink >/dev/null 2>&1 "$d/pkg-config"; then
          link_target=`readlink "$d/pkg-config"`
          case "$link_target" in
            /*) ;;
            *) link_target="$d/$link_target";;
          esac
          case "$link_target" in
            *-pkg-config"$exeext")
               PIKE_CHECK_FILE_ABI(file_abi, "$link_target")
               if test "x$file_abi" = "x$pike_cv_abi"; then
                 pike_cv_tool_prefix=`echo "$link_target" | sed -e 's|.*/||g' -e 's|pkg-config.*||'`
               fi;;
          esac
        fi
        if test "x$pike_cv_tool_prefix" = x; then :; else
	  break;
	fi
      done
      IFS="$SAVE_IFS"
    ])
    if test "x$pike_cv_tool_prefix" = "x"; then
      AC_MSG_RESULT(no)
    else
      AC_MSG_RESULT($pike_cv_tool_prefix)
    fi
  else
    pike_cv_tool_prefix="$ac_tool_prefix"
  fi

  # Compat
  with_abi="$pike_cv_abi"
])

# Directory searching

AC_DEFUN(PIKE_INIT_REAL_DIRS,
[
  real_dirs='/ /usr'
  real_incs='/include /usr/include'
])

# directory, if-true, if-false, real-variable(OBSOLETE)
AC_DEFUN(PIKE_CHECK_ABI_DIR,
[
  AC_REQUIRE([PIKE_SELECT_ABI])dnl
  AC_REQUIRE([PIKE_INIT_REAL_DIRS])dnl

  pike_file_follow_symlink_opt=""

  AC_MSG_CHECKING(whether $1 contains $pike_cv_abi-bit ABI files)
  abi_dir_ok="no"
  abi_dir_dynamic="unknown"
  real_dir="$1"
  while :; do
    if test -d "$1/." ; then :; else 
      AC_MSG_RESULT(no - does not exist)
      break
    fi
    cached="(cached) "
    real_dir=`cd "$1/." && /bin/pwd 2>/dev/null`
    if test "x$real_dir" = "x"; then
      cached="(pwd failed) "
      real_dir="$1"
    fi
    if echo " $pike_cv_32bit_dirs " | grep " $real_dir " >/dev/null; then
      abi_32=yes
    elif echo " $pike_cv_not_32bit_dir " | grep " $real_dir " >/dev/null; then
      abi_32=no
    else
      abi_32=unknown
    fi
    if echo " $pike_cv_64bit_dirs " | grep " $real_dir " >/dev/null; then
      abi_64=yes
    elif echo " $pike_cv_not_64bit_dirs " | grep " $real_dir " >/dev/null; then
      abi_64=no
    else
      abi_64=unknown
    fi
    if echo " $pike_cv_dynamic_dirs " | grep " $real_dir " >/dev/null; then
      abi_dir_dynamic=yes
    elif echo " $pike_cv_not_dynamic_dirs " | grep " $real_dir " >/dev/null; then
      abi_dir_dynamic=no
    else
      abi_dir_dynamic=unknown
    fi
    empty=no
    if echo "$abi_32:$abi_64:$abi_dir_dynamic" | \
	 grep "unknown" >/dev/null; then
      cached=""
      empty=yes
      for f in "$1"/* no; do
        if test -f "$f"; then
	  empty=no
	  # NB: GNU file and BSD file default to not following symlinks.
	  #	Solaris /bin/file does not understand -L. The following
	  #	should support most cases.
          filetype="`POSIXLY_CORRECT=yes file $pike_follow_symlink_opt $f 2>/dev/null`"

	  case "$filetype" in
	    *"symbolic link"*)
	      if file -L $f >/dev/null 2>&1; then
	        pike_file_follow_symbolic_link="-L";
		filetype="`POSIXLY_CORRECT=yes file -L $f 2>/dev/null`"
	      fi
	      ;;
	  esac

	  # NB: /bin/file on Solaris 11 says
	  #     "current ar archive, 32-bit symbol table"
	  #     about most archives. We don't want this
	  #     to be matched in the ABI check below.
	  filetype="`echo $filetype | sed -e 's/...bit symbol table//'`"

	  case "$filetype" in
            *executable*)
              continue;;
            *32-bit*)
  	      abi_32=yes
	      ;;
  	    *64-bit*)
  	      abi_64=yes
	      ;;
	    *32*)
  	      abi_32=yes
	      ;;
	    *64*)
  	      abi_64=yes
	      ;;
	    *386*)
  	      abi_32=yes
	      ;;
	    *ppc*)
              abi_32=yes
	      ;;
	    *archive*)
	      # Try looking deeper into the archive.
	      if type elffile 2>/dev/null >/dev/null; then
		# Solaris 11 or later.
		# Typical output:
		# i386: "current ar archive, 32-bit symbol table, ELF 32-bit LSB relocatable 80386 Version 1"
		# amd64: "current ar archive, 32-bit symbol table, ELF 64-bit LSB relocatable AMD64 Version 1"
	        filetype="`POSIXLY_CORRECT=yes elffile $f 2>/dev/null`"
	      elif type elfdump 2>/dev/null >/dev/null; then
		# Solaris 10 or earlier.
		# Typical output:
		# i386: "ei_class:   ELFCLASS32          ei_data:      ELFDATA2LSB" (repeated)
		# sparcv7: "ei_class:   ELFCLASS32          ei_data:      ELFDATA2MSB"
		# amd64: "ei_class:   ELFCLASS64          ei_data:      ELFDATA2LSB" (repeated)
		# sparc64: "ei_class:   ELFCLASS64          ei_data:      ELFDATA2MSB"
		filetype="`POSIXLY_CORRECT=yes elfdump -e $f 2>/dev/null | grep ELFCLASS`"
	      elif type readelf 2>/dev/null >/dev/null; then
		# GNU binutils.
		# Typical output:
		# x86: "Class:                             ELF32"
		# x86_64: "Class:                             ELF64"
		filetype="`readelf -h $f 2>/dev/null | grep -i class`"
	      fi
	      # NB: Avoid matching "32-bit symbol table".
	      filetype="`echo $filetype | sed -e 's/...bit symbol table//'`"
	      case "$filetype" in
		*ELFCLASS64*)
		  abi_64=yes
		  ;;
		*ELFCLASS32*)
		  abi_32=yes
		  ;;
		*ELF64*)
		  abi_64=yes
		  ;;
		*ELF32*)
		  abi_32=yes
		  ;;
		*64-bit*)
		  abi_64=yes
		  ;;
		*32-bit*)
		  abi_32=yes
		  ;;
		*64*)
		  abi_64=yes
		  ;;
		*32*)
		  abi_32=yes
		  ;;
	      esac
	      ;;
	  esac
	  case "$filetype" in
	    *not\ a\ dynamic*)
	      ;;
	    *not\ a\ shared*)
	      ;;
	    *dynamic*)
	      abi_dir_dynamic=yes
	      ;;
	    *shared*)
	      abi_dir_dynamic=yes
	      ;;
	  esac
	  if test "$abi_dir_dynamic" = "unknown"; then
	     continue;
	  elif test "$abi_64:$pike_cv_abi" = "yes:64" ; then
             break
          elif test "$abi_32:$pike_cv_abi" = "yes:32" ; then
             break
          fi
	elif test "x$empty" = "xyes"; then
	  empty=other
        fi
      done
      if test "$abi_32" = "yes"; then
        pike_cv_32bit_dirs="$pike_cv_32bit_dirs $real_dir"
	if test "$abi_64" = "unknown"; then
          abi_64="no"
	fi
      else
        pike_cv_not_32bit_dirs="$pike_cv_not_32bit_dirs $real_dir"
      fi
      if test "$abi_64" = "yes"; then
        pike_cv_64bit_dirs="$pike_cv_64bit_dirs $real_dir"
	if test "$abi_32" = "unknown"; then
          abi_32="no"
	fi
      elif test "$abi_64" = "no"; then
        pike_cv_not_64bit_dirs="$pike_cv_not_64bit_dirs $real_dir"
      fi
      if test "$abi_32" = "no"; then
        pike_cv_not_32bit_dirs="$pike_cv_not_32bit_dirs $real_dir"
      fi
      if test "$abi_dir_dynamic" = "yes"; then
	pike_cv_dynamic_dirs="$pike_cv_dynamic_dirs $real_dir"
      else
	pike_cv_not_dynamic_dirs="$pike_cv_not_dynamic_dirs $real_dir"
      fi
    fi
    if test "$abi_32:$pike_cv_abi" = "no:32" \
         -o "$abi_64:$pike_cv_abi" = "no:64" \
	 -o "x$empty" = "xyes"; then
      AC_MSG_RESULT([${cached}no, does not contain any $pike_cv_abi-bit ABI files])
    else
      abi_dir_ok="yes"
      AC_MSG_RESULT(${cached}ABI ok)
    fi
    break
  done
  if test "$abi_dir_ok" = "yes"; then
    ifelse([$2], , :, [$2])
  elif test "$abi_dir_ok" = "no"; then
    ifelse([$3], , :, [$3])
  fi
])

# directory, if-added, if-bad, if-already-added
AC_DEFUN(PIKE_CHECK_ABI_LIB_DIR,
[
  PIKE_CHECK_ABI_DIR($1, [
    AC_MSG_CHECKING([what to add to LDFLAGS])
    add_ldflags=""
    if echo " $LDFLAGS " | grep " -L$real_dir " >/dev/null; then :; else
      add_ldflags="-L$real_dir"
    fi
    if test "x$abi_dir_dynamic" = "xyes"; then
      if echo " $LDFLAGS " | grep " -R$real_dir " >/dev/null; then :; else
        add_ldflags="$add_ldflags -R$real_dir"
      fi
    fi
    if test "x$add_ldflags" = "x"; then
      AC_MSG_RESULT([nothing - already added])
      ifelse([$4], , :, [$4])
    else
      OLD_LDFLAGS="${LDFLAGS}"
      LDFLAGS="${LDFLAGS} $add_ldflags -lm"
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main(int argc, char **argv)
{
  double (*foo)(double) = ceil;
  exit(0);
}
        ],[ LDFLAGS="$OLD_LDFLAGS $add_ldflags"
    	    AC_MSG_RESULT($add_ldflags)
        ],[ LDFLAGS="$OLD_LDFLAGS"
    	    AC_MSG_RESULT(nothing - $add_ldflags causes failures)
            add_ldflags=""
        ],[AC_TRY_LINK([
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
    	   ],[
    	     double (*foo)(double) = ceil;
    	     exit(0);
    	   ],[ LDFLAGS="$OLD_LDFLAGS $add_ldflags"
    	       AC_MSG_RESULT($add_ldflags)
    	   ],[ LDFLAGS="$OLD_LDFLAGS"
    	       AC_MSG_RESULT(nothing - $add_ldflags causes failures)
               add_ldflags=""
        ])
      ])
      if test "x$add_ldflags" = "x"; then
        ifelse([$3], , :, [$3])
      else
        ifelse([$2], , :, [$2])
      fi
    fi
    AC_MSG_CHECKING([for pkgconfig directory])
    if test -d "$real_dir/pkgconfig/."; then
      PKG_CONFIG_PATH="$PKG_CONFIG_PATH${PKG_CONFIG_PATH:+:}$real_dir/pkgconfig"
      export PKG_CONFIG_PATH
      AC_MSG_RESULT(yes - $real_dir/pkgconfig)
    else
      AC_MSG_RESULT(no)
    fi
  ], $3)
])

AC_SUBST(PKG_CONFIG_PATH)
AC_SUBST(PKG_CONFIG_LIBDIR)

AC_DEFUN(PIKE_FIND_LIB_INCLUDE,
[
  AC_REQUIRE([PIKE_SELECT_ABI])dnl

  echo Searching for library and include directories...

  #Don't add include dirs if they give us warnings...
  OLD_ac_c_preproc_warn_flag="$ac_c_preproc_warn_flag"
  ac_c_preproc_warn_flag=yes
  AC_TRY_CPP([#include <stdio.h>], , [
    AC_MSG_WARN([Turning on preprocessor warnings causes cpp test to fail.])
    ac_c_preproc_warn_flag="$OLD_ac_c_preproc_warn_flag"
  ])
])

AC_DEFUN(PIKE_PROG_PKG_CONFIG,
[
  # NB: pkg-config does not have native support for multiple ABIs.
  MY_AC_PATH_PROGS(PKG_CONFIG, ${pike_cv_tool_prefix}pkg-config ${ac_tool_prefix}pkg-config, no)
  if test "$pike_cv_sys_os:$pike_cv_abi:$PKG_CONFIG:$PKG_CONFIG_LIBDIR" = \
          "Solaris:64:/usr/bin/pkg-config:"; then
    # Workaround for 32-bit only pkg-config on Solaris 11.
    AC_MSG_CHECKING([Value for PKG_CONFIG_LIBDIR])
    for d in $pike_cv_abi_suffixes; do
      if test "x$d" = "x/."; then continue; fi
      if test -d "/usr/lib$d/pkgconfig/."; then
	PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR/usr/lib$d/pkgconfig:"
      fi
      # Note: Some pkg-config files (eg nspr) for 64-bit OSes are erroneously
      #       installed in /usr/lib/pkgconfig/amd64/.
      if test -d "/usr/lib/pkgconfig$d/."; then
	PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR/usr/lib/pkgconfig$d:"
      fi
    done
    PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR/usr/share/pkgconfig"
    export PKG_CONFIG_LIBDIR
    AC_MSG_RESULT([$PKG_CONFIG_LIBDIR])
  fi
])

dnl package, variable, options
AC_DEFUN(PIKE_LOW_PKG_CONFIG,
[
  AC_MSG_CHECKING([for stuff to add to $2])
  pkg_stuff="`${PKG_CONFIG} $3 $1`"
  AC_MSG_RESULT(${pkg_stuff})
  $2="[$]$2 ${pkg_stuff}"
])

dnl package, on_success_opt, on_failure_opt
AC_DEFUN(PIKE_PKG_CONFIG,
[
  AC_REQUIRE([PIKE_PROG_PKG_CONFIG])dnl
  pike_cv_pkg_config_$1=no
  if test "${PKG_CONFIG}" = no; then :; else
    AC_MSG_CHECKING([if a pkg-config based $1 is installed])
    if "${PKG_CONFIG}" "$1"; then
      AC_MSG_RESULT(yes)
      PKG_SAVE_CPPFLAGS="$CPPFLAGS"
      PKG_SAVE_CFLAGS="$CFLAGS"
      PKG_SAVE_LDFLAGS="$LDFLAGS"
      PKG_SAVE_LIBS="$LIBS"
      PIKE_LOW_PKG_CONFIG([$1], [CPPFLAGS], [--cflags-only-I])
      PIKE_LOW_PKG_CONFIG([$1], [CFLAGS],   [--cflags-only-other])
      PIKE_LOW_PKG_CONFIG([$1], [LDFLAGS],  [--libs-only-L])
      PIKE_LOW_PKG_CONFIG([$1], [LIBS],     [--libs-only-l --libs-only-other])
      AC_MSG_CHECKING([if $1 breaks compilation...])
      AC_TRY_LINK([
#include <stdio.h>
#include <stdlib.h>
],[
        printf("Hello, world\n");
	exit(0);
      ],[
	AC_MSG_RESULT([no, everything seems ok])
	pike_cv_pkg_config_$1=yes
      ],[
	AC_MSG_RESULT([yes, do not use])
	CPPFLAGS="$PKG_SAVE_CPPFLAGS"
	CFLAGS="$PKG_SAVE_CFLAGS"
	LDFLAGS="$PKG_SAVE_LDFLAGS"
	LIBS="$PKG_SAVE_LIBS"
      ])
    else
      AC_MSG_RESULT(no)
    fi
  fi
  ifelse([$2$3], , , [
    if test "x$pike_cv_pkg_config_$1" = "xno"; then
      ifelse([$3], , :, [$3])
    else
      ifelse([$2], , :, [$2])
    fi
  ])
])

define([PIKE_PARSE_SITE_PREFIXES], [
p_site_prefixes_to_add=""
p_site_prefixes_to_remove=""
eval "set ignored $ac_configure_args"
shift
for p_option; do
  echo "Option: $p_option"
  case $p_option in
    --with-site-prefixes*)
      p_useropt=`expr "x$p_option" : 'x--with-site-prefixes=\(.*\)'`
      if test "x$p_useropt" != 'x'  ; then
        p_site_prefixes_to_add="${p_site_prefixes_to_add}:${p_useropt}"
      fi
    ;;

    --with-exclude-site-prefixes*)
      p_useropt=`expr "x$p_option" : 'x--with-exclude-site-prefixes=\(.*\)'`
      if test "x$p_useropt" != 'x' ; then
        p_site_prefixes_to_remove="${p_site_prefixes_to_remove}:${p_useropt}"
      fi
    ;;
  esac
done

m4_divert_once([HELP_WITH], [[
Optional Packages:
  --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]
  --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)]])
m4_divert_once([HELP_WITH], AS_HELP_STRING([--with-site-prefixes=path], [:-separated list of prefixes to search for include, lib and bin dirs.]))
m4_divert_once([HELP_WITH], AS_HELP_STRING([--with-exclude-site-prefixes=path], [:-separated list of prefixes to exclude for include, lib and bin dirs.]))
])

AC_DEFUN([PIKE_DIVERT_BEFORE_HELP],
[ifdef([m4_divert_text], [m4_divert_text([PARSE_ARGS],[$1])],
       [ifdef([AC_DIVERT], [AC_DIVERT([PARSE_ARGS],[$1])],[$2])])])

AC_DEFUN([PIKE_FOREACH_CONFIG_SUBDIR], [
  for $1 in `(cd $srcdir ; echo *)`
  do
    if test -d "$srcdir/[$]$1" ; then
      if test -f "$srcdir/[$]$1/configure.in"; then
        $2
      fi
    fi
  done
])

AC_DEFUN([PIKE_DYNAMIC_CONFIG_SUBDIRS], [PIKE_DIVERT_BEFORE_HELP([
  ac_subdirs_all=''
  PIKE_FOREACH_CONFIG_SUBDIR([a], [ac_subdirs_all="$ac_subdirs_all $a"])
])])
