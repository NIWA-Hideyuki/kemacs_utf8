import re

with open('Css.c', 'r') as f:
    content = f.read()

hex_values = re.findall(r'0x[0-9a-fA-F]+', content)
vals = [int(h, 16) for h in hex_values]

print(f"Total values: {len(vals)}")
print(f"First 10: {[hex(v) for v in vals[:10]]}")

# Search for 'm' (0x6d)
found = False
for i, v in enumerate(vals):
    if v == 0x6d:
        ctx = vals[i:i+16]
        chars = []
        for c in ctx:
            if c == 0:
                chars.append('\\0')
            elif c < 128:
                chars.append(chr(c))
            else:
                chars.append(hex(c))
        print(f"Index {i}: {' '.join(chars)}")
        s = ''.join(chr(c) for c in ctx[:15] if 0 < c < 128)
        print(f"  String context: {s!r}")
        found = True
        break

if not found:
    print("No plain 'm' found")
    # Try packed values (2 chars per int)
    for i, v in enumerate(vals):
        lo = v & 0xFFFF
        hi = (v >> 16) & 0xFFFF
        if lo == 0x6d6f or hi == 0x6d6f:
            chars = []
            for j in range(i, min(i+20, len(vals))):
                c = vals[j]
                lo2 = c & 0xFFFF
                hi2 = (c >> 16) & 0xFFFF
                if lo2 == 0 and hi2 == 0:
                    break
                if lo2:
                    chars.append(chr(lo2) if lo2 < 128 else hex(lo2))
                if hi2:
                    chars.append(chr(hi2) if hi2 < 128 else hex(hi2))
            s = ''.join(c for c in chars if not c.startswith('0x'))
            print(f"Packed 'mo' at index {i}: {s!r}")
            break
