#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.6 2001/02/26 14:12:19 grubba Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

all: split

split: join

join: images

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

clean:
	@rm -rf build || /bin/true
