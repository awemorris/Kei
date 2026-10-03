#!/usr/bin/env python3
"""ws005-p020: types the text read from standard input into a local QEMU guest through QMP (US layout).

    qmp-type-stdin.py SOCKET [--enter] < file

A secret (a Wi-Fi key) must not appear in a command line or a log, so it comes on standard input only; nothing of
it is printed.  A trailing newline of the input is not typed; "--enter" presses Enter after the text.
Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""
import importlib.util
import json
import socket
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def main():
	spec = importlib.util.spec_from_file_location("qmpkeys", ROOT / "plan/ws035/tests/qmp-keys.py")
	keys = importlib.util.module_from_spec(spec)
	spec.loader.exec_module(keys)
	text = sys.stdin.read().rstrip("\n")
	if "--enter" in sys.argv[2:]:
		text += "\n"
	connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	connection.connect(sys.argv[1])
	stream = connection.makefile("rw")
	stream.readline()

	def call(command, arguments=None):
		request = {"execute": command}
		if arguments is not None:
			request["arguments"] = arguments
		stream.write(json.dumps(request) + "\n")
		stream.flush()
		while True:
			reply = json.loads(stream.readline())
			if "return" in reply or "error" in reply:
				return reply

	def key(code, down):
		call("input-send-event", {"events": [{"type": "key", "data": {"down": down, "key": {"type": "qcode", "data": code}}}]})
		time.sleep(0.03)

	call("qmp_capabilities")
	for character in text:
		code, shifted = keys.key_of(character)
		if shifted:
			key("shift", True)
		key(code, True)
		key(code, False)
		if shifted:
			key("shift", False)
	text = None
	print("typed")


if __name__ == "__main__":
	main()
