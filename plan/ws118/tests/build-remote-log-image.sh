#!/bin/sh
# ws118-p001: builds a remote-log image for the Latitude 5320.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
#   plan/ws118/tests/build-remote-log-image.sh VARIANT BUILD [ADDRESS/PREFIX GATEWAY [DNS]]
#
# VARIANT
#   A  the demonstration image as it is (i915, graphical boot, display=edp,
#      kei logged in by itself): reproduces the failure the way it happens.
#   B  i915, the kernel's messages on the screen (ZEDBSD_GRAPHICAL_BOOT=n with
#      the boot lines "display=edp login=graphical"; login=graphical is not
#      repeated, plan/tools/hw5330/README.md 3.4), no automatic login.
#   C  no i915 driver (the firmware's framebuffer only), the kernel's messages
#      on the screen, no automatic login: SSH stays reachable when the i915
#      stops the boot.
#   D  i915 in the kernel but held at boot (ws118-p005): the boot lines
#      "display=edp login=graphical i915.start=manual i915.debug=display" with
#      ZEDBSD_GRAPHICAL_BOOT=n, no automatic login.  The machine comes up on
#      the firmware's framebuffer with the console login and SSH; root then
#      starts the i915 with `sysctl hw.gpu.start=1` and the greeter with
#      `service start greeter`, and reads the detailed display log in dmesg.
#
# Every variant starts sshd and lets root in with plan/tmp/guest/id_ed25519
# (plan/ws075/demo/build-demo-image.sh puts its public half in the image; the
# pair is made with `plan/tools/guest/guest.sh keys` when it is missing).
# The passthrough VBT is never added.
#
# ADDRESS/PREFIX GATEWAY [DNS] give ue0 (the USB LAN) a fixed address in
# /etc/net.conf instead of DHCP; DNS defaults to the gateway.  Without them
# networkd gives ue0 DHCP by itself.
#
# REMOTE_LOG_LEAN=y in the environment leaves out clang, libcxx and remacs
# (for an agent's worktree, which cannot build them); the user's images are
# built without it.
set -eu
cd "$(dirname -- "$0")/../../.."

usage()
{
	echo "usage: $0 A|B|C|D BUILD [ADDRESS/PREFIX GATEWAY [DNS]]" >&2
	exit 2
}

[ $# -eq 2 ] || [ $# -eq 4 ] || [ $# -eq 5 ] || usage
variant=$1
build=$2
shift 2

# The public key root is let in with.
if [ ! -f plan/tmp/guest/id_ed25519.pub ]; then
	plan/tools/guest/guest.sh keys
fi

config=plan/ws118/tests/config-remote-log.mk
case $variant in
A)
	# The demonstration image as it is.
	extra_make=
	;;
B)
	extra_make=B
	;;
C)
	config=plan/ws118/tests/config-remote-log-c.mk
	extra_make=C
	;;
D)
	extra_make=D
	;;
*)
	usage
	;;
esac

# The fixed address, when one is given, as a net.conf in the build directory.
netconf=
if [ $# -ge 2 ]; then
	address=${1%/*}
	prefix=${1#*/}
	gateway=$2
	dns=${3:-$2}
	case $address/$prefix in
	*[!0-9./]*|/*|*/) echo "build-remote-log-image: bad address: $1" >&2; exit 2 ;;
	esac
	mkdir -p "$build"
	netconf=$build/remote-log-net.conf
	cat > "$netconf" <<EOF
# ws118-p001: ue0 (the USB LAN) at a fixed address.
version: 1

interfaces:
  lo0:
    type: loopback
    enabled: true
    ipv4:
      dhcp: false
      addresses:
        - address: 127.0.0.1
          prefix-length: 8
  ue0:
    type: ethernet
    enabled: true
    ipv4:
      dhcp: false
      addresses:
        - address: $address
          prefix-length: $prefix

routes:
  - destination: default
    gateway: $gateway

dns:
  mode: static
  servers:
    - $dns
EOF
fi

# The variant's make arguments, after build-demo-image.sh's own.
case $extra_make in
B)
	exec plan/ws075/demo/build-demo-image.sh "$build" \
		ZEDBSD_CONFIG=$config \
		REMOTE_LOG_NO_AUTOLOGIN=y REMOTE_LOG_NETCONF=$netconf \
		REMOTE_LOG_LEAN=${REMOTE_LOG_LEAN:-n} \
		ZEDBSD_GRAPHICAL_BOOT=n \
		"ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical"
	;;
C)
	exec plan/ws075/demo/build-demo-image.sh "$build" \
		ZEDBSD_CONFIG=$config \
		REMOTE_LOG_NO_AUTOLOGIN=y REMOTE_LOG_NETCONF=$netconf \
		REMOTE_LOG_LEAN=${REMOTE_LOG_LEAN:-n} \
		ZEDBSD_GRAPHICAL_BOOT=n
	;;
D)
	exec plan/ws075/demo/build-demo-image.sh "$build" \
		ZEDBSD_CONFIG=$config \
		REMOTE_LOG_NO_AUTOLOGIN=y REMOTE_LOG_NETCONF=$netconf \
		REMOTE_LOG_LEAN=${REMOTE_LOG_LEAN:-n} \
		ZEDBSD_GRAPHICAL_BOOT=n \
		"ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical i915.start=manual i915.debug=display"
	;;
*)
	exec plan/ws075/demo/build-demo-image.sh "$build" \
		ZEDBSD_CONFIG=$config \
		REMOTE_LOG_NETCONF=$netconf \
		REMOTE_LOG_LEAN=${REMOTE_LOG_LEAN:-n}
	;;
esac
