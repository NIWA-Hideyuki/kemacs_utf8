#include <stdio.h>
#include "kanji.h"
#define OBFSIZ	32	/* enough size for conversion from internal expression
			   to output stream */

/*
 * kputc:	put 16-bit KANJI codes to the file stream in 7-bit format.
 */

#define NORMAL	0
#define JKANJI	1
#define KANA	2

#if HANDLE_UTF
/* File-scope iconv handle for EUC-JP -> UTF-8, closed by kanji_term() at exit */
static iconv_t conv_wcd = (iconv_t)(-1);
static int conv_wcd_failed = 0;   /* iconv_open("UTF-8","EUC-JP") failed */

static int
codepoint_to_utf8(unsigned int cp, char *buf)
{
    if (cp < 0x80) {
        buf[0] = cp;
        return 1;
    } else if (cp < 0x800) {
        buf[0] = 0xC0 | (cp >> 6);
        buf[1] = 0x80 | (cp & 0x3F);
        return 2;
    } else if (cp < 0x10000) {
        buf[0] = 0xE0 | (cp >> 12);
        buf[1] = 0x80 | ((cp >> 6) & 0x3F);
        buf[2] = 0x80 | (cp & 0x3F);
        return 3;
    } else {
        buf[0] = 0xF0 | (cp >> 18);
        buf[1] = 0x80 | ((cp >> 12) & 0x3F);
        buf[2] = 0x80 | ((cp >> 6) & 0x3F);
        buf[3] = 0x80 | (cp & 0x3F);
        return 4;
    }
}
/* Manual EUC-JP -> UTF-8 fallback converter (no iconv dependency).
   Converts each byte: high-bit bytes become 2-byte UTF-8 sequences,
   ASCII bytes are passed through directly. */
static int
eucjp_to_utf8_fallback(char *q, int n1, char **out)
{
    while (n1--) {
        if (*q & 0x80) {
            *(*out)++ = (*q >> 6) & 0x3 | 0xc0;
            *(*out)++ = *q++ & 0x3f | 0x80;
        } else
            *(*out)++ = *q++;
    }
    return 1;
}

static int
ujis_to_utf8(char *q, int n1, char **pp)
{
	size_t l = MAX_U8LEN /* *pp must have MAX_U8LEN bytes space */, n = n1;

	if (!conv_wcd_failed && conv_wcd == (iconv_t)(-1)) {
		conv_wcd = iconv_open("UTF-8", "EUC-JP"); /* EUC-JP -> UTF-8 */
		if (conv_wcd == (iconv_t)(-1)) {
			conv_wcd_failed = 1;
			puts("iconv_open(EUC-JP -> UTF-8) failed; using fallback\n");
		}
	}

	if (!conv_wcd_failed && conv_wcd != (iconv_t)(-1)) {
		if (-1 != iconv(conv_wcd, &q, &n, pp, &l)) {
			return 1; /* *pp proceeded */
		}
		/* iconv failed (e.g. q doesn't correspond to existing ujis kanji) */
	}

	/* Manual fallback conversion (no iconv dependency) */
	return eucjp_to_utf8_fallback(q, n1, pp);
}

/* Close iconv handle in kputc. Called by kanji_term() at program exit. */
void
kputc_iconv_close(void)
{
#if HANDLE_UTF
    if (conv_wcd != (iconv_t)(-1)) {
        iconv_close(conv_wcd);
        conv_wcd = (iconv_t)(-1);
    }
    conv_wcd_failed = 0;
#endif
}
#endif
int
kputc(int c, KSTREAM * kp)
{

	int cc;
	char buf[OBFSIZ]; /* should be enough */
	char *p = buf;

	if (!kp->ks_putf) return EOF;
	if (c == EOF) {
	  /* flush pending output and return to normal state */
	  if (KS_INTERP(kp->ks_flag) != KS_BINARY &&
	      KS_CODE(kp->ks_flag) == KS_JIS) {
	    if (kp->ks_pstate & KANA) {
	      *p++ = SHFTIN;
	    }
	    if (kp->ks_pstate & JKANJI) {
	      *p++ = ESCAPE;
	      *p++ = '(';
	      *p++ = KS_RI(kp->ks_flag) == KS_ROMAJI? 'J':
		     KS_RI(kp->ks_flag) == KS_ASCII? 'B': 'H';
	    }
	    if (p > buf)
	      (void)(*kp->ks_putf)(kp->ks_id, buf, p-buf);
	  }
	  kp->ks_pstate = NORMAL;
	  return EOF;
	}
	if (KS_INTERP(kp->ks_flag) == KS_BINARY) {
	  buf[0] = c & (KS_THRU(kp->ks_flag)? 0xff: 0x7f);
	  (void)(*kp->ks_putf)(kp->ks_id, buf, 1);
	  return c;
	}
	if (isunicode(c)) {
	  /* Unicode codepoint stored directly - output as UTF-8 bytes.
	     Strip UNICODE_MARK (set for BMP codepoints w/o EUC-JP equiv). */
	  char utf8buf[MAX_U8LEN];
	  int ulen = codepoint_to_utf8(UNICODE_CP(c), utf8buf);
	  (void)(*kp->ks_putf)(kp->ks_id, utf8buf, ulen);
	  return c;
	}
	if (iskanji(c)) {
		switch (KS_CODE(kp->ks_flag)) {
		case KS_JIS:
			if (kp->ks_pstate & KANA) {
				*p++ = SHFTIN;
			}
			if (!(kp->ks_pstate & JKANJI)) {
			  *p++ = ESCAPE;
			  *p++ = '$';
			  *p++ = (KS_KI(kp->ks_flag) == KS_NEWJIS)? 'B': '@';
			}
			kp->ks_pstate = JKANJI;
			dekanji(c, *p, p[1]);
			p += 2;
			break;
		case KS_UJIS:
			dekanji(c, *p, p[1]);
			*p++ |= 0x80;
			*p++ |= 0x80;
			break;
		case KS_SJIS:
			dekanji(c, *p, p[1]);
			cc = (*p << 8) | p[1];
			jtos(&cc, &cc);
			*p++ = (cc >> 8) & 0xff;
			*p++ = cc & 0xff;
			break;
#if HANDLE_UTF
		case KS_UTF8:
			{
				char tmpi[2], *q = tmpi;
				dekanji(c, *q, q[1]);
				*q++ |= 0x80;
				*q |= 0x80;
				ujis_to_utf8(tmpi, 2, &p); /* p proceeds */
			}
			break;
#endif
		}
	} else if (iskana(c)) {
		switch (KS_CODE(kp->ks_flag)) {
		case KS_JIS:
			if (!(kp->ks_pstate & KANA)) {
				*p++ = SHFTOU;
				kp->ks_pstate |= KANA;
			}
			dekana(c, *p);
			p++;
			break;
		case KS_UJIS:
			*p++ = SSHFT2;
			dekana(c, *p);
			*p++ |= 0x80;
			break;
		case KS_SJIS:
			dekana(c, *p);
			*p++ |= 0x80;
			break;
#if HANDLE_UTF
		case KS_UTF8:
			{
				char tmpi[2], *q = tmpi;
				*q++ = SSHFT2;
				dekana(c, *q);
				*q |= 0x80;
				ujis_to_utf8(tmpi, 2, &p); /* p proceeds */
			}
			break;
#endif
		}
	} else {
		/* ascii (including control) characters */
		if (KS_CODE(kp->ks_flag) == KS_JIS) {
			if (kp->ks_pstate & KANA) {
				*p++ = SHFTIN;
			}
			if (kp->ks_pstate & JKANJI) {
			  *p++ = ESCAPE;
			  *p++ = '(';
			  *p++ = KS_RI(kp->ks_flag) == KS_ROMAJI? 'J':
			         KS_RI(kp->ks_flag) == KS_ASCII? 'B': 'H';
			}
			kp->ks_pstate = NORMAL;
		}
		switch(c){
		case '\n': /* EOL treatment (Added by Nide) */
			switch (KS_EOL(kp->ks_flag)) {
			case KS_LF:
				*p++ = c;
				break;
			case KS_CR:
				*p++ = '\r';
				break;
			case KS_CRLF:
				*p++ = '\r';
				*p++ = c;
				break;
			}
			break;
		default:
			*p++ = c;
			break;
		}
	}
	(void)(*kp->ks_putf)(kp->ks_id, buf, p-buf);
	return c;
}
