#!/bin/sh

# Arguments:
#   SRCDIR
#   PIKE_SRC_DIR
#   BUILD_BASE

# The following transforms are done:

# Paths starting with $1 ==> $(SRCDIR)
# Paths starting with $2 ==> $(PIKE_SRC_DIR)
# Paths starting with $3 ==> $(BUILD_BASE)
# Paths starting with `pwd` ==> empty
# Rules for object files are duplicated for protos files.
# Remaining absolute paths are removed.
# Lines with only white-space are removed.

sed -e "s@\([ 	]\)$1/\([-a-zA-Z0-9.,_]*\)@\1\$(SRCDIR)/\2@g" -e "s@\([ 	]\)$1/\([-a-zA-Z0-9.,_]*\)@\1\$(PIKE_SRC_DIR)/\2@g" -e "s@\([ 	]\)$3/\([-a-zA-Z0-9.,_]*\)@\1\$(BUILD_BASE)/\2@g" -e "s@\([ 	]\)`pwd`\([^ 	]\)*@\1\2@" -e 's/^\([-a-zA-Z0-9.,_]*\)\.o: /\1.o \1.protos: /g' -e 's@\([ 	]\)/[^ 	]*@\1@g' -e '/^[ 	]*\\$/d' >$1/dependencies

#sed -e "s@/./@/@g
#s@$1/\([-a-zA-Z0-9.,_]*\)@\$(SRCDIR)/\1@g" >$1/dependencies

# sed "s@[-/a-zA-Z0-9.,_][-/a-zA-Z0-9.,_]*/\([-a-zA-Z0-9.,_]*\)@\1@g" > $1/dependencies

