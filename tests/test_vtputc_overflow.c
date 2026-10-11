/*
 * test_vtputc_overflow.c -- Test vtputc overflow '$' marker with wide chars
 *
 * Replicates the KANJI vtputc from display.c and tests that the '$'
 * overflow marker correctly appears at the last column for both narrow
 * and wide character lines that exceed the terminal width.
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---- Minimal type definitions (from estruct.h / display.c) ---- */
typedef unsigned int Char;

#define MAX_SCREEN_LINES 128
#define MAX_SCREEN_COLUMNS 256
#define TRAILER (-1)

typedef struct VIDEO {
    int v_flag;
    Char v_text[MAX_SCREEN_COLUMNS + 1];
} VIDEO;

typedef struct TERM {
    int t_nrow, t_ncol, t_margin, t_scrsiz;
} TERM;

/* ---- Globals (from edef.h / display.c) ---- */
int gbound = 0;
int lbound = 0;
int taboff = 0;
int vtrow = 0, vtcol = 0;
TERM term;
VIDEO *vscreen[MAX_SCREEN_LINES];

/* kanji.h provides iskanji, isunicode, char_width, is_narrow_kanji, etc.
   It includes ../econfig.h which defines KANJI=1. */
#include "../kanji/kanji.h"
/* Include the char_width/ucswidth implementation directly so the
   test can link without the kanji library. */
#include "../kanji/wcwidth.h"
#include "../kanji/wcwidth.c"

/* ---- Inline copy of display.c vtputc (WITH KANJI path) ---- */
static void
vtputc(Char c)
{
    VIDEO *vp;      /* ptr to line being updated */
    vp = vscreen[vtrow];

    if (vtcol >= term.t_ncol) {
        Char *tp;
        ++vtcol;
        for (tp = &vp->v_text[term.t_ncol-1]; *tp == TRAILER; tp--) ;
        for ( ; tp < &vp->v_text[term.t_ncol-1]; *tp++ = ' ') ;
        vp->v_text[term.t_ncol - 1] = '$';
    } else if (c == '\t') {
        do {
            vtputc(' ');
        } while (((vtcol + taboff)&0x07) != 0);
    } else if (c < 0x20 || c == 0x7F) {
        vtputc('^');
        vtputc(c ^ 0x40);
    } else if (c >= 0x80  && c < 0x100) {
        vtputc('\\');
        vtputc('0'+((c & 0700) >> 6));
        vtputc('0'+((c & 0070) >> 3));
        vtputc('0'+((c & 0007) >> 0));
    } else if (iskanji(c) || isunicode(c)) {
        /* JIS X 0208 or Unicode: use East Asian Width to determine size */
        int w = char_width(c);
        /* On Western-locale terminals, hiragana/katakana are displayed as
           1-column narrow even though char_width() returns 2 (UAX #11 Wide).
           Adjust terminal display width to match actual rendering. */
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
            /* wide character: occupies two columns */
            if (vtcol >= term.t_ncol-1 || vtcol < gbound) {
                /* abandon to display */
                vtputc(' ');
                vtputc(' ');
            } else {
                if (vtcol >= gbound) {
                    /* first col */
                    vp->v_text[vtcol] = c;
                    /* second col */
                    vp->v_text[vtcol+1] = TRAILER;
                }
                vtcol += 2;
            }
        } else if (w == 1) {
            /* narrow: one column */
            if (vtcol >= term.t_ncol || vtcol < gbound) {
                /* abandon to display */
                vtputc(' ');
            } else {
                if (vtcol >= gbound)
                    vp->v_text[vtcol] = c;
                ++vtcol;
            }
        } else {
            /* w == 0: zero-width (combining marks, ZWJ, etc.) */
            if (vtcol >= gbound && vtcol < term.t_ncol)
                vp->v_text[vtcol] = c;
            /* do not advance vtcol */
        }
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

/* ---- Wide character test helpers ---- */

/* A typical wide kanji: e.g. 0x4221 (JIS X 0208 row 42, col 21) */
/* In kemacs internal encoding, kanji is 0x2121-0x7E7E, two-byte EUC-JP with 0x80 stripped */
/* 0x4221 should have char_width = 2 (is_narrow_kanji false for row 0x42) */

/* A wide Unicode character: U+4E2D (Chinese character 中) = 0x4E2D */
/* With UNICODE_MARK prefix: 0x4E2D | 0x200000 = 0x204E2D */
/* isunicode(0x204E2D) = true (>= 0x10000), char_width delegates to ucswidth */

/* U+1F600 (emoji) = 0x1F600, stored directly (>= 0x10000) */

#define WIDE_KANJI  0x4221U    /* JIS X 0208 kanji, 2-column */
#define WIDE_UNI    (0x204E2DU) /* U+4E2D with UNICODE_MARK */
#define WIDE_EMOJI  0x1F600U   /* U+1F600 emoji, 2-column */

/* Hiragana あ (U+3042 -> EUC-JP A4A2 -> JIS 0x2422), row 0x24
   Narrow (1-column) on Western-locale terminals (kana_is_wide = 0).
   Wide (2-column) on CJK-locale terminals (kana_is_wide = 1). */
#define HIRA_A      0x2422U    /* hiragana あ */
/* Katakana ア (U+30A2 -> EUC-JP A5A2 -> JIS 0x2522), row 0x25
   Narrow (1-column) on Western-locale terminals (kana_is_wide = 0).
   Wide (2-column) on CJK-locale terminals (kana_is_wide = 1). */
#define KATA_A      0x2522U    /* katakana ア */

/* ---- Tests ---- */

/* Test 1: 100 ASCII chars on 80-col screen -> '$' at position 79 */
static void test_ascii_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 100; i++)
        vtputc('A');
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "ASCII 100 chars: '$' at last column (79)");
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "ASCII 100 chars: 'A' at position 78 (before '$')");

    teardown_screens();
}

/* Test 2: 41 wide kanji chars on 80-col screen -> '$' at position 79 */
static void test_wide_kanji_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 41; i++)
        vtputc(WIDE_KANJI);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "41 wide kanji chars: '$' at last column (79)");
    /* pos 77 is TRAILER (second half of wide char at 76-77), pos 78 is space (cleared by TRAILER cleanup), pos 79 is '$' */

    printf("  DEBUG: pos[77]=0x%08x pos[78]=0x%08x pos[79]=0x%08x\n",
           (unsigned)vscreen[0]->v_text[77],
           (unsigned)vscreen[0]->v_text[78],
           (unsigned)vscreen[0]->v_text[79]);

    teardown_screens();
}

/* Test 3: 36 wide kanji chars (exactly 72 cols) + 1 narrow on 80-col -> fits */
static void test_wide_then_narrow_exact(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 36; i++)
        vtputc(WIDE_KANJI);
    /* 36 * 2 = 72 columns. Add 8 more narrow to fill exactly 80 */
    for (i = 0; i < 8; i++)
        vtputc('B');
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == 'B',
                "36 wide + 8 narrow: last col (79) is 'B' (exact fit, no overflow)");
    TEST_ASSERT(vscreen[0]->v_text[78] == 'B',
                "36 wide + 8 narrow: pos 78 is 'B'");

    teardown_screens();
}

/* Test 4: 36 wide kanji chars + 9 narrow -> '$' at position 79 */
static void test_wide_then_narrow_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 36; i++)
        vtputc(WIDE_KANJI);
    /* 36 * 2 = 72 columns. Add 9 more narrow -> 81 cols, overflow */
    for (i = 0; i < 9; i++)
        vtputc('B');
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "36 wide + 9 narrow: '$' at last column (79) due to overflow");

    teardown_screens();
}

/* Test 5: Wide char that exactly fills the last two columns (positions 78-79) */
/* 39 narrow + 1 wide: 39 + 2 = 41 cols. Need more... */
/* 76 narrow chars fill 0-75 (vtcol=76). Wide char at 76-77 (vtcol=78). */
/* Narrow at 78 (vtcol=79). Narrow at 79 (vtcol=80). Next -> overflow '$' */
/* But what about wide char at 78-79 as the LAST char? */
/* 78 narrow -> pos 0-77, then wide at 78 -> pos 78=c, 79=TRAILER, vtcol=80 */
/* If this is the last char, vteeol doesn't place '$'. pos[79]=TRAILER. */
static void test_wide_exact_fit_last_col(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 78; i++)
        vtputc('A');
    /* 78 narrow chars fill positions 0-77 (vtcol=78) */
    /* Wide char at position 78: fills 78 and 79, vtcol=80 */
    vtputc(WIDE_KANJI);
    /* No more chars. This fills exactly to position 79. */
    vteeol();

    /* The line is exactly 80 columns: 78 narrow + 1 wide (2 cols) = 80. */
    /* No overflow. But position 79 has TRAILER. */
    TEST_ASSERT(vscreen[0]->v_text[79] == TRAILER,
                "wide char at 78-79 exact fit: pos 79 = TRAILER (no overflow)");
    TEST_ASSERT(vscreen[0]->v_text[78] == WIDE_KANJI,
                "wide char at 78-79 exact fit: pos 78 = wide char");

    printf("  DEBUG: pos[78]=0x%08x pos[79]=0x%08x (TRAILER=0x%08x)\n",
           (unsigned)vscreen[0]->v_text[78],
           (unsigned)vscreen[0]->v_text[79],
           (unsigned)TRAILER);

    teardown_screens();
}

/* Test 6: Wide char at position 78-79, then MORE content -> '$' at 79 */
static void test_wide_at_end_then_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 78; i++)
        vtputc('A');
    /* Wide char at positions 78-79 */
    vtputc(WIDE_KANJI);
    /* One more narrow char -> overflow */
    vtputc('B');
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "wide at 78-79 + more: '$' at last column (79)");

    printf("  DEBUG: pos[77]=0x%08x pos[78]=0x%08x pos[79]=0x%08x\n",
           (unsigned)vscreen[0]->v_text[77],
           (unsigned)vscreen[0]->v_text[78],
           (unsigned)vscreen[0]->v_text[79]);

    teardown_screens();
}

/* Test 7: Wide char at 78-79, then another wide char -> '$' at 79 */
static void test_wide_at_end_then_overflow_wide(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 78; i++)
        vtputc('A');
    /* Wide char at positions 78-79 */
    vtputc(WIDE_KANJI);
    /* Another wide char -> overflow */
    vtputc(WIDE_KANJI);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "wide at 78-79 + wide overflow: '$' at last column (79)");

    teardown_screens();
}

/* Test 8: Wide char starts at position 79 (only 1 col left) -> abandoned, '$' */
static void test_wide_start_at_last_col(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 79; i++)
        vtputc('A');
    /* 79 narrow chars fill positions 0-78 (vtcol=79) */
    /* Wide char at position 79: only 1 col left, can't fit 2-col char */
    vtputc(WIDE_KANJI);
    vteeol();

    /* The wide char should be abandoned, '$' placed at 79 */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "wide at position 79 (not enough room): '$' at last column (79)");

    printf("  DEBUG: pos[78]=0x%08x pos[79]=0x%08x\n",
           (unsigned)vscreen[0]->v_text[78],
           (unsigned)vscreen[0]->v_text[79]);

    teardown_screens();
}

/* Test 9: Line numbers ON, wide chars, overflow -> '$' at last col */
static void test_wide_with_gutter_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 7;  /* 6-digit line number + 1 space */
    setup_screens();

    vtmove(0, gbound);
    int i;
    /* 36 wide chars: 36 * 2 = 72 cols from position 7 -> positions 7-78 */
    /* vtcol after 36 wide chars = 7 + 72 = 79 */
    /* Next wide char at pos 79: only 1 col left -> abandoned -> '$' */
    for (i = 0; i < 37; i++)
        vtputc(WIDE_KANJI);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "wide chars with gutter: '$' at last column (79)");

    teardown_screens();
}

/* Test 10: TRAILER from wide char at 78-79, then vteeol doesn't clear it */
/* This is the exact-fit case - no '$' should appear */
static void test_pure_wide_exact_80(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    /* 40 wide chars: 40 * 2 = 80 cols, exactly fills */
    for (i = 0; i < 40; i++)
        vtputc(WIDE_KANJI);
    vteeol();

    /* Should NOT have '$' - line fills exactly */
    printf("  DEBUG: pos[78]=0x%08x pos[79]=0x%08x (TRAILER=0x%08x)\n",
           (unsigned)vscreen[0]->v_text[78],
           (unsigned)vscreen[0]->v_text[79],
           (unsigned)TRAILER);
    TEST_ASSERT(vscreen[0]->v_text[79] == TRAILER,
                "40 wide chars exact fit: pos 79 = TRAILER (no overflow '$')");

    teardown_screens();
}

/* Test 11: 41 wide chars -> '$' after TRAILER cleanup */
static void test_pure_wide_overflow_41(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    /* 41 wide chars: 82 cols, overflow at char 41 */
    for (i = 0; i < 41; i++)
        vtputc(WIDE_KANJI);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "41 wide chars overflow: '$' at last column (79)");
    /* Check TRAILER was cleaned up */
    TEST_ASSERT(vscreen[0]->v_text[78] != TRAILER,
                "41 wide chars overflow: TRAILER cleaned from position 78");

    teardown_screens();
}

/* Test 12: char_width sanity check */
static void test_char_width(void)
{
    /* ASCII */
    TEST_ASSERT(char_width('A') == 1, "char_width('A') == 1");
    /* JIS kanji (should be wide, 2) */
    TEST_ASSERT(char_width(WIDE_KANJI) == 2, "char_width(0x4221) == 2 (wide kanji)");
    /* Unicode emoji */
    TEST_ASSERT(char_width(WIDE_EMOJI) == 2, "char_width(0x1F600) == 2 (emoji)");
    /* Unicode Chinese char */
    TEST_ASSERT(char_width(WIDE_UNI) == 2, "char_width(0x204E2D) == 2 (Chinese)");
    /* char_width() returns canonical UAX #11 width: kana are WIDE (2).
       vtputc() adjusts to 1-column for Western-locale terminal display
       using kana_is_wide; char_width() itself always returns 2 for kana. */
    TEST_ASSERT(char_width(HIRA_A) == 2, "char_width(0x2422 hiragana) == 2 (UAX #11 Wide)");
    TEST_ASSERT(char_width(KATA_A) == 2, "char_width(0x2522 katakana) == 2 (UAX #11 Wide)");
    /* Tagged Unicode kana also Wide (2) per UAX #11 */
    TEST_ASSERT(char_width(0x3042 | 0x200000) == 2, "char_width(U+3042 tagged) == 2 (Wide)");
    TEST_ASSERT(char_width(0x30A2 | 0x200000) == 2, "char_width(U+30A2 tagged) == 2 (Wide)");
}

/* Test 13: 80 hiragana on 80-col screen -> exact fit, no '$' (80 * 1 = 80)
 * Western-locale terminals render kana as 1-column narrow */
static void test_hiragana_exact_fit_80(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    /* 80 narrow kana = 80 columns, exactly fills positions 0-79 */
    for (i = 0; i < 80; i++)
        vtputc(HIRA_A);
    vteeol();

    /* No overflow: line fills exactly. Position 79 should be HIRA_A */
    TEST_ASSERT(vscreen[0]->v_text[79] == HIRA_A,
                "80 hiragana exact fit: pos 79 = HIRA_A (no overflow '$')");

    teardown_screens();
}

/* Test 14: 81 hiragana on 80-col screen -> '$' at position 79 (81 * 1 = 81) */
static void test_hiragana_overflow_81(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    /* 81 narrow kana = 81 columns, overflow at char 81 */
    for (i = 0; i < 81; i++)
        vtputc(HIRA_A);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "81 hiragana overflow: '$' at last column (79)");
    /* 80 kana fill pos 0-79. 81st triggers overflow:
       vtcol=80 >= 80, '$' at 79 replaces last HIRA_A. */
    TEST_ASSERT(vscreen[0]->v_text[78] != TRAILER,
                "81 hiragana overflow: no TRAILER at position 78 (narrow kana)");

    teardown_screens();
}

/* Test 15: 81 katakana on 80-col screen -> '$' at position 79 (81 * 1 = 81) */
static void test_katakana_overflow_81(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    /* 81 narrow kana = 81 columns, overflow */
    for (i = 0; i < 81; i++)
        vtputc(KATA_A);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "81 katakana overflow: '$' at last column (79)");

    teardown_screens();
}

/* Test 16: Mixed ASCII + hiragana overflow -> '$' at position 79
   20 kana (20 cols) + 60 ASCII (60 cols) = 80 cols exact fit, 1 more = overflow */
static void test_mixed_ascii_hiragana_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    /* 20 narrow kana = 20 cols, fills positions 0-19 */
    for (i = 0; i < 20; i++)
        vtputc(HIRA_A);
    /* 60 ASCII = 60 cols, fills positions 20-79. vtcol=80. */
    for (i = 0; i < 60; i++)
        vtputc('A');
    /* Now vtcol=80. Next char overflows -> '$' at pos 79 */
    vtputc('B');
    vteeol();

    /* 20 cols (kana) + 60 cols (ASCII) = 80 cols, full line.
       'B' triggers overflow: vtcol=80 >= 80, '$' at 79 replaces last 'A'. */
    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "80 cols (20 kana + 60 ASCII) + overflow -> '$' at last column");
    TEST_ASSERT(vscreen[0]->v_text[78] == 'A',
                "80 cols: position 78 = ASCII 'A' (before '$')");

    teardown_screens();
}

/* Test 17: 74 katakana with gutter (gbound=7) -> '$' when overflow
   73 kana * 1 = 73 cols from pos 7 -> positions 7-79. 74th triggers overflow. */
static void test_katakana_with_gutter_overflow(void)
{
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 7;  /* 6-digit line number + 1 space */
    setup_screens();

    vtmove(0, gbound);
    int i;
    /* 73 narrow kana: 73 * 1 = 73 cols from position 7 -> positions 7-79 */
    for (i = 0; i < 73; i++)
        vtputc(KATA_A);
    /* 74th kana at vtcol=80: overflow -> '$' at pos 79 */
    vtputc(KATA_A);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "74 katakana with gutter: '$' at last column (79)");

    teardown_screens();
}

/* ---- Wide kana tests (CJK locale: kana_is_wide = 1) ---- */

/* Wide hiragana: 40 * 2 = 80 cols exact fit */
static void test_wide_hiragana_exact_fit(void)
{
    kana_is_wide = 1;
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 40; i++)
        vtputc(HIRA_A);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == TRAILER,
                "40 wide hiragana exact fit: pos 79 = TRAILER (no overflow)");
    TEST_ASSERT(vscreen[0]->v_text[78] == HIRA_A,
                "40 wide hiragana exact fit: pos 78 = HIRA_A");

    teardown_screens();
    kana_is_wide = 0;
}

/* Wide katakana: 41 * 2 = 82 cols overflow */
static void test_wide_katakana_overflow(void)
{
    kana_is_wide = 1;
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 0;
    setup_screens();

    vtmove(0, 0);
    int i;
    for (i = 0; i < 41; i++)
        vtputc(KATA_A);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "41 wide katakana overflow: '$' at last column (79)");
    TEST_ASSERT(vscreen[0]->v_text[78] != TRAILER,
                "41 wide katakana overflow: TRAILER cleaned from 78");

    teardown_screens();
    kana_is_wide = 0;
}

/* Wide hiragana with gutter: 36 * 2 = 72 cols from pos 7 → positions 7-78.
   37th at pos 79: only 1 col left, can't fit 2-col char → '$' */
static void test_wide_katakana_with_gutter(void)
{
    kana_is_wide = 1;
    term.t_nrow = 24; term.t_ncol = 80;
    gbound = 7;
    setup_screens();

    vtmove(0, gbound);
    int i;
    for (i = 0; i < 36; i++)
        vtputc(KATA_A);
    vtputc(KATA_A);
    vteeol();

    TEST_ASSERT(vscreen[0]->v_text[79] == '$',
                "37 wide katakana with gutter: '$' at last column (79)");

    teardown_screens();
    kana_is_wide = 0;
}

int main(void)
{
    /* Default to Western-locale behavior: kana rendered as 1-column narrow.
       Wide-kana tests set kana_is_wide = 1 explicitly. */
    kana_is_wide = 0;

    printf("=== vtputc overflow tests (KANJI/wide-char path) ===\n\n");

    test_char_width();
    test_ascii_overflow();
    test_wide_kanji_overflow();
    test_wide_then_narrow_exact();
    test_wide_then_narrow_overflow();
    test_wide_exact_fit_last_col();
    test_wide_at_end_then_overflow();
    test_wide_at_end_then_overflow_wide();
    test_wide_start_at_last_col();
    test_wide_with_gutter_overflow();
    test_pure_wide_exact_80();
    test_pure_wide_overflow_41();
    /* Western-locale (narrow kana) tests */
    test_hiragana_exact_fit_80();
    test_hiragana_overflow_81();
    test_katakana_overflow_81();
    test_mixed_ascii_hiragana_overflow();
    test_katakana_with_gutter_overflow();
    /* CJK-locale (wide kana) tests */
    test_wide_hiragana_exact_fit();
    test_wide_katakana_overflow();
    test_wide_katakana_with_gutter();

    printf("\n=== Results: %d passed, %d failed (%d total) ===\n",
           test_passed, test_failed, test_count);
    return test_fails ? 1 : 0;
}
