#
#	des - fast & portable DES encryption & decryption.
#	Copyright (C) 1992  Dana L. How
#	Please see the file `README' for the complete copyright notice.
#

# $Id: Makefile,v 1.1 1997/01/10 02:51:59 nisse Exp $

CFLAGS=		-g		# debug
CFLAGS=		-O4		# max opt
CFLAGS=		-O4
CPPFLAGS=	-Dsparc		# use 6+8 general regs
CPPFLAGS=	-Dmc68000	# use 3+4 addr (1+4 live), and 3+3 data regs
CPPFLAGS=	-Dvax		# use 6+0 general regs
CPPFLAGS=	-Di386		# use 3+0 regs, and 3+0 normal variables
CPPFLAGS=	-Dsparc
LDFLAGS=
CC=		gcc
CPP=		$(CC) -E $(CPPFLAGS)
CODEGEN.c=	$(CC) $(CFLAGS) $(CPPFLAGS) -S
COMPILE.c=	$(CC) $(CFLAGS) $(CPPFLAGS) -c
LINK.c=		$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

# hand-entered files that go into the library
SC=		desKerb.c desUtil.c desQuick.c
SO=		desKerb.o desUtil.o desQuick.o

# special generated files
GH=		parity.h rotors.h keymap.h
GC=		desSmallFips.c desSmallCore.c desQuickFips.c desQuickCore.c
GI=		desSmallFips.i desSmallCore.i desQuickFips.i desQuickCore.i
GO=		desSmallFips.o desSmallCore.o desQuickFips.o desQuickCore.o

# what to distribute
RCS=		Makefile README						\
		desCore.h desinfo.h desCode.h				\
		desTest.c $(SC) desdata.c
FILES=		$(RCS)

O=		$(SO) $(GO)

# prefer compilation from .i if .i exists
.SUFFIXES:
.SUFFIXES:	.i .o .c $(SUFFIXES)

.PRECIOUS:	$(RCS) test

test:		desCore.a desTest.o
		$(LINK.c) -o $@ desTest.o desCore.a
		./test

# get stuff from RCS
$(RCS):
		co $@

# test all performance enhancement flags
sure:
		rm *.i; make 'CPPFLAGS=-Di386    -Umc68000 -Usparc'
		rm *.i; make 'CPPFLAGS=-Dvax     -Umc68000 -Usparc'
		rm *.i; make 'CPPFLAGS=-Dmc68000 -Usparc'
		rm *.i; make 'CPPFLAGS=-Dsparc   -Umc68000'

desCore.a:	$O
		ar cru $@ $O
		ranlib $@

desdata.o:	desinfo.h desCore.h
desUtil.o:	$(GH)
$(SO):		desCore.h
$(GI):		desCode.h desCore.h Makefile
desTest.o:	desCore.h

$(GH):		desdata
		./desdata $@ > $@

desdata:	desdata.o
		$(LINK.c) -o $@ desdata.o

tar:		$(FILES)
		tar cf des.tar $(FILES)

shar:		$(FILES)
		makekit -ndes.shar -s53k '-tnow run make' $(FILES)

lint:		desTest.c $(SC) $(GC)
		lint	$(CPPFLAGS) \
			desTest.c $(SC) $(GC) | \
		sed	'/possible pointer alignment problem/d'

warn:		desTest.c $(SC) $(GC)
		gcc2	-Wall -ansi -pedantic $(CPPFLAGS) -O2 \
			desTest.c $(SC) $(GC)

# new rules  (note: tr|sed|tr is NOT necessary,  just there so .i is readable)
.c.i:
	$(CPP) $< > $*.x
	@tr ';'\\012 \\012';' < $*.x |			\
	 sed	-e 's/[ 	][ 	]*/ /g'		\
		-e 's/^ //'				\
		-e 's/ $$//'				\
		-e '/^$$/d'				\
		-e '/^[^;]/s/^/;/'			\
		-e 's/#[^;]*;//g'			\
		-e 's/\([){]\) *\(register\)/\1;\2/g'	\
		-e 's/\([[(]\) /\1/g'			\
		-e 's/ \([])]\)/\1/g'			\
		-e 's/\([^]+0123 ]\) =/\1  =/g'		\
		-e 's/}/};;/g'				\
		-e 's/ *; */;/g'			\
		-e 's/;;;*/;;/g'			\
		-e '1s/^;*//' |				\
	 tr ';'\\012 \\012';' > $@
	@echo ""  >> $@
#	@echo "}" >> $@			# last definition must be a procedure

#		-e 's/\(;[kmxyz][0-9]*\)\([^;]*=\)/\1  \2/g'

.i.o:
		$(CODEGEN.c) $<
		$(COMPILE.c) $*.s

# slowest to quickest
desSmallFips.c:
		@echo '#include "desCode.h"' > $@
		@echo \
'ENCRYPT(DesSmallFipsEncrypt,TEMPSMALL,LOADFIPS,KEYMAPSMALL,SAVEFIPS)' >> $@
		@echo \
'DECRYPT(DesSmallFipsDecrypt,TEMPSMALL,LOADFIPS,KEYMAPSMALL,SAVEFIPS)' >> $@
desSmallCore.c:
		@echo '#include "desCode.h"' > $@
		@echo \
'ENCRYPT(DesSmallCoreEncrypt,TEMPSMALL,LOADCORE,KEYMAPSMALL,SAVECORE)' >> $@
		@echo \
'DECRYPT(DesSmallCoreDecrypt,TEMPSMALL,LOADCORE,KEYMAPSMALL,SAVECORE)' >> $@
desQuickFips.c:
		@echo '#include "desCode.h"' > $@
		@echo \
'ENCRYPT(DesQuickFipsEncrypt,TEMPQUICK,LOADFIPS,KEYMAPQUICK,SAVEFIPS)' >> $@
		@echo \
'DECRYPT(DesQuickFipsDecrypt,TEMPQUICK,LOADFIPS,KEYMAPQUICK,SAVEFIPS)' >> $@
desQuickCore.c:
		@echo '#include "desCode.h"' > $@
		@echo \
'ENCRYPT(DesQuickCoreEncrypt,TEMPQUICK,LOADCORE,KEYMAPQUICK,SAVECORE)' >> $@
		@echo \
'DECRYPT(DesQuickCoreDecrypt,TEMPQUICK,LOADCORE,KEYMAPQUICK,SAVECORE)' >> $@
