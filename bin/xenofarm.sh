#! /bin/sh

# $Id: xenofarm.sh,v 1.19 2003/03/28 10:19:49 grubba Exp $
# This file scripts the xenofarm actions and creates a result package
# to send back.

log() {
  echo $1 | tee -a build/xenofarm/mainlog.txt
}

log_start() {
  log "BEGIN $1"
  TZ=GMT date >> build/xenofarm/mainlog.txt
}

log_end() {
  LASTERR=$1
  if [ "$1" = "0" ] ; then
    log "PASS"
  else
    log "FAIL"
  fi
  TZ=GMT date >> build/xenofarm/mainlog.txt
}

xenofarm_build() {
  log_start compile
  $MAKE $MAKE_FLAGS > build/xenofarm/compilelog.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || return 1
}

xenofarm_post_build() {
  log_start verify
  $MAKE $MAKE_FLAGS METATARGET=verify TESTARGS="-a -T" > \
    build/xenofarm/verifylog.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || return 1
  
  log_start export
  $MAKE $MAKE_FLAGS bin_export > build/xenofarm/exportlog.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || return 1
}


# main code

LC_CTYPE=C
export LC_CTYPE
log "FORMAT 2"

log_start build
xenofarm_build
log_end $?

if [ $LASTERR = 0 ]; then
  log_start post_build
  xenofarm_post_build
  log_end $?
  else :
fi

log_start response_assembly
  # Basic stuff
  cp buildid.txt build/xenofarm/
  # Configuration
  cp "$BUILDDIR/config.info" build/xenofarm/configinfo.txt || /bin/true
  (
    cd "$BUILDDIR"
    test -f config.log && cat config.log
    for f in `find modules post_modules -name config.log -type f`; do
      echo
      echo '###################################################'
      echo '##' `dirname "$f"`
      echo
      cat "$f"
    done
  ) > build/xenofarm/configlogs.txt
  cp "$BUILDDIR/config.cache" build/xenofarm/configcache.txt || /bin/true;
  # Compilation
  if test "`find $BUILDDIR -name '*.fail' -print`" = ""; then :; else
    (
      cd "$BUILDDIR"
      echo
      echo "The following file(s) failed to compile with full optimization."
      echo "This may affect performance negatively."
      find . -name '*.fail' -print | sed -e 's/\.fail$$//' -e 's/^/	/'
    ) >build/xenofarm/compilation_failure.txt
  fi
  # Installation
  cp "$BUILDDIR/dumpmodule.log" build/xenofarm/dumplog.txt || /bin/true
  # Testing
  if test ! -f "build/xenofarm/exportlog.txt"; then
    cp "$BUILDDIR/testsuite" build/xenofarm/testsuite.txt || /bin/true;
  fi
  # Core files
  find . -name "core" -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
       build/xenofarm/_core.txt ";"
  find . -name "*.core" -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
      build/xenofarm/_core.txt ";"
  find . -name "core.*" -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
      build/xenofarm/_core.txt ";"
log_end $?

log "END"
