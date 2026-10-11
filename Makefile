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

# Check if SCCS 'co' command is available
HAS_CO := $(shell command -v co 2>/dev/null)

MAKE="make" $(MFLAGS)

all \
install \
clean \
realclean \
depend \
debug \
lint \
test \
co \
inst-bin \
inst-man \
inst-lib:	$(MAKEFILE) FORCE
	      @ $(MAKE) -f $(MAKEFILE) $@

ifneq ($(HAS_CO),)
# SCCS available: regenerate make.file from mf.c when needed
$(MAKEFILE): mf.c mf.dep $(CONFIG)
		@ echo "Tailoring makefile ..."
		@ echo "## This file is made from mf.c" >$@
		@ echo "## Don't modify this directly!" >>$@
		@ echo "## Instead, modify mf.c and type" >>$@
		@ echo "##	make $(MAKEFILE)" >>$@
		@ echo "" >>$@
		$(CC) -E mf.c | sed -e '/^#/d' -e '/^[.	]*$$/d' \
			-e 's/^ /	/' >>$@
		cat mf.dep >>$@
else
# SCCS not available: regenerate make.file from mf.c if needed
$(MAKEFILE): mf.c mf.dep $(CONFIG) FORCE
	@ echo "Generating makefile from mf.c ..."
	@ echo "## This file is made from mf.c" > $@
	@ echo "## Don't modify this directly!" >> $@
	@ echo "## Instead, modify mf.c and type" >> $@
	@ echo "##	make $(MAKEFILE)" >> $@
	@ echo "" >> $@
	$(CC) -E mf.c | sed -e '/^#/d' -e '/^[.	]*$$/d' \
		-e 's/^ /	/' >> $@
	cat mf.dep >> $@
endif

FORCE:

ifneq ($(HAS_CO),)
.DEFAULT:;	co -l $@
endif
