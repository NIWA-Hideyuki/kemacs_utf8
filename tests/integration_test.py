#!/usr/bin/env python3
"""Integration test: run kemacs with line numbers and long lines, capture screen."""
import os, sys, time, select, pty, struct, fcntl, termios, signal

COLS = 80
ROWS = 24

def run_kemacs(keys_to_send, rc_content=None, file_content=None):
    """Run kemacs in a pty and capture output."""
    master, slave = pty.openpty()
    winsize = struct.pack('HHHH', ROWS, COLS, 0, 0)
    fcntl.ioctl(master, termios.TIOCSWINSZ, winsize)

    # Set up HOME with .kemacsrc
    if rc_content:
        os.makedirs('/tmp/test_rc2', exist_ok=True)
        with open('/tmp/test_rc2/.kemacsrc', 'w') as f:
            f.write(rc_content)
        env_home = '/tmp/test_rc2'
    else:
        env_home = '/home/niwan'

    if file_content:
        with open('/tmp/test_long2.txt', 'w') as f:
            f.write(file_content)
        filepath = '/tmp/test_long2.txt'
    else:
        filepath = '/tmp/test_long.txt'

    env = os.environ.copy()
    env['HOME'] = env_home
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
        os.execvpe('./kemacs', ['./kemacs', '-f', filepath], env)
        os._exit(1)

    os.close(slave)
    output = b''

    time.sleep(0.5)
    # Read initial output
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
        os.write(master, b'\x18\x75')  # Ctrl-X u (exit-emacs? or maybe quit)
    except:
        pass
    time.sleep(0.1)
    try:
        os.write(master, b'\x04')  # Ctrl-D
    except:
        pass
    time.sleep(0.1)
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
                            # Skip private mode sequences
                            while i < len(data) and data[i:i+1] not in (b'h', b'l', b'b', b'0'):
                                i += 1
                            if i < len(data):
                                i += 1
                    else:
                        i += 1
                elif next_c == ord('M') or next_c == ord('D'):
                    i += 1
                elif next_c in (ord('7'), ord('8'), ord('='), ord('>'), ord('c')):
                    i += 1
                elif next_c == ord('(') or next_c == ord(')'):
                    i += 2
                elif next_c == ord(' '):
                    i += 2  # Skip space + intermediate
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
            if col < COLS:
                current_text[row][col] = chr(c)
            col += 1
            i += 1

    # Merge into lines
    lines = {}
    for r, positions in current_text.items():
        if positions:
            max_col = max(positions.keys())
            line = ''.join(positions.get(c, ' ') for c in range(max_col + 1))
            lines[r] = line
    return lines, output

if __name__ == '__main__':
    # Create test file with long lines
    lines = []
    lines.append('short line')
    lines.append('A' * 100)
    lines.append('medium length line for testing')
    lines.append('B' * 80)
    lines.append('C' * 73)
    lines.append('line after long text')
    file_content = '\n'.join(lines) + '\n'

    # .kemacsrc with line numbers ON
    rc_content = """set $linenumbers TRUE
set $linenumwidth 6
"""

    # Run WITHOUT sending Ctrl-X L (since .kemacsrc already has it ON)
    # Just force full redraw with Ctrl-X Ctrl-S (upscreen)
    output = run_kemacs([
        b'\x18\x13',  # Ctrl-X Ctrl-S = upscreen (full redraw)
    ], rc_content=rc_content, file_content=file_content)

    lines, raw = extract_screen(output)
    
    print(f"Total output: {len(raw)} bytes")
    print()

    for i in range(ROWS):
        line = lines.get(i, '')
        line = line.ljust(COLS)[:COLS]
        gutter = line[:7]
        text = line[7:]
        print(f"Row {i:2d}: GUTTER=[{gutter}] TEXT=[{text}]")

    print("\n--- Analysis ---")
    # Check for '$' or non-digit/non-space in gutter
    for i in range(ROWS):
        line = lines.get(i, '')
        if len(line) >= 7:
            gutter = line[:7]
            for j, c in enumerate(gutter):
                if c != ' ' and not c.isdigit():
                    print(f"  Row {i}: unexpected char '{c}' at gutter col {j}: [{gutter}]")
                if c == '$':
                    print(f"  Row {i}: '$' found in gutter at col {j}: [{gutter}]")
    
    # Check for '$' in text area
    for i in range(ROWS):
        line = lines.get(i, '')
        text = line[7:] if len(line) > 7 else ''
        if '$' in text:
            dollar_pos = text.index('$')
            print(f"  Row {i}: '$' at text col {dollar_pos} (screen col {7+dollar_pos})")
