all: tutorial.html tutorial_onepage.html

# lenna.gif lenna-mirrorx.gif lenna-mirrory.gif lenna-rotate.gif lenna-skewx.gif lenna-skewy.gif

.DUMMY:

tutorial.html: .DUMMY
	./wmmltohtml2 <tutorial.wmml html tutorial

tutorial_onepage.html: .DUMMY
	./wmmltohtml2 <tutorial.wmml html_onepage tutorial_onepage

Image.wmml :
	cd ../src/modules/Image; make wmml

the_image_module_onepage.html: Image.wmml the_image_module.wmml
	./wmmltohtml2 <the_image_module.wmml html_onepage the_image_module_onepage

clean:
	rm *.html illustration_cache illustration*.gif 

export:
	tar czvTf tutorial.files tutorial.tar.gz 
	tar czvTf tutorial_onepage.files tutorial_onepage.tar.gz 


