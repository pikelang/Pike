#!/bin/sh

if [ "$1" = "--help" ] ; then
cat <<EOF
  Creates the dependency file "dependencies". This script is
  called by "make depend". Usage:

    fixdepends.sh SRCDIR PIKE_SRC_DIR BUILD_BASE
EOF
exit 0
fi

posix_to_native=cat
for ptn in ./posix_to_native.sh "$3/posix_to_native.sh" ; do
    if [ -x "$ptn" ]; then
        posix_to_native="$ptn"
    fi
done


# The following transforms are done:

# Paths starting with $1 ==> $(SRCDIR)
# Paths starting with $2 ==> $(PIKE_SRC_DIR)
# Paths starting with $3 ==> $(BUILD_BASE)
# Paths starting with `pwd` ==> empty
# Rules for object files are duplicated for protos files (if needed).
# Remaining absolute paths are removed.
# Local files are filtered with respect to existence.
# Lines with only white-space and a terminating backslash are removed.
# Backslashes that precede empty lines with only white-space are removed.

d1="`echo \"$1\" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d2="`echo \"$2\" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d3="`echo \"$3\" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d4="`echo \"$1\" | "$posix_to_native" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d5="`echo \"$2\" | "$posix_to_native" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d6="`echo \"$3\" | "$posix_to_native" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d7="`pwd | sed -e 's:[.*[\\^$@]:\\\\&:g'`"
d8="`pwd | "$posix_to_native" | sed -e 's:[.*[\\^$@]:\\\\&:g'`"

if grep .protos $1/Makefile.in >/dev/null 2>&1; then
  sed -e "s@\([ 	]\)$d1/\([-a-zA-Z0-9.,_]*\)@\1\$(SRCDIR)/\2@g" \
      -e "s@\([ 	]\)$d2/\([-a-zA-Z0-9.,_]*\)@\1\$(PIKE_SRC_DIR)/\2@g" \
      -e "s@\([ 	]\)$d3/\([-a-zA-Z0-9.,_]*\)@\1\$(BUILD_BASE)/\2@g" \
      -e "s@\([ 	]\)$d4/\([-a-zA-Z0-9.,_]*\)@\1\$(SRCDIR)/\2@g" \
      -e "s@\([ 	]\)$d5/\([-a-zA-Z0-9.,_]*\)@\1\$(PIKE_SRC_DIR)/\2@g" \
      -e "s@\([ 	]\)$d6/\([-a-zA-Z0-9.,_]*\)@\1\$(BUILD_BASE)/\2@g" \
      -e "s@\([ 	]\)$d7\([^ 	]\)*@\1./\2@" \
      -e "s@\([ 	]\)$d8\([^ 	]\)*@\1./\2@" \
      -e 's/^\([-a-zA-Z0-9.,_]*\)\.o: /\1.o \1.protos: /g' \
      -e 's@\([ 	]\)/[^ 	]*@\1@g' \
      -e '/^[ 	]*\\$/d'
else
  sed -e "s@\([ 	]\)$d1/\([-a-zA-Z0-9.,_]*\)@\1\$(SRCDIR)/\2@g" \
      -e "s@\([ 	]\)$d2/\([-a-zA-Z0-9.,_]*\)@\1\$(PIKE_SRC_DIR)/\2@g" \
      -e "s@\([ 	]\)$d3/\([-a-zA-Z0-9.,_]*\)@\1\$(BUILD_BASE)/\2@g" \
      -e "s@\([ 	]\)$d4/\([-a-zA-Z0-9.,_]*\)@\1\$(SRCDIR)/\2@g" \
      -e "s@\([ 	]\)$d5/\([-a-zA-Z0-9.,_]*\)@\1\$(PIKE_SRC_DIR)/\2@g" \
      -e "s@\([ 	]\)$d6/\([-a-zA-Z0-9.,_]*\)@\1\$(BUILD_BASE)/\2@g" \
      -e "s@\([ 	]\)$d7\([^ 	]\)*@\1./\2@" \
      -e "s@\([ 	]\)$d8\([^ 	]\)*@\1./\2@" \
      -e 's@\([ 	]\)/[^ 	]*@\1@g' \
      -e '/^[ 	]*\\$/d'
fi >"$1/dependencies"

# Perform filtering. This step is needed due to gcc 3.3. The reason is
# that it tells system and user includes apart by where they are found
# and not by the #include syntax. Thus missing system includes remain
# in the dependency lists. We can get missing system includes when
# --disable-binary is used.
for f in `sed <"$1/dependencies" \
	    -e 's@^.*:@@' -e 's/\\\\$//' \
	    -e 's@\\([ 	]\\)\\$([A-Za-z_]*)/[-a-zA-Z0-9.,_/]*@@g' \
	    -e 's/[ 	]*$//' -e 's/^[ 	]*//' | sort | uniq`; do
  if [ ! -f "$f" ]; then
    echo "WARNING: Missing local header file $f." >&2
    # Buglet: $f is not regexp escaped in the regexps below.
    sed <"$1/dependencies" -e "s@[ 	]$f[ 	]@ @g" \
      -e "s@[ 	]$f\$@@g" >"$1/dependencies.tmp" && \
      mv "$1/dependencies.tmp" "$1/dependencies"
  fi
done

# Perform final cleanup.
sed <"$1/dependencies" -e '/^[ 	]*\\$/d' | sed \
  -e '/\\$/{;N;/\n[ 	]*$/s/\\\(\n\)/\1/;P;D;}' \
    >"$1/dependencies.tmp" && \
  mv "$1/dependencies.tmp" "$1/dependencies"

if test -f "$1/acconfig.h"; then
    # Add suitable dependencies for regenerating $(CONFIG_HEADERS).
    # Note that we have to have a rule for an acconfig.h in case
    # it has gone away to avoid make(1) complaining.
    cat <<EOF
# Regenerate \$(CONFIG_HEADERS) from \$(SRCDIR)/acconfig.h
\$(CONFIG_HEADERS): \$(SRCDIR)/\$(CONFIG_HEADERS).in
\$(SRCDIR)/\$(CONFIG_HEADERS).in \$(SRCDIR)/configure: \$(SRCDIR)/acconfig.h
\$(SRCDIR)/acconfig.h:
EOF
fi >>$1/dependencies


#sed -e "s@/./@/@g
#s@$d1/\([-a-zA-Z0-9.,_]*\)@\$(SRCDIR)/\1@g" >$1/dependencies

# sed "s@[-/a-zA-Z0-9.,_][-/a-zA-Z0-9.,_]*/\([-a-zA-Z0-9.,_]*\)@\1@g" > $1/dependencies

