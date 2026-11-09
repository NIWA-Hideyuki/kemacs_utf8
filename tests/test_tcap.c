#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void kkopen(void);
void kkclose(void);
#define maindef
#include "../tcap.c"
#undef maindef

/* stubs */
int ttclose(void) { return 0; }
Char ttgetc(void) { return 0; }
static char cap_buf[256];
static int cap_len = 0;
static void reset_cap(void) { cap_len = 0; cap_buf[0] = '\0'; }
int ttputc(Char c)
{
    if (cap_len < (int)sizeof(cap_buf) - 1) cap_buf[cap_len++] = (char)c;
    cap_buf[cap_len] = '\0';
    return (int)c;
}
int ttopen(void) { return 0; }
int ttflush(void) { return 0; }
void kkopen(void) { }
void kkclose(void) { }
int tcapkopen(void);
int tcapkclose(void);

/* capture */

static void test_out_str_null(void)
{
    reset_cap();
    out_str(NULL);
    TEST_ASSERT_INT(cap_len, 0, "out_str(NULL) no output");
}

static void test_out_str_empty(void)
{
    reset_cap();
    out_str("");
    TEST_ASSERT_INT(cap_len, 0, "out_str empty no output");
}

static void test_out_str_basic(void)
{
    reset_cap();
    out_str("abc");
    TEST_ASSERT_INT(cap_len, 3, "out_str length");
    TEST_ASSERT_STR(cap_buf, "abc", "out_str content");
}

static void test_putpad_null(void)
{
    reset_cap();
    putpad(NULL);
    TEST_ASSERT_INT(cap_len, 0, "putpad(NULL) no output");
}

static void test_putnpad_null(void)
{
    reset_cap();
    putnpad(NULL, 5);
    TEST_ASSERT_INT(cap_len, 0, "putnpad(NULL) no output");
}

static void test_putpad_str(void)
{
    reset_cap();
    putpad("abc");
    TEST_ASSERT(cap_len >= 3, "putpad outputs at least input");
    TEST_ASSERT(memcmp(cap_buf, "abc", 3) == 0, "putpad prefix");
}

static void test_tcapbeep(void)
{
    reset_cap();
    tcapbeep();
    TEST_ASSERT(cap_len >= 1, "beep outputs");
    TEST_ASSERT(cap_buf[0] == 0x07, "beep is BEL");
}

static void test_tcaprev_no_crash(void)
{
    reset_cap();
    tcaprev(TRUE);
    tcaprev(FALSE);
    tcaprev(TRUE);
    TEST_ASSERT(1, "tcaprev no crash");
}

static void test_tcapmove_no_crash(void)
{
    reset_cap();
    tcapmove(0, 0);
    tcapmove(5, 10);
    TEST_ASSERT(1, "tcapmove no crash");
}

static void test_tcapeeol_no_crash(void)
{
    reset_cap();
    tcapeeol();
    TEST_ASSERT(1, "tcapeeol no crash");
}

static void test_tcapeeop_no_crash(void)
{
    reset_cap();
    tcapeeop();
    TEST_ASSERT(1, "tcapeeop no crash");
}

static void test_tcapkopen_close_no_crash(void)
{
    reset_cap();
    tcapkopen();
    tcapkclose();
    TEST_ASSERT(1, "tcapkopen/close no crash");
}

TEST_LIST({
    {"test_out_str_null", test_out_str_null},
    {"test_out_str_empty", test_out_str_empty},
    {"test_out_str_basic", test_out_str_basic},
    {"test_putpad_null", test_putpad_null},
    {"test_putnpad_null", test_putnpad_null},
    {"test_putpad_str", test_putpad_str},
    {"test_tcapbeep", test_tcapbeep},
    {"test_tcaprev_no_crash", test_tcaprev_no_crash},
    {"test_tcapmove_no_crash", test_tcapmove_no_crash},
    {"test_tcapeeol_no_crash", test_tcapeeol_no_crash},
    {"test_tcapeeop_no_crash", test_tcapeeop_no_crash},
    {"test_tcapkopen_close_no_crash", test_tcapkopen_close_no_crash}
})

int main(void)
{
    RUN_TESTS();
}
