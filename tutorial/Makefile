all: tutorial.html tutorial_onepage.html

# lenna.gif lenna-mirrorx.gif lenna-mirrory.gif lenna-rotate.gif lenna-skewx.gif lenna-skewy.gif

SRCFILES=tutorial.wmml Image.wmml Mysql.wmml

pike=pike

.DUMMY: 

tutorial.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html tutorial

tutorial_onepage.html: .DUMMY  $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml html_onepage tutorial_onepage

tut.html: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml sitebuilder tut

manpages: .DUMMY $(SRCFILES)
	$(pike) ./wmmltohtml2 tutorial.wmml manpages man/man

Image.wmml : 
	(cd ../src/modules/Image; make wmml pike="$(pike)")

Mysql.wmml :
	$(pike) ../bin/mkwmml.pike ../src/modules/Mysql/mysql.c >Mysql.wmml

the_image_module_onepage.html: Image.wmml the_image_module.wmml
	$(pike) ./wmmltohtml2 the_image_module.wmml html_onepage the_image_module_onepage

the_image_module.html: Image.wmml the_image_module.wmml
	$(pike) ./wmmltohtml2 the_image_module.wmml html the_image_module

clean:
	rm *.html *.md illustration_cache illustration*.gif 

rebuild: Image.wmml Mysql.wmml all

export:
	tar czvTf tutorial.files tutorial.tar.gz 
	tar czvTf tutorial_onepage.files tutorial_onepage.tar.gz 


