#!/usr/bin/env python3
"""ws005-p019: types the text read from standard input into the passthrough guest (as h4-ctl.py keys does).

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

A secret (a WiFi key) given to the guest must not appear in a command line, a script's arguments or sudo's log,
so it comes on standard input only:  ssh HOST sudo -n python3 bigbang/h4/keys-stdin.py  with the text piped in.
Nothing of the text is printed or written anywhere.  A trailing newline of the input is not typed; "--enter"
presses Enter after the text.  Copied to ~/bigbang/h4/ by wifi-desktop-hw.sh, next to h4-ctl.py.
"""
import importlib.util
import os
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
# Characters h4-ctl.py does not know (US layout), for shell commands typed into a terminal.
MORE_PLAIN = {',': 'comma', "'": 'apostrophe', '`': 'grave_accent', '[': 'bracket_left', ']': 'bracket_right',
              '\\': 'backslash'}
MORE_SHIFTED = {'!': '1', '@': '2', '#': '3', '$': '4', '%': '5', '^': '6', '(': '9', ')': '0', '+': 'equal',
                '?': 'slash', '~': 'grave_accent', '{': 'bracket_left', '}': 'bracket_right'}


def main():
    spec = importlib.util.spec_from_file_location('h4ctl', os.path.join(DIR, 'h4-ctl.py'))
    ctl = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ctl)
    ctl.PLAIN.update(MORE_PLAIN)
    ctl.SHIFTED.update(MORE_SHIFTED)
    text = sys.stdin.read().rstrip('\n')
    if '--enter' in sys.argv[1:]:
        text += '\n'
    f = ctl.qmp_open()
    ctl.keys(f, text)
    text = None
    print('typed')


if __name__ == '__main__':
    main()
