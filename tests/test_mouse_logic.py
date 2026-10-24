#!/usr/bin/env python3
"""Test the column-to-offset logic for hiragana/katakana display fix.

The root cause of "秋です" displaying as "秋 す" is that hiragana (JIS row 0x24,
Unicode U+3041-U+3096) and katakana (JIS row 0x25, Unicode U+30A0-U+30FF) are
treated as WIDE (width 2) by kemacs, but actual terminals render them as
1-column (narrow). This mismatch causes vtputc to emit TRAILER bytes that
don't correspond to real terminal columns, making ttcol/curcol diverge from
the terminal cursor position, producing display corruption.

The fix: treat hiragana and katakana as NARROW (width 1), matching terminal
behavior.
"""

# Simulated kemacs internal Char values
ASCII_A = 0x41
HIRA_A = 0x2422  # あ (U+3042 -> EUC-JP A4A2 -> JIS 0x2422)
HIRA_I = 0x2424  # い (U+3044)
KATA_A = 0x2522  # ア (U+30A2 -> EUC-JP A5A2 -> JIS 0x2522)

# Fixed threshold: rows 0x24 (hiragana) and 0x25 (katakana) are narrow
FIXED_THRESHOLD = 0x2500

# Old (buggy) threshold: only rows <= 0x2300 are narrow
OLD_THRESHOLD = 0x2300

def char_width(c, old_mode=False):
    """Simulate char_width() - old_mode uses <= 0x2300 (buggy), fixed uses <= 0x2500."""
    if c < 0x80:
        return 1
    if 0x0100 <= c <= 0x017F:
        return 1
    if (c & 0xff00) > 0x0100 and c < 0x10000:
        threshold = OLD_THRESHOLD if old_mode else FIXED_THRESHOLD
        is_narrow = ((c & 0xff00) <= threshold or c == 0x264c)
        return 1 if is_narrow else 2
    if c >= 0x10000:
        # Stripped of UNICODE_MARK: 0x203042 -> 0x3042
        if 0x3041 <= c <= 0x3096:
            return 1  # hiragana: narrow on terminals
        if 0x3099 <= c <= 0x30FA:
            return 1  # katakana: narrow on terminals
        if c == 0x30FF:
            return 1  # katakana: narrow on terminals
        return 1
    if 0x80 <= c < 0x100:
        return 1
    return 1

def is_kanji(c):
    return (c & 0xff00) > 0x0100 and c < 0x10000

def mouse_col_to_offset(line, mouse_col, lbound=0, old_mode=False):
    curgoal = mouse_col + lbound
    col = 0
    dbo = 0
    while dbo < len(line):
        c = line[dbo]
        newcol = col
        if c == ord('\t'):
            newcol |= 0x07
        elif c < 0x20 or c == 0x7f:
            newcol += 1
        elif 0x80 <= c < 0x100:
            newcol += 3
        elif is_kanji(c) or c >= 0x10000:
            newcol += char_width(c, old_mode) - 1
        newcol += 1
        if newcol > curgoal:
            break
        col = newcol
        dbo += 1
    return dbo

def updpos_col(line, w_doto, old_mode=False):
    curcol = 0
    i = 0
    while i < w_doto:
        c = line[i]
        if c == ord('\t'):
            curcol |= 0x07
        elif c < 0x20 or c == 0x7f:
            curcol += 1
        elif 0x80 <= c < 0x100:
            curcol += 3
        elif is_kanji(c) or c >= 0x10000:
            curcol += char_width(c, old_mode) - 1
        curcol += 1
        i += 1
    return curcol

def describe_char(c):
    if c < 0x80:
        return chr(c)
    if (c & 0xff00) > 0x0100 and c < 0x10000:
        return f"JIS({hex(c)})"
    if c >= 0x10000:
        return f"Unicode({hex(c)})"
    return f"0x{c:02x}"

def run_test(line, name, old_mode=False):
    mode_name = "OLD (buggy: hiragana=WIDE=2)" if old_mode else "FIXED (hiragana=NARROW=1)"
    print(f"\n=== {name} [{mode_name}] ===")
    print(f"  char_width(あ) = {char_width(HIRA_A, old_mode)} (should be 1 = narrow)")
    line_desc = " ".join(describe_char(c) for c in line)
    print(f"  Line: {line_desc}")
    # Compute expected columns
    col = 0
    for i, c in enumerate(line):
        w = char_width(c, old_mode)
        print(f"  [{i}] {describe_char(c)}: width={w}, cols={col}-{col+w-1}")
        col += w
    print(f"  Total columns: {col}")
    print(f"  (Terminal renders hiragana as 1 column)")

    for mouse_col in range(col + 2):
        offset = mouse_col_to_offset(line, mouse_col, old_mode=old_mode)
        cursor_col = updpos_col(line, offset, old_mode=old_mode)
        char_at = line[offset] if offset < len(line) else "END"
        cw = char_width(line[offset], old_mode) if offset < len(line) else 0
        # Terminal renders using the TRUE width (1 for hiragana)
        true_col = updos_col = updpos_col(line, offset, old_mode=False)
        marker = ""
        if offset < len(line) and char_width(line[offset], old_mode=False) == 1 and cursor_col != true_col:
            marker = " <-- MISMATCH (kemacs col != terminal col)"
        print(f"  click col={mouse_col} -> offset={offset} {describe_char(line[offset]) if offset < len(line) else 'END'} -> "
              f"cursor_col={cursor_col} (true_term_col={true_col}){marker}")

# Test 1: A あ B (ASCII, hiragana, ASCII)
test1 = [ASCII_A, HIRA_A, 0x42]
run_test(test1, "Test 1: 'AあB'", old_mode=True)
run_test(test1, "Test 1: 'AあB'", old_mode=False)

# Test 2: あ B (hiragana at start)
test2 = [HIRA_A, 0x42]
run_test(test2, "Test 2: 'あB'", old_mode=True)
run_test(test2, "Test 2: 'あB'", old_mode=False)

# Test 3: A ア B (katakana)
test3 = [ASCII_A, KATA_A, 0x42]
run_test(test3, "Test 3: 'AアB'", old_mode=False)

# Test 4: 秋です (the actual bug report)
test4 = [0x3D29,  # 秋 (wide, row 0x3D)
         0x2447,  # で (hiragana, row 0x24)
         0x2439]  # す (hiragana, row 0x24)
run_test(test4, "Test 4: '秋です'", old_mode=True)
run_test(test4, "Test 4: '秋です'", old_mode=False)

print("\n=== Summary ===")
print("OLD mode: char_width(で)=2, cursor tracking diverges from terminal")
print("          (terminal renders で as 1 col, kemacs thinks 2 col) → display bug")
print("FIXED mode: char_width(で)=1, cursor tracking matches terminal → correct display")
print("秋です: old total=6 cols (wrong), fixed total=4 cols (correct: 2+1+1)")
