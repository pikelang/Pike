# The compiler
CC=gcc

# You need the pike source somewhere
PIKESRCDIR=../../pike/src

# And you need a dir where Pike was compiled (might be same as sourcedir)
PIKEBUILDDIR=../../../work/pike

# The SSLeay root is here
#SSLROOT=/usr/local/ssl/

# Include search path
IFLAGS=-I$(PIKESRCDIR) -I$(PIKEBUILDDIR) -Iinclude -Ilib/descore

# Additional C preprocessor flags
CPPFLAGS=

# The flags to generate a shared library
CFLAGS=-Wall -O -g -shared -fpic $(IFLAGS) $(CPPFLAGS) -pedantic -Wall

LD=/usr/ccs/bin/ld

LDFLAGS= -Llib -G # -R$(SSLROOT)/lib -L$(SSLROOT)/lib
#LOADLIBES=-lcrypto -lssl

OBJS=crypto.o idea.o des.o invert.o rc4.o cbc.o pipe.o sha.o # md5.o md2.o
MODS=crypto.so
#MODS=crypto_crypto.so crypto_idea.so crypto_des.so crypto_invert.so \
#crypto_cbc.so crypto_pipe.so crypto_sha.so # crypto_md5.so crypto_md2.so

.SUFFIXES: .c .o .so
all: $(MODS)

# The actual rules

crypto.so : $(OBJS) lib/libib.a
	$(LD) $(LDFLAGS) $(OBJS) -lib -o $@

crypto_%.so: %.o
	$(LD) $(LDFLAGS) $< -lib  -o $@

lib/libib.a:
	cd lib; $(MAKE) libib.a

$(MODS) : lib/libib.a
$(OBJS) : precompiled_crypto.h
