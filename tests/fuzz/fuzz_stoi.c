/* fuzz_stoi.c -- Fuzz harness for stoi() (eval.c).
 *
 * Build (standalone, ASan/UBSan):
 *   cc -std=gnu89 -I../.. -I../../kanji -I../../Cstrings -DKEMACS_TEST \
 *      -fsanitize=address,undefined -g fuzz_stoi.c \
 *      ../../tests/eval_gc.o ../../Css.o -Wl,--gc-sections -o fuzz_stoi
 *
 * Run:
 *   ./fuzz_stoi < input.txt
 *   ./fuzz_stoi corpus/   (directory of seed files)
 *
 * libFuzzer entry point is provided for future use with clang -fsanitize=fuzzer.
 */

#define maindef
#include "../../ecomm.h"
#include "../../estruct.h"
#include "../../edef.h"
#undef maindef

#include <stdlib.h>
#include <string.h>

extern int stoi(Char *);

static void to_char(Char *dst, const unsigned char *src, size_t n, size_t cap)
{
    size_t i;
    for (i = 0; i < n && i + 1 < cap; i++)
        dst[i] = (unsigned char)src[i];
    dst[i] = 0;
}

/* libFuzzer entry point */
int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    Char buf[128];
    to_char(buf, data, size, sizeof(buf));
    (void)stoi(buf);
    return 0;
}

#ifndef __AFL_FUZZER  /* allow standalone build */
int main(int argc, char **argv)
{
    /* If a directory is given, iterate files; else read stdin. */
    if (argc > 1) {
        /* Simple: treat argv[1] as a single file */
        FILE *f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return 1; }
        unsigned char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        LLVMFuzzerTestOneInput(buf, n);
        return 0;
    }
    unsigned char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf), stdin);
    LLVMFuzzerTestOneInput(buf, n);
    return 0;
}
#endif
