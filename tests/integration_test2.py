#!/usr/bin/env python3
"""Integration test: move cursor to end of long line to trigger updext path."""
import os, sys, time, select, pty, struct, fcntl, termios, signal

COLS = 80
ROWS = 24

def run_kemacs_and_capture(keys_to_send):
    master, slave = pty.openpty()
    winsize = struct.pack('HHHH', ROWS, COLS, 0, 0)
    fcntl.ioctl(master, termios.TIOCSWINSZ, winsize)

    os.makedirs('/tmp/test_rc2', exist_ok=True)
    with open('/tmp/test_rc2/.kemacsrc', 'w') as f:
        f.write("set $linenumbers TRUE\nset $linenumwidth 6\n")

    lines = []
    lines.append('short')
    lines.append('A' * 100)
    lines.append('medium line here')
    file_content = '\n'.join(lines) + '\n'
    with open('/tmp/test_long2.txt', 'w') as f:
        f.write(file_content)

    env = os.environ.copy()
    env['HOME'] = '/tmp/test_rc2'
    env['TERM'] = 'xterm'

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
        os.execvpe('./kemacs', ['./kemacs', '-f', '/tmp/test_long2.txt'], env)
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
    time.sleep(0.2)
    try:
        os.kill(pid, signal.SIGTERM)
        os.waitpid(pid, 0)
    except:
        pass
    os.close(master)
    return output

def extract_screen(data):
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
                            while i < len(data) and data[i:i+1] not in (b'h', b'l'):
                                i += 1
                            if i < len(data):
                                i += 1
                    else:
                        i += 1
                elif next_c in (ord('M'), ord('D')):
                    i += 1
                elif next_c in (ord('7'), ord('8'), ord('='), ord('>'), ord('c')):
                    i += 1
                elif next_c in (ord('('), ord(')')):
                    i += 2
                else:
                    i += 1
            else:
                i += 1
        elif c == ord('\r'):
            col = 0
            i += 1
        elif c == ord('\n'):
            i += 1
        elif c < 0x20:
            i += 1
        else:
            if row not in current_text:
                current_text[row] = {}
            if col < COLS:
                current_text[row][col] = chr(c)
            col += 1
            i += 1

    lines = {}
    for r, positions in current_text.items():
        if positions:
            max_col = max(positions.keys())
            line = ''.join(positions.get(c, ' ') for c in range(max_col + 1))
            lines[r] = line
    return lines

def print_screen(lines):
    for i in range(ROWS):
        line = lines.get(i, '')
        if not line:
            continue
        line = line.ljust(COLS)[:COLS]
        gutter = line[:7]
        text = line[7:]
        print(f"Row {i:2d}: GUTTER=[{gutter}] TEXT=[{text}]")

def analyze(lines):
    print("\n--- Analysis ---")
    issues = 0
    for i in range(ROWS):
        line = lines.get(i, '')
        if len(line) >= 7:
            gutter = line[:7]
            for j, c in enumerate(gutter):
                if c != ' ' and not c.isdigit():
                    print(f"  Row {i}: unexpected char '{c}' at gutter col {j}: [{gutter}]")
                    issues += 1
                if c == '$':
                    print(f"  Row {i}: '$' found in gutter at col {j}: [{gutter}]")
                    issues += 1
    if issues == 0:
        print("  No gutter issues found!")
    # Show '$' positions in text
    for i in range(ROWS):
        line = lines.get(i, '')
        text = line[7:] if len(line) > 7 else ''
        if '$' in text:
            dollar_pos = text.index('$')
            print(f"  Row {i}: '$' at text col {dollar_pos} (screen col {7+dollar_pos})")

# Test 1: Initial screen with upscreen
print("=== Test 1: Initial screen (upscreen) ===")
output = run_kemacs_and_capture([
    b'\x18\x13',  # Ctrl-X Ctrl-S = upscreen
])
lines = extract_screen(output)
print_screen(lines)
analyze(lines)

# Test 2: Move cursor to end of long line 2 (100 chars)
# Line 2 is at screen row 1. Move down once, then to end of line
print("\n=== Test 2: Move to line 2, end of line (triggers updext) ===")
output2 = run_kemacs_and_capture([
    b'\x18\x13',  # Ctrl-X Ctrl-S = upscreen (initial redraw)
    b'\x02',      # Ctrl-B? No... Let me use arrow keys
    b'\x1b[B',    # Down arrow (next-line)
    b'\x1b[B',    # Down arrow (next-line) -> now on line 3 (100 A's) - wait, line 2 is "A"*100
    # Actually line 1 is "short", line 2 is "A"*100
    # Cursor starts on line 1. Down arrow -> line 2
    b'\x05',      # Ctrl-E? No. Let me use End key or Ctrl-E (end-of-line)
    # In kemacs, end-of-line is M-, or Ctrl-E? Let me check...
    # Actually kemacs uses Ctrl-E for end-of-line? Or M-.
    # Let me use the kemacs binding: end-of-line is Ctrl-E
    b'\x18\x13',  # Ctrl-X Ctrl-S = upscreen (force redraw after cursor move)
])
lines2 = extract_screen(output2)
print_screen(lines2)
analyze(lines2)
