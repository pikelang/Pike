
all:
	@if `pwd`/../bin/pike -e 'return 0;' 2>/dev/null; then \
	  $(MAKE) pike=`pwd`/../bin/pike low_all; \
	else \
	  $(MAKE) low_all; \
	fi

low_all: tutorial.html tutorial_onepage.html

MADEFILES=Image.wmml Math.wmml Mysql.wmml Parser.wmml Calendar.wmml \
	  Protocols.wmml Gettext.wmml

SRCFILES=tutorial.wmml $(MADEFILES)

pike=pike
DESTINATION=/usr/local/share/pike/tutorial/

.DUMMY: 

# ----------------------------------------------------------------------

PROTOCOLS_MKWMML_DIRS=HTTP LysKOM IRC DNS

MKWMML=$(pike) ../bin/mkwmml.pike

Calendar.wmml :
	$(MKWMML) `/bin/ls -1 ../lib/modules/Calendar.pmod/*.p* | egrep -v '~$$'` >Calendar.wmml

Image.wmml : 
	(cd ../src/modules/Image; make wmml pike="$(pike)")

Math.wmml : 
	$(MKWMML) ../src/modules/Math/*.c >Math.wmml

Mysql.wmml :  ../src/modules/Mysql/mysql.c
	$(MKWMML) ../src/modules/Mysql/mysql.c >Mysql.wmml

Gettext.wmml: ../src/modules/Gettext/gettext.c
	$(MKWMML) ../src/modules/Gettext/gettext.c >Gettext.wmml

Parser.wmml : ../src/modules/Parser/html.c
	$(MKWMML) ../src/modules/Parser/*.pmod.in ../src/modules/Parser/*.c  >Parser.wmml

Protocols.wmml :
	for a in $(PROTOCOLS_MKWMML_DIRS) ; do \
		if test -d ../lib/modules/Protocols.pmod/$$a.pmod; then \
			$(MKWMML) `/bin/ls -1 ../lib/modules/Protocols.pmod/$$a.pmod/*.p* | egrep -v '~$$'`; \
		else \
			$(MKWMML) ../lib/modules/Protocols.pmod/$$a.pmod; \
		fi		\
	done > Protocols.wmml

Modules.wmml: 
	$(MKWMML) `find ../lib/modules -type f -print ` >Modules.wmml


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

tutorial.xml: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml xml tutorial

# latex has to be run twice to work with labels and stuff
# but there are no labels yet...
tutorial.dvi: tutorial.tex
	latex tutorial.tex

tutorial.pdf: tutorial.pdftex
	TEXPSHEADERS=fonts: TEXINPUTS=fonts: TTFONTS=fonts: TFMFONTS=fonts: pdflatex tutorial.pdftex

tutorial.ps: tutorial.dvi
	dvips -a -Z tutorial.dvi -o tutorial.ps

tutorial.ps.gz: tutorial.ps
	gzip -9 <tutorial.ps >tutorial.ps.gz

tutorial.ps.bz2: tutorial.ps
	bzip2 -9 <tutorial.ps >tutorial.ps.bz2

tutorial.pdftex: .DUMMY $(SRCFILES)
	if [ -d fonts ]; then ( cd fonts ; make pike="$(pike)" ) ; fi
	$(pike) ./wmmltohtml2 tutorial.wmml pdflatex tutorial


tut.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml sitebuilder tut

wmml.html: .DUMMY wmml.wmml
	$(pike) ./wmmltohtml2 wmml.wmml html_onepage wmml

manpages: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml manpages man/man

mirar_tutorial: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml mirar_html tutorial

js_tutorial: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml js_html tutorial

the_image_module.html: Image.wmml the_image_module.wmml
	$(pike) ./wmmltohtml2 the_image_module.wmml mirar_html the_image_module

clean:
	-rm -f *.html *.md $(MADEFILES) 
	-rm -f tutorial.dvi tutorial.tex tutorial.aux
	-rm -f tutorial.log tutorial.toc
	-rm -f latex.wcache pdflatex.wcache illustration_cache image_cache gfx/* execute_cache
	if [ -d fonts ]; then ( cd fonts ; make clean ); fi

rebuild:
	-rm -f $(MADEFILES)
	$(MAKE) pike="$(pike)" all

install:
	cp `cat tutorial.files` $(DESTINATION)

export:
	tar czvTf tutorial.files tutorial.tar.gz 
	tar czvTf tutorial_onepage.files tutorial_onepage.tar.gz 

mirar_www_tutorial: mirar_tutorial
	rm /home/mirar/www/pike_tutorial/tutorial* /home/mirar/www/pike_tutorial/gfx/*
	ln -f `cat tutorial.files | grep -v ^gfx/` /home/mirar/www/pike_tutorial
	ln -f `cat tutorial.files | grep ^gfx/` /home/mirar/www/pike_tutorial/gfx
	(cd /home/mirar/www/pike_tutorial; rm tutorial.tar.gz; tar cvzf tutorial.tar.gz `cat /home/mirar/pike7/tutorial/tutorial.files`)
