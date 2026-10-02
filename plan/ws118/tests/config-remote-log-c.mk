# ws118-p001: remote-log variant C for the Latitude 5320: no i915 driver, so
# the console stays on the firmware's framebuffer and SSH is reachable even
# when the i915 stops the boot.
include plan/ws118/tests/config-remote-log.mk
CONFIG_DRIVER_PCI_I915 := n
