#!/bin/sh
# ws099-p028: zdesktop's log keeps no NUL bytes when the file is truncated while zdesktop still writes (as a test's
# next run, "> /tmp/zdesktop.log", does while the old zdesktop ends), on the Venus guest of the Settings image.
#  1. zdesktop started with "> /tmp/lognul.log"; once READY the file is truncated (": > /tmp/lognul.log"), and zdesktop
#     is then stopped (it writes its exit lines: ZWL EXIT).
#  2. The file's size and its size without NUL bytes (tr -d '\000' | wc -c) are equal, and the ZWL EXIT line is in it.
# Before the fix the exit lines landed at zdesktop's old offset after a hole of NUL bytes.
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up)
#   plan/ws099/tests/log-nul-guest.sh [OUTDIR]   (default build/ws099-shots/log-nul)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws099-shots/log-nul}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
status=0
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'

# 1. zdesktop, the log truncated under it, then its end.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/lognul.log; /bin/wayland --timeout=300 --width=1280 --height=800 --glass > /tmp/lognul.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do grep -aq ZWL.READY /tmp/lognul.log && break; sleep 0.5; done; echo started' >/dev/null
guest ': > /tmp/lognul.log; echo truncated' >/dev/null
guest "$stop_all" >/dev/null
sleep 1

# 2. No NUL byte, and the exit line kept.
sizes=$(guest 'a=$(wc -c < /tmp/lognul.log); b=$(tr -d "\000" < /tmp/lognul.log | wc -c); echo "sizes $a $b"' | tail -1)
echo "$sizes"
set -- $sizes x 0 1
[ "$2" = "$3" ] && echo "no NUL byte: ok" || { echo "NUL bytes: $(( ${2:-0} - ${3:-0} )) FAIL"; status=1; }
exits=$(guest 'grep -ac "ZWL EXIT" /tmp/lognul.log' | tail -1)
[ "${exits:-0}" -ge 1 ] 2>/dev/null && echo "the exit line kept: ok" || { echo "the exit line kept: FAIL"; status=1; }
guest 'tr -d "\000" < /tmp/lognul.log | tail -5' > "$out/tail.txt"
[ $status = 0 ] && echo "log-nul-guest: PASS" || echo "log-nul-guest: FAIL"
exit $status
