#include <stdio.h>
#include "Cstrings.h"

Char *
Cstrcat(Char * c1, Char * c2)
{

    Char *c3 = c1;

    if (!c1 || !c2) return NULL;
    c1 += Cstrlen(c1);
    (void)Cstrcpy(c1, c2);
    return c3;
}