#!/usr/bin/env python3
"""ws005-p024: runs plan/tools/guest/guest.py with its forwarded ports taken from P1's range.

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

guest.py picks any free local port for the guest's SSH and debug stub; P1's runs use 10100-10199 so that they
never meet another agent's guest.  The arguments are guest.py's.
"""
import importlib.util
import socket
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def ranged_port():
	"""Returns a free port of 10100-10199 on the loopback."""
	for port in range(10100, 10200):
		with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
			try:
				probe.bind(("127.0.0.1", port))
			except OSError:
				continue
			if port not in ranged_port.given:
				ranged_port.given.add(port)
				return port
	raise SystemExit("guest-ports: no free port in 10100-10199")


ranged_port.given = set()


def main():
	spec = importlib.util.spec_from_file_location("guest", ROOT / "plan/tools/guest/guest.py")
	guest = importlib.util.module_from_spec(spec)
	sys.argv = [str(ROOT / "plan/tools/guest/guest.py")] + sys.argv[1:]
	spec.loader.exec_module(guest)
	guest.free_port = ranged_port
	raise SystemExit(guest.main())


if __name__ == "__main__":
	main()
