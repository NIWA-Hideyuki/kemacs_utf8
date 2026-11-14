/*
 * test_mdmode.c -- Unit tests for kemacs Markdown mode (MDMODE).
 *
 * Covers Phase 1 (mode skeleton, mdinsert, mdindent, md_backspace,
 * md_tab) and Phase 2 (headings, lists, task lists, emphasis, links,
 * quotes, fences, table cell motion, outline).
 *
 * line.c and mdmode.c are included directly after defining maindef
 * (global definitions from edef.h) and static (expose helpers).
 * proto.h is included explicitly because KEMACS_TEST suppresses the
 * proto.h include inside edef.h, and mdmode.c calls pointer-returning
 * buffer functions.
 *
 * Linking: ../Css.o (for Cxstr used by CSTR() macros)
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>

/* ================================================================ */
/*  Include kemacs source files with maindef + static              */
/* ================================================================ */
#define maindef
#define static
#include "../ecomm.h"
#include "../estruct.h"
#include "../edef.h"
#include "../proto.h"
#include "../line.c"
#include "../mdmode.c"
#undef static
#undef maindef

/* ================================================================ */
/*  Stub implementations                                           */
/* ================================================================ */

int tabsize = 0;        /* defined in random.c, needed by mdmode.c */

int mlwrite(Char *fmt, ...) { (void)fmt; return TRUE; }
int rdonly(void)     { return FALSE; }
int upmode(void)     { return TRUE; }
int forwdel(int f, int n) { (void)f; (void)n; return TRUE; }
int pyindent(int f, int n) { (void)f; (void)n; return TRUE; }
int cindent(int f, int n)  { (void)f; (void)n; return TRUE; }

/* Char strings used by the tests (CSTR() is not expanded in test
 * sources, so plain char literals cannot be assigned to Char *). */
static Char md_test_bullets[] = { '-', ' ', '*', ' ', '+', 0 };
static Char md_test_empty[]   = { 0 };
static Char md_test_lang_c[]  = { 'c', 0 };
static Char md_test_preview_default[] = { 'g','r','i','p',' ','%','f', 0 };
static Char md_test_url_buf[] = {
    'h','t','t','p',':','/','/','e','x','a','m','p','l','e','.','c','o','m', 0
};

/* URL returned by the mlreply() stub for md_insert_link tests. */
static Char *md_test_url = NULL;

int mlreply(Char *prompt, Char *buf, int nbuf)
{
    int i;
    (void)prompt;
    if (md_test_url == NULL)
        return FALSE;
    for (i = 0; md_test_url[i] && i < nbuf - 1; i++)
        buf[i] = md_test_url[i];
    buf[i] = 0;
    return TRUE;
}

int getccol(int bflg)
{
    (void)bflg;
    return curwp->w_doto;
}

int gotobol(int f, int n)
{
    (void)f;
    (void)n;
    curwp->w_doto = 0;
    return TRUE;
}

int gotoline(int f, int n)
{
    LINE *lp;
    int i;
    (void)f;
    if (n <= 0)
        return FALSE;
    lp = lforw(curbp->b_linep);
    for (i = 1; i < n && lp != curbp->b_linep; i++)
        lp = lforw(lp);
    curwp->w_dotp = lp;
    curwp->w_doto = 0;
    return TRUE;
}

/* backchar -- move cursor back n chars (simplified, single buffer) */
int backchar(int f, int n)
{
    LINE *lp;
    (void)f;
    while (n--) {
        if (curwp->w_doto == 0) {
            lp = lback(curwp->w_dotp);
            if (lp == curbp->b_linep)
                return FALSE;
            curwp->w_dotp  = lp;
            curwp->w_doto  = llength(lp);
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
    curwp->w_dotp  = dlp;
    curwp->w_doto  = 0;
    return TRUE;
}

/* backline -- move cursor up n lines (simplified) */
int backline(int f, int n)
{
    LINE *dlp;
    (void)f;
    if (n < 0)
        return FALSE;
    dlp = curwp->w_dotp;
    while (n-- && lback(dlp) != curbp->b_linep)
        dlp = lback(dlp);
    curwp->w_dotp  = dlp;
    curwp->w_doto  = 0;
    return TRUE;
}

/* Undo stubs (called from line.c but not under test here) */
void u_record(int type, LINE *lp, int offset, int length,
              Char *data, int datalen, int eob)
{
    (void)type; (void)lp; (void)offset; (void)length;
    (void)data; (void)datalen; (void)eob;
}
void u_clear_undo(BUFFER *bp) { (void)bp; }
void u_prune(BUFFER *bp) { (void)bp; }

/* getregion -- compute the region between dot and mark. */
int getregion(REGION *rp)
{
    LINE  *flp;
    LINE  *blp;
    long   fsize;
    long   bsize;

    if (curwp->w_markp == NULL)
        return FALSE;
    if (curwp->w_dotp == curwp->w_markp) {
        rp->r_linep = curwp->w_dotp;
        if (curwp->w_doto < curwp->w_marko) {
            rp->r_offset = curwp->w_doto;
            rp->r_size   = (long)(curwp->w_marko - curwp->w_doto);
        } else {
            rp->r_offset = curwp->w_marko;
            rp->r_size   = (long)(curwp->w_doto - curwp->w_marko);
        }
        return TRUE;
    }
    blp  = curwp->w_dotp;
    bsize = (long)curwp->w_doto;
    flp  = curwp->w_dotp;
    fsize = (long)(llength(flp) - curwp->w_doto + 1);
    while (flp != curbp->b_linep || lback(blp) != curbp->b_linep) {
        if (flp != curbp->b_linep) {
            flp = lforw(flp);
            if (flp == curwp->w_markp) {
                rp->r_linep  = curwp->w_dotp;
                rp->r_offset = curwp->w_doto;
                rp->r_size   = fsize + curwp->w_marko;
                return TRUE;
            }
            fsize += llength(flp) + 1;
        }
        if (lback(blp) != curbp->b_linep) {
            blp = lback(blp);
            bsize += llength(blp) + 1;
            if (blp == curwp->w_markp) {
                rp->r_linep  = blp;
                rp->r_offset = curwp->w_marko;
                rp->r_size   = bsize - curwp->w_marko;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* -------- minimal buffer stubs for the outline -------- */

BUFFER *bfind(Char *bname, int cflag, int bflag)
{
    BUFFER *bp;
    LINE *lp;
    int len, i;

    for (bp = bheadp; bp != NULL; bp = bp->b_bufp)
        if (Cstrcmp(bname, bp->b_bname) == 0)
            return bp;
    if (cflag == FALSE)
        return NULL;
    bp = (BUFFER *)calloc(1, sizeof(BUFFER));
    if (bp == NULL)
        return NULL;
    lp = lalloc(0);
    if (lp == NULL) {
        free((char *)bp);
        return NULL;
    }
    lp->l_fp = lp;
    lp->l_bp = lp;
    bp->b_linep = lp;
    bp->b_dotp  = lp;
    bp->b_doto  = 0;
    bp->b_markp = NULL;
    bp->b_marko = 0;
    bp->b_flag  = bflag;
    bp->b_mode  = gmode;
    bp->b_nwnd  = 0;
    bp->b_active = TRUE;
    bp->b_nlines = 0;
    len = Cstrlen(bname);
    bp->b_bname = (Char *)malloc((len + 1) * sizeof(Char));
    for (i = 0; i <= len; i++)
        bp->b_bname[i] = bname[i];
    bp->b_fname = (Char *)malloc(sizeof(Char));
    bp->b_fname[0] = 0;
    bp->b_bufp = bheadp;
    bheadp = bp;
    return bp;
}

int bclear(BUFFER *bp)
{
    LINE *lp;

    while ((lp = lforw(bp->b_linep)) != bp->b_linep)
        lfree(lp);
    bp->b_dotp  = bp->b_linep;
    bp->b_doto  = 0;
    bp->b_markp = NULL;
    bp->b_marko = 0;
    bp->b_nlines = 0;
    return TRUE;
}

int addline(BUFFER *bp, Char *text)
{
    LINE *lp;
    int i, n = Cstrlen(text);

    lp = lalloc(n);
    if (lp == NULL)
        return FALSE;
    for (i = 0; i < n; i++)
        lputc(lp, i, text[i]);
    bp->b_linep->l_bp->l_fp = lp;
    lp->l_bp = bp->b_linep->l_bp;
    bp->b_linep->l_bp = lp;
    lp->l_fp = bp->b_linep;
    if (bp->b_dotp == bp->b_linep)
        bp->b_dotp = lp;
    return TRUE;
}

int swbuffer(BUFFER *bp)
{
    curbp = bp;
    curwp->w_bufp = bp;
    curwp->w_dotp = bp->b_dotp;
    curwp->w_doto = bp->b_doto;
    return TRUE;
}

/* ---- Phase 3/4 stubs ---- */

/* vtputc captures the virtual-screen output of md_draw_line(). */
static Char vtputc_buf[NLINE];
static int vtputc_len = 0;

int vtputc(Char c)
{
    if (vtputc_len < NLINE - 1)
	vtputc_buf[vtputc_len++] = c;
    return TRUE;
}

static void vtputc_reset(void)
{
    vtputc_len = 0;
    vtputc_buf[0] = 0;
}

static int vtputc_to_cstr(char *buf, int size)
{
    int i, n = vtputc_len;
    if (n > size - 1)
	n = size - 1;
    for (i = 0; i < n; i++)
	buf[i] = (char)vtputc_buf[i];
    buf[n] = '\0';
    return n;
}

/* spawn_preview records the command instead of running it. */
static Char spawn_cmd_buf[NLINE * 2];
static Char *spawn_last_cmd = NULL;
static Char spawn_tmp_buf[NLINE];
static Char *spawn_last_tmp = NULL;
static Char spawn_bname_buf[NBUFN];
static Char *spawn_last_bname = NULL;
static int spawn_called = 0;

int spawn_preview(Char *cmd, Char *bname, Char *tmpfile)
{
    int i;
    spawn_called = 1;
    Cstrcpy(spawn_cmd_buf, cmd);
    spawn_last_cmd = spawn_cmd_buf;
    if (tmpfile != NULL) {
	Cstrcpy(spawn_tmp_buf, tmpfile);
	spawn_last_tmp = spawn_tmp_buf;
    } else
	spawn_last_tmp = NULL;
    if (bname != NULL) {
	for (i = 0; bname[i] && i < NBUFN - 1; i++)
	    spawn_bname_buf[i] = bname[i];
	spawn_bname_buf[i] = 0;
	spawn_last_bname = spawn_bname_buf;
    } else
	spawn_last_bname = NULL;
    return TRUE;
}

void preview_wait_first(void)
{
}

/* Minimal stand-in for the real shell escaper (no quoting). */
char *shell_command_escape_char(const char *s)
{
    static char b[NLINE];
    int i;

    for (i = 0; s[i] && i < NLINE - 1; i++)
	b[i] = s[i];
    b[i] = '\0';
    return b;
}

/* writeout is referenced by md_preview(); the tests do not save files. */
int writeout(Char *fn)
{
    (void)fn;
    return TRUE;
}

/* ================================================================ */
/*  Test fixtures                                                  */
/* ================================================================ */

static LINE *g_hdr = NULL;   /* buffer header (circular sentinel) */
static BUFFER *fixture_bp = NULL;
static WINDOW *fixture_wp = NULL;

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
    {
        LINE *cur = g_hdr->l_fp;
        while (cur != g_hdr && cur != NULL) {
            LINE *next = cur->l_fp;
            free((char *)cur);
            cur = next;
        }
    }
    free((char *)g_hdr);
    g_hdr = NULL;
}

static void reset_md_env(void)
{
    md_indent_width = 2;
    md_use_spaces = TRUE;
    md_auto_list = TRUE;
    md_heading_max = 6;
    md_list_bullets = md_test_bullets;
    md_fence_lang = md_test_empty;
    md_conceal = FALSE;
    md_auto_pair = FALSE;
    md_preview_on_save = FALSE;
    md_preview_command = md_test_preview_default;
    tabsize = 0;
    lastflag = 0;
    thisflag = 0;
    md_outline_src = NULL;
    md_test_url = NULL;
}

static void setup_buffer_common(int mode)
{
    if (fixture_bp == NULL) {
        fixture_bp = (BUFFER *)calloc(1, sizeof(BUFFER) +
				      2 * sizeof(Char) * NSTRING);
        fixture_wp = (WINDOW *)calloc(1, sizeof(WINDOW));
    }
    curbp = fixture_bp;
    curwp = fixture_wp;
    /* the fixture allocates space for the file/buffer name after the header */
    curbp->b_fname = (Char *)((char *)fixture_bp + sizeof(BUFFER));
    curbp->b_bname = curbp->b_fname + NSTRING;
    curbp->b_fname[0] = 0;
    curbp->b_bname[0] = 0;
    if (bheadp == NULL)
        bheadp = curbp;
    if (wheadp == NULL)
        wheadp = curwp;

    curbp->b_linep = g_hdr;
    curbp->b_nwnd  = 1;
    curbp->b_mode  = mode;
    curbp->b_flag  = 0;
    curbp->b_bufp  = NULL;
    curbp->b_dotp  = lforw(g_hdr);
    curbp->b_doto  = 0;

    curwp->w_bufp  = curbp;
    curwp->w_linep = lforw(g_hdr);
    curwp->w_dotp  = lforw(g_hdr);
    curwp->w_doto  = 0;
    curwp->w_flag  = 0;
    curwp->w_wndp  = NULL;
    curwp->w_markp = NULL;
    curwp->w_marko = 0;

    kbufh = NULL; kbufp = NULL; kused = KBLOCK;
    kterminal = FALSE;

    reset_md_env();
}

static void setup_single(const char *text, int doto, int mode)
{
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

    setup_buffer_common(mode);
    curwp->w_dotp  = text_line;
    curwp->w_doto  = doto;
    curwp->w_linep = text_line;
}

static void setup_multiline(char **lines, int nlines,
                            int dot_line, int dot_offset,
                            int mark_line, int mark_offset,
                            int mode)
{
    int i;
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

    setup_buffer_common(mode);

    lp = lforw(g_hdr);
    for (i = 0; i < dot_line && lforw(lp) != g_hdr; i++)
        lp = lforw(lp);
    curwp->w_dotp  = lp;
    curwp->w_doto  = dot_offset;
    curwp->w_linep = lp;

    if (mark_line >= 0) {
        lp = lforw(g_hdr);
        for (i = 0; i < mark_line && lforw(lp) != g_hdr; i++)
            lp = lforw(lp);
        curwp->w_markp = lp;
        curwp->w_marko = mark_offset;
    }
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

static int cur_text_to_cstr(char *buf, int bufsize)
{
    return line_text_to_cstr(curwp->w_dotp, buf, bufsize);
}

/* ================================================================ */
/*  mdinsert() tests                                                */
/* ================================================================ */

static void test_mdinsert_bullet(void)
{
    char buf[64];
    setup_single("- item", 6, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "- ", "mdinsert: bullet continues");
}

static void test_mdinsert_heading(void)
{
    char buf[64];
    setup_single("# Title", 7, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "mdinsert: heading -> no marker");
}

static void test_mdinsert_ordered(void)
{
    char buf[64];
    setup_single("3. one", 6, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "4. ", "mdinsert: ordered increments");
}

static void test_mdinsert_quote(void)
{
    char buf[64];
    setup_single("> quote", 7, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "> ", "mdinsert: quote continues");
}

static void test_mdinsert_empty_quote(void)
{
    char buf[64];
    setup_single(">", 1, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, ">", "mdinsert: empty quote -> bare >");
}

static void test_mdinsert_task(void)
{
    char buf[64];
    setup_single("- [x] done", 10, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "- [ ] ", "mdinsert: task resets to [ ]");
}

static void test_mdinsert_fence(void)
{
    char buf[64];
    setup_single("```", 3, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "mdinsert: fence -> indent only");
}

static void test_mdinsert_code_block(void)
{
    char buf[64];
    setup_single("    code", 8, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    ", "mdinsert: 4-space code inherits indent");
}

static void test_mdinsert_plain(void)
{
    char buf[64];
    setup_single("plain text", 10, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "mdinsert: plain text -> empty");
}

static void test_mdinsert_inside_fence(void)
{
    char buf[64];
    char *lines[] = {"```", "# not heading"};
    setup_multiline(lines, 2, 1, 13, -1, 0, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "mdinsert: fence suppresses markers");
}

static void test_mdinsert_auto_list_off(void)
{
    char buf[64];
    setup_single("- item", 6, MDMDMOD);
    md_auto_list = FALSE;
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "mdinsert: $md-auto-list FALSE");
}

static void test_mdinsert_indent(void)
{
    char buf[64];
    setup_single("  - item", 8, MDMDMOD);
    mdinsert();
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  - ", "mdinsert: indent + bullet");
}

/* ================================================================ */
/*  md_backspace() tests                                            */
/* ================================================================ */

static void test_md_backspace_marker(void)
{
    char buf[64];
    setup_single("- item", 2, MDMDMOD);
    md_backspace(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "item", "md_backspace: remove bullet");
}

static void test_md_backspace_indented_marker(void)
{
    char buf[64];
    setup_single("  - item", 4, MDMDMOD);
    md_backspace(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "item", "md_backspace: remove indent + bullet");
}

static void test_md_backspace_nested_quote(void)
{
    char buf[64];
    setup_single("> - item", 4, MDMDMOD);
    md_backspace(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "> item", "md_backspace: remove one level");
}

static void test_md_backspace_whitespace(void)
{
    char buf[64];
    setup_single("    x", 4, MDMDMOD);
    md_backspace(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  x", "md_backspace: dedent one level");
}

static void test_md_backspace_normal(void)
{
    char buf[64];
    setup_single("abc", 1, MDMDMOD);
    md_backspace(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "bc", "md_backspace: normal delete");
}

/* ================================================================ */
/*  mdindent() tests                                                */
/* ================================================================ */

static void test_mdindent_normalize(void)
{
    char buf[64];
    setup_single("     - item", 11, MDMDMOD);
    mdindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    - item", "mdindent: 5 -> 4 spaces");
}

static void test_mdindent_over_indent(void)
{
    char buf[64];
    char *lines[] = {"- parent", "    - child"};
    setup_multiline(lines, 2, 1, 11, -1, 0, MDMDMOD);
    mdindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  - child", "mdindent: cap one level above parent");
}

static void test_mdindent_quote_preserved(void)
{
    char buf[64];
    setup_single("  > quoted text", 15, MDMDMOD);
    mdindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  > quoted text", "mdindent: quote prefix preserved");
}

static void test_mdindent_fence_untouched(void)
{
    char buf[64];
    char *lines[] = {"```", "      code", "```"};
    setup_multiline(lines, 3, 1, 12, -1, 0, MDMDMOD);
    mdindent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "      code", "mdindent: fence contents untouched");
}

/* ================================================================ */
/*  Heading tests                                                   */
/* ================================================================ */

static void test_md_insert_heading(void)
{
    char buf[64];
    setup_single("", 0, MDMDMOD);
    md_insert_heading(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "# ", "md-insert-heading: H1");
}

static void test_md_insert_heading_level3(void)
{
    char buf[64];
    setup_single("", 0, MDMDMOD);
    md_insert_heading(TRUE, 3);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "### ", "md-insert-heading: H3");
}

static void test_md_heading_up(void)
{
    char buf[64];
    setup_single("# Title", 7, MDMDMOD);
    md_heading_up(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "## Title", "md-heading-up: H1 -> H2");
}

static void test_md_heading_up_nonheading(void)
{
    char buf[64];
    setup_single("text", 4, MDMDMOD);
    md_heading_up(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "# text", "md-heading-up: plain -> H1");
}

static void test_md_heading_down(void)
{
    char buf[64];
    setup_single("## Title", 8, MDMDMOD);
    md_heading_down(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "# Title", "md-heading-down: H2 -> H1");
}

static void test_md_heading_down_to_normal(void)
{
    char buf[64];
    setup_single("# Title", 7, MDMDMOD);
    md_heading_down(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "Title", "md-heading-down: H1 -> normal");
}

static void test_md_heading_cycle(void)
{
    char buf[64];
    setup_single("text", 4, MDMDMOD);
    md_heading_cycle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "# text", "md-heading-cycle: plain -> H1");
    md_heading_cycle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "## text", "md-heading-cycle: H1 -> H2");
}

static void test_md_heading_cycle_max(void)
{
    char buf[64];
    setup_single("###### Max", 10, MDMDMOD);
    md_heading_cycle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "Max", "md-heading-cycle: H6 -> normal");
}

/* ================================================================ */
/*  List tests                                                      */
/* ================================================================ */

static void test_md_list_toggle(void)
{
    char buf[64];
    setup_single("- item", 6, MDMDMOD);
    md_list_toggle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "* item", "md-list-toggle: - -> *");
}

static void test_md_list_toggle_cycle(void)
{
    char buf[64];
    setup_single("* item", 6, MDMDMOD);
    md_list_toggle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "+ item", "md-list-toggle: * -> +");
}

static void test_md_list_demote(void)
{
    char buf[64];
    setup_single("- item", 0, MDMDMOD);
    md_list_demote(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  - item", "md-list-demote: +2 spaces");
}

static void test_md_list_promote(void)
{
    char buf[64];
    setup_single("  - item", 4, MDMDMOD);
    md_list_promote(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "- item", "md-list-promote: -2 spaces");
}

static void test_md_list_renumber(void)
{
    char buf[64];
    char *lines[] = {"1. a", "1. b", "1. c"};
    LINE *lp;
    setup_multiline(lines, 3, 1, 4, -1, 0, MDMDMOD);
    md_list_renumber(FALSE, 1);
    lp = lforw(g_hdr);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "1. a", "renumber: line 1");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "2. b", "renumber: line 2");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "3. c", "renumber: line 3");
}

static void test_md_task_toggle(void)
{
    char buf[64];
    setup_single("- [ ] task", 10, MDMDMOD);
    md_task_toggle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "- [x] task", "md-task-toggle: [ ] -> [x]");
}

static void test_md_task_toggle_back(void)
{
    char buf[64];
    setup_single("- [X] task", 10, MDMDMOD);
    md_task_toggle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "- [ ] task", "md-task-toggle: [X] -> [ ]");
}

/* ================================================================ */
/*  Emphasis / link tests                                           */
/* ================================================================ */

static void test_md_bold_region(void)
{
    char buf[64];
    char *lines[] = {"hello world"};
    setup_multiline(lines, 1, 0, 5, 0, 0, MDMDMOD);
    md_bold(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "**hello** world", "md-bold: wrap region");
}

static void test_md_bold_toggle_off(void)
{
    char buf[64];
    char *lines[] = {"**hello** world"};
    setup_multiline(lines, 1, 0, 7, 0, 2, MDMDMOD);
    md_bold(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "hello world", "md-bold: unwrap region");
}

static void test_md_italic_word(void)
{
    char buf[64];
    setup_single("hello world", 0, MDMDMOD);
    md_italic(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "*hello* world", "md-italic: wrap word");
}

static void test_md_code_word(void)
{
    char buf[64];
    setup_single("hello world", 0, MDMDMOD);
    md_code(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "`hello` world", "md-code: wrap word");
}

static void test_md_insert_link_plain(void)
{
    char buf[64];
    setup_single("", 0, MDMDMOD);
    md_test_url = md_test_url_buf;
    md_insert_link(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "http://example.com", "md-insert-link: plain URL");
}

static void test_md_insert_link_region(void)
{
    char buf[64];
    char *lines[] = {"hello world"};
    setup_multiline(lines, 1, 0, 5, 0, 0, MDMDMOD);
    md_test_url = md_test_url_buf;
    md_insert_link(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "[hello](http://example.com) world",
                    "md-insert-link: region -> [text](url)");
}

/* ================================================================ */
/*  Quote tests                                                     */
/* ================================================================ */

static void test_md_quote_toggle_add(void)
{
    char buf[64];
    setup_single("plain", 5, MDMDMOD);
    md_quote_toggle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "> plain", "md-quote-toggle: add");
}

static void test_md_quote_toggle_remove(void)
{
    char buf[64];
    setup_single("> plain", 7, MDMDMOD);
    md_quote_toggle(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "plain", "md-quote-toggle: remove");
}

static void test_md_quote_region(void)
{
    char buf[64];
    char *lines[] = {"a", "b", "c"};
    LINE *lp;
    setup_multiline(lines, 3, 1, 1, 0, 0, MDMDMOD);
    md_quote_region(FALSE, 1);
    lp = lforw(g_hdr);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "> a", "md-quote-region: line 1");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "> b", "md-quote-region: line 2");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "c", "md-quote-region: line 3 untouched");
}

/* ================================================================ */
/*  Fence tests                                                     */
/* ================================================================ */

static void test_md_insert_fence(void)
{
    char buf[64];
    LINE *lp;
    setup_single("", 0, MDMDMOD);
    md_insert_fence(FALSE, 1);
    lp = lforw(g_hdr);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "```", "fence: opening");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "", "fence: empty middle line");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "```", "fence: closing");
    TEST_ASSERT(curwp->w_dotp == lforw(lforw(g_hdr)),
                "fence: cursor on middle line");
}

static void test_md_insert_fence_lang(void)
{
    char buf[64];
    LINE *lp;
    setup_single("", 0, MDMDMOD);
    md_fence_lang = md_test_lang_c;
    md_insert_fence(FALSE, 1);
    lp = lforw(g_hdr);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "```c", "fence: language info string");
}

static void test_md_goto_fence(void)
{
    char *lines[] = {"```", "code", "```"};
    LINE *target;
    setup_multiline(lines, 3, 1, 4, -1, 0, MDMDMOD);
    md_goto_fence(FALSE, 1);
    target = lforw(lforw(lforw(g_hdr)));
    TEST_ASSERT(curwp->w_dotp == target,
                "md-goto-fence: inside -> closing fence");
}

static void test_md_goto_fence_back(void)
{
    char *lines[] = {"```", "code", "```"};
    LINE *target;
    setup_multiline(lines, 3, 2, 3, -1, 0, MDMDMOD);
    md_goto_fence(FALSE, 1);
    target = lforw(g_hdr);
    TEST_ASSERT(curwp->w_dotp == target,
                "md-goto-fence: closing -> opening fence");
}

/* ================================================================ */
/*  Table tests                                                     */
/* ================================================================ */

static void test_md_table_next_cell(void)
{
    setup_single("| a | b |", 0, MDMDMOD);
    md_table_next_cell(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_doto, 4, "md-table-next-cell: before next |");
    md_table_next_cell(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_doto, 8, "md-table-next-cell: second cell");
}

static void test_md_tab_table(void)
{
    setup_single("| a | b |", 0, MDMDMOD);
    md_tab(FALSE, 1);
    TEST_ASSERT_INT(curwp->w_doto, 4, "md_tab: table row -> next cell");
}

static void test_md_tab_list_indent(void)
{
    char buf[64];
    setup_single("- item", 0, MDMDMOD);
    md_tab(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  - item", "md_tab: list head -> indent");
}

static void test_md_tab_plain(void)
{
    char buf[64];
    setup_single("abc", 0, MDMDMOD);
    md_tab(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    /* tabsize == 0 means "use real tabs" in kemacs */
    TEST_ASSERT_STR(buf, "\tabc", "md_tab: plain -> tab character");
}

/* ================================================================ */
/*  Outline tests                                                   */
/* ================================================================ */

static void test_md_outline_basic(void)
{
    char buf[64];
    char *lines[] = {"# One", "text", "## Two", "more", "### Three"};
    LINE *lp;

    setup_multiline(lines, 5, 0, 0, -1, 0, MDMDMOD);
    md_outline(FALSE, 1);

    /* curbp is now the outline buffer */
    TEST_ASSERT(curbp != fixture_bp, "outline: switched to outline buffer");
    lp = lforw(curbp->b_linep);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    1  H1  # One", "outline: line 1");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    3  H2  ## Two", "outline: line 2");
    lp = lforw(lp);
    line_text_to_cstr(lp, buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    5  H3  ### Three", "outline: line 3");
}

static void test_md_goto_heading(void)
{
    char *lines[] = {"# One", "text", "## Two", "more", "### Three"};
    LINE *first;

    setup_multiline(lines, 5, 0, 0, -1, 0, MDMDMOD);
    first = lforw(g_hdr);
    md_outline(FALSE, 1);
    md_goto_heading(FALSE, 1);
    TEST_ASSERT(curbp == fixture_bp, "goto-heading: back to source buffer");
    TEST_ASSERT(curwp->w_dotp == first, "goto-heading: first heading");
}

static void test_md_goto_heading_second(void)
{
    char *lines[] = {"# One", "text", "## Two", "more", "### Three"};
    LINE *third;

    setup_multiline(lines, 5, 0, 0, -1, 0, MDMDMOD);
    third = lforw(lforw(lforw(g_hdr)));
    md_outline(FALSE, 1);
    forwline(TRUE, 1);          /* outline line 2 -> source line 3 */
    md_goto_heading(FALSE, 1);
    TEST_ASSERT(curbp == fixture_bp, "goto-heading 2: source buffer");
    TEST_ASSERT(curwp->w_dotp == third, "goto-heading 2: second heading");
}

/* ================================================================ */
/*  Phase 3: conceal and modeline helpers                          */
/* ================================================================ */

static void test_md_draw_line_heading(void)
{
    char buf[64];
    setup_single("# Title", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  Title", "conceal: heading marker -> spaces");
}

static void test_md_draw_line_bullet(void)
{
    char buf[64];
    setup_single("- item", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  item", "conceal: bullet -> spaces");
}

static void test_md_draw_line_quote(void)
{
    char buf[64];
    setup_single("> quote", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "  quote", "conceal: quote -> spaces");
}

static void test_md_draw_line_ordered(void)
{
    char buf[64];
    setup_single("1. one", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "   one", "conceal: ordered marker -> spaces");
}

static void test_md_draw_line_task(void)
{
    char buf[64];
    setup_single("- [ ] task", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "      task", "conceal: task marker -> spaces");
}

static void test_md_draw_line_quote_list(void)
{
    char buf[64];
    setup_single("> - item", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    item", "conceal: quote + list -> spaces");
}

static void test_md_draw_line_fence_visible(void)
{
    char buf[64];
    setup_single("```", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "```", "conceal: fence stays visible");
}

static void test_md_draw_line_indented_code(void)
{
    char buf[64];
    setup_single("    # code", 0, MDMDMOD);
    vtputc_reset();
    md_draw_line(curwp->w_dotp);
    vtputc_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    # code", "conceal: 4-space code unchanged");
}

static void test_md_line_is_list(void)
{
    setup_single("- item", 0, MDMDMOD);
    TEST_ASSERT(md_line_is_list(curwp->w_dotp), "list: bullet");
    setup_single("plain", 0, MDMDMOD);
    TEST_ASSERT(!md_line_is_list(curwp->w_dotp), "list: plain");
    setup_single("> - item", 0, MDMDMOD);
    TEST_ASSERT(md_line_is_list(curwp->w_dotp), "list: quoted bullet");
}

static void test_md_current_heading(void)
{
    char *lines[] = {"# One", "text", "## Two"};
    Char hbuf[NLINE];
    int n;

    setup_multiline(lines, 3, 1, 0, -1, 0, MDMDMOD);
    n = md_current_heading(hbuf, (int)sizeof(hbuf), curwp->w_dotp);
    TEST_ASSERT(n > 0, "heading: found above");
    TEST_ASSERT_CSTR_EQ(hbuf, "# One", "heading: section above");
    setup_multiline(lines, 3, 2, 0, -1, 0, MDMDMOD);
    n = md_current_heading(hbuf, (int)sizeof(hbuf), curwp->w_dotp);
    TEST_ASSERT(n > 0, "heading: found self");
    TEST_ASSERT_CSTR_EQ(hbuf, "## Two", "heading: heading line itself");
    setup_multiline(lines, 3, 0, 0, -1, 0, MDMDMOD);
    n = md_current_heading(hbuf, (int)sizeof(hbuf), curwp->w_dotp);
    TEST_ASSERT(n > 0, "heading: found H1");
    TEST_ASSERT_CSTR_EQ(hbuf, "# One", "heading: H1 itself");
    {
	char *plain[] = {"text", "more"};
	setup_multiline(plain, 2, 0, 0, -1, 0, MDMDMOD);
	TEST_ASSERT(md_current_heading(hbuf, (int)sizeof(hbuf),
				       curwp->w_dotp) < 0,
		    "heading: none before first heading");
    }
}

/* ================================================================ */
/*  Phase 4: auto pair and external preview                        */
/* ================================================================ */

static void test_md_pair_char(void)
{
    TEST_ASSERT(md_pair_char('*'), "pair: *");
    TEST_ASSERT(md_pair_char('`'), "pair: `");
    TEST_ASSERT(md_pair_char('['), "pair: [");
    TEST_ASSERT(md_pair_char('_'), "pair: _");
    TEST_ASSERT(md_pair_char('~'), "pair: ~");
    TEST_ASSERT(!md_pair_char('a'), "pair: 'a' is not paired");
}

static void test_md_self_insert_star(void)
{
    char buf[64];
    setup_single("", 0, MDMDMOD);
    md_self_insert(1, '*');
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "**", "self-insert: **");
    TEST_ASSERT_INT(curwp->w_doto, 1, "self-insert: cursor between");
}

static void test_md_self_insert_bracket(void)
{
    char buf[64];
    setup_single("", 0, MDMDMOD);
    md_self_insert(1, '[');
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "[]", "self-insert: []");
    TEST_ASSERT_INT(curwp->w_doto, 1, "self-insert: cursor between");
}

static void test_md_build_preview_command(void)
{
    static Char prev[] = { 'g','r','i','p',' ','%','f', 0 };
    static Char fn[] = { '/','t','m','p','/','x','.','m','d', 0 };
    Char cmd[NLINE * 2];

    md_preview_command = prev;
    md_build_preview_command(cmd, (int)sizeof(cmd), fn);
    TEST_ASSERT_CSTR_EQ(cmd, "grip /tmp/x.md", "preview: %f substituted");
}

static void test_md_build_preview_command_twice(void)
{
    static Char prev[] = { 'p','a','n','d','o','c',' ','-','s',' ','%','f',
			   ' ','-','o',' ','%','f','.','h','t','m','l', 0 };
    static Char fn[] = { '/','t','m','p','/','x','.','m','d', 0 };
    Char cmd[NLINE * 2];

    md_preview_command = prev;
    md_build_preview_command(cmd, (int)sizeof(cmd), fn);
    TEST_ASSERT_CSTR_EQ(cmd, "pandoc -s /tmp/x.md -o /tmp/x.md.html",
			"preview: both %f substituted");
}

static void test_md_build_preview_command_none(void)
{
    static Char prev[] = { 'e','c','h','o',' ','h','i', 0 };
    static Char fn[] = { '/','t','m','p','/','x','.','m','d', 0 };
    Char cmd[NLINE * 2];

    md_preview_command = prev;
    md_build_preview_command(cmd, (int)sizeof(cmd), fn);
    TEST_ASSERT_CSTR_EQ(cmd, "echo hi", "preview: no %f");
}

static void test_md_preview_no_filename(void)
{
    setup_single("# hi", 0, MDMDMOD);
    curbp->b_fname[0] = 0;
    spawn_called = 0;
    spawn_last_cmd = NULL;
    spawn_last_tmp = NULL;
    spawn_last_bname = NULL;
    md_preview(FALSE, 1);
    TEST_ASSERT(spawn_called, "preview: ran for unnamed buffer");
    {
	const char *expect = "grip /tmp/kemacs_md_";
	int i, ok = 1;
	for (i = 0; expect[i] != '\0'; i++)
	    if ((char)spawn_last_cmd[i] != expect[i]) {
		ok = 0;
		break;
	    }
	TEST_ASSERT(ok, "preview: temp file path used");
    }
    TEST_ASSERT_CSTR_EQ(spawn_last_bname, "*preview*",
		"preview: output buffer name");
    {
	const char *expect = "/tmp/kemacs_md_";
	int i, ok = 1;
	for (i = 0; expect[i] != '\0'; i++)
	    if ((char)spawn_last_tmp[i] != expect[i]) {
		ok = 0;
		break;
	    }
	TEST_ASSERT(ok, "preview: temp file kept for cleanup");
    }
}

static void test_md_conceal_toggle(void)
{
    setup_single("# t", 0, MDMDMOD);
    md_conceal = FALSE;
    md_conceal_toggle(FALSE, 1);
    TEST_ASSERT(md_conceal, "conceal toggle: ON");
    md_conceal_toggle(FALSE, 1);
    TEST_ASSERT(!md_conceal, "conceal toggle: OFF");
}

static void test_md_preview_uses_filename(void)
{
    static Char fn[] = { '/','t','m','p','/','x','.','m','d', 0 };
    setup_single("# hi", 0, MDMDMOD);
    /* give the fixture buffer a file name */
    {
	int i;
	for (i = 0; fn[i]; i++)
	    curbp->b_fname[i] = fn[i];
	curbp->b_fname[i] = 0;
    }
    spawn_called = 0;
    spawn_last_cmd = NULL;
    spawn_last_tmp = (Char *)1;	/* poison: must stay unset */
    md_preview(FALSE, 1);
    TEST_ASSERT(spawn_called, "preview: command ran");
    TEST_ASSERT_CSTR_EQ(spawn_last_cmd, "grip /tmp/x.md",
			"preview: command uses file name");
    TEST_ASSERT(spawn_last_tmp == NULL, "preview: no temp file for named buffer");
    TEST_ASSERT_CSTR_EQ(spawn_last_bname, "*preview*",
			"preview: output buffer name");
}

/* ================================================================ */
/*  Mode + automode tests                                           */
/* ================================================================ */

static void test_md_mode_toggle(void)
{
    setup_single("x", 1, 0);
    md_mode(FALSE, 1);
    TEST_ASSERT(curbp->b_mode & MDMDMOD, "md_mode: toggle ON");
    md_mode(FALSE, 1);
    TEST_ASSERT(!(curbp->b_mode & MDMDMOD), "md_mode: toggle OFF");
}

static void test_md_mode_set_on_off(void)
{
    setup_single("x", 1, 0);
    md_mode(TRUE, 1);
    TEST_ASSERT(curbp->b_mode & MDMDMOD, "md_mode: arg > 0 -> ON");
    md_mode(TRUE, -1);
    TEST_ASSERT(!(curbp->b_mode & MDMDMOD), "md_mode: arg < 0 -> OFF");
}

static void test_md_automode_table(void)
{
    TEST_ASSERT_INT(npatent, 4, "automode: 4 patterns");
    TEST_ASSERT_INT(pattbl[2].mode, MDMDMOD, "automode: .md -> MDMDMOD");
    TEST_ASSERT_INT(pattbl[3].mode, MDMDMOD, "automode: .markdown -> MDMDMOD");
}

static void test_smart_indent_md(void)
{
    char buf[64];
    setup_single("     - item", 11, MDMDMOD);
    smart_indent(FALSE, 1);
    cur_text_to_cstr(buf, sizeof(buf));
    TEST_ASSERT_STR(buf, "    - item", "smart_indent: dispatches to mdindent");
}

/* ================================================================ */
/*  Test registry                                                  */
/* ================================================================ */

TEST_LIST({
    /* mdinsert */
    {"mdinsert bullet",              test_mdinsert_bullet},
    {"mdinsert heading",             test_mdinsert_heading},
    {"mdinsert ordered",             test_mdinsert_ordered},
    {"mdinsert quote",               test_mdinsert_quote},
    {"mdinsert empty quote",         test_mdinsert_empty_quote},
    {"mdinsert task",                test_mdinsert_task},
    {"mdinsert fence",               test_mdinsert_fence},
    {"mdinsert code block",          test_mdinsert_code_block},
    {"mdinsert plain",               test_mdinsert_plain},
    {"mdinsert inside fence",        test_mdinsert_inside_fence},
    {"mdinsert auto-list off",       test_mdinsert_auto_list_off},
    {"mdinsert indent",              test_mdinsert_indent},

    /* md_backspace */
    {"md_backspace marker",          test_md_backspace_marker},
    {"md_backspace indented marker", test_md_backspace_indented_marker},
    {"md_backspace nested quote",    test_md_backspace_nested_quote},
    {"md_backspace whitespace",      test_md_backspace_whitespace},
    {"md_backspace normal",          test_md_backspace_normal},

    /* mdindent */
    {"mdindent normalize",           test_mdindent_normalize},
    {"mdindent over-indent",         test_mdindent_over_indent},
    {"mdindent quote preserved",     test_mdindent_quote_preserved},
    {"mdindent fence untouched",     test_mdindent_fence_untouched},

    /* headings */
    {"md-insert-heading",            test_md_insert_heading},
    {"md-insert-heading H3",         test_md_insert_heading_level3},
    {"md-heading-up",                test_md_heading_up},
    {"md-heading-up non-heading",    test_md_heading_up_nonheading},
    {"md-heading-down",              test_md_heading_down},
    {"md-heading-down to normal",    test_md_heading_down_to_normal},
    {"md-heading-cycle",             test_md_heading_cycle},
    {"md-heading-cycle max",         test_md_heading_cycle_max},

    /* lists */
    {"md-list-toggle",               test_md_list_toggle},
    {"md-list-toggle cycle",         test_md_list_toggle_cycle},
    {"md-list-demote",               test_md_list_demote},
    {"md-list-promote",              test_md_list_promote},
    {"md-list-renumber",             test_md_list_renumber},
    {"md-task-toggle",               test_md_task_toggle},
    {"md-task-toggle back",          test_md_task_toggle_back},

    /* emphasis / links */
    {"md-bold region",               test_md_bold_region},
    {"md-bold toggle off",           test_md_bold_toggle_off},
    {"md-italic word",               test_md_italic_word},
    {"md-code word",                 test_md_code_word},
    {"md-insert-link plain",         test_md_insert_link_plain},
    {"md-insert-link region",        test_md_insert_link_region},

    /* quotes */
    {"md-quote-toggle add",          test_md_quote_toggle_add},
    {"md-quote-toggle remove",       test_md_quote_toggle_remove},
    {"md-quote-region",              test_md_quote_region},

    /* fences */
    {"md-insert-fence",              test_md_insert_fence},
    {"md-insert-fence lang",         test_md_insert_fence_lang},
    {"md-goto-fence",                test_md_goto_fence},
    {"md-goto-fence back",           test_md_goto_fence_back},

    /* tables */
    {"md-table-next-cell",           test_md_table_next_cell},
    {"md-tab table",                 test_md_tab_table},
    {"md-tab list indent",           test_md_tab_list_indent},
    {"md-tab plain",                 test_md_tab_plain},

    /* outline */
    {"md-outline basic",             test_md_outline_basic},
    {"md-goto-heading",              test_md_goto_heading},
    {"md-goto-heading second",       test_md_goto_heading_second},

    /* Phase 3: conceal + modeline helpers */
    {"conceal heading",              test_md_draw_line_heading},
    {"conceal bullet",               test_md_draw_line_bullet},
    {"conceal quote",                test_md_draw_line_quote},
    {"conceal ordered",              test_md_draw_line_ordered},
    {"conceal task",                 test_md_draw_line_task},
    {"conceal quote+list",           test_md_draw_line_quote_list},
    {"conceal fence visible",        test_md_draw_line_fence_visible},
    {"conceal indented code",        test_md_draw_line_indented_code},
    {"md-line-is-list",              test_md_line_is_list},
    {"md-current-heading",           test_md_current_heading},

    /* Phase 4: auto pair + preview */
    {"md-pair-char",                 test_md_pair_char},
    {"md-self-insert star",          test_md_self_insert_star},
    {"md-self-insert bracket",       test_md_self_insert_bracket},
    {"md-build-preview-command",     test_md_build_preview_command},
    {"md-build-preview-command 2",   test_md_build_preview_command_twice},
    {"md-build-preview-command none",test_md_build_preview_command_none},
    {"md-preview uses filename",     test_md_preview_uses_filename},
    {"md-conceal-toggle",            test_md_conceal_toggle},
    {"md-preview no filename",       test_md_preview_no_filename},

    /* mode / automode */
    {"md_mode toggle",               test_md_mode_toggle},
    {"md_mode set on/off",           test_md_mode_set_on_off},
    {"md automode table",            test_md_automode_table},
    {"smart_indent md",              test_smart_indent_md},
})

int main(void)
{
    RUN_TESTS();
}
