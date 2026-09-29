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

/* Return display width of a kemacs internal Char:
   0 = zero-width (combining marks), 1 = narrow, 2 = wide. */
extern int char_width(unsigned int c);

#endif /* WCWIDTH_H */
