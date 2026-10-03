#!/usr/bin/env python3
# ws128-p002: writes a small one-page A4 PDF made by "another program" (no Notes edit data): a grey frame and a
# blue bar, for Notes' Open (the page becomes the background Notes writes on).
#   python3 plan/ws128/tests/make-foreign-pdf.py OUT.pdf
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import sys

content = b"0.6 0.6 0.6 RG 4 w 40 40 515.28 761.89 re S 0.2 0.4 0.9 rg 60 700 300 40 re f\n"
objects = [
    b"<< /Type /Catalog /Pages 2 0 R >>",
    b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
    b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595.28 841.89] /Contents 4 0 R /Resources << >> >>",
    b"<< /Length %d >>\nstream\n" % len(content) + content + b"endstream",
]
out = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
offsets = []
for number, body in enumerate(objects, 1):
    offsets.append(len(out))
    out += b"%d 0 obj\n" % number + body + b"\nendobj\n"
xref = len(out)
out += b"xref\n0 %d\n0000000000 65535 f \n" % (len(objects) + 1)
for offset in offsets:
    out += b"%010d 00000 n \n" % offset
out += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(objects) + 1, xref)
with open(sys.argv[1], "wb") as handle:
    handle.write(out)
