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
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c | sed 's;$(SRCDIR)/;;g' | sed 's;\.\./;;g' | sed 's;\./;;g' >$(SRCDIR)/dependencies

include ./dependencies


