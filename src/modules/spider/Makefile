# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I. -I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=spider.o

spider.a: $(FILES)
	-rm -f spider.a
	ar cq spider.a $(FILES)
	-ranlib spider.a

clean:
	-rm -f *.o

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c | sed 's;$(SRCDIR)/;;g' | sed 's;\.\./;;g' | sed 's;\./;;g' >$(SRCDIR)/dependencies

include ./dependencies

