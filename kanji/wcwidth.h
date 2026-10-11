#ifndef WCWIDTH_H
#define WCWIDTH_H

/*
 * wcwidth.h -- Declarations for Unicode East Asian Width functions.
 *
 * Implemented in wcwidth.c (part of the kanji library).
 */

/* Return display width of a Unicode codepoint:
   -1 = non-printable (control), 0 = combining/zero-width,
    1 = narrow, 2 = wide/fullwidth. */
extern int ucswidth(unsigned int cp);

/* Return display width of a kemacs internal Char:\
   0 = zero-width (combining marks), 1 = narrow, 2 = wide. */
extern int char_width(unsigned int c);

/* Runtime flag: 1 if the terminal locale renders hiragana/katakana as
   2-column wide (CJK locales: ja, ko, zh).  0 for Western-locale
   terminals where kana render as 1-column narrow.  Initialized by
   init_kana_width() at editor startup. */
extern int kana_is_wide;

/* Initialize kana_is_wide by checking locale environment variables
   (LC_ALL, LC_CTYPE, LANG).  Should be called once during editor
   startup, after the terminal is opened but before any redisplay. */
extern void init_kana_width(void);

#endif /* WCWIDTH_H */
