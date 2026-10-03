#!/usr/bin/env python3
# Runs a program under a pty with a given size and TERM, records every byte it writes,
# then sends keys to end it.  capture.py OUT COLS ROWS DELAY KEYS -- argv...
import os, pty, sys, time, select, struct, fcntl, termios, signal
out, cols, rows, delay, keys = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), float(sys.argv[4]), sys.argv[5]
argv = sys.argv[sys.argv.index("--") + 1:]
setsize_before = os.environ.get("CAPTURE_SIZE_AFTER") is None
pid, fd = pty.fork()
if pid == 0:
    if setsize_before:
        fcntl.ioctl(0, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
    else:
        time.sleep(0)
    os.execvp(argv[0], argv)
if not setsize_before:
    time.sleep(float(os.environ["CAPTURE_SIZE_AFTER"]))
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
data = bytearray()
def pump(t):
    end = time.time() + t
    while True:
        r = end - time.time()
        if r <= 0: return True
        a, _, _ = select.select([fd], [], [], r)
        if a:
            try: b = os.read(fd, 65536)
            except OSError: return False
            if not b: return False
            data.extend(b)
pump(delay)
mark = len(data)
os.write(fd, keys.encode().decode("unicode_escape").encode("latin-1"))
pump(3)
try: os.kill(pid, signal.SIGKILL)
except ProcessLookupError: pass
open(out, "wb").write(bytes(data))
open(out + ".mark", "w").write("%d\n" % mark)
print("bytes", len(data), "before keys", mark)
