#!/bin/sh

exec 5>&1 1>&2

TMP_BINDIR="`dirname \"$0\"`"

if [ x"$1" = x--cache ]; then
  CACHE_OUTPUT=yes
  shift 1
else
  CACHE_OUTPUT=no
fi

SCRIPT="$1"
shift 1

case "$SCRIPT" in
  /*|./*)
    if test -f "$SCRIPT"; then :; else
      echo "Script $SCRIPT not found."
      exit 1
    fi
  ;;
  *)
    if test -f ./$SCRIPT ; then
      :
    elif test -f "$TMP_BINDIR/$SCRIPT"; then
      SCRIPT="$TMP_BINDIR/$SCRIPT"
    else
      echo "Script $SCRIPT neither found in the current directory nor in $TMP_BINDIR."
      exit 1
    fi
  ;;
esac

if [ $CACHE_OUTPUT = yes ]; then
  for arg
  do
    INPUT="$arg"
  done
  TMPOUTPUT="${INPUT}.compiled"

  if [ -f "$TMPOUTPUT" ]; then
    if [ "`ls -1Lt $TMPOUTPUT $INPUT $SCRIPT 2>/dev/null | head -n 1`" = "`ls -1Lt $TMPOUTPUT`" ]; then
      cat "$TMPOUTPUT" >&5
      exit 0
    fi
  fi
else
  TMPOUTPUT=x
fi

test $CACHE_OUTPUT = yes && trap 'rm -rf "$TMPOUTPUT" ; exit 1' 1 2 15

if test "x${RUNPIKE-}" != x ; then
echo "precompile: $RUNPIKE $SCRIPT $@"

if [ $CACHE_OUTPUT = yes ]; then
  $RUNPIKE $SCRIPT "$@" >"$TMPOUTPUT"
else
  $RUNPIKE $SCRIPT "$@" >&5
fi

if [ $? = 0 ]; then
  test $CACHE_OUTPUT = yes && cat "$TMPOUTPUT" >&5
exit 0
else
  test $CACHE_OUTPUT = yes && rm "$TMPOUTPUT"
fi
fi

echo "Failed to run $SCRIPT."

exit 1
