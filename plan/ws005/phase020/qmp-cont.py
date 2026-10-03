#!/usr/bin/env python3
"""ws005-p020: resumes the guest with QMP "cont" and prints its status (never leaves a guest paused).

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""
import json
import socket
import sys


def main():
	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	sock.settimeout(10)
	sock.connect(sys.argv[1])
	stream = sock.makefile("rw")
	stream.readline()

	def command(name):
		stream.write(json.dumps({"execute": name}) + "\n")
		stream.flush()
		while True:
			reply = json.loads(stream.readline())
			if "return" in reply or "error" in reply:
				return reply

	command("qmp_capabilities")
	command("cont")
	status = command("query-status")
	print("qmp: status", status.get("return", {}).get("status"))


if __name__ == "__main__":
	main()
