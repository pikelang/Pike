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
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c | sed 's;$(SRCDIR)/;;g' | sed 's;\.\./;;g' | sed 's;\./;;g'>$(SRCDIR)/dependencies

include ./dependencies

