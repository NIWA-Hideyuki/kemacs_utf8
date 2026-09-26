#include <stdio.h>
#include "Cstrings.h"

Char *
Crindex(Char * c, int cc)
{

    Char *c1 = c;

    if (!c || !cc) return NULL;
    c += Cstrlen(c);
    while (--c >= c1 && *c != cc) ;
    return c >= c1? c: NULL;
}
