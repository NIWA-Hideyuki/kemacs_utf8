#include "econfig.h"

/*** configuration parameters ***/
CC	      = cc

/* name, location, owner, group, permission of binary */
PREFIX	      = /usr/local
PROGRAM	      = kemacs
DEST	      = $(PREFIX)/bin
OWNER         = root
GROUP         = staff
MODE	      = 755
/* name and location of manual */
MNAME	      = kemacs.1
MDEST	      = $(PREFIX)/man/man1
/* location of library */
LIBDEST	      = $(PREFIX)/lib/kemacs

RCFILE	      = .kemacsrc

/* Flags to CC, LD and INSTALL */
#ifdef __GNUC__
OPTIM	      = -O2
#else
OPTIM	      = -O
#endif
DEBUG	      =# -g
DEFS	      =
#if MALLOC_VOIDSTAR
GDEFS	      = -std=gnu89
#else
GDEFS	      = -DMALLOC_CHARSTAR -std=gnu89
#endif
#if HAVE_AS_R
SHARESTR      = -R
#endif
#if HAVE_INSTALL
STRIPFLG      = -s
#endif

/* File code for sources */
FCODE         = -j	/* JIS KANJI */
/* FCODE      = -u	/* UJIS KANJI */
/* FCODE      = -s	/* SJIS KANJI */

/* location of include file */
#if HANDLE_UTF
/*EINCL	      = -I/usr/local/include	/* for iconv.h */
EINCL	      =		# Some systems do not need above (Linux, Cygwin etc.) */
#else
EINCL	      =
#endif /* EINCL is included in INCL */
#if KANJI
INCL	      = $(EINCL) -Ikanji -ICstrings
#else
INCL	      = $(EINCL)
#endif

/* runtime libraries */
/*ELIBS	      = -ltermcap -lPW		# termcap + System V */
/*ELIBS	      = -lcurses -lPW		# System V */
#if BSD
ELIBS	      = -ltermlib		/* BSD */
/*ELIBS	      = -ltermlib -lcompat	# FreeBSD */
#endif
#if USG_5_4
/*ELIBS         = -lcurses -lgen -lmalloc */
ELIBS	      = -lncurses		# Linux */
/*ELIBS	      = -ltermcap		# Cygwin */
#endif

/* iconv library */
#if HANDLE_UTF
ILIBS	      =					# with glibc (Linux: iconv is built-in) */
/*ILIBS	      = -liconv				# Cygwin? */
/*ILIBS	      = -liconv -L/usr/local/lib	# BSD etc? */
/*ILIBS	      = -liconv -L/usr/local/lib -R/usr/local/lib  # Solaris etc? */
#else
ILIBS	      =
#endif

/*** end of configuration part ***/

#if BSD
MAKE	      = exec "make" $(MFLAGS)
#endif
MAKE_HERE     = $(MAKE) -f make.file
MAKE_KANJI    = $(MAKE) -f Makefile.kanji CC='$(CC)' CCFLAGS='$(CCFLAGS)' EINCL='$(EINCL)'
MAKE_KPP      = $(MAKE) -f Makefile.kpp CC='$(CC)' CCFLAGS='$(CCFLAGS)' \\
					EINCL='$(EINCL)' ILIBS='$(ILIBS)'
MAKE_CSTR     = $(MAKE) -f Makefile.Cstr CC='$(CC)' CCFLAGS='$(CCFLAGS)' EINCL='$(EINCL)'
KPP	      = kpp/kpp
FTOC	      = kpp/ftoc
#if HAVE_INSTALL
INSTALL	      = install
#else
STRIP	      = strip
#endif
MAKETD	      = maketd
LINT	      = lint
CTAGS	      = ctags

.SUFFIXES:
.SUFFIXES: .o .c .C_ .h .H_

CCFLAGS        = $(OPTIM) $(GDEFS) $(DEBUG)
CFLAGS        = $(CCFLAGS) $(INCL) $(DEFS)
LDFLAGS	      = $(DEBUG) $(DEFS)
LINTFLAGS     = -xhb $(INCL)
#if HAVE_INSTALL
INSTFLAGS_NONROOT = -c $(STRIPFLG) -m $(MODE)
INSTFLAGS     = $(INSTFLAGS_NONROOT) -o $(OWNER) -g $(GROUP)
#endif

#if KANJI
LIBS          = kanji/kanji.a Cstrings/Cstrings.a
#else
LIBS	      =
#endif

OBJS	      = basic.o \\
		bind.o \\
		buffer.o \\
		crypt.o \\
		display.o \\
		eval.o \\
		exec.o \\
		file.o \\
		fileio.o \\
		input.o \\
		isearch.o \\
		kanji.o \\
		line.o \\
		main.o \\
		random.o \\
		region.o \\
		search.o \\
		spawn.o \\
		tcap.o \\
		termio.o \\
		version.o \\
		window.o \\
		word.o

SRCS	      = basic.C_ \\
		bind.C_ \\
		buffer.C_ \\
		crypt.C_ \\
		display.C_ \\
		eval.C_ \\
		exec.C_ \\
		file.C_ \\
		fileio.C_ \\
		input.C_ \\
		isearch.C_ \\
		kanji.C_ \\
		line.C_ \\
		main.C_ \\
		random.C_ \\
		region.C_ \\
		search.C_ \\
		spawn.C_ \\
		tcap.C_ \\
		termio.C_ \\
		version.C_ \\
		window.C_ \\
		word.C_

CSRCS	      = basic.c \\
		bind.c \\
		buffer.c \\
		crypt.c \\
		display.c \\
		eval.c \\
		exec.c \\
		file.c \\
		fileio.c \\
		input.c \\
		isearch.c \\
		kanji.c \\
		line.c \\
		main.c \\
		random.c \\
		region.c \\
		search.c \\
		spawn.c \\
		tcap.c \\
		termio.c \\
		version.c \\
		window.c \\
		word.c

HDRS	      = ebind.H_ \\
		ecomm.H_ \\
		edef.H_ \\
		efunc.H_ \\
		epath.H_ \\
		estruct.H_ \\
		evar.H_ \\
		eversion.H_

HHDRS	      = ebind.h \\
		ecomm.h \\
		edef.h \\
		efunc.h \\
		epath.h \\
		estruct.h \\
		evar.h \\
		eversion.h

MANUAL	      = kemacs.man

DOCS	      = Cmds.narrow Commands.me Macros Readme Tech WhatsNew \\
		emacs.key emacs.hlp emacs.HLP_ emacs.tut emacs.TUT_ \\
		emacs.mss magic.doc Added Install Manifest .kemacsrc ccg
/* where's emacs.MSS? omitted by N.Nide */

#if KANJI
STROBJ	      = Css.o
#else
STROBJ	      =
#endif

#if KANJI
all:		$(KPP) $(FTOC) $(PROGRAM) $(RCFILE)
#else
all:		$(PROGRAM) $(RCFILE)
#endif

debug:;       @ $(MAKE_HERE) all PROGRAM="a.out" \
			OPTIM="" \
			DEBUG="-g"

$(PROGRAM):     $(OBJS) $(LIBS) $(STROBJ)
	      @ echo "Loading $(PROGRAM) ..."
	      $(CC) $(LDFLAGS) $(STROBJ) $(OBJS) $(LIBS) $(ELIBS) $(ILIBS) -o $(PROGRAM)
	      @ echo "done"

clean:
	      @ echo "Cleaning up directory ..."
	      @ cd kpp; $(MAKE_KPP) clean
	      @ cd kanji; $(MAKE_KANJI) clean
	      @ cd Cstrings; $(MAKE_CSTR) clean
	      @ rm -f $(PROGRAM) $(OBJS) $(CSRCS) $(HHDRS) Cxx.* Css.* a.out Cstr.data make.file *.bak core $(RCFILE)

realclean:
	      @ echo "Cleaning all binaries and executables ..."
	      @ cd kpp; $(MAKE_KPP) realclean
	      @ cd kanji; $(MAKE_KANJI) realclean
	      @ cd Cstrings; $(MAKE_CSTR) realclean
	      @ rm -f $(PROGRAM) $(OBJS) $(CSRCS) $(HHDRS) Cxx.* Css.* a.out Cstr.data make.file *.bak core $(RCFILE)

install:	inst-bin inst-man inst-lib

inst-bin:	$(PROGRAM)
	      @ echo Installing $(PROGRAM) in $(DEST) "..."
	      -mkdir $(DEST) || mkdir -p $(DEST)
	       /* if mkdir doesn't have -p option, possibly both fails */
#if HAVE_INSTALL
	      case "`id`" in \\
	      uid=0*|'') $(INSTALL) $(INSTFLAGS) $(PROGRAM) $(DEST);; \\
	      *)         $(INSTALL) $(INSTFLAGS_NONROOT) $(PROGRAM) $(DEST);; \\
	      esac
#else
	      cp $(PROGRAM) $(DEST) && \\
		$(STRIP) $(DEST)/$(PROGRAM) && \\
		chmod $(MODE) $(DEST)/$(PROGRAM)
	      case "`id`" in \\
	      uid=0*|'') \\
		chown $(OWNER) $(DEST)/$(PROGRAM) && \\
		chgrp $(GROUP) $(DEST)/$(PROGRAM);; \\
	      esac
#endif
	      @ echo " done"

inst-man:
	      @ echo -n "Installing manual ..."
	      -mkdir $(MDEST) || mkdir -p $(MDEST)
	      if [ `pwd`/$(MANUAL) != $(MDEST)/$(MNAME) ]; then \\
			cp $(MANUAL) $(MDEST)/$(MNAME); \\
		fi
	      @ echo " done"

inst-lib: $(RCFILE)
	      @ echo -n "Installing library ..."
	      -mkdir $(LIBDEST) || mkdir -p $(LIBDEST)
	      if [ `pwd` != $(LIBDEST) ]; then \\
			cp $(DOCS) $(LIBDEST); \\
			chmod a+x $(LIBDEST)/ccg; \\
		fi
	      @ echo " done"

update:		$(DEST)/$(PROGRAM)

$(DEST)/$(PROGRAM): $(PROGRAM)
	      $(MAKE_HERE) install

lint:		$(CSRCS) $(HHDRS)
	      @ "$(LINT)" $(LINTFLAGS) $(CSRCS) 2>&1 | egrep -v 'possible pointer alignment'

tags:           $(HHDRS) $(CSRCS)
	      @ echo Making tags ...
	      @ $(CTAGS) -t $(HHDRS) $(CSRCS)
	      @ sed -e 's/\.c	/.C_	/' -e 's/\.h	/.H_	/' tags >tags.tmp
	      @ mv tags.tmp tags

depend:		$(HHDRS) $(CSRCS)
	      @ echo Remaking dependencies...
	      @ $(MAKETD) -mmf.dep $(INCL) $(DEFS) $(CSRCS)

FORCE:

kanji/kanji.a:	FORCE
	      cd kanji; $(MAKE_KANJI) all

Cstrings/Cstrings.a:	FORCE
	      cd Cstrings; $(MAKE_CSTR) all

Css.o:		Css.c
		$(CC) -c $(SHARESTR) $*.c

#if KANJI
.C_.c:;	      @ echo processing $*.C_ ...
	      $(KPP) $(FCODE) $*.C_
	      mv Cxx.c $@
#else
.C_.c:;	      @ cp $*.C_ $@
#endif

.c.o:;		$(CC) $(CFLAGS) -c $*.c

#if KANJI
epath.h:	epath.H_
	      sed -e 's|_PREFIX_|$(PREFIX)|g' $*.H_ | $(KPP) $(FCODE) 
	      mv Cxx.c $@

.H_.h:;	      @ echo processing $*.H_ ...
	      $(KPP) $(FCODE) $*.H_
	      mv Cxx.c $@
#else
epath.h:	epath.H_
		sed -e 's|_PREFIX_|$(PREFIX)|g' $*.H_ > $@

.H_.h:;	      @ cp $*.H_ $@
#endif

Css.c:		Cstr.data
	      @ echo Making $@ ...
	      @ $(FTOC)

$(KPP) $(FTOC):	$(LIBS) FORCE
	      cd kpp; $(MAKE_KPP) all

co:		$(HDRS) $(SRCS) $(MANUAL) $(DOCS)

.DEFAULT:;	co -l $@

#if KANJI
$(HHDRS) $(CSRCS): $(KPP)
#endif
$(OBJS): $(HHDRS)

ebind.h: ebind.H_
ecomm.h: ecomm.H_
edef.h: edef.H_
efunc.h: efunc.H_
epath.h: epath.H_
estruct.h: estruct.H_
evar.h: evar.H_
eversion.h: eversion.H_

basic.c: basic.C_
bind.c: bind.C_
buffer.c: buffer.C_
crypt.c: crypt.C_
display.c: display.C_
eval.c: eval.C_
exec.c: exec.C_
file.c: file.C_
fileio.c: fileio.C_
input.c: input.C_
isearch.c: isearch.C_
kanji.c: kanji.C_
line.c: line.C_
main.c: main.C_
random.c: random.C_
region.c: region.C_
search.c: search.C_
spawn.c: spawn.C_
tcap.c: tcap.C_
termio.c: termio.C_
version.c: version.C_
window.c: window.C_
word.c: word.C_

$(RCFILE):	$(RCFILE).tmpl
		sed -e 's|_PREFIX_|$(PREFIX)|g' $(RCFILE).tmpl > $@
