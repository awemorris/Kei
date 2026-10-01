#!/bin/bash
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Builds isolated fixtures and checks actual image pixels, kernel imports and private CPU waits.
set -euo pipefail
BUILD=${KEILAND_LINUX_BUILD:-build/keiland-linux}
BUILD=$(cd "$BUILD" && pwd)
T=$BUILD/test
STAGE=${STAGE:-$BUILD/stage/opt/keiland/lib}
STAGE=$(cd "$STAGE" && pwd)
mkdir -p "$T/gen"
X=$(pkg-config --variable=pkgdatadir wayland-protocols)/stable/linux-dmabuf/linux-dmabuf-v1.xml
SC=$(pkg-config --variable=wayland_scanner wayland-scanner)
"$SC" server-header "$X" "$T/gen/linux-dmabuf-v1-server-protocol.h"
"$SC" private-code "$X" "$T/gen/linux-dmabuf-v1-protocol.c"
${CC:-cc} -std=c11 -Wall -Wextra -Werror -O2 -I"$T/gen" $(pkg-config --cflags wayland-server) -o "$T/dmabuf-probe" plan/tools/keiland-linux/dmabuf-probe.c "$T/gen/linux-dmabuf-v1-protocol.c" $(pkg-config --libs wayland-server)
${CC:-cc} -std=gnu17 -Wall -Wextra -Werror -o "$T/wsi-probe-client" plan/tools/keiland-linux/wsi-probe-client.c -Iuserland/desktop/keiland -L"$BUILD/lib" -l:libwayland-client.so -l:libvulkan.so.1 -Wl,-rpath-link,"$BUILD/lib" -Wl,-rpath,/opt/keiland/lib
${CC:-cc} -std=gnu17 -Wall -Wextra -Werror -fPIC -shared -o "$T/libsync-observe.so" plan/tools/keiland-linux/sync-unavailable.c -ldl
${CC:-cc} -std=gnu17 -Wall -Wextra -Werror -fPIC -shared -DCOMPAT_TEST_SYNC_UNAVAILABLE=1 -o "$T/libsync-unavailable.so" plan/tools/keiland-linux/sync-unavailable.c -ldl
export XDG_RUNTIME_DIR=$T/xdg
mkdir -p -m 0700 "$XDG_RUNTIME_DIR"
for name in fifo fallback resize mailbox; do
  extra=()
  preload=("LD_PRELOAD=$T/libsync-observe.so")
  case "$name" in
  fallback) preload=("LD_PRELOAD=$T/libsync-unavailable.so");;
  resize) extra=(--resize);;
  mailbox) extra=(--mailbox);;
  esac
  timeout 90 "$T/dmabuf-probe" --socket "$XDG_RUNTIME_DIR/probe-0" --frames 90 --timeout 60 --size-log > "$T/probe-$name.out" 2>&1 &
  pid=$!
  trap 'kill "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true' EXIT
  for retry in {1..100}; do
    test -S "$XDG_RUNTIME_DIR/probe-0" && break
    sleep .05
  done
  if timeout 90 env -u DISPLAY KEILAND_DRM_DEVICE=none WAYLAND_DISPLAY=probe-0 LD_LIBRARY_PATH="$STAGE" "${preload[@]}" "$T/wsi-probe-client" --frames 90 --timeout 60 "${extra[@]}" > "$T/client-$name.out" 2>&1; then
    wait "$pid"
    trap - EXIT
    echo "$name client/probe exit=0"
  else
    code=$?
    cat "$T/client-$name.out"
    tail -8 "$T/probe-$name.out"
    exit "$code"
  fi
 done
python3 - "$T" <<'PY'
import re,sys
from pathlib import Path
T=Path(sys.argv[1]); colors=[0xffff0000,0xff00ff00,0xff0000ff]
for name in ['fifo','fallback','resize','mailbox']:
 rows=re.findall(r'^PROBE frame=(\d+) pixel=(0x\w+) fences=(\d+) waited_ms=(\d+) width=(\d+) height=(\d+)$',(T/f'probe-{name}.out').read_text(),re.M)
 assert len(rows)==90,(name,len(rows))
 for N,p,F,M,w,h in rows:
  n=int(N);assert int(p,16)==colors[n%3],(name,n,p)
  assert int(F)>=1,(name,n,F) # Raw kernel stub counts are retained without normalization.
  size=(320,240)
  if name=='resize' and (n//30)%2:size=(400,300)
  assert (int(w),int(h))==size,(name,n,w,h)
 log=(T/f'client-{name}.out').read_text()
 assert 'wsi-probe-client: PASS' in log
 imports=re.findall(r'^IMPORT_SYNC fd=(\d+) flags=(\d+) result=(-?\d+) errno=(\d+)$',log,re.M)
 waits=re.findall(r'^PRIVATE_WAIT count=(\d+) all=(\d+) result=(-?\d+)$',log,re.M)
 assert all((int(c),int(a),int(r))==(1,1,0) for c,a,r in waits),(name,waits)
 if name=='fallback':
  assert len(imports)==1 and tuple(map(int,imports[0][1:]))==(2,-1,25),(name,imports)
  assert len(waits)==270,(name,len(waits)) # acquire + present-reuse + CPU-before-commit, every frame.
 else:
  assert len(imports)==90 and all(int(f)==2 and int(r)==0 for fd,f,r,e in imports),(name,imports)
  assert len(waits)==180,(name,len(waits)) # acquire + present-reuse, every frame.

 print(f'{name}: PASS 90 frames; every color/extent, kernel import and private wait verified; raw fences retained')
PY
printf 'wsi-check: PASS\n'
