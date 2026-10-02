# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Run the real favorite writer only within an exclusively owned configuration directory."""
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix='ws109-favorites-', dir='/tmp') as root:
    subprocess.run([sys.argv[1], root], check=True, timeout=10)
print('PASS owned Favorites configuration removed', flush=True)
