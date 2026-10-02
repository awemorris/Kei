# ws004-p051 (BUG-134): the base amd64 image of ws045-p008 with the AX211
# driver and its separately licensed firmware package, for the bounded VFIO
# passthrough run on the exact 5330 device and for the boot test without it.
include plan/ws045/tests/config-amd64-base.mk
ZEDBSD_USER_PROGRAMS += intelax211-firmware
