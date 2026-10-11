#!/usr/bin/env python3
"""Test: display lines with wide chars + overflow, check for '$' marker."""
import os, sys, time, select, pty, struct, fcntl, termios, signal

COLS = 80
ROWS = 24

def run_kemacs(keys_to_send, rc_content=None, file_content=None):
    master, slave = pty.openpty()
    winsize = struct.pack('HHHH', ROWS, COLS, 0, 0)
    fcntl.ioctl(master, termios.TIOCSWINSZ, winsize)

    if rc_content:
        os.makedirs('/tmp/test_rc3', exist_ok=True)
        with open('/tmp/test_rc3/.kemacsrc', 'w') as f:
            f.write(rc_content)
        env_home = '/tmp/test_rc3'
    else:
        env_home = '/home/niwan'

    if file_content:
        with open('/tmp/test_wide2.txt', 'w') as f:
            f.write(file_content)
        filepath = '/tmp/test_wide2.txt'
    else:
        filepath = '/tmp/test_long.txt'

    env = os.environ.copy()
    env['HOME'] = env_home
    env['TERM'] = 'xterm-256color'

    pid = os.fork()
    if pid == 0:
        os.close(master)
        os.setsid()
        fcntl.ioctl(slave, termios.TIOCSCTTY, 0)
        os.dup2(slave, 0)
        os.dup2(slave, 1)
        os.dup2(slave, 2)
        if slave > 2:
            os.close(slave)
        os.execvpe('./kemacs', ['./kemacs', '-f', filepath], env)
        os._exit(1)

    os.close(slave)
    output = b''

    time.sleep(0.5)
    while True:
        r, _, _ = select.select([master], [], [], 0.5)
        if r:
            try:
                data = os.read(master, 65536)
                if not data:
                    break
                output += data
            except OSError:
                break
        else:
            break

    for key_seq in keys_to_send:
        os.write(master, key_seq)
        time.sleep(0.3)
        while True:
            r, _, _ = select.select([master], [], [], 0.3)
            if r:
                try:
                    data = os.read(master, 65536)
                    if not data:
                        break
                    output += data
                except OSError:
                    break
            else:
                break

    time.sleep(0.3)
    while True:
        r, _, _ = select.select([master], [], [], 0.3)
        if r:
            try:
                data = os.read(master, 65536)
                if not data:
                    break
                output += data
            except OSError:
                break
        else:
            break

    try:
        os.write(master, b'\x18\x63')  # Ctrl-X c (exit)
    except:
        pass
    time.sleep(0.1)
    try:
        os.kill(pid, signal.SIGTERM)
        os.waitpid(pid, 0)
    except:
        pass
    os.close(master)
    return output

def extract_screen(data):
    """Extract screen content from ANSI escape sequences."""
    lines = {}
    current_text = {}
    row = 0
    col = 0
    i = 0
    while i < len(data):
        c = data[i]
        if c == 0x1b:
            i += 1
            if i < len(data):
                next_c = data[i]
                if next_c == ord('['):
                    i += 1
                    params = b''
                    while i < len(data) and (data[i:i+1].isdigit() or data[i:i+1] == b';'):
                        params += data[i:i+1]
                        i += 1
                    if i < len(data):
                        cmd = data[i]
                        i += 1
                        if cmd == ord('H'):
                            parts = params.split(b';')
                            if len(parts) >= 2:
                                row = int(parts[0]) - 1 if parts[0] else 0
                                col = int(parts[1]) - 1 if parts[1] else 0
                            elif len(parts) >= 1 and parts[0]:
                                row = int(parts[0]) - 1
                                col = 0
                        elif cmd == ord('A'):
                            n = int(params) if params else 1
                            row = max(0, row - n)
                        elif cmd == ord('B'):
                            n = int(params) if params else 1
                            row = min(ROWS - 1, row + n)
                        elif cmd == ord('C'):
                            n = int(params) if params else 1
                            col += n
                        elif cmd == ord('D'):
                            n = int(params) if params else 1
                            col = max(0, col - n)
                        elif cmd == ord('J'):
                            n = int(params) if params else 0
                            if n == 2:
                                current_text = {}
                                row = 0
                                col = 0
                        elif cmd == ord('K'):
                            if row in current_text:
                                current_text[row] = {k:v for k,v in current_text[row].items() if k < col}
                            else:
                                current_text[row] = {}
                        elif cmd == ord('m'):
                            pass
                        elif cmd == ord('?'):
                            while i < len(data) and data[i:i+1] not in (b'h', b'l', b'b', b'0'):
                                i += 1
                            if i < len(data):
                                i += 1
                    else:
                        i += 1
                elif next_c in (ord('M'), ord('D')):
                    i += 1
                elif next_c in (ord('7'), ord('8'), ord('='), ord('>'), ord('c')):
                    i += 1
                elif next_c == ord('(') or next_c == ord(')'):
                    i += 2
                elif next_c == ord(' '):
                    i += 2
                    while i < len(data) and data[i:i+1].isalpha():
                        i += 1
                else:
                    i += 1
            else:
                i += 1
        elif c == ord('\r'):
            i += 1
            col = 0
        elif c == ord('\n'):
            i += 1
        elif c < 0x20:
            i += 1
        else:
            if row not in current_text:
                current_text[row] = {}
            # For UTF-8 multi-byte chars, treat as one column but advance by 1
            # (This is approximate - real terminal handles wide chars differently)
            if col < COLS:
                current_text[row][col] = chr(c) if c < 128 else '?'
            col += 1
            i += 1

    lines = {}
    for r, positions in current_text.items():
        if positions:
            max_col = max(positions.keys())
            line = ''.join(positions.get(c, ' ') for c in range(max_col + 1))
            lines[r] = line
    return lines, output

if __name__ == '__main__':
    # Create test file with wide characters (hiragana/katakana/kanji)
    # that overflow an 80-column terminal
    # Line 1: exactly 80 ASCII chars (no overflow, no $)
    line1 = 'A' * 80
    # Line 2: 81 ASCII chars (overflow, should have $)
    line2 = 'A' * 81
    # Line 3: 40 hiragana (80 columns, exact fit, no $)
    line3 = '\u3042' * 40  # あ * 40 = 80 columns
    # Line 4: 41 hiragana (82 columns, overflow, should have $)
    line4 = '\u3042' * 41  # あ * 41 = 82 columns
    # Line 5: 78 ASCII + 1 hiragana at end (80 columns, exact fit, no $)
    line5 = 'B' * 78 + '\u3042'
    # Line 6: 78 ASCII + 1 hiragana + 1 more char (overflow, should have $)
    line6 = 'B' * 78 + '\u3042' + 'C'
    # Line 7: 39 hiragana + 1 hiragana at end (80 columns, exact fit)
    line7 = '\u3042' * 40
    # Line 8: 40 hiragana + 1 more (overflow, should have $)
    line8 = '\u3042' * 40 + 'X'

    file_content = '\n'.join([line1, line2, line3, line4, line5, line6, line7, line8]) + '\n'

    rc_content = "set $linenumbers FALSE\n"

    output = run_kemacs([
        b'\x18\x13',  # Ctrl-X Ctrl-S = upscreen (full redraw)
    ], rc_content=rc_content, file_content=file_content)

    lines, raw = extract_screen(output)

    print(f"Total output: {len(raw)} bytes")
    print()

    for i in range(ROWS):
        line = lines.get(i, '')
        line = line.ljust(COLS)[:COLS]
        print(f"Row {i:2d}: [{line[:80]}]")

    print("\n--- Analysis ---")
    for i in range(ROWS):
        line = lines.get(i, '')
        if '$' in line:
            dollar_pos = line.index('$')
            print(f"  Row {i}: '$' at col {dollar_pos}")
        elif len(line) >= 80:
            print(f"  Row {i}: NO '$' but line is 80+ cols long! Last char: '{line[79] if len(line) > 79 else 'N/A'}'")
        else:
            print(f"  Row {i}: line length = {len(line)}")
