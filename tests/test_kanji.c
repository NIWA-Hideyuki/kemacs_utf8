/*
 * test_kanji.c -- Unit tests for kemacs kanji/UTF-8 conversion
 * and character-width macros.
 *
 * Tests: stoj, jtos, utf8_to_internal, utf8_to_codepoint,
 *        codepoint_to_utf8, ujis_to_utf8, iswidechar, is_narrow_kanji
 */

#include "test.h"
#include <stdlib.h>

/* kanji.h provides Char, iswidechar, iscombchar, char_width, etc. */
#include "../kanji/kanji.h"

/* Include the char_width/ucswidth implementation directly so the
   test can link without the kanji library. */
#include "../kanji/wcwidth.c"

/* ---- Source files under test (included directly) ---- */
/* stoj.c, jtos.c -- standalone, no external deps */
#include "../kanji/stoj.c"
#include "../kanji/jtos.c"

/* Expose static functions from kgetc.c via #define static */
#define static
#include "../kanji/kgetc.c"
#undef static

/* kgetc.c defines JKANJI, KANA as kana-state macros.
 * kputc.c redefines them for output state.  Undef to avoid conflict. */
#undef JKANJI
#undef KANA
#undef NORMAL

/* Expose static functions from kputc.c */
#define static
#include "../kanji/kputc.c"
#undef static

/* ================================================================ */
/*  stoj / jtos                                                     */
/* ================================================================ */

static void check_pair(int jis, int sjis, const char *desc)
{
    int s[1], j[1], j2[1], rc;

    j[0] = jis;
    rc = jtos(j, s);
    TEST_ASSERT_INT(rc, 0, desc);
    TEST_ASSERT_INT(s[0], sjis, desc);

    s[0] = sjis;
    rc = stoj(s, j2);
    TEST_ASSERT_INT(rc, 0, desc);
    TEST_ASSERT_INT(j2[0], jis, desc);
}

static void test_stoj_jtos_known(void)
{
    /* Verified pairs computed from jtos/stoj formulas */
    check_pair(0x2121, 0x8140, "0x2121<->0x8140");  /* fullwidth ＜ */
    check_pair(0x213D, 0x815C, "0x213D<->0x815C");  /* wave dash 〜 */
    check_pair(0x2140, 0x815F, "0x2140<->0x815F");  /* fullwidth ＠ */
    check_pair(0x2171, 0x8191, "0x2171<->0x8191");  /* fullwidth １ */
    check_pair(0x2422, 0x82A0, "0x2422<->0x82A0");  /* hiragana あ */
    check_pair(0x243F, 0x82BD, "0x243F<->0x82BD");  /* hiragana た */
    check_pair(0x2522, 0x8341, "0x2522<->0x8341");  /* katakana ア */
    check_pair(0x3021, 0x889F, "0x3021<->0x889F");  /* katakana マ */
    check_pair(0x4E42, 0x97C0, "0x4E42<->0x97C0");  /* kanji 込 */
}

static void test_stoj_jtos_roundtrip(void)
{
    int jis, j[1], s[1], j2[1], rc;
    int tested = 0;

    for (jis = 0x2121; jis <= 0x7E7E; jis++) {
        int jh = (jis >> 8) & 0xFF;
        int jl = jis & 0xFF;
        if (jh < 0x21 || jh > 0x7E || jl < 0x21 || jl > 0x7E)
            continue;

        j[0] = jis;
        rc = jtos(j, s);
        if (rc != 0) continue;
        rc = stoj(s, j2);
        if (rc != 0) continue;
        TEST_ASSERT_INT(j2[0], jis, "round-trip JIS match");
        tested++;
    }
    TEST_ASSERT(tested > 100, "should have tested at least 100 codes");
}

static void test_stoj_invalid(void)
{
    int s[1], j[1];

    s[0] = 0x0041;  /* ASCII as lead byte */
    TEST_ASSERT_INT(stoj(s, j), 1, "ASCII SJIS should fail");

    s[0] = 0xA140;  /* 0xA0-0xDF gap */
    TEST_ASSERT_INT(stoj(s, j), 1, "0xA1 lead should fail");

    s[0] = 0x817F;  /* trail 0x7F invalid */
    TEST_ASSERT_INT(stoj(s, j), 1, "trail 0x7F should fail");

    s[0] = 0x9F40;  /* valid */
    TEST_ASSERT_INT(stoj(s, j), 0, "0x9F40 should succeed");
}

static void test_jtos_invalid(void)
{
    int j[1], s[1];

    j[0] = 0x0000;
    TEST_ASSERT_INT(jtos(j, s), 1, "0x0000 should fail");

    j[0] = 0x8021;
    TEST_ASSERT_INT(jtos(j, s), 1, "0x8021 should fail");

    j[0] = 0x2100;
    TEST_ASSERT_INT(jtos(j, s), 1, "0x2100 should fail");
}

/* ================================================================ */
/*  utf8_to_codepoint (manual UTF-8 decoder)                       */
/* ================================================================ */

static void test_utf8_to_codepoint(void)
{
    char buf[8];
    int cp;

    /* ASCII */
    buf[0] = 'A';
    cp = utf8_to_codepoint(buf, 1);
    TEST_ASSERT_EQ(cp, 0x41, "ASCII A");

    /* 2-byte: U+00E9 (e-acute) */
    buf[0] = 0xC3; buf[1] = 0xA9;
    cp = utf8_to_codepoint(buf, 2);
    TEST_ASSERT_EQ(cp, 0xE9, "U+00E9");

    /* 3-byte: U+3042 (hiragana a) */
    buf[0] = 0xE3; buf[1] = 0x81; buf[2] = 0x82;
    cp = utf8_to_codepoint(buf, 3);
    TEST_ASSERT_EQ(cp, 0x3042, "U+3042");

    /* 4-byte: U+1F600 (grinning face) */
    buf[0] = 0xF0; buf[1] = 0x9F; buf[2] = 0x98; buf[3] = 0x80;
    cp = utf8_to_codepoint(buf, 4);
    TEST_ASSERT_EQ(cp, 0x1F600, "U+1F600");

    /* Invalid: overlong 0xC0 0x80 */
    buf[0] = 0xC0; buf[1] = 0x80;
    cp = utf8_to_codepoint(buf, 2);
    TEST_ASSERT_EQ(cp, -1, "overlong should be -1");

    /* Invalid: empty */
    cp = utf8_to_codepoint(buf, 0);
    TEST_ASSERT_EQ(cp, -1, "empty should be -1");

    /* Invalid: truncated 3-byte */
    buf[0] = 0xE3; buf[1] = 0x81;
    cp = utf8_to_codepoint(buf, 2);
    TEST_ASSERT_EQ(cp, -1, "truncated 3-byte should be -1");

    /* Invalid: truncated 4-byte */
    buf[0] = 0xF0; buf[1] = 0x9F;
    cp = utf8_to_codepoint(buf, 2);
    TEST_ASSERT_EQ(cp, -1, "truncated 4-byte should be -1");
}

/* ================================================================ */
/*  utf8_to_internal                                              */
/* ================================================================ */

/*
 * utf8_to_internal first tries iconv, then falls back to
 * utf8_to_codepoint.  ASCII and invalid input produce
 * deterministic results from either path.
 */
static void test_utf8_to_internal_ascii(void)
{
    char buf[4];
    int cp;

    buf[0] = 'A';
    cp = utf8_to_internal(buf, 1);
    TEST_ASSERT_EQ(cp, 0x41, "utf8_to_internal 'A'");

    buf[0] = 0x7E;
    cp = utf8_to_internal(buf, 1);
    TEST_ASSERT_EQ(cp, 0x7E, "utf8_to_internal 0x7E");
}

static void test_utf8_to_internal_invalid(void)
{
    char buf[4];
    int cp;

    buf[0] = 0x80;
    cp = utf8_to_internal(buf, 1);
    TEST_ASSERT_EQ(cp, -1, "0x80 should be -1");

    buf[0] = 0xC3;
    cp = utf8_to_internal(buf, 1);
    TEST_ASSERT_EQ(cp, -1, "truncated 2-byte should be -1");

    buf[0] = 0xFF;
    cp = utf8_to_internal(buf, 1);
    TEST_ASSERT_EQ(cp, -1, "0xFF should be -1");
}

/* ================================================================ */
/*  codepoint_to_utf8                                             */
/* ================================================================ */

static void test_codepoint_to_utf8(void)
{
    char buf[8];
    int len;

    len = codepoint_to_utf8(0x41, buf);
    TEST_ASSERT_INT(len, 1, "0x41 length");
    TEST_ASSERT_EQ((unsigned char)buf[0], 0x41, "0x41 byte");

    len = codepoint_to_utf8(0xE9, buf);
    TEST_ASSERT_INT(len, 2, "0xE9 length");
    TEST_ASSERT_EQ((unsigned char)buf[0], 0xC3, "0xE9 byte 0");
    TEST_ASSERT_EQ((unsigned char)buf[1], 0xA9, "0xE9 byte 1");

    len = codepoint_to_utf8(0x3042, buf);
    TEST_ASSERT_INT(len, 3, "0x3042 length");
    TEST_ASSERT_EQ((unsigned char)buf[0], 0xE3, "0x3042 byte 0");
    TEST_ASSERT_EQ((unsigned char)buf[1], 0x81, "0x3042 byte 1");
    TEST_ASSERT_EQ((unsigned char)buf[2], 0x82, "0x3042 byte 2");

    len = codepoint_to_utf8(0x1F600, buf);
    TEST_ASSERT_INT(len, 4, "0x1F600 length");
    TEST_ASSERT_EQ((unsigned char)buf[0], 0xF0, "emoji byte 0");
    TEST_ASSERT_EQ((unsigned char)buf[1], 0x9F, "emoji byte 1");
    TEST_ASSERT_EQ((unsigned char)buf[2], 0x98, "emoji byte 2");
    TEST_ASSERT_EQ((unsigned char)buf[3], 0x80, "emoji byte 3");

    /* Boundaries */
    TEST_ASSERT_INT(codepoint_to_utf8(0x7F, buf),  1, "0x7F is 1-byte (ASCII)");
    TEST_ASSERT_EQ((unsigned char)buf[0], 0x7F, "0x7F byte value");
    TEST_ASSERT_INT(codepoint_to_utf8(0x7FF, buf), 2, "0x7FF is 2-byte");
    TEST_ASSERT_INT(codepoint_to_utf8(0x800, buf), 3, "0x800 is 3-byte");
    TEST_ASSERT_INT(codepoint_to_utf8(0xFFFF, buf), 3, "0xFFFF is 3-byte");
    TEST_ASSERT_INT(codepoint_to_utf8(0x10000, buf), 4, "0x10000 is 4-byte");
    TEST_ASSERT_INT(codepoint_to_utf8(0x10FFFF, buf), 4, "0x10FFFF is 4-byte");
}

/* ================================================================ */
/*  ujis_to_utf8                                                  */
/* ================================================================ */

static void test_ujis_to_utf8_ascii(void)
{
    char inbuf[4];
    char outbuf[32];
    char *outptr = outbuf;
    int rc;

    inbuf[0] = 'A';
    rc = ujis_to_utf8(inbuf, 1, &outptr);
    TEST_ASSERT_INT(rc, 1, "ujis_to_utf8 ASCII return");
    TEST_ASSERT_INT(outptr - outbuf, 1, "ujis_to_utf8 ASCII bytes");
    TEST_ASSERT_EQ((unsigned char)outbuf[0], 'A', "ujis_to_utf8 ASCII byte");
}

static void test_ujis_to_utf8_fullwidth_space(void)
{
    /* EUC-JP 0xA1A1 = fullwidth space U+3000 -> UTF-8 E3 80 80 */
    char inbuf[4];
    char outbuf[32];
    char *outptr = outbuf;
    int rc, outlen;

    inbuf[0] = 0xA1; inbuf[1] = 0xA1;
    rc = ujis_to_utf8(inbuf, 2, &outptr);
    TEST_ASSERT_INT(rc, 1, "ujis_to_utf8 fullwidth space return");
    outlen = outptr - outbuf;
    TEST_ASSERT_INT(outlen, 3, "fullwidth space should produce 3 UTF-8 bytes");
    if (outlen == 3) {
        TEST_ASSERT_EQ((unsigned char)outbuf[0], 0xE3, "0xA1A1 byte 0");
        TEST_ASSERT_EQ((unsigned char)outbuf[1], 0x80, "0xA1A1 byte 1");
        TEST_ASSERT_EQ((unsigned char)outbuf[2], 0x80, "0xA1A1 byte 2");
    }
}

/* ================================================================ */
/*  iswidechar / is_narrow_kanji                                  */
/* ================================================================ */

static void test_iswidechar_ascii(void)
{
    TEST_ASSERT(!iswidechar('A'), "ASCII not wide");
    TEST_ASSERT(!iswidechar(0x7F), "0x7F not wide");
}

static void test_iswidechar_kana(void)
{
    TEST_ASSERT(!iswidechar(0x0141), "kana not wide");
    TEST_ASSERT(!iswidechar(0x015F), "kana upper not wide");
}

static void test_is_narrow_kanji(void)
{
    /* JIS rows 1-3 (symbols, Greek, Latin): narrow */
    TEST_ASSERT(is_narrow_kanji(0x2121), "row 1 narrow");
    TEST_ASSERT(is_narrow_kanji(0x237A), "row 3 narrow");
    /* JIS row 4 (Hiragana): WIDE — same width as kanji */
    TEST_ASSERT(!is_narrow_kanji(0x2421), "row 4 (hiragana) wide");
    TEST_ASSERT(!is_narrow_kanji(0x2447), "row 4 hiragana で wide");
    TEST_ASSERT(!is_narrow_kanji(0x2439), "row 4 hiragana す wide");
    /* JIS row 5 (Katakana): WIDE — same width as kanji */
    TEST_ASSERT(!is_narrow_kanji(0x2521), "row 5 (katakana) wide");
    /* JIS rows 0x28+ (box drawing, kanji): WIDE */
    TEST_ASSERT(!is_narrow_kanji(0x4E42), "high kanji not narrow");
    /* Special: 0x264C (Greek mu) is narrow */
    TEST_ASSERT(is_narrow_kanji(0x264C), "0x264C narrow");
    /* JIS row 6 (Greek) and row 7 (Cyrillic): narrow */
    TEST_ASSERT(is_narrow_kanji(0x2601), "row 6 (Greek) narrow");
    TEST_ASSERT(is_narrow_kanji(0x2701), "row 7 (Cyrillic) narrow");
}

static void test_iswidechar_kanji(void)
{
    TEST_ASSERT(iswidechar(0x4E42), "high kanji wide");
    /* Hiragana and katakana are WIDE (same as kanji) */
    TEST_ASSERT(iswidechar(0x2521), "row 5 katakana WIDE");
    TEST_ASSERT(iswidechar(0x2422), "row 4 hiragana あ WIDE");
    TEST_ASSERT(iswidechar(0x2447), "row 4 hiragana で WIDE");
    TEST_ASSERT(iswidechar(0x2421), "row 4 hiragana 0x2421 WIDE");
    TEST_ASSERT(iswidechar(0x3021), "row 0x30 wide");
    TEST_ASSERT(iswidechar(0x4E42), "high kanji wide");
    /* Unicode_MARK | codepoint: real wide characters */
    TEST_ASSERT(iswidechar(0x3000 | 0x200000), "tagged ideographic space wide");
    TEST_ASSERT(iswidechar(0x1F600 | 0x200000), "tagged emoji wide");
    /* Hiragana U+3042 is WIDE (same as kanji) */
    TEST_ASSERT(iswidechar(0x3042 | 0x200000), "tagged hiragana WIDE");
    /* Katakana U+30A2 is WIDE (same as kanji) */
    TEST_ASSERT(iswidechar(0x30A2 | 0x200000), "tagged katakana WIDE");
    TEST_ASSERT(!iswidechar(0x2121), "row 1 not wide");
    TEST_ASSERT(!iswidechar(0x264C), "0x264C not wide");
    /* Narrow BMP Unicode (e-acute U+00E9) is NOT wide */
    TEST_ASSERT(!iswidechar(0x00E9 | 0x200000), "U+00E9 not wide");
    /* Combining chars are never wide */
    TEST_ASSERT(!iswidechar(0x0300 | 0x200000), "combining acute not wide");
}

/* ================================================================ */
/*  char_width / iscombchar (Unicode East Asian Width)              */
/* ================================================================ */

static void test_char_width_ascii(void)
{
    TEST_ASSERT_INT(char_width('A'), 1, "ASCII A width 1");
    TEST_ASSERT_INT(char_width(0x7E), 1, "0x7E width 1");
    TEST_ASSERT_INT(char_width(0x00), 1, "NUL width 1 (fallback)");
}

static void test_char_width_kanji(void)
{
    /* JIS X 0208 narrow rows (1-3, symbols, Greek): width 1 */
    TEST_ASSERT_INT(char_width(0x2121), 1, "0x2121 (row 1) narrow");
    TEST_ASSERT_INT(char_width(0x237A), 1, "0x237A (row 3) narrow");
    TEST_ASSERT_INT(char_width(0x264C), 1, "0x264C (Greek mu) narrow");
    TEST_ASSERT_INT(char_width(0x2601), 1, "0x2601 (Greek alpha) narrow");
    /* JIS X 0208 wide rows: width 2 */
    TEST_ASSERT_INT(char_width(0x3021), 2, "0x3021 wide");
    TEST_ASSERT_INT(char_width(0x4E42), 2, "0x4E42 wide");
    /* Hiragana in row 0x24 is WIDE (same as kanji) */
    TEST_ASSERT_INT(char_width(0x2422), 2, "0x2422 (hiragana あ) wide (same as kanji)");
    TEST_ASSERT_INT(char_width(0x2447), 2, "0x2447 (hiragana で) wide (same as kanji)");
    TEST_ASSERT_INT(char_width(0x2439), 2, "0x2439 (hiragana す) wide (same as kanji)");
    /* Katakana in row 0x25 is WIDE (same as kanji) */
    TEST_ASSERT_INT(char_width(0x2522), 2, "0x2522 (katakana ア) wide (same as kanji)");
    TEST_ASSERT_INT(char_width(0x3D29), 2, "0x3D29 (kanji 秋) wide");
}

static void test_char_width_halfwidth_kana(void)
{
    TEST_ASSERT_INT(char_width(0x0141), 1, "half-width kana narrow");
    TEST_ASSERT_INT(char_width(0x015F), 1, "half-width kana narrow");
    TEST_ASSERT_INT(char_width(0x0100), 1, "half-width kana boundary");
    TEST_ASSERT_INT(char_width(0x017F), 1, "half-width kana boundary");
}

static void test_char_width_unicode_cjk(void)
{
    /* BMP fullwidth via UNICODE_MARK tag */
    TEST_ASSERT_INT(char_width(0xFF01 | 0x200000), 2, "U+FF01 fullwidth wide");
    TEST_ASSERT_INT(char_width(0x3000 | 0x200000), 2, "U+3000 ideographic space wide");
    /* Direct codepoints >= 0x10000 */
    TEST_ASSERT_INT(char_width(0x1F600), 2, "U+1F600 emoji wide");
    /* Hiragana U+3042 stored as Unicode (tagged with UNICODE_MARK) is WIDE
       (same as kanji) */
    TEST_ASSERT_INT(char_width(0x3042 | 0x200000), 2, "U+3042 hiragana wide (tagged, same as kanji)");
    /* Katakana U+30A2 stored as Unicode (tagged with UNICODE_MARK) is WIDE
       (same as kanji) */
    TEST_ASSERT_INT(char_width(0x30A2 | 0x200000), 2, "U+30A2 katakana wide (tagged, same as kanji)");
    /* Note: 0x3042 without UNICODE_MARK is treated as JIS row 0x30 (kanji, wide) */
    TEST_ASSERT_INT(char_width(0xAC00), 2, "U+AC00 hangul wide");
    TEST_ASSERT_INT(char_width(0x4E00), 2, "U+4E00 CJK wide");
    /* CJK Extension B */
    TEST_ASSERT_INT(char_width(0x20000), 2, "U+20000 CJK Ext B wide");
}

static void test_char_width_combining(void)
{
    /* Combining diacritical marks (width 0) */
    TEST_ASSERT_INT(char_width(0x0300 | 0x200000), 0, "U+0300 combining grave");
    TEST_ASSERT_INT(char_width(0x0301 | 0x200000), 0, "U+0301 combining acute");
    /* ZWJ (U+200D) and ZWNJ (U+200C) */
    TEST_ASSERT_INT(char_width(0x200D | 0x200000), 0, "U+200D ZWJ");
    TEST_ASSERT_INT(char_width(0x200C | 0x200000), 0, "U+200C ZWNJ");
    /* Variation selectors */
    TEST_ASSERT_INT(char_width(0xFE0F | 0x200000), 0, "U+FE0F variation selector");
    TEST_ASSERT_INT(char_width(0xFE00 | 0x200000), 0, "U+FE00 variation selector");
    /* BOM / ZWNBSP */
    TEST_ASSERT_INT(char_width(0xFEFF | 0x200000), 0, "U+FEFF ZWNBSP");
}

static void test_char_width_narrow_unicode(void)
{
    /* BMP characters that are NOT wide (narrow or ambiguous -> width 1) */
    TEST_ASSERT_INT(char_width(0x00E9 | 0x200000), 1, "U+00E9 e-acute narrow");
    TEST_ASSERT_INT(char_width(0x00A1 | 0x200000), 1, "U+00A1 inverted bang narrow");
    TEST_ASSERT_INT(char_width(0x2010 | 0x200000), 1, "U+2010 hyphen narrow");
}

static void test_iscombchar(void)
{
    TEST_ASSERT(iscombchar(0x0301 | 0x200000), "U+0301 combining");
    TEST_ASSERT(iscombchar(0x200D | 0x200000), "U+200D ZWJ");
    TEST_ASSERT(iscombchar(0xFE0F | 0x200000), "U+FE0F variation selector");
    TEST_ASSERT(iscombchar(0xFEFF | 0x200000), "U+FEFF ZWNBSP");
    TEST_ASSERT(!iscombchar('A'), "ASCII not combining");
    TEST_ASSERT(!iscombchar(0x4E42), "CJK kanji not combining");
    TEST_ASSERT(!iscombchar(0x1F600), "emoji not combining");
    TEST_ASSERT(!iscombchar(0x200000 | 0x3000), "ideographic space not combining");
}

static void test_char_width_8bit(void)
{
    /* 8-bit characters (0x80-0xFF): fallback to width 1 */
    TEST_ASSERT_INT(char_width(0x80), 1, "0x80 width 1");
    TEST_ASSERT_INT(char_width(0xFF), 1, "0xFF width 1");
}

/*
 * Regression test for: "秋です" displays correctly with consistent
 * hiragana/katakana width.
 *
 * 秋 (U+79CB) -> EUC-JP 0xBD 0xA9 -> internal 0x3D29 (row 0x3D, kanji, wide)
 * で (U+3067) -> EUC-JP 0xA4 0xC7 -> internal 0x2447 (row 0x24, hiragana)
 * す (U+3059) -> EUC-JP 0xA4 0xB9 -> internal 0x2439 (row 0x24, hiragana)
 *
 * Hiragana (row 0x24) and katakana (row 0x25) are now treated as WIDE (width 2),
 * same as kanji.  The total width of "秋です" is 6 (2 + 2 + 2).
 * Half-width katakana (enkana, 0x0100-0x017F) remain narrow (width 1).
 */
static void test_hiragana_katakana_width(void)
{
    /* 秋 (U+79CB) -> internal 0x3D29 (row 0x3D, kanji, wide) */
    unsigned int kaku = 0x3D29;
    /* で (U+3067) -> internal 0x2447 (row 0x24, hiragana, WIDE) */
    unsigned int de = 0x2447;
    /* す (U+3059) -> internal 0x2439 (row 0x24, hiragana, WIDE) */
    unsigned int su = 0x2439;
    /* ア (U+30A2) -> internal 0x2522 (row 0x25, katakana, WIDE) */
    unsigned int a = 0x2522;

    /* All three characters are wide (width 2), same as kanji */
    TEST_ASSERT_INT(char_width(kaku), 2, "秋 width 2 (wide, kanji)");
    TEST_ASSERT_INT(char_width(de),  2, "で width 2 (wide, hiragana same as kanji)");
    TEST_ASSERT_INT(char_width(su),  2, "す width 2 (wide, hiragana same as kanji)");
    TEST_ASSERT_INT(char_width(a),   2, "ア width 2 (wide, katakana same as kanji)");

    /* is_narrow_kanji must return false for hiragana (row 0x24) and katakana */
    TEST_ASSERT(!is_narrow_kanji(de), "で is NOT narrow kanji (wide)");
    TEST_ASSERT(!is_narrow_kanji(su), "す is NOT narrow kanji (wide)");
    TEST_ASSERT(!is_narrow_kanji(a),  "ア is NOT narrow kanji (wide)");
    /* is_narrow_kanji must still return true for genuinely narrow rows */
    TEST_ASSERT(is_narrow_kanji(0x264C), "0x264C (Greek mu) still narrow");

    /* Total width of "秋です" is 6 (2 + 2 + 2) */
    unsigned int total = 0;
    total += char_width(kaku);
    total += char_width(de);
    total += char_width(su);
    TEST_ASSERT_INT(total, 6, "total width of 秋です is 6 (2+2+2)");

    /* Half-width katakana (enkana 0x0100-0x017F) remain narrow */
    TEST_ASSERT_INT(char_width(0x0141), 1, "half-width katakana 0x0141 still narrow");
}

TEST_LIST({
    {"stoj/jtos known pairs",  test_stoj_jtos_known},
    {"stoj/jtos round-trip",   test_stoj_jtos_roundtrip},
    {"stoj invalid input",     test_stoj_invalid},
    {"jtos invalid input",     test_jtos_invalid},
    {"utf8_to_codepoint",      test_utf8_to_codepoint},
    {"utf8_to_internal ascii", test_utf8_to_internal_ascii},
    {"utf8_to_internal invalid", test_utf8_to_internal_invalid},
    {"codepoint_to_utf8",      test_codepoint_to_utf8},
    {"ujis_to_utf8 ascii",     test_ujis_to_utf8_ascii},
    {"ujis_to_utf8 fullwidth", test_ujis_to_utf8_fullwidth_space},
    {"iswidechar ascii",       test_iswidechar_ascii},
    {"iswidechar kana",        test_iswidechar_kana},
    {"is_narrow_kanji",        test_is_narrow_kanji},
    {"iswidechar kanji",       test_iswidechar_kanji},
    {"char_width ascii",       test_char_width_ascii},
    {"char_width kanji",       test_char_width_kanji},
    {"char_width hankaku kana", test_char_width_halfwidth_kana},
    {"char_width unicode cjk", test_char_width_unicode_cjk},
    {"char_width combining",   test_char_width_combining},
    {"char_width narrow unicode", test_char_width_narrow_unicode},
    {"iscombchar",             test_iscombchar},
    {"char_width 8bit",        test_char_width_8bit},
    {"hiragana/katakana width",  test_hiragana_katakana_width},
})

int main(void)
{
    RUN_TESTS();
}
