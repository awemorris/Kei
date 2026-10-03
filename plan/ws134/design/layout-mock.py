#!/usr/bin/env python3
# ws134-p001: draws the layout sketch of the System Monitor (layout-mock.png) from design.md section 3 -- the plates,
# their layers and the colour tokens, not the final look (no 3D, no glow).  python3 layout-mock.py [OUT]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import math
import sys
from PIL import Image, ImageDraw, ImageFont

W, H = 1440, 900
BG0, BG1 = (16, 21, 30), (24, 31, 43)          # bg.deep, bg.mid
PLATE = {2: (38, 48, 63), 1: (31, 40, 54), 0: (27, 35, 47)}  # surface by layer (front lighter)
EDGE = {2: (92, 118, 146), 1: (64, 82, 104), 0: (48, 61, 78)}
TEXT, TEXT2 = (214, 224, 236), (138, 154, 176)
CYAN, MINT, ICE, AMBER = (92, 196, 230), (110, 214, 176), (170, 214, 240), (230, 168, 76)


def font(size, bold=False):
	for name in (("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"),):
		try:
			return ImageFont.truetype("/usr/share/fonts/truetype/dejavu/" + name, size)
		except OSError:
			pass
	return ImageFont.load_default()


def plate(draw, box, layer, title=None, note=None):
	x0, y0, x1, y1 = box
	shift = {2: 0, 1: 2, 0: 4}[layer]          # the back layers sit a little lower: depth by offset and tone
	draw.rounded_rectangle((x0 + shift, y0 + shift + 3, x1 + shift, y1 + shift + 3), 14, fill=(10, 13, 19))
	draw.rounded_rectangle(box, 14, fill=PLATE[layer], outline=EDGE[layer], width=1)
	if title:
		draw.text((x0 + 18, y0 + 14), title, font=font(17, True), fill=TEXT)
	if note:
		width = draw.textlength(note, font=font(13))
		draw.text((x1 - 18 - width, y0 + 17), note, font=font(13), fill=TEXT2)


def spark(draw, box, colour, phase):
	x0, y0, x1, y1 = box
	points = []
	for i in range(41):
		x = x0 + (x1 - x0) * i / 40
		y = (y0 + y1) / 2 + (y1 - y0) * 0.35 * math.sin(i * 0.45 + phase) * (0.6 + 0.4 * math.sin(i * 0.13))
		points.append((x, y))
	draw.line(points, fill=colour, width=2)


def main():
	out = sys.argv[1] if len(sys.argv) > 1 else "layout-mock.png"
	image = Image.new("RGB", (W, H), BG0)
	draw = ImageDraw.Draw(image)
	for y in range(H):
		t = y / H
		draw.line((0, y, W, y), fill=tuple(int(BG0[i] * (1 - t) + BG1[i] * t) for i in range(3)))
	# layer 0: the faint information layer (a grid far behind)
	for x in range(0, W, 48):
		draw.line((x, 70, x, H), fill=(22, 29, 40))
	# the Keiland titlebar (zdesktop draws it): name, status chip, time range
	draw.rounded_rectangle((24, 10, W - 24, 52), 12, fill=(30, 38, 51), outline=EDGE[1])
	draw.text((44, 20), "System Monitor", font=font(18, True), fill=TEXT)
	pass
	pass
	pass
	pass
	for i, label in enumerate(("1 min", "5 min", "15 min", "1 h")):
		x = 1040 + i * 86
		draw.rounded_rectangle((x, 17, x + 78, 45), 10, fill=(44, 62, 84) if i == 1 else (30, 38, 51), outline=EDGE[1])
		draw.text((x + 18, 22), label, font=font(14), fill=TEXT)
	# layer 2: the summary plates
	labels = (("CPU", "37%", "4 cores", CYAN), ("GPU", "21%", "Venus", ICE), ("Memory", "2.1 / 8 GiB", "Swap 0", CYAN),
		  ("Network", "4.2 Mb/s", "RX 3.9  TX 0.3", MINT), ("Disk", "12 MB/s", "R 9  W 3", AMBER))
	pw = (W - 48 - 4 * 16) / 5
	for i, (name, value, note, colour) in enumerate(labels):
		x0 = 24 + i * (pw + 16)
		plate(draw, (x0, 70, x0 + pw, 190), 2, name, note)
		draw.text((x0 + 18, 102), value, font=font(30, True), fill=TEXT)
		spark(draw, (x0 + 18, 150, x0 + pw - 18, 178), colour, i)
	# layer 1: the middle row -- the CPU tile relief, the state core, the GPU module cards
	plate(draw, (24, 210, 440, 590), 1, "CPU cores", "per-core load")
	for r in range(4):
		for c in range(6):
			x = 70 + c * 52 + r * 18
			y = 290 + r * 52 - (c % 3) * 4
			lit = (r * 6 + c) % 7 == 0
			draw.rounded_rectangle((x, y, x + 42, y + 34), 6, fill=(70, 150, 190) if lit else (44, 58, 78), outline=EDGE[1])
	draw.text((44, 548), "Overall 37%    Highest 78% (core 7)    Lowest 6%", font=font(14), fill=TEXT2)
	plate(draw, (456, 210, 984, 590), 2, "System state", "all nominal")
	cx, cy = 720, 410
	for k, (rw, rh) in enumerate(((240, 70), (200, 56))):
		draw.ellipse((cx - rw, cy + 70 - rh + k * 18, cx + rw, cy + 70 + rh + k * 18), outline=(60, 120, 150), width=2)
	for k, size in enumerate((120, 84, 50)):
		shade = 60 + 40 * k
		draw.rounded_rectangle((cx - size, cy - size, cx + size, cy + size - 20), 14 - 3 * k,
				       fill=(30 + k * 10, 70 + k * 25, 100 + k * 30), outline=(shade + 60, shade + 120, 230))
	draw.text((cx - 44, 540), "NORMAL", font=font(20, True), fill=TEXT)
	plate(draw, (1000, 210, W - 24, 590), 1, "Graphics", "1 device")
	for j, name in enumerate(("GPU 0  Venus (virtio)",)):
		y0 = 256 + j * 162
		plate(draw, (1018, y0, W - 42, y0 + 146), 2 if j == 0 else 0, name, None)
		if j == 0:
			draw.arc((1036, y0 + 46, 1116, y0 + 126), 140, 140 + 0.62 * 260, fill=CYAN, width=8)
			draw.text((1058, y0 + 76), "62%", font=font(16, True), fill=TEXT)
			for k, (lab, frac) in enumerate((("Memory", 0.71), ("Temp", 0.45), ("Power", 0.3))):
				yy = y0 + 54 + k * 28
				draw.text((1140, yy - 2), lab, font=font(13), fill=TEXT2)
				draw.rounded_rectangle((1200, yy + 4, 1370, yy + 10), 3, fill=(44, 58, 78))
				draw.rounded_rectangle((1200, yy + 4, 1200 + 170 * frac, yy + 10), 3, fill=CYAN if k == 0 else MINT)
	# layer 1: the bottom row -- the network flow, the memory strata, the disk lanes
	plate(draw, (24, 606, 560, 830), 1, "Network", "en0 · autoscale 10 Mb/s")
	for k, colour in enumerate((CYAN, MINT)):
		spark(draw, (44, 660 + k * 40, 540, 720 + k * 40), colour, 1.5 * k)
	plate(draw, (576, 606, 864, 830), 1, "Memory", "8 GiB")
	for k, (name, frac, colour) in enumerate((("Used", 0.27, CYAN), ("Cache", 0.31, ICE), ("Available", 0.42, (60, 80, 104)), ("Swap", 0.0, AMBER))):
		y = 650 + k * 42
		draw.rounded_rectangle((600, y, 840, y + 32), 6, fill=(36, 46, 61), outline=EDGE[1])
		draw.rounded_rectangle((600, y, 600 + 240 * max(frac, 0.02), y + 32), 6, fill=colour)
		draw.text((610, y + 7), name, font=font(13, True), fill=(16, 21, 30) if frac > 0.2 else TEXT)
	plate(draw, (880, 606, W - 24, 830), 1, "Disk", "nvme0 · usb0")
	for k, (name, colour) in enumerate((("Read", CYAN), ("Write", AMBER))):
		draw.text((900, 662 + k * 56), name, font=font(13), fill=TEXT2)
		spark(draw, (960, 650 + k * 56, 1250, 690 + k * 56), colour, 0.8 + k)
	plate(draw, (1266, 646, W - 40, 816), 2, "Latency")
	draw.text((1284, 700), "0.8 ms", font=font(24, True), fill=TEXT)
	# layer 1: the events ticker
	plate(draw, (24, 846, W - 24, 890), 0)
	draw.text((44, 858), "Events   14:32  Memory pressure eased    14:35  Disk latency normal    14:36  Network rose to 4 Mb/s",
		  font=font(14), fill=TEXT2)
	image.save(out)


main()
