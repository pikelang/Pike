#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.26 2001/09/09 02:46:18 nilsson Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: tree-split html

tree-split: join _tree-split

_tree-split:
	rm -rf manual-tree-split || /bin/true
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  manual.xml tree-split-template.html manual-tree-split
	@cp tree-split-style.css manual-tree-split/style.css
	@test -d manual-tree-split/images || mkdir manual-tree-split/images
	cp manual/images/* manual-tree-split/images
	@cp src_images/next.gif src_images/prev.gif manual-tree-split/images

html: html_manual
	cp manual/images/* html_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ manual.xml

join: recursive_join
	@$(MAKE) PIKE="$(PIKE)" XMLFILES="`echo build/*/*.xml not_extracted/*.xml`" \
	  autodoc.xml

recursive_join: images
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" join)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" join)

images: extract manual/images
	@cp ../src/post_modules/GTK/refdoc/images/*.png \
	  build/src/post_modules/GTK/refdoc/images
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" images)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" images)

extract: build/src/Makefile build/lib/Makefile
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" extract)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" extract)

manual.xml: autodoc.xml structure.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure.xml autodoc.xml > manual.xml

autodoc.xml: $(XMLFILES) bin/join.pike
	@$(PIKE) bin/join.pike --post-process autodoc.xml $(XMLFILES)

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

build:
	@test -d build || mkdir build

manual:
	@test -d manual || mkdir manual

manual/images: manual
	@test -d manual/images || mkdir manual/images

html_manual:
	@test -d html_manual || mkdir html_manual
	@test -d html_manual/images || mkdir html_manual/images

# Cleanup

clean:
	@rm -rf build || /bin/true

spotless: clean
	@rm -f manual.xml || /bin/true
	@rm -f autodoc.xml || /bin/true
	@rm -rf manual || /bin/true
	@rm -rf html_manual || /bin/true
	@rm -rf manual-tree-split || /bin/true
