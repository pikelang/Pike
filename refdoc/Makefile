#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.8 2001/04/18 15:53:05 grubba Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: split

split: join manual
	@$(PIKE) ../bin/split-autodoc.pike manual.xml structure.xml autodoc

join: recursive_join ../bin/join-autodoc.pike
	@$(PIKE) ../bin/join-autodoc.pike manual.xml \
	  `echo build/src/*.xml build/lib/*.xml`

recursive_join: images
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" join)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" join)

images: extract

extract: build/src/Makefile build/lib/Makefile
	@(cd build/src && $(MAKE) PIKE="$(PIKE)" extract)
	@(cd build/lib && $(MAKE) PIKE="$(PIKE)" extract)

build/src/Makefile: Makefile.in build/src
	@sed -e "s#@SRCDIR@#../../../src#" -e "s#@ROOT@#../..#" \
	  <Makefile.in >build/src/Makefile

build/lib/Makefile: Makefile.in build/lib
	@sed -e "s#@SRCDIR@#../../../lib#" -e "s#@ROOT@#../..#" \
	  <Makefile.in >build/lib/Makefile

build/src: build
	@test -d build/src || mkdir build/src

build/lib: build
	@test -d build/lib || mkdir build/lib

build:
	@test -d build || mkdir build

manual:
	@test -d manual || mkdir manual

clean:
	@rm -rf build || /bin/true
