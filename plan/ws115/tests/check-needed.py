#!/usr/bin/env python3
# ws115-p002: checks that every DT_NEEDED of the ELF files given resolves to a file in the root tree's /lib or
# /usr/lib, the places zedBSD's loader searches (src/rtld/rtld.c), and that each resolved library's own NEEDED
# resolves too.  Prints one line per unresolved name and a summary; exits 0 only when everything resolves.
#
#   check-needed.py ROOTFS READELF FILE-IN-ROOTFS...
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import os
import re
import subprocess
import sys


def needed(readelf, path):
    """Returns the DT_NEEDED names of one ELF file."""
    output = subprocess.run([readelf, '-d', path], capture_output=True, text=True, check=True).stdout
    return re.findall(r'\(NEEDED\)\s+Shared library: \[([^\]]+)\]', output)


def main():
    rootfs, readelf, files = sys.argv[1], sys.argv[2], sys.argv[3:]
    search = [os.path.join(rootfs, 'lib'), os.path.join(rootfs, 'usr/lib')]
    pending = [os.path.join(rootfs, name.lstrip('/')) for name in files]
    seen = set()
    missing = 0
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        for name in needed(readelf, path):
            found = [os.path.join(d, name) for d in search if os.path.exists(os.path.join(d, name))]
            if not found:
                print('check-needed: %s needs %s: not in /lib or /usr/lib' % (path[len(rootfs):], name))
                missing += 1
                continue
            pending.append(os.path.realpath(found[0]))
    print('check-needed: %d files examined, %d unresolved' % (len(seen), missing))
    return 0 if missing == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
