#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*	EVAR.H:	Environment and user variable definitions
		for MicroEMACS

		written 1986 by Daniel Lawrence
*/

/*	structure to hold user variables and their definitions	*/

typedef struct UVAR {
	Char u_name[NVSIZE + 1];		/* name of user variable */
	Char *u_value;				/* value (string) */
} UVAR;

/*	current user variables (This structure will probably change)	*/

#define	MAXVARS		100

UVAR uv[MAXVARS];	/* user variables */

/* environment variable structure */

typedef struct ENVAR {
	Char *ev_name;			/* name of variable */
	byte ev_class;			/* local or global */
	char *ev_addr;			/* address of related object */
	Char *(*ev_gethook)();		/* routine to get the value */
	int (*ev_puthook)();		/* routine to put the value */
} ENVAR;

#define EV_LOCAL	0
#define EV_GLOBAL	1

static BUFFER dummy_buffer;

#define BUF_OFS(f)	(char *)&dummy_buffer.f
#define BUF_ADR(o)	((char *)curbp+((char *)o-(char *)&dummy_buffer))

#if	SHORTNAME
# define get_curchar_hook	get_cchr_hook
# define get_curcol_hook	get_ccol_hook
# define get_curline_hook	get_clin_hook
# define get_curwidth_hook	get_cwid_hook
# define put_curchar_hook	put_cchr_hook
# define put_curcol_hook	put_ccol_hook
# define put_curline_hook	put_clin_hook
# define put_curwidth_hook	put_cwid_hook
#endif	/*SHORTNAME*/

/*
 * hook functions: declared as following
 *
 *	Char *get_hook(ENVAR *);
 *	int put_hook(ENVAR *, Char *);
 */
/* hook for putting to readonly variables */
static int readonly_hook();
/* general get hooks */
static Char *get_int_hook(), *get_bool_hook(), *get_str_hook();
/* general put hooks */
static int put_int_hook(), put_bool_hook(), put_str_hook();

/* variable specific hooks */
static Char *get_pagelen_hook(), *get_curcol_hook(), *get_curline_hook();
static int put_pagelen_hook(), put_curcol_hook(), put_curline_hook();
static int put_curwidth_hook(), put_cbufname_hook(), put_cfname_hook();
#if KANJI
static int put_kterminal_hook();
static Char *get_kcode_hook();
static int put_kcode_hook();
static int put_tcode_hook();
static int put_bufcode_hook();
#endif
static Char *get_curchar_hook();
static int put_curchar_hook();
static int put_modeline_hook();
static int put_showlinenum_hook(), put_linenumwidth_hook();
#if	CLOCK
static int put_cperiod_hook();
#endif

/*	list of recognized environment variables	*/
ENVAR envars[] = {
	{CSTR(&Cxstr[3184]), EV_GLOBAL,	/* current fill column */
		(char *)&fillcol, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3192]), EV_GLOBAL,	/* number of lines used by editor */
		NULL, get_pagelen_hook, put_pagelen_hook},
	{CSTR(&Cxstr[3200]), EV_LOCAL,	/* current column pos of cursor */
		NULL, get_curcol_hook, put_curcol_hook},
	{CSTR(&Cxstr[3207]), EV_LOCAL,	/* current line in file */
		NULL, get_curline_hook, put_curline_hook},
	{CSTR(&Cxstr[3215]), EV_GLOBAL,	/* current screen width */
		(char *)&term.t_ncol, get_int_hook, put_curwidth_hook},
	{CSTR(&Cxstr[3224]), EV_LOCAL,	/* current buffer name */
		BUF_OFS(b_bname), get_str_hook, put_cbufname_hook},
	{CSTR(&Cxstr[3233]), EV_LOCAL,	/* current file name */
		BUF_OFS(b_fname), get_str_hook, put_cfname_hook},
	{CSTR(&Cxstr[3240]),	EV_GLOBAL,	/* macro debugging */
		(char *)&macbug, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3246]), EV_GLOBAL,	/* status of last command */
		(char *)&cmdstatus, get_bool_hook, put_bool_hook},
#if KANJI
	{CSTR(&Cxstr[3253]), EV_GLOBAL,	/* display KANJI messages */
		(char *)&kterminal, get_bool_hook, put_kterminal_hook},
	{CSTR(&Cxstr[3259]), EV_GLOBAL,	/* global file code */
		(char *)&kcode, get_kcode_hook, put_kcode_hook},
	{CSTR(&Cxstr[3265]), EV_GLOBAL,	/* terminal code */
		(char *)&term.t_code, get_kcode_hook, put_tcode_hook},
	{CSTR(&Cxstr[3271]), EV_LOCAL,	/* current buffer's file code */
		BUF_OFS(b_code), get_kcode_hook, put_bufcode_hook},
#endif
#if ASAVE
	{CSTR(&Cxstr[3277]), EV_GLOBAL,	/* # of chars between auto-saves */
	   (char *)&gasave, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3283]), EV_GLOBAL,	/* # of chars until next auto-save */
	   (char *)&gacount, get_int_hook, put_int_hook},
#endif
	{CSTR(&Cxstr[3290]), EV_GLOBAL,	/* last keyboard char struck */
	   (char *)&lastkey, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3298]), EV_GLOBAL,	/* current character under the cursor */
	   NULL, get_curchar_hook, put_curchar_hook},
	{CSTR(&Cxstr[3306]), EV_GLOBAL,	/* display commands on command line */
	   (char *)&discmd, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3313]), EV_GLOBAL,	/* current version number */
	   NULL, version, readonly_hook},
	{CSTR(&Cxstr[3321]), EV_GLOBAL,	/* returns current prog name */
	   NULL, prgname, readonly_hook},
	{CSTR(&Cxstr[3330]), EV_GLOBAL,	/* returns last revised date */
	   NULL, revdate, readonly_hook},
	{CSTR(&Cxstr[3338]), EV_GLOBAL,	/* current random number seed */
	   (char *)&seed, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3343]), EV_GLOBAL,	/* display command line input characters */
	   (char *)&disinp, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3350]), EV_GLOBAL,	/* always escape shell arguments */
	   (char *)&shell_escape_mode, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3369]), EV_GLOBAL,	/* global mode line format */
	   (char *)&gmodeline, get_str_hook, put_modeline_hook},
	{CSTR(&Cxstr[3379]), EV_LOCAL,	/* buffer mode line format */
		BUF_OFS(b_modeline), get_str_hook, put_modeline_hook},
#if	CLOCK
	{CSTR(&Cxstr[3389]), EV_GLOBAL,	/* period of timer in seconds */
		(char *)&cperiod, get_int_hook, put_cperiod_hook},
#endif
	{CSTR(&Cxstr[3397]),	EV_GLOBAL,	/* current time string */
		NULL, getclock, readonly_hook},

	{CSTR(&Cxstr[3402]), EV_GLOBAL, /* max undo records per buffer (0=unlim) */
	   (char *)&undo_limit, get_int_hook, put_int_hook},

	{CSTR(&Cxstr[3413]), EV_GLOBAL,	/* current search pattern */
		(char *)pat, NULL, NULL},
	{CSTR(&Cxstr[3423]), EV_GLOBAL,	/* current replace pattern */
		(char *)rpat, NULL, NULL},
	{CSTR(&Cxstr[3434]), EV_GLOBAL,	/* show line number gutter */
	   (char *)&show_line_numbers, get_bool_hook, put_showlinenum_hook},
	{CSTR(&Cxstr[3446]), EV_GLOBAL,	/* width of line number gutter */
	   (char *)&line_number_width, get_int_hook, put_linenumwidth_hook},
	{CSTR(&Cxstr[3459]), EV_GLOBAL,	/* Python indent width (spaces) */
	   (char *)&py_indent_width, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3475]), EV_GLOBAL,	/* Use spaces for Python indent */
	   (char *)&py_use_spaces, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3489]), EV_GLOBAL,	/* C indent width (spaces) */
	   (char *)&c_indent_width, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3504]), EV_GLOBAL,	/* Use spaces for C indent */
	   (char *)&c_use_spaces, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3517]), EV_GLOBAL,	/* C brace style (0-7, 7=none) */
	   (char *)&c_brace_style, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3531]), EV_GLOBAL,	/* C continuation indent */
	   (char *)&c_cont_indent, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3545]), EV_GLOBAL,	/* case label indent relative to switch */
	   (char *)&c_case_indent, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3559]), EV_GLOBAL,	/* Preprocessor directive indent */
	   (char *)&c_pp_indent, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3571]), EV_GLOBAL,	/* Auto-close brace indentation */
	   (char *)&c_auto_close, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3590]), EV_GLOBAL,	/* Auto-detect C indent style from source */
	   (char *)&c_auto_detect, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3125]), EV_GLOBAL,	/* Follow existing source indentation */
	   (char *)&c_follow_source, get_bool_hook, put_bool_hook},
	{CSTR(&Cxstr[3604]), EV_GLOBAL,	/* GNU style half-indent offset (columns) */
	   (char *)&c_gnu_indent, get_int_hook, put_int_hook},
	{CSTR(&Cxstr[3624]), EV_GLOBAL,	/* Stroustrup: function brace on own line */
	   (char *)&c_brace_func, get_bool_hook, put_bool_hook},
};

#define	NEVARS	(sizeof(envars) / sizeof(ENVAR))

/*	list of recognized user functions	*/

enum FUNC_ADITY {
  NILNAMIC,
  MONAMIC,
  DYNAMIC,
  TRINAMIC,
};

typedef struct UFUNC {
	Char *f_name;	/* name of function */
	enum FUNC_ADITY f_type;
} UFUNC;

UFUNC funcs[] = {
	CSTR(&Cxstr[3648]), DYNAMIC,		/* add two numbers together */
	CSTR(&Cxstr[3652]), DYNAMIC,		/* subtraction */
	CSTR(&Cxstr[3656]), DYNAMIC,		/* multiplication */
	CSTR(&Cxstr[3660]), DYNAMIC,		/* division */
	CSTR(&Cxstr[3664]), DYNAMIC,		/* mod */
	CSTR(&Cxstr[3668]), MONAMIC,		/* negate */
	CSTR(&Cxstr[3672]), DYNAMIC,		/* concatinate string */
	CSTR(&Cxstr[3676]), DYNAMIC,		/* left string(string, len) */
	CSTR(&Cxstr[3680]), DYNAMIC,		/* right string(string, pos) */
	CSTR(&Cxstr[3684]), TRINAMIC,	/* mid string(string, pos, len) */
	CSTR(&Cxstr[3688]), MONAMIC,		/* logical not */
	CSTR(&Cxstr[3692]), DYNAMIC,		/* logical equality check */
	CSTR(&Cxstr[3696]), DYNAMIC,		/* logical less than */
	CSTR(&Cxstr[3700]), DYNAMIC,		/* logical greater than */
	CSTR(&Cxstr[3704]), DYNAMIC,		/* string logical equality check */
	CSTR(&Cxstr[3708]), DYNAMIC,		/* string logical less than */
	CSTR(&Cxstr[3712]), DYNAMIC,		/* string logical greater than */
	CSTR(&Cxstr[3716]), MONAMIC,		/* evaluate indirect value */
	CSTR(&Cxstr[3720]), MONAMIC,		/* get UNIX environment */
	CSTR(&Cxstr[3196]), MONAMIC,		/* length of string */
	CSTR(&Cxstr[377]), DYNAMIC,		/* logical and */
	CSTR(&Cxstr[3724]),  DYNAMIC,		/* logical or */
	CSTR(&Cxstr[3727]), MONAMIC,		/* uppercase string */
	CSTR(&Cxstr[3731]), MONAMIC,		/* lower case string */
	CSTR(&Cxstr[3735]), MONAMIC,		/* Truth of the universe logical test */
	CSTR(&Cxstr[3739]), MONAMIC,		/* char to integer conversion */
	CSTR(&Cxstr[3743]), MONAMIC,		/* integer to char conversion */
	CSTR(&Cxstr[3747]), NILNAMIC,		/* get 1 charater */
	CSTR(&Cxstr[3751]), MONAMIC,		/* get a random number */
	CSTR(&Cxstr[3755]), MONAMIC,		/* absolute value of a number */
	CSTR(&Cxstr[3759]), MONAMIC,		/* function name bound to this key sequence */

	CSTR(&Cxstr[3763]), MONAMIC,
	CSTR(&Cxstr[3767]), MONAMIC,
	CSTR(&Cxstr[2711]), MONAMIC,	/* shell-escape a string argument */
};

#define	NFUNCS	sizeof(funcs) / sizeof(UFUNC)

/* 	and its preprocesor definitions		*/

enum UF_NUMBERS {
	UFADD,
	UFSUB,
	UFTIMES,
	UFDIV,
	UFMOD,
	UFNEG,
	UFCAT,
	UFLEFT,
	UFRIGHT,
	UFMID,
	UFNOT,
	UFEQUAL,
	UFLESS,
	UFGREATER,
	UFSEQUAL,
	UFSLESS,
	UFSGREAT,
	UFIND,
	UFENV,
	UFLENGTH,
	UFAND,
	UFOR,
	UFUPPER,
	UFLOWER,
	UFTRUTH,
	UFASCII,
	UFCHR,
	UFGTKEY,
	UFRND,
	UFABS,
	UFBIND,

	UFBMODE,
	UFGMODE,
	UFESCAPE,
};
