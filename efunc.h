#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*	EFUNC.H:	MicroEMACS function declarations and names



		This file list all the C code functions used by MicroEMACS

	and the names to use to bind keys to them. To add functions,

	declare it here in both the extern function list and the name

	binding table.



*/



#include <signal.h>	/* to see whether SIGTSTP is defined */



/*	External function declarations		*/



extern	int	ctrlg(int f, int n);
extern	int	quit(int f, int n);
extern	int	ctlxlp(int f, int n);
extern	int	ctlxrp(int f, int n);
extern	int	ctlxe(int f, int n);
extern	int	fileread(int f, int n);
extern	int	filefind(int f, int n);
extern	int	filewrite(int f, int n);
extern	int	filesave(int f, int n);
extern	int	filename(void);
extern	int	gotobol(int f, int n);
extern	int	forwchar(int f, int n);
extern	int	gotoeol(int f, int n);
extern	int	backchar(int f, int n);
extern	int	forwline(int f, int n);
extern	int	backline(int f, int n);
extern	int	forwpage(int f, int n);
extern	int	backpage(int f, int n);
extern	int	gotobob(int f, int n);
extern	int	gotoeob(int f, int n);
extern	int	setfillcol(int f, int n);
extern	int	setmark(int f, int n);
extern	int	swapmark(int f, int n);
extern	int	forwsearch(int f, int n);
extern	int	backsearch(int f, int n);
extern	int	sreplace(int f, int n);
extern	int	qreplace(int f, int n);
extern	int	showcpos(int f, int n);
extern	int	nextwind(void);
extern	int	prevwind(int f, int n);
extern	int	onlywind(int f, int n);
extern	int	splitwind(int f, int n);
extern	int	mvdnwind(int f, int n);
extern	int	mvupwind(int f, int n);
extern	int	enlargewind(int f, int n);
extern	int	shrinkwind(int f, int n);
extern	int	listbuffers(int f, int n);
extern	int	usebuffer(int f, int n);
extern	int	killbuffer(int f, int n);
extern	int	reposition(int f, int n);
extern	int	refreshscr(int f, int n);
extern	int	twiddle(int f, int n);
extern	int	tab(int f, int n);
extern	int	newline(int f, int n);
extern	int	indent(int f, int n);
extern	int	openline(int f, int n);
extern	int	deblank(int f, int n);
extern	int	quote(int f, int n);
extern	int	backword(int f, int n);
extern	int	forwword(int f, int n);
extern	int	forwdel(int f, int n);
extern	int	backdel(int f, int n);
extern	int	killtext(int f, int n);
extern	int	yank(int f, int n);
extern	int	upperword(int f, int n);
extern	int	lowerword(int f, int n);
extern	int	upperregion(int f, int n);
extern	int	lowerregion(int f, int n);
extern	int	capword(int f, int n);
extern	int	delfword(int f, int n);
extern	int	delbword(int f, int n);
extern	int	killregion(int f, int n);
extern	int	copyregion(int f, int n);
extern	int	spawncli(int f, int n);
extern	int	spawn(int f, int n);
extern	int	shell_escape_cmd(int f, int n);
char *shell_command_escape_char(const char *);
#ifdef SIGTSTP

extern	int	bktoshell(void);
extern SIGRET_T rtfrmshell(SIGARG_T(dummy));
#endif

extern	int	quickexit(int f, int n);
extern	int	set_mode(int f, int n);
extern	int	delmode(int f, int n);
extern	int	gotoline(int f, int n);
extern	int	namebuffer(int f, int n);
#if	WORDPRO

extern	int	gotobop(int f, int n);
extern	int	gotoeop(int f, int n);
extern	int	fillpara(void);
#endif

extern	int	help(void);
extern	int	deskey(int f, int n);
extern	int	viewfile(int f, int n);
extern	int	insfile(int f, int n);
extern	int	scrnextup(int f, int n);
extern	int	scrnextdw(int f, int n);
extern	int	bindtokey(int f, int n);
extern	int	unbindkey(int f, int n);
extern	int	namedcmd(int f, int n);
extern	int	desbind(void);
extern	int	execcmd(int f, int n);
extern	int	execbuf(int f, int n);
extern	int	execfile(int f, int n);
extern	int	nextbuffer(int f, int n);
#if	WORDPRO

extern	int	killpara(void);
#endif

extern	int	setgmode(int f, int n);
extern	int	delgmode(int f, int n);
extern	int	insspace(int f, int n);
extern	int	forwhunt(int f, int n);
extern	int	backhunt(int f, int n);
extern	int	pipecmd(int f, int n);
extern	int	filter(int f, int n);
extern	int	delwind(int f, int n);
extern	int	cbufn(int f, int n);
extern	int	cbuf1(int f, int n);
extern	int	cbuf2(int f, int n);
extern	int	cbuf3(int f, int n);
extern	int	cbuf4(int f, int n);
extern	int	cbuf5(int f, int n);
extern	int	cbuf6(int f, int n);
extern	int	cbuf7(int f, int n);
extern	int	cbuf8(int f, int n);
extern	int	cbuf9(int f, int n);
extern	int	cbuf10(int f, int n);
extern	int	cbuf11(int f, int n);
extern	int	cbuf12(int f, int n);
extern	int	cbuf13(int f, int n);
extern	int	cbuf14(int f, int n);
extern	int	cbuf15(int f, int n);
extern	int	cbuf16(int f, int n);
extern	int	cbuf17(int f, int n);
extern	int	cbuf18(int f, int n);
extern	int	cbuf19(int f, int n);
extern	int	cbuf20(int f, int n);
extern	int	cbuf21(int f, int n);
extern	int	cbuf22(int f, int n);
extern	int	cbuf23(int f, int n);
extern	int	cbuf24(int f, int n);
extern	int	cbuf25(int f, int n);
extern	int	cbuf26(int f, int n);
extern	int	cbuf27(int f, int n);
extern	int	cbuf28(int f, int n);
extern	int	cbuf29(int f, int n);
extern	int	cbuf30(int f, int n);
extern	int	cbuf31(int f, int n);
extern	int	cbuf32(int f, int n);
extern	int	cbuf33(int f, int n);
extern	int	cbuf34(int f, int n);
extern	int	cbuf35(int f, int n);
extern	int	cbuf36(int f, int n);
extern	int	cbuf37(int f, int n);
extern	int	cbuf38(int f, int n);
extern	int	cbuf39(int f, int n);
extern	int	cbuf40(int f, int n);
extern	int	storemac(int f, int n);
extern	int	resize(int f, int n);
extern	int	clrmes(int f, int n);
extern	int	meta_pfx(void);
extern	int	cex(void);
extern	int	unarg(void);
extern	int	istring(void);
extern	int	unmark(void);
#if	ISRCH

extern	int	fisearch(int f, int n);
extern	int	risearch(int f, int n);
#endif

#if	WORDPRO

extern	int	wordcount(int f, int n);
#endif

extern	int	savewnd(int f, int n);
extern	int	restwnd(int f, int n);
extern	int	upscreen(int f, int n);
extern	int	writemsg(int f, int n);
extern	int	clearkill(int f, int n);
extern	int	noop(int f, int n);
#if	FLABEL

extern	int	fnclabel(int f, int n);
#endif

#if	APROP

extern	int	apro(int f, int n);
#endif

#if	CRYPT

extern	int	setcrkey(int f, int n);
#endif

extern	int	wrapword(int f, int n);
#if	CFENCE

extern	int	getfence(int f, int n);
#endif

extern	int	newsize(int f, int n);
extern	int	setvar(int f, int n);
extern	int	newwidth(int f, int n);
extern	int	changewd(int f, int n);
extern	int	toggle_line_numbers(int f, int n);
extern	int	set_line_number_width(int f, int n);
extern	int	pyinsert(void);
extern	int	pyindent(int f, int n);
extern	int	py_indent_region(int f, int n);
extern	int	py_backspace(int f, int n);
extern	int	py_mode(int f, int n);
extern	int	cinsert(void);
extern	int	cindent(int f, int n);
extern	int	cindent_region(int f, int n);
extern	int	c_mode(int f, int n);
extern	int	c_detect_style(int f, int n);
extern	int	c_follow_source_cmd(int f, int n);
#if AUTOMODE

extern	int	automode(int f, int n);
#endif

extern	int	recedit(bool toplev, int n);
extern	int	exitrecedit(int f, int n);
#if	AEDIT

extern	int	trim(int f, int n);
extern	int	detab(int f, int n);
extern	int	entab(int f, int n);
#endif

#if	PROC

extern	int	storeproc(void);
extern	int	execproc(int f, int n);
#endif

extern	int	updmode(int f, int n);
extern	int	millisleep(int f, int n);
extern	int	undo(int f, int n);
extern	int	redo(int f, int n);


/*	Name to function binding table



		This table gives the names of all the bindable functions

	end their C function address. These are used for the bind-to-key

	function.

*/



NBIND	names[] = {

	{CSTR(&Cxstr[367]),		ctrlg},

	{CSTR(&Cxstr[381]),		set_mode},

	{CSTR(&Cxstr[390]),	setgmode},

#if	APROP

	{CSTR(&Cxstr[406]),		apro},

#endif

#if AUTOMODE

	{CSTR(&Cxstr[414]),	automode},

#endif

	{CSTR(&Cxstr[433]),	backchar},

	{CSTR(&Cxstr[452]),		ctlxlp},

	{CSTR(&Cxstr[464]),	gotobob},

	{CSTR(&Cxstr[482]),	gotobol},

	{CSTR(&Cxstr[500]),		bindtokey},

	{CSTR(&Cxstr[512]),	showcpos},

	{CSTR(&Cxstr[528]),	lowerregion},

	{CSTR(&Cxstr[546]),	upperregion},

	{CSTR(&Cxstr[564]),	capword},

	{CSTR(&Cxstr[585]),	lowerword},

	{CSTR(&Cxstr[601]),	upperword},

	{CSTR(&Cxstr[617]),			changewd},

	{CSTR(&Cxstr[620]),	filename},

	{CSTR(&Cxstr[637]),	newsize},

	{CSTR(&Cxstr[656]),	newwidth},

	{CSTR(&Cxstr[676]),	refreshscr},

	{CSTR(&Cxstr[693]),	clearkill},

	{CSTR(&Cxstr[711]),	clrmes},

	{CSTR(&Cxstr[730]),		copyregion},

#if	WORDPRO

	{CSTR(&Cxstr[742]),		wordcount},

#endif

	{CSTR(&Cxstr[754]),		cex},

	{CSTR(&Cxstr[766]),	deblank},

	{CSTR(&Cxstr[785]),		killbuffer},

	{CSTR(&Cxstr[799]),		delmode},

	{CSTR(&Cxstr[811]),	delgmode},

	{CSTR(&Cxstr[830]),	forwdel},

	{CSTR(&Cxstr[852]),	delfword},

	{CSTR(&Cxstr[869]),	onlywind},

	{CSTR(&Cxstr[890]),	backdel},

	{CSTR(&Cxstr[916]),	delbword},

	{CSTR(&Cxstr[937]),		delwind},

	{CSTR(&Cxstr[951]),	desbind},

	{CSTR(&Cxstr[969]),		deskey},

#if	AEDIT

	{CSTR(&Cxstr[982]),		detab},

#endif

	{CSTR(&Cxstr[993]),		ctlxrp},

	{CSTR(&Cxstr[1003]),		gotoeob},

	{CSTR(&Cxstr[1015]),		gotoeol},

#if	AEDIT

	{CSTR(&Cxstr[1027]),		entab},

#endif

	{CSTR(&Cxstr[1038]),	swapmark},

	{CSTR(&Cxstr[1062]),	execbuf},

	{CSTR(&Cxstr[1077]),	execcmd},

	{CSTR(&Cxstr[1098]),		execfile},

	{CSTR(&Cxstr[1111]),		ctlxe},

	{CSTR(&Cxstr[1125]),	cbufn},

	{CSTR(&Cxstr[1141]),	cbuf1},

	{CSTR(&Cxstr[1157]),	cbuf2},

	{CSTR(&Cxstr[1173]),	cbuf3},

	{CSTR(&Cxstr[1189]),	cbuf4},

	{CSTR(&Cxstr[1205]),	cbuf5},

	{CSTR(&Cxstr[1221]),	cbuf6},

	{CSTR(&Cxstr[1237]),	cbuf7},

	{CSTR(&Cxstr[1253]),	cbuf8},

	{CSTR(&Cxstr[1269]),	cbuf9},

	{CSTR(&Cxstr[1285]),	cbuf10},

	{CSTR(&Cxstr[1302]),	cbuf11},

	{CSTR(&Cxstr[1319]),	cbuf12},

	{CSTR(&Cxstr[1336]),	cbuf13},

	{CSTR(&Cxstr[1353]),	cbuf14},

	{CSTR(&Cxstr[1370]),	cbuf15},

	{CSTR(&Cxstr[1387]),	cbuf16},

	{CSTR(&Cxstr[1404]),	cbuf17},

	{CSTR(&Cxstr[1421]),	cbuf18},

	{CSTR(&Cxstr[1438]),	cbuf19},

	{CSTR(&Cxstr[1455]),	cbuf20},

	{CSTR(&Cxstr[1472]),	cbuf21},

	{CSTR(&Cxstr[1489]),	cbuf22},

	{CSTR(&Cxstr[1506]),	cbuf23},

	{CSTR(&Cxstr[1523]),	cbuf24},

	{CSTR(&Cxstr[1540]),	cbuf25},

	{CSTR(&Cxstr[1557]),	cbuf26},

	{CSTR(&Cxstr[1574]),	cbuf27},

	{CSTR(&Cxstr[1591]),	cbuf28},

	{CSTR(&Cxstr[1608]),	cbuf29},

	{CSTR(&Cxstr[1625]),	cbuf30},

	{CSTR(&Cxstr[1642]),	cbuf31},

	{CSTR(&Cxstr[1659]),	cbuf32},

	{CSTR(&Cxstr[1676]),	cbuf33},

	{CSTR(&Cxstr[1693]),	cbuf34},

	{CSTR(&Cxstr[1710]),	cbuf35},

	{CSTR(&Cxstr[1727]),	cbuf36},

	{CSTR(&Cxstr[1744]),	cbuf37},

	{CSTR(&Cxstr[1761]),	cbuf38},

	{CSTR(&Cxstr[1778]),	cbuf39},

	{CSTR(&Cxstr[1795]),	cbuf40},

	{CSTR(&Cxstr[1812]),	namedcmd},

#if	PROC

	{CSTR(&Cxstr[1834]),	execproc},

#endif

	{CSTR(&Cxstr[1852]),		quit},

	{CSTR(&Cxstr[1863]),	exitrecedit},

#if	WORDPRO

	{CSTR(&Cxstr[1883]),	fillpara},

#endif

	{CSTR(&Cxstr[1898]),		filter},

	{CSTR(&Cxstr[1912]),		filefind},

	{CSTR(&Cxstr[1922]),	forwchar},

	{CSTR(&Cxstr[1940]),		gotoline},

#if	CFENCE

	{CSTR(&Cxstr[1950]),	getfence},

#endif

	{CSTR(&Cxstr[1970]),		enlargewind},

	{CSTR(&Cxstr[1982]),		tab},

	{CSTR(&Cxstr[1993]),		forwhunt},

	{CSTR(&Cxstr[2006]),		backhunt},

	{CSTR(&Cxstr[2020]),			help},

	{CSTR(&Cxstr[2025]),		spawncli},

#if	ISRCH

	{CSTR(&Cxstr[2033]),	fisearch},

#endif

	{CSTR(&Cxstr[2052]),		insfile},

	{CSTR(&Cxstr[2064]),		insspace},

	{CSTR(&Cxstr[2077]),		istring},

#if	WORDPRO

	{CSTR(&Cxstr[2091]),	killpara},

#endif

	{CSTR(&Cxstr[2106]),		killregion},

	{CSTR(&Cxstr[2118]),	killtext},

#if	FLABEL

	{CSTR(&Cxstr[2138]),	fnclabel},

#endif

	{CSTR(&Cxstr[2157]),		listbuffers},

	{CSTR(&Cxstr[2170]),		meta_pfx},

	{CSTR(&Cxstr[2182]),	mvdnwind},

	{CSTR(&Cxstr[2199]),	mvupwind},

	{CSTR(&Cxstr[2214]),		namebuffer},

	{CSTR(&Cxstr[2226]),		newline},

	{CSTR(&Cxstr[2234]),	indent},

	{CSTR(&Cxstr[2253]),		nextbuffer},

	{CSTR(&Cxstr[2265]),		forwline},

	{CSTR(&Cxstr[2275]),		forwpage},

#if	WORDPRO

	{CSTR(&Cxstr[2285]),	gotoeop},

#endif

	{CSTR(&Cxstr[2300]),		nextwind},

	{CSTR(&Cxstr[859]),		forwword},

	{CSTR(&Cxstr[2312]),			noop},

	{CSTR(&Cxstr[2318]),		openline},

	{CSTR(&Cxstr[2328]),		pipecmd},

	{CSTR(&Cxstr[2341]),		backline},

	{CSTR(&Cxstr[2355]),		backpage},

#if	WORDPRO

	{CSTR(&Cxstr[2369]),	gotobop},

#endif

	{CSTR(&Cxstr[2388]),	prevwind},

	{CSTR(&Cxstr[923]),		backword},

	{CSTR(&Cxstr[2404]),	qreplace},

	{CSTR(&Cxstr[2425]),		quickexit},

	{CSTR(&Cxstr[2436]),	quote},

	{CSTR(&Cxstr[2452]),		fileread},

	{CSTR(&Cxstr[1868]),	recedit},

	{CSTR(&Cxstr[2462]),	reposition},

	{CSTR(&Cxstr[2477]),		resize},

	{CSTR(&Cxstr[2491]),	restwnd},

	{CSTR(&Cxstr[2410]),	sreplace},

#if	ISRCH

	{CSTR(&Cxstr[2506]),

					risearch},

#endif

#if	PROC

	{CSTR(&Cxstr[2533]),			execproc},

#endif

	{CSTR(&Cxstr[2537]),		filesave},

	{CSTR(&Cxstr[2547]),		savewnd},

	{CSTR(&Cxstr[2559]),	scrnextdw},

	{CSTR(&Cxstr[2583]),	scrnextup},

 	{CSTR(&Cxstr[2605]),	forwsearch},

	{CSTR(&Cxstr[2620]),	backsearch},

	{CSTR(&Cxstr[2635]),		usebuffer},

	{CSTR(&Cxstr[2649]),			setvar},

#if	CRYPT

	{CSTR(&Cxstr[2653]),	setcrkey},

#endif

	{CSTR(&Cxstr[2672]),	setfillcol},

	{CSTR(&Cxstr[2688]),		setmark},

	{CSTR(&Cxstr[2697]),		spawn},
	{CSTR(&Cxstr[2711]),	shell_escape_cmd},

	{CSTR(&Cxstr[2732]),		shrinkwind},

	{CSTR(&Cxstr[2746]),	millisleep}, /* Added by N.Nide */

	{CSTR(&Cxstr[2762]),	splitwind},

	{CSTR(&Cxstr[2783]),		storemac},

#if	PROC

	{CSTR(&Cxstr[2795]),	storeproc},

#endif

#ifdef SIGTSTP

	{CSTR(&Cxstr[2811]),		bktoshell},

#endif

	{CSTR(&Cxstr[2825]),		redo},
	{CSTR(&Cxstr[2830]),	twiddle},

#if	AEDIT

	{CSTR(&Cxstr[2851]),		trim},

#endif

	{CSTR(&Cxstr[2861]),		unbindkey},

	{CSTR(&Cxstr[2872]),	unarg},

	{CSTR(&Cxstr[2891]),		unmark},

	{CSTR(&Cxstr[2905]),		undo},
	{CSTR(&Cxstr[2910]),	updmode},

	{CSTR(&Cxstr[2926]),		upscreen},

	{CSTR(&Cxstr[2940]),		viewfile},

	{CSTR(&Cxstr[2950]),		wrapword},

	{CSTR(&Cxstr[2960]),		filewrite},

	{CSTR(&Cxstr[2971]),		writemsg},

	{CSTR(&Cxstr[2985]),			yank},
	{CSTR(&Cxstr[2990]),	toggle_line_numbers},
	{CSTR(&Cxstr[3010]),	set_line_number_width},

	{CSTR(&Cxstr[3032]),		pyindent},
	{CSTR(&Cxstr[3042]),	py_indent_region},
	{CSTR(&Cxstr[3059]),		py_backspace},
	{CSTR(&Cxstr[3072]),		py_mode},
	{CSTR(&Cxstr[3080]),		cindent},
	{CSTR(&Cxstr[3088]),	cindent_region},
	{CSTR(&Cxstr[3103]),		c_mode},
	{CSTR(&Cxstr[3110]),	c_detect_style},
	{CSTR(&Cxstr[3125]),	c_follow_source_cmd},
	{CSTR(&Cxstr[6]),			NULL}

};
