# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=sprintf.o

sprintf.a: $(FILES)
	-rm -f sprintf.a
	ar cq sprintf.a $(FILES)
	-ranlib sprintf.a

clean:
	-rm -f *.o *.a

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c  >$(SRCDIR)/dependencies
	cat $(SRCDIR)/Makefile.in.src $(SRCDIR)/dependencies > $(SRCDIR)/Makefile.in
	config.status


sprintf.o: ./sprintf.c ./../../global.h ./../../config.h \
 ./../../machine.h ./../../port.h ./../../error.h ./../../svalue.h \
 ./../../array.h ./../../las.h ./../../dynamic_buffer.h \
 ./../../program.h ./../../stralloc.h ./../../lpc_types.h \
 ./../../add_efun.h ./../../hashtable.h ./../../interpret.h
