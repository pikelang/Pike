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
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c >$(SRCDIR)/dependencies

include ./dependencies

