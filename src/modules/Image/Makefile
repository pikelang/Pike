# $Id: Makefile,v 1.19 1999/07/15 19:01:45 mirar Exp $

# grep -l '**!' {,../_Image*/}**/*.{c,h}  

DOCFILES = doc-header            \
	../					\
	../../../lib/modules/_Image.pmod/module.pmod \
	blit.c					\
	blit_layer_include.h			\
	colors.c				\
	colors.h				\
	colortable.c				\
	colortable.h				\
	colortable_lookup.h			\
	dct.c					\
	default_font.c				\
	encodings/_xpm.c			\
	encodings/any.c				\
	encodings/avs.c				\
	encodings/bmp.c				\
	encodings/gd.c				\
	encodings/gif.c				\
	encodings/gif_lzw.c			\
	encodings/gif_lzw.h			\
	encodings/hrz.c				\
	encodings/iff.c				\
	encodings/ilbm.c			\
	encodings/pcx.c				\
	encodings/png.c				\
	encodings/pnm.c				\
	encodings/psd.c				\
	encodings/psd_constant_strings.h	\
	encodings/tga.c				\
	encodings/x.c				\
	encodings/xbm.c				\
	encodings/xcf.c				\
	encodings/xcf_constant_strings.h	\
	encodings/xwd.c				\
	font.c					\
	image.c					\
	image.h					\
	image_module.c				\
	initstuff.h				\
	layer_channel.h				\
	layer_oper.h				\
	layers.c				\
	match.h					\
	matrix.c				\
	operator.c				\
	orient.c				\
	pattern.c				\
	phase.h					\
	poly.c					\
	polyfill.c				\
	search.c				\
	../_Image_JPEG/acconfig.h		\
	../_Image_JPEG/image_jpeg.c		\
	../_Image_TIFF/acconfig.h		\
	../_Image_TIFF/image_tiff.c		\
	../_Image_TTF/acconfig.h		\
	../_Image_TTF/image_ttf.c		\
	../_Image_XFace/acconfig.h		\
	../_Image_XFace/image_xface.c		\
	../../../lib/modules/_Image_PS.pmod	\
	../../../lib/modules/_Image_PSD.pmod	\
	../../../lib/modules/_Image_XCF.pmod	\
	../../../lib/modules/_Image_XPM.pmod	\
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
