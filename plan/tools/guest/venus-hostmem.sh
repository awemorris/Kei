# The host-visible memory (hostmem) the Venus test guests' virtio-gpu offers, in one place (q647, BUG-144/BUG-124).
# Sourced by the Venus guest scripts; VENUS_HOSTMEM in the environment overrides it for one run.
#
# 1G is the most the kernel takes: src/drivers/gpu/venus/transport.c maps the whole host-visible BAR and refuses
# one larger than VENUS_MAX_APERTURE_BYTES (1 GiB, src/drivers/gpu/venus/internal.h), and every kernel device
# mapping shares the amd64 HAL's device window of 2 GiB (AMD64_DEVICE_PD_COUNT, src/hal/amd64/space.c).  Until
# ws014-p011 (option 1) the limits were 256 MiB and 512 MiB, and hostmem=1G did not start a session (BUG-124).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
VENUS_HOSTMEM=${VENUS_HOSTMEM:-1G}
