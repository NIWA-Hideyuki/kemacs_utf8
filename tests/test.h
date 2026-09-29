/*
 * test.h -- Minimal C unit-test framework for kemacs tests.
 *
 * This is a deliberately small, dependency-free test harness.
 * It provides assertion macros, a test-list primitive, and
 * pass/fail tallying.  Each test file defines TEST_LIST (an
 * array of {name, func} pairs) and calls RUN_TESTS() from main().
 */
#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_count  = 0;   /* total assertions run  */
static int test_passed = 0;   /* assertions that passed */
static int test_failed = 0;   /* assertions that failed */
static int test_fails  = 0;   /* test *functions* with >= 1 failure */

#define TEST_OK      1
#define TEST_FAIL    0

/* ---- assertion helpers ---- */

#define TEST_ASSERT(cond, msg)                                        \
    do {                                                              \
        test_count++;                                                 \
        if (cond) {                                                   \
            test_passed++;                                            \
        } else {                                                      \
            test_failed++;                                            \
            test_fails++;                                             \
            printf("  FAIL: %s (%s:%d): %s\n",                        \
                   __func__, __FILE__, __LINE__, msg);               \
        }                                                             \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, msg)                         \
    do {                                                              \
        test_count++;                                                 \
        if ((actual) == (expected)) {                                 \
            test_passed++;                                            \
        } else {                                                      \
            test_failed++;                                            \
            test_fails++;                                             \
            printf("  FAIL: %s (%s:%d): %s -- "                       \
                   "expected 0x%lx but got 0x%lx\n",                  \
                   __func__, __FILE__, __LINE__, msg,                 \
                   (unsigned long)(expected),                         \
                   (unsigned long)(actual));                          \
        }                                                             \
    } while (0)

#define TEST_ASSERT_INT(actual, expected, msg)                      \
    do {                                                              \
        test_count++;                                                 \
        if ((actual) == (expected)) {                                 \
            test_passed++;                                            \
        } else {                                                      \
            test_failed++;                                            \
            test_fails++;                                             \
            printf("  FAIL: %s (%s:%d): %s -- "                       \
                   "expected %d but got %d\n",                        \
                   __func__, __FILE__, __LINE__, msg,                 \
                   (int)(expected), (int)(actual));                  \
        }                                                             \
    } while (0)

#define TEST_ASSERT_STR(actual, expected, msg)                      \
    do {                                                              \
        test_count++;                                                 \
        if (strcmp((actual), (expected)) == 0) {                     \
            test_passed++;                                            \
        } else {                                                      \
            test_failed++;                                            \
            test_fails++;                                             \
            printf("  FAIL: %s (%s:%d): %s -- "                       \
                   "expected \"%s\" but got \"%s\"\n",                \
                   __func__, __FILE__, __LINE__, msg,                 \
                   (expected), (actual));                            \
        }                                                             \
    } while (0)

#define TEST_ASSERT_BETWEEN(val, lo, hi, msg)                       \
    do {                                                              \
        test_count++;                                                 \
        if ((val) >= (lo) && (val) <= (hi)) {                         \
            test_passed++;                                            \
        } else {                                                      \
            test_failed++;                                            \
            test_fails++;                                             \
            printf("  FAIL: %s (%s:%d): %s -- "                       \
                   "value %ld not in [%ld, %ld]\n",                   \
                   __func__, __FILE__, __LINE__, msg,                 \
                   (long)(val), (long)(lo), (long)(hi));              \
        }                                                             \
    } while (0)

/* A simple test that does not return a value */
#define TEST_CHECK(cond, msg)  TEST_ASSERT(cond, msg)

/* ---- test registry ---- */

typedef void (*test_func_t)(void);

typedef struct {
    const char *name;
    test_func_t func;
} test_entry_t;

#define TEST_LIST(...) \
    static test_entry_t test_list[] = __VA_ARGS__; \
    static int test_list_len = sizeof(test_list) / sizeof(test_list[0]);

#define RUN_TESTS()                                                     \
    do {                                                                \
        int _i;                                                         \
        test_fails = 0;                                                 \
        printf("=== Running %d test(s) ===\n", test_list_len);          \
        for (_i = 0; _i < test_list_len; _i++) {                        \
            printf("  [%s]\n", test_list[_i].name);                    \
            test_list[_i].func();                                       \
        }                                                               \
        printf("=== Results: %d passed, %d failed (%d total) ===\n",   \
               test_passed, test_failed, test_count);                   \
        return test_fails ? 1 : 0;                                      \
    } while (0)

/* helper: print a hex dump of a Char* (unsigned int*) string */
static void
print_uchar_hex(const char *label, const unsigned int *s, int maxlen)
{
    int i;
    printf("  %s: ", label);
    for (i = 0; i < maxlen && s[i] != 0; i++) {
        printf("%04x ", s[i]);
    }
    printf("\n");
}

/* Compare a Char* (unsigned int*) string with a null-terminated C string. */
#define TEST_ASSERT_CSTR_EQ(actual, expected, msg)                       \
    do {                                                                  \
        test_count++;                                                     \
        {                                                                 \
            unsigned int *_a = (actual);                                  \
            const char *_e = (expected);                                  \
            int _i, _ok = 1;                                              \
            for (_i = 0; _e[_i]; _i++) {                                  \
                if ((unsigned char)_a[_i] != (unsigned char)_e[_i]) {  \
                    _ok = 0;                                              \
                    break;                                               \
                }                                                         \
            }                                                             \
            if (_ok && _a[_i] != 0) _ok = 0;                              \
            if (_ok) {                                                    \
                test_passed++;                                            \
            } else {                                                      \
                test_failed++;                                            \
                test_fails++;                                             \
                printf("  FAIL: %s (%s:%d): %s\n",                        \
                       __func__, __FILE__, __LINE__, msg);                 \
            }                                                             \
        }                                                                 \
    } while (0)

#endif /* TEST_H */
