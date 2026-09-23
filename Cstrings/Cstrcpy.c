#include <stdio.h>
#include "Cstrings.h"

Char *
Cstrcpy(Char * c1, Char * c2)
{

    Char *c3 = c1;

    if (!c1 || !c2) return NULL;
    while (*c1++ = *c2++) ;
    return c3;
}