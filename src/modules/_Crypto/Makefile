# The compiler
CC=gcc

# You need the pike source somewhere
PIKESRCDIR=../../pike/src

# And you need a dir where Pike was compiled (might be same as sourcedir)
PIKEBUILDDIR=../../pike/build/sol2.5

# The SSLeay root is here
SSLROOT=/usr/local/ssl/

# Include search path
IFLAGS=-I$(PIKESRCDIR) -I$(PIKEBUILDDIR) -I$(SSLROOT)/include

# Additional C preprocessor flags
CPPFLAGS=

# The flags to generate a shared library
CFLAGS=-Wall -O -shared -fpic $(IFLAGS) $(CPPFLAGS) -pedantic -Wall

LDFLAGS=-shared -fpic -R$(SSLROOT)/lib -L$(SSLROOT)/lib
LOADLIBES=-lcrypto -lssl

OBJS=crypto.o md5.o md2.o idea.o des.o invert.o

# The actual rules

crypto.so: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o crypto.so $(LOADLIBES)

$(OBJS) : precompiled_crypto.h
