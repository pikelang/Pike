#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.55 2003/04/15 14:46:09 nisse Exp $
#
# Henrik Grubbström 2001-02-02
#

BUILDDIR=
PIKE=pike

all: modref traditional

pike.ida.liu.se: modref pelix_modref traditional

# These are the actual build targets for making manuals...

modref: $(BUILDDIR)/modref.xml structure/modref.css structure/modref.html \
	  presentation/make_html.pike presentation/tree-split-autodoc.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_modref

low_modref:
	rm -rf modref || /bin/true
	@mkdir modref
	@cp structure/modref.css modref/style.css
	@$(PIKE) presentation/tree-split-autodoc.pike \
	  $(BUILDDIR)/modref.xml structure/modref.html modref predef
	@mkdir modref/images
	cp $(BUILDDIR)/doc_build/images/* modref/images
	@for file in pike_logo.gif next.gif prev.gif unit.gif \
          pike_line_left.gif pike_line_middle.gif pike_line_right.gif ; do \
	  cp src_images/$$file modref/images/ ; done

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

one_page: onepage $(BUILDDIR)/onepage.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_one_page

low_one_page:
	cp $(BUILDDIR)/doc_build/images/* onepage/
	@$(PIKE) presentation/make_html.pike --img= $(BUILDDIR)/onepage.xml

traditional: traditional_manual $(BUILDDIR)/traditional.xml presentation/make_html.pike
	@$(MAKE) PIKE="$(PIKE)" BUILDDIR="$(BUILDDIR)" low_traditional

low_traditional:
	cp $(BUILDDIR)/doc_build/images/* traditional_manual/images/
	@$(PIKE) presentation/make_html.pike --img=images/ $(BUILDDIR)/traditional.xml

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
