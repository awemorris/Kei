# Kei / zedBSD

Kei is an operating system for computers with touch displays. It is
built on zedBSD, a BSD-based kernel and base system written from
scratch, and ships with Keiland, a Wayland desktop made for touch.

The aim is a commercial UNIX in the line of macOS and iOS, Solaris,
and AIX: an operating system owned end to end, then used to ship
computers that change how the machine is operated. zedBSD is written
to conform to POSIX.1-2024 and to the Single UNIX Specification,
Version 4 (SUSv4). It is not yet a certified UNIX system. Conformance
will keep being raised, and UNIX certification from The Open Group is
a goal. UNIX is a registered trademark of The Open Group.

Getting there means not being bound to an existing kernel or userland
when the whole machine has to move together. Most of the system is
reimplemented. Keiland is a Wayland compositor, and it adds extensions
that existing compositors do not have, so the display, input, and
applications can behave as one machine rather than as a set of loosely
coupled clients. The same reason applies to the GPU stack on zedBSD: a
native Vulkan path, not Linux DRM/KMS or Mesa. The desktop is not
locked to that kernel. Keiland is also ported to Linux and FreeBSD.

The kernel, drivers, libc, and desktop are developed so that hardware
and software can ship as one product: tablets, phones, and PCs
designed by the same person who directs the OS. Everything that runs
on open hardware stays free to use. Features that need the project's
own hardware are still published as source, and only run on that
hardware.

Both zedBSD and Kei are designed and directed by one developer and
implemented with AI coding agents. Current targets are 64-bit x86 PCs
and the Raspberry Pi series.

## Try Kei in 60 seconds

You do not need to compile from scratch to boot Kei. Pre-built test
environments are published for Windows and Linux hosts.

### Windows (VM)

The release archive bundles a custom-patched QEMU build with Windows
Vulkan/GL passthrough.

1. Download the latest `Kei-nightly.zip` from the Releases page of this repository.
2. Extract the archive.
3. Double-click `boot.bat`.

Kei boots into the touch desktop inside QEMU, with hardware graphics
acceleration enabled.

### Linux (VM)

With QEMU, KVM, and VirGL/Venus available, run:

```sh
qemu-system-x86_64 \
  -machine q35,accel=kvm \
  -cpu host \
  -smp 4 \
  -m 8G \
  -drive if=pflash,format=raw,readonly=on,file=data/edk2-x86_64-code.fd \
  -drive if=pflash,format=raw,file=data/ovmf-vars.fd \
  -device qemu-xhci,id=xhci \
  -drive if=none,id=boot,file=data/hdd-image.img,format=raw \
  -device nvme,serial=kei-boot,drive=boot,bootindex=1 \
  -device virtio-vga-gl,blob=on,hostmem=256M,venus=on \
  -display sdl,gl=on \
  -device usb-multitouch,bus=xhci.0,port=1 \
  -device usb-net,bus=xhci.0,port=2,netdev=net0,msos-desc=on \
  -device usb-kbd,bus=xhci.0,port=3 \
  -device usb-tablet,bus=xhci.0,port=4 \
  -netdev "user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22" \
  -serial stdio
```

### Linux portion of Keiland Desktop

```sh
git clone https://github.com/awemorris/zedBSD.git
cd zedBSD
make keiland
make keiland-install
```

Or, one of:

```
sudo dpkg -i keiland-debian13.deb
sudo dpkg -i keiland-ubuntu2604.deb
sudo tar xzf keiland-linux.tar.gz -C /
```

Then restart your display manager such as GDM.

### FreeBSD portion of Keiland Desktop

```sh
git clone https://github.com/awemorris/zedBSD.git
cd zedBSD
make keiland
make keiland-install
```

Or,

```
sudo pkg install keiland-freebsd15.tar.gz -C /
```

Then, run:

```
/opt/keiland/bin/wayland
```

---

## Open hardware and project hardware

Code that runs on generally-sold hardware (PCs, Raspberry Pi series,
and other SBCs) is free for anyone to build and use. That includes
commercial use, modification, and redistribution under the [zlib
License](LICENSE). Buying project hardware is not required to use that
part of the system, and that split is meant to stay.

Some features can only be realized on hardware designed by this
project. Those features are still published as source, under the same
license. They are not a closed edition. They depend on that hardware,
so they do not run on a generic PC or Raspberry Pi. The sold product
is the computer, not a paid OS license.

Kei, Keiland, and zedBSD stay under zlib as the long-term
license. Improvements to the open-hardware system stay freely usable.

---

## Why a new stack

Two objections come up: why not use the existing open-source
operating-system ecosystem, and whether a Wayland compositor written
here is freeloading on that work.

The first reason is the license of the operating system itself. A
distribution assembled from the usual kernel, libc, desktop, and
applications carries a large set of licenses. Tracking that set is a
core job of a project such as Debian. zedBSD, the base userland, the
libraries, Keiland, and the applications written for this system are
all under the zlib License, so the operating system has one
license. Optional third-party packages under `/usr` can still carry
their own licenses; they are not part of that single-license base.

The second reason is who can change the stack. On Linux, the kernel,
libc, desktop, and applications are developed by separate
organizations. A change that needs all of those layers is slow, and
sometimes impossible, to land as one piece of work. Here those layers
are under one direction. A workload can be tuned by changing the
kernel, the library, the compositor, and the application together.

Keiland implements the Wayland specification and adds extensions that
existing compositors do not have. The specification is the
interface. The compositor, its extensions, and the clients are written
for this system so the display stack can move with the rest of the
OS. That is a reimplementation, not a repackage of an existing
compositor.

The new kernel is not a wall. Keiland has been ported so the desktop
also runs on Linux and FreeBSD. The work is published under zlib for
that use: take it, ship it, and build on it. The point of the
open-hardware system is open innovation, not a requirement that every
user boot zedBSD.

---

## What you get

- **zedBSD** — kernel, HAL, drivers, libc, and base userland.

- **Kei** — the zedBSD-based operating system image.

- **Keiland** Wayland compositor and client applications for
    touch. Extensions beyond existing compositors are added so input,
    display, and applications work as one system. The desktop also
    runs on Linux and FreeBSD.

- **Native Vulkan** — a GPU stack that does not sit on Linux DRM/KMS
    or Mesa, and does not use user-space drivers. Intel iGPU (Xe-LP)
    is supported today. NVIDIA and AMD are planned.

- **Two machine classes** — modern 64-bit x86 and Raspberry Pi, plus
    partial retro targets used to show compatibility.

---

## Status

Kei runs on modern computers. Supported development targets:

| Target                      | Role                                          |
|-----------------------------|-----------------------------------------------|
| amd64 (64-bit x86 PC)       | Primary. Default image is GPT + UEFI + NVMe.  |
| Raspberry Pi series         | Primary ARM target.                           |

The zedBSD kernel supports some retro computers.

| Target                      | Role                                          |
|-----------------------------|-----------------------------------------------|
| NEC PC-9800 (i386)          | Partial, compatibility demonstration.         |
| IBM PC/AT (i386)            | Partial, compatibility demonstration.         |
| sun4u (sparcv9)             | Partial, compatibility demonstration.         |
| Sharp X68000 (m68k)         | Partial, compatibility demonstration.         |

---

## Design

The kernel sits on a hardware abstraction layer. Platform-neutral
kernel code stays portable across machines that do not share a CPU,
bus, or boot path.

```
+----------------------------------------------------------------+
| Packages (/usr)                                                |
+----------------------------------------------------------------+
| Wayland desktop (/opt/keiland/)                                |
+----------------------------------------------------------------+
| Base programs (/bin, /lib)                                     |
+----------------------------------------------------------------+
| Base services (/sbin/networkd, /sbin/audiod)                   |
+----------------------------------------------------------------+
| Modern init (/sbin/init)                                       |
+----------------------------------------------------------------+
| Drivers (PCI, USB, GPU, disk, ethernet, wifi, filesystem, ...) |
+----------------------------------------------------------------+
| Kernel (platform-neutral)                                      |
+----------------------------------------------------------------+
| HAL (CPU + BSP)                                                |
+----------------------------------------------------------------+
```

---

## GPU

GPU support is a native Vulkan stack. It does not require Linux
DRM/KMS or Mesa, and it does not use user-space drivers.

Intel Xe-LP is the current driver. NVIDIA and AMD support are planned.

---

## Building

Host prerequisites, configuration, image layout, and the QEMU
procedure are in the build-from-source guide.

On amd64 the default disk image is the native layout: a GPT disk whose
ESP holds the UEFI loader and the kernel, a read-write UFS root
partition, and a swap partition. `make run` starts QEMU with OVMF and
an NVMe disk. Hybrid (UEFI and BIOS), UEFI-only, and BIOS-only layouts
can be selected in `make menuconfig`.

```sh
make                   # same as disk-image
make menuconfig        # write config.mk
make disk-image        # build a disk image
make world             # build vmunix and rootfs
make rootfs            # build rootfs
make vmunix            # build the vmunix kernel
make run               # build a disk image and start QEMU
make toolchain-cache   # install the pinned rev-0 LLVM cache (x86_64 Linux)
make toolchain         # build a toolchain
make help              # short command summary
```

---

## Tree

| Directory            | Description                                      |
|----------------------|--------------------------------------------------|
| `include/`           | Public HAL, kernel, and user ABI interfaces      |
| `src/hal/`           | Architecture HALs and board support              |
| `src/kern/`          | Platform-neutral kernel                          |
| `src/drivers/`       | Device and bus drivers                           |
| `src/libc/`          | zedBSD libc                                      |
| `userland/`          | Userland programs                                |
| `userland/base/`     | Base programs and libraries (`/bin`, `/lib`)     |
| `userland/comp/`     | Compilers                                        |
| `userland/desktop/`  | Keiland programs                                 |
| `userland/firmware/` | Optional per-device firmware packages            |
| `userland/packages/` | Third-party packages (`/usr`)                    |
| `platform/`          | Target Makefiles and tools                       |
| `vendor/`            | External programs                                |
| `tools/`             | Development scripts                              |
| `tests/`             | Tests                                            |

---

## License

Kei, Keiland, and zedBSD are distributed under the zlib License. See
[LICENSE](LICENSE). The license is not a placeholder for a later
proprietary release. The system is meant to stay published under zlib,
including features that only run on project hardware. What runs on
open hardware PCs, Raspberry Pi, and the other public targets stays
free for anyone to use while conformance work, including a future UNIX
certification, continues.
