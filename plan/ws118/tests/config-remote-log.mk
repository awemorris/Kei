# ws118-p001: the remote-log images for the Latitude 5320 (variants A and B).
# The demonstration configuration, which is the CI amd64 configuration with
# the demonstration's few differences (plan/ws075/demo/config-demo-hdmi.mk:
# OpenSSH, the i915 driver and its firmware, display=edp), plus what the
# remote log needs.  Built by plan/ws118/tests/build-remote-log-image.sh,
# which sets the REMOTE_LOG_* variables below.
include plan/ws075/demo/config-demo-hdmi.mk

# REMOTE_LOG_NO_AUTOLOGIN=y: no account is logged in by itself at boot (B and
# C), so a display that fails does not also start a session.
ifeq ($(strip $(REMOTE_LOG_NO_AUTOLOGIN)),y)
ZEDBSD_EXTRA_INPUTS += plan/ws118/tests/empty-autologin
ZEDBSD_EXTRA_FILES += --file /etc/keiland/autologin=plan/ws118/tests/empty-autologin
endif

# REMOTE_LOG_NETCONF=FILE: a /etc/net.conf with a fixed address for ue0 (the
# USB LAN), written by build-remote-log-image.sh.  Without it the image keeps
# the default net.conf, and networkd gives every unnamed wired interface DHCP.
ifneq ($(strip $(REMOTE_LOG_NETCONF)),)
ZEDBSD_EXTRA_INPUTS += $(REMOTE_LOG_NETCONF)
ZEDBSD_EXTRA_FILES += --file /etc/net.conf=$(REMOTE_LOG_NETCONF)
endif

# REMOTE_LOG_LEAN=y: without clang, libcxx and remacs, the packages that need
# the shared toolchain work trees (an agent's worktree cannot build them).
# The images handed to the user are built without it.
ifeq ($(strip $(REMOTE_LOG_LEAN)),y)
ZEDBSD_USER_PROGRAMS := $(filter-out clang libcxx remacs,$(ZEDBSD_USER_PROGRAMS))
endif
