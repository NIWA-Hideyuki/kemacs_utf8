#ifndef KFILE



#include "../econfig.h"



#ifndef FILE

#include <stdio.h>

#endif



#ifndef makedev

#include <sys/types.h>

#endif



#ifdef __STDC__

# include <stdlib.h>

#else

# ifdef MALLOC_CHARSTAR

char *malloc(void), *realloc(void);

# else

void *malloc(void), *realloc(void);

# endif

#endif



#if BSD

# include <strings.h>

#endif

#if USG

# include <string.h>

#endif



#if HANDLE_UTF

# include <iconv.h>

#endif



#define numberof(x) (sizeof(x)/sizeof(*(x)))

#define tailof(x) ((x)+numberof(x))



#define MAX_U8LEN 6

#define MAXPEND ((MAX_U8LEN)-1)	/* max size of pending stream data */



typedef struct {

  caddr_t	ks_id;		/* pointer to stream structure */

  int (*ks_openf)(caddr_t);	/* pointer to stream open function */

  int (*ks_closef)(caddr_t);	/* pointer to stream close function */

  int (*ks_getf)(caddr_t, char *, int);	/* pointer to stream get function */

  int (*ks_putf)(caddr_t, char *, int);	/* pointer to stream put function */

  union ks_un {

    unsigned u_flag; /* controls interpretation of stream */

    struct ks_str {

      unsigned s_thru:1;	/* through 8-bit */

      unsigned s_interp:2;	/* interpretation of stream */

      unsigned s_code:2;	/* KANJI code system */

      unsigned s_pass:1;	/* don't process SI/SO of JIS */

      unsigned s_ki:2;	/* kanji version */

      unsigned s_ri:2;	/* alphabet set */

      unsigned s_eolint:1;	/* EOL interpretation of stream (Add Nide) */

      unsigned s_eol:2;	/* type of EOL (Add Nide; only for fileIO) */

    } u_str;

  } ks_flag;

  int ks_gstate;		/* stream input status */

  int ks_pstate;		/* stream output status */

  int ks_queue[BUFSIZ];	/* buffer for queued characters */

  int ks_qp;		/* queue pointer */

  int ks_ql;		/* queue length */

  int ks_ul;		/* length of unbuffered data (Add Nide) */

  int ks_buf[MAXPEND];	/* buffer for pending stream data */

  int ks_bl;		/* used length of ks_buf[] */

} KSTREAM;



typedef union ks_un	KS_FLAG;

#define KS_VALUE(f)	(f).u_flag

#define KS_THRU(f)	(f).u_str.s_thru

#define KS_INTERP(f)	(f).u_str.s_interp

# define KS_KANJI  0

# define KS_BINARY 1

# define KS_UKANJI 2

#define KS_CODE(f)	(f).u_str.s_code

# define KS_JIS  0

# define KS_UJIS 1

# define KS_SJIS 2

# if HANDLE_UTF

#  define KS_UTF8 3

# endif

#define KS_PASS(f)	(f).u_str.s_pass

#define KS_KI(f)	(f).u_str.s_ki

# define KS_OLDJIS	0

# define KS_NEWJIS	1

#define KS_RI(f)	(f).u_str.s_ri

# define KS_ROMAJI	0

# define KS_ASCII	1

# define KS_BOGUS	2

#define KS_EOLINT(f)	(f).u_str.s_eolint

# define KS_EOLKNOWN	0

# define KS_EOLUK	1

#define KS_EOL(f)	(f).u_str.s_eol

# define KS_LF		0

# define KS_CR		1

# define KS_CRLF	2



#define KFILE		KSTREAM



#define KS_BINMODE	ks_binmode_flag()



#ifdef iscntrl

# undef iscntrl

#endif



#define iscntrl(c) ((c) <= ' ' || (c) == 0177)

#define isascii(c) (((c) & 0xff00) == 0)

#define iskana(c) (((c) & 0xff00) == 0x0100)

#define iskanji(c) (((c) & 0xff00) > 0x0100 && !isunicode(c))
/* isunicode: Unicode codepoints (>= 0x10000) stored directly;
   these are characters that have no EUC-JP equivalent (e.g. emojis) */
#define isunicode(c) ((unsigned int)(c) >= 0x10000)
/* BMP codepoints that have no EUC-JP equivalent (e.g. U+2764 HEAVY BLACK
   HEART, U+FE0F VARIATION SELECTOR-16) are also stored directly, but tagged
   with UNICODE_MARK.  This is needed because such a codepoint's raw value can
   collide with a JIS X 0208 kanji cell (the 0x2121-0x7E7E kanji range overlaps
   e.g. U+2764 == kanji cell 0x2764), which would make iskanji() misroute it.
   UNICODE_MARK (bit 0x200000) lies outside the Unicode code point space
   (max U+10FFFF uses bits 0-20), so it never overlaps a real codepoint *and*
   it lifts the tagged value into the ">= 0x10000" range isunicode() already
   recognizes -- so iskanji() (which excludes isunicode) drops it automatically.
   Callers that encode such a codepoint must strip the mark with UNICODE_CP(). */
#define UNICODE_MARK 0x200000U
#define UNICODE_CP(c) ((unsigned int)(c) & ~UNICODE_MARK)
/* iswidechar: characters that occupy 2 terminal columns */
#define iswidechar(c) (isunicode(c) || (iskanji(c) && !is_narrow_kanji(c)))

/*
 * Characters that are stored as "kanji" (high byte > 0x01) but are actually
 * narrow (1-column wide) on the terminal.  In JIS X 0208, rows 0x21-0x24
 * contain symbols, Greek, and Latin letters that are narrow.  Additionally,
 * the character 0x264c (which iconv maps to U+03BC, Greek mu) is narrow.
 * These must be treated as 1-column, not 2-column, in the virtual screen
 * and ttcol tracking.  Otherwise, cursor positioning via movecursor/TTmove
 * becomes off by one per narrow character, causing leftover characters on
 * mode line updates (e.g., "CUTF8" instead of "UTF8").
 */
#define is_narrow_kanji(c) \
    (iskanji(c) && (((c) & 0xff00) <= 0x2400 || (c) == 0x264c))

  

#define enkana(k, c) ((k) = (((c) & 0x7f) | 0x0100))

#define dekana(k, c) ((c) = ((k) & 0x7f))

  

#define enkanji(k, c1, c2) ((k) = ((((c1) & 0x7f) << 8) | ((c2) & 0x7f)))

#define dekanji(k, c1, c2) ((((c1) = ((k) >> 8) & 0x7f)), ((c2) = (k) & 0x7f))

  

#define ESCAPE		0x1b

#define SHFTIN		0x0f

#define SHFTOU		0x0e

#define SSHFT2		0x8e

#define SSHFT3		0x8f

#define CNTRLZ		0x1a



extern unsigned	ks_binmode_flag(void);
extern KSTREAM *	kalloc(caddr_t id, int (*openf)(caddr_t), int (*closef)(caddr_t), int (*getf)(caddr_t, char *, int), int (*putf)(caddr_t, char *, int), unsigned flag);
extern int		kfree(KSTREAM * ksp);
extern int		kputc(int c, KSTREAM * kp);
extern int		kgetc(KSTREAM * kp);
extern KFILE *		kopen(FILE * fp, unsigned flag, unsigned totime);
extern void		kclose(KFILE * kp);
extern int		jtos(int * j, int * s);
extern int		stoj(int * s, int * j);

/* Close all iconv handles for clean shutdown.
   Register via atexit() or call directly before exit(). */
extern void		kanji_term(void);


#endif /* !KFILE */