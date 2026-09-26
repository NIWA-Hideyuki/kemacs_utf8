#include <stdio.h>
#include "Cstrings.h"

Char *
Cindex(Char * c, int cc)
{

    if (!c || !cc) return NULL;
    while (*c && *c != cc) c++;
    return *c? c: NULL;
}
