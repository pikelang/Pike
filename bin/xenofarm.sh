#! /bin/sh

# $Id: xenofarm.sh,v 1.26 2004/07/14 11:03:01 grubba Exp $
# This file scripts the xenofarm actions and creates a result package
# to send back.

log() {
  echo $1 | tee -a xenofarm_result/mainlog.txt
}

log_start() {
  log "BEGIN $1"
  TZ=GMT date >> xenofarm_result/mainlog.txt
}

log_end() {
  LASTERR=$1
  if [ "$1" = "0" ] ; then
    log "PASS"
  else
    log "FAIL"
  fi
  TZ=GMT date >> xenofarm_result/mainlog.txt
}

xenofarm_build() {
  log_start compile
  $MAKE > xenofarm_result/compilelog.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || return 1
}

xenofarm_post_build() {
  POST_RESULT=0
  log_start benchmark
  $MAKE benchmark > xenofarm_result/benchmark.txt 2>&1
  log_end $?
  POST_RESULT=$LASTERR

  log_start verify
  $MAKE METATARGET=verify TESTARGS="-a -T -F" > \
    xenofarm_result/verifylog.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || return 1
  
  log_start export
  $MAKE bin_export > xenofarm_result/exportlog.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || return 1
  return $POST_RESULT
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
  cp buildid.txt xenofarm_result/
  # Configuration
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
  ) > xenofarm_result/configlogs.txt
  cp "$BUILDDIR/config.info" xenofarm_result/configinfo.txt || /bin/true
  cp "$BUILDDIR/config.cache" xenofarm_result/configcache.txt || /bin/true;
  # Compilation
  if test "`find $BUILDDIR -name '*.fail' -print`" = ""; then :; else
    (
      cd "$BUILDDIR"
      echo
      echo "The following file(s) failed to compile with full optimization."
      echo "This may affect performance negatively."
      find . -name '*.fail' -print | sed -e 's/\.fail$$//' -e 's/^/	/'
    ) >xenofarm_result/compilation_failure.txt
  fi
  # Installation
  cp "$BUILDDIR/dumpmodule.log" xenofarm_result/dumplog.txt || /bin/true
  # Testing
  if test ! -f "xenofarm_result/exportlog.txt"; then
    cp "$BUILDDIR/testsuite" xenofarm_result/testsuite.txt || /bin/true;
  fi
  # Core files
  find . -name "core" -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
       xenofarm_result/_core.txt ";"
  find . -name "*.core" -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
      xenofarm_result/_core.txt ";"
  find . -name "core.*" -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
      xenofarm_result/_core.txt ";"
log_end $?

log "END"
