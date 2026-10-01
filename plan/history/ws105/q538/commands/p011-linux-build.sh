#!/bin/bash
set -euo pipefail
O=$PWD/build/ws105-p011
make keiland-linux-clean > "$O/gcc-clean.log" 2>&1
make -j64 keiland-linux > "$O/gcc-build.log" 2>&1
make keiland-linux-clean CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang > "$O/clang-clean.log" 2>&1
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang > "$O/clang-build.log" 2>&1
make keiland-linux-install DESTDIR="$PWD/build/keiland-linux/stage" > "$O/gcc-install.log" 2>&1
make keiland-linux-install-session DESTDIR="$PWD/build/keiland-linux/stage" >> "$O/gcc-install.log" 2>&1
make keiland-linux-install CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang DESTDIR="$PWD/build/keiland-linux-clang/stage" > "$O/clang-install.log" 2>&1
for b in keiland-linux keiland-linux-clang; do
 sh plan/tools/keiland-linux/elf-check.sh build/$b/stage > "$O/$b-elf.log"
 if test "$b" = keiland-linux; then C=cc; else C=clang; fi
 CC=$C KEILAND_LINUX_BUILD=build/$b sh plan/tools/keiland-linux/header-check.sh > "$O/$b-headers.log" 2>&1
done
sh plan/tools/keiland-linux/makefile-sync.sh > "$O/source-sync.log"
export KEILAND_DRM_DEVICE=none
unset DISPLAY WAYLAND_DISPLAY
timeout 150 sh plan/tools/keiland-linux/interpose-check.sh > "$O/interpose.log" 2>&1
timeout 150 bash plan/tools/keiland-linux/wsi-check.sh > "$O/wsi.log" 2>&1
mkdir -p "$O/empty-xdg"; chmod 700 "$O/empty-xdg"
timeout 60 env XDG_RUNTIME_DIR="$O/empty-xdg" LD_LIBRARY_PATH="$PWD/build/keiland-linux/stage/opt/keiland/lib" vulkaninfo --summary > "$O/vulkaninfo.log" 2>&1
for mode in ordinary sanitize; do
 OPT=(); if test "$mode" = sanitize; then OPT=(-fsanitize=address,undefined -fno-omit-frame-pointer); fi
 clang -D_GNU_SOURCE -std=gnu17 -Wall -Wextra -Werror -g "${OPT[@]}" -I. -Iuserland/desktop/keiland -ffunction-sections -fdata-sections plan/tools/keiland-linux/dbus-wire.c userland/desktop/wayland/linux/dbus-linux.c -Wl,--gc-sections -o "$O/dbus-wire-$mode"
 timeout 60 python3 plan/tools/keiland-linux/dbus-wire.py "$O/dbus-wire-$mode" > "$O/dbus-wire-$mode.log" 2>&1
done
for p in display-probe vk-chain-test; do
 cc -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/stage/opt/keiland/bin/$p plan/tools/keiland-linux/$p.c -ldl -Wl,--no-as-needed -Lbuild/keiland-linux/lib -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
done
for p in network-probe audio-probe; do
 cc -D_GNU_SOURCE -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/stage/opt/keiland/bin/$p plan/tools/keiland-linux/$p.c -Iuserland/desktop/keiland -Lbuild/keiland-linux/lib -l:libkeiland.so -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
done
cc -D_GNU_SOURCE -std=gnu17 -Wall -Wextra -Werror -I. -Iuserland/desktop/keiland -o build/keiland-linux/stage/opt/keiland/bin/dmabuf-forge plan/tools/keiland-linux/dmabuf-forge.c -Lbuild/keiland-linux/lib -l:libwayland-client.so -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
cc -D_GNU_SOURCE -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/stage/opt/keiland/bin/seat-fd plan/tools/keiland-linux/seat-fd.c -Lbuild/keiland-linux/lib -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
python3 - "$O" <<'PY'
from pathlib import Path
import sys
r=Path(sys.argv[1]);assert not any('warning:' in (r/(c+'-build.log')).read_text() for c in ['gcc','clang'])
for c in ['keiland-linux','keiland-linux-clang']:assert 'PASS (24 ELF)' in (r/(c+'-elf.log')).read_text()
assert 'wsi-check: PASS' in (r/'wsi.log').read_text()
print('linux-build-host: PASS gcc/clang warning0 / ELF24 each / headers / source / interpose / WSI / vulkaninfo / D-Bus5x2')
PY
