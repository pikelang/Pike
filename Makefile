#
# Meta Makefile
#

# Use this to pass arguments to configure. Leave empty to keep previous args.
#CONFIGUREARGS=

# Set this to any generic make options you'd otherwise would have to
# pass on the command line. (Using the magic MAKEFLAGS variable
# directly here might not work all the time due to the $(DO_MAKE)
# recursion.)
#MAKE_FLAGS=

# Set to a flag for parallelizing make, e.g. -j2. It's given to make
# at the level where it's most effective.
#MAKE_PARALLEL=-j`test -f /proc/cpuinfo && grep ^processor /proc/cpuinfo | wc -l || echo 1`

# Tip: Remove "-r" from the line below if you don't want to rebuild
# from scratch every time you upgrade the kernel.
OS=`if [ -n \"$$PIKE_BUILD_OS\" ]; then echo \"$$PIKE_BUILD_OS\"; else uname -s -r -m|sed \"s/ /-/g\"|tr \"[A-Z]\" \"[a-z]\"|tr \"/()\" \"___\"; fi`

VPATH=.
BUILDDIR=build/$(OS)
METATARGET=

# This evaluates to a proper make command.
# Regardless of whether the make sets $(MAKE) or not.
# Priority is $(MAKE) before ${MAKE} before make.
MAKE_CMD=`if [ "x$(MAKE)" = "x" ]; then echo "$${MAKE-make}"; else echo "$(MAKE)"; fi`

# Used internally in this file to start a submake to expand
# $(BUILDDIR), $(MAKE_CMD) etc.
DO_MAKE=MAKE="$(MAKE_CMD)" export MAKE && \
  "$${MAKE}" \
  `$$MAKE force --no-print-directory >/dev/null 2>&1 && echo --no-print-directory || :` \
  $(MAKE_FLAGS) "MAKE=$${MAKE}" \
  "CONFIGUREARGS=$(CONFIGUREARGS)" "BUILDDIR=$(BUILDDIR)"

# Used to avoid make compatibility problems.
BIN_TRUE=":"

all: bin/pike compile
	-@$(BIN_TRUE)

force:
	-@$(BIN_TRUE)

src/configure: src/configure.in src/aclocal.m4
	cd src && ./run_autoconfig .

force_autoconfig:
	cd src && ./run_autoconfig .

force_configure:
	-builddir="$(BUILDDIR)"; rm -f "$$builddir/Makefile"
	@$(DO_MAKE) configure

reconfigure:
	-builddir="$(BUILDDIR)"; rm -f "$$builddir/Makefile" "$$builddir/config.cache"
	@$(DO_MAKE) configure

configure_help: src/configure
	cd src && ./configure --help

compile:
	@$(DO_MAKE) METATARGET=$(METATARGET) _make_in_builddir

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
	    if test "x$${CONFIG_SHELL}" = "x" && \
	      /bin/bash -norc -c : 2>/dev/null; then \
	      CONFIG_SHELL="/bin/bash -norc" ; \
	    fi ;\
	    runconfigure () { \
	      CONFIG_SITE=x CONFIG_SHELL=$${CONFIG_SHELL-/bin/sh} $${CONFIG_SHELL-/bin/sh} \
		"$$srcdir"/configure "$$@" || exit $$?; \
	    }; \
	    echo Running $$CONFIG_SHELL $$srcdir/configure $$configureargs in $$builddir; \
	    eval runconfigure $$configureargs; \
	    echo "$$configureargs" > .configureargs; \
	    if test "x$$oldconfigureargs" = "x$$configureargs"; then :; \
	    else \
	      echo Configure arguments have changed - doing make config_change_clean; \
	      $${MAKE} "MAKE=$${MAKE}" config_change_clean || exit $$?; \
	      if test "x$(METATARGET)" = "xsource"; then :; \
	      elif test "x$(METATARGET)" = "xexport"; then :; \
	      else \
		echo Configure arguments have changed - doing make depend; \
		$${MAKE} "MAKE=$${MAKE}" depend || exit $$?; \
	      fi; \
	    fi; \
	  fi; \
	} || exit $$?

# This target should always be executed indirectly through a $(DO_MAKE).
_make_in_builddir: configure
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
	    $${MAKE} $(MAKE_PARALLEL) "MAKE=$${MAKE}" "EXPORT_NAME=$(EXPORT_NAME)" $$target || { \
	      res=$$?; \
	      if test -f remake; then \
		$${MAKE} $(MAKE_PARALLEL) "MAKE=$${MAKE}" "EXPORT_NAME=$(EXPORT_NAME)" $$target || \
		  exit $$?; \
	      else \
		exit $$res; \
	      fi; \
	    }; \
	  done; \
	} || exit $$?

# FIXME: The refdoc stuff ought to use $(BUILDDIR) too.

documentation:
	@$(DO_MAKE) "METATARGET=documentation" _make_in_builddir

doc: documentation

doxygen:
	doxygen refdoc/doxygen.cfg

legal: bin/pike
	bin/pike -e 'Stdio.write_file("COPYRIGHT", \
	  Tools.Legal.Copyright.get_text());'
	bin/pike -e 'Stdio.write_file("COPYING", \
	  Tools.Legal.License.get_text());'

release_checks: bin/pike
	bin/pike tools/release_checks.pike

gtkdoc: bin/pike
	bin/pike src/post_modules/GTK2/build_pgtk.pike --source=src/post_modules/GTK2/source \
		--destination=src/post_modules/GTK2/refdoc \
		output/doc-pikeref.pike

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
	@$(DO_MAKE) "METATARGET=pike" _make_in_builddir

libpike: force
	@$(DO_MAKE) "METATARGET=libpike.so" _make_in_builddir

install: bin/pike
	@$(DO_MAKE) "METATARGET=install" _make_in_builddir

install_nodoc: bin/pike
	@INSTALLARGS="--no-autodoc $(INSTALLARGS)"
	@$(DO_MAKE) "METATARGET=install" _make_in_builddir

install_interactive: bin/pike
	@$(DO_MAKE) "METATARGET=install_interactive" _make_in_builddir

install_interactive_nodoc: bin/pike
	@INSTALLARGS="--no-audodoc $(INSTALLARGS)"
	@$(DO_MAKE) "METATARGET=install_interactive" _make_in_builddir

tinstall: bin/pike
	@$(DO_MAKE) "METATARGET=tinstall" _make_in_builddir

testsuites:
	@$(DO_MAKE) "METATARGET=testsuites" _make_in_builddir
	@$(DO_MAKE) "METATARGET=testsuite" _make_in_builddir

just_verify:
	@$(DO_MAKE) "METATARGET=just_verify" _make_in_builddir

valgrind_just_verify:
	@$(DO_MAKE) "METATARGET=valgrind_just_verify" _make_in_builddir

verify:
	@$(DO_MAKE) "METATARGET=verify" _make_in_builddir

valgrind_verify:
	@$(DO_MAKE) "METATARGET=valgrind_verify" _make_in_builddir

verify_installed:
	@$(DO_MAKE) "METATARGET=verify_installed" _make_in_builddir

check: verify
	-@$(BIN_TRUE)

sure: verify
	-@$(BIN_TRUE)

verbose_verify:
	@$(DO_MAKE) "METATARGET=verbose_verify" _make_in_builddir

gdb_verify:
	@$(DO_MAKE) "METATARGET=gdb_verify" _make_in_builddir

dump_modules:
	@$(DO_MAKE) "METATARGET=dump_modules" _make_in_builddir

force_dump_modules:
	@$(DO_MAKE) "METATARGET=force_dump_modules" _make_in_builddir

delete_dumped_modules:
	@$(DO_MAKE) "METATARGET=delete_dumped_modules" _make_in_builddir

undump_modules:
	@$(DO_MAKE) "METATARGET=undump_modules" _make_in_builddir

run_hilfe:
	@$(DO_MAKE) "METATARGET=run_hilfe" _make_in_builddir

gdb_hilfe:
	@$(DO_MAKE) "METATARGET=gdb_hilfe" _make_in_builddir

source:
	cd src && ./run_autoconfig .
	@PIKE_BUILD_OS=source $(DO_MAKE) \
	  "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "LIMITED_TARGETS=yes" "METATARGET=source" _make_in_builddir

export:
	cd src && ./run_autoconfig .
	@EXPORT_PREREQ=yepp ; echo ; \
	if [ -f "$(BUILDDIR)/autodoc.xml" ]; then : ; else \
	  echo 'No documentation source built.'; \
	  EXPORT_PREREQ=FAIL ; \
	fi ; \
	if [ "$$EXPORT_PREREQ" = "FAIL" ]; then : \
	  echo 'Fix the above error(s) and rerun "make export", or'; \
	  echo '"make export_forced" to bypass these checks.';\
	  echo ; \
	  exit 1; \
	fi
	@PIKE_BUILD_OS=source $(DO_MAKE) \
	  "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "LIMITED_TARGETS=yes" "METATARGET=export" _make_in_builddir

export_forced:
	cd src && ./run_autoconfig .
	@PIKE_BUILD_OS=source $(DO_MAKE) \
	  "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "LIMITED_TARGETS=yes" "METATARGET=export" _make_in_builddir

snapshot_export:
	cd src && ./run_autoconfig .
	@PIKE_BUILD_OS=source $(DO_MAKE) \
	  "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "LIMITED_TARGETS=yes" "METATARGET=snapshot_export" \
	  "EXPORT_NAME=Pike-v%maj.%min-snapshot-%Y%M%D" _make_in_builddir

snapshot: snapshot_export

xenofarm_export:
	cd src && ./run_autoconfig .
	@echo Begin export
	@rm -f export_result.txt
	@PIKE_BUILD_OS=source $(DO_MAKE) \
	  "CONFIGUREARGS=--disable-binary $(CONFIGUREARGS)" \
	  "LIMITED_TARGETS=yes" "METATARGET=snapshot_export" \
	  "EXPORT_NAME=Pike%branch-%Y%M%D-%h%m%s" \
	  "EXPORTARGS=--snapshot $(EXPORTARGS)" \
	  _make_in_builddir >>export_result.txt 2>&1
	@echo Export done

bin_export:
	@$(DO_MAKE) "METATARGET=bin_export" _make_in_builddir

wix:
	@$(DO_MAKE) "METATARGET=wix" _make_in_builddir

feature_list:
	@$(DO_MAKE) "METATARGET=feature_list" _make_in_builddir

solaris_pkg_configure:
	@$(DO_MAKE) "CONFIGUREARGS=--prefix=/opt $(CONFIGUREARGS)" \
	  "METATARGET=configure" _make_in_builddir

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
	-CONFIGUREARGS="$(CONFIGUREARGS)" \
	  MAKE="$(MAKE_CMD)" BUILDDIR="$(BUILDDIR)" /bin/sh bin/xenofarm.sh
	cd xenofarm_result && tar cf - . > ../xenofarm_result.tar
	gzip -f9 xenofarm_result.tar

benchmark:
	@$(DO_MAKE) "METATARGET=run_bench" _make_in_builddir

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

distclean:
	@$(DO_MAKE) delete_builddir
	@PIKE_BUILD_OS=source $(DO_MAKE) delete_builddir
	-rm -f bin/pike

srcclean:
	@git clean -X -f

gitclean: srcclean distclean docclean
	-rm -rf build
	-rm -f export_result.txt
	-rm -f Pike*.tar.gz
	-rm -rf xenofarm_result
	-rm -f xenofarm_result.tar
	-rm -f xenofarm_result.tar.gz

docclean:
	cd refdoc ; $(DO_MAKE) clean
	@$(DO_MAKE) "METATARGET=doc_clean" _make_in_builddir

depend:
	@if test -d build; then \
	  $(DO_MAKE) _depend; \
	else \
	  echo You never need to do \"make depend\" before the first build, ; \
	  echo and doing \"make depend\" in a Pike dist will actually break ; \
	  echo the dist. ;\
	  exit 1; fi;

_depend: configure
	-@cd "$(BUILDDIR)" && \
	$${MAKE} $(MAKE_PARALLEL) "MAKE=$${MAKE}" depend || { \
	  res=$$?; \
	  if test -f remake; then \
	    $${MAKE} $(MAKE_PARALLEL) "MAKE=$${MAKE}" depend; \
	  else \
	    exit $$res; \
	  fi; \
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
