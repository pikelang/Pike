#
# Makefile for building AutoDoc documentation
# $Id$
#

PIKE=pike

all: autodoc

autodoc: release
	@pike autodoc.pike

release:
	@if [ -f mangl.tar.gz ]; then \
	  if [ -r release ]; then rm -rf release; fi; \
	  gunzip -c mangl.tar.gz | tar -x ;\
	else \
	  echo "mangl.tar.gz not found."; \
	  return 1;\
	fi
