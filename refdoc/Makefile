#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.39 2002/02/24 23:22:28 nilsson Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: modref traditional one_page

# These are the actual build targets for making manuals...

modref: build/modref.xml
	@$(MAKE) PIKE="$(PIKE)" low_modref

low_modref:
	rm -rf modref || /bin/true
	@mkdir modref
	@cp structure/modref.css modref/style.css
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  build/modref.xml structure/modref.html modref
	@mkdir modref/images
	cp build/images/* modref/images
	@for file in pike_logo.gif next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif ; do \
	  cp src_images/$$file modref/images/ ; done

wiki: build/modref.xml
	@$(MAKE) PIKE="$(PIKE)" low_wiki

low_wiki:
	rm -rf wiki || /bin/true
	@mkdir wiki
	@$(PIKE) presentation/wiki.pike \
	  build/modref.xml structure/wiki.html wiki
	@mkdir wiki/images
	cp build/images/* wiki/images

one_page: onepage build/onepage.xml
	@$(MAKE) PIKE="$(PIKE)" low_one_page

low_one_page:
	cp build/images/* onepage/
	@$(PIKE) presentation/make_html.pike --img= build/onepage.xml

traditional: traditional_manual build/traditional.xml
	@$(MAKE) PIKE="$(PIKE)" low_traditional

low_traditional:
	cp build/images/* traditional_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ build/traditional.xml

# Sub targets used by the above targets.

build/autodoc.xml: recursive_join build/src/sub_manual.xml build/lib/sub_manual.xml bin/join.pike
	@$(PIKE) bin/join.pike --post-process build/autodoc.xml build/src/sub_manual.xml \
	  build/lib/sub_manual.xml

recursive_join: extract bin/join.pike
	@$(PIKE) bin/join.pike build/src/sub_manual.xml build/src/
	@$(PIKE) bin/join.pike build/lib/sub_manual.xml build/lib/

extract: build/lib build/src build/images images bin/extract.pike bin/mirardoc.pike
	@$(PIKE) bin/extract.pike ../src/ build/src/ build/images/
	@$(PIKE) bin/extract.pike ../lib/ build/lib/ build/images/

images: build/src/post_modules/GTK/refdoc/images
	@cp ../src/post_modules/GTK/refdoc/images/*.png \
	  build/src/post_modules/GTK/refdoc/images

build/onepage.xml: build/autodoc.xml structure/onepage.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure/onepage.xml build/autodoc.xml \
	  > build/onepage.xml

build/traditional.xml: build/autodoc.xml structure/traditional.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure/traditional.xml build/autodoc.xml \
	  > build/traditional.xml

build/modref.xml: build/autodoc.xml structure/modref.xml bin/assembler.pike
	@$(PIKE) bin/assembler.pike structure/modref.xml build/autodoc.xml \
	  > build/modref.xml

# Directories

build/src: build
	@test -d build/src || mkdir build/src

build/lib: build
	@test -d build/lib || mkdir build/lib

build/images: build
	@test -d build/images || mkdir build/images

build:
	@test -d build || mkdir build

onepage:
	@test -d onepage || mkdir onepage

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

# Development targets

touch-mirardoc:
	@touch `grep -r \\*\\*\! ../src/modules/* | cut -d: -f1 | \
	  uniq`
	@touch `grep -r "//\! module" ../lib/modules/* | cut -d: -f1 | \
	  uniq`

nilsson:
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  build/modref.xml structure/modref.html modref


# Cleanup

clean:
	@rm -rf build || /bin/true

spotless: clean
	@rm -rf onepage || /bin/true
	@rm -rf modref || /bin/true
	@rm -rf traditional_manual || /bin/true
