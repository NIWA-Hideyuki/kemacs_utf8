#ifndef _CX_DEF_
extern unsigned int Cxstr[];
#define _CX_DEF_
#endif /*_CX_DEF_*/
/*
 * This file contains the modernized CMODE indent engine for kemacs.
 *
 * CMODE (MDCMOD bit) provides C language-aware indentation for
 * C89/C90/C99/C11/C17/C23.  The engine recognises C control-flow
 * keywords, tracks brace/paren/bracket depth, handles preprocessor
 * nesting, and skips comments and string literals when scanning.
 *
 * Public functions:
 *   cinsert()           -- auto-indent on Enter (replaces old cinsert)
 *   cindent()           -- re-indent current line (M-J)
 *   cindent_region()    -- re-indent selected region
 *   c_mode()            -- toggle CMODE on/off for current buffer
 *
 * Configuration evars (defined in edef.h):
 *   c_indent_width    (default 4)   indent step per level
 *   c_use_spaces      (default 0)   1=spaces, 0=real tabs
 *   c_brace_style     (default 0)   0=K&R, 1=Allman, 2=GNU, 3=Linux,
 *                                 4=Whitesmiths, 5=1TBS, 6=Stroustrup,
 *                                 7=none (auto-respect user placement)
 *   c_cont_indent     (default 2)   extra indent for continuation lines
 *   c_case_indent     (default 0)   case label indent relative to switch
 *   c_pp_indent       (default 0)   preprocessor directive indent
 *   c_auto_close      (default 1)   auto-dedent on closing brace
 *   c_auto_detect     (default 0)   auto-detect style from source
 *   c_follow_source   (default 0)   follow existing source indentation
 *   c_gnu_indent      (default 2)   GNU half-indent offset (columns)
 *   c_brace_func      (default 0)   Stroustrup: function { on own line
 */

#include "ecomm.h"
#include "estruct.h"
#include "edef.h"

/* tabsize is defined in random.c */
extern int tabsize;

#define TS ((tabsize > 0) ? tabsize : 8)

/* ---- Indent stack --------------------------------------------------- */

#define C_MAX_DEPTH 256

#define IST_BRACE    0  /* {  opened a block */
#define IST_PAREN    1  /* (  unclosed paren */
#define IST_BRACKET  2  /* [  unclosed bracket */
#define IST_PREPROC  3  /* #if / #ifdef */
#define IST_SWITCH   4  /* switch (for case/default alignment) */

/*
 * One entry on the backward-scan indent stack.
 *   level  -- column number of the indentation at this depth
 *   type   -- what opened this level
 */
typedef struct {
    int level;
    int type;
} CIndentStack;

static CIndentStack c_indstack[C_MAX_DEPTH];
static int          c_sp;

/* ---- Forward declarations of helpers used by the engine ------------ */

static int  c_skip_comment_or_string(LINE *lp, int offset);
static int  c_line_indent_cols(LINE *lp);
static int  c_last_nonws_char(LINE *lp, int offset);
static int  c_is_keyword(LINE *lp, int offset, const char *kw);
static int  c_starts_pp(LINE *lp, const char *kw);
static int  c_has_unclosed(LINE *lp, int offset, int *balance_out);
static int  c_scan_for_brace_level(LINE *start_lp);
static int  c_scan_for_matching_if(LINE *start_lp);
static int  c_is_function_definition(LINE *lp);
static int  c_line_starts_with_brace(LINE *lp);
static int  c_insert_indent(int cols);
static int  c_delete_indent(void);
static int  c_is_blank_line(LINE *lp);
static LINE *c_prev_nonblank(LINE *lp);
static int  c_is_ctrl_keyword(LINE *lp, int offset);

/* ==================================================================== */
/*  Helper functions                                                     */
/* ==================================================================== */

/*
 * Return the display-column count of leading whitespace on line lp.
 * Tabs advance to the next TS-column tab stop.
 */
static int
c_line_indent_cols(LINE *lp)
{
    int col = 0;
    int i;
    Char c;

    for (i = 0; i < llength(lp); i++) {
        c = lgetc(lp, i);
        if (c == ' ')
            ++col;
        else if (c == '\t')
            col = (col / TS + 1) * TS;
        else
            break;
    }
    return col;
}

/*
 * Return TRUE if the line is blank or whitespace-only.
 */
static int
c_is_blank_line(LINE *lp)
{
    int i;

    for (i = 0; i < llength(lp); i++) {
        Char c = lgetc(lp, i);
        if (c != ' ' && c != '\t')
            return FALSE;
    }
    return TRUE;
}

/*
 * Return the last non-whitespace character on line lp before 'offset'.
 * Returns '\0' if the line is all whitespace up to offset.
 */
static int
c_last_nonws_char(LINE *lp, int offset)
{
    int i;

    for (i = offset - 1; i >= 0; i--) {
        Char c = lgetc(lp, i);
        if (c != ' ' && c != '\t')
            return c;
    }
    return '\0';
}

/*
 * Given a line and an offset, return offset after skipping any
 * whitespace starting at position 'offset'.  If the character at
 * offset is not whitespace, return offset unchanged.
 */
static int
c_skip_ws(LINE *lp, int offset)
{
    while (offset < llength(lp)) {
        Char c = lgetc(lp, offset);
        if (c != ' ' && c != '\t')
            break;
        offset++;
    }
    return offset;
}

/*
 * Walk from 'offset' along line 'lp', skipping a C comment or string
 * literal if one starts at that position.  Returns the offset just
 * past the skipped construct, or 'offset' unchanged if no comment
 * or string starts there.
 */
static int
c_skip_comment_or_string(LINE *lp, int offset)
{
    Char c, c2;
    int  len = llength(lp);

    if (offset >= len)
        return offset;

    c = lgetc(lp, offset);

    /* line comment // ... */
    if (c == '/' && offset + 1 < len) {
        c2 = lgetc(lp, offset + 1);
        if (c2 == '/')
            return len;          /* rest of line is comment */
        if (c2 == '*') {         /* block comment */
            int i = offset + 2;
            while (i + 1 < len) {
                if (lgetc(lp, i) == '*' && lgetc(lp, i + 1) == '/')
                    return i + 2;
                i++;
            }
            return len;          /* unterminated on this line */
        }
    }

    /* string literal "..." */
    if (c == '"') {
        int i = offset + 1;
        while (i < len) {
            Char ch = lgetc(lp, i);
            if (ch == '\\' && i + 1 < len) {
                i += 2;
                continue;
            }
            if (ch == '"')
                return i + 1;
            if (ch == '\n')
                break;
            i++;
        }
        return i;
    }

    /* char literal '...' */
    if (c == '\'') {
        int i = offset + 1;
        while (i < len) {
            Char ch = lgetc(lp, i);
            if (ch == '\\' && i + 1 < len) {
                i += 2;
                continue;
            }
            if (ch == '\'')
                return i + 1;
            i++;
        }
        return i;
    }

    return offset;
}

/*
 * Check if line lp at 'offset' begins with keyword 'kw' followed by
 * a word boundary (whitespace, '(', or end of line).
 * The keyword must be preceded by a non-identifier character or start
 * of the significant (post-indent) text.
 */
static int
c_is_keyword(LINE *lp, int offset, const char *kw)
{
    int  klen = strlen(kw);
    int  len  = llength(lp);
    int  i;

    if (offset + klen > len)
        return FALSE;

    for (i = 0; i < klen; i++) {
        Char c = lgetc(lp, offset + i);
        char ek = kw[i];
        if (c != (Char)ek)
            return FALSE;
    }

    /* Check word boundary after keyword */
    if (offset + klen < len) {
        Char c = lgetc(lp, offset + klen);
        if (isletter(c) || isdigit(c) || c == '_')
            return FALSE;   /* part of a longer identifier */
    }

    /* Check word boundary before keyword (skip leading whitespace) */
    if (offset > 0) {
        Char c = lgetc(lp, offset - 1);
        if (isletter(c) || isdigit(c) || c == '_' || c == '.')
            return FALSE;   /* part of a longer identifier (e.g. foo_if) */
    }

    return TRUE;
}

/*
 * Check if a line starts with a preprocessor keyword.
 * Skips leading whitespace, then checks for the keyword after '#'.
 * 'kw' is matched after '#'.
 */
static int
c_starts_pp(LINE *lp, const char *kw)
{
    int offset;
    int len = llength(lp);

    offset = c_skip_ws(lp, 0);
    if (offset >= len)
        return FALSE;

    if (lgetc(lp, offset) != '#')
        return FALSE;
    offset++;

    /* skip whitespace between # and keyword */
    offset = c_skip_ws(lp, offset);
    if (offset >= len)
        return FALSE;

    return c_is_keyword(lp, offset, kw);
}

/*
 * Check if the line at offset starts with a C control-flow keyword.
 * Returns the indent contribution (0 = no special handling).
 */
static int
c_is_ctrl_keyword(LINE *lp, int offset)
{
    if (c_is_keyword(lp, offset, "if") ||
        c_is_keyword(lp, offset, "for") ||
        c_is_keyword(lp, offset, "while") ||
        c_is_keyword(lp, offset, "switch") ||
        c_is_keyword(lp, offset, "else") ||
        c_is_keyword(lp, offset, "do") ||
        c_is_keyword(lp, offset, "try") ||
        c_is_keyword(lp, offset, "catch") ||
        c_is_keyword(lp, offset, "finally"))
        return TRUE;
    return FALSE;
}

/*
 * Determine if the line at offset starts with a "dedent keyword" --
 * i.e., a keyword that should be aligned with the matching opening
 * control statement at the same indent level.
 *
 *   else   -- aligns with matching if
 *   do     -- do-while: } while aligns with do
 *   case   -- aligned with switch (or +c_case_indent)
 *   default -- aligned with switch (or +c_case_indent)
 */
static int
c_is_dedent_keyword(LINE *lp, int offset)
{
    if (c_is_keyword(lp, offset, "else") ||
        c_is_keyword(lp, offset, "case") ||
        c_is_keyword(lp, offset, "default"))
        return TRUE;
    /* closing brace: align with matching { */
    {
        int len = llength(lp);
        if (offset < len) {
            Char c = lgetc(lp, offset);
            if (c == '}')
                return TRUE;
        }
    }
    return FALSE;
}

/*
 * Check if a line (starting at 'offset', which should be past
 * leading whitespace) has an unclosed '(', '[', or '{' that is
 * still open at the end of the line.
 * Returns TRUE if the primary bracket is '(', '[', or '{'.
 * Sets *open_ch to the opening bracket char and *close_ch to the
 * matching close, or '\0' if none.
 */
static int
c_has_unclosed(LINE *lp, int offset, int *balance_out)
{
    int   i;
    int   len = llength(lp);
    int   paren_depth  = 0;
    int   bracket_depth = 0;
    int   brace_depth  = 0;

    for (i = offset; i < len; i++) {
        /* skip comments and strings */
        int skipped = c_skip_comment_or_string(lp, i);
        if (skipped != i) {
            i = skipped - 1;
            continue;
        }
        Char c = lgetc(lp, i);
        switch (c) {
        case '(': paren_depth++;  break;
        case ')': paren_depth--;  break;
        case '[': bracket_depth++; break;
        case ']': bracket_depth--; break;
        case '{': brace_depth++;  break;
        case '}': brace_depth--;  break;
        case '"': case '\'':
            /* skip string/char literal */
            i = c_skip_comment_or_string(lp, i) - 1;
            break;
        }
    }

    *balance_out = paren_depth * 100 + bracket_depth * 10 + brace_depth;
    return (*balance_out != 0);
}

/*
 * Return a pointer to the previous non-blank line.
 * Returns curbp->b_linep (header) if none.
 */
static LINE *
c_prev_nonblank(LINE *lp)
{
    lp = lback(lp);
    while (lp != curbp->b_linep && c_is_blank_line(lp))
        lp = lback(lp);
    return lp;
}

/*
 * Insert 'cols' columns of indentation using spaces or tabs
 * according to c_use_spaces. When using tabs, each indent level
 * (c_indent_width columns) produces one tab character; remainder
 * within a level uses spaces for alignment.
 */
static int
c_insert_indent(int cols)
{
    int i;

    if (cols <= 0)
        return TRUE;

    if (c_use_spaces) {
        for (i = 0; i < cols; i++) {
            if (linsert(1, ' ') == FALSE)
                return FALSE;
        }
    } else {
        int tabs = cols / c_indent_width;
        int spaces = cols % c_indent_width;
        for (i = 0; i < tabs; i++) {
            if (linsert(1, '\t') == FALSE)
                return FALSE;
        }
        for (i = 0; i < spaces; i++) {
            if (linsert(1, ' ') == FALSE)
                return FALSE;
        }
    }
    return TRUE;
}

/*
 * Delete all leading whitespace on the current line.
 * Leaves cursor at offset 0.
 */
static int
c_delete_indent(void)
{
    int i = 0;
    LINE *lp = curwp->w_dotp;

    while (i < llength(lp)) {
        Char c = lgetc(lp, i);
        if (c != ' ' && c != '\t')
            break;
        i++;
    }

    curwp->w_doto = 0;
    if (i > 0)
        (void)ldelete((long)i, FALSE);
    return TRUE;
}

/*
 * Scan backwards from 'start_lp' (exclusive -- the line after the
 * starting point) to determine the indent (column) at the current
 * brace / paren / bracket / preprocessor context.
 *
 * The scan walks backwards, tracking:
 *   - brace_depth: net { - } count
 *   - paren_depth:  net ( - ) count
 *   - bracket_depth: net [ - ] count
 *   - pp_depth:     #if/#ifdef/#ifndef - #endif count
 *
 * It stops when all depths are zero (balanced context found) or
 * when the buffer header is reached.
 *
 * Returns the base indentation column at which the current context
 * starts (i.e. the indent of the line that opened the current scope).
 */
static int
c_scan_for_brace_level(LINE *start_lp)
{
    LINE *lp;
    int  brace_depth   = 0;
    int  paren_depth   = 0;
    int  bracket_depth = 0;
    int  pp_depth      = 0;
    int  i, len;

    lp = lback(start_lp);
    while (lp != curbp->b_linep) {
        len = llength(lp);

        /* Check for preprocessor directives first */
        if (c_starts_pp(lp, "if") ||
            c_starts_pp(lp, "ifdef") ||
            c_starts_pp(lp, "ifndef")) {
            if (pp_depth == 0)
                return c_line_indent_cols(lp) + c_pp_indent;
            pp_depth--;
        } else if (c_starts_pp(lp, "endif")) {
            pp_depth++;
        } else if (c_starts_pp(lp, "else") ||
                   c_starts_pp(lp, "elif")) {
            /* #else / #elif: if pp_depth is 0, this is the same level as #if */
            if (pp_depth == 0)
                return c_line_indent_cols(lp);
            /* else stay in the #else block */
        }

        /* Scan the line for braces and brackets */
        for (i = 0; i < len; i++) {
            int skipped = c_skip_comment_or_string(lp, i);
            if (skipped != i) {
                i = skipped - 1;
                continue;
            }
            Char c = lgetc(lp, i);
            switch (c) {
            case '{': brace_depth++;   break;
            case '}': brace_depth--;   break;
            case '(': paren_depth++;  break;
            case ')': paren_depth--;   break;
            case '[': bracket_depth++; break;
            case ']': bracket_depth--; break;
            }
        }

        /* If all brackets are balanced, we've found the opening context */
        if (brace_depth == 0 && paren_depth == 0 &&
            bracket_depth == 0 && pp_depth == 0) {
            int offset = c_skip_ws(lp, 0);
            /* If this line starts with a control-flow keyword
               that opens a block -- its indent IS our target indent. */
            if (c_is_ctrl_keyword(lp, offset))
                return c_line_indent_cols(lp);
            /* Otherwise continue scanning backwards -- this line
               may be a body statement inside a block. */
            lp = lback(lp);
            continue;
        }

        /* If only braces are open (parens/brackets balanced),
           we found the brace level */
        if (brace_depth > 0 && paren_depth == 0 &&
            bracket_depth == 0 && pp_depth == 0) {
            /* This line's indent is the base for the block */
            return c_line_indent_cols(lp);
        }

        lp = lback(lp);
    }

    /* Reached top of buffer with unbalanced brackets */
    return 0;
}

/*
 * Scan backward from start_lp to find the matching 'if' for an 'else'.
 * Used by style 7 (none) to align else with its if.
 */
static int
c_scan_for_matching_if(LINE *start_lp)
{
    LINE *lp;
    int  brace_depth   = 0;
    int  paren_depth   = 0;
    int  i, len;

    lp = lback(start_lp);
    while (lp != curbp->b_linep) {
        len = llength(lp);

        /* Skip preprocessor lines - they affect brackets but not braces */
        if (c_starts_pp(lp, "if") ||
            c_starts_pp(lp, "ifdef") ||
            c_starts_pp(lp, "ifndef") ||
            c_starts_pp(lp, "elif") ||
            c_starts_pp(lp, "else")) {
            lp = lback(lp);
            continue;
        }

        for (i = 0; i < len; i++) {
            int skipped = c_skip_comment_or_string(lp, i);
            if (skipped != i) {
                i = skipped - 1;
                continue;
            }
            Char c = lgetc(lp, i);
            switch (c) {
            case '{': brace_depth++;   break;
            case '}': brace_depth--;   break;
            case '(': paren_depth++;  break;
            case ')': paren_depth--;   break;
            }
        }

        if (brace_depth == 0 && paren_depth == 0) {
            int offset = c_skip_ws(lp, 0);
            if (c_is_keyword(lp, offset, "else")) {
                lp = lback(lp);
                continue;
            }
            if (c_is_keyword(lp, offset, "if")) {
                return c_line_indent_cols(lp);
            }
        }
        lp = lback(lp);
    }
    return 0;
}

/*
 * Check if a line is a function definition: the line has a name followed
 * by a parenthesised parameter list and ends with '{' (or the '{' is
 * on the next line for Stroustrup style).
 */
static int
c_is_function_definition(LINE *lp)
{
    int offset = c_skip_ws(lp, 0);
    int len = llength(lp);
    int i, paren_depth = 0, found_paren = FALSE, found_close = FALSE;

    if (offset >= len)
        return FALSE;

    /* Skip type qualifiers and return type to get to the function name.
       Simple heuristic: look for '(' that closes, followed optionally by '{' */
    for (i = offset; i < len; i++) {
        int skipped = c_skip_comment_or_string(lp, i);
        if (skipped != i) {
            i = skipped - 1;
            continue;
        }
        Char c = lgetc(lp, i);
        switch (c) {
        case '(': paren_depth++; found_paren = TRUE; break;
        case ')': paren_depth--; break;
        case '{': if (paren_depth == 0 && found_paren) return TRUE; break;
        case ';': return FALSE;  /* not a function definition */
        }
    }
    /* If line ends with ')' and paren_depth == 0, and line before has '('
       and no ';' - could be a multi-line function signature.
       Also check for '{' at end of line after ')'. */
    if (paren_depth == 0 && found_paren) {
        /* Check if the last non-ws char is '{' (Stroustrup: func() { */
        int last = c_last_nonws_char(lp, len);
        if (last == '{')
            return TRUE;
        /* Or if line just has the closing paren - could be start of
           a function definition that continues on next line */
        int lastch = c_last_nonws_char(lp, len);
        if (lastch == ')')
            return TRUE;
    }
    return FALSE;
}

/*
 * Check if a line (after leading whitespace) starts with '{' or '}'.
 */
static int
c_line_starts_with_brace(LINE *lp)
{
    int offset = c_skip_ws(lp, 0);
    if (offset < llength(lp)) {
        Char c = lgetc(lp, offset);
        if (c == '{' || c == '}')
            return TRUE;
    }
    return FALSE;
}

/* === c_detect_style ==================================================== */

/*
 * Auto-detect the C brace style and indent settings from existing source.
 * Scans the first 200 lines of the current buffer.
 * Falls back to style 7 (none) with tab indent if detection is ambiguous.
 */
int
c_detect_style(int f, int n)
{
    (void)f;
    (void)n;

    int tab_count = 0, space_count = 0;
    int space2 = 0, space4 = 0, space8 = 0;
    int kr_count = 0, allman_count = 0, gnu_count = 0, whitesmiths_count = 0;
    int total_indented = 0;
    LINE *lp;
    int line_count = 0;

    lp = lforw(curbp->b_linep);
    while (lp != curbp->b_linep && line_count < 200) {
        line_count++;
        int len = llength(lp);
        int i = 0;

        /* Analyze leading whitespace */
        int has_tab = FALSE, has_space = FALSE, space_len = 0;
        while (i < len) {
            Char c = lgetc(lp, i);
            if (c == ' ') {
                has_space = TRUE;
                space_len++;
            } else if (c == '\t') {
                has_tab = TRUE;
                break;
            } else {
                break;
            }
            i++;
        }

        if (has_tab && !has_space) {
            tab_count++;
            total_indented++;
        } else if (has_space && !has_tab) {
            space_count++;
            total_indented++;
            if (space_len == 2) space2++;
            else if (space_len == 4) space4++;
            else if (space_len == 8) space8++;
            /* also check multiples */
            else if (space_len > 0 && space_len % 2 == 0 && space_len <= 8) space2++;
        } else if (has_tab && has_space) {
            tab_count++;
            total_indented++;
        }

        /* Analyze brace style */
        /* K&R: line ends with '{' */
        int lastch = c_last_nonws_char(lp, len);
        if (lastch == '{') {
            /* Check if the line also has a control statement */
            int offset = c_skip_ws(lp, 0);
            if (c_is_ctrl_keyword(lp, offset) || c_is_function_definition(lp)) {
                kr_count++;
            }
        }

        /* Allman/GNU/Whitesmiths: line starts with '{' */
        int brace_offset = c_skip_ws(lp, 0);
        if (brace_offset < len && lgetc(lp, brace_offset) == '{') {
            int brace_indent = c_line_indent_cols(lp);
            int prev_lp_indent = 0;
            LINE *plp = c_prev_nonblank(lp);
            if (plp != curbp->b_linep)
                prev_lp_indent = c_line_indent_cols(plp);

            if (brace_indent == prev_lp_indent + c_gnu_indent)
                gnu_count++;
            else if (brace_indent == prev_lp_indent + c_indent_width)
                whitesmiths_count++;
            else if (brace_indent == prev_lp_indent)
                allman_count++;
        }

        lp = lforw(lp);
    }

    /* Set indent unit */
    if (tab_count > 0 && space_count == 0) {
        c_use_spaces = FALSE;
    } else if (space_count > 0 && tab_count == 0) {
        c_use_spaces = TRUE;
        if (space4 >= space2 && space4 >= space8)
            c_indent_width = 4;
        else if (space2 >= space8)
            c_indent_width = 2;
        else
            c_indent_width = 8;
    } else if (tab_count > 0 && space_count > 0) {
        /* Google style: tabs for indent, spaces for alignment */
        c_use_spaces = FALSE;
    }
    /* else: no indented lines found - fall back */

    /* If we couldn't determine indent, default to tabs */
    if (total_indented == 0) {
        c_use_spaces = FALSE;
        c_indent_width = 8;
        c_brace_style = 7;
        mlwrite(CSTR(&Cxstr[8678]));
        return TRUE;
    }

    /* Set brace style */
    if (kr_count > 0 && allman_count == 0 && gnu_count == 0 && whitesmiths_count == 0) {
        c_brace_style = 0;  /* K&R */
    } else if (allman_count > 0 && kr_count == 0) {
        c_brace_style = 1;  /* Allman */
    } else if (gnu_count > 0 && kr_count == 0) {
        c_brace_style = 2;  /* GNU */
    } else if (whitesmiths_count > 0 && kr_count == 0) {
        c_brace_style = 4;  /* Whitesmiths */
    } else {
        /* Mixed styles or ambiguous - use none */
        c_brace_style = 7;
    }

    mlwrite(CSTR(&Cxstr[8730]),
        c_brace_style, c_indent_width,
        c_use_spaces ? CSTR(&Cxstr[8787]) : CSTR(&Cxstr[8791]), line_count);
    return TRUE;
}

/* === c_follow_source_cmd ================================================= */

/*
 * Toggle or set the follow-source mode.  When enabled, CMODE copies the
 * indentation of the previous line verbatim, ignoring c_indent_width
 * and c_use_spaces.  This lets the user match whatever the existing
 * source already uses.
 */
int
c_follow_source_cmd(int f, int n)
{
    (void)n;
    if (f)
        c_follow_source = (n > 0);
    else
        c_follow_source = !c_follow_source;
    mlwrite(c_follow_source ?
        CSTR(&Cxstr[8794]) :
        CSTR(&Cxstr[8816]));
    return TRUE;
}

/*========================================================================*/
/*  Public functions                                                     */

/*
 * cmode_insert() -- auto-indent on newline in CMODE.
 *
 * Called from newline() when MDCMOD is set and n == 1.  The cursor
 * is at position w_doto on line w_dotp.  The text before the cursor
 * is what was typed on this line.  After the newline, that text
 * becomes the previous line and the new (empty) line follows.
 *
 * Indentation for the new line is computed based on the content
 * before the cursor:
 *   - ends with '{'    * indent + c_indent_width
 *   - ends with '}'    * find matching { level, indent there
 *   - unclosed '(' '[' * continuation indent
 *   - preprocessor     * handle #if/#endif/#else/#elif
 *   - ctrl keyword     * if no '{' on line, indent +1
 *   - else/case/default * align with parent construct
 *   - default          * copy current line's indent
 */
int
cinsert(void)
{
    Char *cptr;
    int  tptr;
    int  cur_indent;
    int  target_indent;
    int  last_ch;
    int  i;
    int  len;

    cptr  = &curwp->w_dotp->l_text[0];
    tptr  = curwp->w_doto;
    len   = llength(curwp->w_dotp);

    /* --- compute current line's indentation --- */
    cur_indent = 0;
    for (i = 0; i < tptr; i++) {
        if (cptr[i] == ' ')
            ++cur_indent;
        else if (cptr[i] == '\t')
            cur_indent = (cur_indent / TS + 1) * TS;
        else
            break;
    }

    /* --- find last non-whitespace char before cursor --- */
    last_ch = c_last_nonws_char(curwp->w_dotp, tptr);

    /* --- follow-source: copy previous line's indent verbatim --- */
    if (c_follow_source) {
        LINE *prev = c_prev_nonblank(curwp->w_dotp);
        if (prev != curbp->b_linep)
            target_indent = c_line_indent_cols(prev);
    } else {

    /* --- Determine target indent --- */
    target_indent = cur_indent;

    switch (last_ch) {
    case '{':
        /* opening brace: next line is indented one level deeper */
        target_indent = cur_indent + c_indent_width;
        break;

    case '}':
        /* closing brace: new line aligns with the matching { */
        {
            int bal = 0;
            int j;

            /* scan backwards to find the matching { */
            for (j = tptr - 1; j >= 0; j--) {
                int skipped = c_skip_comment_or_string(curwp->w_dotp, j);
                if (skipped != j) {
                    j = skipped;
                    continue;
                }
                Char c = lgetc(curwp->w_dotp, j);
                if (c == '}')
                    bal++;
                else if (c == '{') {
                    if (bal == 0) {
                        /* found matching { on this line */
                        int indent_before = 0;
                        int k;
                        for (k = 0; k < j; k++) {
                            if (cptr[k] == ' ')
                                ++indent_before;
                            else if (cptr[k] == '\t')
                                indent_before = (indent_before / TS + 1) * TS;
                            else
                                break;
                        }
                        target_indent = indent_before;
                        break;
                    }
                    bal--;
                }
            }
            if (j < 0) {
                /* matching { is on a previous line -- scan backwards */
                target_indent = c_scan_for_brace_level(curwp->w_dotp);
            }
        }
        break;

    case ')':
    case ']':
    case ';':
    case '\0':
    default:
        /* Check for continuation: unclosed brackets */
        {
            int bal = 0;
            if (c_has_unclosed(curwp->w_dotp, 0, &bal)) {
                if (bal > 0)
                    target_indent = cur_indent + c_cont_indent;
            } else {
                /* Check if the line has a control-flow keyword
                   but no opening brace on the same line.
                   If so, indent the body */
                int offset = c_skip_ws(curwp->w_dotp, 0);
                if (c_is_ctrl_keyword(curwp->w_dotp, offset)) {
                    int has_brace = FALSE;
                    int j;
                    for (j = offset; j < tptr; j++) {
                        int skipped = c_skip_comment_or_string(curwp->w_dotp, j);
                        if (skipped != j) {
                            j = skipped - 1;
                            continue;
                        }
                        if (lgetc(curwp->w_dotp, j) == '{') {
                            has_brace = TRUE;
                            break;
                        }
                    }
                    if (!has_brace)
                        target_indent = cur_indent + c_indent_width;
                }
            }
        }
        break;
    }

    /* --- Style-specific adjustments --- */
    /* none (style 7): respect user's brace placement unconditionally */
    if (c_brace_style == 7) {
        if (last_ch == '}')
            target_indent = c_scan_for_brace_level(curwp->w_dotp);
        /* { at end of line: body already +c_indent_width (respect user) */
        /* { on own line: body already +c_indent_width from { line (respect user) */
        /* no adjustment needed: all cases respect user placement */
    }
    /* GNU (style 2): half-indent for { and } on own lines */
    else if (c_brace_style == 2) {
        if (last_ch == '}') {
            int scan_indent = c_scan_for_brace_level(curwp->w_dotp);
            /* GNU: } aligns with {, which is at control + c_gnu_indent */
            if (c_line_starts_with_brace(curwp->w_dotp)) {
                int prev_lp = c_line_indent_cols(c_prev_nonblank(curwp->w_dotp));
                if (prev_lp >= 0)
                    target_indent = prev_lp + c_gnu_indent;
            }
        }
        if (last_ch == '{' && c_line_starts_with_brace(curwp->w_dotp)) {
            /* { on own line: body indent = { line indent + c_indent_width
               But GNU body is at control + c_gnu_indent + c_indent_width,
               which equals { indent + c_indent_width (already correct) */
        }
    }
    /* Whitesmiths (style 4): { and } on own lines at body level */
    else if (c_brace_style == 4) {
        if (last_ch == '}') {
            int scan_indent = c_scan_for_brace_level(curwp->w_dotp);
            /* Whitesmiths: } at { indent = control + c_indent_width */
            if (c_line_starts_with_brace(curwp->w_dotp)) {
                int prev_lp = c_line_indent_cols(c_prev_nonblank(curwp->w_dotp));
                if (prev_lp >= 0)
                    target_indent = prev_lp + c_indent_width;
            }
        }
    }
    /* Stroustrup (style 6): function { on own line if c_brace_func */
    else if (c_brace_style == 6) {
        if (last_ch == '}') {
            /* Stroustrup: } aligns with matching { placement
               (already handled by c_scan_for_brace_level) */
        }
    }

    /* --- Handle preprocessor directives --- */
    if (c_starts_pp(curwp->w_dotp, "if") ||
        c_starts_pp(curwp->w_dotp, "ifdef") ||
        c_starts_pp(curwp->w_dotp, "ifndef")) {
        target_indent = cur_indent + c_pp_indent + c_indent_width;
    } else if (c_starts_pp(curwp->w_dotp, "else") ||
               c_starts_pp(curwp->w_dotp, "elif")) {
        /* #else/#elif align with the opening #if */
        target_indent = c_scan_for_brace_level(curwp->w_dotp);
    } else if (c_starts_pp(curwp->w_dotp, "endif")) {
        target_indent = c_scan_for_brace_level(curwp->w_dotp);
    }

    /* --- Handle do-while: } while(x); aligns with do --- */
    {
        int offset = c_skip_ws(curwp->w_dotp, 0);
        if (c_is_keyword(curwp->w_dotp, offset, "while")) {
            /* Check if matching 'do' on a previous line */
            /* For simplicity, keep at the brace level */
            if (last_ch == ';' && tptr > 0) {
                /* line ends with ';' after while -- likely do-while */
                /* scan backwards to find 'do' */
                LINE *dlp = c_prev_nonblank(curwp->w_dotp);
                if (dlp != curbp->b_linep) {
                    int doff = c_skip_ws(dlp, 0);
                    if (c_is_keyword(dlp, doff, "do"))
                        target_indent = c_line_indent_cols(dlp);
                }
            }
        }
    }

    }  /* end c_follow_source else block */

    /* --- Insert the newline --- */
    if (lnewline() == FALSE)
        return FALSE;

    /* --- Insert computed indentation --- */
    if (target_indent < 0)
        target_indent = 0;

    if (c_insert_indent(target_indent) == FALSE)
        return FALSE;

    lchange(WFEDIT);
    return TRUE;
}

/*
 * cmode_indent() -- re-indent the current line (bound to M-J).
 *
 * Deletes the leading whitespace on the current line and re-inserts
 * the correct indentation computed from the context of previous lines.
 */
/*ARGSUSED*/
int
cindent(int f, int n)
{
    LINE *prev;
    int  prev_indent;
    int  target_indent;
    int  offset;
    int  i;
    int  ts = TS;

    if (curbp->b_mode & MDVIEW)
        return rdonly();
    if (n < 0)
        return FALSE;
    if (n == 0)
        n = 1;

    for (i = 0; i < n; i++) {
        /* --- delete current leading whitespace --- */
        c_delete_indent();

        /* --- find the previous non-blank line --- */
        prev = c_prev_nonblank(curwp->w_dotp);

        if (prev == curbp->b_linep) {
            /* no previous line -- column 0 */
            target_indent = 0;
        } else {
            prev_indent = c_line_indent_cols(prev);
            target_indent = prev_indent;

            /* Check the last non-whitespace char on the previous line */
            int last_ch = c_last_nonws_char(prev, llength(prev));

            switch (last_ch) {
            case '{':
                target_indent = prev_indent + c_indent_width;
                break;

            case '}':
                /* find matching brace level */
                target_indent = c_scan_for_brace_level(curwp->w_dotp);
                break;

            case ';':
                target_indent = prev_indent;
                break;

            default:
                /* Check for unclosed brackets on previous line */
                {
                    int bal = 0;
                    if (c_has_unclosed(prev, 0, &bal) && bal > 0) {
                        /* previous line has unclosed ( or [ -- continue */
                        target_indent = prev_indent + c_cont_indent;
                    } else {
                        /* Check if previous line is a control-flow keyword */
                        int off = c_skip_ws(prev, 0);
                        if (c_is_ctrl_keyword(prev, off)) {
                            int has_brace = FALSE;
                            int j;
                            for (j = off; j < llength(prev); j++) {
                                int skipped = c_skip_comment_or_string(prev, j);
                                if (skipped != j) {
                                    j = skipped - 1;
                                    continue;
                                }
                                if (lgetc(prev, j) == '{') {
                                    has_brace = TRUE;
                                    break;
                                }
                            }
                            if (!has_brace)
                                target_indent = prev_indent + c_indent_width;
                            else if (last_ch == '{')
                                target_indent = prev_indent + c_indent_width;
                        } else if (last_ch == '{' && c_is_keyword(prev, off, "else")) {
                            /* else { -- indent +1 */
                            target_indent = prev_indent + c_indent_width;
                        }
                    }
                }
                break;
            }

            /* --- Handle control-flow keyword on current line --- */
            offset = c_skip_ws(curwp->w_dotp, 0);

            /* Dedent keywords: align with parent construct */
            if (c_is_dedent_keyword(curwp->w_dotp, offset)) {
                int scan_indent = c_scan_for_brace_level(curwp->w_dotp);
                if (c_is_keyword(curwp->w_dotp, offset, "else") ||
                    c_is_keyword(curwp->w_dotp, offset, "do")) {
                    target_indent = scan_indent;
                } else {
                    /* case/default: switch level + c_case_indent */
                    target_indent = scan_indent + c_case_indent;
                }
            }

            /* --- Style-specific adjustments --- */
            if (c_brace_style == 7) {
                /* none: use c_scan_for_matching_if for else alignment */
                if (c_is_keyword(curwp->w_dotp, offset, "else"))
                    target_indent = c_scan_for_matching_if(curwp->w_dotp);
                /* for }, } already uses c_scan_for_brace_level (correct) */
                /* for { and other lines: respect user placement */
            } else if (c_brace_style == 4) {
                /* Whitesmiths: } at { level (body level), not control level */
                if (c_is_dedent_keyword(curwp->w_dotp, offset) &&
                    c_is_keyword(curwp->w_dotp, offset, "case") == FALSE &&
                    c_is_keyword(curwp->w_dotp, offset, "default") == FALSE) {
                    int scan_indent = c_scan_for_brace_level(curwp->w_dotp);
                    /* For Whitesmiths, } should be at { indent, not control */
                    target_indent = scan_indent;
                }
            } else if (c_brace_style == 2) {
                /* GNU: } at { level which is at control + c_gnu_indent */
                if (c_is_dedent_keyword(curwp->w_dotp, offset) &&
                    c_is_keyword(curwp->w_dotp, offset, "case") == FALSE &&
                    c_is_keyword(curwp->w_dotp, offset, "default") == FALSE) {
                    /* scan_indent already gives { line indent for GNU */
                    /* which is control + c_gnu_indent -- correct */
                }
            }

            /* Preprocessor directives */
            if (c_starts_pp(curwp->w_dotp, "else") ||
                c_starts_pp(curwp->w_dotp, "elif") ||
                c_starts_pp(curwp->w_dotp, "endif")) {
                target_indent = c_scan_for_brace_level(curwp->w_dotp);
            }

            /* Handle preprocessor: previous line is #if/#ifdef/#ifndef */
            if (c_starts_pp(prev, "if") ||
                c_starts_pp(prev, "ifdef") ||
                c_starts_pp(prev, "ifndef")) {
                target_indent = prev_indent + c_pp_indent + c_indent_width;
            }

            /* do-while: } while(x); line */
            {
                int off2 = c_skip_ws(curwp->w_dotp, 0);
                if (c_is_keyword(curwp->w_dotp, off2, "while")) {
                    LINE *dlp = c_prev_nonblank(curwp->w_dotp);
                    if (dlp != curbp->b_linep) {
                        int doff = c_skip_ws(dlp, 0);
                        if (c_is_keyword(dlp, doff, "do")) {
                            target_indent = c_line_indent_cols(dlp);
                        }
                    }
                }
            }
        }

        if (target_indent < 0)
            target_indent = 0;

        /* --- insert computed indentation --- */
        if (c_insert_indent(target_indent) == FALSE) {
            if (i < n - 1)
                (void)forwline(TRUE, 1);
            lchange(WFEDIT);
            return FALSE;
        }

        /* move down for repeat (if n > 1) */
        if (i < n - 1)
            (void)forwline(TRUE, 1);
    }

    lchange(WFEDIT);
    return TRUE;
}

/*
 * cmode_indent_region() -- re-indent a selected region.
 *
 * Uses a forward scan with an indent stack to track context:
 *   - Each '{' pushes a brace level
 *   - Each '}' pops a brace level
 *   - #if/#ifdef pushes a preprocessor level
 *   - #endif pops a preprocessor level
 *   - case/default aligned relative to switch
 *   - Control-flow keywords without braces indent the next line
 */
/*ARGSUSED*/
int
cindent_region(int f, int n)
{
    REGION region;
    LINE  *lp;
    LINE  *last_lp;
    int   base_indent;
    int   indent_stack[C_MAX_DEPTH];
    int   sp = 0;
    int   pp_stack[C_MAX_DEPTH];
    int   pp_sp = 0;
    int   remaining;
    int   offset;
    int   line_len;
    int   rc;
    int   prev_brace_open;   /* did prev line end with unclosed { ? */
    int   prev_has_ctrl;     /* did prev line have a ctrl keyword (no brace)? */
    int   prev_last_ch;      /* last non-ws char of prev line */
    int   i;
    int   ts = TS;

    if (curbp->b_mode & MDVIEW)
        return rdonly();

    rc = getregion(&region);
    if (rc != TRUE)
        return rc;

    /* --- find the last line of the region --- */
    last_lp  = region.r_linep;
    offset   = region.r_offset;
    remaining = (int)region.r_size;

    while (remaining > 0 && last_lp != curbp->b_linep) {
        line_len = llength(last_lp);
        if (offset < line_len)
            remaining -= (line_len - offset) + 1;  /* content + newline */
        else
            remaining -= 1;                         /* just newline */
        if (remaining <= 0)
            break;
        last_lp = lforw(last_lp);
        offset = 0;
        if (last_lp == curbp->b_linep)
            break;
    }

    /* --- determine base indent from the line before the region --- */
    lp = lback(region.r_linep);
    if (lp == curbp->b_linep)
        base_indent = 0;
    else
        base_indent = c_line_indent_cols(lp);

    /* --- initialize indent stack --- */
    indent_stack[0] = base_indent;
    sp = 0;
    pp_stack[0] = 0;
    pp_sp = 0;

    /* --- save cursor position --- */
    LINE *save_dotp = curwp->w_dotp;

    prev_brace_open = FALSE;
    prev_has_ctrl = FALSE;
    prev_last_ch = '\0';

    /* --- walk through each line of the region --- */
    lp = region.r_linep;
    while (1) {
        int col;
        int line_indent;

        /* skip blank lines -- leave them untouched */
        if (c_is_blank_line(lp)) {
            prev_brace_open = FALSE;
            prev_has_ctrl = FALSE;
            prev_last_ch = '\0';
            if (lp == last_lp)
                break;
            lp = lforw(lp);
            if (lp == curbp->b_linep)
                break;
            continue;
        }

        line_indent = c_line_indent_cols(lp);

        /* --- pop indent stack if needed --- */
        {
            int skip_pop = FALSE;

            /* don't pop if we're inside an unclosed bracket context
               from the previous line */
            if (prev_brace_open || prev_has_ctrl)
                skip_pop = TRUE;

            if (!skip_pop && pp_sp > 0) {
                /* pop preprocessor levels based on #endif */
                int bal = 0;
                int j;
                for (j = 0; j < llength(lp); j++) {
                    int skipped = c_skip_comment_or_string(lp, j);
                    if (skipped != j) {
                        j = skipped - 1;
                        continue;
                    }
                    Char c = lgetc(lp, j);
                    if (c == '#') {
                        if (c_starts_pp(lp, "endif"))
                            pp_sp--;
                        else if (c_starts_pp(lp, "if") ||
                                 c_starts_pp(lp, "ifdef") ||
                                 c_starts_pp(lp, "ifndef"))
                            pp_sp++;
                    }
                }
                while (pp_sp < 0) pp_sp = 0;
            }

            /* pop brace stack based on current indent vs stack */
            if (!skip_pop) {
                while (sp > 0 && line_indent < indent_stack[sp]) {
                    sp--;
                }
            }
        }

        col = indent_stack[sp] + pp_sp * c_pp_indent;

        /* --- adjust for keywords on this line --- */
        offset = c_skip_ws(lp, 0);

        /* Dedent keywords: case, default */
        if (c_is_keyword(lp, offset, "case") ||
            c_is_keyword(lp, offset, "default")) {
            col = indent_stack[sp];
            col += c_case_indent;
        }

        /* else: align with matching if */
        if (c_is_keyword(lp, offset, "else"))
            col = c_scan_for_brace_level(lp);

        /* do: at same level as its while */
        /* (no special handling needed here) */

        /* --- Style-specific adjustments for standalone braces --- */
        if (c_brace_style == 7) {
            /* none: for standalone }, compute indent from matching { */
            if (c_line_starts_with_brace(lp)) {
                int lastch = c_last_nonws_char(lp, llength(lp));
                if (lastch == '}')
                    col = c_scan_for_brace_level(lp);
                /* For standalone {, body should be { indent + c_indent_width */
                /* This is already handled by the push logic below */
            }
        } else if (c_brace_style == 2) {
            /* GNU: standalone } at control + c_gnu_indent */
            if (c_line_starts_with_brace(lp)) {
                int lastch = c_last_nonws_char(lp, llength(lp));
                if (lastch == '}') {
                    col = c_scan_for_brace_level(lp);
                    /* c_scan_for_brace_level returns { line's indent
                       which is control + c_gnu_indent for GNU */
                }
            }
        } else if (c_brace_style == 4) {
            /* Whitesmiths: standalone } at { level */
            if (c_line_starts_with_brace(lp)) {
                int lastch = c_last_nonws_char(lp, llength(lp));
                if (lastch == '}')
                    col = c_scan_for_brace_level(lp);
            }
        }

        /* --- re-indent this line --- */
        curwp->w_dotp = lp;
        curwp->w_doto = 0;
        c_delete_indent();
        if (c_insert_indent(col) == FALSE) {
            curwp->w_dotp = save_dotp;
            curwp->w_doto = 0;
            lchange(WFEDIT);
            return FALSE;
        }

        /* --- compute context for next line --- */
        prev_last_ch = c_last_nonws_char(lp, llength(lp));
        prev_brace_open = FALSE;
        prev_has_ctrl = FALSE;

        /* Check for unclosed brackets on this line */
        {
            int bal = 0;
            if (c_has_unclosed(lp, 0, &bal) && bal > 0)
                prev_brace_open = TRUE;
        }

        /* If line ends with '{', push indent level */
        if (prev_last_ch == '{') {
            if (sp < C_MAX_DEPTH - 1) {
                sp++;
                indent_stack[sp] = col + c_indent_width;
            }
        }

        /* If line has a control keyword without brace, next line is +1 */
        {
            int off2 = c_skip_ws(lp, 0);
            int has_brace = FALSE;
            int j;
            if (c_is_ctrl_keyword(lp, off2)) {
                for (j = off2; j < llength(lp); j++) {
                    int skipped = c_skip_comment_or_string(lp, j);
                    if (skipped != j) {
                        j = skipped - 1;
                        continue;
                    }
                    if (lgetc(lp, j) == '{') {
                        has_brace = TRUE;
                        break;
                    }
                }
                if (!has_brace && prev_last_ch != '{')
                    prev_has_ctrl = TRUE;
            }
        }

        /* Handle preprocessor directives */
        {
            if (c_starts_pp(lp, "if") ||
                c_starts_pp(lp, "ifdef") ||
                c_starts_pp(lp, "ifndef")) {
                if (pp_sp < C_MAX_DEPTH - 1) {
                    pp_sp++;
                    pp_stack[pp_sp] = col;
                }
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
 * c_mode() -- toggle CMODE on/off for the current buffer.
 *
 * With no argument (f == FALSE), toggles the MDCMOD bit.
 * With a positive argument, turns CMODE on.
 * With a negative argument, turns CMODE off.
 *
 * Registered as "c-mode" in efunc.H_.
 */
/*ARGSUSED*/
int
c_mode(int f, int n)
{
    if (curbp->b_mode & MDVIEW)
        return rdonly();

    if (f) {
        if (n > 0)
            curbp->b_mode |= MDCMOD;
        else
            curbp->b_mode &= ~MDCMOD;
    } else
        curbp->b_mode ^= MDCMOD;

    upmode();
    curwp->w_flag |= WFMODE;
    mlwrite(CSTR(&Cxstr[8839]),
        (curbp->b_mode & MDCMOD) ? CSTR(&Cxstr[7198]) : CSTR(&Cxstr[7214]));
    return TRUE;
}
