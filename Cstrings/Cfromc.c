#include <stdio.h>
#include "kanji.h"
#include "Cstrings.h"

#include "../econfig.h"
#include <string.h>
#if BSD
# include <strings.h>
#endif

struct KS {
    char *p;
};

static int copen(struct KS *), cclose(struct KS *), cget(struct KS *, char *, int);

Char *
Cfromc(char * c)
{


    Char *C;
    int len;
    KSTREAM *ksp;
    struct KS ks;
    static int blen = 0;
    static Char *buf;

    if (!c) return NULL;
    if (blen <= (len = strlen(c))) {
	len = ((len+1) | 255) + 1;	/* multiple of 256 */
	if (blen) free((char *)buf);
	buf = (Char *)malloc((unsigned)len*sizeof(Char));
	if (!buf) {
	    blen = 0;
	    return NULL;
	}
	blen = len;
    }
    /* the length of C is no longer than that of c */
    /* Decode the byte string with the terminal code so that OS strings
       (argv, getenv, getcwd) carry the same internal Char encoding as
       text read from the keyboard or a file. */
    ks.p = c;
    if ((ksp = kalloc((caddr_t)&ks, (int (*)(caddr_t))copen, (int (*)(caddr_t))cclose, (int (*)(caddr_t, char *, int))cget, NULL, ks_termcode_flag())) == NULL)
	return NULL;
    for (C = buf; (*C = kgetc(ksp)) != EOF; C++) ;
    *C = '\0';
    (void)kfree(ksp);
    return buf;
}

/*ARGSUSED*/
static int
copen(struct KS * id)
{

    return 0;
}

/*ARGSUSED*/
static int
cclose(struct KS * id)
{

    return 0;
}

static int
cget(struct KS * id, char * buf, int len)
{

    int n;

    if (!*id->p) return -1;
    n = strlen(id->p);
    if (n > len) n = len;
    (void)strncpy(buf, id->p, n);
    id->p += n;
    return n;
}
