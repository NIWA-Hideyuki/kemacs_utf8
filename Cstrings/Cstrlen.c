#include "Cstrings.h"

int
Cstrlen(Char * c)
{

    register int n;

    for (n = 0; *c++; n++) ;
    return n;
}