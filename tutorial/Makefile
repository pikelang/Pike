all:
	@if `pwd`/../bin/pike -e 'return 0;' 2>/dev/null; then \
	  $(MAKE) pike=`pwd`/../bin/pike low_all; \
	else \
	  $(MAKE) low_all; \
	fi

low_all: tutorial.html tutorial_onepage.html

MADEFILES=Image.wmml Mysql.wmml ProtocolsLysKOM.wmml \
	ProtocolsHTTP.wmml Parser.wmml Calendar.wmml \

SRCFILES=tutorial.wmml $(MADEFILES)

pike=pike
DESTINATION=/usr/local/share/pike/tutorial/

.DUMMY: 

# ----------------------------------------------------------------------

Image.wmml : 
	(cd ../src/modules/Image; make wmml pike="$(pike)")

Mysql.wmml : 
	$(pike) ../bin/mkwmml.pike ../src/modules/Mysql/mysql.c >Mysql.wmml

Parser.wmml : 
	$(pike) ../bin/mkwmml.pike ../src/modules/Parser/*.pmod.in ../src/modules/Parser/*.c  >Parser.wmml

ProtocolsHTTP.wmml :
	$(pike) ../bin/mkwmml.pike `/bin/ls -1 ../lib/modules/Protocols.pmod/HTTP.pmod/*.p???` >ProtocolsHTTP.wmml

ProtocolsLysKOM.wmml :
	$(pike) ../bin/mkwmml.pike `/bin/ls -1 ../lib/modules/Protocols.pmod/LysKOM.pmod/*.p???` >ProtocolsLysKOM.wmml


Calendar.wmml :
	$(pike) ../bin/mkwmml.pike `/bin/ls -1 ../lib/modules/Calendar.pmod/*.p* | egrep -v '~$$'` >Calendar.wmml


# ----------------------------------------------------------------------

srcfiles: $(SRCFILES) 

tutorial.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html tutorial

tutorial_onepage.html: .DUMMY  $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html_onepage tutorial_onepage

tutorial_split.html: .DUMMY  $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html_section_pages tutorial_split

tutorial.tex: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml latex tutorial

# latex has to be run twice to work with labels and stuff
# but there are no labels yet...
tutorial.dvi: tutorial.tex
	latex tutorial.tex

tutorial.pdf: tutorial.pdftex
	pdflatex tutorial.pdftex

tutorial.ps: tutorial.dvi
	dvips -Z tutorial.dvi -o tutorial.ps

tutorial.ps.gz: tutorial.ps
	gzip -9 <tutorial.ps >tutorial.ps.gz

tutorial.ps.bz2: tutorial.ps
	bzip2 -9 <tutorial.ps >tutorial.ps.bz2

tutorial.pdftex: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml pdflatex tutorial


tut.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml sitebuilder tut

wmml.html: .DUMMY wmml.wmml
	$(pike) ./wmmltohtml2 wmml.wmml html_onepage wmml

manpages: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml manpages man/man

mirar_tutorial: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml mirar_html tutorial

the_image_module.html: Image.wmml the_image_module.wmml
	$(pike) ./wmmltohtml2 the_image_module.wmml mirar_html the_image_module

clean:
	-rm -f *.html *.md $(MADEFILES) 
	-rm -f tutorial.dvi tutorial.tex tutorial.aux
	-rm -f tutorial.log tutorial.toc
	-rm -f latex_wcache illustration_cache image_cache gfx/* execute_cache

rebuild:
	-rm -f $(MADEFILES)
	make pike="$(pike)" Image.wmml Mysql.wmml tutorial.html

install:
	cp `cat tutorial.files` $(DESTINATION)

export:
	tar czvTf tutorial.files tutorial.tar.gz 
	tar czvTf tutorial_onepage.files tutorial_onepage.tar.gz 

mirar_www_tutorial: mirar_tutorial
	rm /home/mirar/www/pike_tutorial/tutorial* /home/mirar/www/pike_tutorial/gfx/*
	ln -f `cat tutorial.files | grep -v ^gfx/` /home/mirar/www/pike_tutorial
	ln -f `cat tutorial.files | grep ^gfx/` /home/mirar/www/pike_tutorial/gfx
