#include <stdio.h>
#include "Cstrings.h"

Char *
Cstrncpy(Char * c1, Char * c2, int n)
{

    Char *c3 = c1;

    if (!c1 || !c2) return NULL;
    while (n-- > 0 && (*c1++ = *c2++)) ;
    if (c1 > c3 && c1[-1]) *c1 = '\0';
    return c3;
}
