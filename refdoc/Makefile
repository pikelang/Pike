#
# Main Makefile for extracting AutoDoc mk II to XML
#
# $Id: Makefile,v 1.5 2001/02/06 14:47:29 grubba Exp $
#
# Henrik Grubbström 2001-02-02
#

PIKE=pike

VPATH=../src:.

SRCDIR=../src
ROOT=.

all: xml recurse

.SUFFIXES: .c .xml

.c.xml: @(ROOT)/../bin/autodoc.pike
	@rm -f "$@.new" 2>/dev/null || /bin/true
	@$(PIKE) $(ROOT)/../bin/autodoc.pike "$<" >"$@.new" && \
	  mv -f "$@.new" "$@"

xml:
	$(MAKE) PIKE="$(PIKE)" `(cd $(SRCDIR) && ls *.c) | sed -e 's/\.c$$/\.xml/'`

recurse: subdirectories
	@for d in */.; do \
	  if test "$$d" = "CVS/." -o ! -d "$$d"; then :; else \
	    (cd $$d && make PIKE="$(PIKE)" all) || exit 1; \
	  fi; \
	done

subdirectories:
	@for d in `cd $(SRCDIR) && echo *`; do \
	  if test "$$d" = "CVS" -o ! -d "$(SRCDIR)/$$d/."; then :; else \
	    if test -d "$$d"; then :; else \
	      mkdir "$$d" || exit 1; \
	    fi; \
	    if test -f "$$d/Makefile"; then :; else \
	      sed -e "s#@SRCDIR@#../$(SRCDIR)/$$d#" -e "s#@ROOT@#../$(ROOT)#" \
	        <$(ROOT)/Makefile.in >"$$d/Makefile" || exit 1; \
	    fi; \
	  fi; \
	done

clean:
	-@rm *.xml
	@for d in *; do \
	  if test "$$d" = "CVS" -o ! -d "$$d/."; then :; else \
	    rm -rf "$$d"; \
	  fi; \
	done
