/*
 * test_python.c -- Unit tests for kemacs Python mode (PYMODE) indentation.
 *
 * Tests cover: pyinsert, pyindent, py_backspace, py_indent_region
 *
 * line.c and python.c are included directly after defining maindef
 * (to provide global-variable definitions from edef.h) and static
 * (to expose internal helper functions).  External dependencies
 * are satisfied by minimal stubs defined below.
 *
 * Linking: ../Css.o (for Cxstr, used in mlwrite() calls via lchange)
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>

/* ---- K&R-style forward declarations for stubs (before includes) ---- */
int mlwrite();
int rdonly();
int upmode();
int backchar();
int forwdel();
int forwline();
int getregion();

/* ================================================================ */
/*  Include kemacs source files with maindef + static              */
/* ================================================================ */
#define maindef
#define static
#include "../ecomm.h"
#include "../estruct.h"
#include "../edef.h"
#include "../line.c"
#include "../python.c"
#undef static
#undef maindef

/* ================================================================ */
/*  Stub implementations                                           */
/* ================================================================ */

int tabsize = 0;        /* defined in random.c, needed by python.c */

int mlwrite()        { return TRUE; }
int rdonly()         { return FALSE; }
int upmode()         { return TRUE; }
int forwdel()        { return TRUE; }

/* backchar -- move cursor back n chars (simplified, single-buffer) */
int backchar(int f, int n)
{
    LINE *lp;
    (void)f;
    while (n--) {
        if (curwp->w_doto == 0) {
            lp = lback(curwp->w_dotp);
            if (lp == curbp->b_linep)
                return FALSE;
            curwp->w_dotp  = lp;
            curwp->w_doto  = llength(lp);
        } else {
            curwp->w_doto--;
        }
    }
    return TRUE;
}

/* forwline -- move cursor down n lines (simplified) */
int forwline(int f, int n)
{
    LINE *dlp;
    (void)f;
    if (n < 0)
        return FALSE;
    dlp = curwp->w_dotp;
    while (n-- && dlp != curbp->b_linep)
        dlp = lforw(dlp);
    curwp->w_dotp  = dlp;
    curwp->w_doto  = 0;
    return TRUE;
}

/* Undo stubs (called from line.c but not under test here) */
void u_record(int type, LINE *lp, int offset, int length,
              Char *data, int datalen, int eob) { (void)type; }
void u_clear_undo(BUFFER *bp) { (void)bp; }
void u_prune(BUFFER *bp) { (void)bp; }

/*
 * getregion -- compute the region between dot and mark.
 * Mirrors the implementation in region.c without CSTR/mlwrite calls.
 */
int getregion(REGION *rp)
{
    LINE  *flp;
    LINE  *blp;
    long   fsize;
    long   bsize;

    if (curwp->w_markp == NULL)
        return FALSE;
    if (curwp->w_dotp == curwp->w_markp) {
        rp->r_linep = curwp->w_dotp;
        if (curwp->w_doto < curwp->w_marko) {
            rp->r_offset = curwp->w_doto;
            rp->r_size   = (long)(curwp->w_marko - curwp->w_doto);
        } else {
            rp->r_offset = curwp->w_marko;
            rp->r_size   = (long)(curwp->w_doto - curwp->w_marko);
        }
        return TRUE;
    }
    blp  = curwp->w_dotp;
    bsize = (long)curwp->w_doto;
    flp  = curwp->w_dotp;
    fsize = (long)(llength(flp) - curwp->w_doto + 1);
    while (flp != curbp->b_linep || lback(blp) != curbp->b_linep) {
        if (flp != curbp->b_linep) {
            flp = lforw(flp);
            if (flp == curwp->w_markp) {
                rp->r_linep  = curwp->w_dotp;
                rp->r_offset = curwp->w_doto;
                rp->r_size   = fsize + curwp->w_marko;
                return TRUE;
            }
            fsize += llength(flp) + 1;
        }
        if (lback(blp) != curbp->b_linep) {
            blp = lback(blp);
            bsize += llength(blp) + 1;
            if (blp == curwp->w_markp) {
                rp->r_linep  = blp;
                rp->r_offset = curwp->w_marko;
                rp->r_size   = bsize - curwp->w_marko;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* ================================================================ */
/*  Test fixtures                                                  */
/* ================================================================ */

static LINE *g_hdr = NULL;   /* buffer header (circular sentinel) */

static LINE *alloc_test_line(int capacity)
{
    LINE *lp;
    int size = (capacity + NBLOCK - 1) & ~(NBLOCK - 1);
    if (size == 0)
        size = NBLOCK;
    lp = (LINE *)malloc(sizeof(LINE) + sizeof(Char) * size);
    if (lp == NULL)
        return NULL;
    lp->l_size = size;
    lp->l_used = 0;
    return lp;
}

static LINE *line_from_text(const char *text)
{
    int i, len = strlen(text);
    LINE *lp = alloc_test_line(len > 0 ? len + 1 : 1);
    if (lp == NULL)
        return NULL;
    for (i = 0; i < len; i++)
        lp->l_text[i] = (unsigned char)text[i];
    lp->l_used = len;
    return lp;
}

static void cleanup_test_lines(void)
{
    if (g_hdr == NULL)
        return;
    LINE *cur = g_hdr->l_fp;
    while (cur != g_hdr && cur != NULL) {
        LINE *next = cur->l_fp;
        free((char *)cur);
        cur = next;
    }
    free((char *)g_hdr);
    g_hdr = NULL;
}

static void reset_python_env(void)
{
    py_indent_width = 4;
    py_use_spaces = TRUE;
    tabsize = 0;
    lastflag = 0;
    thisflag = 0;
}

/*
 * Set up a single-line buffer with content.
 * After: curbp->b_linep = g_hdr, curwp->w_dotp = text_line,
 *        curwp->w_doto = doto, curbp->b_mode = mode
 */
static void setup_single(const char *text, int doto, int mode)
{
    int i, len = strlen(text);
    LINE *text_line;

    cleanup_test_lines();
    g_hdr = alloc_test_line(0);
    g_hdr->l_fp = g_hdr;
    g_hdr->l_bp = g_hdr;

    text_line = line_from_text(text);
    g_hdr->l_fp = text_line;
    g_hdr->l_bp = text_line;
    text_line->l_fp = g_hdr;
    text_line->l_bp = g_hdr;

    if (curbp == NULL) {
        curbp = (BUFFER *)calloc(1, sizeof(BUFFER) + sizeof(Char)*NSTRING);
    }
    if (curwp == NULL) {
        curwp = (WINDOW *)calloc(1, sizeof(WINDOW));
    }
    if (bheadp == NULL) bheadp = curbp;
    if (wheadp == NULL) wheadp = curwp;

    curbp->b_linep = g_hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = mode;
    curbp->b_flag  = 0;
    curbp->b_bufp  = NULL;
    curbp->b_dotp  = text_line;
    curbp->b_doto  = 0;

    curwp->w_bufp  = curbp;
    curwp->w_linep = text_line;
    curwp->w_dotp  = text_line;
    curwp->w_doto  = doto;
    curwp->w_flag  = 0;
    curwp->w_wndp  = NULL;
    curwp->w_markp = NULL;
    curwp->w_marko = 0;

    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;

    reset_python_env();
}

/*
 * Set up a multi-line buffer from an array of strings.
 * The cursor is placed on line dot_line, at offset dot_offset.
 * If mark_line >= 0, set mark on that line at mark_offset.
 */
static void setup_multiline(char **lines, int nlines,
                            int dot_line, int dot_offset,
                            int mark_line, int mark_offset,
                            int mode)
{
    int i, len;
    LINE *lp, *prev;

    cleanup_test_lines();
    g_hdr = alloc_test_line(0);
    g_hdr->l_fp = g_hdr;
    g_hdr->l_bp = g_hdr;

    prev = g_hdr;
    for (i = 0; i < nlines; i++) {
        lp = line_from_text(lines[i]);
        prev->l_fp = lp;
        lp->l_bp = prev;
        lp->l_fp = g_hdr;
        g_hdr->l_bp = lp;
        prev = lp;
    }

    if (curbp == NULL) {
        curbp = (BUFFER *)calloc(1, sizeof(BUFFER) + sizeof(Char)*NSTRING);
    }
    if (curwp == NULL) {
        curwp = (WINDOW *)calloc(1, sizeof(WINDOW));
    }
    if (bheadp == NULL) bheadp = curbp;
    if (wheadp == NULL) wheadp = curwp;

    curbp->b_linep = g_hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = mode;
    curbp->b_flag  = 0;
    curbp->b_bufp  = NULL;

    /* Set cursor (dot) */
    lp = lforw(g_hdr);
    for (i = 0; i < dot_line && lforw(lp) != g_hdr; i++)
        lp = lforw(lp);
    curwp->w_dotp  = lp;
    curwp->w_doto  = dot_offset;
    curwp->w_linep = lp;
    curwp->w_flag  = 0;
    curwp->w_wndp  = NULL;

    /* Set mark */
    if (mark_line >= 0) {
        lp = lforw(g_hdr);
        for (i = 0; i < mark_line && lforw(lp) != g_hdr; i++)
            lp = lforw(lp);
        curwp->w_markp = lp;
        curwp->w_marko = mark_offset;
    } else {
        curwp->w_markp = NULL;
        curwp->w_marko = 0;
    }

    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;

    reset_python_env();
}

/* Helper: copy a LINE's text into a C string buffer */
static int line_text_to_cstr(LINE *lp, char *buf, int bufsize)
{
    int i, len = lp->l_used;
    if (len > bufsize - 1)
        len = bufsize - 1;
    for (i = 0; i < len; i++)
        buf[i] = (char)lp->l_text[i];
    buf[len] = '\0';
    return len;
}

/* Helper: copy current line text into buf */
static int cur_text_to_cstr(char *buf, int bufsize)
{
    return line_text_to_cstr(curwp->w_dotp, buf, bufsize);
}

/* ================================================================ */
/*  pyinsert() tests                                                */
/* ================================================================ */

static void test_py_insert_after_colon(void)
{
    char buf[64];

    /* "if True:" cursor at end -> new line gets 4 spaces */
    setup_single("if True:", 8, MDPYMOD);
    pyinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 4, "pyinsert: new line has 4 spaces");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    ", "pyinsert: new line is 4 spaces");
}

static void test_py_insert_no_colon(void)
{
    char buf[64];

    /* "x = 1" cursor at end -> new line gets 0 indent */
    setup_single("x = 1", 5, MDPYMOD);
    pyinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 0, "pyinsert: no colon -> 0 indent");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "pyinsert: new line is empty");
}

static void test_py_insert_inherited_indent(void)
{
    char buf[64];

    /* "    x = 1" (4 spaces) cursor at END -> new line inherits 4 spaces */
    setup_single("    x = 1", 9, MDPYMOD);
    pyinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 4, "pyinsert: inherited indent = 4");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    ", "pyinsert: new line has 4 spaces");
}

static void test_py_insert_nested_colon(void)
{
    char buf[64];

    /* "    if True:" (4 spaces + colon) -> new line gets 8 spaces */
    setup_single("    if True:", 12, MDPYMOD);
    pyinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 8, "pyinsert: nested colon -> 8 indent");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "        ", "pyinsert: new line has 8 spaces");
}

/* ================================================================ */
/*  py_backspace() tests                                            */
/* ================================================================ */

static void test_py_backspace_dedent(void)
{
    /* "    x = 1" cursor at offset 4 (after 4 spaces) -> delete 4 */
    setup_single("    x = 1", 4, MDPYMOD);
    py_backspace(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "py_backspace: 4 spaces removed (1234x = 1 -> x = 1)");
    TEST_ASSERT_INT(curwp->w_doto, 0, "py_backspace: cursor at 0 after dedent");
}

static void test_py_backspace_partial_dedent(void)
{
    /* "      x" (6 spaces) cursor at offset 6 -> delete 4 (one indent width) */
    setup_single("      x", 6, MDPYMOD);
    py_backspace(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 3, "py_backspace: partial -> 4 removed, 2 spaces remain");
    TEST_ASSERT_INT(curwp->w_doto, 2, "py_backspace: cursor at 2 after partial");
}

static void test_py_backspace_not_whitespace(void)
{
    /* "    x" cursor at offset 5 (after 'x') -> normal backspace */
    setup_single("    x", 5, MDPYMOD);
    py_backspace(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 4, "py_backspace: normal del removes 1 char");
    TEST_ASSERT_INT(curwp->w_doto, 4, "py_backspace: cursor at 4 after normal del");
}

static void test_py_backspace_tab_mode(void)
{
    /* "\tx = 1" cursor at offset 1 (after tab), tab mode -> delete 1 char */
    setup_single("\tx = 1", 1, MDPYMOD);
    py_use_spaces = FALSE;
    py_backspace(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "py_backspace: tab mode deletes 1 char");
    TEST_ASSERT_INT(curwp->w_doto, 0, "py_backspace: cursor at 0 after tab delete");
}

static void test_py_backspace_at_col_zero(void)
{
    /* "x = 1" cursor at offset 0 -> backchar fails (no prev line) */
    setup_single("x = 1", 0, MDPYMOD);
    int rc = py_backspace(FALSE, 1);
    TEST_ASSERT_INT(rc, FALSE, "py_backspace: col 0 -> backchar fails, returns FALSE");
}

static void test_py_backspace_no_pymode(void)
{
    /* Without MDPYMOD, should do normal backspace */
    setup_single("x = 1", 1, 0);  /* mode = 0, no MDPYMOD */
    py_backspace(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 4, "py_backspace: no PYMOD -> normal del");
    TEST_ASSERT_INT(curwp->w_doto, 0, "py_backspace: cursor at 0 in normal mode");
}

/* ================================================================ */
/*  pyindent() tests                                                */
/* ================================================================ */

static void test_py_indent_with_colon(void)
{
    char buf[64];

    /* Line 0: "if True:" (ends with colon)
       Line 1: "x = 1" -> should become "    x = 1" (4 spaces)
    */
    char *lines[] = {"if True:", "x = 1"};
    setup_multiline(lines, 2, 1, 5, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x = 1", "pyindent: indent after colon = 4");
}

static void test_py_indent_no_colon(void)
{
    char buf[64];

    /* Line 0: "x = 1" (no colon, indent 0)
       Line 1: "y = 2" -> stays "y = 2" (indent 0)
    */
    char *lines[] = {"x = 1", "y = 2"};
    setup_multiline(lines, 2, 1, 5, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "y = 2", "pyindent: no colon -> same indent");
}

static void test_py_indent_dedent_keyword(void)
{
    char buf[64];

    /* Line 0: "if True:" (indent 0, colon)
       Line 1: "    x = 1" (indent 4)
       Line 2: "    else:" -> should become "else:" (dedent to 0)
    */
    char *lines[] = {"if True:", "    x = 1", "    else:"};
    setup_multiline(lines, 3, 2, 9, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "else:", "pyindent: dedent keyword -> 0");
}

static void test_py_indent_skip_blank_prev(void)
{
    char buf[64];

    /* Line 0: "if True:" (ends with colon)
       Line 1: "" (blank)
       Line 2: "y = 2" -> should become "    y = 2" (from non-blank prev)
    */
    char *lines[] = {"if True:", "", "y = 2"};
    setup_multiline(lines, 3, 2, 5, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    y = 2", "pyindent: skip blank prev, indent after colon");
}

static void test_py_indent_nested_colon(void)
{
    char buf[64];

    /* Line 0: "if True:" (colon, indent 0)
       Line 1: "if nested:" -> should become "    if nested:" (4 spaces from colon)
    */
    char *lines[] = {"if True:", "if nested:"};
    setup_multiline(lines, 2, 1, 10, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    if nested:", "pyindent: nested colon -> indent 4");
}

static void test_py_indent_already_correct(void)
{
    char buf[64];

    /* Line 0: "if True:" (colon, indent 0)
       Line 1: "    x = 1" -> already correct, stays the same
    */
    char *lines[] = {"if True:", "    x = 1"};
    setup_multiline(lines, 2, 1, 0, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x = 1", "pyindent: already correct -> unchanged");
}

static void test_py_indent_no_previous_line(void)
{
    char buf[64];

    /* Line 0 is the only line (first line of buffer)
       Cursor on line 0 -> no previous line -> indent 0
    */
    char *lines[] = {"    x = 1"};
    setup_multiline(lines, 1, 0, 0, -1, 0, MDPYMOD);
    pyindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "x = 1", "pyindent: first line -> indent 0");
}

/* ================================================================ */
/*  py_indent_region() tests                                        */
/* ================================================================ */

static void test_py_indent_region_simple(void)
{
    /* Region: lines 1-4
       Line 0: "" (before region, base indent 0)
       Line 1: "if True:" -> col=0, push 4
       Line 2: "    x = 1" -> col=4
       Line 3: "      y = 2" -> col=4 (FIX: 6->4)
       Line 4: "    z = 3" -> col=4
    */
    char *lines[] = {"", "if True:", "    x = 1", "      y = 2", "    z = 3"};
    char buf[64];
    LINE *lp;

    setup_multiline(lines, 5, 4, 9, 1, 0, MDPYMOD);
    py_indent_region(FALSE, 1);

    lp = lforw(g_hdr);  /* line 0: "" */
    TEST_ASSERT_INT(llength(lp), 0, "region: line 0 unchanged");

    lp = lforw(lp);  /* line 1: "if True:" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "if True:", "region: line 1 unchanged");

    lp = lforw(lp);  /* line 2: "    x = 1" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x = 1", "region: line 2 unchanged");

    lp = lforw(lp);  /* line 3: "    y = 2" (fixed from 6 spaces) */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    y = 2", "region: line 3 fixed 6->4");

    lp = lforw(lp);  /* line 4: "    z = 3" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    z = 3", "region: line 4 unchanged");
}

static void test_py_indent_region_nested(void)
{
    /* Region: lines 1-5
       Line 0: "" (before region, base indent 0)
       Line 1: "if True:" -> col=0, push 4
       Line 2: "    x = 1" -> col=4
       Line 3: "      if nested:" -> col=4 (FIX: 6->4), push 8
       Line 4: "        y = 2" -> col=8
       Line 5: "    z = 3" -> col=4
    */
    char *lines[] = {"", "if True:", "    x = 1", "      if nested:", "        y = 2", "    z = 3"};
    char buf[64];
    LINE *lp;

    setup_multiline(lines, 6, 5, 9, 1, 0, MDPYMOD);
    py_indent_region(FALSE, 1);

    lp = lforw(g_hdr);  /* line 0: "" */
    TEST_ASSERT_INT(llength(lp), 0, "nested region: line 0 unchanged");

    lp = lforw(lp);  /* line 1: "if True:" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "if True:", "nested region: line 1 unchanged");

    lp = lforw(lp);  /* line 2: "    x = 1" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x = 1", "nested region: line 2 unchanged");

    lp = lforw(lp);  /* line 3: "    if nested:" (fixed from 6 spaces) */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    if nested:", "nested region: line 3 fixed 6->4");

    lp = lforw(lp);  /* line 4: "        y = 2" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "        y = 2", "nested region: line 4 unchanged");

    lp = lforw(lp);  /* line 5: "    z = 3" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    z = 3", "nested region: line 5 unchanged");
}

static void test_py_indent_region_dedent_keyword(void)
{
    /* Region: lines 1-5
       Line 0: "" (before region, base indent 0)
       Line 1: "if True:" -> col=0, push 4
       Line 2: "    x = 1" -> col=4
       Line 3: "    if nested:" -> col=4, push 8
       Line 4: "        y = 2" -> col=8
       Line 5: "    else:" -> dedent keyword, align with if nested: at col=4
    */
    char *lines[] = {"", "if True:", "    x = 1", "    if nested:", "        y = 2", "    else:"};
    char buf[64];
    LINE *lp;

    setup_multiline(lines, 6, 5, 9, 1, 0, MDPYMOD);
    py_indent_region(FALSE, 1);

    lp = lforw(g_hdr);
    lp = lforw(lp);  /* line 1 */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "if True:", "dedent region: line 1");

    lp = lforw(lp);  /* line 2 */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x = 1", "dedent region: line 2");

    lp = lforw(lp);  /* line 3 */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    if nested:", "dedent region: line 3");

    lp = lforw(lp);  /* line 4 */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "        y = 2", "dedent region: line 4");

    lp = lforw(lp);  /* line 5: "    else:" -> "    else:" (align with if nested: at 4) */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    else:", "dedent region: line 5 dedented to 4 (align with if nested:)");
}

static void test_py_indent_region_else_body(void)
{
    /* Regression test: the line after 'else:' (which ends with ':')
       must be indented even when its current indent is 0.

       Region: lines 1-4
       Line 0: "" (before region, base indent 0)
       Line 1: "if True:"  -> col=0, push 4
       Line 2: "    x=1"   -> col=4
       Line 3: "else:"     -> col=0 (dedent keyword), push 4
       Line 4: "x=2"       -> col=4 (indented after else: colon)
    */
    char *lines[] = {"", "if True:", "    x=1", "else:", "x=2"};
    char buf[64];
    LINE *lp;

    setup_multiline(lines, 5, 4, 3, 1, 0, MDPYMOD);
    py_indent_region(FALSE, 1);

    lp = lforw(g_hdr);
    lp = lforw(lp);  /* line 1: "if True:" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "if True:", "else_body region: line 1");

    lp = lforw(lp);  /* line 2: "    x=1" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x=1", "else_body region: line 2");

    lp = lforw(lp);  /* line 3: "else:" */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "else:", "else_body region: line 3 (dedent to 0)");

    lp = lforw(lp);  /* line 4: "x=2" -> "    x=2" (indented after else: colon) */
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    x=2", "else_body region: line 4 indented to 4 after else: colon");
}

/* ================================================================ */
/*  py_mode() tests                                                 */
/* ================================================================ */

static void test_py_mode_set_on(void)
{
    setup_single("x = 1", 5, 0);
    py_mode(TRUE, 1);
    TEST_ASSERT(curbp->b_mode & MDPYMOD,
        "py_mode: with arg -> PYMODE ON");
}

static void test_py_mode_toggle(void)
{
    setup_single("x = 1", 5, 0);
    py_mode(FALSE, 1);  /* no arg -> toggle */
    TEST_ASSERT(curbp->b_mode & MDPYMOD,
        "py_mode: toggle from OFF -> ON");
    py_mode(FALSE, 1);  /* toggle again -> OFF */
    TEST_ASSERT(!(curbp->b_mode & MDPYMOD),
        "py_mode: toggle from ON -> OFF");
}

static void test_py_mode_set_off(void)
{
    setup_single("x = 1", 5, MDPYMOD);
    py_mode(TRUE, -1);  /* negative arg -> OFF */
    TEST_ASSERT(!(curbp->b_mode & MDPYMOD),
        "py_mode: negative arg -> PYMODE OFF");
}

/* ================================================================ */
/*  Test registry                                                  */
/* ================================================================ */

TEST_LIST({
    /* pyinsert tests */
    {"pyinsert after colon",            test_py_insert_after_colon},
    {"pyinsert no colon",                test_py_insert_no_colon},
    {"pyinsert inherited indent",        test_py_insert_inherited_indent},
    {"pyinsert nested colon",            test_py_insert_nested_colon},

    /* py_backspace tests */
    {"py_backspace dedent",              test_py_backspace_dedent},
    {"py_backspace partial dedent",      test_py_backspace_partial_dedent},
    {"py_backspace not whitespace",      test_py_backspace_not_whitespace},
    {"py_backspace tab mode",            test_py_backspace_tab_mode},
    {"py_backspace at col zero",         test_py_backspace_at_col_zero},
    {"py_backspace no pymode",           test_py_backspace_no_pymode},

    /* py_mode tests */
    {"py_mode set on (with arg)",       test_py_mode_set_on},
    {"py_mode toggle",                  test_py_mode_toggle},
    {"py_mode set off (neg arg)",       test_py_mode_set_off},

    /* pyindent tests */
    {"pyindent with colon",              test_py_indent_with_colon},
    {"pyindent no colon",                test_py_indent_no_colon},
    {"pyindent dedent keyword",          test_py_indent_dedent_keyword},
    {"pyindent skip blank prev",         test_py_indent_skip_blank_prev},
    {"pyindent nested colon",            test_py_indent_nested_colon},
    {"pyindent already correct",         test_py_indent_already_correct},
    {"pyindent no previous line",        test_py_indent_no_previous_line},

    /* py_indent_region tests */
    {"py_indent_region simple",          test_py_indent_region_simple},
    {"py_indent_region nested",          test_py_indent_region_nested},
    {"py_indent_region dedent keyword",  test_py_indent_region_dedent_keyword},
    {"py_indent_region else body",       test_py_indent_region_else_body},
})

int main(void)
{
    RUN_TESTS();
}
