#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*	EDEF:		Global variable definitions for

			MicroEMACS 3.2



			written by Dave G. Conroy

			modified by Steve Wilhite, George Jones

			greatly modified by Daniel Lawrence
			*/

			#ifndef EDEF_H_
			#define EDEF_H_




#if	SHORTNAME

/* shorten long names */

# define mlreply	mlrepl

# define mlreplyt	mlrept

# define tcapeeol	tcpeol

# define tcapeeop	tcpeop

# define scrnextup	scnxup

# define scrnextdw	scnxdw

# if	KANJI

#  define kinsoku1	kinsk1

#  define kinsoku2	kinsk2

# endif

#endif	/*SHORTNAME*/



/* some global fuction declarations */



#ifdef __STDC__

# include <stdlib.h>

# if defined(__unix__) || defined(__MACH__)

#  include <unistd.h>

# endif

#else

# ifdef MALLOC_CHARSTAR

char *malloc();

# else

void *malloc();

# endif

#endif

//Char *itoa();
Char *itoaa();

Char *getval();

Char *token();

Char *ltos();

Char ectoc(Ckey c);
Ckey ctoec(Char c);
Char tgetc();

Ckey get1key();

#if HAVE_REGEX

# include <regex.h>

#else

# if	BSD

char *re_comp();

int re_exec();

# endif

# if	USG

char *regcmp();

char *regex();

# endif

#endif

int getccol();	/* get current column */

Char *binding_list_buffer_name();

Char *buffer_list_buffer_name();

Char *help_buffer_name();

Char *version();

Char *prgname();

Char *revdate();

#if	KANJI

Char *code_string();

int string_code();

unsigned kfclose();

void kanji_term(void);

#endif

Char *mkupper();

Char *mklower();

Char *getclock();

Char *key2fname();

Ckey strcmd(Char * s);


#ifdef	maindef

# define EXT

# define INIT(x)	= x

#else

# define EXT		extern

# define INIT(x)

#endif



/* for MAIN.C */



/* initialized global definitions */



EXT int		fillcol		INIT(72);			/* Current fill column	*/

EXT int	py_indent_width	INIT(4);	/* Python indent width (spaces) */
EXT int	py_use_spaces	INIT(TRUE);	/* Use spaces (not tabs) for Python indent */

/* CMODE configuration variables */
EXT int	c_indent_width	INIT(4);	/* C indent width (spaces) */
EXT int	c_use_spaces	INIT(FALSE);	/* Use tabs (not spaces) for C indent */
EXT int	c_brace_style	INIT(0);	/* 0=K&R,1=Allman,2=GNU,3=Linux,4=Whitesmiths,5=1TBS,6=Stroustrup,7=none */
EXT int	c_auto_detect	INIT(FALSE);  /* Auto-detect style from source */
EXT int	c_follow_source	INIT(FALSE); /* Follow existing source indent */
EXT int	c_gnu_indent	INIT(2);	/* GNU half-indent (columns) */
EXT int	c_brace_func	INIT(FALSE);	/* Stroustrup: function { on own line */
EXT int	c_cont_indent	INIT(2);	/* Continuation line indent */
EXT int	c_case_indent	INIT(0);	/* case label indent relative to switch */
EXT int	c_pp_indent	INIT(0);	/* Preprocessor directive indent */
EXT int	c_auto_close	INIT(TRUE);	/* Auto-close brace indentation */

EXT Ckey	kbdm[NKBDM]	INIT({CTLX|')'});	/* Macro				*/

EXT Char	*execstr	INIT(NULL);			/* pointer to string to execute	*/

EXT Char	golabel[NPAT] INIT({'\0'});		/* current line to go to */

EXT int		execlevel	INIT(0);			/* execution IF level	*/

EXT int		eolexist	INIT(TRUE);			/* does clear to EOL exist */

EXT int		revexist	INIT(FALSE);		/* does reverse video exist? */

EXT Char	*modename[]						/* name of modes		*/

#ifdef maindef

	= {	CSTR(&Cxstr[140]), CSTR(&Cxstr[145]), CSTR(&Cxstr[151]), CSTR(&Cxstr[158]),

		CSTR(&Cxstr[164]), CSTR(&Cxstr[170]), CSTR(&Cxstr[175]),

# if MAGIC

		CSTR(&Cxstr[180]),

# endif

# if CRYPT

		CSTR(&Cxstr[186]),

# endif

# if ASAVE

		CSTR(&Cxstr[192]),

# endif

# if KANJI

		CSTR(&Cxstr[198]),

# endif

		NULL}

#endif /* maindef */

	;

EXT Char	modecode[]						/* letters to represent modes */

#ifdef maindef

	= {	'W', 'C', 'P', 'S', 'E', 'V', 'O',

# if MAGIC

		'M',

# endif

# if CRYPT

		'Y',

# endif

# if ASAVE

		'A',

# endif

# if KANJI

		'B',

# endif

		'\0'}

#endif /* maindef */

		;

EXT int		gmode		INIT(0);			/* global editor mode		*/

#if COLOR

EXT int		gfcolor		INIT(7);			/* global forgrnd color (white) */

EXT int		gbcolor		INIT(0);			/* global backgrnd color (black)*/

#endif

#if ASAVE

EXT int		gasave		INIT(256);			/* global ASAVE size		*/

EXT int		gacount		INIT(256);			/* count until next ASAVE	*/

#endif

EXT int		sgarbf		INIT(TRUE);			/* TRUE if screen is garbage */

EXT int		mpresf		INIT(FALSE);		/* TRUE if message in last line */

EXT int		clexec		INIT(FALSE);		/* command line execution flag	*/

EXT int		mstore		INIT(FALSE);		/* storing text to macro flag	*/

EXT int		discmd		INIT(TRUE);			/* display command flag		*/

EXT int		disinp		INIT(TRUE);		/* display input characters	*/
EXT int		show_line_numbers	INIT(FALSE);	/* show line number gutter	*/
EXT int		line_number_width	INIT(6);	/* width of line number gutter */
EXT int		effective_lw		INIT(6);	/* effective gutter width (auto-grown) */

EXT struct BUFFER *bstore INIT(NULL);		/* buffer to store macro text to */

EXT int		vtrow		INIT(0);			/* Row location of SW cursor */

EXT int		vtcol		INIT(0);			/* Column location of SW cursor */

EXT int		ttrow		INIT(HUGE);			/* Row location of HW cursor */

EXT int		ttcol		INIT(HUGE);			/* Column location of HW cursor */

EXT int		lbound		INIT(0);			/* leftmost column of current	*/

											/* line being displayed 		*/

EXT int		taboff		INIT(0);		/* tab offset for display	*/
EXT int		gbound		INIT(0);		/* gutter boundary (left margin)	*/

EXT Ckey		metac		INIT(CTLFLG|'[');		/* current meta character */

EXT Ckey	ctlxc		INIT(CTLFLG|'X');		/* current control X prefix char */

EXT Ckey	reptc		INIT(CTLFLG|'U');		/* current universal repeat char */

EXT Ckey	abortc		INIT(CTLFLG|'G');		/* current abort command char	*/



EXT Char	quotec		INIT(0x11);			/* quote char during mlreply() */

#if COLOR

EXT Char	*cname[]						/* names of colors		*/

# ifdef maindef

	= {	CSTR(&Cxstr[205]), CSTR(&Cxstr[211]), CSTR(&Cxstr[215]), CSTR(&Cxstr[221]),

		CSTR(&Cxstr[228]), CSTR(&Cxstr[233]), CSTR(&Cxstr[241]), CSTR(&Cxstr[246]) }

# endif

	;

#endif /* COLOR */

EXT KILL	*kbufp		INIT(NULL);			/* current kill buffer chunk ptr */

EXT KILL	*kbufh		INIT(NULL);			/* kill buffer header pointer	*/

EXT int		kused		INIT(KBLOCK);		/* # of bytes used in kill buffer*/

EXT WINDOW	*swindow	INIT(NULL);			/* saved window pointer			*/

#if KMSGS

EXT int		kterminal	INIT(0);			/* use KANJI messages			*/

#endif

#if AUTOMODE

EXT int		npatent		INIT(2);			/* # of pattern table entry	*/

EXT PATTBL	pattbl[NMPAT] 				/* pattern table	*/

# ifdef maindef

	= {	{ "\\.[ch]$", MDCMOD },
		{ "\\.py$",   MDPYMOD } }

# endif

	;

#endif /* AUTOMODE */

#if	CRYPT

EXT int		cryptflag	INIT(FALSE);		/* currently encrypting?	*/

#endif

EXT Ckey	*kbdend		INIT(&kbdm[0]);		/* ptr to end of the keyboard	*/

EXT int		kbdmode		INIT(STOP);			/* current keyboard macro mode	*/

EXT int		kbdrep		INIT(0);			/* number of repetitions	*/

#if	RESTRICT

EXT int		restflag	INIT(FALSE);		/* restricted use?		*/

#endif

EXT int		lastkey		INIT(0);			/* last keystoke		*/

EXT int		seed		INIT(0);			/* random number seed		*/

EXT int		macbug		INIT(FALSE);		/* macro debuging flag	*/
EXT int		shell_escape_mode INIT(FALSE);	/* $shell-command-mode: 0=raw, 1=always-quote */

EXT Char	*errorm		INIT(CSTR(&Cxstr[252])); /* error literal		*/

EXT Char	*truem		INIT(CSTR(&Cxstr[258]));	/* true literal			*/

EXT Char	*falsem		INIT(CSTR(&Cxstr[263])); /* false litereal		*/

EXT int		cmdstatus	INIT(TRUE);			/* last command status		*/

EXT int		reclevel	INIT(0);			/* recursive level of editing	*/

EXT BUFFER	*blistp		INIT(NULL);			/* Buffer for C-X C-B		*/

EXT BUFFER	*bdlistp	INIT(NULL);			/* Binding List Buffer		*/

EXT BUFFER	*hlpbp		INIT(NULL);			/* Help Buffer			*/

#ifndef maindef

EXT KEYTAB	keytab[];

EXT NBIND	names[];

#endif

#if KANJI

EXT Char	*gmodeline	INIT(CSTR(&Cxstr[269]));

#else

EXT Char	*gmodeline	INIT(CSTR(&Cxstr[322]));

#endif

					/* Global mode line format	*/

#if	CLOCK

EXT int		clflag		INIT(0);	/* flag for clock	*/

EXT int		cperiod		INIT(CPERIOD);	/* timer interval	*/

#endif



/* uninitialized global definitions */



EXT int		currow;			/* Cursor row			*/

EXT int		curcol;			/* Cursor column		*/

EXT int		thisflag;		/* Flags, this command		*/

EXT int		lastflag;		/* Flags, last command		*/

EXT int		curgoal;		/* Goal for C-P, C-N		*/

EXT WINDOW	*curwp;			/* Current window		*/

EXT BUFFER	*curbp;			/* Current buffer		*/

EXT WINDOW	*wheadp;		/* Head of list of windows	*/

EXT BUFFER	*bheadp;		/* Head of list of buffers	*/



EXT BUFFER	*bfind();		/* Lookup a buffer by name	*/

EXT WINDOW	*wpopup();		/* Pop up window creation	*/

EXT LINE	*lalloc();		/* Allocate a line		*/

#if	KANJI

EXT KS_FLAG	kcode;			/* global file code		*/

#endif	/*KANJI*/

EXT Ckey	*kbdptr;		/* current position in keyboard buf */

EXT Char    pat[NPAT];		/* Search pattern		*/

EXT Char	tap[NPAT];		/* Reversed pattern array.	*/

EXT Char	rpat[NPAT];		/* replacement pattern		*/



/* The variable matchlen holds the length of the matched

 * string - used by the replace functions.

 * The variable patmatch holds the string that satisfies

 * the search command.

 * The variables matchline and matchoff hold the line and

 * offset position of the start of match.

 */

EXT int		matchlen		INIT(0);

EXT int		mlenold			INIT(0);

EXT Char	*patmatch		INIT(NULL);

EXT LINE	*matchline		INIT(NULL);

EXT int		matchoff		INIT(0);



#if	MAGIC

/*

 * The variable magical determines if there are actual

 * metacharacters in the string - if not, then we don't

 * have to use the slower MAGIC mode search functions.

 */

EXT int		magical			INIT(FALSE);

EXT MC		mcpat[NPAT];		/* the magic pattern		*/

EXT MC		tapcm[NPAT];		/* the reversed magic pattern	*/



#endif /* MAGIC */



/* terminal table defined only in TERM.C */



#ifndef termdef

extern	TERM	term;			/* Terminal information.	*/

#endif



EXT int	undo_limit	INIT(0);	/* $undo-limit: 0 = unlimited */
EXT int	u_in_undo	INIT(FALSE);	/* TRUE while undo/redo in progress */
EXT int	u_new_bndry	INIT(TRUE);	/* set TRUE before each command	*/

#undef EXT

#undef INIT

/*	Undo utility functions (used by line.c, buffer.c)	*/
extern void u_record(int type, LINE *lp, int offset, int length,
                     Char *data, int datalen, int eob);
extern void u_clear_undo(BUFFER *bp);
extern void u_prune(BUFFER *bp);

#endif /* EDEF_H_ */
