# q578-i01 / P9 generation 1 checkpoint

Status: in-progress. This is a worker draft; main owns canonical projections.

## Rules and prerequisite outputs

Read AGENTS.md in full, Guardrail, C full standard, automation coverage,
agents/protocol, P9 Queue and approved Phase snapshot, Master/WS099 and
p001/p002. Reviewed actual c10-soak.sh, c5-hw.sh, hdmi-h4-hw.sh, h4-lock.sh,
h4-qemu.sh, h4-ctl.py, h4-png.py and ufs-cat.py before implementation.
No C source, HAL, toolchain, shared build, Board or cache changes.

2026-10-02 04:52 UTC survey: solaris10-man resolves to chaos; SSH worked,
no remote QEMU, GPU 0000:00:02.0 already bound to vfio-pci. Local hardware
lock had no holder, exact H4 owner marker absent. H4's vfio request is
idempotent for that binding; no rebind/reboot or foreign process termination.
H4 uses debugcon only to locate capture buffers; acceptance uses PNGs and
the session/sessiond files read from the stopped guest disk. Console/serial
files fetched mechanically by H4 were not read for any judgment.

## Test runner

First commit 4bdd9224f added c10-hw.sh; main ACK integrated it as 7fc178474.
Commit 21042cf4e adds recoverable round/time/PID checkpoints. Both have
sh -n and git diff --check PASS. Manual shell review covered early refusal,
exact OUTDIR ownership, original QEMU PID checks, bounded interaction commands,
trap cleanup, pipeline exit status, fresh disk receipt, missing evidence and
error/restart refusal. A missing-image invocation returned 2 and created no
output directory. The C standard is authoritative but no new C was generated.

The Phase's menu/drag procedure was extended with an extra Terminal open/exit
close every ten rounds, retaining the original ten apps. This exercises the
already approved C10 window opening/closing criterion. PNG evidence below
shows the extra real Terminal and its removal, rather than inferring closure
from a sent key.

## Old image fixture and external interruption

Source: main/build/ws103/p007f-pt.img, timestamp 2026-10-01 00:57 JST,
copied to own build/ws099-p014/fixture-old.img. SHA256
`1d74f747a6731a1d22e3dc0f361c0958fa1b5c8481cd4a3635f87c5928b8c7f8`.
Its apps.conf has eleven entries; /bin/textedit is absent, so Home displays
the ten expected apps and the established tile coordinates are correct.
This image predates the current source and is only fixture validation.

Command: sh plan/ws099/tests/c10-hw.sh build/ws099-p014/fixture-old.img
build/ws099-p014/c10-short-old 3. QEMU started 04:57:24 UTC, original PID
20441; local exact owner was the named OUTDIR and lock holder PID 163048.
Around 05:03 UTC the execution tool session exited 143 and the foreground
runner disappeared before its final receipt. The remote QEMU remained alive;
this is an incomplete short attempt, not a compositor-failure conclusion.
Old script lacked the newly added atomic checkpoint, so exact final elapsed
operation time/round count is unknown. It is not a short PASS or 60-minute PASS.

Owner and original QEMU PID were read back and matched before recovery.
Opened Terminal, copied /run/user/1000/session.log to /home/kei/c10-hw.log,
synced, captured rescued PNG and stopped only owned QEMU using H4. H4 reported
machine given back. Follow-up confirmed owner absent, lock free and no remote
QEMU. Rescued session has zero ZWL ERROR/FAILED; its Wiseview records report
windows=10. This limited observation does not establish full C10 clearance.

- [Ten apps](evidence/opened-live.png)
- [Extra Terminal open](evidence/window-open-1-live.png)
- [Extra Terminal closed](evidence/window-closed-1-live.png)
- [Selected disk session events](evidence/old-short-events.txt)

All shown images use actual PLANE_SURFLIVE register live-buffer selection.

## Current continuation

Main prepared a fresh full demo from source 5ac9b753d: build/main-n3-demo-pt/
hdd-image.img, SHA256
`72003343313d85b5e0950f5659ca6af5a6183ef3c0776c68a1e7ecdfba0b3e7e`.
Main reported build exit 0, project warnings 0, native-image check PASS;
reused canonical Noct accelerator/stamp without toolchain edits. Worker
read-only copied it to build/ws099-p014/fresh-5ac9b753d.img and verified hash.
/bin/textedit remains absent, preserving the ten visible tiles.

Fresh three-minute run used setsid/nohup to survive tool-session limits,
PID/session ID 188402, OUTDIR build/ws099-p014/c10-short-fresh. It completed:
rounds=14, requested minutes=3, actual operation elapsed_seconds=187, errors=0,
restarts=0, exit.status=0. Inspected its opened/extra-open/extra-closed PNGs and
saved-live.png: ten app windows remain, the extra Terminal closes with exit,
and the final real Terminal shows the log-copy/receipt/sync command followed
by a fresh shell prompt. Session receipt C10_HW_SAVED_1790917793 is present.
Disk session records show Wiseview windows=10 throughout. Exact owner and
hardware lock were given back; remote QEMU absent, GPU still vfio-pci.

[Fresh short receipts and PNGs](evidence/fresh-short/) retain output, full
disk session/sessiond logs, atomic checkpoint, cleanup receipt and four PNGs.
Console/serial logs are excluded. This validates the fixture and runner,
but supplies only 187 seconds of C10 operation.

The approved 60-minute run started detached at 2026-10-02 05:14 UTC:
PID/session ID 203752, OUTDIR build/ws099-p014/c10-full-fresh, same fresh
source/image. At checkpoint it is initializing. Whole-hour receipt/periodic
PNGs and final criteria evaluation remain outstanding. Main's supplementary
framebuffer boot result is separate and pending. Phase remains in-progress;
no clearance claim or GitHub publication.
