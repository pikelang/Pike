# The compiler
CC=gcc

# You need the pike source somewhere
PIKESRCDIR=../../pike/src

# And you need a dir where Pike was compiled (might be same as sourcedir)
PIKEBUILDDIR=../../../work/pike

# The SSLeay root is here
SSLROOT=/usr/local/ssl/

# Include search path
IFLAGS=-I$(PIKESRCDIR) -I$(PIKEBUILDDIR) -Iinclude -I$(SSLROOT)/include

# Additional C preprocessor flags
CPPFLAGS=

# The flags to generate a shared library
CFLAGS=-Wall -O -shared -fpic $(IFLAGS) $(CPPFLAGS) -pedantic -Wall

LDFLAGS=-shared -fpic -Llib # -R$(SSLROOT)/lib -L$(SSLROOT)/lib
#LOADLIBES=-lcrypto -lssl

OBJS=crypto.o md5.o md2.o idea.o des.o invert.o cbc.o pipe.o sha.o

# The actual rules

crypto.so: $(OBJS) lib/libib.a
	$(CC) $(LDFLAGS) $(OBJS) -lib -o crypto.so

idea.so: idea.o lib/libib.a
	$(CC) $(LDFLAGS) idea.o -lib -o idea.so

sha.so: sha.o lib/libib.a
	$(CC) $(LDFLAGS) sha.o -lib -o sha.so

lib/libib.a:
	cd lib; $(MAKE) libib.a

$(OBJS) : precompiled_crypto.h
