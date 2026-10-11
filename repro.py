#!/usr/bin/env python3
"""Test heart emoji encoding/decoding through kemacs in batch mode."""
import os, pty, time, select, re, sys

def read_available(fd, timeout=0.5):
    output = b''
    deadline = time.time() + timeout
    while time.time() < deadline:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                data = os.read(fd, 4096)
                if not data:
                    return output
                output += data
            except OSError:
                return output
        else:
            if output:
                break
    return output

def run_term(args, input_seq=b''):
    """Run kemacs in a pty, send input_seq, capture output."""
    pid, fd = pty.fork()
    if pid == 0:
        os.environ['TERM'] = 'xterm-256color'
        os.environ['COLUMNS'] = '120'
        os.environ['LINES'] = '40'
        os.execv('./kemacs', ['./kemacs'] + args)
    else:
        time.sleep(0.5)
        read_available(fd, 0.3)  # drain startup
        for item in input_seq:
            if isinstance(item, (int, float)) and not isinstance(item, (bytes, bytearray)):
                time.sleep(float(item))
                read_available(fd, 0.2)
                continue
            if isinstance(item, str):
                os.write(fd, item.encode())
            else:
                os.write(fd, item)
            time.sleep(0.15)
        read_available(fd, 0.5)
        # exit
        try:
            os.write(fd, b'\x18\x63')  # Ctrl-X Ctrl-C
        except OSError:
            pass
        time.sleep(0.8)
        try:
            out = read_available(fd, 0.3)
        except OSError:
            out = b''
        try:
            os.close(fd)
        except OSError:
            pass
        _, status = os.waitpid(pid, 0)
        return out

# Test: load file, display it, save it
# Heart emoji = U+2764 U+FE0F = UTF-8: e2 9d a4 ef b8 8f
input_seq = [
    b'x',         # M-x
    0.3,
    b'find-file',
    0.3,
    b'\r',
    0.4,
    b'/tmp/test_heart.txt\r',
    0.6,           # wait for file to load
    b'\x18s',       # C-x C-s save
    0.3,
    b'\r',         # confirm filename
    0.5,
]

print("=== Capturing terminal output ===")
output = run_term(['-w'], input_seq)

# Strip ANSI
text = re.sub(rb'\x1b\[[0-9;]*[mK]', b'', output)
text = re.sub(rb'\x1b\[[\(\)][AB012]', b'', text)
text = text.replace(b'\r', b'\n')

print(f"Raw output length: {len(output)} bytes")
print(f"Processed text length: {len(text)} bytes")

# Look for heart bytes
idx = output.find(b'\xe2\x9d\xa4')
if idx >= 0:
    chunk = output[idx:idx+30]
    hexstr = ' '.join(f'{b:02x}' for b in chunk)
    print(f"\nHeart bytes found at offset {idx}: {hexstr}")
else:
    # Look for garbled output (section sign + a-umlaut = §ä)
    idx2 = output.find(b'\xc2\xa7')
    if idx2 >= 0:
        chunk = output[idx2:idx2+20]
        hexstr = ' '.join(f'{b:02x}' for b in chunk)
        print(f"\nGARBLED: section sign found at {idx2}: {hexstr}")
    # Look for any UTF-8 in output
    utf8_seqs = []
    i = 0
    while i < len(output):
        b = output[i]
        if b >= 0x80:
            if b < 0xC0:
                i += 1
                continue
            elif b < 0xE0:
                seq = output[i:i+2]
                if len(seq) == 2:
                    utf8_seqs.append((i, seq.hex()))
                i += 2
            elif b < 0xF0:
                seq = output[i:i+3]
                if len(seq) == 3:
                    utf8_seqs.append((i, seq.hex()))
                i += 3
            elif b < 0xF8:
                seq = output[i:i+4]
                if len(seq) == 4:
                    utf8_seqs.append((i, seq.hex()))
                i += 4
            else:
                i += 1
        else:
            i += 1
    
    print(f"\nAll UTF-8 sequences found in output:")
    for off, hexstr in utf8_seqs[:30]:
        print(f"  offset {off}: {hexstr}")

# Check output file
print("\n=== Output file check ===")
if os.path.exists('/tmp/test_heart_output.txt'):
    with open('/tmp/test_heart_output.txt', 'rb') as f:
        out_data = f.read()
    print(f"  Output hex: {out_data.hex()}")
    if b'\xe2\x9d\xa4' in out_data:
        print("  PASS: heart emoji preserved")
    else:
        print("  FAIL: heart emoji garbled in output file")
else:
    print("  Output file not created (save may have failed)")
