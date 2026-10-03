# The number of parallel jobs of the image and test builds, in one place (ws129-p011, 2026-10-03 user: full builds
# use -j16 so that several can run at once without the host's I/O falling behind).
# Sourced, from the repository's root, by the scripts that run make themselves; ZEDBSD_JOBS in the environment
# overrides it for one run.  The toolchain's and the packages' own parallelism (build-jobs.mk, toolchain/,
# userland/packages/) is not set here.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
ZEDBSD_JOBS=${ZEDBSD_JOBS:-16}
