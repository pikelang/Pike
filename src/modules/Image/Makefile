# $Id: Makefile,v 1.18 1999/06/12 19:30:27 mirar Exp $

# grep -l '**!' {,../_Image*/}**/*.{c,h}  

DOCFILES = doc-header            \
	blit.c					\
	colors.c				\
	colortable.c				\
	dct.c					\
	default_font.c				\
	encodings/_xpm.c			\
	encodings/any.c				\
	encodings/avs.c				\
	encodings/bmp.c				\
	encodings/gd.c				\
	encodings/gif.c				\
	encodings/gif_lzw.c			\
	encodings/hrz.c				\
	encodings/iff.c				\
	encodings/ilbm.c			\
	encodings/pcx.c				\
	encodings/png.c				\
	encodings/pnm.c				\
	encodings/psd.c				\
	encodings/tga.c				\
	encodings/x.c				\
	encodings/xbm.c				\
	encodings/xcf.c				\
	encodings/xwd.c				\
	font.c					\
	image.c					\
	image_module.c				\
	layers.c				\
	matrix.c				\
	operator.c				\
	orient.c				\
	pattern.c				\
	polyfill.c				\
	search.c				\
	blit_layer_include.h			\
	colortable.h				\
	colortable_lookup.h			\
	encodings/gif_lzw.h			\
	image.h					\
	layer_channel.h				\
	layer_oper.h				\
	match.h					\
	phase.h					\
	../_Image_JPEG/image_jpeg.c		\
	../_Image_TIFF/image_tiff.c		\
	../_Image_TTF/image_ttf.c		\
	../_Image_XFace/image_xface.c		\
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
