# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=math.o

math.a: $(FILES)
	-rm -f math.a
	ar cq math.a $(FILES)
	-ranlib math.a

clean:
	-rm -f *.o

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c  >$(SRCDIR)/dependencies
	cat $(SRCDIR)/Makefile.in.src $(SRCDIR)/dependencies > $(SRCDIR)/Makefile.in
	config.status

math.o: ./math.c ./../../interpret.h ./../../program.h \
 ./../../config.h ./../../machine.h ./../../types.h ./../../add_efun.h \
 ./../../svalue.h ./../../hashtable.h
