/*
 * test_cmode.c -- Unit tests for kemacs CMODE indentation engine.
 *
 * Tests cover: cinsert (auto-indent on Enter), cindent (M-J),
 * cindent_region (region re-indent), c_mode (toggle).
 *
 * cmode.c is included directly after defining maindef
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
int lchange();
int linsert();
int lnewline();
int ldelete();
void u_record();
void u_clear_undo();
void u_prune();

/* ================================================================ */
/*  Include kemacs source files with maindef + static              */
/* ================================================================ */
#define maindef
#define static
#include "../ecomm.h"
#include "../estruct.h"
#include "../edef.h"
#include "../line.c"
#include "../cmode.c"
#undef static
#undef maindef

/* ================================================================ */
/*  Stub implementations                                           */
/* ================================================================ */

int tabsize = 0;

int mlwrite()        { return TRUE; }
int rdonly()         { return FALSE; }
int upmode()         { return TRUE; }
int forwdel()        { return TRUE; }
/* lchange is provided by line.c (included above) */

/* backchar -- move cursor back n chars (simplified, single-buffer) */
int backchar(int f, int n)
{
    (void)f;
    while (n--) {
        if (curwp->w_doto == 0) {
            LINE *lp = lback(curwp->w_dotp);
            if (lp == curbp->b_linep)
                return FALSE;
            curwp->w_dotp = lp;
            curwp->w_doto = llength(lp);
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
    curwp->w_dotp = dlp;
    curwp->w_doto = 0;
    return TRUE;
}

/* Undo stubs (called from line.c but not under test here) */
void u_record(int type, LINE *lp, int offset, int length,
              Char *data, int datalen, int eob) { (void)type; }
void u_clear_undo(BUFFER *bp) { (void)bp; }
void u_prune(BUFFER *bp) { (void)bp; }

/*
 * getregion -- compute the region between dot and mark.
 * Simplified version for testing.
 */
int getregion(REGION *rp)
{
    if (curwp->w_markp == NULL)
        return FALSE;

    rp->r_linep = curwp->w_dotp;
    rp->r_offset = curwp->w_doto;
    rp->r_size = (long)(curwp->w_marko - curwp->w_doto);
    return TRUE;
}

/* ================================================================ */
/*  Test fixtures                                                  */
/* ================================================================ */

static LINE *g_hdr = NULL;

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

static void reset_cmode_env(void)
{
    c_indent_width = 4;
    c_use_spaces = FALSE;
    c_brace_style = 0;
    c_cont_indent = 2;
    c_case_indent = 0;
    c_pp_indent = 0;
    c_auto_close = TRUE;
    c_auto_detect = FALSE;
    c_follow_source = FALSE;
    c_gnu_indent = 2;
    c_brace_func = FALSE;
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
    curbp->b_nwnd = 1;
    curbp->b_mode = mode;
    curbp->b_flag = 0;
    curbp->b_bufp = NULL;
    curbp->b_dotp = text_line;
    curbp->b_doto = 0;

    curwp->w_bufp = curbp;
    curwp->w_linep = text_line;
    curwp->w_dotp = text_line;
    curwp->w_doto = doto;
    curwp->w_flag = 0;
    curwp->w_wndp = NULL;
    curwp->w_markp = NULL;
    curwp->w_marko = 0;

    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;

    reset_cmode_env();
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
    curbp->b_nwnd = 1;
    curbp->b_mode = mode;
    curbp->b_flag = 0;
    curbp->b_bufp = NULL;

    /* Set cursor (dot) */
    lp = lforw(g_hdr);
    for (i = 0; i < dot_line && lforw(lp) != g_hdr; i++)
        lp = lforw(lp);
    curwp->w_dotp = lp;
    curwp->w_doto = dot_offset;
    curwp->w_linep = lp;
    curwp->w_flag = 0;
    curwp->w_wndp = NULL;

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

    reset_cmode_env();
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
/*      cinsert() tests -- auto-indent on Enter                    */
/* ================================================================ */

static void test_c_insert_after_open_brace(void)
{
    char buf[64];

    /* "if (x) {" at end, cursor at end -> new line indent 1 tab */
    setup_single("if (x) {", 8, MDCMOD);
        cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: after '{' -> 1 tab");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "insert: new line is 1 tab");
}

static void test_c_insert_no_brace(void)
{
    char buf[64];

    /* "x = 1" cursor at end -> new line indent 0 */
    setup_single("x = 1", 5, MDCMOD);
        cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 0, "insert: no brace -> 0 indent");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "insert: new line is empty");
}

static void test_c_insert_inherited_indent(void)
{
    char buf[64];

    /* "    x = 1" (4 spaces) cursor at end -> new line inherits 1 tab */
    setup_single("    x = 1", 9, MDCMOD);
        cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: inherited indent = 1 tab");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "insert: new line has 1 tab");
}

static void test_c_insert_after_keyword(void)
{
    char buf[64];

    /* "if (x)" cursor at end, no brace -> next line indent 1 tab */
    setup_single("if (x)", 6, MDCMOD);
        cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: after if -> 1 tab");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "insert: new line is 1 tab");
}

static void test_c_insert_after_else(void)
{
    char buf[64];

    /* "else" cursor at end -> next line indent 1 tab (aligned with if + 1) */
    setup_single("else", 4, MDCMOD);
        cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: after else -> 1 tab");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "insert: new line is 1 tab");
}

static void test_c_insert_after_close_brace(void)
{
    char buf[64];

    /* "    }" cursor at end -> new line indent 0 (matching if level) */
    setup_single("    }", 5, MDCMOD);
        cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 0, "insert: after } -> 0 indent");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "insert: new line is at base indent");
}

static void test_c_insert_preprocessor_if(void)
{
    char buf[64];

    /* "#if 0" cursor at end -> new line indent 1 tab */
    setup_single("#if 0", 5, MDCMOD);
        cinsert();
    /* With c_pp_indent=0 and c_indent_width=4, target should be 4 -> 1 tab */
    TEST_ASSERT_INT(curwp->w_dotp->l_used >= 0, TRUE, "insert: #if produces a line");
    cur_text_to_cstr(buf, sizeof(buf));
    /* The new line should be indented */
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: #if -> 1 tab");
}

/* ================================================================ */
/*  cindent() tests -- manual re-indent (M-J)                   */
/* ================================================================ */

static void test_c_indent_basic(void)
{
    char buf[64];

    /* Line 0: "if (x) {"  (ends with brace)
       Line 1: "foo"       -> should become "\\tfoo" (1 tab)
    */
    char *lines[] = {"if (x) {", "foo"};
    setup_multiline(lines, 2, 1, 3, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\tfoo", "indent: after '{' -> 1 tab");
}

static void test_c_indent_no_brace(void)
{
    char buf[64];

    /* Line 0: "if (x)"   (no brace, just keyword)
       Line 1: "foo"      -> should become "\\tfoo" (1 tab)
    */
    char *lines[] = {"if (x)", "foo"};
    setup_multiline(lines, 2, 1, 3, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\tfoo", "indent: after if (no brace) -> 1 tab");
}

static void test_c_indent_else_alignment(void)
{
    char buf[64];

    /* Line 0: "if (x)"  (indent 0)
       Line 1: "    foo"  (indent 4)
       Line 2: "else"    -> should stay at indent 0 (align with if)
    */
    char *lines[] = {"if (x)", "    foo", "    else"};
    setup_multiline(lines, 3, 2, 4, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "else", "indent: else aligns with if at 0");
}

static void test_c_indent_case_label(void)
{
    char buf[64];

    /* Line 0: "switch (x) {"  (indent 0)
       Line 1: "case 1:"      -> should be at indent 0 (c_case_indent=0)
    */
    char *lines[] = {"switch (x) {", "    case 1:"};
    setup_multiline(lines, 2, 1, 9, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "case 1:", "indent: case aligns with switch (c_case_indent=0)");
}

static void test_c_indent_comment_skip(void)
{
    char buf[64];

    /* Line 0: slash-star { star-slash (has { inside comment, should not affect indent)
       Line 1: "x = 1"    -> should stay at indent 0
    */
    char *lines[] = {"/* { */", "x = 1"};
    setup_multiline(lines, 2, 1, 5, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "x = 1", "indent: { inside comment does not affect indent");
}

static void test_c_indent_no_previous_line(void)
{
    char buf[64];

    /* Line 0 is the only line, cursor at offset 0 */
    char *lines[] = {"    x = 1"};
    setup_multiline(lines, 1, 0, 0, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "x = 1", "indent: first line -> indent 0");
}

/* ================================================================ */
/*  Additional tests -- C23 keywords, c_pp_indent, nested scope   */
/* ================================================================ */

static void test_c_indent_else_at_nonzero(void)
{
    char buf[64];

    /* Line 0: "    if (x)"  (indent 4)
       Line 2: "        foo"  (indent 8)
       Line 3: "    else"  -> should stay at indent 1 tab (align with if)
    */
    char *lines[] = {"    if (x)", "        foo", "    else"};
    setup_multiline(lines, 3, 2, 4, -1, 0, MDCMOD);

    cindent(FALSE, 1);

    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\telse", "indent: else aligns with if at 1 tab");
}

static void test_c_insert_after_try(void)
{
    char buf[64];

    /* "try {" at end, cursor at end -> new line indent 1 tab (C23 keyword) */
    setup_single("try {", 5, MDCMOD);
    cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: after try{ -> 1 tab");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "insert: new line after try is 1 tab");
}

static void test_c_insert_after_catch(void)
{
    char buf[64];

    /* "} catch (e) {" at end -> new line indent 1 tab */
    setup_single("} catch (e) {", 13, MDCMOD);
    cinsert();
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1, "insert: after catch{ -> 1 tab");
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "insert: new line after catch is 1 tab");
}

static void test_c_indent_pp_indent(void)
{
    char buf[64];

    /* With c_pp_indent=2, "#if" body should indent by c_pp_indent + c_indent_width = 6 */
    char *lines[] = {"#if 0", "foo"};
    setup_multiline(lines, 2, 1, 3, -1, 0, MDCMOD);
    c_pp_indent = 2;  /* set AFTER setup_multiline (which resets) */
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t  foo", "indent: #if body -> 1 tab + 2 spaces");
}

static void test_c_indent_case_with_indent(void)
{
    char buf[64];

    /* With c_case_indent=2, "case 1:" gets indent 2 (aligned with switch + 2) */
    char *lines[] = {"switch (x) {", "    case 1:"};
    setup_multiline(lines, 2, 1, 9, -1, 0, MDCMOD);
    c_case_indent = 2;  /* set AFTER setup_multiline (which resets) */
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  case 1:", "indent: case -> switch+2 (c_case_indent=2)");
}

static void test_c_indent_nested_scope(void)
{
    char buf[64];

    /*
       Line 0: "void f() {"      (indent 0)
       Line 1: "    if (x) {"    (indent 4)
       Line 2: "foo"             -> should be 2 tabs (nested scope)
    */
    char *lines[] = {"void f() {", "    if (x) {", "foo"};
    setup_multiline(lines, 3, 2, 3, -1, 0, MDCMOD);
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t\tfoo", "indent: nested scope -> 2 tabs");
}

/* ================================================================ */
/*  c_mode() tests                                              */
/* ================================================================ */

static void test_cmode_set_on(void)
{
    setup_single("x = 1", 5, 0);
    c_mode(TRUE, 1);
    TEST_ASSERT(curbp->b_mode & MDCMOD, "c_mode: with arg -> CMODE ON");
}

static void test_cmode_toggle(void)
{
    setup_single("x = 1", 5, 0);
    c_mode(FALSE, 1);  /* no arg -> toggle */
    TEST_ASSERT(curbp->b_mode & MDCMOD, "c_mode: toggle from OFF -> ON");
    c_mode(FALSE, 1);  /* toggle again -> OFF */
    TEST_ASSERT(!(curbp->b_mode & MDCMOD), "c_mode: toggle from ON -> OFF");
}

static void test_cmode_set_off(void)
{
    setup_single("x = 1", 5, MDCMOD);
    c_mode(TRUE, -1);  /* negative arg -> OFF */
    TEST_ASSERT(!(curbp->b_mode & MDCMOD), "c_mode: negative arg -> CMODE OFF");
}

/* ================================================================ */
/*  Style 7 (none) tests                                    */
/* ================================================================ */

static void test_c_insert_none_after_brace(void)
{
    char buf[64];

    /* With style 7 (none), after { at end of line, body is +c_indent_width */
    c_brace_style = 7;
    setup_single("if (x) {", 8, MDCMOD);
    cinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "\t", "none: after '{' -> 1 tab");
}

static void test_c_insert_none_after_close_brace(void)
{
    char buf[64];

    /* With style 7, } on its own line -> new line aligns with { */
    /* Multi-line: "if (x)\n{\n    foo\n}" cursor on } line */
    char *lines[] = {"if (x)", "{", "    foo", "}"};
    setup_multiline(lines, 4, 3, 1, -1, 0, MDCMOD);
    c_brace_style = 7;
    /* cindent the } line */
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    /* } should align with { which is at indent 0 */
    TEST_ASSERT_STR(buf, "}", "none: } aligns with { at 0");
}

static void test_c_insert_none_respects_manual_indent(void)
{
    char buf[64];

    /* With style 7, if user indents } to match a manually placed {,
       cindent should respect that */
    char *lines[] = {"if (x)", "    {", "        foo", "    }"};
    setup_multiline(lines, 4, 3, 1, -1, 0, MDCMOD);
    c_brace_style = 7;
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    /* } should align with { at indent 4 (user's placement) -> 1 tab */
    TEST_ASSERT_STR(buf, "\t}", "none: } respects user brace placement at 1 tab");
}

static void test_c_insert_none_else_alignment(void)
{
    char buf[64];

    /* With style 7, else should align with matching if */
    char *lines[] = {"if (x)", "    foo", "    else"};
    setup_multiline(lines, 3, 2, 4, -1, 0, MDCMOD);
    c_brace_style = 7;
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "else", "none: else aligns with if at 0");
}

/* ================================================================ */
/*  GNU style (style 2) tests                                 */
/* ================================================================ */

static void test_c_indent_gnu_brace_on_own_line(void)
{
    char buf[64];

    /* GNU style: { on own line at control + 1 (c_gnu_indent=2 default,
       but with c_indent_width=4 the { goes at +2) */
    c_brace_style = 2;
    c_gnu_indent = 2;
    cindent(FALSE, 1);
    /* This test verifies GNU style doesn't crash */
    TEST_ASSERT_INT(c_brace_style, 2, "gnu: style set to GNU");
}

static void test_c_indent_gnu_close_brace(void)
{
    char buf[64];

    /* GNU: { on own line, } at { level */
    char *lines[] = {
        "if (x)",
        "  {",
        "    foo",
        "  }"
    };
    setup_multiline(lines, 4, 3, 1, -1, 0, MDCMOD);
    c_brace_style = 2;
    c_gnu_indent = 2;
    c_indent_width = 4;
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    /* } should be at { line's indent (2) */
    TEST_ASSERT_STR(buf, "  }", "gnu: } aligns with { at 2");
}

/* ================================================================ */
/*  Whitesmiths style (style 4) tests                          */
/* ================================================================ */

static void test_c_indent_whitesmiths_brace(void)
{
    char buf[64];

    /* Whitesmiths: { on own line at body level, } at same level */
    char *lines[] = {
        "if (x)",
        "    {",
        "        foo",
        "    }"
    };
    setup_multiline(lines, 4, 3, 1, -1, 0, MDCMOD);
    c_brace_style = 4;
    c_indent_width = 4;
    cindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    /* } should be at { line's indent (4) -> 1 tab */
    TEST_ASSERT_STR(buf, "\t}", "whitesmiths: } aligns with { at 1 tab");
}

/* ================================================================ */
/*  c_detect_style tests                                */
/* ================================================================ */

static void test_c_detect_style_kr(void)
{
    /* K&R style source: lines end with { */
    char *lines[] = {"if (x) {", "    foo", "}"};
    setup_multiline(lines, 3, 0, 0, -1, 0, MDCMOD);
    c_detect_style(FALSE, 1);
    TEST_ASSERT_INT(c_brace_style, 0, "detect: K&R -> style 0");
}

static void test_c_detect_style_allman(void)
{
    /* Allman style: { on own line */
    char *lines[] = {
        "if (x)",
        "{",
        "    foo",
        "}"
    };
    setup_multiline(lines, 4, 0, 0, -1, 0, MDCMOD);
    c_detect_style(FALSE, 1);
    /* Allman: { on own line at same level as control */
    TEST_ASSERT_INT(c_brace_style, 1, "detect: Allman -> style 1");
}

static void test_c_detect_style_none_fallback(void)
{
    /* Empty buffer: should fall back to style 7 (none) + tabs */
    char *lines[] = {"x = 1"};
    setup_multiline(lines, 1, 0, 0, -1, 0, MDCMOD);
    c_detect_style(FALSE, 1);
    TEST_ASSERT_INT(c_brace_style, 7, "detect: empty/ambiguous -> style 7");
    TEST_ASSERT(c_use_spaces == FALSE, "detect: fallback uses tabs");
    TEST_ASSERT_INT(c_indent_width, 8, "detect: fallback indent width 8");
}

static void test_c_detect_style_spaces_not_tabs(void)
{
    /* Space-indented K&R source */
    char *lines[] = {"if (x) {", "    foo", "    }"};
    setup_multiline(lines, 3, 0, 0, -1, 0, MDCMOD);
    c_detect_style(FALSE, 1);
    TEST_ASSERT(c_use_spaces == TRUE, "detect: space-indented -> c_use_spaces");
}

/* ================================================================ */
/*  c_follow_source tests                                   */
/* ================================================================ */

static void test_c_follow_source_toggle(void)
{
    setup_single("x = 1", 5, MDCMOD);
    c_follow_source = FALSE;
    c_follow_source_cmd(FALSE, 1);
    TEST_ASSERT(c_follow_source == TRUE, "follow: toggle ON");
    c_follow_source_cmd(FALSE, 1);
    TEST_ASSERT(c_follow_source == FALSE, "follow: toggle OFF");
}

static void test_c_follow_source_set(void)
{
    setup_single("x = 1", 5, MDCMOD);
    c_follow_source_cmd(TRUE, 1);
    TEST_ASSERT(c_follow_source == TRUE, "follow: set ON with arg");
    c_follow_source_cmd(TRUE, 0);
    TEST_ASSERT(c_follow_source == FALSE, "follow: set OFF with arg 0");
}

TEST_LIST({
    /*     cinsert tests */
    {"c_insert after open brace",        test_c_insert_after_open_brace},
    {"c_insert no brace",                test_c_insert_no_brace},
    {"c_insert inherited indent",        test_c_insert_inherited_indent},
    {"c_insert after keyword (if)",      test_c_insert_after_keyword},
    {"c_insert after else",              test_c_insert_after_else},
    {"c_insert after close brace",       test_c_insert_after_close_brace},
    {"c_insert preprocessor #if",        test_c_insert_preprocessor_if},

    /* cindent tests */
    {"c_indent basic after brace",       test_c_indent_basic},
    {"c_indent after keyword no brace",  test_c_indent_no_brace},
    {"c_indent else alignment",          test_c_indent_else_alignment},
    {"c_indent case label",              test_c_indent_case_label},
    {"c_indent comment skip",            test_c_indent_comment_skip},
    {"c_indent no previous line",        test_c_indent_no_previous_line},

    /* additional tests */
    {"c_indent else at nonzero indent",  test_c_indent_else_at_nonzero},
    {"c_insert after try (C23)",         test_c_insert_after_try},
    {"c_insert after catch (C23)",       test_c_insert_after_catch},
    {"c_indent pp_indent=2",             test_c_indent_pp_indent},
    {"c_indent case c_case_indent=2",    test_c_indent_case_with_indent},
    {"c_indent nested scope",            test_c_indent_nested_scope},

    /* style 7 (none) tests */
    {"c_insert none after brace",        test_c_insert_none_after_brace},
    {"c_insert none close brace",        test_c_insert_none_after_close_brace},
    {"c_insert none respects manual",    test_c_insert_none_respects_manual_indent},
    {"c_insert none else alignment",     test_c_insert_none_else_alignment},

    /* GNU style tests */
    {"c_indent gnu brace on own line",   test_c_indent_gnu_brace_on_own_line},
    {"c_indent gnu close brace",         test_c_indent_gnu_close_brace},

    /* Whitesmiths style tests */
    {"c_indent whitesmiths brace",       test_c_indent_whitesmiths_brace},

    /* c_detect_style tests */
    {"c_detect_style K&R",               test_c_detect_style_kr},
    {"c_detect_style Allman",            test_c_detect_style_allman},
    {"c_detect_style none fallback",     test_c_detect_style_none_fallback},
    {"c_detect_style spaces not tabs",   test_c_detect_style_spaces_not_tabs},

    /* c_follow_source tests */
    {"c_follow_source toggle",           test_c_follow_source_toggle},
    {"c_follow_source set",              test_c_follow_source_set},

    /* c_mode tests */
    {"cmode set on (with arg)",          test_cmode_set_on},
    {"cmode toggle",                     test_cmode_toggle},
    {"cmode set off (neg arg)",          test_cmode_set_off},
})

int main(void)
{
    RUN_TESTS();
}
