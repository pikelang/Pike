#! /bin/sh

#
# Creates testsuit files form the testsuit.in files
# found in the lib directory.
# $Id$
#

src_dir=
dest_dir=
bin_dir=

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

    if [ testsuite.in != "$fn" ]; then continue; fi

    if [ -f "$dest_dir$path"testsuite ] && \
       [ "" = "`find \"\$src_dir\$path\$fn\" -newer \"\$dest_dir\$path\"testsuite -print`" ]; then
       echo "$path"testsuite already up to date.
    else
       if exec 5>"$dest_dir$path"testsuite; then :; else
         echo >&2 "Could not create $dest_dir$path"testsuite
	 continue
       fi
       "$bin_dir"mktestsuite "$src_dir$path$fn" >&5
       exec 5>&-
       echo "$path"testsuite updated.
    fi    
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
