#
# $Id: Makefile,v 1.4 1999/02/05 18:54:17 grubba Exp $
#
# Meta Makefile
#

VPATH=.
MAKE=make
prefix=/usr/local
OS=`uname -srm|sed -e 's/ /-/g'|tr '[A-Z]' '[a-z]'|tr '/' '_'`
BUILDDIR=build/$(OS)
METATARGET=

# Used to avoid make compatibility problems.
BIN_TRUE=":"

all: bin/pike compile
	-@$(BIN_TRUE)

src/configure: src/configure.in
	cd src && ./run_autoconfig . 2>&1 | grep -v warning
	-@(cd "$(BUILDDIR)" && rm -f Makefile .prefix-h)

configure: src/configure
	@builddir="$(BUILDDIR)"; \
	srcdir=`pwd`/src; \
	( \
	  IFS='/'; dir=""; \
	  for d in $$builddir; do \
	    dir="$$dir$$d"; \
	    test -z "$$dir" -o -d "$$dir" || mkdir "$$dir" || exit 1; \
	    dir="$$dir/"; \
	  done \
	) && \
	cd "$$builddir" && \
	if test -f Makefile -a -f .prefix-h && test "`cat .prefix-h`" = "$(prefix)"; then :; else \
	  echo Running "$$srcdir"/configure in "$$builddir"; \
	  CONFIG_SITE=x "$$srcdir"/configure --prefix=$(prefix) $(CONFIGFLAGS) && \
	  ( echo "$(prefix)" > .prefix-h; rm -f main.o; :; ) \
	fi

compile: configure
	@builddir="$(BUILDDIR)"; \
	metatarget="$(METATARGET)"; \
	test -f "$$builddir"/pike || metatarget="new_peep_engine pike $$metatarget"; \
	cd "$$builddir" && ( \
	  echo Making in "$$builddir" $$metatarget; \
	  rm -f remake; \
	  $(MAKE) all $$metatarget || ( test -f remake && $(MAKE) all $$metatarget ) \
	)

force:
	-@$(BIN_TRUE)

bin/pike: force
	sed -e "s|\"BASEDIR\"|\"`pwd`\"|" < bin/pike.in > bin/pike
	chmod a+x bin/pike

install:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=install"

verify:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=verify"

verify_installed:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=verify_installed"

check: verify
	-@$(BIN_TRUE)

sure: verify
	-@$(BIN_TRUE)

verbose_verify:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=verbose_verify"

gdb_verify:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=gdb_verify"

run_hilfe:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=run_hilfe"

feature_list:
	@$(MAKE) "MAKE=$(MAKE)" "prefix=$(prefix)" "BUILDDIR=$(BUILDDIR)" "METATARGET=feature_list"

clean:
	-cd "$(BUILDDIR)" && $(MAKE) clean

spotless:
	-cd "$(BUILDDIR)" && $(MAKE) spotless

distclean:
	-rm -rf build

cvsclean: distclean
	for d in `find src -type d -print`; do if test -f "$$d/.cvsignore"; then (cd "$$d" && rm -f `cat ".cvsignore"`); else :; fi; done
