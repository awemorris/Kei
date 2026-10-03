# The host-visible memory (hostmem) the Venus test guests' virtio-gpu offers, in one place (q647, BUG-144/BUG-124).
# Sourced by the Venus guest scripts; VENUS_HOSTMEM in the environment overrides it for one run.
#
# 256M is the most the kernel takes today: src/drivers/gpu/venus/transport.c maps the whole host-visible BAR and refuses
# one larger than VENUS_MAX_APERTURE_BYTES (256 MiB, src/drivers/gpu/venus/internal.h), and every kernel device
# mapping shares the amd64 HAL's device window of 512 MiB (AMD64_DEVICE_PD_COUNT, src/hal/amd64/space.c).  A larger
# value makes Venus refuse its aperture (BUG-124's hostmem=1G run did not start a session).  See plan/ws014/phase011.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
VENUS_HOSTMEM=${VENUS_HOSTMEM:-256M}
