#include <stdio.h>

#include "kanji.h"
#include "Cstrings.h"
#include "kpp.h"

#if	BSD
# include <sys/file.h>
#endif
#if	USG
# define L_SET	0
# define L_INCR	1
# define L_XTND	2
# include <memory.h>
# define bcmp	memcmp
#endif

#define MARKER_HEAD	'C'
#define STR_MARKER	"STR("
#define CHR_MARKER	"CHR("
#define MARKER_LEN	4

void flushdata(void);
static char *prog;
static char *file;
static int line = 0;
static int fcode;

int
main(int argc, char **argv)
{


	FILE *f;
void usage(char * msg);
int kpp(FILE * fi);

	prog = argv[0];
	fcode = DEF_F_CODE;
	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		switch ((*argv)[1]) {
		case 'j':
		case 'J':
			fcode = KS_JIS;
			break;
		case 'u':
		case 'U':
			fcode = KS_UJIS;
			break;
		case 's':
		case 'S':
			fcode = KS_SJIS;
			break;
#if HANDLE_UTF
		case 'w':
		case 'W':
			fcode = KS_UTF8;
			break;
#endif
		default:
			usage("Illegal option");
			exit(1);
		}
	}
	if (argc > 1) {
		usage("Too many arguments");
		exit(1);
	}
	if (!argc) {
		f = stdin;
		file = "stdin";
	} else {
		if (!(f = fopen(argv[0], "r"))) {
			fprintf(stderr, "Can't open %s\n", argv[0]);
			exit(1);
		}
		file = argv[0];
	}
	return kpp(f);
}

void
usage(char * msg)
{

	fprintf(stderr, "%s\n", msg);
	fprintf(stderr, "usage: %s [file]\n", prog);
}

static Char buf[BUFSIZ];
static FILE *fo;
static KFILE *kp;

int
kpp(FILE * fi)
{

int kreadln(KFILE * kp, Char * buf, int len);
	KS_FLAG flag;

	KS_VALUE(flag) = 0;
	KS_THRU(flag) = 1;		/* thru 8bit */
	KS_INTERP(flag) = KS_KANJI;	/* KANJI code */
	KS_CODE(flag) = fcode;		/* JIS/UJIS/SJIS/UTF8 */
	KS_PASS(flag) = 0;		/* process ^O/^N */
	KS_EOLINT(flag) = KS_EOLKNOWN;	/* EOL type is fixed on KS_LF */
	KS_EOL(flag) = KS_LF;		/* fixed on KS_LF */
	if (!(kp = kopen(fi, KS_VALUE(flag), 0))) {
		fprintf(stderr, "Can't open KANJI stream on the file.\n");
		return 1;
	}
	if (!(fo = fopen(OFNAME, "w"))) {
		fprintf(stderr, "Can't open %s for output.\n", OFNAME);
		return 1;
	}
	fprintf(fo, "#ifndef %s\n", ARFLAG);
	fprintf(fo, "extern %s %s[];\n", ARTYPE, ARNAME);
	 /* added 'extern' by N.Nide */
	fprintf(fo, "#define %s\n", ARFLAG);
	fprintf(fo, "#endif /*%s*/\n", ARFLAG);
	while (kreadln(kp, buf, BUFSIZ) > 0) {
		register Char *p, *q;
void flushch(Char ch, FILE * fp);

		p = buf;
		if (*p == '#') {
			/* skip preprocessor line */
			while (*p) flushch(*p++, fo);
			continue;
		}
		while ((q = Cindex(p, MARKER_HEAD)) != NULL) {
			Char *dostr(Char *), *dochr(Char *);

			while (p <= q) flushch(*p++, fo);
			/* now p points (possibly) to the 2nd char of marker */
			if (Cstrncmp(p, Cfromc(STR_MARKER), MARKER_LEN) == 0) {
				/* string marker found */
				fprintf(fo, STR_MARKER);
				p = dostr(p+MARKER_LEN);
				fputc(')', fo);
				continue;
			}
			if (Cstrncmp(p, Cfromc(CHR_MARKER), MARKER_LEN) == 0) {
				/* character marker found */
				fprintf(fo, CHR_MARKER);
				p = dochr(p+MARKER_LEN);
				fputc(')', fo);
				continue;
			}
		}
		while (*p) flushch(*p++, fo);
	}
	flushdata();
	(void)fclose(fo);
	(void)kclose(kp);
	return 0;
}

int
kreadln(KFILE * kp, Char * buf, int len)
{

	register Char ch, *p = buf;
	static int newln = 1;

	if (newln) newln = 0, line++;
	while (--len > 0 && (ch = kgetc(kp)) != EOF) {
		*p++ = ch;
		if (ch == '\n') {
			newln++;
			break;
		}
	}
	*p = '\0';
	return p-buf;
}

void
flushch(Char ch, FILE * fp)
{

	static Char bb[2] = {0, 0};

	bb[0] = ch;
	fprintf(fp, "%s", cfromC(bb));
}

Char *
dostr(Char * p)
{

	int idx;
	Char *readstr(Char *);
int addstr(void);

	p = readstr(p);
	if ((idx = addstr()) >= 0)
		fprintf(fo, "&%s[%d]", ARNAME, idx);
	return p;
}

static Char Cbuf[BUFSIZ];
static int Cblen;	/* null string has count 1. */

Char *
readstr(Char * p)
{

	register Char c, *Cp;
Char readch(Char ** p);
	void err(char *), errchr(Char, char *);

	Cblen = 0;
	Cp = Cbuf;
	while ((c = *p++) == ' ' || c == '\t') ;
	if (c != '"') {
		errchr(c, "CSTR");
		return p-1;
	}
	while (*p && *p != '"') {
		c = readch(&p);
		if (c == EOF) {
			/* continue to next line */
			if (kreadln(kp, buf, BUFSIZ) < 1) {
				err("unexpected end of file");
				return p;
			}
			p = buf;
			continue;
		}
		*Cp++ = c;
		Cblen++;
	}
	*Cp++ = '\0';
	Cblen++;
	if (!*p) {
		err("unexpected end of line");
		return p;
	}
	p++;
	while ((c = *p++) == ' ' || c == '\t') ;
	if (c != ')') {
		errchr(c, "CSTR");
		return p-1;
	}
	return p;
}

#define TAB_INCR	100

static off_t *stab;
static int tabsiz = 0, maxtab = 0;

static FILE *dfp = NULL;

int
addstr(void)
{

	static char fbuf[BUFSIZ*2], dbuf[BUFSIZ*2];
	register int i;
	register Char *p;

	if (!dfp) {
		register int c1, c2;
		register Char ch;

		if ((dfp = fopen(DFNAME, "a+")) == NULL) {
			fprintf(stderr, "can't create %s.\n", DFNAME);
			exit(1);
		}
		if (fseek(dfp, 0, L_SET) < 0) {
			fprintf(stderr, "seek error!\n");
			exit(1);
		}
		if ((stab = (off_t *)malloc(sizeof(off_t)*TAB_INCR)) == NULL) {
			fprintf(stderr, "no enough memory.\n");
			exit(1);
		}
		maxtab = TAB_INCR;
		stab[0] = 0;
		tabsiz = 1;
		while ((c1 = getc(dfp)) != EOF) {
			if ((c2 = getc(dfp)) == EOF) {
				fprintf(stderr, "%s: bad format\n", DFNAME);
				break;
			}
			enkanji(ch, c1, c2);
			if (!ch) {
				if (tabsiz >= maxtab) {
					if ((stab = (off_t *)realloc((char *)stab, sizeof(off_t)*(maxtab+TAB_INCR))) == NULL) {
						fprintf(stderr, "no enough memory.\n");
						exit(1);
					}
					maxtab += TAB_INCR;
				}
				stab[tabsiz++] = ftell(dfp);
			}
		}
	}
	/* expand string to file image */
	i = 0;
	p = Cbuf;
	do {
		dekanji(*p, dbuf[i], dbuf[i+1]);
		i += 2;
	} while (*p++);
	for (i=1; i<tabsiz; i++) {
		if (Cblen <= (stab[i]-stab[i-1])/2) {
			register off_t off = stab[i]-Cblen*2;
			int n;

			if (fseek(dfp, off, L_SET) < 0) {
				fprintf(stderr, "seek error!\n");
				exit(1);
			}
			if ((n = fread(fbuf, 2, Cblen, dfp)) < Cblen) {
				fprintf(stderr, "read error! expected %d, read %d\n", Cblen, n);
				exit(1);
			}
			if (bcmp(dbuf, fbuf, Cblen*2) == 0) {
				/* same string found */
				return off/2;	/* index is half of offset */
			}
		}
	}
	/* blanc new string. add to file */
	if (fseek(dfp, 0, L_XTND) < 0) {
		fprintf(stderr, "seek error!\n");
		exit(1);
	}
	if (fwrite(dbuf, 2, Cblen, dfp) < Cblen) {
		fprintf(stderr, "write error!\n");
		exit(1);
	}
	if (tabsiz >= maxtab) {
		if ((stab = (off_t *)realloc((char *)stab, sizeof(off_t)*(maxtab+TAB_INCR))) == NULL) {
			fprintf(stderr, "no enough memory.\n");
			exit(1);
		}
		maxtab += TAB_INCR;
	}
	return (stab[tabsiz++] = ftell(dfp))/2-Cblen;
}

void
flushdata(void)
{

	if (dfp != NULL) (void)fclose(dfp);
}

Char *
dochr(Char * p)
{

	register Char c;
Char readch(Char ** p);
	void err(char *), errchr(Char, char *);

	while ((c = *p++) == ' ' || c == '\t') ;
	if (c != '\'') {
		errchr(c, "CCHR");
		return p-1;
	}
	if ((c = readch(&p)) == EOF) {
		err("unexpected end of line\n");
		return p;
	}
	fprintf(fo, "0x%x", c);
	if ((c = *p++) != '\'') {
		errchr(c, "CCHR");
		return p-1;
	}
	while ((c = *p++) == ' ' || c == '\t') ;
	if (c != ')') {
		errchr(c, "CCHR");
		return p-1;
	}
	return p;
}

Char
readch(Char ** p)
{

	register Char c, ch;

	if ((c = *(*p)++) == '\\') {
		c = *(*p)++;
		if (c >= '0' && c < '8') { /* isdigit is not safe for 'Char' */
			ch = c - '0';
			c = *(*p)++;
			if (c >= '0' && c < '8') {
				ch = (ch<<3) + c - '0';
				c = *(*p)++;
				if (c >= '0' && c < '8') {
					ch = (ch<<3) + c - '0';
				} else (*p)--;
			} else (*p)--;
		} else switch (c) {
		case 'n': ch = '\n'; break;
		case 'r': ch = '\r'; break;
		case 'f': ch = '\f'; break;
		case 't': ch = '\t'; break;
		case 'b': ch = '\b'; break;
		case '\n': ch = EOF; break;
		default: ch = c; break;
		}
	} else ch = c;
	return ch;
}

void
err(char * s)
{

	fprintf(stderr, "\"%s\", line %d: %s\n", file, line, s);
}

void
errchr(Char c, char * s)
{

	static Char bb[2];
	static char mbuf[BUFSIZ];

	bb[0] = c;
	(void)sprintf(mbuf, "unexpected char '%s' in %s.", cfromC(bb), s);
	err(mbuf);
}
