# ws004-p051 (BUG-134, Q1 2026-10-02): the image the user boots on the
# Latitude 5330 from USB to isolate the AX211 problem on the bare machine.
# The CI amd64 configuration (graphical boot, i915, HDA, ACPI, AX211 driver,
# AX211 firmware package) unchanged except that the three packages which take
# hours to build in a fresh tree (clang, libcxx, remacs) are left out.  No
# passthrough VBT (I915_TEST_VBT stays n).
include config/ci/config-amd64.mk
ZEDBSD_USER_PROGRAMS := $(filter-out clang libcxx remacs,$(ZEDBSD_USER_PROGRAMS))
