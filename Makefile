#
# $Id: Makefile,v 1.19 1999/11/08 23:27:32 mast Exp $
#
# Meta Makefile
#

VPATH=.
MAKE=make
OS=`uname -srm|sed -e 's/ /-/g'|tr '[A-Z]' '[a-z]'|tr '/' '_'`
BUILDDIR=build/$(OS)
METATARGET=

# Use this to pass arguments to configure. Leave empty to keep previous args.
CONFIGUREARGS=

# Set to a flag for parallelizing make, e.g. -j2. It's given to make
# at the level where it's most effective.
MAKE_PARALLEL=

# Used to avoid make compatibility problems.
BIN_TRUE=":"

MAKE_FLAGS="MAKE=$(MAKE)" "CONFIGUREARGS=$(CONFIGUREARGS)" "BUILDDIR=$(BUILDDIR)"

all: bin/pike compile
	-@$(BIN_TRUE)

force:
	-@$(BIN_TRUE)

src/configure: src/configure.in
	cd src && ./run_autoconfig . 2>&1 | grep -v warning
	-rm -f "$(BUILDDIR)/Makefile"

force_configure:
	cd src && ./run_autoconfig . 2>&1 | grep -v warning
	-rm -f "$(BUILDDIR)/Makefile"

builddir:
	@builddir="$(BUILDDIR)"; \
	{ \
	  IFS='/'; dir=""; \
	  for d in $$builddir; do \
	    dir="$$dir$$d"; \
	    test -z "$$dir" -o -d "$$dir" || mkdir "$$dir" || exit 1; \
	    dir="$$dir/"; \
	  done; \
	}

configure: src/configure builddir
	@builddir="$(BUILDDIR)"; \
	srcdir=`pwd`/src; \
	cd "$$builddir" && { \
	  if test -f .configureargs -a -z "$(CONFIGUREARGS)"; then \
	    configureargs="`cat .configureargs`"; \
	  else \
	    configureargs="$(CONFIGUREARGS)"; \
	  fi; \
	  echo; \
	  echo Configure arguments: $$configureargs; \
	  echo; \
	  if test -f Makefile -a -f config.cache -a -f .configureargs && \
	     test "`cat .configureargs`" = "$$configureargs"; then :; \
	  else \
	    echo Running "$$srcdir"/configure $$configureargs in "$$builddir"; \
	    CONFIG_SITE=x "$$srcdir"/configure $$configureargs && { \
	      echo "$$configureargs" > .configureargs; \
	      $(MAKE) "MAKE=$(MAKE)" clean > /dev/null; \
	      :; \
	    } \
	  fi; \
	}

compile: configure
	@builddir="$(BUILDDIR)"; \
	metatarget="$(METATARGET)"; \
	test -x "$$builddir"/pike || metatarget="all $$metatarget"; \
	cd "$$builddir" && for target in $$metatarget; do \
	  echo Making $$target in "$$builddir"; \
	  rm -f remake; \
	  $(MAKE) "MAKE=$(MAKE)" "MAKE_PARALLEL=$(MAKE_PARALLEL)" $$target || { \
	    res=$$?; \
	    if test -f remake; then \
	      $(MAKE) "MAKE=$(MAKE)" "MAKE_PARALLEL=$(MAKE_PARALLEL)" $$target || \
		exit $$?; \
	    else \
	      exit $$res; \
	    fi; \
	  } \
	done

bin/pike: force
	sed -e "s|\"BASEDIR\"|\"`pwd`\"|" < bin/pike.in > bin/pike
	chmod a+x bin/pike

install:
	@$(MAKE) "METATARGET=install"

just_verify:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=just_verify"

verify:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=verify"

verify_installed:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=verify_installed"

check: verify
	-@$(BIN_TRUE)

sure: verify
	-@$(BIN_TRUE)

verbose_verify:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=verbose_verify"

gdb_verify:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=gdb_verify"

run_hilfe:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=run_hilfe"

feature_list:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=feature_list"

clean:
	-cd "$(BUILDDIR)" && test -f Makefile && $(MAKE) "MAKE=$(MAKE)" clean

spotless:
	-cd "$(BUILDDIR)" && test -f Makefile && $(MAKE) "MAKE=$(MAKE)" spotless

distclean:
	-rm -rf build bin/pike

cvsclean: distclean
	for d in `find src -type d -print`; do if test -f "$$d/.cvsignore"; then (cd "$$d" && rm -f `cat ".cvsignore"`); else :; fi; done

depend: configure
	-cd "$(BUILDDIR)" && $(MAKE) "MAKE=$(MAKE)" depend
