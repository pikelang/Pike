#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.18 2001/07/27 03:18:54 nilsson Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: split

tree-split: join _tree-split

_tree-split:
	@rm -rf manual-tree-split || /bin/true
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  manual.xml tree-split-template.html manual-tree-split
	@mkdir manual-tree-split/images
	@cp manual/images/* manual-tree-split/images

html: split _html

_html: html_manual manual/Makefile
	@cp manual/images/* html_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ \
	  manual.xml > html_manual/manual.html
	@(cd manual && $(MAKE) PIKE="$(PIKE)" html)

split: join manual
	@$(PIKE) ../bin/split-autodoc.pike manual.xml structure.xml .

join: recursive_join
	@$(MAKE) PIKE="$(PIKE)" XMLFILES="`echo build/*/*.xml`" manual.xml

recursive_join: images
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" join)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" join)

images: extract manual/images
	@cp ../src/post_modules/GTK/refdoc/images/*.png \
	  build/src/post_modules/GTK/refdoc/images
	@$(PIKE) ../bin/autodoc_images.pike build/ manual/images

extract: build/src/Makefile build/lib/Makefile
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" extract)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" extract)

manual.xml: $(XMLFILES) ../bin/join-autodoc.pike
	@$(PIKE) ../bin/join-autodoc.pike --post-process manual.xml $(XMLFILES)

build/src/Makefile: Makefile.in build/src
	@sed -e "s#@SRCDIR@#../../../src#" -e "s#@ROOT@#../..#" \
	  <Makefile.in >build/src/Makefile

build/lib/Makefile: Makefile.in build/lib
	@sed -e "s#@SRCDIR@#../../../lib#" -e "s#@ROOT@#../..#" \
	  <Makefile.in >build/lib/Makefile

manual/Makefile: PostMakefile.in manual
	@sed -e "s#@IMGDIR@#images#" -e "s#@ROOT@#../#" \
	  -e "s#@DEST@#../html_manual/#" <PostMakefile.in >manual/Makefile

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
	@rm -rf manual || /bin/true
	@rm -rf html_manual || /bin/true
	@rm -rf manual-tree-split || /bin/true
