#!/bin/sh
# BUG-095: a oneshot service that asks init to cut the power (as the capture image's poweroff service does):
# a while after the boot, so the test sees the guest up first.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
sleep 30
sync
exec /sbin/poweroff
