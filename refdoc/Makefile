#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.33 2001/11/05 00:51:40 nilsson Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: modref traditional

modref: build/modref.xml
	rm -rf modref || /bin/true
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  build/modref.xml tree-split-template.html modref
	@cp tree-split-style.css modref/style.css
	@mkdir modref/images
	cp build/images/* modref/images
	@cp src_images/next.gif src_images/prev.gif modref/images

html: html_manual manual.xml
	cp build/images/* html_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ manual.xml

traditional: traditional_manual build/traditional.xml
	cp build/images/* traditional_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ build/traditional.xml

build/autodoc.xml: recursive_join
	@$(MAKE) PIKE="$(PIKE)" XMLFILES="`echo build/*/*.xml`" \
	  autodoc.xml

recursive_join: extract
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" join)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" join)

extract: build/src/Makefile build/lib/Makefile build/images images
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" extract)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" extract)

images: build/src/post_modules/GTK/refdoc/images
	@cp ../src/post_modules/GTK/refdoc/images/*.png \
	  build/src/post_modules/GTK/refdoc/images

manual.xml: build/autodoc.xml structure/onepage.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure/onepage.xml build/autodoc.xml > manual.xml

build/traditional.xml: build/autodoc.xml structure/traditional.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure/traditional.xml build/autodoc.xml > build/traditional.xml

build/modref.xml: build/autodoc.xml structure/modref.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure/modref.xml build/autodoc.xml > build/modref.xml

autodoc.xml: $(XMLFILES) bin/join.pike
	@$(PIKE) bin/join.pike --post-process build/autodoc.xml $(XMLFILES)

build/src/Makefile: Makefile.in build/src
	@sed -e "s#@SRCDIR@#../../../src#" \
	     -e "s#@ROOT@#../..#" \
	     -e "s#@BUILDROOT@#..#" \
	  <Makefile.in >build/src/Makefile

build/lib/Makefile: Makefile.in build/lib
	@sed -e "s#@SRCDIR@#../../../lib#" \
	     -e "s#@ROOT@#../..#" \
	     -e "s#@BUILDROOT@#..#" \
	  <Makefile.in >build/lib/Makefile

# Directories

build/src: build
	@test -d build/src || mkdir build/src

build/lib: build
	@test -d build/lib || mkdir build/lib

build/images: build
	@test -d build/images || mkdir build/images

build:
	@test -d build || mkdir build

manual:
	@test -d manual || mkdir manual

html_manual:
	@test -d html_manual || mkdir html_manual
	@test -d html_manual/images || mkdir html_manual/images

traditional_manual:
	@test -d traditional_manual || mkdir traditional_manual
	@test -d traditional_manual/images || mkdir traditional_manual/images

build/src/post_modules/GTK/refdoc/images: build/src
	@test -d build/src/post_modules || \
	  mkdir build/src/post_modules
	@test -d build/src/post_modules/GTK || \
	  mkdir build/src/post_modules/GTK
	@test -d build/src/post_modules/GTK/refdoc || \
	  mkdir build/src/post_modules/GTK/refdoc
	@test -d build/src/post_modules/GTK/refdoc/images || \
	  mkdir build/src/post_modules/GTK/refdoc/images

# Cleanup

clean:
	@rm -rf build || /bin/true

spotless: clean
	@rm -f manual.xml || /bin/true
	@rm -rf manual || /bin/true
	@rm -rf html_manual || /bin/true
	@rm -rf modref || /bin/true
	@rm -rf traditional_manual || /bin/true
