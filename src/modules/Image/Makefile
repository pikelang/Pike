
DOCFILES = doc-header \
   blit.c blit_layer_include.h colortable.c colortable.h dct.c font.c \
   image.c image.h lzw.c lzw.h matrix.c operator.c pattern.c pnm.c \
   polyfill.c quant.c togif.c x.c encodings/gif.c encodings/gif_lzw.c \
   encodings/gif_lzw.h \
   doc-footer

pike=pike

default:
	echo "This makefile only builds documentation. Please run configure."
	echo "To build documentation, type 'make doc'."

docs:	$(DOCFILES) mkdoc.pike Makefile
	$(pike) mkdoc.pike $(DOCFILES)

doc:	docs

