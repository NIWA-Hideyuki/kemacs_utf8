/*
 * test_eval.c -- Unit tests for kemacs expression evaluator
 * helper functions from eval.c.
 *
 * Tests: stoi, itoaa, gettyp, stol, ltos
 *
 * eval.c is NOT included via #include (it has duplicate static
 * variable names).  Instead, test_eval.o is linked against a
 * separately-compiled eval.o that was built with
 * -ffunction-sections -fdata-sections.  The linker (--gc-sections)
 * retains only stoi, itoaa, gettyp, stol, ltos and drops the
 * rest of eval.c, which has heavy kemacs runtime dependencies.
 * Cxstr is provided by ../Css.o.
 */
#include "test.h"
#include <stdlib.h>
#include <string.h>

/* ---- kemacs globals (for eval.o's extern references) ---- */
#define maindef
#include "../ecomm.h"
#include "../estruct.h"
#include "../edef.h"
#undef maindef

/* Forward declarations for eval functions under test.
   (stoi and gettyp are not in edef.h; declared here.) */
extern int stoi(Char *);
extern int gettyp(Char *);

/* ---- Helper: copy a C string into a Char array ---- */
static void to_char(Char *dst, const char *src, int n)
{
    int i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = (unsigned char)src[i];
    dst[i] = 0;
}

/* ---- Tests for stoi() ---- */
static void test_stoi_basic(void)
{
    Char s[32];
    to_char(s, "42", 32);
    TEST_ASSERT_INT(stoi(s), 42, "stoi: 42");

    to_char(s, "0", 32);
    TEST_ASSERT_INT(stoi(s), 0, "stoi: 0");

    to_char(s, "-1", 32);
    TEST_ASSERT_INT(stoi(s), -1, "stoi: -1");

    to_char(s, "-100", 32);
    TEST_ASSERT_INT(stoi(s), -100, "stoi: -100");

    to_char(s, "  42", 32);
    TEST_ASSERT_INT(stoi(s), 42, "stoi: leading spaces");

    to_char(s, "123abc", 32);
    TEST_ASSERT_INT(stoi(s), 0, "stoi: non-digit stops parse");

    to_char(s, "abc", 32);
    TEST_ASSERT_INT(stoi(s), 0, "stoi: all non-digit");

    (void)to_char;
}

static void test_stoi_edge(void)
{
    Char s[32];
    to_char(s, "", 32);
    TEST_ASSERT_INT(stoi(s), 0, "stoi: empty string");

    to_char(s, "+42", 32);
    TEST_ASSERT_INT(stoi(s), 42, "stoi: explicit +");

    to_char(s, "-", 32);
    TEST_ASSERT_INT(stoi(s), 0, "stoi: sign only");

    to_char(s, "  -5", 32);
    TEST_ASSERT_INT(stoi(s), -5, "stoi: spaces then sign");
}

/* ---- Tests for itoaa() ---- */
static void test_itoaa_basic(void)
{
    Char *p;

    p = itoaa(0);
    TEST_ASSERT(p != NULL, "itoaa(0) not NULL");
    TEST_ASSERT_INT(p[0], '0', "itoaa(0) first char");
    TEST_ASSERT_INT(p[1], 0,   "itoaa(0) terminator");

    p = itoaa(42);
    TEST_ASSERT_CSTR_EQ(p, "42", "itoaa(42)");

    p = itoaa(1);
    TEST_ASSERT_CSTR_EQ(p, "1", "itoaa(1)");

    p = itoaa(100);
    TEST_ASSERT_CSTR_EQ(p, "100", "itoaa(100)");
}

static void test_itoaa_negative(void)
{
    Char *p;

    p = itoaa(-1);
    TEST_ASSERT_CSTR_EQ(p, "-1", "itoaa(-1)");

    p = itoaa(-100);
    TEST_ASSERT_CSTR_EQ(p, "-100", "itoaa(-100)");
}

/* ---- Tests for gettyp() ---- */
static void test_gettyp_literals(void)
{
    Char tok[64];

    to_char(tok, "hello", 64);
    TEST_ASSERT_INT(gettyp(tok), TKCMD, "plain command");

    to_char(tok, "0", 64);
    TEST_ASSERT_INT(gettyp(tok), TKLIT, "numeric literal");

    to_char(tok, "123", 64);
    TEST_ASSERT_INT(gettyp(tok), TKLIT, "multi-digit literal");

    to_char(tok, "", 64);
    TEST_ASSERT_INT(gettyp(tok), TKNUL, "empty token");
}

static void test_gettyp_quoted(void)
{
    Char tok[64];

    to_char(tok, "\"hello\"", 64);
    TEST_ASSERT_INT(gettyp(tok), TKSTR, "quoted string");
}

static void test_gettyp_prefixes(void)
{
    Char tok[64];

    to_char(tok, "!foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKDIR, "directive (!)");

    to_char(tok, "@foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKARG, "argument (@)");

    to_char(tok, "#foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKBUF, "buffer (#)");

    to_char(tok, "$foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKENV, "environment ($)");

    to_char(tok, "%foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKVAR, "variable (%)");

    to_char(tok, "&foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKFUN, "function (&)");

    to_char(tok, "*foo", 64);
    TEST_ASSERT_INT(gettyp(tok), TKLBL, "label (*)");
}

/* ---- Tests for stol() ---- */
static void test_stol(void)
{
    Char tok[64];

    to_char(tok, "T", 64);
    TEST_ASSERT_INT(stol(tok), TRUE, "stol: T -> TRUE");

    to_char(tok, "t", 64);
    TEST_ASSERT_INT(stol(tok), TRUE, "stol: t -> TRUE");

    to_char(tok, "F", 64);
    TEST_ASSERT_INT(stol(tok), FALSE, "stol: F -> FALSE");

    to_char(tok, "f", 64);
    TEST_ASSERT_INT(stol(tok), FALSE, "stol: f -> FALSE");

    to_char(tok, "0", 64);
    TEST_ASSERT_INT(stol(tok), FALSE, "stol: 0 -> FALSE");

    to_char(tok, "42", 64);
    TEST_ASSERT_INT(stol(tok), TRUE, "stol: 42 -> TRUE");

    to_char(tok, "1", 64);
    TEST_ASSERT_INT(stol(tok), TRUE, "stol: 1 -> TRUE");

    to_char(tok, "", 64);
    TEST_ASSERT_INT(stol(tok), FALSE, "stol: empty -> FALSE");
}

/* ---- Tests for ltos() ---- */
static void test_ltos(void)
{
    TEST_ASSERT_EQ(ltos(TRUE), truem, "ltos(TRUE) == truem");
    TEST_ASSERT_EQ(ltos(FALSE), falsem, "ltos(FALSE) == falsem");

    /* Verify truem/falsem point to "TRUE"/"FALSE" in Cxstr */
    TEST_ASSERT_INT(truem[0], 'T', "truem starts with 'T'");
    TEST_ASSERT_INT(truem[1], 'R', "truem[1] = 'R'");
    TEST_ASSERT_INT(falsem[0], 'F', "falsem starts with 'F'");
    TEST_ASSERT_INT(falsem[1], 'A', "falsem[1] = 'A'");
}

/* ---- Test registry ---- */
TEST_LIST({
    {"stoi_basic",     test_stoi_basic},
    {"stoi_edge",      test_stoi_edge},
    {"itoaa_basic",    test_itoaa_basic},
    {"itoaa_negative", test_itoaa_negative},
    {"gettyp_literals",test_gettyp_literals},
    {"gettyp_quoted",  test_gettyp_quoted},
    {"gettyp_prefixes",test_gettyp_prefixes},
    {"stol",           test_stol},
    {"ltos",           test_ltos},
})

int main(void)
{
    RUN_TESTS();
}
