#!/usr/bin/env python3
"""Test kemacs mouse event handling with burst input."""
import pty
import os
import sys
import time
import select
import signal
import struct
import fcntl
import termios

def run_test():
    master, slave = pty.openpty()
    winsize = struct.pack('HHHH', 24, 80, 0, 0)
    fcntl.ioctl(slave, termios.TIOCSWINSZ, winsize)
    
    pid = os.fork()
    if pid == 0:
        os.close(master)
        os.setsid()
        os.dup2(slave, 0)
        os.dup2(slave, 1)
        os.dup2(slave, 2)
        if os.isatty(slave):
            os.close(slave)
        os.execvp('./kemacs', ['./kemacs'])
        os._exit(1)
    
    os.close(slave)
    time.sleep(2)
    
    # Read initial output
    output = b''
    while True:
        r, _, _ = select.select([master], [], [], 1.0)
        if r:
            try:
                data = os.read(master, 4096)
                if not data:
                    break
                output += data
            except OSError:
                break
        else:
            break
    
    has_1000 = b'\x1b[?1000h' in output
    has_1006 = b'\x1b[?1006h' in output
    print(f"Mouse 1000h at startup: {has_1000}")
    print(f"Mouse 1006h at startup: {has_1006}")
    
    # Send mouse event as a BURST (all bytes at once, like a real terminal)
    mouse_event = b'\x1b[<0;5;10M'  # SGR left click at row 10, col 5
    os.write(master, mouse_event)
    time.sleep(1)
    
    # Read output
    mouse_output = b''
    while True:
        r, _, _ = select.select([master], [], [], 1.0)
        if r:
            try:
                data = os.read(master, 4096)
                if not data:
                    break
                mouse_output += data
            except OSError:
                break
        else:
            break
    
    print(f"\nOutput after burst mouse event ({len(mouse_output)} bytes):")
    # Check for signs of mouse processing
    # If mouse was handled, we should see:
    # - The mouse cursor '<' at position (10, 5) on screen
    # - A cursor position update
    # If mouse was NOT handled, we'd see "undefined key" or chars at (1,2)...
    
    # Check for cursor position at row 10
    if b'\x1b[10;5H' in mouse_output:
        print("FOUND cursor move to (10,5)! Mouse parsing WORKS!")
    elif b'\x1b[10;' in mouse_output:
        print("Cursor moved to row 10 (col varies). Mouse parsing WORKS!")
    else:
        # Check if we see the raw escape chars being inserted (meaning mouse NOT parsed)
        raw_chars = mouse_output.count(b'0;5;10M')
        print(f"Raw escape chars in output (mouse NOT parsed): {raw_chars > 0}")
        print(f"Output (repr): {mouse_output[:400]!r}")
    
    # Now test with individual bytes with small delay (5ms < 10ms timeout)
    print("\n--- Test 2: Individual bytes with 5ms delay ---")
    for i, byte in enumerate(mouse_event):
        os.write(master, bytes([byte]))
        if i < len(mouse_event) - 1:
            time.sleep(0.005)  # 5ms delay
    time.sleep(1)
    
    mouse_output2 = b''
    while True:
        r, _, _ = select.select([master], [], [], 1.0)
        if r:
            try:
                data = os.read(master, 4096)
                if not data:
                    break
                mouse_output2 += data
            except OSError:
                break
        else:
            break
    
    if b'\x1b[10;' in mouse_output2:
        print("Cursor moved to row 10. Mouse parsing WORKS with 5ms delay!")
    else:
        print(f"Mouse NOT parsed with 5ms delay")
        print(f"Output (repr): {mouse_output2[:400]!r}")
    
    # Cleanup
    try:
        os.kill(pid, signal.SIGTERM)
        os.waitpid(pid, 0)
    except:
        pass
    os.close(master)

if __name__ == '__main__':
    run_test()
