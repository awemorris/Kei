# Native FreeBSD Keiland regression probes

Retained from WS109's actual FreeBSD15.1/i915 QEMU acceptance. These do not launch a VM,
change host devices or supply fake GPU/service providers. Run only in an explicitly owned,
prepared native guest; compile from the repository root with its native headers/Clang19.
Native build/install instructions: [FreeBSD guide](../../../userland/desktop/README.freebsd.md).
Full C style and [native scope](../../standards/ws109-native.md) apply. Actual acceptance receipts:
[q568](../../history/ws109/q568/result.md), [q569](../../history/ws109/q569/result.md),
[q570](../../history/ws109/q570/result.md), [q572](../../history/ws109/q572/result.md).

## Build/header/ELF audit

After `all install header-dependencies` with the same independent native build and DESTDIR:

```sh
python3.11 plan/tools/keiland-freebsd/native-build-audit.py build/native-final /root/keiland-stage-final/opt/keiland
```

It checks actual selected objects, native/system header families, private library SONAME/NEEDED
and install boundaries. No global loader or standard system headers are replaced.

## Native sync and Vulkan window probes

Compile `sync-rejected.c` with production `libvulkan-compat/freebsd/sync-freebsd.c`;
compile `dmabuf-export-rejected.c` with `wayland/freebsd/sync-freebsd.c`.
Both use real native pipes/closed files and check output/error/borrowed-fd ownership.
`dmabuf-native-flags.c` reuses the registered real Vulkan exporter; link the production native
sync adapter plus staged `-l:libvulkan.so.1 -l:libwayland-client.so`, with `-I.`, the native build
include directory and `/usr/local/include`. Its real GPU zeroaccess query tests BUG-130's bounded
unavailable-ioctl adaptation; this does not resolve the upstream driver bug.
`wsi-window-freebsd.c` is a proper xdg-toplevel client; link the same two staged libraries, then
run `--frames 3 --timeout 5` against the actual native compositor. Require its positive
`wsi-probe-client: PASS` marker, not a tracing wrapper's exit code alone.

## Owned Intel GPU/seat and main-app fixtures

Compile `console-abi-freebsd.c` as `/tmp/ws109-console-abi`; it supplies installed native ioctl
constants to the independent capture/restoration controller. The fixtures expect the actual
Intel ICD, `/dev/dri/card0`, native seatd, and the reviewed staged prefix
`/root/keiland-stage-final/opt/keiland`. They refuse an existing `/opt/keiland` or seatd socket,
create only their own temporary installation/runtime/daemon, and restore captured console state
and input permissions. Run as root only inside that explicitly owned guest; the GUI runs as
nobody with supplementary video, without changing accounts/device permissions.

- `gpu-seat-fixture-freebsd.py`: actual wltest GPU window while VT1→2→1 retires/reacquires native
  leases. Polls actual fd generations with six-second limits. After `/tmp/ws109-lifecycle-ready`
  appears, inject QMP USB pointer/key events before the window ends; positive input/frame/exit
  evidence is required. A readonly DRM inquiry fd may remain while the primary lease is retired.
- `main-apps-gpu-freebsd.py`: seven main native apps and App Home. When
  `/tmp/ws109-app-terminal-ready` appears, send QMP keys `freebsd` followed by Enter. A real shell
  behind Terminal's native PTY must write exactly that text. Text/PNG/PDF fixtures are generated
  in its own runtime. `--only-desktop` checks just App Home's actual authenticated desktop role.
  The fixed desktop token is an ordinary supported compositor configuration; no production
  test-only switch is added. Fixtures and observer logs remain separate from QEMU console logs.

The test controller must inject through the owned QEMU's QMP endpoint; the guest scripts do not
open remote SSH connections or store credentials. The main-app GUI success and actual file-open
markers are individually checked; external hardware/radio tests remain user-waived, not tested.
