#!/usr/bin/env python3
"""Reproduce the kemacs gutter overflow issue using a PTY with proper screen tracking."""
import os, pty, sys, time, select, termios, struct, fcntl, signal, re

class TerminalScreen:
    def __init__(self, rows=24, cols=80):
        self.rows = rows
        self.cols = cols
        self.screen = [[' '] * cols for _ in range(rows)]
        self.cursor_r = 0
        self.cursor_c = 0
        self.buf = b''

    def process(self, data):
        self.buf += data
        while True:
            esc_idx = self.buf.find(b'\x1b')
            if esc_idx < 0:
                if self.buf:
                    self._print_plain(self.buf.decode('utf-8', errors='replace'))
                    self.buf = b''
                break
            if esc_idx > 0:
                plain = self.buf[:esc_idx].decode('utf-8', errors='replace')
                self._print_plain(plain)
            self.buf = self.buf[esc_idx:]
            if len(self.buf) < 2:
                break
            c1 = self.buf[1]
            if c1 == b'['[0]:
                i = 2
                while i < len(self.buf) and (self.buf[i] < 0x40 or self.buf[i] > 0x7e):
                    i += 1
                if i < len(self.buf):
                    params = self.buf[2:i]
                    final = self.buf[i]
                    self._handle_csi(params, final)
                    self.buf = self.buf[i+1:]
                else:
                    break
            elif c1 == b']'[0]:
                i = 2
                while i < len(self.buf):
                    if self.buf[i] == 0x07:
                        break
                    if i + 1 < len(self.buf) and self.buf[i] == 0x1b and self.buf[i+1] == b'\\'[0]:
                        i += 1
                        break
                    i += 1
                if i < len(self.buf):
                    self.buf = self.buf[i+1:]
                else:
                    break
            elif c1 in (b'=', b'<', b'>'):
                self.buf = self.buf[2:]
            elif c1 in (b'h', b'l'):
                self.buf = self.buf[2:]
            else:
                self.buf = self.buf[1:]

    def _handle_csi(self, params, final):
        param_str = params.decode('ascii', errors='replace')
        nums = []
        for p in param_str.split(';'):
            if p == '' or p == '?':
                continue
            try:
                nums.append(int(p))
            except:
                nums.append(0)
        if not nums:
            nums = [0]
        if final == b'H'[0]:
            r = nums[0] if nums[0] > 0 else 1
            c = nums[1] if len(nums) > 1 and nums[1] > 0 else 1
            self.cursor_r = min(r - 1, self.rows - 1)
            self.cursor_c = min(c - 1, self.cols - 1)
        elif final == b'f'[0]:
            self._handle_csi(params, b'H'[0])
        elif final == b'J'[0]:
            mode = nums[0] if nums else 0
            if mode == 2:
                self.screen = [[' '] * self.cols for _ in range(self.rows)]
            elif mode == 0:
                for r in range(self.cursor_r, self.rows):
                    start = self.cursor_c if r == self.cursor_r else 0
                    for c in range(start, self.cols):
                        self.screen[r][c] = ' '
        elif final == b'K'[0]:
            mode = nums[0] if nums else 0
            if mode in (0, 2):
                for c in range(self.cursor_c if mode==0 else 0, self.cols):
                    self.screen[self.cursor_r][c] = ' '
        elif final == b'A'[0]:
            r = nums[0] if nums else 1
            self.cursor_r = max(0, self.cursor_r - r)
        elif final == b'B'[0]:
            r = nums[0] if nums else 1
            self.cursor_r = min(self.rows - 1, self.cursor_r + r)
        elif final == b'C'[0]:
            c = nums[0] if nums else 1
            self.cursor_c = min(self.cols - 1, self.cursor_c + c)
        elif final == b'D'[0]:
            c = nums[0] if nums else 1
            self.cursor_c = max(0, self.cursor_c - c)

    def _print_plain(self, text):
        for c in text:
            if c == '\n':
                self.cursor_r += 1
                if self.cursor_r >= self.rows:
                    self.cursor_r = self.rows - 1
                self.cursor_c = 0
            elif c == '\r':
                self.cursor_c = 0
            else:
                if 0 <= self.cursor_r < self.rows and 0 <= self.cursor_c < self.cols:
                    self.screen[self.cursor_r][self.cursor_c] = c
                self.cursor_c += 1
                if self.cursor_c >= self.cols:
                    self.cursor_c = 0
                    self.cursor_r += 1
                    if self.cursor_r >= self.rows:
                        self.cursor_r = self.rows - 1

    def dump(self, title=""):
        if title:
            print(f"=== {title} ===")
        for r in range(self.rows):
            line = ''.join(self.screen[r])
            print(f"  {r:3d}|{line[:self.cols]}|")
        print(f"  cursor: ({self.cursor_r}, {self.cursor_c})")

def read_available(fd, timeout=2.0):
    data = b''
    end_time = time.time() + timeout
    while time.time() < end_time:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                chunk = os.read(fd, 65536)
                if not chunk:
                    break
                data += chunk
                end_time = time.time() + 0.5
            except:
                break
        else:
            if data:
                break
    return data

def main():
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
        if slave > 2:
            os.close(slave)
        env = dict(os.environ)
        env['TERM'] = 'xterm'
        env['HOME'] = '/tmp'
        os.execvpe('./kemacs', ['./kemacs',
                                '-xtoggle-line-numbers',
                                '/tmp/longtest.txt',
                                '-x140 forward-character'], env)
        os._exit(1)

    os.close(slave)
    screen = TerminalScreen(24, 80)

    data = read_available(master, 3.0)
    screen.process(data)
    screen.dump("After toggle-line-numbers + 140 forward-character")

    # C-v (page down)
    os.write(master, b'\x16')
    time.sleep(0.5)
    data = read_available(master, 2.0)
    screen.process(data)
    screen.dump("After C-v (page down)")

    # C-z (page up)
    os.write(master, b'\x1a')
    time.sleep(0.5)
    data = read_available(master, 2.0)
    screen.process(data)
    screen.dump("After C-z (page up)")

    # C-v again
    os.write(master, b'\x16')
    time.sleep(0.5)
    data = read_available(master, 2.0)
    screen.process(data)
    screen.dump("After C-v again")

    os.write(master, b'\x18\x03')  # C-x C-c
    time.sleep(0.5)
    os.kill(pid, signal.SIGKILL)
    os.waitpid(pid, 0)
    os.close(master)

if __name__ == '__main__':
    main()
