#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* stubs (declarations before includes not needed; define after) */

#define maindef
#define static
#include "../ecomm.h"
#include "../estruct.h"
#include "../edef.h"
#include "../proto.h"
#include "../spawn.c"
#undef static
#undef maindef

TERM term;   /* satisfy extern; zero-initialized */
int ttopen(void) { return 0; }
int millisleep(int f, int n) { (void)f; (void)n; return 0; }
int mlwrite(Char *fmt, ...) { (void)fmt; return 0; }
int rdonly(void) { return FALSE; }

int resterr(void) { return FALSE; }
int vttidy(void) { return 0; }
int movecursor(int r, int c) { (void)r; (void)c; return 0; }
int mlputs(Char *s) { (void)s; return 0; }
int mlrepl(Char *p, Char *b, int n) { (void)p; (void)b; (void)n; return TRUE; }
int mlreply(Char *p, Char *b, int n) { (void)p; (void)b; (void)n; return TRUE; }
BUFFER *bfind(Char *n, int f, int a) { (void)n; (void)f; (void)a; return NULL; }
int onlywind(int f, int n) { (void)f; (void)n; return 0; }
int zotbuf(BUFFER *bp) { (void)bp; return 0; }
int splitwind(int f, int n) { (void)f; (void)n; return 0; }
int getfile(Char *f, int n) { (void)f; (void)n; return 0; }
int copystr(Char **a, Char *b) { (void)a; (void)b; return 0; }
int upmode(void) { return 0; }
int writeout(Char *f) { (void)f; return 0; }
int readin(Char *f, int n) { (void)f; (void)n; return 0; }
Char *safeC(Char *c) { return c; }
Char tgetc(void) { return 0; }

static void test_escape_basic(void)
{
    char *r = shell_command_escape_char("hello");
    TEST_ASSERT(r != NULL, "non-null");
    TEST_ASSERT_STR(r, "'hello'", "basic quoting");
}

static void test_escape_empty(void)
{
    char *r = shell_command_escape_char("");
    TEST_ASSERT(r != NULL, "non-null");
    TEST_ASSERT_STR(r, "''", "empty string");
}

static void test_escape_single_quote(void)
{
    char *r = shell_command_escape_char("it's");
    TEST_ASSERT(r != NULL, "non-null");
    TEST_ASSERT_STR(r, "'it'\\''s'", "single quote escaped");
}

static void test_escape_null(void)
{
    char *r = shell_command_escape_char(NULL);
    TEST_ASSERT(r != NULL, "null treated as empty");
    TEST_ASSERT_STR(r, "''", "null -> empty");
}

static void test_escape_overflow(void)
{
    char big[6000];
    memset(big, 'a', sizeof(big)-1);
    big[sizeof(big)-1] = '\0';
    char *r = shell_command_escape_char(big);
    TEST_ASSERT(r == NULL, "overflow returns NULL");
}

TEST_LIST({
    {"test_escape_basic", test_escape_basic},
    {"test_escape_empty", test_escape_empty},
    {"test_escape_single_quote", test_escape_single_quote},
    {"test_escape_null", test_escape_null},
    {"test_escape_overflow", test_escape_overflow}
})

int main(void)
{
    RUN_TESTS();
}
