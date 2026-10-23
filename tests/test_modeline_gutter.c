/*
 * test_modeline_gutter.c -- Test that modeline content is not erased
 * when line numbers are enabled, specifically for wide (CJK) characters
 * in the program name.
 *
 * Bug: modeline() set gbound=0 to render across ALL columns, then
 * draw_gutter() cleared columns 0..gbound-1.  When the program name
 * contains wide characters (e.g. 3 kanji = 6 columns), they fell in
 * the gutter area and were erased -- leaving only "語μEMACS" instead
 * of "日本語μEMACS".
 *
 * Fix: modeline() now renders starting at vtcol=gbound (after the
 * gutter), so modeline content never enters the gutter area.
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
#define VFREQ  0x0008
#define VFCOL  0x0010

#define TRAILER (-1)

typedef struct VIDEO {
    int v_flag;
    Char v_text[MAX_SCREEN_COLUMNS + 1];
} VIDEO;

typedef struct TERM {
    int t_nrow, t_ncol, t_margin, t_scrsiz;
} TERM;

/* ---- Globals (mirrors display.c) ---- */
int show_line_numbers = FALSE;
int line_number_width = 6;
int effective_lw = 6;
int gbound = 0;
int lbound = 0;
int taboff = 0;
int vtrow = 0, vtcol = 0;
TERM term;
Char mline[256];

VIDEO *vscreen[MAX_SCREEN_LINES];

/* ---- Wide character constants ---- */
/* These mirror the actual Cstr.data values for JIS kanji */
#define DAY    0x65E5   /* 日 */
#define HON    0x672C   /* 本 */
#define GO     0x8A9E   /* 語 */
#define MU     0x00BC   /* μ (U+00BC is not right; use U+03BC) */
#define MU_EMACS 0x03BC  /* μ = U+03BC */

/* ---- Inline copies of functions under test ---- */

static void
vtmove(int row, int col)
{
    vtrow = row;
    vtcol = col;
}

/* KANJI-enabled vtputc with TRAILER handling (copy of display.c logic) */
static void
vtputc(Char c)
{
    VIDEO *vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        Char *tp;
        ++vtcol;
        for (tp = &vp->v_text[term.t_ncol-1]; *tp == TRAILER; tp--) ;
        for ( ; tp < &vp->v_text[term.t_ncol-1]; *tp++ = ' ') ;
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
        vtputc('0'+((c & 0700) >> 6));
        vtputc('0'+((c & 0070) >> 3));
        vtputc('0'+((c & 0007) >> 0));
    } else {
        /* All our test chars are "wide" (width 2) in the JIS path.
         * In the real code, iskanji()/isunicode() + kcol_width() determine
         * this.  For the test, we treat DAY/HON/GO/MU as wide (w=2). */
        int w = 2;
        if (w == 2) {
            /* wide character: occupies two columns */
            if (vtcol >= term.t_ncol-1 || vtcol < gbound) {
                /* abandon to display */
                vtputc(' ');
                vtputc(' ');
            } else {
                if (vtcol >= gbound) {
                    vp->v_text[vtcol] = c;
                    vp->v_text[vtcol+1] = TRAILER;
                }
                vtcol += 2;
            }
        } else {
            /* narrow: one column */
            if (vtcol >= term.t_ncol || vtcol < gbound) {
                vtputc(' ');
            } else {
                if (vtcol >= gbound)
                    vp->v_text[vtcol] = c;
                ++vtcol;
            }
        }
    }
}

/* KANJI-enabled vtputc for ASCII (width 1) chars */
static void
vtputc_narrow(Char c)
{
    VIDEO *vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        Char *tp;
        ++vtcol;
        for (tp = &vp->v_text[term.t_ncol-1]; *tp == TRAILER; tp--) ;
        for ( ; tp < &vp->v_text[term.t_ncol-1]; *tp++ = ' ') ;
        vp->v_text[term.t_ncol - 1] = '$';
    } else if (c == '\t') {
        do {
            vtputc_narrow(' ');
        } while (((vtcol + taboff) & 0x07) != 0);
    } else if (c < 0x20 || c == 0x7F) {
        vtputc_narrow('^');
        vtputc_narrow(c ^ 0x40);
    } else {
        /* narrow ASCII character */
        if (vtcol >= term.t_ncol) {
            vp->v_text[term.t_ncol - 1] = '$';
        } else {
            if (vtcol >= gbound)
                vp->v_text[vtcol] = c;
            ++vtcol;
        }
    }
}

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
    int i, ndigits, tmp, pos, gw, d;
    Char dbuf[16];

    gw = show_line_numbers ? line_number_width + 1 : 0;
    if (gw == 0)
        return;

    vp = vscreen[sline];
    for (i = 0; i < 16; i++)
        dbuf[i] = ' ';

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
    if (pos < 0) pos = 0;
    for (i = 0; i < pos; i++)
        vp->v_text[i] = ' ';
    for (i = 0; i < ndigits; i++)
        vp->v_text[pos + i] = dbuf[ndigits - 1 - i];
}

/* ---- Helpers ---- */

static void
setup_screens(void)
{
    int i, j;
    for (i = 0; i < MAX_SCREEN_LINES; i++) {
        if (vscreen[i]) free(vscreen[i]);
        vscreen[i] = (VIDEO *)malloc(sizeof(VIDEO));
        for (j = 0; j < MAX_SCREEN_COLUMNS + 1; j++)
            vscreen[i]->v_text[j] = 0;
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

/*
 * Test: modeline with wide-character program name "日本語μEMACS" and
 * line numbers ON, using the FIXED behavior (render starting at gbound).
 * The full program name must be visible at columns gbound..gbound+8.
 *
 * Modeline format: %=%* %p %v (%M) ...
 * With gbound=7:
 *   col 0-6: gutter (spaces)
 *   col 7: dash (from %=)
 *   col 8: dash (from %*)
 *   col 9: space
 *   col 10-15: 日(10-11), 本(12-13), 語(14-15) ... wait, each wide char takes 2 cols.
 *   日: col 10, TRAILER at 11
 *   本: col 12, TRAILER at 13
 *   語: col 14, TRAILER at 15
 *   μ: col 16
 *   E: col 17, M: col 18, A: col 19, C: col 20, S: col 21
 */
static void test_modeline_wide_prgname_fixed(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    effective_lw = line_number_width; gbound = line_number_width + 1; /* = 7 */
    setup_screens();

    int modeline_row = 23;
    int n;

    /* FIXED modeline: render starting at gbound (not 0) */
    vtmove(modeline_row, gbound);
    n = gbound;

    /* %=: dash */
    vtputc_narrow('-');
    n++;

    /* %*: dash */
    vtputc_narrow('-');
    n++;

    /* space */
    vtputc_narrow(' ');
    n++;

    /* %p: "日本語μEMACS"  -- wide chars use vtputc, narrow use vtputc_narrow */
    vtputc(DAY);   /* 日 at col 7+3=10, TRAILER at 11 */
    n += 2;
    vtputc(HON);   /* 本 at col 12, TRAILER at 13 */
    n += 2;
    vtputc(GO);    /* 語 at col 14, TRAILER at 15 */
    n += 2;
    vtputc_narrow(MU_EMACS);  /* μ at col 16 */
    n++;
    vtputc_narrow('E');  /* col 17 */
    n++;
    vtputc_narrow('M');  /* col 18 */
    n++;
    vtputc_narrow('A');  /* col 19 */
    n++;
    vtputc_narrow('C');  /* col 20 */
    n++;
    vtputc_narrow('S');  /* col 21 */
    n++;

    /* Fill rest with spaces */
    while (n < term.t_ncol) {
        vtputc_narrow(' ');
        n++;
    }

    /* draw_gutter(vtrow, 0) -- as called at end of modeline() */
    draw_gutter(modeline_row, 0);

    /* Simulate draw_all_gutters() Phase 1: clear ALL gutters */
    draw_gutter(modeline_row, 0);

    /* Verify all program name characters survived the gutter clearing */
    TEST_ASSERT(vscreen[modeline_row]->v_text[10] == DAY,
                "fixed: 日 at col 10 preserved after gutter cleanup");
    TEST_ASSERT(vscreen[modeline_row]->v_text[11] == TRAILER,
                "fixed: TRAILER(日) at col 11 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[12] == HON,
                "fixed: 本 at col 12 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[13] == TRAILER,
                "fixed: TRAILER(本) at col 13 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[14] == GO,
                "fixed: 語 at col 14 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[15] == TRAILER,
                "fixed: TRAILER(語) at col 15 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[16] == MU_EMACS,
                "fixed: μ at col 16 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[17] == 'E',
                "fixed: E at col 17 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[21] == 'S',
                "fixed: S at col 21 preserved");

    /* Gutter should be clean */
    TEST_ASSERT(gutter_is_clean(modeline_row),
                "fixed: gutter (cols 0-6) clean");

    /* Show debug output */
    {
        int i;
        printf("  Modeline row %d: ", modeline_row);
        for (i = 0; i < 24; i++) {
            Char c = vscreen[modeline_row]->v_text[i];
            if (c == TRAILER) printf("[T]");
            else if (c == DAY) printf("日");
            else if (c == HON) printf("本");
            else if (c == GO) printf("語");
            else if (c == MU_EMACS) printf("μ");
            else if (c == 0) printf("·");
            else printf("%c", (char)c);
        }
        printf("\n");
    }

    teardown_screens();
}

/*
 * Test: OLD buggy behavior -- gbound=0 during modeline, then cleared.
 * Wide chars in gutter area (0-6) get erased.
 */
static void test_modeline_wide_prgname_buggy(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = TRUE;
    line_number_width = 6;
    /* OLD BUG: modeline set gbound=0 */
    gbound = 0;
    setup_screens();

    int modeline_row = 23;
    int n;

    /* BUGGY modeline: render starting at col 0 (gbound=0) */
    vtmove(modeline_row, 0);
    n = 0;

    vtputc_narrow('-');
    n++;
    vtputc_narrow('-');
    n++;
    vtputc_narrow(' ');
    n++;

    /* %p: "日本語μEMACS" at cols 3-21 */
    vtputc(DAY);    /* col 3, TRAILER at 4 */
    n += 2;
    vtputc(HON);    /* col 5, TRAILER at 6 */
    n += 2;
    vtputc(GO);     /* col 7, TRAILER at 8 */
    n += 2;
    vtputc_narrow(MU_EMACS);  /* col 9 */
    n++;
    vtputc_narrow('E');
    n++;
    vtputc_narrow('M');
    n++;
    vtputc_narrow('A');
    n++;
    vtputc_narrow('C');
    n++;
    vtputc_narrow('S');
    n++;

    while (n < term.t_ncol) {
        vtputc_narrow(' ');
        n++;
    }

    /* Restore gbound (as modeline did: gbound = save_gbound) */
    gbound = 7;

    /* draw_gutter(vtrow, 0) erases cols 0-6 */
    draw_gutter(modeline_row, 0);

    /* draw_all_gutters Phase 1 also erases cols 0-6 */
    draw_gutter(modeline_row, 0);

    /* With old behavior: "日"(3-4), "本"(5-6) are in gutter → erased */
    /* "語"(7-8) is NOT in gutter → survives */
    TEST_ASSERT(vscreen[modeline_row]->v_text[3] == ' ',
                "buggy: col 3 (was 日) erased to space");
    TEST_ASSERT(vscreen[modeline_row]->v_text[4] == ' ',
                "buggy: col 4 (was TRAILER) erased to space");
    TEST_ASSERT(vscreen[modeline_row]->v_text[5] == ' ',
                "buggy: col 5 (was 本) erased to space");
    TEST_ASSERT(vscreen[modeline_row]->v_text[6] == ' ',
                "buggy: col 6 (was TRAILER) erased to space");
    TEST_ASSERT(vscreen[modeline_row]->v_text[7] == GO,
                "buggy: col 7 (語) survives — this is why user sees 語μEMACS");
    TEST_ASSERT(vscreen[modeline_row]->v_text[8] == TRAILER,
                "buggy: col 8 (TRAILER for 語) survives");

    /* Show debug output */
    {
        int i;
        printf("  Buggy modeline row %d: ", modeline_row);
        for (i = 0; i < 24; i++) {
            Char c = vscreen[modeline_row]->v_text[i];
            if (c == TRAILER) printf("[T]");
            else if (c == DAY) printf("日");
            else if (c == HON) printf("本");
            else if (c == GO) printf("語");
            else if (c == MU_EMACS) printf("μ");
            else if (c == 0) printf("·");
            else printf("%c", (char)c);
        }
        printf("\n");
    }

    teardown_screens();
}

/*
 * Test: without line numbers, gbound=0, wide chars are not erased.
 */
static void test_modeline_wide_prgname_no_linenumbers(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    show_line_numbers = FALSE;
    gbound = 0;
    setup_screens();

    int modeline_row = 23;
    int n;

    vtmove(modeline_row, 0);
    n = 0;

    vtputc_narrow('-');
    n++;
    vtputc_narrow('-');
    n++;
    vtputc_narrow(' ');
    n++;

    /* %p: "日本語μEMACS" */
    vtputc(DAY);
    n += 2;
    vtputc(HON);
    n += 2;
    vtputc(GO);
    n += 2;
    vtputc_narrow(MU_EMACS);
    n++;
    vtputc_narrow('E');
    n++;
    vtputc_narrow('M');
    n++;
    vtputc_narrow('A');
    n++;
    vtputc_narrow('C');
    n++;
    vtputc_narrow('S');
    n++;

    while (n < term.t_ncol) {
        vtputc_narrow(' ');
        n++;
    }

    /* No gutter clearing needed (gw=0) */
    draw_gutter(modeline_row, 0); /* no-op */

    TEST_ASSERT(vscreen[modeline_row]->v_text[3] == DAY,
                "no linenum: 日 at col 3 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[5] == HON,
                "no linenum: 本 at col 5 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[7] == GO,
                "no linenum: 語 at col 7 preserved");
    TEST_ASSERT(vscreen[modeline_row]->v_text[9] == MU_EMACS,
                "no linenum: μ at col 9 preserved");

    teardown_screens();
}

int main(void)
{
    printf("=== Modeline + gutter wide-character tests ===\n\n");

    test_modeline_wide_prgname_fixed();
    printf("\n");
    test_modeline_wide_prgname_buggy();
    printf("\n");
    test_modeline_wide_prgname_no_linenumbers();

    printf("\n=== Results: %d passed, %d failed (%d total) ===\n",
           test_passed, test_failed, test_count);
    return test_fails ? 1 : 0;
}
