/*
 * test_search.c -- Unit tests for kemacs regex/matching functions.
 *
 * Tests: scanner, amatch, mceq, eq
 *
 * search.c is included directly after defining maindef (to provide
 * global-variable definitions from edef.h) and static (to expose the
 * static functions amatch, mceq, eq, boundry, nextch, etc.).
 * External dependencies are satisfied by minimal stubs defined below.
 *
 * Linking: ../Css.o (for Cxstr), ../Cstrings/Cstrings.a (for Cstrcpy etc.)
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>

/* Stubs for functions NOT declared in edef.h (implicit declarations in
   search.c).  Empty-param declarations match the K&R implicit style. */
int mlwrite();
int mlrepl();
int mlrept();
int mlyesno();
int backchar();
int forwchar();
int linsert();
int ldelete();
int lnewline();
int rdonly();
int update();

/* ================================================================ */
/*  Include search.c with maindef (globals defined) + static exposed */
/* ================================================================ */
#define maindef
#define static
#include "../search.c"
#undef static
#undef maindef

/* ================================================================ */
/*  Stub implementations                                            */
/* ================================================================ */
/* Note: tgetc IS declared in edef.h as 'Char tgetc()', so the stub
   must match.  All other stubs are for functions not declared in
   edef.h (implicit declarations in search.c).                    */
int mlwrite()        { return TRUE; }
int mlrepl()         { return TRUE; }
int mlrept()         { return TRUE; }
int mlyesno()        { return TRUE; }
int backchar()       { return TRUE; }
int forwchar()       { return TRUE; }
int linsert()        { return TRUE; }
int ldelete()        { return TRUE; }
int lnewline()       { return TRUE; }
int rdonly()         { return FALSE; }
int update()         { return TRUE; }
Char tgetc()         { return 0; }

/* term is declared 'extern TERM term;' in edef.h (#ifndef termdef)
   and referenced by replaces(); provide a zero-init definition. */
TERM term;

/* mlreplyt is used under #if MAGIC, not declared in edef.h */
int mlreplyt()       { return TRUE; }

/* ================================================================ */
/*  Test fixtures                                                    */
/* ================================================================ */

static LINE *g_line = NULL;
static LINE *g_hdr = NULL;

static LINE *alloc_test_line(int capacity)
{
    /* Allocate a LINE with room for 'capacity' chars (like lalloc) */
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

static void cleanup_test_lines(void)
{
    if (g_hdr == NULL)
        return;
    /* Walk circular list and free all lines */
    LINE *cur = g_hdr->l_fp;
    while (cur != g_hdr && cur != NULL) {
        LINE *next = cur->l_fp;
        free((char *)cur);
        cur = next;
    }
    free((char *)g_hdr);
    g_hdr = NULL;
    g_line = NULL;
}

static void setup_one_line(const char *text)
{
    int i, len = strlen(text);
    LINE *hdr, *lp;

    cleanup_test_lines();

    /* Allocate header and text line */
    hdr = alloc_test_line(0);
    lp  = alloc_test_line(len + 1);
    g_hdr = hdr;
    g_line = lp;

    for (i = 0; i < len; i++)
        lp->l_text[i] = (unsigned char)text[i];
    lp->l_used = len;

    /* Circular double-linked list: hdr <-> lp */
    hdr->l_fp = lp;   hdr->l_bp = lp;
    lp->l_fp  = hdr; lp->l_bp  = hdr;

    /* curbp and curwp are defined as NULL by edef.h with INIT(NULL).
       Allocate and initialize them for the test. */
    if (curbp == NULL) {
        curbp = (BUFFER *)calloc(1, sizeof(BUFFER) + sizeof(Char)*NSTRING);
    }
    if (curwp == NULL) {
        curwp = (WINDOW *)calloc(1, sizeof(WINDOW));
    }
    if (wheadp == NULL) {
        wheadp = curwp;
    }

    curbp->b_linep = hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = MDEXACT;   /* exact matching -> no ctab/adjcase */

    curwp->w_bufp  = curbp;
    curwp->w_dotp  = lp;
    curwp->w_doto  = 0;
    curwp->w_flag  = 0;
    wheadp = curwp;
    curwp->w_wndp  = NULL;

    bheadp = curbp;
    curbp->b_bufp = NULL;

    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;

    /* Reset match globals */
    matchlen = 0;
    matchoff = 0;
    matchline = NULL;
    patmatch = NULL;
}

/* ================================================================ */
/*  scanner tests                                                    */
/* ================================================================ */

static void test_scanner_found(void)
{
    setup_one_line("hello world");

    pat[0] = 'w'; pat[1] = 'o'; pat[2] = 'r'; pat[3] = 'l';
    pat[4] = 'd'; pat[5] = '\0';

    int rc = scanner(pat, FORWARD, PTBEG);
    TEST_ASSERT_INT(rc, TRUE, "scanner finds 'world' from start");
    TEST_ASSERT_INT(curwp->w_doto, 6, "cursor at start of match (PTBEG)");
    TEST_ASSERT(curwp->w_dotp == g_line, "cursor on right line");

    /* Search from offset 3, should still find it */
    setup_one_line("hello world");
    curwp->w_doto = 3;
    rc = scanner(pat, FORWARD, PTBEG);
    TEST_ASSERT_INT(rc, TRUE, "scanner finds 'world' from offset 3");
    TEST_ASSERT_INT(curwp->w_doto, 6, "cursor at start from offset 3");
}

static void test_scanner_found_ptend(void)
{
    setup_one_line("hello world");

    pat[0] = 'w'; pat[1] = 'o'; pat[2] = 'r'; pat[3] = 'l';
    pat[4] = 'd'; pat[5] = '\0';

    int rc = scanner(pat, FORWARD, PTEND);
    TEST_ASSERT_INT(rc, TRUE, "scanner PTEND finds 'world'");
    TEST_ASSERT_INT(curwp->w_doto, 11, "cursor at end of match (PTEND)");
}

static void test_scanner_not_found(void)
{
    setup_one_line("hello world");

    pat[0] = 'x'; pat[1] = 'y'; pat[2] = 'z'; pat[3] = '\0';

    int rc = scanner(pat, FORWARD, PTBEG);
    TEST_ASSERT_INT(rc, FALSE, "scanner fails for non-existent pattern");
}

static void test_scanner_empty_pattern(void)
{
    setup_one_line("hello");

    pat[0] = '\0';
    int rc = scanner(pat, FORWARD, PTBEG);
    TEST_ASSERT_INT(rc, FALSE, "empty pattern should fail");
}

static void test_scanner_at_start(void)
{
    setup_one_line("hello");

    pat[0] = 'h'; pat[1] = 'e'; pat[2] = 'l'; pat[3] = 'l';
    pat[4] = 'o'; pat[5] = '\0';

    int rc = scanner(pat, FORWARD, PTEND);
    TEST_ASSERT_INT(rc, TRUE, "scanner finds pattern at start");
    TEST_ASSERT_INT(curwp->w_doto, 5, "cursor at end of 'hello' (PTEND)");
}

static void test_scanner_single_char(void)
{
    /* Multi-occurrence: should find first occurrence */
    setup_one_line("hello");

    pat[0] = 'l'; pat[1] = '\0';

    int rc = scanner(pat, FORWARD, PTBEG);
    TEST_ASSERT_INT(rc, TRUE, "scanner finds single char 'l'");
    TEST_ASSERT_INT(curwp->w_doto, 2, "cursor at first 'l'");
}

/* ================================================================ */
/*  amatch tests                                                     */
/* ================================================================ */

static void test_amatch_match(void)
{
    setup_one_line("hello");

    LINE *cl = curwp->w_dotp;
    int co = 0;

    mcpat[0].mc_type = LITCHAR; mcpat[0].u.lchar = 'h';
    mcpat[1].mc_type = LITCHAR; mcpat[1].u.lchar = 'e';
    mcpat[2].mc_type = LITCHAR; mcpat[2].u.lchar = 'l';
    mcpat[3].mc_type = MCNIL;

    matchlen = 0;
    int rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, TRUE, "amatch succeeds for 'hel' in 'hello'");
    TEST_ASSERT_INT(co, 3, "cursor advanced to end of match");
    TEST_ASSERT_INT(matchlen, 3, "matchlen recorded");
}

static void test_amatch_fail(void)
{
    setup_one_line("hello");

    LINE *cl = curwp->w_dotp;
    int co = 0;

    mcpat[0].mc_type = LITCHAR; mcpat[0].u.lchar = 'h';
    mcpat[1].mc_type = LITCHAR; mcpat[1].u.lchar = 'X';  /* mismatch */
    mcpat[2].mc_type = MCNIL;

    int rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, FALSE, "amatch fails on 2nd char mismatch");
}

static void test_amatch_any(void)
{
    setup_one_line("hello");

    LINE *cl = curwp->w_dotp;
    int co = 1;  /* start at 'e' */

    mcpat[0].mc_type = ANY;
    mcpat[1].mc_type = LITCHAR; mcpat[1].u.lchar = 'l';
    mcpat[2].mc_type = LITCHAR; mcpat[2].u.lchar = 'l';
    mcpat[3].mc_type = LITCHAR; mcpat[3].u.lchar = 'o';
    mcpat[4].mc_type = MCNIL;

    matchlen = 0;
    int rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, TRUE, "amatch with ANY matches 'ello'");
    TEST_ASSERT_INT(matchlen, 4, "ANY consumed 4 chars");
}

static void test_amatch_bol(void)
{
    setup_one_line("hello");

    LINE *cl = curwp->w_dotp;
    int co = 0;

    mcpat[0].mc_type = BOL;
    mcpat[1].mc_type = LITCHAR; mcpat[1].u.lchar = 'h';
    mcpat[2].mc_type = MCNIL;

    matchlen = 0;
    int rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, TRUE, "BOL+match at position 0");

    /* BOL should fail at non-zero offset */
    co = 1;
    rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, FALSE, "BOL fails at non-zero offset");
}

static void test_amatch_eol(void)
{
    setup_one_line("hello");

    LINE *cl = curwp->w_dotp;
    /* 'o' is at position 4 in "hello" (h=0,e=1,l=2,l=3,o=4) */
    int co = 4;

    mcpat[0].mc_type = LITCHAR; mcpat[0].u.lchar = 'o';
    mcpat[1].mc_type = EOL;
    mcpat[2].mc_type = MCNIL;

    matchlen = 0;
    int rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, TRUE, "match 'o' then EOL at line end");
}

static void test_amatch_partial_fail(void)
{
    setup_one_line("hello");

    LINE *cl = curwp->w_dotp;
    int co = 0;

    mcpat[0].mc_type = LITCHAR; mcpat[0].u.lchar = 'h';
    mcpat[1].mc_type = LITCHAR; mcpat[1].u.lchar = 'e';
    mcpat[2].mc_type = LITCHAR; mcpat[2].u.lchar = 'X';
    mcpat[3].mc_type = MCNIL;

    int rc = amatch(mcpat, FORWARD, &cl, &co);
    TEST_ASSERT_INT(rc, FALSE, "amatch fails on 3rd char mismatch");
}

/* ================================================================ */
/*  mceq tests                                                       */
/* ================================================================ */

static void test_mceq_litchar(void)
{
    MC mc;
    mc.mc_type = LITCHAR; mc.u.lchar = 'a';
    TEST_ASSERT_INT(mceq('a', &mc), TRUE, "mceq LITCHAR exact match");
    TEST_ASSERT_INT(mceq('b', &mc), FALSE, "mceq LITCHAR mismatch");
}

static void test_mceq_any(void)
{
    MC mc;
    mc.mc_type = ANY;
    TEST_ASSERT_INT(mceq('x', &mc), TRUE, "mceq ANY matches any char");
    TEST_ASSERT_INT(mceq('a', &mc), TRUE, "mceq ANY matches 'a'");
    TEST_ASSERT_INT(mceq('\n', &mc), FALSE, "mceq ANY rejects newline");
}

static void test_mceq_case_exact(void)
{
    /* With MDEXACT: no case folding */
    setup_one_line("Hello");
    MC mc;
    mc.mc_type = LITCHAR; mc.u.lchar = 'H';
    TEST_ASSERT_INT(mceq('H', &mc), TRUE, "exact match H");
    TEST_ASSERT_INT(mceq('h', &mc), FALSE, "exact no match h vs H");
}

static void test_mceq_case_fold(void)
{
    /* Without MDEXACT: case folding via eq/adjcase */
    setup_one_line("Hello");
    curbp->b_mode = 0;  /* clear MDEXACT */
    MC mc;
    mc.mc_type = LITCHAR; mc.u.lchar = 'H';
    TEST_ASSERT_INT(mceq('h', &mc), TRUE, "case-fold match h vs H");
    TEST_ASSERT_INT(mceq('H', &mc), TRUE, "case-fold match H vs H");
    TEST_ASSERT_INT(mceq('x', &mc), FALSE, "case-fold mismatch x vs H");
}

/* ================================================================ */
/*  eq tests                                                         */
/* ================================================================ */

static void test_eq_exact(void)
{
    setup_one_line("test");
    /* MDEXACT set in setup */
    TEST_ASSERT_INT(eq('a', 'a'), TRUE, "eq exact: a==a");
    TEST_ASSERT_INT(eq('a', 'b'), FALSE, "eq exact: a!=b");
}

static void test_eq_casefold(void)
{
    setup_one_line("test");
    curbp->b_mode = 0;  /* clear MDEXACT */
    TEST_ASSERT_INT(eq('a', 'A'), TRUE, "eq fold: a==A");
    TEST_ASSERT_INT(eq('a', 'b'), FALSE, "eq fold: a!=b");
}

TEST_LIST({
    {"scanner found",            test_scanner_found},
    {"scanner found (PTEND)",    test_scanner_found_ptend},
    {"scanner not found",        test_scanner_not_found},
    {"scanner empty pattern",    test_scanner_empty_pattern},
    {"scanner at start",         test_scanner_at_start},
    {"scanner single char",      test_scanner_single_char},
    {"amatch match",             test_amatch_match},
    {"amatch fail",              test_amatch_fail},
    {"amatch with ANY",          test_amatch_any},
    {"amatch with BOL",          test_amatch_bol},
    {"amatch with EOL",          test_amatch_eol},
    {"amatch partial fail",      test_amatch_partial_fail},
    {"mceq LITCHAR",             test_mceq_litchar},
    {"mceq ANY",                 test_mceq_any},
    {"mceq case exact",          test_mceq_case_exact},
    {"mceq case fold",           test_mceq_case_fold},
    {"eq exact",                 test_eq_exact},
    {"eq casefold",              test_eq_casefold},
})

int main(void)
{
    RUN_TESTS();
}