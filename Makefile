#
# $Id: Makefile,v 1.34 2001/01/24 18:35:39 mast Exp $
#
# Meta Makefile
#

VPATH=.
MAKE=make
MAKEFLAGS=
OS=`uname -s -r -m|sed -e s/\ /-/g|tr [A-Z] [a-z]|tr / _`
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

force_autoconfig:
	cd src && ./run_autoconfig . 2>&1 | grep -v warning

force_configure:
	-rm -f "$(BUILDDIR)/Makefile"
	@$(MAKE) $(MAKE_FLAGS) configure

builddir:
	@builddir="$(BUILDDIR)"; \
	{ \
	  IFS='/'; \
	  dir=`echo "$$builddir" | sed -e 's|[^/].*||'`; \
	  for d in $$builddir; do \
	    dir="$$dir$$d"; \
	    test x"$$dir" = x -o -d "$$dir" || mkdir "$$dir" || exit 1; \
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
	  MAKE=$(MAKE) ; export MAKE ;\
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
	test -f "$$builddir"/master.pike -a -x "$$builddir"/pike || \
          metatarget="all $$metatarget"; \
	test "x$$metatarget" = x && metatarget=all; \
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
	@builddir='$(BUILDDIR)'; \
	case $$builddir in /*) ;; *) builddir="`pwd`/$$builddir";; esac; \
	sed -e "s@\"BUILDDIR\"@$$builddir@" < bin/pike.in > bin/pike
	@chmod a+x bin/pike

# This skips the modules.
pike: bin/pike
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=pike"

install:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=install"

install_interactive:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=install_interactive"

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

export:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=export"

bin_export:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=bin_export"

feature_list:
	@$(MAKE) $(MAKE_FLAGS) "METATARGET=feature_list"

clean:
	-cd "$(BUILDDIR)" && test -f Makefile && $(MAKE) "MAKE=$(MAKE)" clean || { \
	  res=$$?; \
	  if test -f remake; then $(MAKE) "MAKE=$(MAKE)" clean
	  else exit $$res; fi; \
	}

spotless:
	-cd "$(BUILDDIR)" && test -f Makefile && $(MAKE) "MAKE=$(MAKE)" spotless || { \
	  res=$$?; \
	  if test -f remake; then $(MAKE) "MAKE=$(MAKE)" clean \
	  else exit $$res; fi; \
	}

distclean:
	-rm -rf "$(BUILDDIR)" bin/pike

cvsclean: distclean
	for d in `find src -type d -print`; do \
	  if test -f "$$d/.cvsignore"; then \
	    (cd "$$d" && rm -f `cat ".cvsignore"`); \
	  else :; fi; \
	done

depend: configure
	-@cd "$(BUILDDIR)" && \
	$(MAKE) "MAKE=$(MAKE)" "MAKE_PARALLEL=$(MAKE_PARALLEL)" depend || { \
	  res=$$?; \
	  if test -f remake; then \
	    $(MAKE) "MAKE=$(MAKE)" "MAKE_PARALLEL=$(MAKE_PARALLEL)" depend \
	  else \
	    exit $$res; \
	  fi; \
	}

pikefun_TAGS:
	cd src && etags -l none -r \
	'/[ 	]*\(PMOD_PROTO \|PMOD_EXPORT \|static \|extern \)*void[ 	]\{1,\}f_\([a-zA-Z0-9_]*\)[ 	]*([ 	]*INT32/\2/' \
	`find . -type f -name '*.[ch]' -print`
	cd lib/modules && etags -l none -r \
	'/[ \t]*\(\<\(public\|inline\|final\|static\|protected\|local\|optional\|private\|nomask\|variant\)\>[ \t]\{1,\}\)*\(\<class\>\)[ \t]\{1,\}\<\([a-zA-Z¡-ÿ_][a-zA-Z¡-ÿ_0-9]*\)\>/\4/' \
	-r '/[ \t]*\(\<\(mixed\|float\|int\|program\|string\|function\|function(.*)\|array\|array(.*)\|mapping\|mapping(.*)\|multiset\|multiset(.*)\|object\|object(.*)\|void\|constant\|class\)\>)*\|\<\([A-ZÀ-ÖØ-ß][a-zA-Z¡-ÿ_0-9]*\)\>\)[ \t]\{1,\}\(\<\([_a-zA-Z¡-ÿ][_a-zA-Z¡-ÿ0-9]*\)\>\|``?\(!=\|->=?\|<[<=]\|==\|>[=>]\|\[\]=?\|()\|[%-!^&+*<>|~\/]\)\)[ \t]*(/\4/' \
	-r '/#[ \t]*define[ \t]+\([_a-zA-Z]+\)(?/\1/' \
	`find . -type f '(' -name '*.pmod' -o -name '*.pike' ')' -print`
