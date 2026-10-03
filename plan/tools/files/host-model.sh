#!/bin/sh
# ws093-p003: builds files' host tests (host-build.sh) and runs files-model (host-model.c) in a temporary folder.
#
#   sh plan/tools/files/host-model.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
sh plan/tools/files/host-build.sh || exit 1
temporary=$(mktemp -d)
# The thumbnails kept on disk go to the temporary folder, not the user's cache (ws127-p002).
XDG_CACHE_HOME=$temporary/cache timeout 300 build/ws071-host/files-model "$temporary"
status=$?
rm -rf "$temporary"
exit $status
