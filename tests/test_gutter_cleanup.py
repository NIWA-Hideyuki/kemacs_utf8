#!/usr/bin/env python3
"""PTY test: run kemacs with line numbers and check gutter for garbage."""
import os
import pty
import struct
import fcntl
import termios
import time
import sys
import select

def run_kemacs_pyt(commands, lines=24, cols=60, kemacsrc=None):
    """Run kemacs in a PTY, return captured output bytes."""
    master, slave = pty.openpty()
    winsize = struct.pack('HHHH', lines, cols, 0, 0)
    fcntl.ioctl(slave, termios.TIOCSWINSZ, winsize)

    pid = os.fork()
    if pid == 0:
        os.setsid()
        os.dup2(slave, 0)
        os.dup2(slave, 1)
        os.dup2(slave, 2)
        os.close(master)
        os.close(slave)
        env = os.environ.copy()
        env['TERM'] = 'xterm'
        env['COLUMNS'] = str(cols)
        env['LINES'] = str(lines)
        env['HOME'] = '/tmp/kh2'
        os.makedirs(env['HOME'], exist_ok=True)
        if kemacsrc:
            with open(os.path.join(env['HOME'], '.kemacsrc'), 'w') as f:
                f.write(kemacsrc)
        else:
            with open(os.path.join(env['HOME'], '.kemacsrc'), 'w') as f:
                f.write('set $linenumbers TRUE\n')
        cmd = ['./kemacs', '/tmp/gutter_test.txt']
        for c in commands:
            cmd += ['-x', c]
        os.execvpe(cmd[0], cmd, env)
        os._exit(1)

    os.close(slave)
    output = b''
    start = time.time()
    while True:
        if time.time() - start > 8:
            break
        r, _, _ = select.select([master], [], [], 0.1)
        if r:
            try:
                data = os.read(master, 4096)
            except OSError:
                break
            if not data:
                break
            output += data
        # Check if child exited
        try:
            wpid, status = os.waitpid(pid, os.WNOHANG)
            if wpid != 0:
                # Drain remaining output
                time.sleep(0.1)
                try:
                    while True:
                        r2, _, _ = select.select([master], [], [], 0.1)
                        if r2:
                            data = os.read(master, 4096)
                            if not data:
                                break
                            output += data
                        else:
                            break
                except OSError:
                    pass
                break
        except ChildProcessError:
            break

    try:
        os.close(master)
    except OSError:
        pass
    return output

def parse_ansi_screen(data):
    """Parse ANSI escape sequences to reconstruct the screen grid."""
    screen = {}
    cur_row = 0
    cur_col = 0

    i = 0
    while i < len(data):
        if data[i:i+1] == b'\x1b':
            if i+1 < len(data) and data[i+1:i+2] == b'[':
                j = i + 2
                params = b''
                while j < len(data) and data[j:j+1] not in (b'A', b'B', b'C', b'D', b'H', b'J', b'K', b'm', b'r'):
                    params += data[j:j+1]
                    j += 1
                if j < len(data):
                    cmd = data[j:j+1]
                    ps = params.decode('ascii', errors='replace')
                    if cmd == b'H':
                        parts = ps.strip().split(';')
                        if len(parts) == 2:
                            try:
                                cur_row = int(parts[0]) - 1
                                cur_col = int(parts[1]) - 1
                            except:
                                pass
                        elif len(parts) == 1:
                            try:
                                cur_row = int(parts[0]) - 1
                                cur_col = 0
                            except:
                                pass
                    elif cmd == b'J':
                        if ps.strip() == '2':
                            screen = {}
                    elif cmd == b'K':
                        for c in range(cur_col, 300):
                            screen[(cur_row, c)] = ' '
                    i = j + 1
                    continue
                else:
                    i = j
                    continue
            elif i+1 < len(data) and data[i+1:i+2] == b']':
                j = i + 2
                if j < len(data) and data[j:j+1] == b'0':
                    j += 1
                    while j < len(data) and data[j:j+1] != b'\x07':
                        j += 1
                else:
                    while j < len(data) and data[j:j+1] not in (b'\x07', b'\x1b'):
                        j += 1
                i = j + 1
                continue
            elif i+1 < len(data) and data[i+1:i+2] in (b'=', b'>'):
                i += 2
                continue
            i += 1
            continue
        elif data[i:i+1] == b'\r':
            cur_col = 0
            i += 1
            continue
        elif data[i:i+1] == b'\n':
            cur_row += 1
            cur_col = 0
            i += 1
            continue
        elif data[i:i+1] == b'\x07':
            i += 1
            continue
        elif data[i:i+1] == b'\x08':
            cur_col = max(0, cur_col - 1)
            i += 1
            continue

        c = data[i:i+1]
        if len(c) == 1:
            try:
                ch = c.decode('utf-8', errors='replace')
            except:
                ch = '?'
            screen[(cur_row, cur_col)] = ch
        cur_col += 1
        i += 1

    return screen

def print_screen(screen, rows=22, cols=60):
    """Print the screen content for debugging."""
    for row in range(rows):
        chars = []
        for col in range(cols):
            c = screen.get((row, col), ' ')
            chars.append(c)
        print(f"  {row:2d} |{''.join(chars)}|")

def check_gutter(screen, gw=7):
    """Check that gutter columns contain only digits or spaces."""
    errors = []
    rows = set(r for (r, c) in screen.keys())
    for row in sorted(rows):
        if row > 21:
            continue
        gutter_chars = []
        for col in range(gw):
            c = screen.get((row, col), ' ')
            gutter_chars.append(c)
        gutter_str = ''.join(gutter_chars)
        cleaned = gutter_str.strip()
        if cleaned == '':
            continue  # all spaces, OK
        # Check for non-digit, non-space
        for k, c in enumerate(gutter_str):
            if c != ' ' and not c.isdigit():
                errors.append(f"Row {row}: gutter garbage '{c}' (ord={ord(c)}) at col {k}: '{gutter_str}'")
        # Check right-justification: digits should be right-aligned
        # The last char (col gw-1) is the separator space — allow it
        if cleaned:
            leftmost = gutter_str.index(cleaned[0])
            # Everything before leftmost should be spaces
            for k in range(leftmost):
                if gutter_str[k] != ' ':
                    errors.append(f"Row {row}: non-space before digits: '{gutter_str}'")
            # Everything from leftmost to gw-2 should be digits (last col is separator)
            for k in range(leftmost, gw - 1):
                if not gutter_str[k].isdigit():
                    errors.append(f"Row {row}: non-digit in number area: '{gutter_str}'")
    return errors

def main():
    print("=== Test 1: 3-line file on 24-row terminal, exit-emacs ===")
    output = run_kemacs_pyt(['exit-emacs'])
    screen = parse_ansi_screen(output)
    errors = check_gutter(screen, gw=7)
    if errors:
        print("FAIL:")
        for e in errors[:20]:
            print(f"  {e}")
        print("\nScreen:")
        print_screen(screen)
        sys.exit(1)
    else:
        print("PASS: gutter clean")

    print("\n=== Test 2: refresh-screen then exit ===")
    output = run_kemacs_pyt(['refresh-screen', 'exit-emacs'])
    screen = parse_ansi_screen(output)
    errors = check_gutter(screen, gw=7)
    if errors:
        print("FAIL:")
        for e in errors[:20]:
            print(f"  {e}")
        print("\nScreen:")
        print_screen(screen)
        sys.exit(1)
    else:
        print("PASS: gutter clean after refresh")

    print("\n=== Test 3: scroll down (one-line) then exit ===")
    output = run_kemacs_pyt(['scroll-down-line', 'exit-emacs'])
    screen = parse_ansi_screen(output)
    errors = check_gutter(screen, gw=7)
    if errors:
        print("FAIL:")
        for e in errors[:20]:
            print(f"  {e}")
        print("\nScreen:")
        print_screen(screen)
        sys.exit(1)
    else:
        print("PASS: gutter clean after scroll")

    print("\n=== Test 4: toggle line numbers off then on ===")
    output = run_kemacs_pyt(['toggle-line-numbers', 'toggle-line-numbers', 'exit-emacs'])
    screen = parse_ansi_screen(output)
    errors = check_gutter(screen, gw=7)
    if errors:
        print("FAIL:")
        for e in errors[:20]:
            print(f"  {e}")
        print("\nScreen:")
        print_screen(screen)
        sys.exit(1)
    else:
        print("PASS: gutter clean after toggle")

    print("\nAll tests passed!")

if __name__ == '__main__':
    main()
