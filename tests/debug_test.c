#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal stubs */
int mlwrite() { return TRUE; }
int backchar() { return TRUE; }
int forwchar() { return TRUE; }
int ldelete() { return TRUE; }
int linsert() { return TRUE; }
int lnewline() { return TRUE; }
int rdonly() { return FALSE; }
int update() { return TRUE; }
int mlreplyt() { return TRUE; }
Char tgetc() { return 0; }
TERM term;

#define maindef
#define static
#include "../search.c"
#undef static
#undef maindef

int main() {
    printf("setup started\n");
    /* minimal: just test eq */
    printf("main entered\n");
    return 0;
}
