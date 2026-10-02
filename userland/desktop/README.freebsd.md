# Keiland on FreeBSD 15

The native port uses FreeBSD libc, system Vulkan/Mesa, and seatd. The compositor,
renderers, Wayland client and UI libraries use the same shared implementations as
Linux. No Linuxulator is required. The initial build targets FreeBSD 15 amd64.

## Build and install

The verified environment is FreeBSD 15.1-RELEASE-p4 amd64, base Clang 19.1.7,
gmake 4.4.1, Python 3.11.16, Meson 1.10.2, Ninja 1.13.2, Vulkan loader/headers
1.4.356, libdrm 2.4.133, Mesa 26.1.3 and seatd 0.9.3. Install the native build
and runtime prerequisites on the FreeBSD machine:

```sh
pkg install gmake python311 meson ninja vulkan-headers vulkan-loader libdrm mesa-dri seatd
```

From the repository root, build and review a staged installation:

```sh
gmake -j16 -f userland/desktop/keiland-freebsd.mk all CC=cc
gmake -j16 -f userland/desktop/keiland-freebsd.mk install CC=cc DESTDIR=/tmp/keiland-stage
```

Install the reviewed build as root:

```sh
gmake -f userland/desktop/keiland-freebsd.mk install CC=cc
```

The default prefix is `/opt/keiland`; its private RUNPATH supplies the libraries.
No global `LD_LIBRARY_PATH`, standard library replacement or Python alias is
needed. `KEILAND_PREFIX`, `KEILAND_FREEBSD_BUILD`, `KEILAND_FREEBSD_LOCALBASE` and
`KEILAND_FREEBSD_PYTHON` can select another prefix/build/localbase/interpreter.
Use the same selections for build and install. The default interpreter is
`python3.11`; this must match the installed native Meson/Python tools.

The build independently fetches and verifies the pinned MIT seatd client source,
Noto emoji font and Japanese dictionary. The private seat client enables only
the seatd backend. The system seatd daemon supplies device authority; Keiland
does not use the system libseat client, whose installed dependencies may differ.
Private compatibility headers remain in the build directory. Public Keiland,
Wayland-client, UI, PDF and seat client headers are installed under the prefix.
The model/textures, fonts/licenses, dictionary, App Home and generated wallpapers
are installed with the applications. The default wallpaper is generated Aurora;
`KEILAND_FREEBSD_WALLPAPER` selects a different existing picture at build time.

## Seat and graphics

Install and load the FreeBSD drm-kmod driver appropriate to the actual GPU using
that machine's normal FreeBSD driver configuration. Keiland uses standard Vulkan
and its display extensions; a usable DRM primary node and supported Vulkan
physical device are needed for a real screen. Software Vulkan can verify command
execution but does not supply the physical display, input or dma-buf fence gates.

The installed seatd service defaults to the `video` group. Configure the intended
session user for that group, then enable/start the native authority as root:

```sh
pw groupmod video -m SESSION_USER
sysrc seatd_enable=YES
service seatd start
```

Start Keiland from a local VT after the user has a new login with this membership.
The daemon's normal VT-bound configuration supplies native VT activation and
withdrawal. Each device has a separate kernel fd and daemon lease; pause closes
input/output and primary ownership before ACK, resume acquires fresh leases.
Daemon loss ends the compositor. No direct-root device-open fallback exists.
Do not change evdev permissions to make the compositor run.

## Launch

As the session user, prepare an owned runtime directory and launch from the VT:

```sh
mkdir -p "$HOME/.cache/keiland-runtime"
chmod 700 "$HOME/.cache/keiland-runtime"
export XDG_RUNTIME_DIR="$HOME/.cache/keiland-runtime"
export WAYLAND_DISPLAY=wayland-keiland
export KEILAND_SEAT=seatd
export KEILAND_DRM_DEVICE=/dev/dri/card0
/opt/keiland/bin/wayland
```

Select the actual DRM primary node when it differs from `/dev/dri/card0`.
A standard Vulkan loader's normal device/ICD configuration selects the GPU;
`VK_DRIVER_FILES` may explicitly select an installed ICD when needed. The primary
must correspond to that physical device. App Home uses the installed native
`/opt/keiland/etc/keiland/apps.conf`. Terminal uses native `forkpty`/libutil;
Files obtains a kernel mount snapshot and translates user/system extended
attributes through native extattr. Unknown Linux attribute namespaces and
unsupported atomic flags are refused rather than emulated. Audio controls use
OSS `/dev/mixer*`; networking uses actual FreeBSD interfaces/net80211 and the
existing WPA control protocol. A real WPA service and supported radio are needed
for scan/connect/disconnect, and user access must follow that service's policy.

## Verification state and QEMU Venus gate

Native full build/DESTDIR, installed ELF/public ABI, real 1 MiB software Vulkan
fill/copy, CPU UI/PDF, UFS attributes/mounts, PTY, OSS controls, wired state,
native evdev leases and real VT callbacks were verified on the dedicated guest.
The initial guest configuration has no usable DRM card or WiFi radio.

The user waived real-machine tests on 2026-10-02 and selected actual Venus usage
in FreeBSD QEMU as the graphics acceptance condition. This requires a compatible
host/virglrenderer/QEMU configuration and native guest kernel/Mesa Venus path.
Software lavapipe, host-only Venus capability or another OS's guest results do
not prove that condition. Native WiFi ABI/refusal and WPA wire tests remain
classified separately from the waived physical-radio operations. Final source
conformance and Linux/zedBSD/native regression still apply. See WS109's current
Queue/evidence for the bounded Venus capability investigation and final result.
