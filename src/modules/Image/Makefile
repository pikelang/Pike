
DOCFILES = doc-header \
   blit.c blit_layer_include.h colortable.c colortable.h dct.c font.c \
   image.c image.h matrix.c operator.c pattern.c pnm.c \
   polyfill.c togif.c x.c encodings/gif.c encodings/gif_lzw.c \
   encodings/gif_lzw.h encodings/pnm.c ../_Image_JPEG/image_jpeg.c \
   encodings/x.c \
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

