# q558 / verified native seat client partial result

Client/package/real IPC subset cleared; wholep003/F3 remains uncleared. Nativecompositor seat callbacks,
VT switching/restoration and actual GPU/display/positiveDMA acceptance remain subsequent work.
User authorized continuing after the previous turn; owned guest was already stopped. Restarted same
owned disk/config; SSH loopback47969 and QMP loginPNG verified. Idle between turns is not investigation.
This resumed attempt took less than60min actual execution. No serial/console log interpretation.

## Source and license boundary

Official seatd0.9.3 archive https://git.sr.ht/~kennylevinsen/seatd/archive/0.9.3.tar.gz,
SHA256302564d54d8e28191fadfd734f2675ecb0c9e0615a58011b89ef15dfa4dbaa96 /42086bytes,
verified against https://raw.githubusercontent.com/freebsd/freebsd-ports/main/sysutils/seatd/distinfo.
MIT originalLICENSE included unchanged in /opt/keiland/share/licenses/libseat/LICENSE. External
implementation extracted only under ignored native build/external; no source imported into own
base/desktop tree. Independent userland/packages/libseat/Makefile.freebsd verifies hash before
extract/build; native fetch recipe verified separately. This is p001's existing permissive client
alternative, not an expansion of the drm-kmod exception. Installed systemlibseat's LGPLbasu remains
excluded from Keiland linkage.

[Native build](seat-client-build.txt): FreeBSD15.1-p4/baseClang19.1.7, gmake4.4.1 -j16,
Meson1.10.2_1,1/Ninja1.13.2,4 installed only in owned guest. Exact Meson options in recipe:
logind/builtin/server/examples/manpages disabled, seatd enabled, defaultpath=/var/run/seatd.sock,
default_library=shared, prefix=/opt/keiland/libdir=lib/werror=true. Actual libseat.so.1 SONAME,
native libc.so.7 only; no basu/logind dependency. Private runtime/header/originallicense staged
under /root/keiland-stage-seat/opt/keiland. [Audit/fetch](seat-client-audit.txt).

## Real native authorization and lifetime

[Native client probe](seat-client-test.txt) compiled against the actual staged publicheader and
private libseat.so.1. Installed FreeBSDports seatd0.9.3_1 daemon -u nobody -n readinessfd -l error,
normal headless SEATD_VTBOUND=0, real default /var/run/seatd.sock asserted absent before startup.
No dummy protocolpeer. Clientuid/gid65534/no supplementarygroups; direct /dev/input/event1 read
EACCES but actual servicelease transfers kernel evdevfd. Native EVIOCGNAME/ID/BIT queries report
System mouse, bus6, eventbits07. ReceivedFD_CLOEXEC checked. Client explicitly closes kernel fd,
then daemondeviceID, then seat; own daemon terminatedexit0, ownsocketgone, inputmodeunchanged.
LIBSEAT_BACKEND=seatd is supported clientbackendselection and prevents the upstream noop backend;
no production test-onlyswitch. Callbackstate/activation bounded, actual childtimeout10s/readiness5s.
No VT mode mutation/defaultVT acceptance claim from this headless authorization test. Physical
VT/input/DRMlease withdrawal/reactivation/display shutdown must be verified after implementation.

NewC probe reviewed against fullC rules: declarations/public/staticorder/callbackstate lifetime,
fallible ioctl and service returns, separate descriptor/protocol ownership, semantic paragraph
comments/success exits. clang-format19.1.7 applied with mandated comments/argument layout retained;
style-check0, native finalprobe repeated PASS, diff-check PASS. External vendorimplementation retains
its own style/notice under the package boundary. No make check/hostinstallation/toolchain mutation/
sharedbuild deletion/push/remotepublication. Final fullWS source/threeOS/physical gates retained.
Local records/outbox pending publication.
