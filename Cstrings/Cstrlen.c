#include "Cstrings.h"

int
Cstrlen(Char * c)
{

    int n;

    for (n = 0; *c++; n++) ;
    return n;
}