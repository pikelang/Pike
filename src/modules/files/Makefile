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
	gcc -MM $(PREFLAGS) $(SRCDIR)/*.c | $(FIXDEP) $(SRCDIR)/Makefile.in



#Dependencies begin here, DO NOT REMOVE THIS LINE!!!!
datagram.o: datagram.c
efuns.o: efuns.c \
  global.h machine.h \
  config.h \
  debug.h \
  port.h \
  interpret.h \
  program.h \
  svalue.h \
  stralloc.h \
  array.h \
  las.h \
  dynamic_buffer.h \
  mapping.h \
  macros.h \
  fd_control.h file_machine.h
file.o: file.c \
  global.h machine.h \
  config.h \
  debug.h \
  port.h \
  interpret.h \
  program.h \
  svalue.h \
  stralloc.h \
  array.h \
  las.h \
  dynamic_buffer.h \
  object.h \
  macros.h \
  backend.h \
  fd_control.h file_machine.h \
  file.h \
  error.h \
  lpc_signal.h
socket.o: socket.c \
  global.h machine.h \
  config.h \
  debug.h \
  port.h \
  interpret.h \
  program.h \
  svalue.h \
  stralloc.h \
  array.h \
  las.h \
  dynamic_buffer.h \
  object.h \
  macros.h \
  backend.h \
  fd_control.h file_machine.h \
  file.h
