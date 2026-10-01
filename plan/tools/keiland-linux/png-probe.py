#!/usr/bin/env python3
# zedBSD
# Copyright (C) 2026 Awe Morris
# SPDX-License-Identifier: Zlib
"""Reads non-interlaced 8-bit RGB/RGBA PNGs without third-party libraries."""
import struct
import sys
import zlib
from pathlib import Path


def read_png(path):
	data = Path(path).read_bytes()
	if data[:8] != b'\x89PNG\r\n\x1a\n':
		raise ValueError('not a PNG')
	position = 8
	compressed = bytearray()
	while position < len(data):
		length = struct.unpack_from('>I', data, position)[0]
		kind = data[position + 4:position + 8]
		body = data[position + 8:position + 8 + length]
		if kind == b'IHDR':
			width, height, depth, colour, method, filtering, interlace = struct.unpack('>IIBBBBB', body)
			if depth != 8 or colour not in (2, 6) or method or filtering or interlace:
				raise ValueError('only non-interlaced 8-bit RGB/RGBA is supported')
		elif kind == b'IDAT':
			compressed.extend(body)
		elif kind == b'IEND':
			break
		position += length + 12
	channels = 3 if colour == 2 else 4
	stride = width * channels
	raw = zlib.decompress(compressed)
	if len(raw) != height * (stride + 1):
		raise ValueError('invalid PNG scanline length')
	rows = []
	previous = bytearray(stride)
	for y in range(height):
		filter_kind = raw[y * (stride + 1)]
		row = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
		for x in range(stride):
			left = row[x - channels] if x >= channels else 0
			above = previous[x]
			corner = previous[x - channels] if x >= channels else 0
			if filter_kind == 0:
				predictor = 0
			elif filter_kind == 1:
				predictor = left
			elif filter_kind == 2:
				predictor = above
			elif filter_kind == 3:
				predictor = (left + above) // 2
			elif filter_kind == 4:
				p = left + above - corner
				distances = (abs(p - left), abs(p - above), abs(p - corner))
				predictor = (left, above, corner)[distances.index(min(distances))]
			else:
				raise ValueError('unknown PNG filter')
			row[x] = (row[x] + predictor) & 255
		rows.append(row)
		previous = row
	return width, height, channels, rows


def main():
	if len(sys.argv) == 3 and sys.argv[1] == '--size':
		width, height, _, _ = read_png(sys.argv[2])
		print(width, height)
		return
	if len(sys.argv) < 4 or len(sys.argv) % 2:
		raise ValueError('usage: png-probe.py PNG X Y [X Y ...] | --size PNG')
	width, height, channels, rows = read_png(sys.argv[1])
	for index in range(2, len(sys.argv), 2):
		x, y = int(sys.argv[index]), int(sys.argv[index + 1])
		if not (0 <= x < width and 0 <= y < height):
			raise ValueError('pixel outside image')
		colour = rows[y][x * channels:x * channels + 3]
		print(x, y, '#' + colour.hex())


if __name__ == '__main__':
	main()
