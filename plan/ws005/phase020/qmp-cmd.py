#!/usr/bin/env python3
"""ws005-p020: sends one QMP command:  qmp-cmd.py QMP_SOCKET COMMAND [JSON_ARGUMENTS].

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""
import json, socket, sys
s = socket.socket(socket.AF_UNIX); s.connect(sys.argv[1]); f = s.makefile("rw"); f.readline()
def c(cmd, args=None):
	m = {"execute": cmd}
	if args: m["arguments"] = args
	f.write(json.dumps(m) + "\n"); f.flush()
	while True:
		r = json.loads(f.readline())
		if "return" in r or "error" in r: return r
c("qmp_capabilities"); print(c(sys.argv[2], json.loads(sys.argv[3]) if len(sys.argv) > 3 else None))
