#!/usr/bin/env python3
"""Test: dump raw terminal output to check if $ is sent for wide char overflow."""
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
        with open('/tmp/test_raw3.txt', 'w') as f:
            f.write(file_content)
        filepath = '/tmp/test_raw3.txt'
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
        os.write(master, b'\x18\x63')
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

if __name__ == '__main__':
    # Create test file with wide characters
    # Line 1: 81 ASCII chars → should have $
    line1 = 'A' * 81
    # Line 2: 41 hiragana → 82 terminal cols → should have $
    line2 = '\u3042' * 41  # 41 x 2 = 82 cols
    # Line 3: 40 hiragana → 80 cols → exact fit, no $
    line3 = '\u3042' * 40  # 40 x 2 = 80 cols
    # Line 4: 79 ASCII + 1 hiragana → 81 cols → should have $
    line4 = 'B' * 79 + '\u3042'
    # Line 5: 78 ASCII + 2 hiragana → 82 cols → should have $
    line5 = 'B' * 78 + '\u3042' * 2

    file_content = '\n'.join([line1, line2, line3, line4, line5]) + '\n'

    rc_content = "set $linenumbers FALSE\n"

    output = run_kemacs([
        b'\x18\x13',  # Ctrl-X Ctrl-S = upscreen (full redraw)
    ], rc_content=rc_content, file_content=file_content)

    # Find all $ (0x24) positions in the output
    # But also look at escape sequences to understand cursor positioning
    print("=== Searching for '$' (0x24) in output ===")
    dollar_positions = [(i, output[i:i+10]) for i in range(len(output)) if output[i] == 0x24 and output[i-1:i] != b'\x1b[']
    for pos, context in dollar_positions:
        context_hex = ' '.join(f'{b:02x}' for b in context)
        context_ascii = ''.join(chr(b) if 32 <= b < 127 else '.' for b in context)
        print(f"  Position {pos}: hex={context_hex} ascii={context_ascii}")

    print(f"\n=== Full output (hex dump of last 800 bytes) ===")
    start = max(0, len(output) - 800)
    for i in range(start, len(output), 16):
        chunk = output[i:i+16]
        hex_part = ' '.join(f'{b:02x}' for b in chunk)
        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        print(f"  {i:4d}: {hex_part:48s} {ascii_part}")

    print(f"\n=== Full output (hex dump from beginning, first 800 bytes) ===")
    for i in range(0, min(800, len(output)), 16):
        chunk = output[i:i+16]
        hex_part = ' '.join(f'{b:02x}' for b in chunk)
        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        print(f"  {i:4d}: {hex_part:48s} {ascii_part}")
