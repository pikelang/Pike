# Generated automatically from Makefile.in by configure.
SRCDIR=.
VPATH=.:./../..:../..
PREFLAGS=-I. -I$(SRCDIR) -I$(SRCDIR)/../.. -I../..
CFLAGS=$(PREFLAGS) $(OTHERFLAGS)

FILES=file.o efuns.o socket.o

files.a: $(FILES)
	-rm -f files.a
	ar cq files.a $(FILES)
	-ranlib files.a

clean:
	-rm -f *.o *.a

depend:
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c | sed 's;$(SRCDIR)/;;g' | sed 's;\.\./;;g' | sed 's;\./;;g' >$(SRCDIR)/dependencies

include ./dependencies

