#!/bin/sh
# ws005-p020: traces guest kernel functions through QEMU's gdbstub for SECONDS, then always resumes the guest
# with QMP "cont" (a guest is never left paused).  2026-10-03 user approval of the gdbstub trace and QMP cont.
#   GUEST_RUNTIME=... TRACE_FUNCTIONS=f1,f2:dev sh plan/ws005/phase020/trace.sh SECONDS VMUNIX
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
runtime=${GUEST_RUNTIME:?}
seconds=${1:?seconds}
symbols=${2:?vmunix}
port=$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["debug_port"])' "$runtime/session.json")
timeout -k 20 -s INT "$seconds" gdb -q -batch -nx -ex "set pagination off" -ex "set confirm off" \
    -ex "file $symbols" -ex "target remote 127.0.0.1:$port" \
    -ex "source plan/ws005/phase020/trace.py" -ex "detach" 2>&1 \
    | grep -v '^\[\|^Continuing\|^Program received\|^0x'
sleep 1
python3 plan/ws005/phase020/qmp-cont.py "$runtime/qmp.sock"
