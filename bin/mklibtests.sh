#! /bin/sh

#
# Creates testsuite files from the testsuite.in files
# found in the lib directory. Also copies *.test files.
#

src_dir=
dest_dir=
bin_dir=

up_to_date() {
    src_name="$1"
    if [ "x$2" = "x" ]; then
	dst_name="$1";
    else
	dst_name="$2";
    fi
    if [ -f "$dest_dir$path$dst_name" ] && \
	   [ "x" = "x`find \"\$src_dir\$path\$src_name\" -newer \"\$dest_dir\$path\$dst_name\" -print`" ]; then
	echo "$dest_dir$path$dst_name already up to date."
	return 0
    fi
    return 1
}

recurse () {
  path="$1"
  ls -1 "$src_dir$path" | while read fn; do
    if [ -d "$src_dir$path$fn" ]; then
      if [ ! -d "$dest_dir$path$fn" ]; then
        if mkdir -p "$dest_dir$path$fn"; then :; else
	  echo >&2 "Could not create $dest_dir$path$fn"
	  continue
	fi
      fi
      ( recurse "$path$fn"/ )
      continue
    fi

    dfn="$fn"
    case "$fn" in
	testsuite.in)
	    dfn="testsuite"
	    ;;
	*.test)
	    ;;
	*)
	    continue
	    ;;
    esac

    if up_to_date "$fn" "$dfn"; then continue; fi

    if exec 5>"$dest_dir$path$dfn"; then :; else
        echo >&2 "Could not create $dest_dir$path$dfn"
	continue
    fi

    if [ "$dfn" = "$fn" ]; then
	cat "$src_dir$path$fn" >&5
    else
       if [ "$PIKE_PATH_TRANSLATE" = "" ]; then
         "$bin_dir"mktestsuite "$src_dir$path$fn" >&5 -DSRCDIR="$src_dir$path"
       else
         "$bin_dir"mktestsuite "$src_dir$path$fn" >&5 \
	   -DSRCDIR="`echo $src_dir$path|sed -e $PIKE_PATH_TRANSLATE|$bin_dir/../src/posix_to_native.sh`"
       fi
    fi

    exec 5>&-
    echo "$dest_dir$path$dfn updated."
  done
}

for arg do
  case "$arg" in
    --srcdir=*) src_dir="`echo \"\$arg\" | sed -e 's/^--srcdir=//'`";;
    --destdir=*) dest_dir="`echo \"\$arg\" | sed -e 's/^--destdir=//'`";;
    --bindir=*) bin_dir="`echo \"\$arg\" | sed -e 's/^--bindir=//'`";;
  esac
done

if [ x = x"$src_dir" ]; then
  echo >&2 "No source directory selected."
  exit 1
fi

if [ x = x"$dest_dir" ]; then
  echo >&2 "No destination directory selected."
  exit 1
fi

if [ x = x"$bin_dir" ]; then
  echo >&2 "No binary directory selected."
  exit 1
fi

case "$src_dir" in */) ;; *) src_dir="$src_dir"/;; esac
case "$dest_dir" in */) ;; *) dest_dir="$dest_dir"/;; esac
case "$bin_dir" in */) ;; *) bin_dir="$bin_dir"/;; esac

recurse ""

exit 0
