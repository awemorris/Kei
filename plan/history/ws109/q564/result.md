# q564 / native applications and data integration

Queue subset cleared; whole p002 remains uncleared pending final complete F2 audit.
Actual native missing UFS attribute errno=ENOATTR is now exposed by the private header as ENODATA,
so common Files tags takes its existing absence branch without changing the native errno or app code.
Real UFS probe checks this common spelling after actual attribute removal; root/native values,
fd/link/no-follow/ERANGE/list and real unprivileged permissions still PASS.

[Native full selected applications build](native-apps-build.txt): FreeBSD15.1-RELEASE-p4,
Clang19.1.7/gmake4.4.1, gmake -j16 -f userland/desktop/keiland-freebsd.mk all
KEILAND_FREEBSD_BUILD=build/native-ui CC=cc, warning0. All 13 selected app/test applications
plus compositor and real libraries built; five generated gradients/default Aurora, verified native
fetch/SHA256 dictionary and native apps.conf. [Header/DESTDIR inventory](native-compositor-audit.txt)
contains332source memberships/322unique own C. Actual final links use native libc/libutil/pthreads,
private MIT seat client and standard Vulkan frontend; no source algorithm copied from another OS.
Initial fixture upload omitted existing own sha256.h, causing actual build failure;
[retained failure](native-apps-fixture-header-failure.txt). Corrected fixture upload includes this
unchanged own header; no algorithm or native compatibility change was needed.

[Real installed ELF/data/runtime and dictionary tests](native-apps-test.txt): 25 native ELF files
(14 executables including compositor, 11 shared libraries) audited using an exclusively owned temporary
/opt/keiland installation, without LD_LIBRARY_PATH. Every dependency resolves; no basu; private libseat
resolves from /opt/keiland/lib and depends only on libc.so.7. Every project ELF has private RUNPATH;
external seat client needs only base libc and has no private RUNPATH requirement. Initial fixture
incorrectly applied this requirement to external client; [retained fixture error](native-apps-fixture-rpath-failure.txt),
then corrected audit checks the actual libc-only contract. Public native headers exclude private
pty/xattr bridges. App Home commands all exist; model and 13 textures, dictionaries, all four fonts,
font/client licenses, wallpapers/default checked. 12 real GUI/IME applications run their ordinary
connection path against an absent owned Wayland endpoint and refuse exit1; vkdemo malformed CLI exits2.
No successful GUI operation claimed. Fixture HOME/XDG directories and installation are owned and
removed. Existing portable Japanese engine test compiled unchanged with native compiler and actual
installed dictionary:150passed/0failed, no sanitizer claim/nativeengine modification.

[Native mount probe](native-mount-test.txt) compares actual getfsstat snapshot with independent
getmntinfo:3real mounts/root UFS/devfs, path/type/order/end/refusal/independent storage PASS.
Actual Linux table probe compares owned string copies before libc's global getmntent record can be
replaced by independent parser:35real mounts/path/type/order/end/refusal PASS.
[Linux Files -j16 build](linux-files-build.txt) warning0/exit0. makefile-sync/OSboundary PASS;
OSboundary emits existing unrelated top suffix-rule warnings, not native/apps compiler warnings.
New mount module/probes clang-format19.1.7+fullmanual review/style0. Attribute probe17goto
candidates remain allowed single forward common cleanup, other rules0. Retained Places source
full review and final WS standards/threeOS/physical checks remain p005; no relocation exception.
Python bytecompile/diffcheck PASS. No make check/push/publication/toolchain/shared cleanup.
