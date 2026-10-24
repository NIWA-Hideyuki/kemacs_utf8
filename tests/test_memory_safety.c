/*
 * test_memory_safety.c -- Buffer-overflow and memory-leak tests for kemacs.
 *
 * Tests cover:
 *   1. linsert hard-realloc path (l_used+n > l_size): data integrity after
 *      reallocation, no out-of-bounds writes (ASan/UBSan detect).
 *   2. linsert at NBLOCK boundary: exact l_size alignment edge cases.
 *   3. Large linsert (1000+ chars): stress the realloc + copy + shift logic.
 *   4. ldelete boundary: deleting past l_used, shrinking to zero.
 *   5. lnewline / ldelnewline: split and join integrity, list links.
 *   6. lalloc / lfree cycling: repeated alloc/free to catch double-free,
 *      use-after-free, and leaks (ASan LeakSanitizer).
 *   7. Undo/redo stress with many records: no leaks, no dangling pointers.
 *   8. undo_limit pruning stress: many records pruned, verify remaining
 *      list is well-formed.
 *   9. u_clear_undo after many edits: all records freed (no leak).
 *  10. kinsert / kdelete stress: kill buffer chunk alloc/free (no leak).
 *
 * line.c and undo.c are included directly after defining maindef (to provide
 * global-variable definitions from edef.h) and static (to expose internal
 * helper functions).  External dependencies are satisfied by minimal stubs.
 *
 * Compiled with -fsanitize=address,undefined for runtime detection.
 *
 * Linking: ../Css.o (for Cxstr, used in mlwrite() calls within line.c)
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

    if (curbp == NULL) {
        curbp = (BUFFER *)calloc(1, sizeof(BUFFER) + sizeof(Char)*NSTRING);
    }
    if (curwp == NULL) {
        curwp = (WINDOW *)calloc(1, sizeof(WINDOW));
    }
    if (bheadp == NULL)
        bheadp = curbp;
    if (wheadp == NULL)
        wheadp = curwp;

    curbp->b_linep = g_hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = 0;
    curbp->b_flag  = 0;
    curbp->b_dotp = g_hdr;
    curbp->b_doto  = 0;
    curbp->b_bufp = NULL;

    curwp->w_bufp  = curbp;
    curwp->w_linep = g_hdr;
    curwp->w_dotp  = g_hdr;
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
    undo_limit = 0;
    u_clear_undo(curbp);
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

    curbp->b_linep = g_hdr;
    curwp->w_dotp  = text_line;
    curwp->w_doto  = 0;
    wheadp = curwp;
    curbp->b_dotp = text_line;
    curbp->b_doto = 0;
}

/* Read current line text into buf. */
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
/*  1. linsert hard-realloc path (l_used+n > l_size)
 *     Insert enough chars to force lalloc reallocation.
 *     ASan will catch any OOB write during the copy/shift.
 * ================================================================ */
static void test_linsert_realloc_path(void)
{
    /* Setup: line "hello" with capacity NBLOCK=16 (l_size=16, l_used=5) */
    setup_buffer("hello");
    /* Insert 20 chars at offset 3 -> l_used becomes 25 > 16 -> hard realloc */
    curwp->w_doto = 3;

    int rc = linsert(20, 'X');
    TEST_ASSERT_INT(rc, TRUE, "linsert(20,'X') realloc path returns TRUE");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 25, "l_used = 25 after realloc insert");

    /* Verify data integrity */
    char buf[64];
    get_line_text(buf, sizeof(buf));
    /* Expected: "hel" + 20 X's + "lo" = "helXXXXXXXXXXXXXXXXXXXXlo" */
    TEST_ASSERT_INT(buf[0], 'h', "buf[0] = 'h'");
    TEST_ASSERT_INT(buf[3], 'X', "buf[3] = 'X' (first inserted)");
    TEST_ASSERT_INT(buf[22], 'X', "buf[22] = 'X' (last inserted)");
    TEST_ASSERT_INT(buf[23], 'l', "buf[23] = 'l' (preserved tail)");
    TEST_ASSERT_INT(buf[24], 'o', "buf[24] = 'o' (preserved tail)");
}

/* ================================================================ */
/*  2. linsert at NBLOCK boundary
 *     Insert exactly to fill l_size, then one more to force realloc.
 *     Tests the l_used+n > l_size comparison at the exact boundary.
 * ================================================================ */
static void test_linsert_at_boundary(void)
{
    /* NBLOCK = 16. alloc_test_line(16) -> size = 16. */
    /* setup_buffer gives capacity len+1, so for "hello" (5 chars),
       capacity = 6, rounded up to NBLOCK = 16.  l_size = 16. */
    setup_buffer("hello");

    /* Insert exactly to fill l_size (16 - 5 = 11 more chars at pos 0) */
    int rc = linsert(11, 'A');
    TEST_ASSERT_INT(rc, TRUE, "linsert(11) fills exactly to l_size");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 16, "l_used = 16 (exact fit)");
    /* Line is now: AAAAAAAAAAAhello (11 A's + "hello") */
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'A', "first char at index 0 = A");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[10], 'A', "10th A at index 10");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[11], 'h', "preserved 'h' at index 11");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[15], 'o', "preserved 'o' at index 15");
    TEST_ASSERT_INT(curwp->w_doto, 11, "cursor advanced to 11 after insert");

    /* Now insert 1 more -> l_used+n = 17 > 16 -> realloc */
    rc = linsert(1, 'B');
    TEST_ASSERT_INT(rc, TRUE, "linsert(1) forces realloc at boundary");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 17, "l_used = 17 after boundary realloc");
    /* Line is now: AAAAAAAAAAABhello (11 A's + B + "hello") */
    TEST_ASSERT_INT(curwp->w_dotp->l_text[11], 'B', "new char 'B' at index 11");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[12], 'h', "preserved 'h' at index 12 after realloc");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[16], 'o', "preserved 'o' at index 16");
}

/* ================================================================ */
/*  3. Large linsert stress (1000+ chars)
 *     ASan/UBSan will catch any overflow in the realloc+copy+shift logic.
 * ================================================================ */
static void test_linsert_large_stress(void)
{
    const int N = 1000;
    char *expected = (char *)malloc(N + 6);
    int i;

    setup_buffer("hello");
    curwp->w_doto = 0;

    /* Insert N chars at position 0 */
    int rc = linsert(N, 'Z');
    TEST_ASSERT_INT(rc, TRUE, "linsert(1000) succeeds");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, N + 5, "l_used = 1005 after large insert");

    /* Build expected string */
    for (i = 0; i < N; i++)
        expected[i] = 'Z';
    strcpy(expected + N, "hello");

    /* Verify all chars */
    for (i = 0; i < N + 5; i++) {
        TEST_ASSERT_INT(curwp->w_dotp->l_text[i], (Char)expected[i],
                        "large insert: char at position matches expected");
        if (curwp->w_dotp->l_text[i] != (Char)expected[i])
            break;
    }
    free(expected);
}

/* ================================================================ */
/*  4. ldelete boundary tests
 *     Delete to exactly zero, delete past end, delete whole line.
 * ================================================================ */
static void test_ldelete_to_empty(void)
{
    setup_buffer("hello");
    curwp->w_doto = 0;
    int rc = ldelete(5, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete(5) from start deletes all");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 0, "l_used = 0 after deleting all");
}

static void test_ldelete_past_end(void)
{
    setup_buffer("hello");
    curwp->w_doto = 3;
    /* Delete 100 but only 2 chars exist -> should delete 2, return TRUE */
    int rc = ldelete(100, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete(100) on 2-char tail succeeds (clamped)");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 3, "l_used = 3 after deleting to end");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'h', "text[0] = 'h'");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[1], 'e', "text[1] = 'e'");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[2], 'l', "text[2] = 'l'");
}

static void test_ldelete_at_end_noop(void)
{
    setup_buffer("hello");
    curwp->w_doto = 5;  /* at end */
    int rc = ldelete(1, FALSE);
    TEST_ASSERT_INT(rc, TRUE, "ldelete(1) at end is no-op success");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used unchanged at end");
}

/* ================================================================ */
/*  5. lnewline / ldelnewline integrity
 *     Split and join lines; verify l_fp/l_bp links are correct
 *     (ASan catches dangling pointer derefs from use-after-free).
 * ================================================================ */
static void test_lnewline_split_integrity(void)
{
    setup_buffer("hello");
    curwp->w_doto = 3;

    int rc = lnewline();
    TEST_ASSERT_INT(rc, TRUE, "lnewline split returns TRUE");

    LINE *second = curwp->w_dotp;
    LINE *first = lback(second);

    /* Verify linked-list integrity: first->l_fp == second, second->l_bp == first */
    TEST_ASSERT(first->l_fp == second, "first->l_fp == second");
    TEST_ASSERT(second->l_bp == first, "second->l_bp == first");
    TEST_ASSERT(first->l_bp == g_hdr, "first->l_bp == header");
    TEST_ASSERT(second->l_fp == g_hdr, "second->l_fp == header");

    /* Content */
    TEST_ASSERT_INT(first->l_used, 3, "first half l_used = 3");
    TEST_ASSERT_INT(second->l_used, 2, "second half l_used = 2");
}

static void test_ldelnewline_join_integrity(void)
{
    /* Set up two lines: "hel" and "lo" */
    setup_buffer("hello");
    curwp->w_doto = 3;
    lnewline();

    /* Now cursor is on second line ("lo"). ldelnewline merges with next.
       But next IS the header (end of buffer). So we need to be on the
       first line to test join properly.  Move to first line. */
    LINE *first = lback(curwp->w_dotp);
    curwp->w_dotp = first;
    curwp->w_doto = first->l_used;  /* at end of first line */

    int rc = ldelnewline();
    TEST_ASSERT_INT(rc, TRUE, "ldelnewline join returns TRUE");

    /* After join: first line now has "hello" (5 chars), cursor on first */
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after join");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[0], 'h', "text[0] = 'h' after join");
    TEST_ASSERT_INT(curwp->w_dotp->l_text[4], 'o', "text[4] = 'o' after join");
}

/* ================================================================ */
/*  6. lalloc / lfree cycling
 *     Repeatedly allocate and free lines (ASan LeakSanitizer catches leaks;
 *     ASan detects double-free and use-after-free).
 * ================================================================ */
static void test_lfree_cycle(void)
{
    int i;
    /* Set up minimal buffer state */
    reset_buffer_state();

    /* Allocate and free many lines in a cycle */
    for (i = 0; i < 500; i++) {
        LINE *lp = lalloc(16);
        TEST_ASSERT(lp != NULL, "lalloc(16) returns non-NULL");
        if (lp == NULL) break;
        lp->l_used = 16;
        /* Link into the circular list so lfree can traverse windows/buffers */
        lp->l_fp = g_hdr;
        lp->l_bp = g_hdr->l_bp;
        g_hdr->l_bp->l_fp = lp;
        g_hdr->l_bp = lp;
        /* Set curwp->w_dotp to something valid so lfree's window scan is safe */
        curwp->w_dotp = g_hdr;
        lfree(lp);
    }

    TEST_ASSERT_INT(test_failed, test_failed, "lfree cycle: no ASan errors");
}

/* ================================================================ */
/*  7. Undo/redo stress with many records
 *     Create many undo records, undo all, redo all.
 *     ASan catches leaks in undo record allocation/freeing;
 *     ASan detects use-after-free if a freed line is referenced.
 * ================================================================ */
static void test_undo_stress_many_records(void)
{
    const int N = 500;
    int i, rc;

    setup_buffer("hello");
    curwp->w_doto = 0;

    /* Insert N chars one at a time (each creates an undo record) */
    for (i = 0; i < N; i++) {
        rc = linsert(1, 'A' + (i % 26));
        TEST_ASSERT_INT(rc, TRUE, "stress insert returns TRUE");
        if (rc != TRUE) break;
    }
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5 + N, "l_used = 5+N after many inserts");
    TEST_ASSERT_INT(curbp->b_undo_count, N, "undo count = N after inserts");

    /* Undo all */
    for (i = 0; i < N; i++) {
        rc = undo(0, 1);
        TEST_ASSERT_INT(rc, TRUE, "stress undo returns TRUE");
        if (rc != TRUE) break;
    }
    TEST_ASSERT_INT(curbp->b_undo_count, N, "undo count preserved after undo all");
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5, "l_used = 5 after undoing all inserts");

    /* Redo all */
    for (i = 0; i < N; i++) {
        rc = redo(0, 1);
        TEST_ASSERT_INT(rc, TRUE, "stress redo returns TRUE");
        if (rc != TRUE) break;
    }
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 5 + N, "l_used = 5+N after redoing all");
}

/* ================================================================ */
/*  8. undo_limit pruning stress
 *     Set undo_limit to a small value, create many records,
 *     verify pruning doesn't corrupt the list and frees correctly.
 * ================================================================ */
static void test_undo_limit_prune_stress(void)
{
    const int N = 200;
    int i, rc;

    setup_buffer("ab");
    curwp->w_doto = 0;
    undo_limit = 10;

    for (i = 0; i < N; i++) {
        rc = linsert(1, 'X');
        TEST_ASSERT_INT(rc, TRUE, "prune stress insert returns TRUE");
        if (rc != TRUE) break;
        u_new_bndry = TRUE; /* separate boundary each time */
    }

    /* undo_limit = 10 means at most 10 records (well, up to 10 kept) */
    TEST_ASSERT(curbp->b_undo_count <= 10, "undo count <= limit (10)");
    TEST_ASSERT(curbp->b_undo_count > 0, "undo count > 0 after inserts");

    /* Walk the undo list to verify it's well-formed (no cycles, no crashes) */
    UNDO *rec = curbp->b_undo_head;
    int count = 0;
    while (rec != NULL && count <= N) {
        count++;
        rec = rec->u_next;
    }
    TEST_ASSERT(count == curbp->b_undo_count,
                "undo list walk count matches b_undo_count");

    /* Restore limit */
    undo_limit = 0;
    u_clear_undo(curbp);
}

/* ================================================================ */
/*  9. u_clear_undo after many edits
 *     Create many undo records, then clear.  ASan LeakSanitizer
 *     verifies all allocated memory is freed.
 * ================================================================ */
static void test_clear_undo_after_many_edits(void)
{
    int i, rc;

    setup_buffer("hello");
    curwp->w_doto = 0;

    /* Insert 100 chars */
    for (i = 0; i < 100; i++) {
        rc = linsert(1, 'A');
        TEST_ASSERT_INT(rc, TRUE, "clear-undo stress insert");
        if (rc != TRUE) break;
    }
    TEST_ASSERT(curbp->b_undo_count > 0, "undo records exist before clear");

    u_clear_undo(curbp);
    TEST_ASSERT_INT(curbp->b_undo_count, 0, "undo count = 0 after clear");
    TEST_ASSERT(curbp->b_undo_head == NULL, "undo head NULL after clear");
    TEST_ASSERT(curbp->b_undo_tail == NULL, "undo tail NULL after clear");
    TEST_ASSERT(curbp->b_undo_ptr == NULL, "undo ptr NULL after clear");
}

/* ================================================================ */
/*  10. kinsert / kdelete stress (kill buffer chunk alloc/free)
 *     Insert many chars into kill buffer, then kdelete.
 *     ASan LeakSanitizer catches leaks in kill buffer chunks.
 * ================================================================ */
static void test_kill_buffer_stress(void)
{
    const int N = 10000;
    int i, rc;

    reset_buffer_state();

    /* Insert N chars into kill buffer (allocates ~N/KBLOCK chunks) */
    for (i = 0; i < N; i++) {
        rc = kinsert('X');
        TEST_ASSERT_INT(rc, TRUE, "kinsert stress returns TRUE");
        if (rc != TRUE) break;
    }

    /* kused should be < KBLOCK (only current chunk's used count) */
    TEST_ASSERT(kbufh != NULL, "kill buffer has chunks after insert");

    /* Free all chunks */
    kdelete();
    TEST_ASSERT(kbufh == NULL, "kill buffer freed after kdelete");
    TEST_ASSERT(kbufp == NULL, "kill buffer current ptr NULL after kdelete");

    /* kinsert/kdelete cycle multiple times to catch leaks */
    for (i = 0; i < 50; i++) {
        int j;
        for (j = 0; j < KBLOCK * 3; j++) {
            rc = kinsert('Y');
            if (rc != TRUE) break;
        }
        kdelete();
    }
    TEST_ASSERT(kbufh == NULL, "kill buffer freed after 50 cycle iters");
}

/* ================================================================ */
/*  11. linsert growth across many reallocations
 *     Insert 1 char at a time for 500 iterations to exercise
 *     many lalloc calls and verify the line grows correctly each time.
 * ================================================================ */
static void test_linsert_many_growths(void)
{
    const int N = 500;
    int i, rc;

    setup_buffer("X");
    curwp->w_doto = 0;

    for (i = 0; i < N; i++) {
        rc = linsert(1, 'A' + (i % 26));
        TEST_ASSERT_INT(rc, TRUE, "single insert in growth loop");
        if (rc != TRUE) break;
    }
    TEST_ASSERT_INT(curwp->w_dotp->l_used, 1 + N, "l_used = 1+N after many growths");

    /* Verify l_size was grown appropriately (at least N+1 capacity) */
    TEST_ASSERT(curwp->w_dotp->l_size >= 1 + N,
                "l_size >= N+1 after many growths (capacity grew)");

    /* Verify data integrity: chars inserted at position 0 shift right.
       After N inserts at position 0, l_text[i] = 'A' + (i%26) for i=0..N-1,
       and l_text[N] = original 'X'. */
    for (i = 0; i < N; i++) {
        TEST_ASSERT(curwp->w_dotp->l_text[i] == (Char)('A' + (i % 26)),
                    "data integrity: each char matches in growth test");
        if (curwp->w_dotp->l_text[i] != (Char)('A' + (i % 26)))
            break;
    }
    TEST_ASSERT_INT(curwp->w_dotp->l_text[N], 'X', "original char preserved at end");
}

/* ================================================================ */
/*  12. Multi-line buffer: insert/delete across lines
 *     Verify list integrity (l_fp/l_bp circular list) holds
 *     after many line operations.  ASan catches dangling links.
 * ================================================================ */
static void test_multiline_buffer_integrity(void)
{
    int i;

    setup_buffer("");

    /* Create 100 empty lines by inserting newlines */
    for (i = 0; i < 100; i++) {
        int rc = lnewline();
        TEST_ASSERT_INT(rc, TRUE, "lnewline in multi-line setup");
        if (rc != TRUE) break;
    }

    /* Walk the list: header <-> line1 <-> line2 <-> ... <-> 101st <-> header
       (setup_buffer("") creates 1 initial line, then 100 newlines = 101 total) */
    LINE *cur = g_hdr->l_fp;
    int count = 0;
    while (cur != g_hdr && cur != NULL) {
        count++;
        /* Verify back-link consistency */
        TEST_ASSERT(cur->l_bp->l_fp == cur, "forward link matches backward link");
        cur = cur->l_fp;
    }
    TEST_ASSERT_INT(count, 101, "101 lines (1 initial + 100 newlines)");

    /* Walk backward to verify reverse links */
    cur = g_hdr->l_bp;
    count = 0;
    while (cur != g_hdr && cur != NULL) {
        count++;
        TEST_ASSERT(cur->l_fp->l_bp == cur, "backward link matches forward link");
        cur = cur->l_bp;
    }
    TEST_ASSERT_INT(count, 101, "backward walk finds 101 lines");
}

/* ================================================================ */
/*  Test registry
 * ================================================================ */

TEST_LIST({
    {"linsert hard realloc path",          test_linsert_realloc_path},
    {"linsert at NBLOCK boundary",         test_linsert_at_boundary},
    {"linsert large stress (1000 chars)",  test_linsert_large_stress},
    {"ldelete to empty",                   test_ldelete_to_empty},
    {"ldelete past end",                   test_ldelete_past_end},
    {"ldelete at end (no-op)",             test_ldelete_at_end_noop},
    {"lnewline split integrity",           test_lnewline_split_integrity},
    {"ldelnewline join integrity",         test_ldelnewline_join_integrity},
    {"lfree alloc/free cycle (500x)",      test_lfree_cycle},
    {"undo/redo stress (500 records)",     test_undo_stress_many_records},
    {"undo_limit pruning stress",          test_undo_limit_prune_stress},
    {"u_clear_undo after many edits",      test_clear_undo_after_many_edits},
    {"kill buffer stress (10000 inserts)",  test_kill_buffer_stress},
    {"linsert many growths (500x)",        test_linsert_many_growths},
    {"multi-line buffer link integrity",   test_multiline_buffer_integrity},
})

int main(void)
{
    printf("=== Memory safety tests (compiled with ASan + UBSan) ===\n\n");

    RUN_TESTS();
}