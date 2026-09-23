#include "Cstrings.h"

int
Cstrcmp(Char * c1, Char * c2)
{

    if (c1 == c2) return 0;
    if (!c1) return -1;
    if (!c2) return 1;
    while (*c1 && *c1 == *c2) c1++, c2++;
    return (int)(*c1-*c2);
}