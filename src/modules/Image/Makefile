# $Id: Makefile,v 1.15 1999/04/09 10:20:22 mirar Exp $

DOCFILES = doc-header            \
   image.c  			 \
   acconfig.h			 \
   blit.c			 \
   blit_layer_include.h		 \
   colors.c			 \
   colors.h			 \
   colortable.c			 \
   colortable.h			 \
   colortable_lookup.h		 \
   dct.c			 \
   font.c			 \
   image.h			 \
   match.h			 \
   matrix.c			 \
   operator.c			 \
   orient.c			 \
   pattern.c			 \
   phase.h			 \
   pnm_compat.c			 \
   polyfill.c			 \
   search.c			 \
   togif.c			 \
   x.c				 \
   encodings/_xpm.c		 \
   encodings/any.c		 \
   encodings/bmp.c		 \
   encodings/gif.c		 \
   encodings/gif_lzw.c		 \
   encodings/gif_lzw.h		 \
   encodings/iff.c		 \
   encodings/ilbm.c		 \
   encodings/pcx.c		 \
   encodings/png.c		 \
   encodings/pnm.c		 \
   encodings/tga.c		 \
   encodings/x.c		 \
   encodings/xbm.c		 \
   encodings/xwd.c		 \
   ../_Image_JPEG/acconfig.h	 \
   ../_Image_JPEG/image_jpeg.c	 \
   ../_Image_TIFF/acconfig.h	 \
   ../_Image_TIFF/image_tiff.c	 \
   ../_Image_TTF/acconfig.h	 \
   ../_Image_TTF/image_ttf.c	 \
   ../_Image_XFace/acconfig.h	 \
   ../_Image_XFace/image_xface.c \
   doc-footer                     

WMMLDOCPLACE = ../../../tutorial/Image.wmml

pike=pike

default:
	echo "This makefile only builds documentation. Please run configure."
	echo "To build HTML documentation, type 'make doc'."
	echo "To build WMML documentation, type 'make wmml'."

doc:	$(DOCFILES) mkdoc.pike Makefile
	$(pike) mkdoc.pike $(DOCFILES)

$(WMMLDOCPLACE): $(DOCFILES) ../../../bin/mkwmml.pike Makefile
	$(pike) ../../../bin/mkwmml.pike --nonverbose $(DOCFILES) \
	  >$(WMMLDOCPLACE)

wmml:	$(WMMLDOCPLACE)

module_testsuite.in: testsuite.in.in mktests.pike
	pike mktests.pike -t testsuite.in.in >module_testsuite.in

tests.pike: testsuite.in.in mktests.pike
	pike mktests.pike testsuite.in.in >tests.pike

test:	tests.pike
	pike tests.pike
