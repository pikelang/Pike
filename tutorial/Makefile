all: tutorial.html tutorial_onepage.html

# lenna.gif lenna-mirrorx.gif lenna-mirrory.gif lenna-rotate.gif lenna-skewx.gif lenna-skewy.gif

MADEFILES=Image.wmml Mysql.wmml ProtocolsHTTP.wmml Parser.wmml Calendar.wmml
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
	$(pike) ../bin/mkwmml.pike ../src/modules/Parser/*.c >Parser.wmml

ProtocolsHTTP.wmml :
	$(pike) ../bin/mkwmml.pike ../lib/modules/Protocols.pmod/HTTP.pmod/*.p*[^~] >ProtocolsHTTP.wmml

Calendar.wmml :
	$(pike) ../bin/mkwmml.pike ../lib/modules/Calendar.pmod/*.p*[^~] >Calendar.wmml


# ----------------------------------------------------------------------

srcfiles: $(SRCFILES) 

tutorial.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html tutorial

tutorial_onepage.html: .DUMMY  $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html_onepage tutorial_onepage

tut.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml sitebuilder tut

wmml.html: .DUMMY wmml.wmml
	$(pike) ./wmmltohtml2 wmml.wmml html_onepage wmml

manpages: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml manpages man/man

the_image_module_onepage.html: .DUMMY Image.wmml the_image_module.wmml
	$(pike) ./wmmltohtml2 the_image_module.wmml html_onepage the_image_module_onepage

the_image_module.html: Image.wmml the_image_module.wmml
	$(pike) ./wmmltohtml2 the_image_module.wmml html the_image_module

clean:
	-rm *.html *.md illustration_cache illustration*.gif $(MADEFILES)

rebuild:
	-rm $(MADEFILES)
	make pike="$(pike)" Image.wmml Mysql.wmml tutorial.html

install:
	cp `cat tutorial.files` $(DESTINATION)

export:
	tar czvTf tutorial.files tutorial.tar.gz 
	tar czvTf tutorial_onepage.files tutorial_onepage.tar.gz 

mirar_www: Image.wmml the_image_module_onepage.html
	ln -f illustration*.gif the_image_module_onepage.html /home/mirar/www/image
