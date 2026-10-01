# q556 / real native UI libraries partial result

L2 library subset cleared; whole p002/F2 remains uncleared until actual compositor/main-app integration.
Verified q553 L1 +q554/q555 actual service source, no fake libkeiland provider or physical acceptance.
Three native package Makefile.freebsd selections added: actual shared libkeiland sources+FreeBSD
OSS/AF_LINK+sharedWPA, existing shared libkeiui CPU/Vulkan/Wayland/widget implementation, actual libpdf.
Independent FreeBSD top makefile selects these libraries and stages their public keiland.h/keiui.h/pdf.h.
No Linux rules are included, no source algorithms changed, no target libc/headers/compiler used.

Native FreeBSD15.1-p4/amd64/baseClang19.1.7/gmake4.4.1:
`gmake -j16 -f userland/desktop/keiland-freebsd.mk libraries KEILAND_FREEBSD_BUILD=build/native-ui CC=cc`
[fresh build](ui-native-build.txt) exit0/warning0,113 actual C sources (52 foundation+61 added memberships),
ten shared ELF+existing digest archive. `install header-dependencies` with DESTDIR=/root/keiland-stage-ui
exit0, native BSD install/private /opt/keiland. [Native source/header/ldd audit](ui-native-audit.txt):113sources,
3461 native absolute header entries/106 unique native headers; no Linux/zedBSD backend selected or
/usr/include/linux/gnu headers; libc.so.7/libsys.so.7/libthr.so.3, our privately staged Wayland/Vulkan/UI/PDF
and codec libraries; no libbasu/libseat/libasound/systemWayland/EGL/GLES dependency. Native ELF SONAME
and RUNPATH=/opt/keiland/lib in [full staged/public evidence](ui-native-test.txt). Digest archive remains
build-only, matching L1/native/Linux design. Loader backend chain already separately verified q553;
UI-client linking does not claim new physical rendering.

New ui-libraries.c compiled `cc -std=gnu17 -Wall -Wextra -Werror -fPIE -pie` using only installed public
header directory and private staged library link/rpath-link. Native public run PASS: version contracts,
actual OSS86/86/muteoff and vtnet0 wired state through real libkeiland.so; libkeiui clear/fill all64pixels
compared independently against public rectangle geometry; libpdf writes one100x80page, reopens native
serialized file, checks page count/dimensions and independent writer/reader content hashes through
real digest implementation. Owned temporary PDF/path/writer/document/canvas/service resources cleaned.
No mixer or host/device mutation. No compositor window/physical GPU/WiFi claim.

clang-format19.1.7 applied with canonical argument and three-clause lines retained; new probe style-check0
and full manual ownership/error/public/static/paragraph/declaration review. Final public compile/run
repeated after formatting, PASS. Makefile membership/native symlink/private install/source selection
reviewed; gitdiff-check PASS. Audit controller initially expected114 from a mistaken membership count;
actualfresh compiled/source list both113, corrected audit expectation and passed. No production C fix
needed. Linux/zedBSD rules unchanged in this attempt; affected final3OS/fullWS conformance remains p005.
No make check/shared cleanup/hosttoolchain change/push/GitHub publication; local outbox pending.
