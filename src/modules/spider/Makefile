# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I. -I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=spider.o discdate.o stardate.o sdebug.o

spider.a: $(FILES)
	-rm -f spider.a
	ar cq spider.a $(FILES)
	-ranlib spider.a

clean:
	-rm -f *.o *.a

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c  >$(SRCDIR)/dependencies

include ./dependencies

