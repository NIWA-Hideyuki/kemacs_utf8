#include <stdio.h>
#include "kanji.h"
#include "Cstrings.h"
#include "kpp.h"

int
main(void)
{
	FILE *dfp, *cfp;
	int i, c1, c2;
	Char ch;

	if ((dfp = fopen(DFNAME, "r")) == NULL) {
		fprintf(stderr, "can't open %s\n", DFNAME);
		exit(1);
	}
	if ((cfp = fopen(CFNAME, "w")) == NULL) {
		fprintf(stderr, "can't open %s\n", CFNAME);
		exit(1);
	}
	fprintf(cfp, "%s %s[] = {", ARTYPE, ARNAME);
	for (i=0; (c1 = getc(dfp)) != EOF; i++) {
		if ((c2 = getc(dfp)) == EOF) {
			fprintf(stderr, "%s: bad format.\n", DFNAME);
			break;
		}
		enkanji(ch, c1, c2);
		fprintf(cfp, "%s0x%x,", (i%8)? " ": "\n", ch);
	}
	fprintf(cfp, "\n};\n");
	(void)fclose(dfp);
	(void)fclose(cfp);
	exit(0);
}