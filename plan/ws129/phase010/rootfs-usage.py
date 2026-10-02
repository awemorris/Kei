#!/usr/bin/env python3
"""ws129-p010: reports how full the root partition of a zedBSD disk image is.

    rootfs-usage.py IMAGE [PARTITION_NAME]      (default zedBSD-root)

The GPT names the partition; its UFS superblock (at 64 KiB into it) gives the
fragments, the free blocks and fragments, and the inodes (cylinder groups times
inodes per group, and the free ones), the counters tools/build/check-ufs-image.py
checks.  Prints the partition's size and what is used and free of each.
Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""
import struct
import sys


def partition(image, wanted):
    """Returns (offset, size) in bytes of the GPT partition named wanted."""
    image.seek(512)
    header = image.read(92)
    if header[:8] != b"EFI PART":
        raise SystemExit("no GPT header")
    entries_lba, count, entry_size = struct.unpack_from("<QII", header, 72)
    image.seek(entries_lba * 512)
    table = image.read(count * entry_size)
    for index in range(count):
        entry = table[index * entry_size:(index + 1) * entry_size]
        first, last = struct.unpack_from("<QQ", entry, 32)
        name = entry[56:128].decode("utf-16-le").rstrip("\0")
        if name == wanted:
            return first * 512, (last - first + 1) * 512
    raise SystemExit(f"no partition named {wanted}")


def main():
    path = sys.argv[1]
    wanted = sys.argv[2] if len(sys.argv) > 2 else "zedBSD-root"
    with open(path, "rb") as image:
        offset, size = partition(image, wanted)
        image.seek(offset + 65536)
        sb = image.read(8192)
    if struct.unpack_from("<I", sb, 1372)[0] != 0x19540119:
        raise SystemExit("bad UFS magic")
    bsize, fsize, frag = struct.unpack_from("<III", sb, 48)
    ncg = struct.unpack_from("<I", sb, 44)[0]
    ipg = struct.unpack_from("<I", sb, 184)[0]
    ndir, nbfree, nifree, nffree = struct.unpack_from("<QQQQ", sb, 1008)
    fragments = struct.unpack_from("<Q", sb, 1080)[0]
    free_bytes = (nbfree * frag + nffree) * fsize
    total_bytes = fragments * fsize
    inodes = ncg * ipg
    print(f"partition {wanted}: {size / 1048576:.0f} MiB at {offset}")
    print(f"space: {total_bytes / 1048576:.1f} MiB, used {(total_bytes - free_bytes) / 1048576:.1f} MiB, "
          f"free {free_bytes / 1048576:.1f} MiB ({100.0 * free_bytes / total_bytes:.1f}%)")
    print(f"inodes: {inodes}, used {inodes - nifree}, free {nifree} ({100.0 * nifree / inodes:.1f}%), directories {ndir}")


if __name__ == "__main__":
    main()
