import socket, struct, os, sys, array
def pad(b, n): return b + b'\0' * ((-len(b)) % n)
def s_(v, b):  # string/object path: align 4, u32 len, bytes, NUL
    b = pad(b, 4); e = v.encode(); return b + struct.pack('<I', len(e)) + e + b'\0'
def g_(v, b):  # signature: u8 len, bytes, NUL
    e = v.encode(); return b + bytes([len(e)]) + e + b'\0'
def msg(mtype, serial, fields, body=b'', flags=0):
    # fixed part (12 bytes) then a(yv)
    arr = b''
    base = 16  # offset of first array element (after 12 + 4 length)
    for code, sig, val in fields:
        cur = pad(b'\0' * base + arr, 8)[base:]   # struct aligns to 8 relative to message start
        arr = cur + bytes([code]) + g_(sig, b'')[0:0] + bytes([len(sig)]) + sig.encode() + b'\0'
        full = b'\0' * base + arr
        if sig in ('s', 'o'): full = s_(val, full)
        elif sig == 'g': full = g_(val, full)
        elif sig == 'u': full = pad(full, 4) + struct.pack('<I', val)
        arr = full[base:]
    hdr = b'l' + bytes([mtype, flags, 1]) + struct.pack('<III', len(body), serial, len(arr)) + arr
    return pad(hdr, 8) + body
def take_device_example():
    body = struct.pack('<II', 226, 0)
    return msg(1, 5, [(1, 'o', '/org/freedesktop/login1/session/_32'), (6, 's', 'org.freedesktop.login1'),
                      (2, 's', 'org.freedesktop.login1.Session'), (3, 's', 'TakeDevice'), (8, 'g', 'uu')], body)
if sys.argv[1] == 'dump':
    m = take_device_example()
    for i in range(0, len(m), 16):
        chunk = m[i:i+16]
        print('%04x  %-48s %s' % (i, ' '.join('%02x' % c for c in chunk), ''.join(chr(c) if 32 <= c < 127 else '.' for c in chunk)))
    print('total', len(m))
    sys.exit(0)
c = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM); c.connect('/run/dbus/system_bus_socket')
c.sendall(b'\0' + b'AUTH EXTERNAL ' + str(os.getuid()).encode().hex().encode() + b'\r\n')
print('auth:', c.recv(256))
c.sendall(b'NEGOTIATE_UNIX_FD\r\n'); print('fd:', c.recv(256))
c.sendall(b'BEGIN\r\n')
c.sendall(msg(1, 1, [(1, 'o', '/org/freedesktop/DBus'), (6, 's', 'org.freedesktop.DBus'), (2, 's', 'org.freedesktop.DBus'), (3, 's', 'Hello')]))
body = struct.pack('<I', os.getpid())
c.sendall(msg(1, 2, [(1, 'o', '/org/freedesktop/login1'), (6, 's', 'org.freedesktop.login1'), (2, 's', 'org.freedesktop.login1.Manager'), (3, 's', 'GetSessionByPID'), (8, 'g', 'u')], body))
import time; time.sleep(0.5)
data = c.recv(65536)
print(data)
