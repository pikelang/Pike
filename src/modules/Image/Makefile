# $Id: Makefile,v 1.9 1998/04/16 00:38:52 mirar Exp $

DOCFILES = doc-header \
   image.c \
   ../_Image_JPEG/acconfig.h \
   ../_Image_JPEG/image_jpeg.c \
   ../_Image_XFace/acconfig.h \
   ../_Image_XFace/image_xface.c \
   acconfig.h \
   blit.c \
   blit_layer_include.h \
   colortable.c \
   colortable.h \
   colortable_lookup.h \
   dct.c \
   encodings/gif.c \
   encodings/gif_lzw.c \
   encodings/gif_lzw.h \
   encodings/png.c \
   encodings/pnm.c \
   encodings/x.c \
   encodings/xwd.c \
   font.c \
   image.h \
   matrix.c \
   myorient.c \
   operator.c \
   orient.c \
   pattern.c \
   pnm.c \
   polyfill.c \
   togif.c \
   x.c \
   doc-footer

WMMLDOCPLACE = ../../../tutorial/Image.wmml

pike=pike

default:
	echo "This makefile only builds documentation. Please run configure."
	echo "To build HTML documentation, type 'make doc'."
	echo "To build WMML documentation, type 'make wmml'."

doc:	$(DOCFILES) mkdoc.pike Makefile
	$(pike) mkdoc.pike $(DOCFILES)

$(WMMLDOCPLACE): $(DOCFILES) mkwmml.pike Makefile
	$(pike) mkwmml.pike --nonverbose $(DOCFILES) >$(WMMLDOCPLACE)

wmml:	$(WMMLDOCPLACE)

