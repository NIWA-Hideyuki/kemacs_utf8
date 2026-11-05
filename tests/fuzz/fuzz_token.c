#define maindef
#include "../../ecomm.h"
#include "../../estruct.h"
#include "../../edef.h"
#undef maindef

#include <stdlib.h>
#include <string.h>

extern Char *token(Char *src, Char *tok, int size);

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    /* Split input: first half src, size param from last byte */
    size_t half = size / 2;
    Char src[256];
    Char tok[256];
    size_t i;
    for (i = 0; i < half && i + 1 < sizeof(src); i++)
        src[i] = (unsigned char)data[i];
    src[i] = 0;
    int sz = (size > 0) ? (data[size-1] % 256) : 64;
    if (sz == 0) sz = 1;
    token(src, tok, sz);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned char buf[4096];
    size_t n;
    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        if (!f) { perror(argv[1]); return 1; }
        n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
    } else {
        n = fread(buf, 1, sizeof(buf), stdin);
    }
    LLVMFuzzerTestOneInput(buf, n);
    return 0;
}
