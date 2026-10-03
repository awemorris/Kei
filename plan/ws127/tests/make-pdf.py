#!/usr/bin/env python3
"""ws127-p002: writes a small valid one-page PDF (A6 portrait: a red band and a blue square on white) for the
PDF thumbnail check.  Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

    make-pdf.py OUT.pdf
"""
import sys


def main():
	content = b"1 0 0 rg 0 300 298 120 re f 0 0 1 rg 80 80 140 140 re f\n"
	objects = [
		b"<< /Type /Catalog /Pages 2 0 R >>",
		b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
		b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 298 420] /Contents 4 0 R /Resources << >> >>",
		b"<< /Length " + str(len(content)).encode() + b" >>\nstream\n" + content + b"endstream",
	]
	out = bytearray(b"%PDF-1.4\n")
	offsets = []
	for number, body in enumerate(objects, 1):
		offsets.append(len(out))
		out += str(number).encode() + b" 0 obj\n" + body + b"\nendobj\n"
	xref = len(out)
	out += b"xref\n0 " + str(len(objects) + 1).encode() + b"\n0000000000 65535 f \n"
	for offset in offsets:
		out += b"%010d 00000 n \n" % offset
	out += b"trailer\n<< /Size " + str(len(objects) + 1).encode() + b" /Root 1 0 R >>\nstartxref\n" + str(xref).encode() + b"\n%%EOF\n"
	with open(sys.argv[1], "wb") as file:
		file.write(out)


if __name__ == "__main__":
	main()
