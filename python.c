#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*
 * This file contains the Python mode functions for kemacs.
 *
 * Python uses indentation as syntax (PEP 8).  These functions provide
 * automatic indentation support when the buffer has the PYMODE bit set:
 *
 *   - pyinsert:     called from newline() to auto-indent after a newline
 *   - pyindent:     re-indent the current line(s) to the correct level
 *   - py_indent_region: re-indent a selected region using Python rules
 *   - py_backspace:  called from backdel() to dedent on Backspace key
 *
 * Key behaviors:
 *   - A line ending with ':' increases the indent of the next line by
 *     $py-indent-width (default 4 spaces).
 *   - A line ending with '\\' is a continuation; the next line inherits
 *     the same indentation.
 *   - Dedent keywords (else:, elif:, except:, finally:) are aligned with
 *     the block opener (one indent level less than the block body).
 *   - Blank lines and comment lines are left at the current indent level.
 */

#include	"ecomm.h"
#include	"estruct.h"
#include	"edef.h"

/* tabsize is defined in random.c */
extern int tabsize;

/*
 * ---------------------------------------------------------------------------
 *  Helper functions
 * ---------------------------------------------------------------------------
 */

/*
 * Compute the display column of the indentation on a line.
 * Tabs are expanded to the next tab stop.
 */
static int
get_indent_cols(LINE *lp, int ts)
{
	int col = 0;
	int i;
	Char c;

	for (i = 0; i < llength(lp); i++) {
		c = lgetc(lp, i);
		if (c == ' ')
			++col;
		else if (c == '\t')
			col = (col / ts + 1) * ts;
		else
			break;
	}
	return col;
}

/*
 * Return TRUE if the last non-whitespace character on the line is ':'.
 * This detects Python block-openers (if/for/while/def/class/try/with, etc.)
 */
static int
line_ends_with_colon(LINE *lp)
{
	int i;

	for (i = llength(lp) - 1; i >= 0; i--) {
		Char c = lgetc(lp, i);
		if (c != ' ' && c != '\t')
			return (c == ':');
	}
	return FALSE;
}

/*
 * Return TRUE if the line ends with a backslash (line continuation).
 */
static int
line_ends_with_backslash(LINE *lp)
{
	int i;

	for (i = llength(lp) - 1; i >= 0; i--) {
		Char c = lgetc(lp, i);
		if (c != ' ' && c != '\t')
			return (c == '\\');
	}
	return FALSE;
}

/*
 * Return TRUE if the line (after skipping whitespace) starts with a
 * Python dedent keyword: else, elif, except, finally.
 * These keywords should be at the same indent as the block opener
 * (if/try/for/while/with) that contains them.
 */
static int
is_dedent_keyword(LINE *lp)
{
	int i;
	int len = llength(lp);

	/* skip leading whitespace */
	for (i = 0; i < len; i++) {
		Char c = lgetc(lp, i);
		if (c != ' ' && c != '\t')
			break;
	}

	if (i >= len)
		return FALSE;

#define CH(idx) lgetc(lp, i + (idx))

	/* else: */
	if (i + 4 <= len &&
	    CH(0) == 'e' && CH(1) == 'l' && CH(2) == 's' && CH(3) == 'e')
		if (i + 4 == len || CH(4) == ':' || CH(4) == ' ')
			return TRUE;

	/* elif: */
	if (i + 5 <= len &&
	    CH(0) == 'e' && CH(1) == 'l' && CH(2) == 'i' && CH(3) == 'f')
		if (i + 5 == len || CH(5) == ':' || CH(5) == ' ')
			return TRUE;

	/* except: */
	if (i + 7 <= len &&
	    CH(0) == 'e' && CH(1) == 'x' && CH(2) == 'c' &&
	    CH(3) == 'e' && CH(4) == 'p' && CH(5) == 't')
		if (i + 6 == len || CH(6) == ':' || CH(6) == ' ')
			return TRUE;

	/* finally: */
	if (i + 8 <= len &&
	    CH(0) == 'f' && CH(1) == 'i' && CH(2) == 'n' && CH(3) == 'a' &&
	    CH(4) == 'l' && CH(5) == 'l' && CH(6) == 'l' && CH(7) == 'y')
		if (i + 8 == len || CH(8) == ':' || CH(8) == ' ')
			return TRUE;

#undef CH
	return FALSE;
}

/*
 * Insert n spaces at the cursor position.
 * (Used when py_use_spaces is TRUE.)
 */
static int
insert_spaces(int n)
{
	int i;
	for (i = 0; i < n; i++) {
		if (linsert(1, ' ') == FALSE)
			return FALSE;
	}
	return TRUE;
}

/*
 * Insert indentation of 'cols' display columns, using spaces or tabs
 * depending on the py_use_spaces flag.
 */
static int
insert_indent(int cols)
{
	int ts = (tabsize > 0) ? tabsize : 8;

	if (py_use_spaces)
		return insert_spaces(cols);

	/* tab-based: use tabs for full tab stops, spaces for remainder */
	{
		int tabs = cols / ts;
		int spaces = cols % ts;
		int i;

		for (i = 0; i < tabs; i++)
			if (linsert(1, '\t') == FALSE)
				return FALSE;
		for (i = 0; i < spaces; i++)
			if (linsert(1, ' ') == FALSE)
				return FALSE;
	}
	return TRUE;
}

/*
 * Delete all leading whitespace on the current line and return the
 * number of columns it occupied.
 * Cursor is left at offset 0 on the current line.
 */
static int
delete_leading_whitespace(void)
{
	int i = 0;

	while (i < llength(curwp->w_dotp)) {
		Char c = lgetc(curwp->w_dotp, i);
		if (c != ' ' && c != '\t')
			break;
		i++;
	}

	curwp->w_doto = 0;
	if (i > 0)
		(void)ldelete((long)i, FALSE);

	return i;
}

/*
 * ---------------------------------------------------------------------------
 *  pyinsert() -- auto-indent on newline in Python mode
 * ---------------------------------------------------------------------------
 *
 * Called from newline() when the buffer has the MDPYMOD bit set and the
 * argument is the default (n == 1).  The cursor is on the current line
 * at offset curwp->w_doto.
 *
 *  1. Compute the indentation (in columns) of the current line.
 *  2. Check whether the text before the cursor ends with ':' or '\\'.
 *  3. Insert a newline (splits the line at the cursor).
 *  4. Insert the same indentation on the new line.
 *  5. If the line ended with ':', add one indentation level.
 *
 * This mirrors cinsert() for CMODE, but uses py_indent_width instead of
 * the hardcoded tab(8) logic.
 */
int
pyinsert(void)
{
	Char *cptr;	/* pointer into current line text */
	int tptr;	/* cursor offset on current line   */
	int ts;		/* effective tab size              */
	int indent_cols;/* indentation of current line (cols) */
	int i;
	int extra;	/* extra indent if line ends with ':' */

	cptr = &curwp->w_dotp->l_text[0];
	tptr = curwp->w_doto;
	ts = (tabsize > 0) ? tabsize : 8;

	/* --- 1. compute indentation of current line in columns --- */
	indent_cols = 0;
	for (i = 0; i < tptr; i++) {
		if (cptr[i] == ' ')
			++indent_cols;
		else if (cptr[i] == '\t')
			indent_cols = (indent_cols / ts + 1) * ts;
		else
			break;
	}

	/* --- 2. check if the line ends with ':' or backslash --- */
	extra = 0;
	if (tptr > 0) {
		Char last_ch = cptr[tptr - 1];
		if (last_ch == ':')
			extra = py_indent_width;
		/* if backslash-continuation, extra stays 0 -- inherit indent */
	}

	/* --- 3. insert the newline --- */
	if (lnewline() == FALSE)
		return FALSE;

	/* --- 4 & 5. insert indentation + possible extra --- */
	if (insert_indent(indent_cols + extra) == FALSE)
		return FALSE;

	return TRUE;
}

/*
 * ---------------------------------------------------------------------------
 *  pyindent() -- re-indent the current line (bound to M-J)
 * ---------------------------------------------------------------------------
 *
 * Examines the previous non-blank line and computes the correct
 * indentation for the current line:
 *
 *  - If the previous line ends with ':', add one indent level.
 *  - If the current line is a dedent keyword (else:, elif:, etc.),
 *    subtract one indent level (align with the block opener).
 *  - Continuation lines (after backslash) inherit the previous line's indent.
 *
 * The leading whitespace on the current line is deleted and replaced
 * with the computed indentation.
 */
/*ARGSUSED*/
int
pyindent(int f, int n)
{
	LINE *lp;	/* previous line */
	int ts;		/* effective tab size */
	int prev_indent;/* indent of previous non-blank line (cols) */
	int add_indent;	/* extra indent from ':' on previous line */
	int col;	/* target indentation column */
	int dedent_kw;	/* is current line a dedent keyword? */
	int i;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;
	if (n == 0)
		n = 1;		/* repeat count */

	ts = (tabsize > 0) ? tabsize : 8;

	for (i = 0; i < n; i++) {
		/* find the previous non-blank line */
		lp = lback(curwp->w_dotp);
		while (lp != curbp->b_linep &&
		       (llength(lp) == 0 ||
		        (get_indent_cols(lp, ts) == llength(lp))))
			lp = lback(lp);

		if (lp == curbp->b_linep) {
			/* no previous line -- indent from column 0 */
			col = 0;
		} else {
			prev_indent = get_indent_cols(lp, ts);

			add_indent = 0;
			if (line_ends_with_colon(lp))
				add_indent = py_indent_width;

			col = prev_indent + add_indent;

			/* dedent keywords go back one level */
			dedent_kw = is_dedent_keyword(curwp->w_dotp);
			if (dedent_kw)
				col -= py_indent_width;
		}

		if (col < 0)
			col = 0;

		/* delete current leading whitespace */
		delete_leading_whitespace();

		/* insert correct indentation */
		if (insert_indent(col) == FALSE)
			return FALSE;

		/* move down for repeat (if n > 1) */
		if (i < n - 1)
			(void)forwline(TRUE, 1);
	}

	lchange(WFEDIT);
	return TRUE;
}

/*
 * ---------------------------------------------------------------------------
 *  py_backspace() -- dedent on backspace in Python mode
 * ---------------------------------------------------------------------------
 *
 * Called from backdel() when the buffer has the MDPYMOD bit set and the
 * argument is the default (n == 1).
 *
 * If the cursor is on a line whose leading whitespace extends to the cursor
 * (i.e. all characters before the cursor are spaces or tabs), this function
 * deletes one indentation level instead of a single character:
 *
 *   - When py_use_spaces is TRUE, it removes up to py_indent_width
 *     spaces (or fewer if the line is indented less).
 *   - When py_use_spaces is FALSE, it removes one tab character.
 *
 * If the cursor is not at an indentation boundary (i.e. there is non-
 * whitespace before the cursor), or the cursor is at column 0, normal
 * backspace behavior is used (backchar + ldelete).
 */
/*ARGSUSED*/
int
py_backspace(int f, int n)
{
	LINE	*lp;
	int	ts;
	int	col;		/* display column of whitespace before cursor */
	int	i;
	int	delete_chars;

	if (curbp->b_mode & MDVIEW)
		return(rdonly());
	if (n < 0)
		return (forwdel(f, -n));

	if (f != FALSE) {		/* Really a kill.	*/
		if ((lastflag&CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}

	/* Only special-case single backspace */
	if (n == 1 && f == FALSE && (curbp->b_mode & MDPYMOD)) {
		ts = (tabsize > 0) ? tabsize : 8;
		lp = curwp->w_dotp;

		/* Check if all characters before cursor are whitespace */
		col = 0;
		for (i = 0; i < curwp->w_doto; i++) {
			Char c = lgetc(lp, i);
			if (c != ' ' && c != '\t')
				break;
			if (c == ' ')
				col++;
			else
				col = (col / ts + 1) * ts;
		}

		/* If all whitespace before cursor and cursor not at pos 0 */
		if (i == curwp->w_doto && curwp->w_doto > 0) {
			/* Delete one indentation step */
			if (py_use_spaces) {
				delete_chars = py_indent_width;
				if (delete_chars > curwp->w_doto)
					delete_chars = curwp->w_doto;
			} else {
				/* Tab mode: delete one character (whitespace) */
				delete_chars = 1;
			}

			curwp->w_doto -= delete_chars;
			(void)ldelete((long)delete_chars, f);
			lchange(WFEDIT);
			return TRUE;
		}
	}

	/* Normal backspace: move back and delete */
	if ((i = backchar(f, n)) == TRUE)
		i = ldelete((long)n, f);
	return (i);
}

/*
 * ---------------------------------------------------------------------------
 *  py_indent_region() -- re-indent a selected region (invoked via M-x py-format-region)
 * ---------------------------------------------------------------------------
 *
 * Uses an indent stack to track Python block levels and compute correct
 * indentation for each line in the region:
 *
 *   - Base indent is taken from the line before the region.
 *   - When a line ends with ':', push the next indent level (col + width).
 *   - When a line's current indent is less than the stack top AND the
 *     previous line did NOT end with ':' or '\\' (no new block or
 *     continuation), pop the stack. This detects block endings.
 *   - Dedent keywords (else:, elif:, etc.) go back one indent level.
 *     If the stack pop already brought us to the right level, no
 *     further adjustment is needed; otherwise, subtract one level.
 *   - Backslash continuations inherit the previous line's indent (no pop).
 *   - Blank lines are left untouched.
 *
 * Key fix: the stack pop is NOT triggered when the previous line ended
 * with ':' or '\\' -- a new block is starting (or continuation), so
 * the current line should be indented, not dedented. Previously, wrong
 * current indentation caused premature stack pops that prevented lines
 * after 'else:' from being indented correctly.
 *
 * Only the indentation (leading whitespace) of each line is modified;
 * the rest of the line is left untouched.
 */
/*ARGSUSED*/
int
py_indent_region(int f, int n)
{
	REGION		region;		/* region to process       */
	LINE		*lp;		/* current line in walk    */
	LINE		*last_lp;	/* last line of region     */
	LINE		*save_dotp;	/* saved cursor line       */
	int		ts;		/* effective tab size      */
	int		indent_stack[256];/* indent column stack    */
	int		sp;		/* stack pointer           */
	int		base_indent;	/* indent of line before region */
	int		current_indent;	/* indent of current line  */
	int		col;		/* target indent column    */
	int		remaining;	/* chars left in region    */
	int		offset;		/* offset on current line  */
	int		line_len;	/* length of current line  */
	int		rc;
	int		prev_colon;	/* did prev line end with ':'? */
	int		prev_backslash;/* did prev line end with '\\'? */
	int		popped;		/* did we pop the stack?  */

	if (curbp->b_mode & MDVIEW)
		return rdonly();

	rc = getregion(&region);
	if (rc != TRUE)
		return rc;

	ts = (tabsize > 0) ? tabsize : 8;

	/* --- find the last line that the region spans --- */
	last_lp = region.r_linep;
	offset = region.r_offset;
	remaining = (int)region.r_size;

	while (remaining > 0 && last_lp != curbp->b_linep) {
		line_len = llength(last_lp);
		if (offset < line_len)
			remaining -= (line_len - offset) + 1; /* content + newline */
		else
			remaining -= 1; /* just newline */
		if (remaining <= 0)
			break;
		last_lp = lforw(last_lp);
		offset = 0;
		if (last_lp == curbp->b_linep)
			break;
	}

	/* --- determine base indent from the line before the region --- */
	lp = lback(region.r_linep);
	if (lp == curbp->b_linep) {
		base_indent = 0;
		prev_colon = FALSE;
		prev_backslash = FALSE;
	} else {
		base_indent = get_indent_cols(lp, ts);
		prev_colon = line_ends_with_colon(lp);
		prev_backslash = line_ends_with_backslash(lp);
	}

	/* --- initialize indent stack --- */
	indent_stack[0] = base_indent;
	sp = 0;

	/* --- save cursor position --- */
	save_dotp = curwp->w_dotp;

	/* --- walk through each line of the region --- */
	lp = region.r_linep;
	while (1) {
		/* skip blank lines -- leave them untouched */
		if (llength(lp) == 0 ||
		    get_indent_cols(lp, ts) == llength(lp)) {
			if (lp == last_lp)
				break;
			lp = lforw(lp);
			if (lp == curbp->b_linep)
				break;
			continue;
		}

		current_indent = get_indent_cols(lp, ts);

		/*
		 * Pop the stack when the current line's indent is less
		 * than the stack top - this detects block endings.
		 * BUT: do NOT pop when the prev line ended with ':' or
		 * '\\' (new block starting or continuation - the line
		 * should be indented, not dedented).
		 */
		popped = FALSE;
		if (!prev_colon && !prev_backslash) {
			while (sp > 0 && current_indent < indent_stack[sp]) {
				sp--;
				popped = TRUE;
			}
		}

		col = indent_stack[sp];

		/*
		 * Dedent keywords (else:, elif:, etc.) align with the
		 * block opener. If the stack pop already brought us to
		 * the right level, no adjustment needed. Otherwise,
		 * subtract one level.
		 */
		if (sp > 0 && is_dedent_keyword(lp) && !popped) {
			col -= py_indent_width;
			if (col < 0)
				col = 0;
		}

		/* re-indent this line */
		curwp->w_dotp = lp;
		curwp->w_doto = 0;
		delete_leading_whitespace();
		if (insert_indent(col) == FALSE) {
			curwp->w_dotp = save_dotp;
			curwp->w_doto = 0;
			lchange(WFEDIT);
			return FALSE;
		}

		/* update context for next line */
		prev_colon = line_ends_with_colon(lp);
		prev_backslash = line_ends_with_backslash(lp);

		/* if line ends with ':', push next indent level */
		if (prev_colon) {
			if (sp < 255) {
				sp++;
				indent_stack[sp] = col + py_indent_width;
			}
		}

		if (lp == last_lp)
			break;
		lp = lforw(lp);
		if (lp == curbp->b_linep)
			break;
	}

	/* --- restore cursor position --- */
	curwp->w_dotp = save_dotp;
	curwp->w_doto = 0;

	lchange(WFHARD);
	return TRUE;
}
/*
 * ---------------------------------------------------------------------------
 *  py_mode() -- toggle Python mode on/off for the current buffer
 * ---------------------------------------------------------------------------
 *
 * With no argument, toggles the MDPYMOD bit on the current buffer.
 * With a positive argument, turns PYMODE on.
 * With a negative argument, turns PYMODE off.
 *
 * Registered as the named command "py-mode" in efunc.H_.
 * Invoke via:  M-x py-mode   (then type "py-mode" at the prompt)
 */
/*ARGSUSED*/
int
py_mode(int f, int n)
{
	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (f) {
		if (n > 0)
			curbp->b_mode |= MDPYMOD;
		else
			curbp->b_mode &= ~MDPYMOD;
	} else
		curbp->b_mode ^= MDPYMOD;
	upmode();
	curwp->w_flag |= WFMODE;
	mlwrite(CSTR(&Cxstr[8663]),
	    (curbp->b_mode & MDPYMOD) ? CSTR(&Cxstr[7198]) : CSTR(&Cxstr[7214]));
	return TRUE;
}
