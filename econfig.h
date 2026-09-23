#ifndef ECONFIG
#define ECONFIG

/*
 * Basic configuration of kemacs.
 * Further configurations may be done in estruct.h
 */

#ifdef BSD
#  undef BSD	/* to avoid confliction of 'BSD' definition below. (Nide) */
#endif

/* OS definitions */
#define BSD		0	/* 1: BSD, 0: System V (Linux/WSL) */

/* compiler facility */
#define STRASSOK	1	/* accepts struct/union assignment? */
#define SHORTNAME	0	/* need short name? */

/* OS versions */
/* for USG */
#define USG_5_4		1	/* System V R4 (ignored under BSD) */
/* I do not know about USG versions very well... */

/* Although in BSD environment, some as don't understand -R option */
#define HAVE_AS_R	0	/* 0: no -R, 1: has -R */

/* 1993/7/6 by NIDE, N. */
#define HAVE_VARARGS	1	/* handle varargs */
#define HAVE_STDARG	1	/* handle stdarg.h; has higher priority than
				   HAVE_VARARGS */
#define VOID_SIGFN	1	/* 2nd arg of signal() is (void*)() */
#define INT_SIGARG	0	/* signal handler takes an int arg */
#define USE_STTY_CMD	0	/* use external stty command to
				   set terminal mode */
#define HAVE_REGEX	1	/* have POSIX regex */
#define USE_ENVSZ	0	/* use COLUMNS and LINES environment to
				   determine screen size */
#define HAVE_TERMIOS	1	/* have termios struct and tc[sg]etattr
				   functions. */
#define HAVE_INSTALL	1	/* have install command. default is 1 under
				   BSD, and 0 under SysV */
#define USE_GETCWD	1	/* if defined to 1, use getcwd(), and
				   if defined to 0, use getwd() */
#define HAVE_SELECT	1	/* use select() */
#define HAVE_TIME_T	1	/* have time_t */
#define MALLOC_VOIDSTAR	1	/* malloc() returns void* */
#define HANDLE_UTF	1	/* handle UTF-8 (need iconv) */

/* KANJI usage */
#define KANJI		1		/* use KANJI feature	*/

/* KANJI code defaults (pre-definition changed to KS_UJIS by N.Nide) */
/* if DEF_*_CODE is not defined, BINARY is assumed. */
/* KI/RI are only needed for JIS */
/* File code */
#define DEF_F_CODE	KS_UTF8		/* JIS/UJIS/SJIS/UTF8	*/
#define DEF_F_KI	KS_NEWJIS	/* OLDJIS/NEWJIS	*/
#define DEF_F_RI	KS_ASCII	/* ROMAJI/ASCII/BOGUS	*/
#define DEF_F_EOL	KS_CRLF		/* LF/CR/CRLF		*/
/* Terminal code */
#define DEF_T_CODE	KS_UTF8		/* JIS/UJIS/SJIS/UTF8	*/
#define DEF_T_KI	KS_NEWJIS	/* NEWJIS	*/
#define DEF_T_RI	KS_ASCII	/* ROMAJI/ASCII/BOGUS	*/

#define DEF_T_THRU	1		/* can through 8bit?	*/

/* terminal capabilities */
#define KANJI_TERMCAP	"KJ"	/* KANJI capability */
/* if defined, default of disp code and key code are specified through	*/
/*	this capability.		*/
/*	if capability is not in TERMCAP, KANJI feature disabled by	*/
/*	default. if flag capability exist, KANJI enabled and above	*/
/*	defaults are used. if string capability "XYZ" exist, CODE, KI	*/
/*	and RI are set according to each.	*/
/*	X=j/J/U/S. if X==[Jj], Y & Z are needed. Y=@/B, Z=B/J/H.	*/
/*	If X=J, code is JIS and can through 8bit. X=j means 7bit only.	*/
/*	Other code implies 8bit through (of cource :-P)		*/
/*	e.g. capability of "KJ=j@H" means 7bit only, JIS-OLD-BOGUS	*/
/*	terminal, while "KJ=U" means 8bit UJIS terminal.		*/
/*	Note: Display code and Keyboard code cannot specified		*/
/*	separately in the TERMCAP.		*/
/* if not defined, specifications above are used.		*/

/******* end of configuration *******/

/* maintain consistency */

#ifdef USG
#  undef USG
#endif
#define USG		(!BSD)

#if BSD /* force some options */
# undef	 USG_5_4
#  define USG_5_4	0
#endif /*BSD*/
/* no idea of USG... */

#if HAVE_STDARG
# undef HAVE_VARARGS
#  define HAVE_VARARGS	0
#endif

#if VOID_SIGFN
# define SIGRET_T void
#else
# define SIGRET_T int
#endif
#if INT_SIGARG
# define SIGARG_T(x) int x
#else
# define SIGARG_T(x)
#endif

#endif /* #ifdef ECONFIG */
