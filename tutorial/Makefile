all: tutorial.html tutorial_onepage.html

# lenna.gif lenna-mirrorx.gif lenna-mirrory.gif lenna-rotate.gif lenna-skewx.gif lenna-skewy.gif

tutorial.html: tutorial.wmml
	./wmmltohtml2 <tutorial.wmml html tutorial

tutorial_onepage.html: tutorial.wmml
	./wmmltohtml2 <tutorial.wmml html_onepage tutorial_onepage

Image.wmml :
	cd ../src/modules/Image; make wmml

the_image_module_onepage.html: Image.wmml the_image_module.wmml
	./wmmltohtml2 <the_image_module.wmml html_onepage the_image_module_onepage

clean:
	rm *.html illustration_cache illustration*.gif 

export:
	(cd .. ; tar czvf tutorial.tar.gz \
	tutorial/tutorial*.html \
	tutorial/left.gif tutorial/right.gif tutorial/up.gif \
	tutorial/multipart.gif \
	tutorial/illustration*.gif

# tutorial.html: tutorial.wmml wmml_to_html
#	./wmml_to_html <tutorial.wmml >tutorial.html

BG_R=A0
BG_G=E0
BG_B=C0

PIKE_BG=0x$(BG_R),0x$(BG_G),0x$(BG_B)
P=pike -e 'write(Image.image()->fromppm(Process.popen("rasttopnm lenna.rs"))
Q=->scale(1.0/3)->change_color($(PIKE_BG),255,0,255)->toppm())'|ppmquant 256|ppmtogif
T=-trans rgb:ff/00/ff

lenna.gif: lenna.rs Makefile
	$P $Q  >lenna.gif

lenna-mirrorx.gif: lenna.rs  Makefile
	$P ->mirrorx() $Q >lenna-mirrorx.gif

lenna-mirrory.gif: lenna.rs Makefile
	$P ->mirrory() $Q >lenna-mirrory.gif

lenna-rotate.gif: lenna.rs Makefile
	$P ->rotate(45, $(PIKE_BG) )->autocrop() $Q $T >lenna-rotate.gif

lenna-skewx.gif: lenna.rs Makefile
	$P ->skewx(0.3, $(PIKE_BG) ) $Q $T >lenna-skewx.gif

lenna-skewy.gif: lenna.rs Makefile
	$P ->skewy(0.3, $(PIKE_BG) ) $Q $T >lenna-skewy.gif

