# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=regexp.o glue.o

regexp.a: $(FILES)
	-rm -f regexp.a
	ar cq regexp.a $(FILES)
	-ranlib regexp.a

clean:
	-rm -f *.o *.a

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c  >$(SRCDIR)/dependencies
	cat $(SRCDIR)/Makefile.in.src $(SRCDIR)/dependencies > $(SRCDIR)/Makefile.in
	config.status

glue.o: ./glue.c ./../../global.h ./../../config.h ./../../machine.h \
 ./../../port.h ./../../interpret.h ./../../program.h ./../../svalue.h \
 ./../../stralloc.h ./../../array.h ./../../las.h \
 ./../../dynamic_buffer.h ./../../object.h ./../../macros.h
regexp.o: ./regexp.c ./../../global.h ./../../config.h \
 ./../../machine.h ./../../port.h ./regexp.h ./../../error.h \
 ./../../svalue.h
