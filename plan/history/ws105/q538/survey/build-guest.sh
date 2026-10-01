#!/bin/bash
# build-guest.sh [OUT] [VARIANT] -- the Debian 13 guest image for the WS105 Linux tests (sketch, verified 2026-10-01).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -euo pipefail
export LC_ALL=C.UTF-8                 # the host's ja_JP locale is not generated; perl (mmdebstrap) warns otherwise
export PATH="$PATH:/usr/sbin:/sbin"   # mkfs.ext4 is in /usr/sbin, not in a user's PATH

OUT=${1:-build/keiland-linux/guest}
VARIANT=${2:-base}
FORCE=${FORCE:-0}
MIRROR=${MIRROR:-http://deb.debian.org/debian}
SIZE=${SIZE:-8G}

PACKAGES=${PACKAGES:-linux-image-amd64,systemd-sysv,udev,dbus,libpam-systemd,openssh-server,sudo,kmod,iproute2,mesa-vulkan-drivers,libvulkan1,vulkan-tools,weston,wpasupplicant,hostapd,iw,alsa-utils,kbd,rsync}
case $VARIANT in
base) ;;
gdm) PACKAGES="$PACKAGES,gdm3" ;;
*) echo "build-guest.sh: unknown variant $VARIANT" >&2; exit 2 ;;
esac

if [ -e "$OUT/guest.img" ] && [ "$FORCE" != 1 ]; then
	echo "build-guest.sh: $OUT/guest.img exists (FORCE=1 rebuilds)"
	exit 0
fi
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
[ -e "$OUT/id_ed25519" ] || ssh-keygen -q -t ed25519 -N '' -C keiland-guest -f "$OUT/id_ed25519"

rm -f "$OUT/rootfs.tar" "$OUT/guest.img.new"
# unshare mode: no sudo; /etc/subuid has awe:100000:65536.  Hooks without a special keyword run under sh in the
# user namespace (as the mapped root) with the chroot at $1; files outside are brought in with "upload".
mmdebstrap --mode=unshare --variant=important --include="$PACKAGES" \
	--customize-hook='echo keiland-guest > "$1/etc/hostname"' \
	--customize-hook='printf "127.0.0.1\tlocalhost\n127.0.1.1\tkeiland-guest\n" > "$1/etc/hosts"' \
	--customize-hook='rm -f "$1/etc/resolv.conf"; echo "nameserver 10.0.2.3" > "$1/etc/resolv.conf"' \
	--customize-hook='echo "/dev/vda / ext4 defaults 0 1" > "$1/etc/fstab"' \
	--customize-hook='mkdir -p "$1/etc/systemd/network" && printf "[Match]\nName=en*\n\n[Network]\nDHCP=yes\n" > "$1/etc/systemd/network/20-wired.network"' \
	--customize-hook='chroot "$1" systemctl enable systemd-networkd.service' \
	--customize-hook='if [ -e "$1/usr/lib/systemd/system/wpa_supplicant.service" ]; then chroot "$1" systemctl disable wpa_supplicant.service; fi' \
	--customize-hook='chroot "$1" passwd --delete root' \
	--customize-hook='chroot "$1" sh -c "getent group netdev >/dev/null || groupadd --system netdev"' \
	--customize-hook='chroot "$1" useradd --create-home --uid 1000 --user-group --groups video,input,audio,render,netdev --shell /bin/bash kei' \
	--customize-hook='echo kei:kei | chroot "$1" chpasswd' \
	--customize-hook='mkdir -p "$1/root/.ssh" "$1/home/kei/.ssh"' \
	--customize-hook="upload $OUT/id_ed25519.pub /root/.ssh/authorized_keys" \
	--customize-hook="upload $OUT/id_ed25519.pub /home/kei/.ssh/authorized_keys" \
	--customize-hook='chmod 700 "$1/root/.ssh" "$1/home/kei/.ssh"; chmod 600 "$1/root/.ssh/authorized_keys" "$1/home/kei/.ssh/authorized_keys"; chroot "$1" chown -R kei:kei /home/kei/.ssh' \
	--customize-hook='mkdir -p "$1/etc/ssh/sshd_config.d" && echo "PermitRootLogin prohibit-password" > "$1/etc/ssh/sshd_config.d/keiland.conf"' \
	--customize-hook="download /vmlinuz $OUT/vmlinuz" \
	--customize-hook="download /initrd.img $OUT/initrd.img" \
	trixie "$OUT/rootfs.tar" "$MIRROR"

# An 8 GiB sparse ext4 image filled from the tarball (mke2fs 1.47.2 -d reads tar through libarchive; no root,
# ownership comes from the tar headers).
truncate -s "$SIZE" "$OUT/guest.img.new"
mkfs.ext4 -q -F -L keiland-root -d "$OUT/rootfs.tar" "$OUT/guest.img.new"
mv "$OUT/guest.img.new" "$OUT/guest.img"
rm -f "$OUT/rootfs.tar"
echo "build-guest.sh: $OUT/guest.img"
