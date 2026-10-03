#!/bin/sh
# BUG-027 (ws046-p015): the ticket's measurement of the file-backed page fault, on a guest started afresh from the
# image of build-bug027-image.sh: ffault on the first 32 MiB of libLLVM.so.23.1 twice (the first right after the boot,
# cold; the second with the file's pages in memory), then kbench on the same file.  The ticket measured 1.8 ms a page
# cold and 0.34 ms warm (read() 0.05 ms).  PASS when the read fault is at most COLD_US a page cold (default 180,
# a tenth of the ticket's) and WARM_US warm (default 34), and the second pass is not slower than read() by more than
# RATIO times (default 10; the ticket's was 7 to 36).  The numbers go to OUTDIR/ffault-1.txt, ffault-2.txt, kbench.txt.
#
#   plan/ws046/tests/bug027/bug027-test.sh IMAGE [OUTDIR]
# Prints "bug027: PASS" or "bug027: FAIL".
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../../.."
image=${1:?usage: bug027-test.sh IMAGE [OUTDIR]}
out=${2:-build/ws046-bug027}
mkdir -p "$out"
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws046-bug027-run}"
export GUEST_RUNTIME
guest() { timeout 300 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
python3 plan/tools/guest/guest.py start "$image" >/dev/null || { echo "bug027: FAIL (the guest did not start)"; exit 1; }
timeout 260 python3 plan/tools/guest/guest.py wait --timeout 240 >/dev/null || { echo "bug027: FAIL (no SSH)"; python3 plan/tools/guest/guest.py stop >/dev/null 2>&1; exit 1; }
file=/var/bug027/libLLVM.so.23.1
guest "/bin/ffault $file 32" | grep FFAULT | tee "$out/ffault-1.txt"
guest "/bin/ffault $file 32" | grep FFAULT | tee "$out/ffault-2.txt"
guest "/bin/kbench $file" | tee "$out/kbench.txt"
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1

# Judges the numbers.
python3 - "$out" "${COLD_US:-180}" "${WARM_US:-34}" "${RATIO:-10}" <<'PY'
import re, sys
out, cold, warm, ratio = sys.argv[1], float(sys.argv[2]), float(sys.argv[3]), float(sys.argv[4])
def read(name):
	values = {}
	for line in open("%s/%s" % (out, name)):
		match = re.match(r"FFAULT (.+?)\s+pages=\d+ total_ms=\d+ us_per_page=([0-9.]+)", line)
		if match:
			values[match.group(1).strip()] = float(match.group(2))
	return values
first, second = read("ffault-1.txt"), read("ffault-2.txt")
status = 0
for label, values, limit in (("cold", first, cold), ("warm", second, warm)):
	value = values.get("read fault")
	if value is None or value > limit:
		print("%s read fault: %s us/page (want <= %g) FAIL" % (label, value, limit)); status = 1
	else:
		print("%s read fault: %.2f us/page (<= %g) ok" % (label, value, limit))
fault, plain = second.get("read fault"), second.get("read() 1 MiB")
if fault is None or plain is None or plain <= 0 or fault > plain * ratio:
	print("warm fault / read(): %s / %s (want <= %gx) FAIL" % (fault, plain, ratio)); status = 1
else:
	print("warm fault / read(): %.1fx (<= %gx) ok" % (fault / plain, ratio))
sys.exit(status)
PY
[ $? -eq 0 ] && { echo "bug027: PASS"; exit 0; }
echo "bug027: FAIL"
exit 1
