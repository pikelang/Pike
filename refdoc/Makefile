#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id$
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: modref traditional

pike.ida.liu.se: modref pelix_modref traditional

caudium.net: modref caudium_modref traditional

# These are the actual build targets for making manuals...

modref: build/modref.xml structure/modref.css structure/modref.html \
	  presentation/make_html.pike presentation/tree-split-autodoc.pike
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

pelix_modref: build/modref.xml structure/pelix_modref.html \
	        presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" low_pelix_modref

low_pelix_modref:
	rm -rf pelix_modref || /bin/true
	@mkdir pelix_modref
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  build/modref.xml structure/pelix_modref.html pelix_modref
	@mkdir pelix_modref/images
	cp build/images/* pelix_modref/images

caudium_modref: build/modref.xml structure/caudium_modref.html \
		presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" low_caudium_modref

low_caudium_modref:
	rm -rf caudium_modref || /bin/true
	@mkdir caudium_modref
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  build/modref.xml structure/caudium_modref.html caudium_modref
	@mkdir caudium_modref/images
	cp build/images/* caudium_modref/images

one_page: onepage build/onepage.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" low_one_page

low_one_page:
	cp build/images/* onepage/
	@$(PIKE) presentation/make_html.pike --img= build/onepage.xml

traditional: traditional_manual build/traditional.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" low_traditional

low_traditional:
	cp build/images/* traditional_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ build/traditional.xml

# Sub targets used by the above targets.

build/autodoc.xml: build/src build/lib build/images images always
	$(PIKE) -x extract_autodoc ../src/ src_images/ \
	  build/src/ build/images/
	$(PIKE) -x extract_autodoc ../lib/ src_images/ \
	  build/lib/ build/images/
	$(PIKE) -x join_autodoc --post-process build/autodoc.xml \
	  build/src/ build/lib/

always:

images: build/src/post_modules/GTK/refdoc/images
	@cp ../src/post_modules/GTK/refdoc/images/*.png \
	  build/src/post_modules/GTK/refdoc/images

build/onepage.xml: build/autodoc.xml structure/onepage.xml
	@$(PIKE) -x assemble_autodoc structure/onepage.xml build/autodoc.xml \
	  > build/onepage.xml

build/traditional.xml: build/autodoc.xml structure/traditional.xml
	@$(PIKE) -x assemble_autodoc structure/traditional.xml build/autodoc.xml \
	  > build/traditional.xml

build/modref.xml: build/autodoc.xml structure/modref.xml
	@$(PIKE) -x assemble_autodoc structure/modref.xml build/autodoc.xml \
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


# Cleanup

clean:
	@rm -rf build || /bin/true

spotless: clean
	-test -f .cvsignore && rm -rf `cat .cvsignore`
