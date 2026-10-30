#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*
 * The routines in this file implement commands that work word or a
 * paragraph at a time.	 There are all sorts of word mode commands.  If I
 * do any sentence mode commands, they are likely to be put in this file. 
 */

#include	"ecomm.h"
#include	"estruct.h"
#include	"edef.h"

#if	KANJI
static int isword_gap(Char c1, Char c2);
#endif

/* Word wrap on n-spaces. Back-over whatever precedes the point on the current
 * line and stop on the first word-break or the beginning of the line. If we
 * reach the beginning of the line, jump back to the end of the word and start
 * a new line.	Otherwise, break the line at the word-break, eat it, and jump
 * back to the end of the word.
 * Returns TRUE on success, FALSE on errors.
 */
/*ARGSUSED*/
int wrapword(int f, int n)
{
	int cnt;	/* size of word wrapped to next line */
	Char c;	/* charector temporary */

	/* backup from the <NL> 1 char */
	if (!backchar(0, 1))
		return(FALSE);

	/* back up until we aren't in a word,
	   make sure there is a break in the line */
	cnt = 0;
	while (((c = lgetc(curwp->w_dotp, curwp->w_doto)) != ' ')
#if KANJI
				&& !iskanji(c) && !iskana(c) && !isunicode(c)
#endif
				&& (c != '\t')) {
		cnt++;
		if (!backchar(0, 1))
			return(FALSE);
		/* if we make it to the beginning, start a new line */
		if (curwp->w_doto == 0) {
			(void)gotoeol(FALSE, 0);
			return(lnewline());
		}
	}

#if KANJI
	if (iskanji(c) || iskana(c) || isunicode(c)) {
		if (!forwchar(0, 1)) return FALSE;
	} else {
#endif
	/* delete the forward white space */
	if (!forwdel(0, 1))
		return(FALSE);
#if KANJI
	}
#endif

	/* put in a end of line */
	if (!lnewline())
		return(FALSE);

	/* and past the first word */
	while (cnt-- > 0) {
		if (forwchar(FALSE, 1) == FALSE)
			return(FALSE);
	}
	return(TRUE);
}

/*
 * Move the cursor backward by "n" words. All of the details of motion are
 * performed by the "backchar" and "forwchar" routines. Error if you try to
 * move beyond the buffers.
 */
int backword(int f, int n)
{
	if (n < 0)
		return (forwword(f, -n));
	while (n--) {
		do {
			if (backchar(FALSE, 1) == FALSE)
				return (FALSE);
		} while (top_of_word() == FALSE);
	}
	return TRUE;
}

/*
 * Move the cursor forward by the specified number of words. All of the motion
 * is done by "forwchar". Error if you try and move beyond the buffer's end.
 */
int forwword(int f, int n)
{
	if (n < 0)
		return (backword(f, -n));
	while (n--) {
#if	NFWORD
		do {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		} while (top_of_word() == FALSE);
#else	/* !NFWORD */
		while (end_of_word() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		}
		if (forwchar(FALSE, 1) == FALSE)
			return (FALSE);
#endif	/* NFWORD */
	}
	return(TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move,
 * convert any characters to upper case. Error if you try and move beyond the
 * end of the buffer. Bound to "M-U".
 */
/*ARGSUSED*/
int upperword(int f, int n)
{
	Char	c;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return (FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		}
		for (;;) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (islower(c)) {
				lputc(curwp->w_dotp, curwp->w_doto, CHCASE(c));
				lchange(WFHARD);
			}
			if (end_of_word()) break;
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		}
		if (forwchar(FALSE, 1) == FALSE)
			return (FALSE);
	}
	return (TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move
 * convert characters to lower case. Error if you try and move over the end of
 * the buffer. Bound to "M-L".
 */
/*ARGSUSED*/
int lowerword(int f, int n)
{
	Char	c;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return (FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		}
		for (;;) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (isupper(c)) {
				lputc(curwp->w_dotp, curwp->w_doto, CHCASE(c));
				lchange(WFHARD);
			}
			if (end_of_word()) break;
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		}
		if (forwchar(FALSE, 1) == FALSE)
			return (FALSE);
	}
	return (TRUE);
}

/*
 * Move the cursor forward by the specified number of words. As you move
 * convert the first character of the word to upper case, and subsequent
 * characters to lower case. Error if you try and move past the end of the
 * buffer. Bound to "M-C".
 */
/*ARGSUSED*/
int capword(int f, int n)
{
	Char	c;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return (FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
		}
		c = lgetc(curwp->w_dotp, curwp->w_doto);
		if (islower(c)) {
			lputc(curwp->w_dotp, curwp->w_doto, CHCASE(c));
			lchange(WFHARD);
		}
		while (!end_of_word()) {
		    if (forwchar(FALSE, 1) == FALSE)
			    return (FALSE);
		    c = lgetc(curwp->w_dotp, curwp->w_doto);
		    if (isupper(c)) {
			lputc(curwp->w_dotp, curwp->w_doto, CHCASE(c));
			lchange(WFHARD);
		    }
		}
		if (forwchar(FALSE, 1) == FALSE)
		    return (FALSE);
	}
	return (TRUE);
}

/*
 * Kill forward by "n" words. Remember the location of dot. Move forward by
 * the right number of words. Put dot back where it was and issue the kill
 * command for the right number of characters. Bound to "M-D".
 */
/*ARGSUSED*/
int delfword(int f, int n)
{
	LINE	*dotp;
	int	doto;
	long size;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return (FALSE);
	if ((lastflag&CFKILL) == 0)	/* Clear kill buffer if */
		kdelete();		/* last wasn't a kill.	*/
	thisflag |= CFKILL;
	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	size = 0;
	while (n--) {
#if	NFWORD
		if (iseol()) {
			if (forwchar(FALSE,1) == FALSE)
				return(FALSE);
			++size;
		}

		if (inword()) {
		  while (!end_of_word()) {
		    if (forwchar(FALSE,1) == FALSE)
		      return(FALSE);
		    ++size;
		  }
		  if (forwchar(FALSE,1) == FALSE)
		    return(FALSE);
		  ++size;

		  while (iswhite()) {
		    if (forwchar(FALSE, 1) == FALSE)
		      return (FALSE);
		    ++size;
		  }
		} else {
		  while (!top_of_word() && !iseol()) {
		    if (forwchar(FALSE,1) == FALSE)
		      return(FALSE);
		    ++size;
		  }
		}
#else	/*!NFWORD*/
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
			++size;
		}

		while (!end_of_word()) {
			if (forwchar(FALSE, 1) == FALSE)
				return (FALSE);
			++size;
		}
		if (forwchar(FALSE, 1) == FALSE)
			return (FALSE);
		++size;
#endif	/*!NFWORD*/
	}
	curwp->w_dotp = dotp;
	curwp->w_doto = doto;
	return (ldelete(size, TRUE));
}

/*
 * Kill backwards by "n" words. Move backwards by the desired number of words,
 * counting the characters. When dot is finally moved to its resting place,
 * fire off the kill command. Bound to "M-Rubout" and to "M-Backspace".
 */
/*ARGSUSED*/
int delbword(int f, int n)
{
	long size;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return (FALSE);
	if ((lastflag&CFKILL) == 0)	/* Clear kill buffer if */
		kdelete();		/* last wasn't a kill.	*/
	thisflag |= CFKILL;
	size = 0;
	while (n--) {
		do {
			if (backchar(FALSE, 1) == FALSE)
				return (FALSE);
			++size;
		} while (top_of_word() == FALSE);
	}
	return (ldelete(size, TRUE));
}

/*
 * Return TRUE if the character at dot is a character that is considered to be
 * part of a word. The word character list is hard coded. Should be setable.
 */
int inword(void)
{
	Char	c;

	if (iseol())
		return (FALSE);
	c = lgetc(curwp->w_dotp, curwp->w_doto);
	if (isletter(c) ||
	    isdigit(c) ||
#if KANJI
	    (iskanji(c) && c != CCHR(0x2121)) ||
#endif
	    c=='$' || c=='_')			/* For identifiers	*/
		return (TRUE);
	return (FALSE);
}

/*
 * Return TRUE if dot points to the top of a word.
 */
int top_of_word(void)
{
	Char c1, c2;
	int s;

	if (!inword())
	    return FALSE;
	if (curwp->w_doto <= 0)
	    return TRUE;
	curwp->w_doto--;
	s = inword();
	curwp->w_doto++;
	if (!s)
	    return TRUE;
#if	KANJI
	c1 = lgetc(curwp->w_dotp, curwp->w_doto);
	c2 = lgetc(curwp->w_dotp, curwp->w_doto-1);
	return isword_gap(c2, c1);
#else	/*!KANJI*/
	return FALSE;
#endif	/*KANJI*/
}

/*
 * Return TRUE if dot points to the end of a word.
 */
int end_of_word(void)
{
	Char c1, c2;
	int s;

	if (!inword())
	    return FALSE;
	if (iseol())
	    return TRUE;
	curwp->w_doto++;
	s = inword();
	curwp->w_doto--;
	if (!s)
	    return TRUE;
#if	KANJI
	c1 = lgetc(curwp->w_dotp, curwp->w_doto);
	c2 = lgetc(curwp->w_dotp, curwp->w_doto+1);
	return isword_gap(c1, c2);
#else	/*!KANJI*/
	return FALSE;
#endif	/*KANJI*/
}

#if	KANJI
/*
 * Return TRUE if there is gap between c1 and c2.
 */
int isword_gap(Char c1, Char c2)
{

    if (isascii(c1) && isascii(c2) ||
	iskana(c1) && iskana(c2)) {
	return FALSE;
    }
    if (iskanji(c1) && iskanji(c2)) {
	return ch_type(c1) != ch_type(c2);
    }
    return TRUE;
}

/*
 * character type of Kanji.
 */
enum ch_type {
    CH_SYMBOL,
    CH_NUMBER,
    CH_ALPHA,
    CH_HIRA,
    CH_KATA,
    CH_GREEK,
    CH_RUSSIAN,
    CH_KANJI,
};

int ch_type(Char c)
{
    int h;

    h = (c >> 8) & 0xff;
    if (h < 0x30) {
	switch (h) {
	case 0x23:
	    return ((c & 0xff) == 0x30)? (int)CH_NUMBER:
					 (int)CH_ALPHA;
	case 0x24:
	    return (int)CH_HIRA;
	case 0x25:
	    return (int)CH_KATA;
	case 0x26:
	    return (int)CH_GREEK;
	case 0x27:
	    return (int)CH_RUSSIAN;
	}
	return (int)CH_SYMBOL;
    }
    return (int)CH_KANJI;
}
#endif	/*KANJI*/

#if	WORDPRO

#define BEGIN	if (1) {
#define END	} else

/*ARGSUSED*/
int fillpara(int f, int n)  /* Fill the current paragraph according to the current
		   fill column						*/
{
	Char c;		/* current char durring scan	*/
	int wordlen;		/* length of current word	*/
	int clength;		/* position on line during fill */
	int eopflag;		/* Are we at the End-Of-Paragraph? */
	enum {
		sf_none,
		sf_space,
		sf_nl
	}	    sflag;		/* separator info */
	enum {
		wf_norm,
		wf_dot
	}	    lwflag, cwflag;	/* last and current word info */
	LINE *eopline;		/* pointer to line just past EOP */
	Char wbuf[NSTRING];		/* buffer for current word	*/

	/* flush current word */
#define flush_word() BEGIN\
		int i;\
		/* check this word */\
		i = 0;\
		if (lwflag == wf_dot) i++;\
		if (sflag != sf_none) i++;\
		if (clength+i+wordlen <= fillcol) {\
			/* add this word to this line */\
			/* spaces between last word */\
			(void)linsert(i, ' ');\
			clength += i;\
		} else {\
			/* wrap to next line */\
			(void)lnewline();\
			clength = 0;\
		}\
		/* then insert word, anyway */\
		for (i=0; i<wordlen; )\
			(void)linsert(1, wbuf[i++]);\
		/* update status */\
		clength += wordlen;\
		lwflag = cwflag;\
		wordlen = 0;\
		/* sfalg should be set properly */\
	END /* flush_word */

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (fillcol == 0) {	/* no fill column set */
		mlwrite(
#if KMSGS
		    kterminal? CSTR(&Cxstr[8548]):
#endif
		    CSTR(&Cxstr[8568]));
		return(FALSE);
	}

	/* record the pointer to the line just past the EOP */
	(void)gotoeop(FALSE, 1);
	eopline = lforw(curwp->w_dotp);

	/* and back top the begining of the paragraph */
	(void)gotobop(FALSE, 1);

	/* initialize various info */
	clength = getccol(FALSE);
	wordlen = 0;
	lwflag = cwflag = wf_norm;
	sflag = sf_none;

	/* scan through lines, filling words */
	eopflag = FALSE;
	do {
		/* get the next character in the paragraph */
		if (curwp->w_doto == llength(curwp->w_dotp)) {
			c = '\n';
			if (lforw(curwp->w_dotp) == eopline)
				eopflag = TRUE;
		} else
			c = lgetc(curwp->w_dotp, curwp->w_doto);
		/* and then delete it */
		(void)ldelete(1L, FALSE);

		/* examine current character */
		if (c == ' ' || c == '\t' || c == '\n') {
			/* word separator */
			if (wordlen) {
				flush_word();
				sflag = sf_space;
			} else {
				if (c == '\n' && sflag == sf_none)
					sflag = sf_nl;
				else
					sflag = sf_space;
			}
		}
#if KANJI
		/* else if (c == KSPACE) { */
			/* KANJI space */
			/* is there any difference with other KANJI chars? */
		/* } */
		else if (iskanji(c) || iskana(c) || isunicode(c)) {
			int sp = 0;

			/* KANJI or KANA character */
			if (wordlen) {
				flush_word();
				sflag = sf_none;
			}
			if (lwflag == wf_dot) sp++;
			if (sflag == sf_space) sp++;
			if (should_wrap(c, clength+sp)) {
				(void)lnewline();
				clength = 0;
			} else {
				(void)linsert(sp, ' ');
				clength += sp;
			}
			(void)linsert(1, c);
			clength += char_width(c);
			lwflag = wf_norm;
			sflag = sf_none;
		}
#endif /* KANJI */
		else {
			/* normal word character */
			cwflag = (c == '.')? wf_dot: wf_norm;
			if (wordlen < NSTRING-1)
				wbuf[wordlen++] = c;
		}
	} while (!eopflag);
	/* and add a last newline for the end of our new paragraph */
	if (eopline == curbp->b_linep) {
		/* end of buffer. not add newline. instead, advance dot */
		(void)forwchar(FALSE, 1);
	} else {
		(void)lnewline();
	}
	return(TRUE);
}

/*ARGSUSED*/
int killpara(int f, int n)  /* delete n paragraphs starting with the current one */
{
	int status;	/* returned status of functions */

	while (n--) {		/* for each paragraph to delete */

		/* mark out the end and begining of the para to delete */
		(void)gotoeop(FALSE, 1);

		/* set the mark here */
		curwp->w_markp = curwp->w_dotp;
		curwp->w_marko = curwp->w_doto;

		/* go to the beginning of the paragraph */
		(void)gotobop(FALSE, 1);
		curwp->w_doto = 0;	/* force us to the beginning of line */

		/* and delete it */
		if ((status = killregion(FALSE, 1)) != TRUE)
			return(status);

		/* and clean up the 2 extra lines */
		(void)ldelete(2L, TRUE);
	}
	return(TRUE);
}


/*	wordcount:	count the # of words in the marked region,
			along with average word sizes, # of chars, etc,
			and report on them.			*/

/*ARGSUSED*/
int wordcount(int f, int n)
{
	LINE *lp;	/* current line to scan */
	int offset;	/* current char to scan */
	long size;		/* size of region left to count */
	Char ch;	/* current character to scan */
	int wordflag;	/* are we in a word now? */
	int lastword;	/* were we just in a word? */
	long nwords;		/* total # of words */
	long nchars;		/* total number of chars */
	int nlines;		/* total number of lines in region */
	int avgch;		/* average number of chars/word */
	int status;		/* status return code */
	REGION region;		/* region to look at */

	/* make sure we have a region to count */
	if ((status = getregion(&region)) != TRUE)
		return(status);
	lp = region.r_linep;
	offset = region.r_offset;
	size = region.r_size;

	/* count up things */
	lastword = FALSE;
	nchars = 0L;
	nwords = 0L;
	nlines = 0;
	while (size--) {

		/* get the current character */
		if (offset == llength(lp)) {	/* end of line */
			ch = '\n';
			lp = lforw(lp);
			offset = 0;
			++nlines;
		} else {
			ch = lgetc(lp, offset);
			++offset;
		}

		/* and tabulate it */
		wordflag = (isletter(ch) || isdigit(ch) ||
#if KANJI
			    (iskanji(ch)) || isunicode(ch) ||
#endif
			    (ch == '$' || ch == '_'));
		if (wordflag == TRUE && lastword == FALSE)
			++nwords;
		lastword = wordflag;
		++nchars;
	}

	/* and report on the info */
	if (nwords > 0L)
		avgch = (int)((100L * nchars) / nwords);
	else
		avgch = 0;

	mlwrite(
#if KMSGS
	    kterminal? CSTR(&Cxstr[8587]):
#endif
	    CSTR(&Cxstr[8618]),
		nwords, nchars, nlines + 1, avgch);
	return(TRUE);
}
#endif /*WORDPRO*/
#if KANJI
/*
 * return TRUE if character C placed at column COL should be wrapped around.
 */
int should_wrap(Char c, int col)
{
    return ((col += char_width(c)) >= fillcol && kinsoku1(c)) ||
	   (col > fillcol && !kinsoku2(c));
}

/*
 * KANA codes in following text is not portable in case inner
 * representation is changed.
 */

/*
 * return TRUE if C is a character which should never be placed
 * at the end of the line.
 */
int kinsoku1(Char c)
{
    switch (c) {
    case '(':
    case '<':
    case '{':
    case '[':
	return (TRUE);
    }
    switch (c) {
    case CCHR(0x2146): /* opening quote */
    case CCHR(0x2148):
    case CCHR(0x214a): /* opening paren */
    case CCHR(0x214c):
    case CCHR(0x214e):
    case CCHR(0x2150):
    case CCHR(0x2152):
    case CCHR(0x2154):
    case CCHR(0x2156):
    case CCHR(0x2158):
    case CCHR(0x215a):
	return TRUE;
    }
    switch (c) { /* for KANA */
    case 0x122: /* oepning paren */
	return TRUE;
    }
    return FALSE;
}

/*
 * return TRUE if C is a character which should never be placed
 * at the top of line.
 */
int kinsoku2(Char c)
{
    switch (c) {
    case '!':
    case ')':
    case ',':
    case '.':
    case '?':
    case ';':
    case ':':
    case ']':
    case '}':
    case '>':
	return (TRUE);
    }
    switch (c) {
    case CCHR(0x2122): /* tou-ten */
    case CCHR(0x2123): /* ku-ten */
    case CCHR(0x2124): /* comma */
    case CCHR(0x2125): /* peirod */
    case CCHR(0x2126): /* mid dot */
    case CCHR(0x2129): /* question mark */
    case CCHR(0x212a): /* exclaimation mark */
    case CCHR(0x212b): /* daku-ten */
    case CCHR(0x212c): /* han-daku-ten */
    case CCHR(0x2133): /* kurikaeshi */
    case CCHR(0x2134): /* daku-ten tuki kurikaeshi */
    case CCHR(0x2135): /* kurikaeshi */
    case CCHR(0x2136): /* daku-ten tuki kurikaeshi */
    case CCHR(0x2137): /* kurikaeshi */
    case CCHR(0x2138): /* kurikaeshi */
    case CCHR(0x2139): /* kurikaeshi */
    case CCHR(0x213c): /* chou-on kigou */
    case CCHR(0x2147): /* closing single quote */
    case CCHR(0x2149): /* closing double quote */
    case CCHR(0x214b): /* closing paren */
    case CCHR(0x214d): /* closing broad-angled brace */
    case CCHR(0x214f): /* closing brace */
    case CCHR(0x2151): /* closing curly brace */
    case CCHR(0x2157): /* closing angle bracket */
    case CCHR(0x2159): /* closing double angle bracket */
    case CCHR(0x215b): /* closing filled bracket */
    case CCHR(0x2421): case CCHR(0x2521): /* small 'a' */
    case CCHR(0x2423): case CCHR(0x2523): /* small 'i' */
    case CCHR(0x2425): case CCHR(0x2525): /* small 'u' */
    case CCHR(0x2427): case CCHR(0x2527): /* small 'e' */
    case CCHR(0x2429): case CCHR(0x2529): /* small 'o' */
    case CCHR(0x2443): case CCHR(0x2543): /* small 'tsu' */
    case CCHR(0x2463): case CCHR(0x2563): /* small 'ya' */
    case CCHR(0x2465): case CCHR(0x2565): /* small 'yu' */
    case CCHR(0x2467): case CCHR(0x2567): /* small 'yo' */
    case CCHR(0x246e): case CCHR(0x256e): /* small 'wa' */
    case CCHR(0x2575): /* small 'ka' */
    case CCHR(0x2576): /* small 'ke' */
	return TRUE;
    }
    switch (c) { /* for KANA */
    case 0x121: /* kuten */
    case 0x123: /* closing paren */
    case 0x124: /* touten */
    case 0x125: /* nakaten */
    case 0x127: /* small "a" */
    case 0x128: /* small "i" */
    case 0x129: /* small "u" */
    case 0x12a: /* small "e" */
    case 0x12b: /* small "o" */
    case 0x12c: /* small "ya" */
    case 0x12d: /* small "yu" */
    case 0x12e: /* small "yo" */
    case 0x12f: /* small "tsu" */
    case 0x130: /* cho-on kigou */
    case 0x15e: /* dakuten */
    case 0x15f: /* han-dakuten */
	return TRUE;
    }
    return FALSE;
}

#endif /*KANJI*/

/*
 *  return TRUE if the current character is whitespace,
 *  else return FALSE.
 */

int iswhite(void)
{
  Char c;

  if (iseol())
    return (FALSE);
  c = lgetc(curwp->w_dotp, curwp->w_doto);
  if (c == ' ' || c == '\t') return (TRUE);
  return (FALSE);
}

int iseol(void)
{
    return (curwp->w_doto == llength(curwp->w_dotp));
}
