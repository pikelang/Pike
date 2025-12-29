#! /bin/sh

# This file scripts the xenofarm actions and creates a result package
# to send back.

# Simplify informing various scripts that they should apply
# Xenofarm defaults.
PIKE_XENOFARM_DEFAULTS="yes"
export PIKE_XENOFARM_DEFAULTS

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
  log_start features
  bin/pike -x features > xenofarm_result/features.txt 2>&1
  log_end $?
  [ $LASTERR = 0 ] || POST_RESULT=$LASTERR

  if [ "x$SKIP_BENCHMARK" = "x" ]; then
      log_start benchmark
      $MAKE benchmark > xenofarm_result/benchmark.txt 2>&1
      log_end $?
      [ $LASTERR = 0 ] || POST_RESULT=$LASTERR
  fi

  if [ "x$SKIP_VERIFY" = "x" ]; then
      log_start verify
      # Note: verify and valgrind_verify perform the same actions
      #       if not compiled --with-valgrind.
      $MAKE METATARGET=valgrind_verify TESTARGS="-a -T -F" > \
            xenofarm_result/verifylog.txt 2>&1
      log_end $?
      [ $LASTERR = 0 ] || POST_RESULT=$LASTERR
  fi

  if [ "x$MAKE_DOC" = "x" ]; then :; else
      log_start doc
      DOC_RESULT=0
      while :; do
          log_start extract_autodoc
          $MAKE METATARGET=autodoc.xml \
                >xenofarm_result/extract_autodoc.txt 2>&1
          log_end $?
          if [ -f "$BUILDDIR/resolution.log" ]; then
              cp "$BUILDDIR/resolution.log" xenofarm_result/resolution_log.txt
          fi
          if [ $LASTERR = 0 ]; then :; else
              DOC_RESULT=$LASTERR
              break;
          fi

          log_start assemble_autodoc
          ASSEMBLE_RESULT=0
          log_start assemble_modref
          $MAKE METATARGET=modref.xml \
                >xenofarm_result/assemble_modref.txt 2>&1
          log_end $?
          [ $LASTERR = 0 ] || ASSEMBLE_RESULT=$LASTERR

          log_start assemble_onepage
          $MAKE METATARGET=onepage.xml \
                >xenofarm_result/assemble_onepage.txt 2>&1
          log_end $?
          [ $LASTERR = 0 ] || ASSEMBLE_RESULT=$LASTERR

          log_start assemble_traditional
          $MAKE METATARGET=traditional.xml \
                >xenofarm_result/assemble_traditional.txt 2>&1
          log_end $?
          [ $LASTERR = 0 ] || ASSEMBLE_RESULT=$LASTERR
          log_end $ASSEMBLE_RESULT

          if [ $LASTERR = 0 ]; then :; else
              DOC_RESULT=$LASTERR
              break;
          fi

          log_start render_autodoc
          RENDER_RESULT=0
          log_start render_modref
          DOCTARGET=modref $MAKE METATARGET=documentation \
                           >xenofarm_result/render_modref.txt 2>&1
          log_end $?
          [ $LASTERR = 0 ] || RENDER_RESULT=$LASTERR

          log_start render_onepage
          DOCTARGET=onepage $MAKE METATARGET=documentation \
                           >xenofarm_result/render_onepage.txt 2>&1
          log_end $?
          [ $LASTERR = 0 ] || RENDER_RESULT=$LASTERR
          log_start render_traditional
          DOCTARGET=traditional $MAKE METATARGET=documentation \
                           >xenofarm_result/render_traditional.txt 2>&1
          log_end $?
          [ $LASTERR = 0 ] || RENDER_RESULT=$LASTERR

          log_end $RENDER_RESULT
          [ $LASTERR = 0 ] || DOC_RESULT=$LASTERR

          break;
      done
      log_end $DOC_RESULT
      [ $LASTERR = 0 ] || POST_RESULT=$LASTERR
  fi
  
  if [ "x$SKIP_EXPORT" = "x" ]; then
      log_start export
      PIKE_VERSION_SUFFIX="`sed -e 's/^revision:\(.......\).*/-\1/p' -ed buildid.txt`"
      export PIKE_VERSION_SUFFIX
      $MAKE bin_export INSTALLER_DESTDIR="`pwd`/xenofarm_result" > \
            xenofarm_result/exportlog.txt 2>&1
      log_end $?
      [ $LASTERR = 0 ] || return 1
  fi
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
    for f in `find . -name config.log -type f | sort`; do
      echo
      echo '###################################################'
      echo '##' `dirname "$f"`
      echo
      cat "$f"
    done
  ) > xenofarm_result/configlogs.txt
  cp "$BUILDDIR/config.info" xenofarm_result/configinfo.txt || true
  if [ "x$CONFIG_CACHE_FILE" = "x" ]; then
      CONFIG_CACHE_FILE="$BUILDDIR/config.cache"
  fi
  cp "$CONFIG_CACHE_FILE" xenofarm_result/configcache.txt || true;
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
  cp "$BUILDDIR/dumpmodule.log" xenofarm_result/dumplog.txt || true
  # Testing
  if test ! -f "xenofarm_result/exportlog.txt"; then
    cp "$BUILDDIR/testsuite" xenofarm_result/testsuite.txt || true;
  fi
  # Core files
  find . -name "core" -print -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
       xenofarm_result/_core.txt ";"
  find . -name "*.core" -print -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
      xenofarm_result/_core.txt ";"
  find . -name "core.*" -print -exec \
    gdb --batch --nx --command=bin/xenofarm_gdb_cmd "$BUILDDIR/pike" {} >> \
      xenofarm_result/_core.txt ";"
log_end $?

log "END"
