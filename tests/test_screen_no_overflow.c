/*
 * test_screen_no_overflow.c -- Test that screen line data does not
 * overflow into adjacent lines and that vscreen/pscreen stay consistent.
 *
 * Tests:
 *   1. Cross-line isolation: writing to vscreen[0] must NOT corrupt
 *      vscreen[1] (or any other line).
 *   2. Column boundary: v_text writes must stay within [0, term.t_ncol-1].
 *      Positions at term.t_ncol and beyond must be untouched.
 *   3. show_line_numbers gap bug: when show_line_numbers is TRUE, text
 *      must fill the full text area [gbound, term.t_ncol-1] with no gap
 *      of stale/uninitialized data between the last written character
 *      and the '$' overflow marker.
 *   4. Virtual-to-physical sync: updateline() must copy vscreen content
 *      to pscreen within [0, term.t_ncol-1] and not corrupt pscreen
 *      at positions beyond the terminal width.
 *
 * The inline vtputc below is a faithful copy of the ACTUAL display.C_
 * logic (including the show_line_numbers adjustment) to catch bugs in
 * the real code path.
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- Minimal type definitions ---- */
typedef unsigned int Char;

#define FALSE 0
#define TRUE 1

#define MAX_SCREEN_LINES   128
#define MAX_SCREEN_COLUMNS 256

#define VFCHG 0x0001
#define VFEXT 0x0002
#define VFREQ 0x0008
#define TRAILER (-1)

typedef struct VIDEO {
    int  v_flag;
    Char v_text[MAX_SCREEN_COLUMNS + 1];
} VIDEO;

typedef struct TERM {
    int t_nrow, t_ncol, t_margin, t_scrsiz;
} TERM;

/* ---- Globals (mirrors display.c / edef.h) ---- */
int gbound = 0;
int lbound = 0;
int taboff = 0;
int vtrow = 0, vtcol = 0;
int show_line_numbers = FALSE;
int line_number_width = 6;
int ttrow = 0, ttcol = 0;
int currow = 0, curcol = 0;
int eolexist = TRUE;
int revexist = FALSE;
TERM term;
VIDEO *vscreen[MAX_SCREEN_LINES];
VIDEO *pscreen[MAX_SCREEN_LINES];

/* KANJI character classification (from kanji.h) */
#define UNICODE_MARK 0x200000U
#define UNICODE_CP(c) ((unsigned int)(c) & ~UNICODE_MARK)
#define iskanji(c) (((c) & 0xff00) > 0x0100 && !isunicode(c))
#define isunicode(c) ((unsigned int)(c) >= 0x10000)
#define iscombchar(c) (char_width(c) == 0)
#define iswidechar(c) (char_width(c) == 2)

int kana_is_wide;

extern int char_width(unsigned int c);
extern void init_kana_width(void);

/* ---- Include char_width/ucswidth implementation ---- */
#include "../kanji/kanji.h"
#include "../kanji/wcwidth.h"
#include "../kanji/wcwidth.c"

/* ---- Faithful copy of display.C_ vtputc (WITH show_line_numbers) ---- */
static void
vtputc(Char c)
{
    VIDEO *vp;
    vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        Char *tp;
        ++vtcol;
#if KANJI
        for (tp = &vp->v_text[term.t_ncol-1]; *tp == TRAILER; tp--) ;
        for ( ; tp < &vp->v_text[term.t_ncol-1]; *tp++ = ' ') ;
#endif
        vp->v_text[term.t_ncol - 1] = '$';
    } else if (c == '\t') {
        do {
            vtputc(' ');
        } while (((vtcol + taboff)&0x07) != 0);
    } else if (c < 0x20 || c == 0x7F) {
        vtputc('^');
        vtputc(c ^ 0x40);
    } else if (c >= 0x80 && c < 0x100) {
        vtputc('\\');
        vtputc('0'+((c & 0700) >> 6));
        vtputc('0'+((c & 0070) >> 3));
        vtputc('0'+((c & 0007) >> 0));
#if KANJI
    } else if (iskanji(c) || isunicode(c)) {
        int w = char_width(c);
        /* Western-locale: kana rendered as 1-column narrow */
        if (w == 2 && !kana_is_wide) {
            if (iskanji(c)) {
                if (((c & 0xff00) == 0x2400) || ((c & 0xff00) == 0x2500))
                    w = 1;
            } else if (isunicode(c)) {
                unsigned int uc = UNICODE_CP(c);
                if (uc >= 0x3040 && uc <= 0x30FF)
                    w = 1;
            }
        }
        if (w == 2) {
            if (vtcol >= term.t_ncol-1 || vtcol < gbound) {
                vtputc(' ');
                vtputc(' ');
            } else {
                if (vtcol >= gbound) {
                    vp->v_text[vtcol] = c;
                    vp->v_text[vtcol+1] = TRAILER;
                }
                vtcol += 2;
            }
        } else if (w == 1) {
            if (vtcol >= term.t_ncol || vtcol < gbound) {
                vtputc(' ');
            } else {
                if (vtcol >= gbound)
                    vp->v_text[vtcol] = c;
                ++vtcol;
            }
        } else {
            /* w == 0: zero-width */
            if (vtcol >= gbound && vtcol < term.t_ncol)
                vp->v_text[vtcol] = c;
        }
#endif
    } else {
        if (vtcol >= gbound)
            vp->v_text[vtcol] = c;
        ++vtcol;
    }
}

static void
vtmove(int row, int col)
{
    vtrow = row;
    vtcol = col;
}

/* Faithful copy of display.C_ vteeol */
static void
vteeol(void)
{
    VIDEO *vp;
    int i;
    vp = vscreen[vtrow];
    for (i = 0; i < gbound; i++)
        vp->v_text[i] = ' ';
    while (vtcol < term.t_ncol) {
        if (vtcol >= gbound)
            vp->v_text[vtcol] = ' ';
        ++vtcol;
    }
}

/* Simplified updateline: copies vscreen row to pscreen row within
   [0, term.t_ncol-1].  Mirrors the essential behavior of display.C_'s
   updateline() without the reverse-video path. */
static void
updateline(int row)
{
    VIDEO *vp1 = vscreen[row];
    VIDEO *vp2 = pscreen[row];
    int i;
    for (i = 0; i < term.t_ncol; i++)
        vp2->v_text[i] = vp1->v_text[i];
}

/* ---- Faithful copy of display.C_ draw_gutter ---- */
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

    /* clear entire gutter area */
    for (i = 0; i < gw; i++)
        vp->v_text[i] = ' ';

    if (lineno <= 0)
        return;

    /* count digits */
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

    /* right-align within line_number_width columns */
    pos = line_number_width - ndigits;
    if (pos < 0)
        pos = 0;
    for (i = 0; i < pos; i++)
        vp->v_text[i] = ' ';
    for (i = 0; i < ndigits; i++)
        vp->v_text[pos + i] = dbuf[ndigits - 1 - i];
    /* separator space at position line_number_width */
    vp->v_text[line_number_width] = ' ';
}

/* ---- Simulate updext (place left-overflow '$' at gbound) ---- */
static void
simulate_updext(int currow, int curcol)
{
    int rcursor;
    int lbound_val;
    int gw = show_line_numbers ? line_number_width + 1 : 0;
    int lineno = currow + 1;

    rcursor = ((curcol - (term.t_ncol - gw)) % term.t_scrsiz) + term.t_margin;
    lbound_val = curcol - rcursor;

    /* draw gutter */
    draw_gutter(currow, lineno);
    /* position virtual cursor */
    vtmove(currow, gw - lbound_val);
    taboff = 0;

    /* place left overflow '$' at gbound */
    vscreen[currow]->v_text[gw] = '$';
}

/* ---- Simulate updall re-draw of a single line (the scrolling case) ---- */
static void
simulate_updall_line(int sline, int lineno)
{
    int gw = show_line_numbers ? line_number_width + 1 : 0;
    int j;

    vscreen[sline]->v_flag |= VFCHG;
    vscreen[sline]->v_flag &= ~VFREQ;
    if (gw > 0)
        draw_gutter(sline, lineno);
    vtmove(sline, gw);
    /* draw long text that overflows: fills [gw..79] then '$' at 79 */
    for (j = 0; j < term.t_ncol - gw + 10; j++)
        vtputc('X');
    vteeol();
    if (gw > 0)
        draw_gutter(sline, lineno);
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

        if (pscreen[i]) free(pscreen[i]);
        pscreen[i] = (VIDEO *)malloc(sizeof(VIDEO));
        memset(pscreen[i]->v_text, 0, sizeof(Char) * (MAX_SCREEN_COLUMNS + 1));
        pscreen[i]->v_flag = 0;
    }
}

static void
teardown_screens(void)
{
    int i;
    for (i = 0; i < MAX_SCREEN_LINES; i++) {
        if (vscreen[i]) { free(vscreen[i]); vscreen[i] = NULL; }
        if (pscreen[i]) { free(pscreen[i]); pscreen[i] = NULL; }
    }
}

/* ---- Wide character test constants ---- */
#define WIDE_KANJI  0x4221U     /* JIS X 0208 kanji, 2-column */
#define WIDE_UNI    (0x204E2DU) /* U+4E2D with UNICODE_MARK */
#define WIDE_EMOJI  0x1F600U   /* U+1F600 emoji, 2-column */
#define HIRA_A      0x2422U    /* hiragana あ */
#define KATA_A      0x2522U    /* katakana ア */

/* ---- Helper: check that a screen line has no content beyond term.t_ncol ---- */
static int
line_within_bounds(int row)
{
    int i;
    for (i = term.t_ncol; i < MAX_SCREEN_COLUMNS; i++) {
        Char v = vscreen[row]->v_text[i];
        if (v != 0 && v != ' ')
            return 0;
    }
    return 1;
}

/* ---- Helper: check another screen line was not corrupted ---- */
static int
line_unchanged(int row, Char fill_value)
{
    int i;
    for (i = 0; i <= MAX_SCREEN_COLUMNS; i++) {
        Char v = vscreen[row]->v_text[i];
        if (fill_value == 0) {
            if (v != 0)
                return 0;
        } else {
            if (v != fill_value)
                return 0;
        }
    }
    return 1;
}

/* ---- Helper: verify vscreen and pscreen match on a row ---- */
static int
screens_match(int row)
{
    int i;
    for (i = 0; i < term.t_ncol; i++) {
        if (vscreen[row]->v_text[i] != pscreen[row]->v_text[i])
            return 0;
    }
    return 1;
}

/* ============================================================ *
 * Test 1: ASCII overflow — writing past t_ncol must NOT corrupt
 *          vscreen[1] or any position beyond t_ncol-1 on the
 *          same line.
 * ============================================================ */
static void
test_ascii_no_cross_line(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* Write 200 ASCII chars to line 0 (way past 80-col limit) */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 200; i++)
        vtputc('A');
    vteeol();

    /* Line 0 should have '$' at position 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "ASCII overflow: '$' at last column (79)");

    /* Line 0 positions 0-78 should be 'A' (the overflow chars are
       all 'A' since they just set '$') */
    TEST_ASSERT(vscreen[0]->v_text[0] == 'A',
                "ASCII overflow: first char at position 0");
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "ASCII overflow: char at position 78 is 'A'");

    /* Line 0 must NOT have content beyond t_ncol (except '$' at 79) */
    TEST_ASSERT(line_within_bounds(0),
                "ASCII overflow: no data beyond term.t_ncol on line 0");

    /* Line 1 must be completely untouched (all zeros from setup) */
    TEST_ASSERT(line_unchanged(1, 0),
                "ASCII overflow: vscreen[1] must not be corrupted by vscreen[0] writes");

    teardown_screens();
}

/* ============================================================ *
 * Test 2: Wide character overflow — same isolation check with
 *          2-column kanji characters.
 * ============================================================ */
static void
test_wide_no_cross_line(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* Write 100 wide kanji chars to line 0 */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 100; i++)
        vtputc(WIDE_KANJI);
    vteeol();

    /* Should have '$' at position 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "wide kanji overflow: '$' at last column (79)");

    /* Line 0 must not have data beyond t_ncol */
    TEST_ASSERT(line_within_bounds(0),
                "wide kanji overflow: no data beyond term.t_ncol on line 0");

    /* Line 1 must be untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "wide kanji overflow: vscreen[1] must not be corrupted");

    /* Also check lines 2-5 are untouched */
    TEST_ASSERT(line_unchanged(2, 0), "vscreen[2] untouched");
    TEST_ASSERT(line_unchanged(3, 0), "vscreen[3] untouched");
    TEST_ASSERT(line_unchanged(5, 0), "vscreen[5] untouched");

    teardown_screens();
}

/* ============================================================ *
 * Test 3: Wide char at exact last-column boundary — writing a
 *          wide char that starts at position 78 (occupying 78-79)
 *          must NOT write at position 80 (which would be out of
 *          the valid range and could corrupt the next line's header).
 * ============================================================ */
static void
test_wide_exact_boundary(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* 78 narrow chars fill positions 0-77, then wide char at 78-79 */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 78; i++)
        vtputc('A');
    vtputc(WIDE_KANJI);  /* occupies 78 and 79 */

    /* No overflow, no '$' */
    TEST_ASSERT(vscreen[0]->v_text[79] == TRAILER,
                "wide at boundary: pos 79 = TRAILER (exact fit, no overflow)");
    TEST_ASSERT(vscreen[0]->v_text[78] == WIDE_KANJI,
                "wide at boundary: pos 78 = wide char");

    /* Position 80 (beyond t_ncol) must not be written */
    TEST_ASSERT(vscreen[0]->v_text[80] == 0,
                "wide at boundary: pos 80 untouched (no overflow into array beyond t_ncol)");

    /* Line 1 must be untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "wide at boundary: vscreen[1] must not be corrupted");

    teardown_screens();
}

/* ============================================================ *
 * Test 4: show_line_numbers gap bug — when line numbers are ON,
 *          the text should fill the full text area [gbound, t_ncol-1]
 *          with no gap of uninitialized data.  The buggy overflow
 *          check (t_ncol - line_number_width) causes text to overflow
 *          6 columns early, leaving positions 74-78 as zeros/stale.
 * ============================================================ */
static void
test_linenum_no_gap(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    gbound = line_number_width + 1; /* = 7 */
    kana_is_wide = 0;
    setup_screens();

    /* Write 73 ASCII chars starting at gbound=7.
       Text area = positions 7..79 = 73 columns.
       73 chars should fill exactly to position 79. */
    vtmove(0, gbound);
    int i;
    for (i = 0; i < 73; i++)
        vtputc('A');
    vteeol();

    /* Position 79 should be 'A' (exact fit, no overflow) */
    TEST_ASSERT(vscreen[0]->v_text[79] == 'A',
                "linenum exact fit: pos 79 = 'A' (text fills entire text area)");

    /* Positions 7-79 should all have content or spaces, not zeros */
    {
        int j;
        int has_zero = 0;
        for (j = gbound; j < term.t_ncol; j++) {
            if (vscreen[0]->v_text[j] == 0)
                has_zero = 1;
        }
        TEST_ASSERT(!has_zero,
                    "linenum exact fit: no zero/uninitialized values in text area [7,79]");
    }

    teardown_screens();
}

/* ============================================================ *
 * Test 5: show_line_numbers with overflow — 74 chars (one more
 *          than the text area width).  The '$' should be at 79,
 *          and positions 74-78 should be ' ' (cleared by vteeol),
 *          not zeros or stale data.
 * ============================================================ */
static void
test_linenum_overflow_no_stale(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    gbound = 7;
    kana_is_wide = 0;
    setup_screens();

    /* 74 chars: 73 fill positions 7-79, 74th overflows -> '$' at 79 */
    vtmove(0, gbound);
    int i;
    for (i = 0; i < 74; i++)
        vtputc('A');
    vteeol();

    /* '$' at last column */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "linenum overflow: '$' at last column (79)");

    /* Position 78 should be 'A' (the 73rd char), not stale */
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "linenum overflow: pos 78 = 'A' (last text char before '$')");

    /* Positions between last text char and '$' should be spaces (from vteeol)
       or text chars, NOT zeros or stale data */
    TEST_ASSERT(vscreen[0]->v_text[74] != 0,
                "linenum overflow: pos 74 not zero (cleared by vteeol)");

    teardown_screens();
}

/* ============================================================ *
 * Test 6: Virtual-to-physical screen sync — after updateline,
 *          pscreen should match vscreen within [0, t_ncol-1],
 *          and pscreen should not have data written beyond t_ncol.
 * ============================================================ */
static void
test_vscreen_to_pscreen_sync(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* Write some text to vscreen[0] */
    vtmove(0, 0);
    const char *text = "Hello, World!";
    int i;
    for (i = 0; text[i]; i++)
        vtputc((Char)text[i]);
    vteeol();

    /* Mark as changed, then sync to pscreen */
    vscreen[0]->v_flag |= VFCHG;
    updateline(0);

    /* pscreen[0] should match vscreen[0] within [0, 79] */
    TEST_ASSERT(screens_match(0),
                "pscreen[0] matches vscreen[0] within [0, t_ncol-1]");

    /* pscreen[0] positions beyond t_ncol should still be 0 (untouched) */
    TEST_ASSERT(pscreen[0]->v_text[80] == 0,
                "pscreen[0] pos 80 untouched (no write beyond t_ncol)");

    /* pscreen[1] should be untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "pscreen[1] untouched after pscreen[0] sync");

    teardown_screens();
}

/* ============================================================ *
 * Test 7: Multi-line write — writing to multiple lines, each
 *          with overflow, must keep lines isolated.
 * ============================================================ */
static void
test_multi_line_isolation(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* Line 0: 100 'A's (overflow) */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 100; i++)
        vtputc('A');
    vteeol();

    /* Line 1: 100 'B's (overflow) */
    vtmove(1, 0);
    for (i = 0; i < 100; i++)
        vtputc('B');
    vteeol();

    /* Line 0 should have 'A' at 0, '$' at 79 */
    TEST_ASSERT(vscreen[0]->v_text[0] == 'A', "line 0 starts with 'A'");
    TEST_ASSERT(vscreen[0]->v_text[79] == '$', "line 0 has '$' at 79");

    /* Line 1 should have 'B' at 0, '$' at 79 */
    TEST_ASSERT(vscreen[1]->v_text[0] == 'B', "line 1 starts with 'B'");
    TEST_ASSERT(vscreen[1]->v_text[79] == '$', "line 1 has '$' at 79");

    /* Line 0's '$' at 79 must not appear in line 1's text area */
    TEST_ASSERT(vscreen[1]->v_text[79] == '$', "line 1 has '$'");
    /* But line 0 must not have leaked into line 1's content */
    TEST_ASSERT(vscreen[1]->v_text[1] == 'B',
                "line 1 position 1 is 'B', not leaked from line 0");

    /* Line 0 must not have leaked into line 1 at any position */
    TEST_ASSERT(line_unchanged(2, 0),
                "line 2 untouched by multi-line writes");

    teardown_screens();
}

/* ============================================================ *
 * Test 8: Zero-width char at overflow boundary — a zero-width
 *          combining char written when vtcol is at the last
 *          column should NOT write beyond t_ncol.
 * ============================================================ */
static void
test_zero_width_at_boundary(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* 80 ASCII chars fill positions 0-79 */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 80; i++)
        vtputc('A');

    /* Position 79 is 'A', vtcol is now 80 */
    TEST_ASSERT(vscreen[0]->v_text[79] == 'A',
                "zero-width test: pos 79 = 'A' (exact fill)");

    /* Next char triggers overflow, '$' at 79 */
    vtputc('B');
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "zero-width test: overflow '$' at 79");

    /* Position 80 should still be 0 */
    TEST_ASSERT(vscreen[0]->v_text[80] == 0,
                "zero-width test: pos 80 untouched");

    /* Line 1 untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "zero-width test: vscreen[1] untouched");

    teardown_screens();
}

/* ============================================================ *
 * Test 9: Wide char (CJK locale, kana_is_wide=1) overflow isolation
 * ============================================================ */
static void
test_wide_kana_overflow_isolation(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 1;
    setup_screens();

    /* 50 wide kana chars: 50 * 2 = 100 cols, overflow at char 40 */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 50; i++)
        vtputc(KATA_A);
    vteeol();

    /* '$' at position 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "wide kana overflow: '$' at last column (79)");

    /* No data beyond t_ncol */
    TEST_ASSERT(line_within_bounds(0),
                "wide kana overflow: no data beyond t_ncol on line 0");

    /* Line 1 untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "wide kana overflow: vscreen[1] untouched");

    teardown_screens();
    kana_is_wide = 0;
}

/* ============================================================ *
 * Test 10: TRAILER cleanup during overflow — when a wide char
 *          occupies the last two columns (78-79) and more text
 *          follows, the TRAILER at position 79 must be cleaned up
 *          and '$' placed correctly.  No data should leak beyond 79.
 * ============================================================ */
static void
test_trailer_cleanup_no_leak(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    kana_is_wide = 0;
    setup_screens();

    /* 78 ASCII chars fill 0-77. Wide char at 78-79 (TRAILER at 79).
       Then one more ASCII triggers overflow. */
    vtmove(0, 0);
    int i;
    for (i = 0; i < 78; i++)
        vtputc('A');
    vtputc(WIDE_KANJI);  /* positions 78=c, 79=TRAILER */
    vtputc('B');         /* overflow: should clean TRAILER, place '$' at 79 */
    vteeol();

    /* '$' should be at 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "TRAILER cleanup: '$' at 79 after overflow");

    /* Position 78 should NOT be TRAILER (should be ' ' cleared by cleanup) */
    TEST_ASSERT(vscreen[0]->v_text[78] != TRAILER,
                "TRAILER cleanup: pos 78 not TRAILER after overflow");

    /* Position 80 should be 0 (untouched) */
    TEST_ASSERT(vscreen[0]->v_text[80] == 0,
                "TRAILER cleanup: pos 80 untouched");

    /* Line 1 untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "TRAILER cleanup: vscreen[1] untouched");

    teardown_screens();
}

/* ============================================================ *
 * Test 11: Line number gutter width consistency — when
 *          show_line_numbers is TRUE, verify that writing text
 *          that exactly fills the text area (t_ncol - gbound chars)
 *          does NOT produce '$' (no overflow), and all positions
 *          in [gbound, t_ncol-1] are filled.
 * ============================================================ */
static void
test_linenum_exact_fill(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    gbound = 7;
    kana_is_wide = 0;
    setup_screens();

    /* Text area = positions 7..79 = 73 columns.
       Write exactly 73 chars. */
    vtmove(0, gbound);
    int i;
    for (i = 0; i < 73; i++)
        vtputc('A');
    vteeol();

    /* No overflow: '$' should NOT be at position 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] != '$',
                "linenum exact fill: no '$' (73 chars fit exactly)");
    TEST_ASSERT(vscreen[0]->v_text[79] == 'A',
                "linenum exact fill: pos 79 = 'A' (last char)");

    /* No zeros in text area */
    {
        int j;
        int ok = 1;
        for (j = gbound; j < term.t_ncol; j++) {
            if (vscreen[0]->v_text[j] == 0)
                ok = 0;
        }
        TEST_ASSERT(ok,
                    "linenum exact fill: no uninitialized zeros in text area");
    }

    teardown_screens();
}

/* ============================================================ *
 * Test 12: show_line_numbers with wide chars — verify wide chars
 *          don't overflow into the next line when line numbers are on.
 * ============================================================ */
static void
test_linenum_wide_no_cross_line(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    gbound = 7;
    kana_is_wide = 0;
    setup_screens();

    /* Write 40 wide kanji chars: 40*2=80 cols from position 7.
       Should fill positions 7-78 (37 cols * 2 = 74 cols), then
       38th wide char at 79 would need TRAILER at 80 -> overflow.
       Actually: positions 7,9,11,... 7+2*36=79. 36 wide chars
       fill positions 7-78 (with trailers at 8,10,...78).
       37th wide char at position 79: vtcol=79, check
       79 >= 74 (with show_line_numbers bug) -> TRUE -> overflow '$' */

    vtmove(0, gbound);
    int i;
    for (i = 0; i < 40; i++)
        vtputc(WIDE_KANJI);
    vteeol();

    /* '$' should be at 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "linenum+wide: '$' at last column");

    /* No data beyond t_ncol */
    TEST_ASSERT(line_within_bounds(0),
                "linenum+wide: no data beyond t_ncol on line 0");

    /* Line 1 untouched */
    TEST_ASSERT(line_unchanged(1, 0),
                "linenum+wide: vscreen[1] untouched");

    teardown_screens();
}

/* ============================================================ *
 * Test 13: draw_gutter padding — when show_line_numbers is TRUE,
 *          the gutter must be padded with SPACES, not '0' (zero).
 *          The old code filled padding positions with '0', making
 *          the gutter look like "000042 " instead of "    42".
 * ============================================================ */
static void
test_gutter_padding_spaces(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    gbound = 7;
    kana_is_wide = 0;
    setup_screens();

    /* Draw gutter for line 1 (1 digit, 5 padding chars) */
    draw_gutter(0, 1);

    /* Positions 0-4 should be spaces (padding), NOT '0' */
    int i;
    int ok = 1;
    for (i = 0; i < 5; i++) {
        if (vscreen[0]->v_text[i] != ' ')
            ok = 0;
    }
    TEST_ASSERT(ok,
                "gutter padding: positions 0-4 must be spaces, not '0'");

    /* Position 5 should be '1' (the digit) */
    TEST_ASSERT(vscreen[0]->v_text[5] == '1',
                "gutter padding: position 5 = '1' (line number digit)");

    /* Position 6 should be ' ' (separator) */
    TEST_ASSERT(vscreen[0]->v_text[6] == ' ',
                "gutter padding: position 6 = ' ' (separator)");

    /* Gutter positions must NOT be '$' */
    for (i = 0; i < 7; i++) {
        if (vscreen[0]->v_text[i] == '$')
            ok = 0;
    }
    TEST_ASSERT(ok,
                "gutter padding: no '$' in gutter area [0..6]");

    /* Test with line number 42 (2 digits, 4 padding chars) */
    draw_gutter(1, 42);
    ok = 1;
    for (i = 0; i < 4; i++) {
        if (vscreen[1]->v_text[i] != ' ')
            ok = 0;
    }
    TEST_ASSERT(ok,
                "gutter padding: line 42, positions 0-3 must be spaces");
    TEST_ASSERT(vscreen[1]->v_text[4] == '4',
                "gutter padding: line 42, position 4 = '4'");
    TEST_ASSERT(vscreen[1]->v_text[5] == '2',
                "gutter padding: line 42, position 5 = '2'");

    teardown_screens();
}

/* ============================================================ *
 * Test 14: updext left-overflow marker clears on scroll — when
 *          the cursor is on an extended (horizontally scrolled)
 *          line, updext() places '$' at position gbound (left
 *          overflow indicator).  When the window is scrolled
 *          (simulated by updall re-drawing the line), that '$'
 *          must be overwritten and not persist.
 * ============================================================ */
static void
test_updext_clears_on_scroll(void)
{
    term.t_nrow = 24;
    term.t_ncol = 80;
    term.t_scrsiz = 64;   /* t_ncol - 2 * (t_ncol/10) = 80 - 16 */
    term.t_margin = 8;
    show_line_numbers = TRUE;
    line_number_width = 6;
    gbound = 7;
    kana_is_wide = 0;
    setup_screens();

    /* Simulate: cursor at column 100 of a long line (extended).
       updext() places '$' at gbound (position 7) for left overflow. */
    simulate_updext(5, 100);

    /* Verify '$' is at position gbound (7) */
    TEST_ASSERT(vscreen[5]->v_text[7] == '$',
                "updext left overflow: '$' placed at position gbound (7)");

    /* Verify '$' is NOT in the gutter area (positions 0-6) */
    {
        int i;
        int ok = 1;
        for (i = 0; i < 7; i++) {
            if (vscreen[5]->v_text[i] == '$')
                ok = 0;
        }
        TEST_ASSERT(ok,
                    "updext left overflow: no '$' in gutter [0..6]");
    }

    /* Simulate scroll: window re-draws this line (as happens during
       page-down/up when updall re-draws all lines).  The '$' at
       position 7 must be overwritten by text. */
    simulate_updall_line(5, 1);

    /* '$' at position 7 should now be 'X' (text from re-draw), NOT '$' */
    TEST_ASSERT(vscreen[5]->v_text[7] == 'X',
                "after scroll: position 7 overwritten by text ('X'), '$' cleared");
    TEST_ASSERT(vscreen[5]->v_text[7] != '$',
                "after scroll: '$' at gbound is cleared by updall re-draw");

    /* '$' at position 79 (right overflow from text fill) is expected */
    TEST_ASSERT(vscreen[5]->v_text[79] == '$',
                "after scroll: '$' at 79 (right overflow from full-width text)");

    /* Gutter should be properly drawn with spaces */
    {
        int i;
        int ok = 1;
        for (i = 0; i < 4; i++) {
            if (vscreen[5]->v_text[i] != ' ')
                ok = 0;
        }
        TEST_ASSERT(ok,
                    "after scroll: gutter padding is spaces, not '0'");
    }

    /* Line 6 should be untouched by the re-draw of line 5 */
    TEST_ASSERT(line_unchanged(6, 0),
                "after scroll: vscreen[6] untouched by line 5 re-draw");

    teardown_screens();
}

/* ============================================================ */
int main(void)
{
    kana_is_wide = 0;
    printf("=== Screen no-overflow tests ===\n\n");

    test_ascii_no_cross_line();
    test_wide_no_cross_line();
    test_wide_exact_boundary();
    test_linenum_no_gap();
    test_linenum_overflow_no_stale();
    test_vscreen_to_pscreen_sync();
    test_multi_line_isolation();
    test_zero_width_at_boundary();
    test_wide_kana_overflow_isolation();
    test_trailer_cleanup_no_leak();
    test_linenum_exact_fill();
    test_linenum_wide_no_cross_line();
    test_gutter_padding_spaces();
    test_updext_clears_on_scroll();

    printf("\n=== Results: %d passed, %d failed (%d total) ===\n",
           test_passed, test_failed, test_count);
    return test_fails ? 1 : 0;
}
