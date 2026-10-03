#!/usr/bin/env python3
# ws134-p003: rasterizes a scene file of preview.c the way the monitor's shaders draw it (shaders/shape.frag and
# glyph.frag: the solid, plate, line, gradient, area and disc shapes, and the atlas's glyphs), over the clear colour,
# with straight-alpha blending, into a PNG.  For looking at a layout on the host; the guest's picture is the reference.
#   preview.py SCENE OUT.png
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
import struct
import sys

import numpy as np
from PIL import Image


def main():
    data = open(sys.argv[1], "rb").read()
    width, height, vertex_count, draw_count, atlas_width, atlas_height = struct.unpack_from("<6I", data, 0)
    offset = 24
    vertices = np.frombuffer(data, dtype="<f4", count=vertex_count * 16, offset=offset).reshape(vertex_count, 16)
    offset += vertex_count * 64
    draws = np.frombuffer(data, dtype="<u4", count=draw_count * 3, offset=offset).reshape(draw_count, 3)
    offset += draw_count * 12
    atlas = np.frombuffer(data, dtype="<u4", count=atlas_width * atlas_height, offset=offset).reshape(atlas_height, atlas_width)
    atlas_alpha = ((atlas >> 24) & 0xff).astype(np.float32) / 255.0
    image = np.zeros((height, width, 3), dtype=np.float32)
    image[:, :] = (0.063, 0.082, 0.118)
    for pipe, first, count in draws:
        for triangle in range(first, first + count, 3):
            draw_triangle(image, vertices[triangle:triangle + 3], pipe, atlas_alpha)
    Image.fromarray((np.clip(image, 0, 1) * 255 + 0.5).astype(np.uint8)).save(sys.argv[2])


def draw_triangle(image, v, pipe, atlas_alpha):
    height, width, _ = image.shape
    x = v[:, 0]
    y = v[:, 1]
    x0, x1 = int(max(np.floor(x.min()), 0)), int(min(np.ceil(x.max()), width - 1))
    y0, y1 = int(max(np.floor(y.min()), 0)), int(min(np.ceil(y.max()), height - 1))
    if x1 < x0 or y1 < y0:
        return
    px, py = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
    d = (y[1] - y[2]) * (x[0] - x[2]) + (x[2] - x[1]) * (y[0] - y[2])
    if abs(d) < 1e-9:
        return
    w0 = ((y[1] - y[2]) * (px - x[2]) + (x[2] - x[1]) * (py - y[2])) / d
    w1 = ((y[2] - y[0]) * (px - x[2]) + (x[0] - x[2]) * (py - y[2])) / d
    w2 = 1.0 - w0 - w1
    inside = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
    if not inside.any():
        return
    def lerp(column):
        return w0 * v[0, column] + w1 * v[1, column] + w2 * v[2, column]
    lx, ly = lerp(2), lerp(3)
    shape = v[0, 4:8]
    color = np.stack([lerp(8), lerp(9), lerp(10), lerp(11)], axis=-1)
    second = np.stack([lerp(12), lerp(13), lerp(14), lerp(15)], axis=-1)
    if pipe == 1:
        ah, aw = atlas_alpha.shape
        tx = np.clip((lx * aw).astype(int), 0, aw - 1)
        ty = np.clip((ly * ah).astype(int), 0, ah - 1)
        rgb = color[..., :3]
        alpha = color[..., 3] * atlas_alpha[ty, tx]
    else:
        rgb, alpha = shade(lx, ly, shape, color, second)
    alpha = np.where(inside, alpha, 0.0)
    region = image[y0:y1 + 1, x0:x1 + 1]
    region[:] = rgb * alpha[..., None] + region * (1.0 - alpha[..., None])


def shade(lx, ly, shape, color, second):
    kind = shape[3]
    hx, hy, radius = shape[0], shape[1], shape[2]
    if kind < 0.5:
        return color[..., :3], color[..., 3]
    if kind < 1.5:
        qx = np.abs(lx) - hx + radius
        qy = np.abs(ly) - hy + radius
        dist = np.sqrt(np.maximum(qx, 0) ** 2 + np.maximum(qy, 0) ** 2) + np.minimum(np.maximum(qx, qy), 0) - radius
        inside = np.clip(0.5 - dist, 0, 1)
        edge = np.clip(1.0 - np.abs(dist + 0.75), 0, 1)
        t = np.clip(0.5 - ly / (2.0 * hy), 0, 1)
        fill = color[..., :3] * (0.94 + 0.12 * t)[..., None]
        lit = np.clip(1.0 - np.abs(ly + hy - 1.5), 0, 1) * (np.abs(lx) <= hx - radius)
        fill = fill * (1 - (edge * second[..., 3])[..., None]) + second[..., :3] * (edge * second[..., 3])[..., None]
        fill = fill + 0.06 * lit[..., None]
        return fill, color[..., 3] * inside
    if kind < 2.5:
        cover = np.clip(hy - np.abs(ly), 0, 1)
        return color[..., :3], color[..., 3] * cover
    if kind < 4.5:
        t = np.clip(ly / (2.0 * hy) + 0.5, 0, 1)[..., None]
        mixed = color * (1 - t) + second * t
        return mixed[..., :3], mixed[..., 3]
    dist = np.sqrt(lx * lx + ly * ly) - hx
    return color[..., :3], color[..., 3] * np.clip(0.5 - dist, 0, 1)


main()
