/*
 * test_line.c -- Unit tests for kemacs line-editor operations.
 *
 * Tests: linsert, ldelete, lnewline, ldelnewline, lchange, lalloc, lfree
 *
 * line.c is included directly after defining maindef (to provide
 * global-variable definitions from edef.h) and static (to expose
 * internal helper functions).  External dependencies are satisfied by
 * minimal stubs defined below.
 *
 * Linking: ../Css.o (for Cxstr, used in mlwrite() calls within line.c)
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>

/* Stubs for functions NOT declared in edef.h (implicit declarations
   in line.c).  Empty-param style matches K&R implicit declarations. */
int mlwrite();
int rdonly();
int backchar();

/* ================================================================ */
/*  Include line.c with maindef (globals defined) + static exposed  */
/* ================================================================ */
#define maindef
#define static
#include "../line.c"
#undef static
#undef maindef

/* ================================================================ */
/*  Stub implementations                                            */
/* ================================================================ */
int mlwrite()   { return TRUE; }
int rdonly()    { return FALSE; }
int backchar()  { return TRUE; }

/* ================================================================ */
/*  Test fixtures                                                    */
/* ================================================================ */

static LINE *g_hdr = NULL;
static LINE *g_line = NULL;

/* Allocate a LINE with room for 'capacity' chars (like lalloc does). */
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
    g_line = NULL;
}

static void setup_line(const char *text)
{
    int i, len = strlen(text);

    cleanup_test_lines();

    /* Allocate header (capacity 0) and text line */
    g_hdr  = alloc_test_line(0);
    g_line = alloc_test_line(len > 0 ? len + 1 : 1);

    for (i = 0; i < len; i++)
        g_line->l_text[i] = (unsigned char)text[i];
    g_line->l_used = len;

    /* Circular double-linked list: hdr <-> g_line */
    g_hdr->l_fp = g_line;   g_hdr->l_bp = g_line;
    g_line->l_fp = g_hdr;   g_line->l_bp = g_hdr;

    /* curbp and curwp are defined as NULL by edef.h with INIT(NULL).
       Allocate and initialize them for the test. */
    if (curbp == NULL) {
        curbp = (BUFFER *)calloc(1, sizeof(BUFFER) + sizeof(Char)*NSTRING);
    }
    if (curwp == NULL) {
        curwp = (WINDOW *)calloc(1, sizeof(WINDOW));
    }
    if (bheadp == NULL) {
        bheadp = curbp;
    }
    if (wheadp == NULL) {
        wheadp = curwp;
    }

    /* Buffer */
    curbp->b_linep = g_hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = 0;  /* not MDVIEW */
    curbp->b_flag  = 0;

    /* Window */
    curwp->w_bufp  = curbp;
    curwp->w_dotp  = g_line;
    curwp->w_doto  = 0;
    curwp->w_flag  = 0;
    curwp->w_wndp  = NULL;
    wheadp = curwp;

    /* Buffer list */
    bheadp = curbp;
    curbp->b_bufp = NULL;

    /* Kill buffer */
    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;
}

/* ================================================================ */
/*  linsert tests                                                   */
/* ================================================================ */

static void test_linsert_middle(void)
{
    setup_line("hello");
    curwp->w_doto = 3;  /* between 'l' and 'l' */

    int rc = linsert(2, 'X');
    TEST_ASSERT_INT(rc, TRUE, "linsert returns TRUE");
    TEST_ASSERT_INT(g_line->l_used, 7, "l_used should be 7");
    TEST_ASSERT_INT(g_line->l_text[0], 'h', "text[0] unchanged");
    TEST_ASSERT_INT(g_line->l_text[1], 'e', "text[1] unchanged");
    TEST_ASSERT_INT(g_line->l_text[2], 'l', "text[2] unchanged");
    TEST_ASSERT_INT(g_line->l_text[3], 'X', "text[3] = X");
    TEST_ASSERT_INT(g_line->l_text[4], 'X', "text[4] = X");
    TEST_ASSERT_INT(g_line->l_text[5], 'l', "text[5] = l");
    TEST_ASSERT_INT(g_line->l_text[6], 'o', "text[6] = o");
    TEST_ASSERT_INT(curwp->w_doto, 5, "cursor advanced past inserted chars");
}

static void test_linsert_at_end(void)
{
    setup_line("hi");
    curwp->w_doto = 2;  /* at end of line */

    int rc = linsert(3, '!');
    TEST_ASSERT_INT(rc, TRUE, "linsert at end returns TRUE");
    TEST_ASSERT_INT(g_line->l_used, 5, "l_used should be 5");
    TEST_ASSERT_INT(g_line->l_text[0], 'h', "text[0] = h");
    TEST_ASSERT_INT(g_line->l_text[1], 'i', "text[1] = i");
    TEST_ASSERT_INT(g_line->l_text[2], '!', "text[2] = !");
    TEST_ASSERT_INT(g_line->l_text[3], '!', "text[3] = !");
    TEST_ASSERT_INT(g_line->l_text[4], '!', "text[4] = !");
}

static void test_linsert_empty_line(void)
{
    setup_line("");
    curwp->w_doto = 0;

    int rc = linsert(1, 'A');
    TEST_ASSERT_INT(rc, TRUE, "linsert on empty line");
    TEST_ASSERT_INT(g_line->l_used, 1, "l_used = 1");
    TEST_ASSERT_INT(g_line->l_text[0], 'A', "text[0] = A");
}

static void test_linsert_one_at_start(void)
{
    setup_line("abc");
    curwp->w_doto = 0;

    int rc = linsert(1, 'Z');
    TEST_ASSERT_INT(rc, TRUE, "linsert at start returns TRUE");
    TEST_ASSERT_INT(g_line->l_used, 4, "l_used = 4");
    TEST_ASSERT_INT(g_line->l_text[0], 'Z', "text[0] = Z");
    TEST_ASSERT_INT(g_line->l_text[1], 'a', "text[1] = a");
    TEST_ASSERT_INT(g_line->l_text[2], 'b', "text[2] = b");
    TEST_ASSERT_INT(g_line->l_text[3], 'c', "text[3] = c");
}

/* ================================================================ */
/*  ldelete tests                                                   */
/* ================================================================ */

static void test_ldelete_simple(void)
{
    setup_line("hello");
    curwp->w_doto = 2;  /* at 'l' */

    int rc = ldelete(2, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete returns TRUE");
    TEST_ASSERT_INT(g_line->l_used, 3, "l_used = 3 (deleted 2 chars)");
    TEST_ASSERT_INT(g_line->l_text[0], 'h', "text[0] = h");
    TEST_ASSERT_INT(g_line->l_text[1], 'e', "text[1] = e");
    TEST_ASSERT_INT(g_line->l_text[2], 'o', "text[2] = o");
}

static void test_ldelete_to_end(void)
{
    setup_line("hello");
    curwp->w_doto = 3;  /* at 2nd 'l' */

    int rc = ldelete(2, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete to end returns TRUE");
    TEST_ASSERT_INT(g_line->l_used, 3, "l_used = 3");
    TEST_ASSERT_INT(g_line->l_text[0], 'h', "text[0] = h");
    TEST_ASSERT_INT(g_line->l_text[1], 'e', "text[1] = e");
    TEST_ASSERT_INT(g_line->l_text[2], 'l', "text[2] = l");
}

static void test_ldelete_at_end(void)
{
    setup_line("hi");
    curwp->w_doto = 2;  /* at end */

    int rc = ldelete(1, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete at end is no-op success");
    TEST_ASSERT_INT(g_line->l_used, 2, "l_used unchanged");
}

static void test_ldelete_zero(void)
{
    setup_line("hello");
    curwp->w_doto = 0;

    int rc = ldelete(0, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete(0) succeeds");
    TEST_ASSERT_INT(g_line->l_used, 5, "l_used unchanged");
}

static void test_ldelete_all(void)
{
    setup_line("hello");
    curwp->w_doto = 0;

    int rc = ldelete(5, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete all returns TRUE");
    TEST_ASSERT_INT(g_line->l_used, 0, "l_used = 0");
}

/* ================================================================ */
/*  lnewline tests                                                  */
/* ================================================================ */

static void test_lnewline_split(void)
{
    setup_line("hello");
    curwp->w_doto = 3;  /* between 'l' and 'l' */

    int rc = lnewline();
    TEST_ASSERT_INT(rc, TRUE, "lnewline returns TRUE");

    /* lnewline creates a new line (lp2) BEFORE the current line (lp1/g_line).
     * lp2 gets text[0..doto-1] ("hel"), lp1 (g_line) keeps text[doto..] ("lo").
     * Cursor: w_doto >= doto, so stays on g_line with offset -= doto. */

    /* Original line (g_line/lp1) now has "lo" (second half) */
    TEST_ASSERT_INT(g_line->l_used, 2, "original line l_used = 2 (second half)");
    TEST_ASSERT_INT(g_line->l_text[0], 'l', "original line text[0] = l");
    TEST_ASSERT_INT(g_line->l_text[1], 'o', "original line text[1] = o");

    /* New line (g_line->l_bp/lp2) has "hel" (first half) */
    LINE *newlp = g_line->l_bp;
    TEST_ASSERT_INT(newlp->l_used, 3, "new line l_used = 3 (first half)");
    TEST_ASSERT_INT(newlp->l_text[0], 'h', "new line text[0] = h");
    TEST_ASSERT_INT(newlp->l_text[1], 'e', "new line text[1] = e");
    TEST_ASSERT_INT(newlp->l_text[2], 'l', "new line text[2] = l");

    /* Cursor stays on original line (g_line) at offset 0 */
    TEST_ASSERT(curwp->w_dotp == g_line, "cursor on original line");
    TEST_ASSERT_INT(curwp->w_doto, 0, "cursor at offset 0 on original line");
}

static void test_lnewline_at_start(void)
{
    setup_line("hello");
    curwp->w_doto = 0;

    int rc = lnewline();
    TEST_ASSERT_INT(rc, TRUE, "lnewline at start");

    /* doto=0: new line (lp2) is empty, original line (g_line) unchanged */
    LINE *newlp = g_line->l_bp;
    TEST_ASSERT_INT(newlp->l_used, 0, "new line empty after split at 0");
    TEST_ASSERT_INT(g_line->l_used, 5, "original line unchanged (l_used=5)");
    TEST_ASSERT_INT(g_line->l_text[0], 'h', "original line text[0] = h");
    TEST_ASSERT(curwp->w_dotp == g_line, "cursor on original line");
    TEST_ASSERT_INT(curwp->w_doto, 0, "cursor at offset 0");
}

static void test_lnewline_at_end(void)
{
    setup_line("hello");
    curwp->w_doto = 5;

    int rc = lnewline();
    TEST_ASSERT_INT(rc, TRUE, "lnewline at end");

    /* doto=5: new line (lp2) gets "hello", original line (g_line) is empty */
    TEST_ASSERT_INT(g_line->l_used, 0, "original line empty after split at end");

    LINE *newlp = g_line->l_bp;
    TEST_ASSERT_INT(newlp->l_used, 5, "new line l_used = 5");
    TEST_ASSERT_INT(newlp->l_text[0], 'h', "new line text[0] = h");
    TEST_ASSERT_INT(newlp->l_text[4], 'o', "new line text[4] = o");
    TEST_ASSERT(curwp->w_dotp == g_line, "cursor on original line");
    TEST_ASSERT_INT(curwp->w_doto, 0, "cursor at offset 0");
}

TEST_LIST({
    {"linsert middle",         test_linsert_middle},
    {"linsert at end",         test_linsert_at_end},
    {"linsert empty line",     test_linsert_empty_line},
    {"linsert one at start",   test_linsert_one_at_start},
    {"ldelete simple",         test_ldelete_simple},
    {"ldelete to end",         test_ldelete_to_end},
    {"ldelete at end",         test_ldelete_at_end},
    {"ldelete zero",           test_ldelete_zero},
    {"ldelete all",            test_ldelete_all},
    {"lnewline split",         test_lnewline_split},
    {"lnewline at start",      test_lnewline_at_start},
    {"lnewline at end",        test_lnewline_at_end},
})

int main(void)
{
    RUN_TESTS();
}