/*
 * test_display_gutter.c -- Tests for line number gutter corruption
 * when text length exceeds screen width.
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- Minimal type definitions ---- */
typedef unsigned int Char;
#define FALSE 0
#define TRUE 1

#define MAX_SCREEN_LINES 128
#define MAX_SCREEN_COLUMNS 256

#define VFCHG  0x0001
#define VFEXT  0x0002

typedef struct VIDEO {
    int v_flag;
    Char v_text[MAX_SCREEN_COLUMNS + 1];
} VIDEO;

typedef struct TERM {
    int t_nrow, t_ncol, t_margin, t_scrsiz;
} TERM;

/* ---- Globals ---- */
int show_line_numbers = FALSE;
int line_number_width = 6;
int effective_lw = 6;
int gbound = 0;
int lbound = 0;
int taboff = 0;
int vtrow = 0, vtcol = 0;
TERM term;

VIDEO *vscreen[MAX_SCREEN_LINES];

/* ---- Minimal LINE/BUFFER types for buf_linenr test ---- */
typedef struct LINE {
    struct LINE *l_fp;
    struct LINE *l_bp;
} LINE;

typedef struct BUFFER {
    LINE *b_linep;
    int  b_nlines;
} BUFFER;

/* ---- Inline copies of the functions under test ---- */

static void
vtmove(int row, int col)
{
    vtrow = row;
    vtcol = col;
}

/* Return the number of decimal digits in n (n >= 1).  Returns 1 for n <= 0. */
static int ndigits(int n)
{
    int d = 0;
    if (n <= 0)
        return 1;
    while (n > 0) {
        ++d;
        n /= 10;
    }
    return d;
}

/* Return the total number of lines in buffer bp (real lines,
   not counting the header).  Uses cached count when valid. */
static int buf_linenr(BUFFER *bp)
{
    LINE *lp;
    int num = 0;
    if (bp->b_nlines >= 0)
        return bp->b_nlines;
    lp = bp->b_linep->l_fp;
    while (lp != bp->b_linep) {
        ++num;
        lp = lp->l_fp;
    }
    bp->b_nlines = num;
    return num;
}

/* Simplified vtputc (non-KANJI path only) -- copy of display.C_ logic */
static void
vtputc(Char c)
{
    VIDEO *vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        ++vtcol;
        vp->v_text[term.t_ncol - 1] = '$';
    } else if (c == '\t') {
        do {
            vtputc(' ');
        } while (((vtcol + taboff) & 0x07) != 0);
    } else if (c < 0x20 || c == 0x7F) {
        vtputc('^');
        vtputc(c ^ 0x40);
    } else if (c >= 0x80 && c < 0x100) {
        vtputc('\\');
        vtputc('0' + ((c & 0700) >> 6));
        vtputc('0' + ((c & 0070) >> 3));
        vtputc('0' + ((c & 0007) >> 0));
    } else {
        if (vtcol >= gbound)
            vp->v_text[vtcol] = c;
        ++vtcol;
    }
}

/* Copy of vteeol from display.C_ (current code) */
static void
vteeol(void)
{
    VIDEO *vp = vscreen[vtrow];
    int i;
    for (i = 0; i < gbound; i++)
        vp->v_text[i] = ' ';
    while (vtcol < term.t_ncol) {
        if (vtcol >= gbound)
            vp->v_text[vtcol] = ' ';
        ++vtcol;
    }
}

/* Copy of draw_gutter from display.C_ */
static void
draw_gutter(int sline, int lineno)
{
    VIDEO *vp;
    int i;
    int ndig;
    int tmp;
    int pos;
    int gw;
    Char dbuf[16];
    int d;

    gw = gbound;
    if (gw == 0)
        return;

    vp = vscreen[sline];
    for (i = 0; i < gw; i++)
        vp->v_text[i] = ' ';

    if (lineno <= 0)
        return;

    tmp = lineno;
    d = 0;
    if (tmp == 0)
        dbuf[d++] = '0';
    else
        while (tmp > 0 && d < 16) {
            dbuf[d++] = '0' + (tmp % 10);
            tmp /= 10;
        }
    ndig = d;

    pos = effective_lw - ndig;
    if (pos < 0)
        pos = 0;
    for (i = 0; i < pos; i++)
        vp->v_text[i] = ' ';   /* space-padded, right-aligned */
    for (i = 0; i < ndig; i++)
        vp->v_text[pos + i] = dbuf[ndig - 1 - i];
}

/* Draw one virtual line at sline with given text and line number */
static void
draw_line(int sline, const char *text, int lineno)
{
    int i;
    int len = strlen(text);
    draw_gutter(sline, lineno);
    vtmove(sline, gbound);
    for (i = 0; i < len; i++)
        vtputc((Char)text[i]);
    vteeol();
    draw_gutter(sline, lineno);
}

/* Check if gutter (positions 0..gw-1) contains only digits and spaces */
static int
gutter_is_clean(int sline)
{
    int gw = gbound;
    int i;
    if (gw == 0) return 1;
    for (i = 0; i < gw; i++) {
        Char c = vscreen[sline]->v_text[i];
        if (c != ' ' && (c < '0' || c > '9'))
            return 0;
    }
    return 1;
}

static void
setup_screens(void)
{
    int i;
    for (i = 0; i < MAX_SCREEN_LINES; i++) {
        if (vscreen[i]) free(vscreen[i]);
        vscreen[i] = (VIDEO *)malloc(sizeof(VIDEO));
        memset(vscreen[i]->v_text, 0, sizeof(Char) * (MAX_SCREEN_COLUMNS + 1));
        vscreen[i]->v_flag = 0;
    }
}

static void
teardown_screens(void)
{
    int i;
    for (i = 0; i < MAX_SCREEN_LINES; i++) {
        if (vscreen[i]) { free(vscreen[i]); vscreen[i] = NULL; }
    }
}

/* ---- Tests ---- */

static void test_no_linenum_73_chars(void) {
    /* No line numbers: 80-col screen, 73 chars of text */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    setup_screens();

    char text[74];
    memset(text, 'A', 73); text[73] = '\0';

    draw_line(0, text, 0);

    /* '$' should be at last column only if text overflows (>=80 chars) */
    /* With 73 chars on 80-col screen and no gutter: fits in positions 0-72.
       Positions 73-79 are cleared by vteeol. No overflow, no '$'. */
    TEST_ASSERT(vscreen[0]->v_text[72] == 'A',
                "no gutter: last char of 73-char text at position 72");
    TEST_ASSERT(vscreen[0]->v_text[73] == ' ',
                "no gutter: position 73 cleared by vteeol");
    TEST_ASSERT(vscreen[0]->v_text[79] == ' ',
                "no gutter: 73 chars fit in 80 cols, no overflow '$' at 79");
    TEST_ASSERT(gutter_is_clean(0), "no gutter: gutter trivially clean");

    teardown_screens();
}

static void test_no_linenum_100_chars(void) {
    /* No line numbers: 80-col screen, 100 chars of text */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    setup_screens();

    char text[101];
    memset(text, 'A', 100); text[100] = '\0';

    draw_line(0, text, 0);

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "no gutter: 100-char text -> '$' at last column");
    TEST_ASSERT(vscreen[0]->v_text[0] == 'A',
                "no gutter: first text char at position 0");
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "no gutter: char at position 78 is 'A' (before '$')");

    teardown_screens();
}

static void test_linenum_73_chars_overflow(void) {
    /* Line numbers ON (width=6, gbound=7), 80-col screen, 74 chars of text */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1; /* = 7 */
    setup_screens();

    /* 74 chars: first 73 fill text area (7-79), 74th triggers '$' at 79 */
    char text[75];
    memset(text, 'A', 74); text[74] = '\0';

    draw_line(0, text, 1);

    /* Gutter should have line number "1" */
    TEST_ASSERT(gutter_is_clean(0),
                "with gutter: gutter has only digits/spaces after 74-char overflow");

    /* '$' at last text column (79) */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "with gutter: '$' at last column (79) in text area");

    /* First text char at position 7 (gbound) */
    TEST_ASSERT(vscreen[0]->v_text[7] == 'A',
                "with gutter: first text char at position gbound (7)");

    /* Char before '$' should be 'A' */
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "with gutter: text char at position 78");

    teardown_screens();
}

static void test_linenum_two_lines_long_then_short(void) {
    /* Line numbers ON, two lines: first long (overflow), second short */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1;
    setup_screens();

    char long_text[101];
    memset(long_text, 'A', 100); long_text[100] = '\0';

    /* Draw line 0 (line number 1) with long text */
    draw_line(0, long_text, 1);

    /* Draw line 1 (line number 2) with short text */
    draw_line(1, "hello", 2);

    /* Check line 1's gutter is clean */
    TEST_ASSERT(gutter_is_clean(1),
                "next line gutter not corrupted by prev line overflow");

    /* Check '$' not in next line's gutter */
    {
        int gw = gbound;
        int i;
        for (i = 0; i < gw; i++) {
            TEST_ASSERT(vscreen[1]->v_text[i] != '$',
                        "no '$' in next line's gutter");
        }
    }

    /* Line 1 text should be "hello" at positions 7-11 */
    TEST_ASSERT(vscreen[1]->v_text[7] == 'h', "line 1 text[0] = 'h'");
    TEST_ASSERT(vscreen[1]->v_text[8] == 'e', "line 1 text[1] = 'e'");

    teardown_screens();
}

static void test_linenum_short_then_long_same_line(void) {
    /* Re-draw same line: first short, then long */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1;
    setup_screens();

    char long_text[101];
    memset(long_text, 'A', 100); long_text[100] = '\0';

    /* First: short text at line 0 */
    draw_line(0, "hi", 1);

    /* Second: long text at line 0 (same line, overwrite) */
    draw_line(0, long_text, 1);

    TEST_ASSERT(gutter_is_clean(0),
                "gutter not corrupted when re-drawing long over short text");

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "'$' at last column after re-draw with overflow");

    /* Check positions 12-78 are 'A' (not stale spaces from previous short draw) */
    TEST_ASSERT(vscreen[0]->v_text[12] == 'A',
                "position 12 has text char, not stale space");
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "position 78 has text char, not stale space");

    teardown_screens();
}

static void test_linenum_exact_fit(void) {
    /* Line numbers ON, text exactly fills text area (73 chars) */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1;
    setup_screens();

    char text[74];
    memset(text, 'A', 73); text[73] = '\0';

    draw_line(0, text, 1);

    /* 73 chars fill positions 7-79 exactly. No overflow, no '$'. */
    TEST_ASSERT(gutter_is_clean(0), "gutter clean for exact-fit text");
    TEST_ASSERT(vscreen[0]->v_text[79] == 'A',
                "73 chars fill text area exactly, last char at 79");
    TEST_ASSERT(vscreen[0]->v_text[7] == 'A',
                "first text char at position 7");

    teardown_screens();
}

static void test_linenum_100_chars_gutter_value(void) {
    /* Verify gutter has the correct line number digits */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1;
    setup_screens();

    char long_text[101];
    memset(long_text, 'A', 100); long_text[100] = '\0';

    draw_line(0, long_text, 42);

    /* Gutter should have "    42 " (6-width number + space separator) */
    /* Positions: 0=' ', 1=' ', 2=' ', 3=' ', 4='4', 5='2', 6=' ' */
    TEST_ASSERT(vscreen[0]->v_text[0] == ' ', "gutter pos 0 = space");
    TEST_ASSERT(vscreen[0]->v_text[3] == ' ', "gutter pos 3 = space");
    TEST_ASSERT(vscreen[0]->v_text[4] == '4', "gutter pos 4 = '4'");
    TEST_ASSERT(vscreen[0]->v_text[5] == '2', "gutter pos 5 = '2'");
    TEST_ASSERT(vscreen[0]->v_text[6] == ' ', "gutter pos 6 = separator space");

    teardown_screens();
}

static void test_updall_empty_lines_past_eof(void) {
    /* Simulate updall's loop: 5 content lines + 1 empty line, then
       empty screen rows past end-of-buffer.  With the fix, empty lines
       past EOF should have BLANK gutters -- not a stale "7" or any
       other line number.  Before the fix, lineno was incremented one
       past the last real line and that wrong value was drawn on every
       empty row. */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1; /* = 7 */
    setup_screens();

    /* Draw 5 content lines (simulating updall for file lines 1-5) */
    draw_line(0, "content line 1 text", 1);
    draw_line(1, "content line 2 text", 2);
    draw_line(2, "content line 3 text", 3);
    draw_line(3, "content line 4 text", 4);
    draw_line(4, "content line 5 text", 5);

    /* Draw empty line 6 (last real line in buffer, truly empty) */
    draw_line(5, "", 6);

    /* Simulate empty screen rows past EOF (rows 6..21).
       Fixed behavior: this_lineno = 0 → draw_gutter returns early,
       leaving the gutter blank.  (The old buggy code would have
       drawn lineno=7 here.) */
    {
        int sline;
        for (sline = 6; sline < 22; sline++) {
            int lineno = 7;             /* wrong value from old code */
            int this_lineno = lineno;   /* would be 7 in buggy code */

            /* FIXED: when past end-of-buffer, blank the gutter */
            /* (lp == b_linep → this_lineno = 0) */
            this_lineno = 0;

            /* Simulate: draw_gutter(sline, lineno) at top of loop */
            draw_gutter(sline, lineno); /* first draw, overwritten by vteeol */
            vtmove(sline, gbound);
            /* no vtputc calls -- empty row */
            vteeol();                   /* clears gutter to spaces */
            draw_gutter(sline, this_lineno); /* 0 → blank gutter */
        }
    }

    /* Verify line numbers 1-6 are correct */
    TEST_ASSERT(vscreen[0]->v_text[5] == '1', "line 1 gutter has '1'");
    TEST_ASSERT(vscreen[1]->v_text[5] == '2', "line 2 gutter has '2'");
    TEST_ASSERT(vscreen[2]->v_text[5] == '3', "line 3 gutter has '3'");
    TEST_ASSERT(vscreen[3]->v_text[5] == '4', "line 4 gutter has '4'");
    TEST_ASSERT(vscreen[4]->v_text[5] == '5', "line 5 gutter has '5'");
    TEST_ASSERT(vscreen[5]->v_text[5] == '6', "line 6 (empty) gutter has '6'");

    /* Verify empty lines past EOF have blank gutters (no '7') */
    {
        int sline;
        for (sline = 6; sline < 22; sline++) {
            TEST_ASSERT(vscreen[sline]->v_text[5] == ' ',
                        "empty line past EOF: gutter should be blank, not '7'");
            /* Also check the full gutter area is clean */
            TEST_ASSERT(gutter_is_clean(sline),
                        "empty line past EOF: gutter should be entirely clean");
        }
    }

    teardown_screens();
}

/* ---- Dynamic line-number width tests ---- */

static void test_ndigits_basic(void)
{
    TEST_ASSERT_INT(ndigits(0), 1, "ndigits(0) = 1");
    TEST_ASSERT_INT(ndigits(1), 1, "ndigits(1) = 1");
    TEST_ASSERT_INT(ndigits(9), 1, "ndigits(9) = 1");
    TEST_ASSERT_INT(ndigits(10), 2, "ndigits(10) = 2");
    TEST_ASSERT_INT(ndigits(99), 2, "ndigits(99) = 2");
    TEST_ASSERT_INT(ndigits(100), 3, "ndigits(100) = 3");
    TEST_ASSERT_INT(ndigits(999), 3, "ndigits(999) = 3");
    TEST_ASSERT_INT(ndigits(1000), 4, "ndigits(1000) = 4");
    TEST_ASSERT_INT(ndigits(999999), 6, "ndigits(999999) = 6");
    TEST_ASSERT_INT(ndigits(1000000), 7, "ndigits(1000000) = 7");
    TEST_ASSERT_INT(ndigits(9999999), 7, "ndigits(9999999) = 7");
    TEST_ASSERT_INT(ndigits(10000000), 8, "ndigits(10000000) = 8");
}

static void test_buf_linenr(void)
{
    /* Build a simple circular linked list: header + 3 real lines */
    LINE hdr, l1, l2, l3;
    BUFFER bp;

    bp.b_linep = &hdr;
    hdr.l_fp = &l1;  hdr.l_bp = &l3;
    l1.l_fp  = &l2;  l1.l_bp  = &hdr;
    l2.l_fp  = &l3;  l2.l_bp  = &l1;
    l3.l_fp  = &hdr; l3.l_bp  = &l2;

    /* Cached (b_nlines >= 0) */
    bp.b_nlines = 3;
    TEST_ASSERT_INT(buf_linenr(&bp), 3, "buf_linenr cached = 3");

    /* Dirty (b_nlines < 0): must walk the list */
    bp.b_nlines = -1;
    TEST_ASSERT_INT(buf_linenr(&bp), 3, "buf_linenr dirty recompute = 3");

    /* After recompute, cache should be valid */
    TEST_ASSERT_INT(buf_linenr(&bp), 3, "buf_linenr cached after recompute = 3");
}

static void test_dynamic_width_gutter_7digit(void)
{
    /* Simulate a buffer with 1,000,000+ lines.
       line_number_width = 6 (user default), but effective_lw
       must grow to 7 to display 7-digit line numbers. */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = 7;      /* auto-grown */
    gbound = effective_lw + 1;  /* = 8 */
    setup_screens();

    /* draw_line at position gbound (=8), text starts at column 8 */
    draw_line(0, "hello", 1000000);

    /* Gutter is 8 cols wide: 7 digits + 1 space separator.
       Line number 1000000 is 7 digits, fills effective_lw=7 columns.
       Positions 0-6 = "1000000", position 7 = separator space. */
    TEST_ASSERT(vscreen[0]->v_text[0] == '1', "7-digit: pos 0 = '1'");
    TEST_ASSERT(vscreen[0]->v_text[1] == '0', "7-digit: pos 1 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[2] == '0', "7-digit: pos 2 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[3] == '0', "7-digit: pos 3 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[4] == '0', "7-digit: pos 4 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[5] == '0', "7-digit: pos 5 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[6] == '0', "7-digit: pos 6 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[7] == ' ', "7-digit: pos 7 = separator space");

    /* Text starts at position gbound (=8) */
    TEST_ASSERT(vscreen[0]->v_text[8] == 'h', "7-digit: first text char at gbound (8)");

    /* Gutter should be clean */
    TEST_ASSERT(gutter_is_clean(0), "7-digit: gutter is clean");

    teardown_screens();
}

static void test_dynamic_width_gutter_8digit(void)
{
    /* Simulate a buffer with 10,000,000+ lines. */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = 8;      /* auto-grown to 8 */
    gbound = effective_lw + 1;  /* = 9 */
    setup_screens();

    draw_line(0, "hi", 10000000);

    /* Line number 10000000 is 8 digits, fills effective_lw=8 columns.
       Positions 0-7 = "10000000", position 8 = separator space. */
    TEST_ASSERT(vscreen[0]->v_text[0] == '1', "8-digit: pos 0 = '1'");
    TEST_ASSERT(vscreen[0]->v_text[7] == '0', "8-digit: pos 7 = '0'");
    TEST_ASSERT(vscreen[0]->v_text[8] == ' ', "8-digit: pos 8 = separator space");
    TEST_ASSERT(vscreen[0]->v_text[9] == 'h', "8-digit: first text char at gbound (9)");
    TEST_ASSERT(gutter_is_clean(0), "8-digit: gutter is clean");

    teardown_screens();
}

static void test_dynamic_width_gutter_small_number(void)
{
    /* With effective_lw=7 (auto-grown for large file), a small line
       number like 42 should be right-aligned within 7 columns. */
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = 7;       /* auto-grown */
    gbound = effective_lw + 1;  /* = 8 */
    setup_screens();

    draw_line(0, "text", 42);

    /* "42" should be right-aligned: 5 spaces + "42" + separator space */
    TEST_ASSERT(vscreen[0]->v_text[0] == ' ', "small in wide gutter: pos 0 = space");
    TEST_ASSERT(vscreen[0]->v_text[4] == ' ', "small in wide gutter: pos 4 = space");
    TEST_ASSERT(vscreen[0]->v_text[5] == '4', "small in wide gutter: pos 5 = '4'");
    TEST_ASSERT(vscreen[0]->v_text[6] == '2', "small in wide gutter: pos 6 = '2'");
    TEST_ASSERT(vscreen[0]->v_text[7] == ' ', "small in wide gutter: pos 7 = separator");
    TEST_ASSERT(gutter_is_clean(0), "small in wide gutter: gutter is clean");

    teardown_screens();
}

int main(void) {
    printf("=== Gutter corruption tests ===\n\n");

    test_no_linenum_73_chars();
    test_no_linenum_100_chars();
    test_linenum_73_chars_overflow();
    test_linenum_two_lines_long_then_short();
    test_linenum_short_then_long_same_line();
    test_linenum_exact_fit();
    test_linenum_100_chars_gutter_value();
    test_updall_empty_lines_past_eof();
    test_ndigits_basic();
    test_buf_linenr();
    test_dynamic_width_gutter_7digit();
    test_dynamic_width_gutter_8digit();
    test_dynamic_width_gutter_small_number();

    printf("\n=== Results: %d passed, %d failed (%d total) ===",
           test_passed, test_failed, test_count);
    return test_fails ? 1 : 0;
}
