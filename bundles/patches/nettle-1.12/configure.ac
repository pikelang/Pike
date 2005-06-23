dnl -*- mode: shell-script; sh-indentation: 2; -*-

dnl Process this file with autoconf to produce a configure script.

AC_INIT([nettle], [1.12], [nettle-bugs@lists.lysator.liu.se])
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR([arcfour.c])
# Needed to stop autoconf from looking for files in parent directories.
AC_CONFIG_AUX_DIR([.])

AC_CONFIG_HEADER([config.h])

SHLIBMAJOR=2
SHLIBMINOR=3

AC_CANONICAL_HOST

# Command line options
AC_ARG_WITH(include-path,
  AC_HELP_STRING([--with-include-path], [A colon-separated list of directories to search for include files]),,
  [with_include_path=''])

if test x$with_include_path != x ; then
  CPPFLAGS="$CPPFLAGS -I`echo $with_include_path | sed 's/:/ -I/g'`"
fi

AC_ARG_WITH(lib-path,
  AC_HELP_STRING([--with-lib-path], [A colon-separated list of directories to search for libraries]),,
  [with_lib_path=''])

if test x$with_lib_path != x ; then
  LDFLAGS="$LDFLAGS -L`echo $with_lib_path | sed 's/:/ -L/g'`"
fi

AC_ARG_ENABLE(public-key,
  AC_HELP_STRING([--disable-public-key], [Disable public key algorithms]),,
  [enable_public_key=yes])

AC_ARG_ENABLE(assembler,
  AC_HELP_STRING([--disable-assembler],[Disable assembler code]),,
  [enable_assembler=yes])

AC_ARG_ENABLE(shared,
  AC_HELP_STRING([--enable-shared], [Build a shared library]),,
  [enable_shared=no])

AC_ARG_ENABLE(pic,
  AC_HELP_STRING([--disable-pic],
  [Do not try to compile library files as position independent code]),,
  [enable_pic=yes])

LSH_RPATH_INIT([`echo $with_lib_path | sed 's/:/ /g'` \
    `echo $exec_prefix | sed "s@^NONE@$prefix/lib@g" | sed "s@^NONE@$ac_default_prefix/lib@g"` \
    /usr/local/lib /sw/local/lib /sw/lib \
    /usr/gnu/lib /opt/gnu/lib /sw/gnu/lib /usr/freeware/lib /usr/pkg/lib])

# Checks for programs.
AC_PROG_CC

# When $CC foo.c -o foo creates both foo and foo.exe, autoconf picks
# up the foo.exe and sets exeext to .exe. That is correct for cygwin,
# which has some kind of magic link from foo to foo.exe, but not for
# rntcl. A better check for the cygwin case would check if the
# contents of foo and foo.exe are equal; in the rntcl case, foo is a
# sh script, and foo.exe is a windows executable.

if test "x$CC" = xrntcl ; then
    AC_MSG_NOTICE([Compiling with rntcl; clearing EXEEXT])
    ac_exeext=''
    ac_cv_exeext=''
    EXEEXT=''
fi

AC_PROG_MAKE_SET
AC_PROG_RANLIB

if test "x$ac_cv_prog_cc_stdc" = xno ; then
  AC_ERROR([the C compiler doesn't handle ANSI-C])
fi

AC_PROG_INSTALL

LSH_DEPENDENCY_TRACKING

# Select assembler code
asm_path=
case "$host" in
  [i?86*-*-* | k[5-8]*-*-* | pentium*-*-* | athlon-*-*])
    asm_path=x86
    ;;
  *sparc*-*-*)
    AC_TRY_CPP([
#if defined(__sparcv9) || defined(__arch64__)
#error 64-bit sparc
#endif
    ], [
      asm_path=sparc
    ], [
      asm_path=sparc64
    ])
    ;;
  *)
    enable_assembler=no
    ;;
esac

# echo "enable_assembler: $enable_assembler, asm_path: $asm_path"

if test "x$enable_assembler" = xyes ; then
  if test -n "$asm_path"; then
    AC_MSG_NOTICE([Looking for assembler files in $asm_path/.])
    found=no
    for tmp_f in aes.asm aes-encrypt.asm aes-decrypt.asm \
		 arcfour-crypt.asm sha1-compress.asm machine.m4; do
#       echo "Looking for $srcdir/$asm_path/$tmp_f"
      if test -f "$srcdir/$asm_path/$tmp_f"; then
#        echo found
        found=yes
        AC_CONFIG_LINKS($tmp_f:$asm_path/$tmp_f)
      fi
    done
    if test "$found" = no; then
      enable_assembler=no
      AC_MSG_WARN([No assembler files found.])
    fi
  fi
fi

LSH_CCPIC

SHLIBCFLAGS="$CCPIC"

case `uname -sr` in
  CYGWIN*)
    SHLIBFORLINK='cygnettle-$(SHLIBMAJOR)-$(SHLIBMINOR).dll'
    SHLIBSONAME=''
    SHLIBFILE='libnettle.dll.a'
    SHLIBLINK='$(CC) $(LDFLAGS) -shared -Wl,--out-implib=$(SHLIBFILE) -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive'
    SHLIBLIBS='-Wl,--no-whole-archive $(LIBS)'
    ;;
  *)
    SHLIBFORLINK=libnettle.so
    SHLIBSONAME='$(SHLIBFORLINK).$(SHLIBMAJOR)'
    SHLIBFILE='$(SHLIBSONAME).$(SHLIBMINOR)'
    SHLIBLINK='$(CC) $(LDFLAGS) -shared -Wl,-soname=$(SHLIBSONAME)'
    # FIXME: It might make some sense to link with -lgmp
    SHLIBLIBS=''
    ;;
esac

if test "x$enable_shared" = xyes ; then
  SHLIBTARGET='$(SHLIBFORLINK)'
  SHLIBINSTALL=install-shared
else
  SHLIBTARGET=''
  SHLIBINSTALL=''
fi

if test "x$enable_pic" = xyes; then
  CCPIC_MAYBE="$CCPIC"
else
  CCPIC_MAYBE=''
fi
AC_SUBST([CCPIC_MAYBE])

LSH_MAKE_CONDITIONAL([ENABLE_ASM], [test x$enable_assembler = xyes])

AC_SUBST(SHLIBCFLAGS)
AC_SUBST(SHLIBMAJOR)
AC_SUBST(SHLIBMINOR)
AC_SUBST(SHLIBFORLINK)
AC_SUBST(SHLIBSONAME)
AC_SUBST(SHLIBFILE)
AC_SUBST(SHLIBLINK)
AC_SUBST(SHLIBTARGET)
AC_SUBST(SHLIBINSTALL)
AC_SUBST(SHLIBLIBS)

AC_PATH_PROG(M4, m4, m4)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

LSH_FUNC_ALLOCA

# Needed by the supplied memcmp.c
AC_C_BIGENDIAN
AC_REPLACE_FUNCS(memxor)

LSH_GCC_ATTRIBUTES

# According to Simon Josefsson, looking for uint32_t and friends in
# sys/types.h is needed on some systems, in particular cygwin.
AX_CREATE_STDINT_H([nettle-types.h], [sys/types.h])

# Checks for libraries
AC_CHECK_LIB(gmp, __gmpz_getlimbn,,
    [AC_MSG_WARN(
[GNU MP not found, or not 3.1 or up, see http://www.swox.com/gmp.
Support for public key algorithms will be unavailable.])]
    enable_public_key=no)

if test "x$enable_public_key" = xyes ; then
  AC_DEFINE(WITH_PUBLIC_KEY)
  RSA_EXAMPLES='rsa-keygen$(EXEEXT) rsa-sign$(EXEEXT) rsa-verify$(EXEEXT) rsa-encrypt$(EXEEXT) rsa-decrypt$(EXEEXT)'
else
  RSA_EXAMPLES=""
fi

AH_TEMPLATE([WITH_PUBLIC_KEY], [Define to enable public key features])
AC_SUBST(RSA_EXAMPLES)

# Add -R flags needed to run programs linked with gmp
LSH_RPATH_FIX

AH_TEMPLATE([HAVE_LIBCRYPTO],
	    [Define if you have openssl's libcrypto (used for benchmarking)])

OPENSSL_LIBFLAGS=''

# Check for openssl's libcrypto (used only for benchmarking)
AC_CHECK_LIB(crypto, BF_ecb_encrypt,
  [AC_DEFINE(HAVE_LIBCRYPTO)
   OPENSSL_LIBFLAGS='-lcrypto'
])

AC_SUBST(OPENSSL_LIBFLAGS)

# Set these flags *last*, or else the test programs won't compile
if test x$GCC = xyes ; then
  # Using -ggdb3 makes (some versions of) Redhat's gcc-2.96 dump core
  if "$CC" --version | grep '^2\.96$' 1>/dev/null 2>&1; then
    true
  else
    CFLAGS="$CFLAGS -ggdb3"
  fi
  CFLAGS="$CFLAGS -Wall -W \
  -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes \
  -Wpointer-arith -Wbad-function-cast -Wnested-externs"

# Don't enable -Wcast-align as it results in tons of warnings in the
# DES code. And when using stdio.
# Don't enable -Waggregate-return, as that causes warnings for glibc
# inttypes.h.
fi

AC_CONFIG_FILES([config.make config.m4 Makefile])
AC_CONFIG_FILES([tools/Makefile testsuite/Makefile examples/Makefile])

AC_OUTPUT

