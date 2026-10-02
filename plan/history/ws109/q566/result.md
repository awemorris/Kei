# q566 final changed-source conformance subset

Scoped item cleared; whole p005 uncleared pending actual FreeBSD QEMU Venus. Physical-machine
GPU/WiFi gates waived by current user's explicit reply, not reported as tested.

## Full standards review

[Final source inventory](source-inventory.md) includes surviving C/headers/build recipes,
OS selectors/membership/test fixtures/docs from baseline681b1566. Authoritative full
plan/coding-style.md, Guardrail and native scoped rules applied, no relocation exception.
Earlier q553..q565 full changed-source reviews retained and final surviving source checked:
ANSI declarations, definitions/static prototypes/order; semantic paragraphs/decisions/loops/
returns; short-circuit/evaluation order; errors/errno; fd ownership/lifetime; native OS/API
boundaries; license/include/link/install/provider membership. Two remaining full-file scopes
wire.c and places.c corrected/reviewed this attempt. Formatter Clang19.1.7 InheritParentConfig
ColumnLimit0 plus required definition/condition shapes; style-check has48 candidates:
46 permitted forward cleanup jumps,2 CMSG_LEN macro false call reports. No other findings.
Formatter alone cannot verify semantics; manual review supplies ownership/comments/order.

Wire closes malformed ancillary rights without retrying recycled descriptor numbers, preserves
protocol failure/accepted fd ownership, public dispatch/order. Existing selected-source Wayland
contract suite normal and ASAN/UBSAN PASS. Old WS014 fixture missing modern source/header paths
failed; corrected only owned fixture with actual20 selected sources and private-header alias.
Native final real SCM_RIGHTS delayed FIFO/CLOEXEC/truncation tests PASS; initial missing fixture
upload retained as failed setup receipt, actual uploaded source retry passed.

Files Favorites checks individual writes and fclose, bounded config paths/mkdir errors. Its real
buffered close failure previously returned success; now returns native error. Real Linux and
FreeBSD RLIMIT_FSIZE tests verify EFBIG instead of false success and normal add/duplicate/order/
remove bytes. Existing Linux Files model checks PASS after registered host membership adds real
mount adapter. No fake providers/production test switch. New fixture bytecompile/manual review.

## Affected verification

FreeBSD15.1-p4 Clang19.1.7/gmake4.4.1: -j16 native-final all/install/header-dependencies PASS,
compiler warning0; header/object/private11shared libraries audit PASS with334memberships,
324unique C,628header tokens. Existing q565 native public25ELF/install/CPUUI/PDF/Vulkan1MiB
checks remain classified; changed wire additionally tested native directly. Actual OSS/network/
seat/VT prior hash-stable receipts retained; no unnecessary broad repeats.
Linux GCC14.2 final fullall -j16 PASS/warning0, registered Files hostmodel +Wayland tests PASS.
zedBSD make -j16 disk-image PASS, final native-imagechecker PASS, boot-test.sh final login PNG
PASS and inspected/shown. Final incremental build has one pre-existing nested make jobserver
warning, no project compiler warnings. Initial image build rebuilt configured external packages:
343 warnings from Noct/OpenSSL/OpenSSH/build infrastructure, classified external rather than
claiming universal warning0. No tracked toolchain/package policy was edited; no unrelated massfix.
OS boundary +Linux makefile sync PASS; git diff --check PASS. No make check or shared cleanup.

[Native operational documentation](/home/awe/zedBSD-claude1/userland/desktop/README.freebsd.md)
records actual versions/dependencies/private prefix/build/install/seat service/local VT launch,
backend limits and updated QEMU Venus acceptance. No hardware request remains.

## Remaining acceptance

Actual FreeBSD guest Venus not yet supplied. Installed native Mesa has intel/lvp/radeon ICDs,
initial guest no DRM card; this is not yet a conclusive unsupported-stack finding. Next bounded
p003 Queue investigates host-QEMU-virglrenderer/native kernel/Mesa chain. Kernel/driver port
remains outside existing scope and requires a concrete human scope decision if necessary.
Full p005 and WS109 remain uncleared/incomplete. WIP only, no push/publication/remote close;
required local decision/Phase/WS events durable in outbox. Foreign changes preserved.
