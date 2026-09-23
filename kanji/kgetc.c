#include "kanji.h"

/*
 * kgetc():	get character sequence from file stream and
 *          returns them in 16-bit format.
 */

#define KST_STATE	0xf000
#define KST_EOF		0x1000 /* input stream is at EOF state */
#define KST_NODATA	0x2000 /* nothing to read rignt now */
#define KST_ADDREAD	0x4000 /* just after restoring unbufferd bytes
				  at the tail of ks_queue (or broken bytes
				  in ks_buf) */

#define KANJI_STATE	0x0f00
#define JKANJI		0x0100
#define KANA		0x0200
#define G1KANA		0x0400 /* KANA by ^N/^O */

#define ESC_STATE	0x000f
#define NON_ESC		0x0000
#define ESC		0x0001
#define ESC_DOL		0x0002
#define ESC_PAR		0x0003
#define ESC_DOL_PAR	0x0004

#define CR_STATE	0x00f0
#define NON_EOL		0x0000
#define CR		0x0010
#define DOSEOF		0x0020

#define putq(c)		(kp->ks_queue[kp->ks_ql++] = (c))

static void
sweep_esc(KSTREAM * kp)
{


  switch (kp->ks_gstate & ESC_STATE) {
  case ESC:
    putq(ESCAPE);
    break;
  case ESC_PAR:
    putq(ESCAPE);
    putq('(');
    break;
  case ESC_DOL:
    putq(ESCAPE);
    putq('$');
    break;
  case ESC_DOL_PAR:
    putq(ESCAPE);
    putq('$');
    putq('(');
    break;
  }
  kp->ks_gstate &= ~ESC_STATE;
}

#if HANDLE_UTF
static int
utf8_restbytes(int c)
{

  return c < 0x80 ? 0 : c < 0xc2 ? -1: c < 0xe0 ? 1 : c < 0xf0 ? 2 :
	 c < 0xf8 ? 3 : c < 0xfc ? 4 : c < 0xfe ? 5 : -1;
}

static int
utf8_to_internal(char *p, int n1)
{
  static iconv_t cd = (iconv_t)(-1);
  char tmpb[3], *tmpi = tmpb;
  size_t tmpn = sizeof(tmpb), n = n1;
  int cc;

#if 0
  if (p == NULL) { /* try to close cd */
    if (cd != (iconv_t)(-1) && iconv_close(cd) == 0) cd = (iconv_t)(-1);
    return -1;
  }
#endif
  if (cd == (iconv_t)(-1)) {
    cd = iconv_open("EUC-JP", "UTF-8"); /* UTF-8 -> EUC-JP */
    if (cd == (iconv_t)(-1)) {
      puts("iconv_open() failed ... Why?\n"); exit(1);
    }
  }
  if (-1 == iconv(cd, &p, &n, &tmpi, &tmpn)) return -1;
  if (!(tmpb[0] & 0x80)) {
    cc = tmpb[0] & 0x7f;
  } else
  if (tmpb[0] == (char)SSHFT2) {
    enkana(cc, tmpb[1] & 0x7f);
  } else
  if (tmpb[0] == (char)SSHFT3) {
    enkanji(cc, tmpb[1]&0x7f, tmpb[2]&0x7f);
    cc |= 0x8000;
     /* Using highest bit to represent hojo-kanji set
        (but actually kemacs doesn't support hojo-kanji) */
  } else {
    enkanji(cc, tmpb[0]&0x7f, tmpb[1]&0x7f);
  }
  return cc;
}
#endif /* HANDLE_UTF */

static int
kdetermine(char *p, int n)
{
  int ujis_rest = 0, sjis_rest = 0;
#if HANDLE_UTF
  int utf8_rest = 0, possible = 3;
#else
  int possible = 2;
#endif
  register int c;

  for(; n && possible > 1; n--, p++){
    c = *p & 0xff;
#define impossible(r) ((r) = -1, possible--)
#if HANDLE_UTF
    switch(utf8_rest){
    case 0:
      if((utf8_rest = utf8_restbytes(c)) < 0) possible--;
       /* ... but actually kemacs support only JIS X0208 kanjis */
      break;
    case -1:
      break;
    default:
      if(0x80 <= c && c <= 0xbf) utf8_rest--;
      else impossible(utf8_rest);
      break;
    }
#endif /* HANDLE_UTF */
    switch(sjis_rest){
    case 0:
      if(!(c & 0x80) || 0xa0 <= c && c <= 0xdf) ;
      else if(0x81 <= c && c <= 0xfc) sjis_rest = 1;
      else impossible(sjis_rest);
      break;
    case -1:
      break;
    default:
      if(0x40 <= c && c <= 0xfc && c != 0x7f) sjis_rest = 0;
      else impossible(sjis_rest);
      break;
    }
    switch(ujis_rest){
    case 0:
      if(!(c & 0x80)) ;
      else if(0xa1 <= c && c <= 0xfe || c == SSHFT2) ujis_rest = 1;
      else if(c == SSHFT3) ujis_rest = 2;
       /* ... but actually kemacs doesn't support Hojo-kanji */
      else impossible(ujis_rest);
      break;
    case -1:
      break;
    default:
      if(0xa1 <= c && c <= 0xfe) ujis_rest--;
      else impossible(ujis_rest);
      break;
    }
  }
  if(possible != 1) return -1;
  return ujis_rest >= 0 ? KS_UJIS : sjis_rest >= 0 ? KS_SJIS :
#if HANDLE_UTF
    KS_UTF8;
#else
    -1;
#endif
}

int
kgetc(KSTREAM * kp)
{

  register int cc;
  int c1;
  register int n;
  char buf[BUFSIZ];
  register char *p;
  int kst;

  for (;;) {
    if (kp->ks_ql > 0) {
      cc = kp->ks_queue[kp->ks_qp++];
      if (kp->ks_qp >= kp->ks_ql) kp->ks_ql = 0;
      return cc;
    }
    kp->ks_qp = 0;

     /* Read from stream. If unbuffered bytes exist, restore them first */
    switch (kp->ks_gstate & KST_STATE) {
    case KST_EOF|KST_ADDREAD: case KST_NODATA|KST_ADDREAD:
     /* At previous time, we had no data to read, but since we have
        unbuffered data, we did not go into `if(!n)' process below.
        We have to do it now */
      kp->ks_gstate &= ~KST_ADDREAD;
      n = 0;
      break;
    case KST_EOF:
      return EOF;
    default: /* 0 or KST_ADDREAD or KST_NODATA */
     /* Unbuffered bytes are located at the tail of ks_queue. Restore it */
      {
	int rest = kp->ks_ul; /* Amount of unbuffered bytes (normally 0).
	 kp->ks_ul can be non-zero only when (kp->ks_gstate&KST_STATE) is 0 */
	kp->ks_ul = 0;
#if BSD
	bcopy((void *)tailof(kp->ks_queue) - rest, buf, rest);
#endif
#if USG
	memcpy(buf, (void *)tailof(kp->ks_queue) - rest, rest);
#endif
	n = (*kp->ks_getf)(kp->ks_id, buf + rest, sizeof(buf) - rest);
	 /* EOF (n < 0), or nothing to read right now (n == 0) */
	kp->ks_gstate &= ~KST_STATE; /* (kp->ks_gstate&KST_STATE) is cleared */
	if (n <= 0) kp->ks_gstate |= n < 0 ? n = 0, KST_EOF : KST_NODATA;
	if (rest) kp->ks_gstate |= KST_ADDREAD;
	n += rest;
      }
      break;
    }
    /* At this moment, possible values of (kp->ks_gstate&KST_STATE) are:
	KST_EOF, KST_NODATA (if n == 0)
	0, KST_ADDREAD, KST_EOF|KST_ADDREAD, KST_NODATA|KST_ADDREAD (if n > 0)
	  Once KST_EOF is set, it will never be reset */

    if (!n) {
     /* No data now. Flush buffered chars or reset escaping status, and
        continue. (kp->ks_gstate&KST_STATE) is now KST_EOF or KST_NODATA */
      if (kp -> ks_bl) { /* flush buffered broken chars */
	int *q = kp -> ks_buf, n1 = kp -> ks_bl;
	while (n1--) putq(*q++);
	kp -> ks_bl = 0;
	kp->ks_gstate |= KST_ADDREAD; 
	 /* Since we may have to reset CR or escaping status afterward,
	    we set KST_ADDREAD here, and go into `if(!n)' part again at
	    the next iteration of loop */
	continue;
      }
       /* Reset CR status (Added by Nide); never comes here when BINARY mode */
      switch (kp->ks_gstate & CR_STATE) {
      case CR:
	if (KS_EOLINT(kp->ks_flag) == KS_EOLUK) {
	  KS_EOLINT(kp->ks_flag) = KS_EOLKNOWN;
	  KS_EOL(kp->ks_flag) = KS_CR;
	  putq('\n');
	} else { /* then KS_EOL(kp->ks_flag) == KS_CRLF */
	  putq('\r');
	}
	break;
      case DOSEOF:
	if (KS_EOL(kp->ks_flag) != KS_CRLF) putq(CNTRLZ);
	 /* Do not output ^Z at EOF if KS_CRLF (ad hoc), including the case
	    KS_EOLINT(kp->ks_flag) == KS_EOLUK */
	break;
      }
      kp->ks_gstate &= ~CR_STATE;

       /* Reset escaping status */
      if (kp->ks_gstate & KST_STATE == KST_EOF /* Add Nide */ ||
	  KS_INTERP(kp->ks_flag) == KS_KANJI &&
	  KS_CODE(kp->ks_flag) == KS_JIS) {
	sweep_esc(kp);
      }
      continue;
    }
    for (p = buf; n-- > 0; ) {
      if (KS_INTERP(kp->ks_flag) == KS_BINARY) {
	putq(*p++ & (KS_THRU(kp->ks_flag)?0xff: 0x7f));
	continue;
      }
      cc = *p++;
      if (!KS_THRU(kp->ks_flag) ||
	  (KS_INTERP(kp->ks_flag) == KS_KANJI &&
	   KS_CODE(kp->ks_flag) == KS_JIS)) {
	/* input characters are stripped to 7 bits */
	cc &= 0177;
      } else
	cc &= 0xff;
      if (kp->ks_bl) {
      	int c2;
	/* combine broken multibyte character */
	switch (KS_CODE(kp->ks_flag)) {
	case KS_JIS:
	  c1 = kp->ks_buf[0], kp->ks_bl = 0;
	  if (cc > ' ' && cc < 0x7f) {
	    KS_INTERP(kp->ks_flag) = KS_KANJI;
	    enkanji(cc, c1, cc);
	    putq(cc);
	    continue;
	  }
	  putq(c1);
	  break;
	case KS_UJIS:
	  c1 = kp->ks_buf[0], kp->ks_bl = 0;
	  if (cc & 0x80 &&
	      (cc & 0x7f) > ' ' && (cc & 0x7f)< 0x7f) {
	    /* UJIS KANA or KANJI */
	    KS_INTERP(kp->ks_flag) = KS_KANJI;
	    c1 == SSHFT2 ? enkana(cc, cc) : enkanji(cc, c1&0x7f, cc&0x7f);
	    putq(cc);
	    continue;
	  }
	  putq(c1);
	  break;
	case KS_SJIS:
	  c1 = kp->ks_buf[0], kp->ks_bl = 0;
	  if (!iscntrl(cc) && ((c2 = (c1 << 8) | cc), !stoj(&c2, &c2))){
	    KS_INTERP(kp->ks_flag) = KS_KANJI;
	    if (c2 & 0x8000) putq(SSHFT3); /* treat as ujis hojo-kanji
	     (but kemacs actually does not support hojo-kanji) */
	    enkanji(cc, (c2>>8)&0x7f, c2&0x7f);
	    putq(cc);
	    continue;
	  }
	  putq(c1);
	  break;
#if HANDLE_UTF
	case KS_UTF8:
	  {
	    int l = utf8_restbytes(kp->ks_buf[0]) + 1, *q = kp->ks_buf, m;
	    char tmpb[MAX_U8LEN], *tmpi;

	    p--, n++;
	    if (l > kp->ks_bl + n) {
	     /* bytes in kp->ks_buf and buf[] are still insufficient to 
		compose a UTF8 letter. We put them toghther into kp->ks_buf */
	      q += kp->ks_bl, kp->ks_bl += n;
	      while (n--) *q++ = *p++ & 0xff;
	      continue;
	    }

	     /* first put the contents of kp->ks_buf into tmpb */
	    m = kp->ks_bl, tmpi = tmpb;
	    while (m--) *tmpi++ = *q++;
	     /* then put the rest bytes which are enough to compose
	        a UTF letter from buf[] into tmpb */
	    m = l - kp->ks_bl, kp->ks_bl = 0;
#if BSD
	    bcopy(p, tmpi, m);
#endif
#if USG
	    memcpy(tmpi, p, m);
#endif
	    p += m, n -= m;
	    if (-1 != (cc = utf8_to_internal(tmpb, l))) {
	      KS_INTERP(kp->ks_flag) = KS_KANJI;
	      putq(cc);
	      continue;
	    }
	     /* in this case, unlike in other kanji codes,
		output all illegal bytes */
	    tmpi = tmpb;
	    while (l--) putq(*tmpi++ & 0xff);
	    continue;
	  }
#endif /* HANDLE_UTF */
	}
      }

      switch (kp->ks_gstate & CR_STATE) {
       /* EOL treating (Added by Nide) */
      case CR:
	kp->ks_gstate &= ~CR_STATE;
	if (KS_EOLINT(kp->ks_flag) == KS_EOLUK) {
	  KS_EOLINT(kp->ks_flag) = KS_EOLKNOWN;
	  switch (cc) {
	  case '\n':
	    KS_EOL(kp->ks_flag) = KS_CRLF;
	    break;
	  case '\r':
	    cc = '\n'; /* FALLTHROUGH */
	  default: /* including ^Z */
	    KS_EOL(kp->ks_flag) = KS_CR;
	    putq('\n');
	    break;
	  }
	} else { /* then KS_EOL(kp->ks_flag) == KS_CRLF */
	  switch (cc) {
	  case '\n':
	    break;
	  case '\r':
	    putq('\r');
	    kp->ks_gstate |= CR;
	    continue;
	  case CNTRLZ:
	    putq('\r');
	    kp->ks_gstate |= DOSEOF;
	    continue;
	  default:
	    putq('\r');
	    break;
	  }
	}
	break; /* normal treating for cc is needed */
      case DOSEOF:
	putq(CNTRLZ);
	kp->ks_gstate &= ~CR_STATE;
	switch (cc) {
	case '\n':
	  if (KS_EOLINT(kp->ks_flag) == KS_EOLUK) {
	    KS_EOLINT(kp->ks_flag) = KS_EOLKNOWN;
	    KS_EOL(kp->ks_flag) = KS_LF;
	  }
	  break;
	case '\r':
	  kp->ks_gstate |= CR;
	  continue;
	case CNTRLZ:
	  kp->ks_gstate |= DOSEOF;
	  continue;
	default:
	  break;
	}
	break;
      case NON_EOL:
	if (KS_EOLINT(kp->ks_flag) == KS_EOLUK) {
	  switch (cc) {
	  case '\n':
	    KS_EOLINT(kp->ks_flag) = KS_EOLKNOWN;
	    KS_EOL(kp->ks_flag) = KS_LF;
	    break;
	  case '\r':
	    sweep_esc(kp);
	    kp->ks_gstate |= CR;
	    continue; /* no output at present */
	  case CNTRLZ:
	    sweep_esc(kp);
	    kp->ks_gstate |= DOSEOF;
	    continue;
	  default:
	    break;
	  }
	} else switch (KS_EOL(kp->ks_flag)) {
	case KS_LF:
	  break;
	case KS_CR:
	  if (cc == '\r') cc = '\n';
	  break;
	case KS_CRLF:
	  switch(cc){
	  case '\r':
	    sweep_esc(kp);
	    kp->ks_gstate |= CR;
	    continue;
	  case CNTRLZ:
	    sweep_esc(kp);
	    kp->ks_gstate |= DOSEOF;
	    continue;
	  }
	  break;
	}
	break;
      }

      switch (kst = (kp->ks_gstate & ESC_STATE)) {
      case NON_ESC:
	switch (cc) {
	case ESCAPE:
	  if (KS_CODE(kp->ks_flag) == KS_JIS ||
	      KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    KS_CODE(kp->ks_flag) = KS_JIS;
	    kp->ks_gstate += ESC;
	    continue;
	  }
	  /* no special meaning */
	  break;
	case SHFTOU:
	  if (KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    /* treat as JIS KANJI */
	    KS_INTERP(kp->ks_flag) = KS_KANJI;
	    KS_CODE(kp->ks_flag) = KS_JIS;
	    KS_PASS(kp->ks_flag) = 0;
	  }
	  if (KS_CODE(kp->ks_flag) == KS_JIS && !KS_PASS(kp->ks_flag)) {
	    kp->ks_gstate |= G1KANA;
	    continue;
	  }
	  /* no special meaning */
	  break;
	case SHFTIN:
	  if (KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    /* treat as JIS KANJI */
	    KS_INTERP(kp->ks_flag) = KS_KANJI;
	    KS_CODE(kp->ks_flag) = KS_JIS;
	    KS_PASS(kp->ks_flag) = 0;
	  }
	  if (KS_CODE(kp->ks_flag) == KS_JIS && !KS_PASS(kp->ks_flag)) {
	    kp->ks_gstate &= ~G1KANA;
	    continue;
	  }
	  /* no special meaning */
	  break;
#if 0 /* moved into the case cc&0x80 */
	case SSHFT2:
	  if (KS_CODE(kp->ks_flag) == KS_UJIS ||
	      KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    KS_CODE(kp->ks_flag) = KS_UJIS;
	    if (n < 1) {
	      /* broken */
	      kp->ks_buf[0] = cc;
	      kp->ks_bl = 1;
	      continue;
	    }
	    c1 = *p++;
	    if (c1 & 0x80 && !iscntrl(c1 & 0x7f)) {
	      KS_INTERP(kp->ks_flag) = KS_KANJI;
	      n--;
	      enkana(cc, c1 & 0x7f);
	      putq(cc);
	      continue;
	    }
	    p--;
	  }
	  /* no special meaning */
	  break;
#endif /* if 0 */
	}
	if (cc & 0x80) {
	  if (KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    /* To use as many bytes as possible in determining kanji code,
	       if the current byte (in cc) is not at the top of the buffer,
	       then first we unbuffer the rest bytes here (to the unused area
	       of ks_queue), and afterward, read from input stream to fill
	       the buffer */
	    n++, p--;

	    if (p == buf) {
	     /* cc is at the top of the buffer, so we can already use
	        the full length of buffer to determine kanji code.
	        We do not have to unbuffer and refill the buffer */
	      int kcode = kdetermine(p, n);
	      if (kcode >= 0) {
		KS_CODE(kp->ks_flag) = kcode;
		KS_INTERP(kp->ks_flag) = KS_KANJI;
	      }
	    } else {
	      if (n <= (sizeof(kp->ks_queue) - kp->ks_ql * sizeof(int)) &&
		  !(kp->ks_gstate & KST_STATE)) {
		 /* Enough area rest in kp->ks_queue, and we have not already
		    have buffered bytes */
#if BSD
		bcopy(p, (void *)tailof(kp->ks_queue) - n, n);
#endif
#if USG
		memcpy((void *)tailof(kp->ks_queue) - n, p, n);
#endif
		 /* unbuffering done, so escape from the loop */
		kp->ks_ul = n, n = 0; continue;
	      }
	    }

	    n--, p++;
	  }

	  if ((0xa1 <= cc && cc <= 0xfe || cc == SSHFT2) &&
	      (KS_CODE(kp->ks_flag) == KS_UJIS ||
	       KS_INTERP(kp->ks_flag) == KS_UKANJI)) {
	       /* May be 1st byte of UJIS kanji (including SSHFT2) */
	    KS_CODE(kp->ks_flag) = KS_UJIS;
	    if (n < 1) {
	       /* broken */
	      kp->ks_buf[0] = cc;
	      kp->ks_bl = 1;
	      continue;
	    }
	    c1 = *p++ & 0xff;
	    if (c1 & 0x80 && !iscntrl(c1 & 0x7f)) {
	      KS_INTERP(kp->ks_flag) = KS_KANJI;
	      n--;
	      cc == SSHFT2 ? 
	        enkana(cc, c1&0x7f) : enkanji(cc, cc&0x7f, c1&0x7f);
	      putq(cc);
	      continue;
	    }
	    p--;
	  }
	  if (KS_CODE(kp->ks_flag) == KS_SJIS ||
	      KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    KS_CODE(kp->ks_flag) = KS_SJIS;
	    if (0xa0 <= cc && cc <= 0xdf) { /* SJIS KANA */
	      KS_INTERP(kp->ks_flag) = KS_KANJI;
	      enkana(cc, cc&0x7f);
	      putq(cc);
	      continue;
	    } else { /* May be 1st byte of SJIS kanji */
	      if (n < 1) {
		 /* broken */
		kp->ks_buf[0] = cc;
		kp->ks_bl = 1;
		continue;
	      }
	      c1 = *p++ & 0xff;
	      if (!iscntrl(c1) && ((c1 |= (cc << 8)), !stoj(&c1, &c1))) {
		n--;
		KS_INTERP(kp->ks_flag) = KS_KANJI;
		if (c1 & 0x8000) putq(SSHFT3); /* treat as ujis hojo-kanji
		 (but kemacs actually does not support hojo-kanji) */
		enkanji(cc, (c1>>8)&0x7f, c1&0x7f);
		putq(cc);
		continue;
	      }
	      p--;
	    }
	  }
#if HANDLE_UTF
	  if (KS_CODE(kp->ks_flag) == KS_UTF8 ||
	      KS_INTERP(kp->ks_flag) == KS_UKANJI) {
	    int n1 = utf8_restbytes(cc) + 1;
	    if (n1 > 1) {
	      p--, n++;

	      KS_CODE(kp->ks_flag) = KS_UTF8;
	      if (n < n1) {
		 /* broken */
		int *q = kp->ks_buf;
		kp->ks_bl = n;
		while (n--) *q++ = *p++ & 0xff;
		continue;
	      }
	      if ((cc = utf8_to_internal(p, n1)) != -1) {
		 /* conversion succeed */
		p += n1, n -= n1;
		KS_INTERP(kp->ks_flag) = KS_KANJI;
		if (cc & 0x8000) cc &= ~0x8000, putq(SSHFT3);
		putq(cc);
		continue;
	      }
	       /* in this case, unlike in other kanji codes,
		  output all illegal bytes */
	      while (n1--) putq(*p++ & 0xff), n--;
	      continue;
	    }
	  }
#endif /* HANDLE_UTF */
	}
	if (iscntrl(cc)) {
	  putq(cc);
	  continue;
	}
	if (KS_CODE(kp->ks_flag) == KS_JIS && cc > ' ' && cc < 0x7f) {
	  if (kp->ks_gstate & (KANA | G1KANA)) {
	    /* Kana precedes Kanji */
	    enkana(cc, cc&0x7f);
	    putq(cc);
	    continue;
	  }
	  if (kp->ks_gstate & JKANJI) {
	    /* get second byte */
	    if (n < 1) {
	      /* kanji code is broken. */
	      kp->ks_buf[0] = cc;
	      kp->ks_bl = 1;
	      continue;
	    }
	    c1 = *p++ & 0x7f; /* !THRU assumed */
	    if (c1 > ' ' && c1 < 0x7f) {
	      n--;
	      enkanji(cc, cc, c1);
	      putq(cc);
	      continue;
	    }
	    p--;
	  }
	}
	/* Alphabet or uncategolized (META) characters */
	putq(cc);
	continue;
      case ESC:
	/* got ESC */
	kp->ks_gstate -= ESC;
	if (cc == '$') {
	  kp->ks_gstate += ESC_DOL;
	  continue;
	}
	if (cc == '(') {
	  kp->ks_gstate += ESC_PAR;
	  continue;
	}
	/* other ESC sequence. pass through */
	putq(ESCAPE);
	putq(cc);
	continue;
	
      case ESC_DOL: case ESC_DOL_PAR:
	/* got ESC $ or ESC $ ( */
	kp->ks_gstate &= ~ESC_STATE;
	if (kst == ESC_DOL && cc == '(') {
	  kp->ks_gstate += ESC_DOL_PAR;
	  continue;
	}
	if (cc == '@' || cc == 'B') {
	  KS_INTERP(kp->ks_flag) = KS_KANJI;
	  kp->ks_gstate &= ~(JKANJI|KANA), kp->ks_gstate |= JKANJI;
	  KS_KI(kp->ks_flag) = (cc == '@')? KS_OLDJIS: KS_NEWJIS;
	  continue;
	}
	/* other ESC sequence. pass through */
	putq(ESCAPE);
	if (kst == ESC_DOL_PAR) putq('(');
	putq('$');
	putq(cc);
	continue;
      case ESC_PAR:
	/* got ESC ( */
	kp->ks_gstate -= ESC_PAR;
	if (cc == 'A' || cc == 'B' || cc == 'J' || cc == 'H') {
	  KS_INTERP(kp->ks_flag) = KS_KANJI;
	  kp->ks_gstate &= ~(JKANJI|KANA);
	  KS_RI(kp->ks_flag) = (cc <= 'B')? KS_ASCII: KS_ROMAJI;
	  continue;
	}
	if (cc == 'I'){
	  KS_INTERP(kp->ks_flag) = KS_KANJI;
	  kp->ks_gstate &= ~(JKANJI|KANA), kp->ks_gstate |= KANA;
	  continue;
	}
	/* other ESC sequence. pass through */
	putq(ESCAPE);
	putq('(');
	putq(cc);
	continue;
      }
    }
  }
  /*NOTREACHED*/
}