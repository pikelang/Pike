#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id$
#
# Henrik Grubbström 2001-02-02
#

BUILDDIR=
DESTDIR=
PIKE=pike

all: modref traditional

pike.ida.liu.se: modref pelix_modref traditional

caudium.net: modref caudium_modref traditional

# These are the actual build targets for making manuals...

modref: $(BUILDDIR)/modref.xml structure/modref.css structure/modref.html \
	  presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" DESTDIR="$(DESTDIR)" low_modref

module_modref: $(BUILDDIR)/module_modref.xml structure/modref.css structure/module_modref.html \
	  presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" DESTDIR="$(DESTDIR)" low_module_modref

low_module_modref:
	rm -rf $(DESTDIR)modref || /bin/true
	@mkdir $(DESTDIR)modref
	@cp structure/modref.css $(DESTDIR)modref/style.css
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/module_modref.xml structure/module_modref.html $(DESTDIR)modref predef
	@mkdir $(DESTDIR)modref/images
	-cp $(BUILDDIR)/doc_build/images/* $(DESTDIR)modref/images
	@for file in pike_logo.gif next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif ; do \
	  cp src_images/$$file $(DESTDIR)modref/images/ ; done

low_modref:
	rm -rf $(DESTDIR)modref || /bin/true
	@test -d $(DESTDIR)modref || mkdir modref
	@cp structure/modref.css modref/style.css
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/modref.xml structure/modref.html $(DESTDIR)modref predef
	@test -d $(DESTDIR)modref/images || mkdir $(DESTDIR)modref/images
	-cp $(BUILDDIR)/doc_build/images/* $(DESTDIR)modref/images
	@for file in pike_logo.gif next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif ; do \
	  cp src_images/$$file $(DESTDIR)modref/images/ ; done

pelix_modref: $(BUILDDIR)/modref.xml structure/pelix_modref.html \
	        presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_pelix_modref

low_pelix_modref:
	rm -rf pelix_modref || /bin/true
	@mkdir pelix_modref
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/modref.xml structure/pelix_modref.html pelix_modref predef
	@mkdir pelix_modref/images
	cp $(BUILDDIR)/doc_build/images/* pelix_modref/images

caudium_modref: $(BUILDDIR)/modref.xml structure/caudium_modref.html \
		presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_caudium_modref

low_caudium_modref:
	rm -rf caudium_modref || /bin/true
	@mkdir caudium_modref
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/modref.xml structure/caudium_modref.html caudium_modref predef
	@mkdir pelix_modref/images
	cp $(BUILDDIR)/doc_build/images/* caudium_modref/images

one_page: onepage $(BUILDDIR)/onepage.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_one_page

low_one_page:
	cp $(BUILDDIR)/doc_build/images/* onepage/
	@$(PIKE) presentation/make_html.pike --img= $(BUILDDIR)/onepage.xml

traditional: traditional_manual $(BUILDDIR)/traditional.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" DESTDIR="$(DESTDIR)" low_traditional

low_traditional:
	cp $(BUILDDIR)/doc_build/images/* $(DESTDIR)traditional_manual/images/
	@$(PIKE) presentation/make_html.pike --dest=$(DESTDIR) --img=images/ $(BUILDDIR)/traditional.xml

# Sub targets used by the above targets.

always:

images: $(BUILDDIR)/doc_build/src/post_modules/GTK/refdoc/images
	@cp ../src/post_modules/GTK/refdoc/images/*.png \
	  $(BUILDDIR)/doc_build/src/post_modules/GTK/refdoc/images

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
	true

spotless:
	-test -f .cvsignore && rm -rf `cat .cvsignore`
