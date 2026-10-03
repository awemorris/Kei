# ws134-p002: the lean Files image (plan/tools/files/config-amd64-files.mk: zdesktop, the fonts, the guest harness) with
# the System Monitor.  Build with plan/ws134/tests/build-monitor-image.sh BUILD.
include plan/tools/files/config-amd64-files.mk
ZEDBSD_USER_PROGRAMS += monitor
