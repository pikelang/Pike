# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=pipe.o

pipe.a: $(FILES)
	-rm -f pipe.a
	ar cq pipe.a $(FILES)
	-ranlib pipe.a

clean:
	-rm -f *.o *.a

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c  >$(SRCDIR)/dependencies
	cat $(SRCDIR)/Makefile.in.src $(SRCDIR)/dependencies > $(SRCDIR)/Makefile.in
	config.status

pipe.o: ./pipe.c ./../../global.h ./../../config.h ./../../machine.h \
 ./../../port.h ./../../stralloc.h ./../../macros.h ./../../object.h \
 ./../../svalue.h ./../../add_efun.h ./../../hashtable.h \
 ./../../interpret.h ./../../program.h ./../../error.h
