# ws004-p051 (BUG-134): the 5330 demonstration image (graphical boot, i915,
# the desktop) with the AX211 driver and its firmware package, without the
# external packages that need the shared package work trees (OpenSSH,
# OpenSSL, curl, clang, libcxx, remacs), for the passthrough reproduction of
# the boot stop with the iGPU and the AX211 both given to the guest.
include plan/ws075/demo/config-demo-hdmi.mk
CONFIG_DRIVER_PCI_INTEL_AX211 := y
ZEDBSD_USER_PROGRAMS := $(filter-out openssl openssh curl clang libcxx remacs \
	ca-certificates,$(ZEDBSD_USER_PROGRAMS)) intelax211-firmware
ZEDBSD_NOCT_ACCEL := n
