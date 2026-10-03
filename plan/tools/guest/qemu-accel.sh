# The accelerator of the amd64 test guests, in one place (ws129-p011, 2026-10-03 user: every test uses KVM).
# Sourced by the scripts that start qemu-system-x86_64 themselves; plan/tools/guest/guest.py applies the same rule.
#
#   KVM  when /dev/kvm can be read and written:  -accel kvm -cpu host
#   TCG  otherwise, or with QEMU_NO_KVM=1 (the scripts' --no-kvm):  -cpu MODEL (the script's own, default max)
#
#   . plan/tools/guest/qemu-accel.sh
#   qemu_accel_mode               prints kvm or tcg
#   qemu_accel_args [MODEL]       prints the options, to be expanded unquoted: $(qemu_accel_args max)
#                                 MODEL "" keeps QEMU's default CPU (no -cpu) under TCG
#
# Only amd64 guests on an x86-64 host use it: the i386 (PC-98, PC/AT BIOS) and raspi4b guests stay on TCG.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

# Succeeds when the guest is to run under KVM.
qemu_accel_kvm()
{
	case ${QEMU_NO_KVM:-} in
	''|0|no) ;;
	*) return 1 ;;
	esac
	[ -r /dev/kvm ] && [ -w /dev/kvm ]
}

# Prints the accelerator's name.
qemu_accel_mode()
{
	if qemu_accel_kvm; then
		echo kvm
	else
		echo tcg
	fi
}

# Prints the accelerator and CPU options; $1 is the CPU model under TCG.
qemu_accel_args()
{
	if qemu_accel_kvm; then
		echo "-accel kvm -cpu host"
	elif [ -n "${1-max}" ]; then
		echo "-cpu ${1-max}"
	else
		echo
	fi
}
