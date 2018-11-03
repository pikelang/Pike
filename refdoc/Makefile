#
# Main Makefile for extracting AutoDoc mk II to XML
#
# Henrik Grubbström 2001-02-02
#

# make devel can be used if changes are made only to the CSS or JS and you
# don't feel like recompiling the entire doc to test the changes

BUILDDIR=
DESTDIR=
PIKE=pike

all: modref traditional

pike.ida.liu.se: modref traditional

# These are the actual build targets for making manuals...

modref: $(BUILDDIR)/modref.xml structure/modref.css structure/modref.html \
	  structure/modref.js presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" DESTDIR="$(DESTDIR)" low_modref

module_modref: $(BUILDDIR)/module_modref.xml structure/modref.css structure/modref.html \
	  structure/modref.js presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" DESTDIR="$(DESTDIR)" low_module_modref

low_module_modref:
	-rm -rf $(DESTDIR)modref
	@mkdir $(DESTDIR)modref
	@cp structure/modref.css $(DESTDIR)modref/style.css
	@cp structure/modref.js $(DESTDIR)modref/site.js
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/module_modref.xml structure/modref.html $(DESTDIR)modref predef
	@test -d $(DESTDIR)modref/images || mkdir $(DESTDIR)modref/images
	@for file in pike-logo.svg pike-logo.png next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif \
          eye-slash.svg; do \
	  cp src_images/$$file $(DESTDIR)modref/images/ ; done
	-cp $(BUILDDIR)/doc_build/images/* $(DESTDIR)modref/images/

low_modref:
	-rm -rf $(DESTDIR)modref
	@test -d $(DESTDIR)modref || mkdir modref
	@cp structure/modref.css $(DESTDIR)modref/style.css
	@cp structure/modref.js $(DESTDIR)modref/site.js
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/modref.xml structure/modref.html $(DESTDIR)modref predef
	@test -d $(DESTDIR)modref/images || mkdir $(DESTDIR)modref/images
	-cp $(BUILDDIR)/doc_build/images/* $(DESTDIR)modref/images
	@for file in pike-logo.svg pike-logo.png next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif \
          eye-slash.svg; do \
	  cp src_images/$$file $(DESTDIR)modref/images/ ; done

devel:
	@cp structure/modref.css $(DESTDIR)modref/style.css
	@cp structure/modref.js $(DESTDIR)modref/site.js
	@for file in pike-logo.svg pike-logo.png next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif \
          eye-slash.svg; do \
	  cp src_images/$$file $(DESTDIR)modref/images/ ; done

one_page: onepage $(BUILDDIR)/onepage.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_one_page

low_one_page:
	cp $(BUILDDIR)/doc_build/images/* onepage/
	@$(PIKE) presentation/make_html.pike --img= $(BUILDDIR)/onepage.xml

traditional: traditional_manual $(BUILDDIR)/traditional.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" DESTDIR="$(DESTDIR)" low_traditional

low_traditional:
	cp $(BUILDDIR)/doc_build/images/* $(DESTDIR)traditional_manual/images/
	cp structure/modref.css traditional_manual/modref.css
	@$(PIKE) presentation/make_html.pike --dest=$(DESTDIR) --img=images/ \
		--template=structure/chapters.html $(BUILDDIR)/traditional.xml

# Sub targets used by the above targets.

always:

images: $(BUILDDIR)/doc_build/src/post_modules/GTK2/refdoc/images
	@cp ../src/post_modules/GTK2/refdoc/images/*.png \
	  $(BUILDDIR)/doc_build/src/post_modules/GTK2/refdoc/images

# Directories

onepage:
	@test -d onepage || mkdir onepage

traditional_manual:
	@test -d traditional_manual || mkdir traditional_manual
	@test -d traditional_manual/images || mkdir traditional_manual/images


# Development targets

touch-mirardoc:
	@touch `grep -r \\*\\*\! ../src/modules/* | cut -d: -f1 | \
	  uniq`
	@touch `grep -r "//\! module" ../lib/modules/* | cut -d: -f1 | \
	  uniq`


# Cleanup

clean:
	-rm -fr modref onepage traditional_manual pelix_modref
