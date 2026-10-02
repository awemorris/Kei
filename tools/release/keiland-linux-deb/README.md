# Keiland Debian packages

```sh
make keiland-linux-debian
make keiland-linux-ubuntu2604
```

Both commands boot the pinned amd64 distribution in QEMU, build with its native
compiler and dpkg, then install the deb in a second fresh guest. The checks cover
runtime dependencies, the public Vulkan client, an actual KMS desktop and Terminal
keyboard input, reinstall, upgrade, removal and preservation of user data.

The host needs Python 3.12+, Git, curl, OpenSSH, QEMU (`qemu-system-x86_64` and
`qemu-img`), and xorriso. KVM is used when accessible; otherwise QEMU uses TCG.
`KEILAND_DEB_ACCEL=auto|kvm|tcg` explicitly selects emulator acceleration;
`auto` is the default. CI uses TCG so it also runs without nested virtualization.
The amd64 guests use 8 GiB of RAM. The host's desktop, input devices and `/opt/keiland` are untouched. Connections
use a temporary key and a randomly assigned **127.0.0.1** SSH forwarding port.
The base images are verified against [inputs.json](inputs.json) on every run.
Each emulator writes into its own overlay. SSH and QMP screenshots verify
readiness and behavior; serial output is disabled.

`KEILAND_DEB_BUILD` selects the workspace (default `build/keiland-deb`).
Successful outputs are in `artifacts/debian13/` and `artifacts/ubuntu2604/`:

- `keiland_<version>_amd64.deb`: production desktop, libraries, fonts, dictionaries
  and session registration; test apps and development headers are excluded.
- `.manifest.json`: exact paths, modes and payload hashes.
- `.buildinfo.json`: native OS, compiler, dependencies and installed build packages,
  source archive and guest image hashes. This is a Keiland JSON record, not the
  Debian standard `.buildinfo` format.
- `.sha256`: checksums for the deb and its build/manifest records.
- `.smoke.json`, PNGs and the compositor's application journal: install/runtime
  evidence. No guest console or serial logs are used.

The runtime layout is `/opt/keiland`; system Vulkan/Mesa, libdrm, glibc and seat
services remain distribution packages. Select **Keiland** in a display manager,
or run `/opt/keiland/bin/keiland-desktop` as root from a text console.
The Apps configuration is a dpkg conffile. Installation starts no session, and
removal/purge do not erase files in users' homes.

The original `make keiland-linux` and DESTDIR installation keep their development
and demo contents. Packaging filters only its private staging tree.

Limits: amd64 only; QEMU software Vulkan evidence does not certify physical GPUs,
WiFi hardware, or every display manager. Build and test failures stop the target.
Downloads/SSH/apt/build/boot are bounded, and emulator processes are retired even
when a command fails. Artifacts are published only after runtime checks succeed.
