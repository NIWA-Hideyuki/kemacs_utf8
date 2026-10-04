/*
 * test_undo.c -- Unit tests for the kemacs undo/redo system.
 *
 * Tests cover: INSERT undo/redo, DELETE undo/redo, NEWLINE/JOIN
 * undo/redo, boundary grouping, u_clear_undo, and undo-limit
 * pruning.
 *
 * line.c and undo.c are included directly after defining maindef
 * (to provide global-variable definitions from edef.h) and static
 * (to expose internal helper functions).  External dependencies
 * are satisfied by minimal stubs defined below.
 *
 * Linking: ../Css.o (for Cxstr, used in mlwrite() calls)
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>

/* Stubs for functions NOT declared in edef.h */
int mlwrite();
int rdonly();
int backchar();

/* ================================================================ */
/*  Include line.c and undo.c with maindef (globals defined) + static exposed */
/* ================================================================ */
#define maindef
#define static
#include "../line.c"
#include "../undo.c"
#undef static
#undef maindef

/* ================================================================ */
/*  Stub implementations                                            */
/* ================================================================ */
int mlwrite()   { return TRUE; }
int rdonly()    { return FALSE; }
int backchar()  { return TRUE; }

/* ================================================================ */
/*  Test fixtures                                                   */
/* ================================================================ */

static LINE *g_hdr = NULL;   /* buffer header line (circular list sentinel) */

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
}

static void reset_buffer_state(void)
{
    cleanup_test_lines();
    g_hdr = alloc_test_line(0);
    g_hdr->l_fp = g_hdr;
    g_hdr->l_bp = g_hdr;
}

/*
 * Set up a buffer with a single text line containing 'text'.
 * After this call:
 *   curbp->b_linep = g_hdr (circular header)
 *   curwp->w_dotp  = text_line
 *   curwp->w_doto  = 0
 */
static void setup_buffer(const char *text)
{
    int i, len = strlen(text);
    LINE *text_line;

    reset_buffer_state();

    text_line = alloc_test_line(len > 0 ? len + 1 : 1);
    for (i = 0; i < len; i++)
        text_line->l_text[i] = (unsigned char)text[i];
    text_line->l_used = len;

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
    if (bheadp == NULL) {
        bheadp = curbp;
    }
    if (wheadp == NULL) {
        wheadp = curwp;
    }

    curbp->b_linep = g_hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = 0;
    curbp->b_flag  = 0;
    curbp->b_dotp  = text_line;
    curbp->b_doto  = 0;
    curbp->b_bufp  = NULL;

    curwp->w_bufp  = curbp;
    curwp->w_linep = text_line;
    curwp->w_dotp  = text_line;
    curwp->w_doto  = 0;
    curwp->w_flag  = 0;
    curwp->w_wndp  = NULL;
    wheadp = curwp;

    bheadp = curbp;
    curbp->b_bufp = NULL;

    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;

    u_in_undo = FALSE;
    u_new_bndry = TRUE;
    u_clear_undo(curbp);
}

/*
 * Read the current line content (curwp->w_dotp) into buf.
 */
static int get_line_text(char *buf, int bufsize)
{
    int i, len = curwp->w_dotp->l_used;
    if (len > bufsize - 1)
        len = bufsize - 1;
    for (i = 0; i < len; i++)
        buf[i] = (char)curwp->w_dotp->l_text[i];
    buf[len] = '\0';
    return len;
}

/* ================================================================ */
/*  INSERT undo/redo tests                                           */
/* ================================================================ */

static void test_undo_insert_single(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 0;

    int rc = linsert(1, 'X');
    TEST_ASSERT_INT(rc, TRUE, "linsert returns TRUE");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 6, "l_used = 6 after insert");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'X', "char inserted at 0");

    rc = undo(0, 1);
    TEST_ASSERT_INT(rc, TRUE, "undo returns TRUE");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undo");
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "hello", "text restored after undo");
}

static void test_undo_insert_multiple_in_one_command(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 5;

    linsert(3, '!');
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "hello!!!", "text after insert");

    undo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undo");
    TEST_ASSERT_STR(buf, "hello", "text restored after undo");
}

static void test_redo_insert_single(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 0;

    linsert(1, 'X');
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'X', "inserted X");

    undo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "undo removes X");
    TEST_ASSERT_STR(buf, "hello", "text after undo");

    redo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 6, "redo restores l_used");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'X', "redo re-inserts X");
    TEST_ASSERT_STR(buf, "Xhello", "text after redo");
}

/* ================================================================ */
/*  DELETE undo/redo tests                                           */
/* ================================================================ */

static void test_undo_delete_single(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 1;

    int rc = ldelete(1, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete returns TRUE");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 4, "l_used = 4 after delete");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'h', "text[0] = h");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[1], 'l', "text[1] = l (was 'e')");

    rc = undo(0, 1);
    TEST_ASSERT_INT(rc, TRUE, "undo returns TRUE");
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undo");
    TEST_ASSERT_STR(buf, "hello", "text restored after undo");
}

static void test_undo_delete_multiple_in_one_command(void)
{
    char buf[64];

    setup_buffer("hello world");
    curwp->w_doto = 5;

    ldelete(6, FALSE);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after delete");

    undo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 11, "l_used = 11 after undo");
    TEST_ASSERT_STR(buf, "hello world", "text restored after undo");
}

static void test_redo_delete_single(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 0;

    ldelete(2, FALSE);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 3, "l_used = 3 after delete");
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "llo", "text after delete");

    undo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undo");
    TEST_ASSERT_STR(buf, "hello", "text after undo");

    redo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 3, "l_used = 3 after redo");
    TEST_ASSERT_STR(buf, "llo", "text after redo");
}

/* ================================================================ */
/*  NEWLINE (split) undo/redo tests                                 */
/* ================================================================ */

static void test_undo_newline_split(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 3;  /* split between "hel" and "lo" */

    int rc = lnewline();
    TEST_ASSERT_INT(rc, TRUE, "lnewline returns TRUE");

    /* After split: first half = "hel", second half (cur line) = "lo" */
    LINE *second = curwp->w_dotp;
    LINE *first = lback(second);
    TEST_ASSERT_INT(first->l_used, 3, "first half l_used = 3");
    TEST_ASSERT_INT(first->l_text[0], 'h', "first half text[0] = h");
    TEST_ASSERT_INT(second->l_used, 2, "second half l_used = 2");
    TEST_ASSERT_INT(second->l_text[0], 'l', "second half text[0] = l");

    /* Undo should join them back */
    rc = undo(0, 1);
    TEST_ASSERT_INT(rc, TRUE, "undo returns TRUE");
    /* After join, cursor is on the merged line */
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undo");
    TEST_ASSERT_STR(buf, "hello", "text restored after undo");
}

static void test_redo_newline_split(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 3;

    lnewline();
    LINE *first = lback(curwp->w_dotp);
    TEST_ASSERT_INT(first->l_used, 3, "first half after lnewline");

    undo(0, 1);
    /* After undo, joined line has "hello", cursor on merged line */
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used after undo");
    TEST_ASSERT_STR(buf, "hello", "text after undo join");

    /* Redo should split again at offset 3 */
    redo(0, 1);
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 2, "second half after redo");
    TEST_ASSERT_STR(buf, "lo", "second half content after redo");

    /* First half should be "hel" */
    first = lback(curwp->w_dotp);
    TEST_ASSERT_INT(first->l_used, 3, "first half restored after redo");
    TEST_ASSERT_INT(first->l_text[0], 'h', "first half text[0] = h");
    TEST_ASSERT_INT(first->l_text[2], 'l', "first half text[2] = l");
}

/* ================================================================ */
/*  JOIN undo/redo tests                                            */
/* ================================================================ */

static void test_undo_join_lines(void)
{
    char buf[64];

    /* Set up buffer with "hello" (capacity >= 5, NBLOCK-aligned) */
    setup_buffer("hello");
    curwp->w_doto = 5;  /* cursor at end of "hello" */

    /* Insert newline to create second line */
    lnewline();
    LINE *first = lback(curwp->w_dotp);
    TEST_ASSERT_INT(first->l_used, 5, "first line = 'hello'");

    /* Insert "hi" into the second (empty) line */
    curwp->w_doto = 0;
    linsert(1, 'h');
    linsert(1, 'i');
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 2, "second line has 'hi'");

    /* Now join: cursor on first-line at end, delete triggers ldelnewline */
    u_new_bndry = TRUE;  /* separate undo boundary for the join */
    curwp->w_dotp = first;
    curwp->w_doto = first->l_used;  /* = 5, at end of "hello" */
    ldelete(1, FALSE);

    /* After join: line has "hellohi" */
    LINE *joined = curwp->w_dotp;
    TEST_ASSERT_INT(joined->l_used, 7, "merged line l_used = 7");
    TEST_ASSERT_INT(joined->l_text[0], 'h', "merged text[0] = h");
    TEST_ASSERT_INT(joined->l_text[5], 'h', "merged text[5] = h (from 'hi')");

    /* Undo should split the line back */
    undo(0, 1);
    first = lback(curwp->w_dotp);
    TEST_ASSERT_INT(first->l_used, 5, "first half after undo");
    TEST_ASSERT_INT(first->l_text[0], 'h', "first half text[0] = h");
    TEST_ASSERT_INT(first->l_text[4], 'o', "first half text[4] = o");

    LINE *second = first->l_fp;
    TEST_ASSERT(second != g_hdr, "second line exists after undo");
    TEST_ASSERT_INT(second->l_used, 2, "second half l_used = 2");
    TEST_ASSERT_INT(second->l_text[0], 'h', "second half text[0] = h");
    TEST_ASSERT_INT(second->l_text[1], 'i', "second half text[1] = i");

    /* Redo should rejoin */
    redo(0, 1);
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 7, "merged line l_used after redo");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'h', "merged text[0] = h after redo");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[6], 'i', "merged text[6] = i after redo");
}

/* ================================================================ */
/*  u_clear_undo tests                                               */
/* ================================================================ */

static void test_clear_undo(void)
{
    setup_buffer("hello");
    curwp->w_doto = 0;

    linsert(2, 'X');
    TEST_ASSERT(curbp->b_undo_count > 0, "undo records exist after insert");

    u_clear_undo(curbp);
    TEST_ASSERT_INT(curbp->b_undo_count, 0, "undo count = 0 after clear");
    TEST_ASSERT(curbp->b_undo_head == NULL, "undo head = NULL after clear");
    TEST_ASSERT(curbp->b_undo_tail == NULL, "undo tail = NULL after clear");
    TEST_ASSERT(curbp->b_undo_ptr == NULL, "undo ptr = NULL after clear");
}

/* ================================================================ */
/*  undo_limit pruning tests                                        */
/* ================================================================ */

static void test_undo_limit_pruning(void)
{
    setup_buffer("a");
    undo_limit = 3;  /* only keep 3 records */

    curwp->w_doto = 0;
    /* Insert 4 characters, one per "command" (set boundary each time) */
    linsert(1, 'b');
    u_new_bndry = TRUE;
    linsert(1, 'c');
    u_new_bndry = TRUE;
    linsert(1, 'd');
    u_new_bndry = TRUE;
    linsert(1, 'e');

    TEST_ASSERT_INT(curbp->b_undo_count, 3, "undo count = 3 (pruned to limit)");
    TEST_ASSERT(curbp->b_undo_head != NULL, "undo head not NULL after pruning");

    /* Restore limit */
    undo_limit = 0;
}

/* ================================================================ */
/*  Boundary grouping tests                                         */
/* ================================================================ */

static void test_boundary_grouping(void)
{
    char buf[64];

    setup_buffer("hello");
    curwp->w_doto = 0;

    /* Two inserts in one boundary should be undone together */
    linsert(1, 'X');
    /* u_new_bndry is now FALSE, so second insert is same boundary */
    linsert(1, 'Y');

    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "XYhello", "text after two inserts");

    undo(0, 1);
    /* Both X and Y should be undone in one call */
    get_line_text(buf, sizeof(buf));
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undo (both removed)");
    TEST_ASSERT_STR(buf, "hello", "text fully restored");
}

static void test_no_undo_when_nothing_to_undo(void)
{
    setup_buffer("hello");

    /* No edits - undo should be a no-op */
    int rc = undo(0, 1);
    TEST_ASSERT_INT(rc, TRUE, "undo with no records returns TRUE");
}

/* ================================================================ */
/*  Test registry                                                  */
/* ================================================================ */

TEST_LIST({
    {"undo insert single",              test_undo_insert_single},
    {"undo insert multi (boundary)",    test_undo_insert_multiple_in_one_command},
    {"redo insert single",              test_redo_insert_single},
    {"undo delete single",              test_undo_delete_single},
    {"undo delete multi (boundary)",    test_undo_delete_multiple_in_one_command},
    {"redo delete single",              test_redo_delete_single},
    {"undo newline split",              test_undo_newline_split},
    {"redo newline split",              test_redo_newline_split},
    {"undo join lines",                 test_undo_join_lines},
    {"clear undo",                      test_clear_undo},
    {"undo limit pruning",              test_undo_limit_pruning},
    {"boundary grouping",               test_boundary_grouping},
    {"undo nothing to undo",            test_no_undo_when_nothing_to_undo},
})

int main(void)
{
    RUN_TESTS();
}
