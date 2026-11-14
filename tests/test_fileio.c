#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int mlwrite();

#define maindef
#include "../ecomm.h"
#include "../estruct.h"
#include "../edef.h"
#include "../fileio.c"
#undef maindef

int mlwrite() { return TRUE; }
TERM term;

static void test_ffropen_missing(void)
{
    Char *fn = Cfromc("/tmp/kemacs_fileio_no_such_file_xyz.txt");
    int rc = ffropen(fn, 0);
    TEST_ASSERT_INT(rc, FIOFNF, "missing file returns FIOFNF");
    Free(fn);
}

static void test_roundtrip(void)
{
    char tmpname[] = "/tmp/kemacs_fileio_roundtrip.txt";
    Char *fn = Cfromc(tmpname);
    Char buf[NLINE];
    int rc;
    rc = ffwopen(fn, 0);
    TEST_ASSERT_INT(rc, FIOSUC, "ffwopen success");
    Char line[] = {'h','i',0};
    rc = ffputline(line, 2);
    TEST_ASSERT_INT(rc, FIOSUC, "ffputline success");
    rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "ffclose write success");
    rc = ffropen(fn, 0);
    TEST_ASSERT_INT(rc, FIOSUC, "ffropen success");
    rc = ffgetline(buf, NLINE);
    TEST_ASSERT_INT(rc, FIOSUC, "ffgetline success");
    TEST_ASSERT_INT(buf[0], (Char)'h', "content[0]");
    TEST_ASSERT_INT(buf[1], (Char)'i', "content[1]");
    TEST_ASSERT_INT(buf[2], 0, "content null-terminated");
    rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "ffclose read success");
    unlink(tmpname);
    Free(fn);
}

static void test_ffgetline_eof(void)
{
    char tmpname[] = "/tmp/kemacs_fileio_empty.txt";
    FILE *f = fopen(tmpname, "w"); if (f) fclose(f);
    Char *fn = Cfromc(tmpname);
    Char buf[NLINE];
    int rc;
    rc = ffropen(fn, 0);
    TEST_ASSERT_INT(rc, FIOSUC, "open empty");
    rc = ffgetline(buf, NLINE);
    TEST_ASSERT_INT(rc, FIOEOF, "empty file returns FIOEOF");
    rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "close");
    unlink(tmpname);
    Free(fn);
}

static void test_ffgetline_long(void)
{
    char tmpname[] = "/tmp/kemacs_fileio_long.txt";
    FILE *f = fopen(tmpname, "w");
    if (f) { fputs("abcdefghijklmnopqrstuvwxyz", f); fclose(f); }
    Char *fn = Cfromc(tmpname);
    Char buf[8];
    int rc;
    rc = ffropen(fn, 0);
    TEST_ASSERT_INT(rc, FIOSUC, "open long");
    rc = ffgetline(buf, 8);
    TEST_ASSERT_INT(rc, FIOLNG, "long line returns FIOLNG");
    rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "close");
    unlink(tmpname);
    Free(fn);
}

static void test_ffclose_null(void)
{
    int rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "ffclose on NULL ffp returns success");
}

/*
 * Cfromc() must decode the locale (terminal-code) byte string into internal
 * Chars, and cfromC() must encode back to the identical bytes.  Before the
 * fix both were byte-transparent, so a Japanese file name held raw UTF-8
 * bytes and the modeline rendered them as octal escapes (\343\203\206...).
 */
static void test_japanese_filename_decode(void)
{
    char tmpname[] = "/tmp/kemacs_fileio_テスト.txt";
    Char *fn = Cfromc(tmpname);
    char *back;

    /* テスト is 9 UTF-8 bytes but only 3 internal Chars. */
    TEST_ASSERT_INT(Cstrlen(fn), (int)strlen(tmpname) - 6,
                    "Cfromc decodes UTF-8 multibyte sequences");
    /* The internal Char must not be a raw UTF-8 lead byte (0xE3). */
    TEST_ASSERT(fn[19] != (Char)0xE3,
                "Cfromc result is internal kanji, not a raw UTF-8 byte");

    back = cfromC(fn);
    TEST_ASSERT_STR(back, tmpname, "cfromC round-trips to the original bytes");
}

static void test_japanese_filename_io(void)
{
    char tmpname[] = "/tmp/kemacs_fileio_テスト.txt";
    Char *fn = Cfromc(tmpname);
    Char buf[NLINE];
    int rc;

    rc = ffwopen(fn, 0);
    TEST_ASSERT_INT(rc, FIOSUC, "ffwopen with Japanese name");
    {
        Char line[] = {'h','i',0};
        rc = ffputline(line, 2);
        TEST_ASSERT_INT(rc, FIOSUC, "ffputline");
    }
    rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "ffclose write");

    rc = ffropen(fn, 0);
    TEST_ASSERT_INT(rc, FIOSUC, "ffropen with Japanese name");
    rc = ffgetline(buf, NLINE);
    TEST_ASSERT_INT(rc, FIOSUC, "ffgetline");
    TEST_ASSERT_INT(buf[0], (Char)'h', "content[0]");
    TEST_ASSERT_INT(buf[1], (Char)'i', "content[1]");
    rc = ffclose(NULL);
    TEST_ASSERT_INT(rc, FIOSUC, "ffclose read");

    unlink(tmpname);
    Free(fn);
}

TEST_LIST({
    {"test_ffropen_missing", test_ffropen_missing},
    {"test_roundtrip", test_roundtrip},
    {"test_ffgetline_eof", test_ffgetline_eof},
    {"test_ffgetline_long", test_ffgetline_long},
    {"test_ffclose_null", test_ffclose_null},
    {"test_japanese_filename_decode", test_japanese_filename_decode},
    {"test_japanese_filename_io", test_japanese_filename_io}
})

int main(void)
{
    RUN_TESTS();
}
