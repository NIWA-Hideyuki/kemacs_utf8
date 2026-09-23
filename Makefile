#
# frontend of Makefile for kemacs
#

SHELL=/bin/sh
CC=cc
# This value of $(CC) is used only when tailoring makefile.
# If you want to change the compiler for making kemacs itself,
# modify mf.c instead.

MAKEFILE=make.file
CONFIG=econfig.h

MAKE="make" $(MFLAGS)

all \
install \
clean \
realclean \
depend \
debug \
lint \
co \
inst-bin \
inst-man \
inst-lib:	$(MAKEFILE) FORCE
	      @ $(MAKE) -f $(MAKEFILE) $@

$(MAKEFILE):	mf.c mf.dep $(CONFIG)
	      @ echo "Tailoring makefile ..."
	      @ echo "## This file is made from mf.c" >$@
	      @ echo "## Don't modify this directly!" >>$@
	      @ echo "## Instead, modify mf.c and type" >>$@
	      @ echo "##	make $(MAKEFILE)" >>$@
	      @ echo "" >>$@
	      $(CC) -E mf.c | sed -e '/^#/d' -e '/^[.	]*$$/d' \
			-e 's/^ /	/' >>$@
	      cat mf.dep >>$@

FORCE:

.DEFAULT:;	co -l $@
