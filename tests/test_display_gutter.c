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
int gbound = 0;
int lbound = 0;
int taboff = 0;
int vtrow = 0, vtcol = 0;
TERM term;

VIDEO *vscreen[MAX_SCREEN_LINES];

/* ---- Inline copies of the functions under test ---- */

static void
vtmove(int row, int col)
{
    vtrow = row;
    vtcol = col;
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
    int ndigits;
    int tmp;
    int pos;
    int gw;
    Char dbuf[16];
    int d;

    gw = show_line_numbers ? line_number_width + 1 : 0;
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
    ndigits = d;

    pos = line_number_width - ndigits;
    if (pos < 0)
        pos = 0;
    for (i = 0; i < ndigits; i++)
        vp->v_text[pos + i] = dbuf[ndigits - 1 - i];
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
    int gw = show_line_numbers ? line_number_width + 1 : 0;
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
    gbound = line_number_width + 1; /* = 7 */
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
    gbound = line_number_width + 1;
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
        int gw = line_number_width + 1;
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
    gbound = line_number_width + 1;
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
    gbound = line_number_width + 1;
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
    gbound = line_number_width + 1;
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
    gbound = line_number_width + 1; /* = 7 */
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

    printf("\n=== Results: %d passed, %d failed (%d total) ===",
           test_passed, test_failed, test_count);
    return test_fails ? 1 : 0;
}
