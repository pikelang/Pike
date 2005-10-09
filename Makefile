#
# $Id: Makefile,v 1.143 2005/10/09 15:26:25 nilsson Exp $
#
# Meta Makefile
#

VPATH=.
OS=`uname -s -r -m|sed \"s/ /-/g\"|tr \"[A-Z]\" \"[a-z]\"|tr \"/()\" \"___\"`
BUILDDIR=build/$(OS)
METATARGET=

# This evaluates to a proper make command.
# Regardless of whether the make sets $(MAKE) or not.
# Priority is $(MAKE) before ${MAKE} before make.
MAKE_CMD=`if [ "x$(MAKE)" = "x" ]; then echo "$${MAKE-make}"; else echo "$(MAKE)"; fi`

# Evaluates to $(MAKE_CMD) and sets ${MAKE} and $(MAKE)
DO_MAKE=MAKE="$(MAKE_CMD)" export MAKE && "$${MAKE}" "MAKE=$${MAKE}"

# Use this to pass arguments to configure. Leave empty to keep previous args.
CONFIGUREARGS=

# Set to a flag for parallelizing make, e.g. -j2. It's given to make
# at the level where it's most effective.
MAKE_PARALLEL=

# Used to avoid make compatibility problems.
BIN_TRUE=":"

MAKE_FLAGS="CONFIGUREARGS=$(CONFIGUREARGS)" "BUILDDIR=$(BUILDDIR)"

all: bin/pike compile
	-@$(BIN_TRUE)

force:
	-@$(BIN_TRUE)

src/configure:
	cd src && ./run_autoconfig . 2>&1 | grep -v warning

force_autoconfig:
	cd src && ./run_autoconfig . 2>&1 | grep -v warning

force_configure:
	-rm -f "$(BUILDDIR)/Makefile"
	@$(DO_MAKE) $(MAKE_FLAGS) configure

configure_help: src/configure
	cd src && ./configure --help

builddir:
	@builddir="$(BUILDDIR)"; \
	if [ -d "$$builddir" ]; then : ; else NEWBUILD=YEP ; fi; \
	OLDIFS="$IFS"; \
	IFS='/'; \
	dir=`echo "$$builddir" | sed -e 's|[^/].*||'`; \
	for d in $$builddir; do \
	  dir="$$dir$$d"; \
	  if test x"$$dir" = x -o -d "$$dir"; then :; else \
	    echo "Creating $$dir..."; \
	    mkdir "$$dir" || exit 1; \
	  fi; \
	  dir="$$dir/"; \
	done; \
	IFS="$OLDIFS"; \
	if [ "x$$NEWBUILD" = "xYEP" ]; then \
	  if [ -f "refdoc/autodoc.xml" ]; then \
	    cp "refdoc/autodoc.xml" "$$builddir"; \
	  fi; \
	  mkdir "$$builddir/doc_build"; \
	  if [ -d "refdoc/images" ]; then \
	    cp -R "refdoc/images" "$$builddir/doc_build/images"; \
	  fi; \
	fi; \
	cd "$$builddir"

configure: src/configure builddir
	@builddir="$(BUILDDIR)"; \
	srcdir=`pwd`/src; \
	cd "$$builddir" && { \
	  if test "x$(CONFIGUREARGS)" = x--help; then \
	    "$$srcdir"/configure --help; \
	    exit 1; \
	  fi; \
	  if test "x$(CONFIGUREARGS)" = x; then :; else \
	    configureargs="$(CONFIGUREARGS)"; \
	    oldconfigureargs="$(CONFIGUREARGS)"; \
	  fi; \
	  if test -f .configureargs; then \
	    oldconfigureargs="`cat .configureargs`"; \
	  else :; fi; \
	  if test "x$(CONFIGUREARGS)" = x; then \
	    configureargs="$$oldconfigureargs"; \
	  else :; fi; \
	  MAKE="$(MAKE_CMD)"; \
	  export MAKE; \
	  echo; \
	  echo Configure arguments: $$configureargs; \
	  echo 'Use `make CONFIGUREARGS="..."' "...'" 'to change them.'; \
	  echo 'They will be retained in the build directory.'; \
	  echo; \
	  if test -f Makefile -a -f config.status -a -f .configureargs -a \
		  "x$$oldconfigureargs" = "x$$configureargs"; then :; \
	  else \
	    echo Running $$srcdir/configure $$configureargs in $$builddir; \
	    if test "x$${CONFIG_SHELL}" = "x" && \
	      /bin/bash -norc -c : 2>/dev/null; then \
	      CONFIG_SHELL="/bin/bash -norc" ; \
	    fi ;\
	    runconfigure () { \
	      CONFIG_SITE=x $${CONFIG_SHELL-/bin/sh} \
		"$$srcdir"/configure "$$@" || exit $$?; \
	    }; \
	    eval runconfigure $$configureargs; \
	    echo "$$configureargs" > .configureargs; \
	    if test "x$$oldconfigureargs" = "x$$configureargs"; then :; \
	    else \
	      echo Configure arguments have changed - doing make clean; \
	      $${MAKE} "MAKE=$${MAKE}" clean || exit $$?; \
	      if test "x$(METATARGET)" = "xsource"; then :; \
	      elif test "x$(METATARGET)" = "xexport"; then :; \
	      else \
		echo Configure arguments have changed - doing make depend; \
		$${MAKE} "MAKE=$${MAKE}" depend || exit $$?; \
	      fi; \
	    fi; \
	  fi; \
	} || exit $$?

compile: configure
	@builddir="$(BUILDDIR)"; \
	cd "$$builddir" && { \
	  metatarget="$(METATARGET)"; \
	  if test "x$(LIMITED_TARGETS)" = "x"; then \
	    if test -f master.pike -a -x pike; then :; \
	    elif test "x$$metatarget" = xpike; then :; \
	    else metatarget="all $$metatarget"; fi; \
	    if test "x$$metatarget" = x; then metatarget=all; else :; fi; \
	  else :; fi; \
	  MAKE="$(MAKE_CMD)"; \
	  export MAKE; \
	  for target in $$metatarget; do \
	    echo Making $$target in "$$builddir"; \
	    rm -f remake; \
	    $${MAKE} "MAKE=$${MAKE}" "MAKE_PARALLEL=$(MAKE_PARALLEL)" "EXPORT_NAME=$(EXPORT_NAME)" $$target || { \
	      res=$$?; \
	      if test -f remake; then \
		$${MAKE} "MAKE=$${MAKE}" "MAKE_PARALLEL=$(MAKE_PARALLEL)" "EXPORT_NAME=$(EXPORT_NAME)" $$target || \
		  exit $$?; \
	      else \
		exit $$res; \
	      fi; \
	    }; \
	  done; \
	} || exit $$?

# FIXME: The refdoc stuff ought to use $(BUILDDIR) too.

documentation:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=documentation"

doc: documentation

legal: bin/pike
	bin/pike -e 'Stdio.write_file("COPYRIGHT", \
	  Tools.Legal.Copyright.get_text());'
	bin/pike -e 'Stdio.write_file("COPYING", \
	  Tools.Legal.License.get_text());'

# Don't make bin/pike if we're recursing with a $(METATARGET) since we
# don't want the backquote expression which usually is part of
# $(BUILDDIR) to be expanded in that script. It's better to evaluate
# it when the script is run, so that it can choose the build directory
# for the right architecture.
bin/pike: force
	@if test \! -x bin/pike -o "x$(METATARGET)" = x; then \
	  builddir='$(BUILDDIR)'; \
	  case $$builddir in /*) ;; *) builddir="`pwd`/$$builddir";; esac; \
	  sed -e "s@\"BUILDDIR\"@$$builddir@" < bin/pike.in > bin/pike && \
	  chmod a+x bin/pike; \
	else :; fi

# This skips the modules.
pike: bin/pike
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=pike"

install: bin/pike
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=install"

install_nodoc: bin/pike
	@INSTALLARGS="--no-autodoc $(INSTALLARGS)"
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=install"

install_interactive: bin/pike
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=install_interactive"

install_interactive_nodoc: bin/pike
	@INSTALLARGS="--no-autodoc $(INSTALLARGS)"
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=install_interactive"

tinstall: bin/pike
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=tinstall"

testsuites:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=testsuites"
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=testsuite"

just_verify:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=just_verify"

valgrind_verify:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=valgrind_verify"

verify:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=verify"

verify_installed:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=verify_installed"

check: verify
	-@$(BIN_TRUE)

sure: verify
	-@$(BIN_TRUE)

verbose_verify:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=verbose_verify"

gdb_verify:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=gdb_verify"

dump_modules:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=dump_modules"

force_dump_modules:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=force_dump_modules"

delete_dumped_modules:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=delete_dumped_modules"

undump_modules:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=undump_modules"

run_hilfe:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=run_hilfe"

source:
	@$(DO_MAKE) "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "OS=source" "LIMITED_TARGETS=yes" "METATARGET=source" compile

export:
	@if [ -f "$(BUILDDIR)/autodoc.xml" ]; then : ; else \
	  echo ; \
	  echo 'No documentation source built!'; \
	  echo 'Type "make doc" to make a documentation source file'; \
	  echo 'or "make export_nodoc" to export without a documentation'; \
	  echo 'source.' ; \
	  echo ; \
	  exit 1; \
	fi
	@if ls bundles/gmp-*.tar.gz > /dev/null 2>&1; then : ; else \
	  echo ; \
	  echo 'Missing GMP bundle.'; \
	  exit 1; \
	fi
	@if ls bundles/nettle-*.tar.gz > /dev/null 2>&1; then : ; else \
	  echo ; \
	  echo 'Missing Nettle bundle.'; \
	  exit 1; \
	fi
	@$(DO_MAKE) "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "OS=source" "LIMITED_TARGETS=yes" "METATARGET=export" compile

export_nodoc:
	@$(DO_MAKE) "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "OS=source" "LIMITED_TARGETS=yes" "METATARGET=export" compile

snapshot_export:
	@$(DO_MAKE) "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "OS=source" "LIMITED_TARGETS=yes" "METATARGET=snapshot_export" \
	  "EXPORT_NAME=Pike-v%maj.%min-snapshot-%Y%M%D" compile

snapshot: snapshot_export

xenofarm_export:
	@echo Begin export
	@rm -f export_result.txt
	@echo "Adding bundles from $$HOME/pike_bundles/..." >>export_result.txt
	@for f in "$$HOME/pike_bundles/"* no; do \
	  if test -f "$$f"; then \
	    echo "  Bundling" `echo "$$f"|sed -e 's/.*\///g'`; \
	    cp -f "$$f" bundles/; \
	  fi; \
	done >>export_result.txt
	@$(DO_MAKE) "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "OS=source" "LIMITED_TARGETS=yes" "METATARGET=snapshot_export" \
	  "EXPORT_NAME=Pike%maj.%min-%Y%M%D-%h%m%s" \
	  "EXPORTARGS=--snapshot $(EXPORTARGS)" \
	  compile >>export_result.txt 2>&1
	@echo Export done

bin_export:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=bin_export"

feature_list:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=feature_list"

solaris_pkg_configure:
	@$(DO_MAKE) "CONFIGUREARGS=--prefix=/opt $(CONFIGUREARGS)" \
	  "METATARGET=configure"

solaris_pkg: solaris_pkg_configure bin/pike
	@test -d "${BUILDDIR}/solaris_pkg_build" || mkdir "${BUILDDIR}/solaris_pkg_build"
	@cd "${BUILDDIR}" && $(DO_MAKE) \
		"buildroot=solaris_pkg_build/" \
		install
	@bin/pike bin/make_solaris_pkg.pike --prefix="/opt" --installroot="`pwd`/${BUILDDIR}/solaris_pkg_build"  --pkgdest="`pwd`"
	@test -d "${BUILDDIR}/solaris_pkg_build" && rm -rf "${BUILDDIR}/solaris_pkg_build"
	@ls -l *pkg

xenofarm_feature:
	$(DO_MAKE) BUILDDIR="$(BUILDDIR)" \
	  CONFIGUREARGS="$(CONFIGUREARGS) --with-cdebug --with-security --with-double-precision --with-profiling --with-keypair-loop --with-new-multisets" \
	  xenofarm

xenofarm:
	-rm -rf xenofarm_result
	mkdir xenofarm_result
	-CCACHE_LOGFILE="`pwd`/xenofarm_result/ccache.log.txt" \
	  MAKE="$(MAKE_CMD)" BUILDDIR="$(BUILDDIR)" \
	  CONFIGUREARGS="$(CONFIGUREARGS)" /bin/sh bin/xenofarm.sh
	cd xenofarm_result && tar cf - . > ../xenofarm_result.tar
	gzip -f9 xenofarm_result.tar

benchmark:
	@$(DO_MAKE) $(MAKE_FLAGS) "METATARGET=run_bench"

clean:
	-cd "$(BUILDDIR)" && test -f Makefile && $(DO_MAKE) clean || { \
	  res=$$?; \
	  if test -f remake; then $(DO_MAKE) clean; \
	  else exit $$res; fi; \
	} || exit $$?

spotless:
	-cd "$(BUILDDIR)" && test -f Makefile && $(DO_MAKE) spotless || { \
	  res=$$?; \
	  if test -f remake; then $(DO_MAKE) spotless; \
	  else exit $$res; fi; \
	} || exit $$?

delete_builddir:
	-rm -rf "$(BUILDDIR)"

distclean: delete_builddir
	$(DO_MAKE) "OS=source" delete_builddir
	-rm -f bin/pike

srcclean:
	for d in `find src -type d -print`; do \
	  if test -f "$$d/.cvsignore"; then \
	    (cd "$$d" && rm -f `cat ".cvsignore"`); \
	  else :; fi; \
	done

cvsclean: srcclean distclean docspotless
	-rm -rf build
	-rm -f export_result.txt
	-rm -f Pike*.tar.gz

docclean:
	-rm -rf "$(BUILDDIR)/doc_build"
	-rm -f "$(BUILDDIR)/autodoc.xml"
	-rm -f "$(BUILDDIR)/modref.xml"
	-rm -f "$(BUILDDIR)/onepage.xml"
	-rm -f "$(BUILDDIR)/traditional.xml"

docspotless:
	if test -f "refdoc/Makefile"; then \
	  cd refdoc; $(DO_MAKE) spotless; \
	else :; fi

depend:
	@if test -d build; then \
	  $(MAKE) "MAKE=$(MAKE)" "MAKE_PARALLEL=$(MAKE_PARALLEL)" _depend; \
	else \
	  echo You never need to do \"make depend\" before the first build, ; \
	  echo and doing \"make depend\" in a Pike dist will actually break ; \
	  echo the dist. ;\
	  exit 1; fi;

_depend: configure
	-@cd "$(BUILDDIR)" && \
	$(DO_MAKE) "MAKE_PARALLEL=$(MAKE_PARALLEL)" depend || { \
	  res=$$?; \
	  if test -f remake; then $(DO_MAKE) "MAKE_PARALLEL=$(MAKE_PARALLEL)" depend; \
	  else exit $$res; fi; \
	} || exit $$?

#! Creates tags files src/TAGS (C-level methods) and lib/modules/TAGS
#! (pike-level methods). The typical use case for an etags file is finding the
#! file and line where a class or method was defined. This feature is by
#! default bound to the keyboard sequence Meta-. in emacs.
#! @note
#!   Finding C-level methods isn't trivial even with the src/TAGS table loaded;
#!   this make target could use some improvement.
pikefun_TAGS:
	cd src && etags -l none -r \
	'/[ 	]*\(PMOD_PROTO \|PMOD_EXPORT \|static \|extern \)*void[ 	]\{1,\}f_\([a-zA-Z0-9_]*\)[ 	]*([ 	]*INT32/\2/' \
	`find . -type f -name '*.[ch]' -print`
	cd lib/modules && ../../bin/pike_etags.sh \
	  `find . -type f '(' -name '*.pmod' -o -name '*.pike' ')' -print`
